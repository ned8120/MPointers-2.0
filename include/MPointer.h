#ifndef MPOINTER_H
#define MPOINTER_H

#include <string>
#include <memory>
#include <typeinfo>
#include <cstdint>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <atomic>

#include "Node.h" // Include the Node definition

// Forward declarations
template <typename T>
class MPointer;

// Message types for communication with Memory Manager
enum class MessageType : uint8_t {
    CREATE = 1,
    SET = 2,
    GET = 3,
    INCREASE_REF_COUNT = 4,
    DECREASE_REF_COUNT = 5
};

// Structure for messages sent to the Memory Manager
struct Message {
    MessageType type;
    int id;
    size_t size;
    char typeStr[32];  // For storing type name
    char data[1024];   // For data (adjust size as needed)
};

// Client for Memory Manager communication
class MemoryManagerClient {
public:
    static void Init(int port, const std::string& host = "127.0.0.1");
    static void Cleanup();
    
    static int Create(size_t size, const std::string& type);
    static bool Set(int id, const void* value, size_t size);
    static bool Get(int id, void* value, size_t size);
    static bool IncreaseRefCount(int id);
    static bool DecreaseRefCount(int id);
    static bool IsInitialized() { return initialized; }

private:
    static int clientSocket;
    static std::string host;
    static int port;
    static std::atomic<bool> initialized;
    
    static bool sendMessage(const Message& message, Message& response);
};

// MPointer template class
template <typename T>
class MPointer {
public:
    // Default constructor
    MPointer() : id(-1), valuePtr(nullptr) {}
    
    // Destructor
    ~MPointer() {
        if (id != -1 && MemoryManagerClient::IsInitialized()) {
            MemoryManagerClient::DecreaseRefCount(id);
        }
    }
    
    // Copy constructor
    MPointer(const MPointer<T>& other) : id(other.id), valuePtr(nullptr) {
        if (id != -1 && MemoryManagerClient::IsInitialized()) {
            MemoryManagerClient::IncreaseRefCount(id);
        }
    }
    
    // Assignment operator
    MPointer<T>& operator=(const MPointer<T>& other) {
        if (this != &other) {
            if (id != -1 && MemoryManagerClient::IsInitialized()) {
                MemoryManagerClient::DecreaseRefCount(id);
            }
            
            id = other.id;
            valuePtr = nullptr;
            
            if (id != -1 && MemoryManagerClient::IsInitialized()) {
                MemoryManagerClient::IncreaseRefCount(id);
            }
        }
        return *this;
    }
    
    // Static initialization method
    static void Init(int port, const std::string& host = "127.0.0.1") {
        MemoryManagerClient::Init(port, host);
    }
    
    // New method (instead of new operator)
    static MPointer<T> New() {
        MPointer<T> ptr;
        
        // Create a block in the memory manager
        int id = MemoryManagerClient::Create(sizeof(T), typeid(T).name());
        if (id == -1) {
            throw std::runtime_error("Failed to allocate memory in Memory Manager");
        }
        
        // Set the ID
        ptr.id = id;
        
        // Initialize with default constructor value
        T defaultValue = T();
        if (!MemoryManagerClient::Set(id, &defaultValue, sizeof(T))) {
            // If set fails, decrease the reference count and throw
            MemoryManagerClient::DecreaseRefCount(id);
            throw std::runtime_error("Failed to initialize memory in Memory Manager");
        }
        
        return ptr;
    }
    
    // Dereference operator
    T& operator*() {
        if (id == -1) {
            throw std::runtime_error("Attempting to dereference a null MPointer");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        // Get the value from memory manager
        if (!MemoryManagerClient::Get(id, &valueCache, sizeof(T))) {
            throw std::runtime_error("Failed to get value from Memory Manager");
        }
        
        return valueCache;
    }
    
    // Arrow operator
    T* operator->() {
        if (id == -1) {
            throw std::runtime_error("Attempting to use -> on a null MPointer");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        if (!MemoryManagerClient::Get(id, &valueCache, sizeof(T))) {
            throw std::runtime_error("Failed to get value from Memory Manager");
        }
        
        // Store the value locally for the pointer to work
        valuePtr = &valueCache;
        return valuePtr;
    }
    
    // ID access method
    int getId() const {
        return id;
    }
    
    // Assignment operator for values
    T& operator=(const T& value) {
        if (id == -1) {
            throw std::runtime_error("Attempting to assign to a null MPointer");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        if (!MemoryManagerClient::Set(id, &value, sizeof(T))) {
            throw std::runtime_error("Failed to set value in Memory Manager");
        }
        
        // Update local cache
        valueCache = value;
        
        return valueCache;
    }

    // Friend declaration to allow LinkedList implementation to access id directly
    template <typename U>
    friend class LinkedList;
    
    // Friend declaration for Node
    template <typename U>
    friend struct Node;

private:
    int id;              // ID of the block in Memory Manager
    T valueCache;        // Temporary storage for dereferenced value
    T* valuePtr;         // Pointer to valueCache for -> operator
};

// Static members initialization
int MemoryManagerClient::clientSocket = -1;
std::string MemoryManagerClient::host = "127.0.0.1";
int MemoryManagerClient::port = 8080;
std::atomic<bool> MemoryManagerClient::initialized(false);

// MemoryManagerClient implementation
void MemoryManagerClient::Init(int port, const std::string& host) {
    if (initialized) {
        return;
    }
    
    MemoryManagerClient::port = port;
    MemoryManagerClient::host = host;
    initialized = true;
    
    std::cout << "MemoryManagerClient initialized with port " << port << " and host " << host << std::endl;
}

void MemoryManagerClient::Cleanup() {
    if (clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
    }
    initialized = false;
    std::cout << "MemoryManagerClient cleaned up" << std::endl;
}

int MemoryManagerClient::Create(size_t size, const std::string& type) {
    if (!initialized) {
        throw std::runtime_error("MemoryManagerClient not initialized");
    }
    
    Message message;
    message.type = MessageType::CREATE;
    message.id = 0;  // Not used for CREATE
    message.size = size;
    strncpy(message.typeStr, type.c_str(), sizeof(message.typeStr) - 1);
    message.typeStr[sizeof(message.typeStr) - 1] = '\0';
    
    // Clear the data field
    memset(message.data, 0, sizeof(message.data));
    
    Message response;
    if (!sendMessage(message, response)) {
        std::cerr << "Failed to create memory block of size " << size 
                  << " for type " << type << std::endl;
        return -1;
    }
    
    std::cout << "Successfully created memory block with ID: " << response.id << std::endl;
    return response.id;
}

bool MemoryManagerClient::Set(int id, const void* value, size_t size) {
    if (!initialized) {
        throw std::runtime_error("MemoryManagerClient not initialized");
    }
    
    if (id == -1) {
        std::cerr << "Cannot set value for invalid ID (-1)" << std::endl;
        return false;
    }
    
    Message message;
    message.type = MessageType::SET;
    message.id = id;
    message.size = size;
    
    if (size > sizeof(message.data)) {
        std::cerr << "Warning: Data size " << size << " exceeds message buffer size " 
                  << sizeof(message.data) << ". Data will be truncated." << std::endl;
        size = sizeof(message.data);
    }
    
    memcpy(message.data, value, size);
    
    Message response;
    if (!sendMessage(message, response)) {
        std::cerr << "Failed to set value for ID: " << id << std::endl;
        return false;
    }
    
    return response.id != -1;
}

bool MemoryManagerClient::Get(int id, void* value, size_t size) {
    if (!initialized) {
        throw std::runtime_error("MemoryManagerClient not initialized");
    }
    
    if (id == -1) {
        std::cerr << "Cannot get value for invalid ID (-1)" << std::endl;
        return false;
    }
    
    Message message;
    message.type = MessageType::GET;
    message.id = id;
    message.size = size;
    memset(message.data, 0, sizeof(message.data));
    
    Message response;
    if (!sendMessage(message, response)) {
        std::cerr << "Failed to get value for ID: " << id << std::endl;
        return false;
    }
    
    if (size > sizeof(response.data)) {
        std::cerr << "Warning: Data size " << size << " exceeds message buffer size " 
                  << sizeof(response.data) << ". Data will be truncated." << std::endl;
        size = sizeof(response.data);
    }
    
    memcpy(value, response.data, size);
    return true;
}

bool MemoryManagerClient::IncreaseRefCount(int id) {
    if (!initialized) {
        throw std::runtime_error("MemoryManagerClient not initialized");
    }
    
    if (id == -1) {
        std::cerr << "Cannot increase ref count for invalid ID (-1)" << std::endl;
        return false;
    }
    
    Message message;
    message.type = MessageType::INCREASE_REF_COUNT;
    message.id = id;
    message.size = 0;
    
    Message response;
    return sendMessage(message, response);
}

bool MemoryManagerClient::DecreaseRefCount(int id) {
    if (!initialized) {
        throw std::runtime_error("MemoryManagerClient not initialized");
    }
    
    if (id == -1) {
        std::cerr << "Cannot decrease ref count for invalid ID (-1)" << std::endl;
        return false;
    }
    
    Message message;
    message.type = MessageType::DECREASE_REF_COUNT;
    message.id = id;
    message.size = 0;
    
    Message response;
    return sendMessage(message, response);
}

bool MemoryManagerClient::sendMessage(const Message& message, Message& response) {
    // Create a temporary socket for each message
    int tempSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tempSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Connect to Memory Manager
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        close(tempSocket);
        return false;
    }
    
    if (connect(tempSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed to " << host << ":" << port << std::endl;
        close(tempSocket);
        return false;
    }
    
    // Create a wire-format message that matches the server's expectation
    struct {
        uint8_t type;
        int id;
        size_t size;
        char typeStr[32];
        char data[1024];
    } wireMsg;
    
    // Copy fields from the original message
    wireMsg.type = static_cast<uint8_t>(message.type);
    wireMsg.id = message.id;
    wireMsg.size = message.size;
    strncpy(wireMsg.typeStr, message.typeStr, sizeof(wireMsg.typeStr) - 1);
    wireMsg.typeStr[sizeof(wireMsg.typeStr) - 1] = '\0';
    memcpy(wireMsg.data, message.data, sizeof(wireMsg.data));
    
    // Send message
    if (send(tempSocket, &wireMsg, sizeof(wireMsg), 0) != sizeof(wireMsg)) {
        std::cerr << "Failed to send message" << std::endl;
        close(tempSocket);
        return false;
    }
    
    // Receive response into wire format first
    if (recv(tempSocket, &wireMsg, sizeof(wireMsg), 0) != sizeof(wireMsg)) {
        std::cerr << "Failed to receive response" << std::endl;
        close(tempSocket);
        return false;
    }
    
    // Copy back to the response format
    response.type = static_cast<MessageType>(wireMsg.type);
    response.id = wireMsg.id;
    response.size = wireMsg.size;
    strncpy(response.typeStr, wireMsg.typeStr, sizeof(response.typeStr) - 1);
    response.typeStr[sizeof(response.typeStr) - 1] = '\0';
    memcpy(response.data, wireMsg.data, sizeof(response.data));
    
    // Close socket
    close(tempSocket);
    
    // Debug output for specific message types
    if (message.type == MessageType::CREATE) {
        std::cout << "CREATE request: size=" << message.size 
                  << ", type=" << message.typeStr 
                  << " → response id=" << response.id << std::endl;
    }
    
    return response.id != -1;
}

// Specialization for std::string
template<>
class MPointer<std::string> {
public:
    // Default constructor - explicitly initialize all members
    MPointer() : id(-1), valueCache(), valuePtr(nullptr) {}
    
    // Destructor with safer implementation
    ~MPointer() {
        // Only decrease ref count if id is valid and client is initialized
        if (id != -1 && MemoryManagerClient::IsInitialized()) {
            try {
                MemoryManagerClient::DecreaseRefCount(id);
            } catch (const std::exception& e) {
                // Log but don't throw from destructor
                std::cerr << "Error in MPointer<std::string> destructor: " << e.what() << std::endl;
            }
        }
        // Clear the valueCache to avoid potential issues with string data
        valueCache.clear();
        valuePtr = nullptr;
    }
    
    // Copy constructor - explicitly initialize all members
    MPointer(const MPointer<std::string>& other) : id(other.id), valueCache(), valuePtr(nullptr) {
        if (id != -1 && MemoryManagerClient::IsInitialized()) {
            MemoryManagerClient::IncreaseRefCount(id);
        }
    }
    
    // Assignment operator - properly handle all members
    MPointer<std::string>& operator=(const MPointer<std::string>& other) {
        if (this != &other) {
            if (id != -1 && MemoryManagerClient::IsInitialized()) {
                MemoryManagerClient::DecreaseRefCount(id);
            }
            
            id = other.id;
            valueCache.clear();  // Clear existing string data
            valuePtr = nullptr;
            
            if (id != -1 && MemoryManagerClient::IsInitialized()) {
                MemoryManagerClient::IncreaseRefCount(id);
            }
        }
        return *this;
    }
    
    // New method with simplified approach for string
    static MPointer<std::string> New() {
        MPointer<std::string> ptr;
        
        // Create a block of appropriate size for a Node<std::string>
        int id = MemoryManagerClient::Create(sizeof(Node<std::string>), "Node<std::string>");
        if (id == -1) {
            throw std::runtime_error("Failed to allocate memory for string node");
        }
        
        // Set the ID
        ptr.id = id;
        
        // Initialize with empty string
        Node<std::string> node;
        node.setData("");
        node.nextId = -1;
        node.prevId = -1;
        
        if (!MemoryManagerClient::Set(id, &node, sizeof(Node<std::string>))) {
            // If set fails, decrease the reference count and throw
            MemoryManagerClient::DecreaseRefCount(id);
            throw std::runtime_error("Failed to initialize memory for string node");
        }
        
        return ptr;
    }
    
    // Dereference operator for string
    std::string& operator*() {
        if (id == -1) {
            throw std::runtime_error("Attempting to dereference a null MPointer<std::string>");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        // Get the Node<std::string> from memory manager
        Node<std::string> node;
        if (!MemoryManagerClient::Get(id, &node, sizeof(Node<std::string>))) {
            throw std::runtime_error("Failed to get string node from Memory Manager");
        }
        
        // Update local cache
        valueCache = node.getData();
        return valueCache;
    }
    
    // Arrow operator for string
    std::string* operator->() {
        if (id == -1) {
            throw std::runtime_error("Attempting to use -> on a null MPointer<std::string>");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        // Get the Node<std::string> from memory manager
        Node<std::string> node;
        if (!MemoryManagerClient::Get(id, &node, sizeof(Node<std::string>))) {
            throw std::runtime_error("Failed to get string node from Memory Manager");
        }
        
        // Update local cache
        valueCache = node.getData();
        valuePtr = &valueCache;
        return valuePtr;
    }
    
    // ID access method
    int getId() const {
        return id;
    }
    
    // Assignment operator for string values
    std::string& operator=(const std::string& value) {
        if (id == -1) {
            throw std::runtime_error("Attempting to assign to a null MPointer<std::string>");
        }
        
        if (!MemoryManagerClient::IsInitialized()) {
            throw std::runtime_error("MemoryManagerClient not initialized");
        }
        
        // Get the current node
        Node<std::string> node;
        if (!MemoryManagerClient::Get(id, &node, sizeof(Node<std::string>))) {
            throw std::runtime_error("Failed to get string node for assignment");
        }
        
        // Update the string data
        node.setData(value);
        
        // Save the updated node
        if (!MemoryManagerClient::Set(id, &node, sizeof(Node<std::string>))) {
            throw std::runtime_error("Failed to update string node");
        }
        
        // Update local cache
        valueCache = value;
        return valueCache;
    }

    // Friend declarations
    template <typename U>
    friend class LinkedList;
    
    template <typename U>
    friend struct Node;

private:
    int id;                 // ID of the block in Memory Manager
    std::string valueCache; // Local cache for the string
    std::string* valuePtr;  // Pointer to the local cache
};

#endif // MPOINTER_H