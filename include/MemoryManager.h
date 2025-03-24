#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <limits> // Para std::numeric_limits

class MemoryBlock {
public:
    MemoryBlock(size_t offset, size_t size, const std::string& type);
    ~MemoryBlock() = default;

    size_t offset;      // Offset from the start of the memory pool
    size_t size;        // Size of the block in bytes
    std::string type;   // Type of data stored
    int refCount;       // Reference counter
    bool inUse;         // Flag to mark if block is in use
};

class MemoryManager {
public:
    MemoryManager(int port, size_t sizeInMB, const std::string& dumpFolder);
    ~MemoryManager();

    // Server methods
    bool startServer();
    void stopServer();

    // Memory management methods
    int create(size_t size, const std::string& type);
    bool set(int id, const void* value, size_t valueSize);
    bool get(int id, void* value, size_t valueSize);
    bool increaseRefCount(int id);
    bool decreaseRefCount(int id);

private:
    // Memory pool
    void* memoryPool;
    size_t poolSize;
    
    // Dump folder
    std::string dumpFolder;
    
    // Mapping of IDs to memory blocks
    std::map<int, MemoryBlock> blocks;
    int nextId;
    
    // Thread safety
    std::mutex blocksMutex;
    
    // Server
    int port;
    bool running;
    std::thread serverThread;
    std::thread gcThread;
    
    // Private methods
    void serverLoop();
    void garbageCollector();
    void createMemoryDump();
    
    // Memory allocation helpers
    size_t findFreeSpace(size_t size);
    void defragmentMemory();
};

#endif // MEMORY_MANAGER_H