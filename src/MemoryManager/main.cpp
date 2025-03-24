#include "../../include/MemoryManager.h"
#include <iostream>
#include <string>
#include <cstdlib>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " LISTEN_PORT SIZE_MB DUMP_FOLDER" << std::endl;
    std::cout << "  LISTEN_PORT: Port to listen for connections" << std::endl;
    std::cout << "  SIZE_MB: Size of memory pool in megabytes" << std::endl;
    std::cout << "  DUMP_FOLDER: Folder to store memory dumps" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        // Parse command line arguments
        int port = std::stoi(argv[1]);
        size_t sizeMB = std::stoll(argv[2]);
        std::string dumpFolder = argv[3];
        
        // Create memory manager
        MemoryManager memoryManager(port, sizeMB, dumpFolder);
        
        // Start server
        if (!memoryManager.startServer()) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        std::cout << "Memory Manager started. Press Enter to stop..." << std::endl;
        std::cin.get();
        
        // Stop server (will be called by destructor anyway)
        memoryManager.stopServer();
        
        std::cout << "Memory Manager stopped" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}