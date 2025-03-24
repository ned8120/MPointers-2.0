#include "../../include/MemoryManager.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <filesystem>
#include <algorithm> // Añadido para std::sort

// MemoryBlock implementation
MemoryBlock::MemoryBlock(size_t offset, size_t size, const std::string& type)
    : offset(offset), size(size), type(type), refCount(1), inUse(true) {
}

// MemoryManager implementation
MemoryManager::MemoryManager(int port, size_t sizeInMB, const std::string& dumpFolder)
    : memoryPool(nullptr), poolSize(sizeInMB * 1024 * 1024), dumpFolder(dumpFolder),
      nextId(1), port(port), running(false) {
    
    // Create the dump folder if it doesn't exist
    if (!std::filesystem::exists(dumpFolder)) {
        std::filesystem::create_directories(dumpFolder);
    }
    
    // Allocate memory pool (this is the ONLY malloc in the project)
    memoryPool = malloc(poolSize);
    if (!memoryPool) {
        std::cerr << "Failed to allocate memory pool of size " << sizeInMB << "MB" << std::endl;
        exit(1);
    }
    
    std::cout << "Memory pool of " << sizeInMB << "MB allocated at " << memoryPool << std::endl;
}

MemoryManager::~MemoryManager() {
    stopServer();
    
    // Free the memory pool
    if (memoryPool) {
        free(memoryPool);
        memoryPool = nullptr;
    }
}

bool MemoryManager::startServer() {
    running = true;
    
    // Start server thread
    serverThread = std::thread(&MemoryManager::serverLoop, this);
    
    // Start garbage collector thread
    gcThread = std::thread(&MemoryManager::garbageCollector, this);
    
    return true;
}

void MemoryManager::stopServer() {
    running = false;
    
    // Wait for threads to finish
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    if (gcThread.joinable()) {
        gcThread.join();
    }
}

int MemoryManager::create(size_t size, const std::string& type) {
    std::lock_guard<std::mutex> lock(blocksMutex);
    
    // Find free space in the memory pool
    size_t offset = findFreeSpace(size);
    if (offset == std::numeric_limits<size_t>::max()) {
        // Try to defragment and find space again
        defragmentMemory();
        offset = findFreeSpace(size);
        
        if (offset == std::numeric_limits<size_t>::max()) {
            std::cerr << "Failed to allocate " << size << " bytes for type " << type << std::endl;
            return -1;
        }
    }
    
    // Create a new memory block
    int id = nextId++;
    blocks.emplace(id, MemoryBlock(offset, size, type));
    
    // Create memory dump
    createMemoryDump();
    
    return id;
}

bool MemoryManager::set(int id, const void* value, size_t valueSize) {
    std::lock_guard<std::mutex> lock(blocksMutex);
    
    auto it = blocks.find(id);
    if (it == blocks.end() || !it->second.inUse) {
        return false;
    }
    
    // Check if sizes match
    if (valueSize > it->second.size) {
        std::cerr << "Value size " << valueSize << " exceeds block size " << it->second.size << std::endl;
        return false;
    }
    
    // Copy value to memory
    char* dest = static_cast<char*>(memoryPool) + it->second.offset;
    std::memcpy(dest, value, valueSize);
    
    // Create memory dump
    createMemoryDump();
    
    return true;
}

bool MemoryManager::get(int id, void* value, size_t valueSize) {
    std::lock_guard<std::mutex> lock(blocksMutex);
    
    auto it = blocks.find(id);
    if (it == blocks.end() || !it->second.inUse) {
        return false;
    }
    
    // Check if sizes match
    if (valueSize > it->second.size) {
        std::cerr << "Value size " << valueSize << " exceeds block size " << it->second.size << std::endl;
        return false;
    }
    
    // Copy memory to value
    char* src = static_cast<char*>(memoryPool) + it->second.offset;
    std::memcpy(value, src, valueSize);
    
    return true;
}

bool MemoryManager::increaseRefCount(int id) {
    std::lock_guard<std::mutex> lock(blocksMutex);
    
    auto it = blocks.find(id);
    if (it == blocks.end() || !it->second.inUse) {
        return false;
    }
    
    it->second.refCount++;
    return true;
}

bool MemoryManager::decreaseRefCount(int id) {
    std::lock_guard<std::mutex> lock(blocksMutex);
    
    auto it = blocks.find(id);
    if (it == blocks.end() || !it->second.inUse) {
        return false;
    }
    
    it->second.refCount--;
    
    // Create memory dump
    createMemoryDump();
    
    return true;
}

void MemoryManager::serverLoop() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket);
        return;
    }
    
    // Setup server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket to port " << port << std::endl;
        close(serverSocket);
        return;
    }
    
    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return;
    }
    
    std::cout << "Memory Manager listening on port " << port << std::endl;
    
    // Message struct definition (must match the one in MPointer.h)
    struct Message {
        uint8_t type;  // MessageType enum
        int id;
        size_t size;
        char typeStr[32];
        char data[1024];
    };
    
    // Accept and handle connections
    while (running) {
        // Accept a connection (with timeout to check running flag)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);
        
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;
        
        int ready = select(serverSocket + 1, &readSet, nullptr, nullptr, &timeout);
        
        if (ready <= 0) {
            // Timeout or error, check if we should continue running
            continue;
        }
        
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        
        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        // Handle the client
        Message request, response;
        
        // Receive message
        ssize_t bytesRead = recv(clientSocket, &request, sizeof(Message), 0);
        if (bytesRead != sizeof(Message)) {
            std::cerr << "Error reading message" << std::endl;
            close(clientSocket);
            continue;
        }
        
        // Process message
        std::cout << "Received message type: " << (int)request.type << std::endl;
        
        // Initialize response
        memset(&response, 0, sizeof(Message));
        response.type = request.type;
        response.id = request.id;
        
        // Process based on message type
        switch (request.type) {
            case 1:  // CREATE
                response.id = create(request.size, request.typeStr);
                std::cout << "Created block with ID: " << response.id << std::endl;
                break;
                
            case 2:  // SET
                if (set(request.id, request.data, request.size)) {
                    std::cout << "Set value for ID: " << request.id << std::endl;
                } else {
                    std::cerr << "Failed to set value for ID: " << request.id << std::endl;
                    response.id = -1;
                }
                break;
                
            case 3:  // GET
                if (get(request.id, response.data, request.size)) {
                    std::cout << "Got value for ID: " << request.id << std::endl;
                } else {
                    std::cerr << "Failed to get value for ID: " << request.id << std::endl;
                    response.id = -1;
                }
                break;
                
            case 4:  // INCREASE_REF_COUNT
                if (increaseRefCount(request.id)) {
                    std::cout << "Increased ref count for ID: " << request.id << std::endl;
                } else {
                    std::cerr << "Failed to increase ref count for ID: " << request.id << std::endl;
                    response.id = -1;
                }
                break;
                
            case 5:  // DECREASE_REF_COUNT
                if (decreaseRefCount(request.id)) {
                    std::cout << "Decreased ref count for ID: " << request.id << std::endl;
                } else {
                    std::cerr << "Failed to decrease ref count for ID: " << request.id << std::endl;
                    response.id = -1;
                }
                break;
                
            default:
                std::cerr << "Unknown message type: " << (int)request.type << std::endl;
                response.id = -1;
                break;
        }
        
        // Send response
        send(clientSocket, &response, sizeof(Message), 0);
        
        // Close client socket
        close(clientSocket);
    }
    
    // Close server socket
    close(serverSocket);
}

void MemoryManager::garbageCollector() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(blocksMutex);
            
            // Find and free blocks with zero references
            for (auto it = blocks.begin(); it != blocks.end(); ++it) {
                if (it->second.inUse && it->second.refCount <= 0) {
                    std::cout << "Garbage collector freeing block " << it->first << std::endl;
                    it->second.inUse = false;
                }
            }
        }
        
        // Sleep for a while
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void MemoryManager::createMemoryDump() {
    // Create timestamp for filename
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream filename;
    filename << dumpFolder << "/memory_dump_";
    filename << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
    filename << "_" << std::setfill('0') << std::setw(3) << now_ms.count() << ".txt";
    
    // Create dump file
    std::ofstream dumpFile(filename.str());
    if (!dumpFile) {
        std::cerr << "Failed to create memory dump file: " << filename.str() << std::endl;
        return;
    }
    
    // Write header
    dumpFile << "Memory Dump - " << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S") 
             << "." << std::setfill('0') << std::setw(3) << now_ms.count() << std::endl;
    dumpFile << "Total Memory: " << poolSize << " bytes" << std::endl;
    dumpFile << "----------------------------------------" << std::endl;
    
    // Write block information
    dumpFile << "Blocks:" << std::endl;
    for (const auto& pair : blocks) {
        const auto& id = pair.first;
        const auto& block = pair.second;
        
        dumpFile << "ID: " << id 
                 << ", Offset: " << block.offset 
                 << ", Size: " << block.size 
                 << ", Type: " << block.type 
                 << ", RefCount: " << block.refCount 
                 << ", InUse: " << (block.inUse ? "Yes" : "No") << std::endl;
    }
    
    dumpFile.close();
}

size_t MemoryManager::findFreeSpace(size_t size) {
    // Simple first-fit algorithm
    std::vector<std::pair<size_t, size_t>> usedRanges;
    
    // Collect all used memory ranges
    for (const auto& pair : blocks) {
        if (pair.second.inUse) {
            usedRanges.emplace_back(pair.second.offset, pair.second.offset + pair.second.size);
        }
    }
    
    // Sort by offset
    std::sort(usedRanges.begin(), usedRanges.end());
    
    // Check for gaps
    size_t currentOffset = 0;
    for (const auto& range : usedRanges) {
        if (range.first - currentOffset >= size) {
            return currentOffset;
        }
        currentOffset = std::max(currentOffset, range.second);
    }
    
    // Check if there's space at the end
    if (poolSize - currentOffset >= size) {
        return currentOffset;
    }
    
    return std::numeric_limits<size_t>::max();  // No space found
}

void MemoryManager::defragmentMemory() {
    std::cout << "Defragmenting memory..." << std::endl;
    
    // Collect all active blocks
    std::vector<std::pair<int, MemoryBlock*>> activeBlocks;
    for (auto& pair : blocks) {
        if (pair.second.inUse) {
            activeBlocks.emplace_back(pair.first, &pair.second);
        }
    }
    
    // Sort by offset
    std::sort(activeBlocks.begin(), activeBlocks.end(), 
              [](const auto& a, const auto& b) { return a.second->offset < b.second->offset; });
    
    // Compact memory
    size_t currentOffset = 0;
    for (auto& [id, block] : activeBlocks) {
        if (block->offset != currentOffset) {
            // Move data
            char* src = static_cast<char*>(memoryPool) + block->offset;
            char* dest = static_cast<char*>(memoryPool) + currentOffset;
            std::memmove(dest, src, block->size);
            
            // Update offset
            block->offset = currentOffset;
        }
        currentOffset += block->size;
    }
    
    std::cout << "Defragmentation complete. Free space: " << (poolSize - currentOffset) << " bytes" << std::endl;
}