#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstring>
#include <iostream>

// Debug function to print memory contents in hex
inline void debugPrintMemory(const void* data, size_t size, const char* label) {
    std::cout << "DEBUG Memory dump [" << label << "] (" << size << " bytes): ";
    const unsigned char* byteData = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < std::min(size, (size_t)16); i++) {
        printf("%02x ", byteData[i]);
    }
    std::cout << std::endl;
}

// Node structure for LinkedList - specifically designed to maintain consistent memory layout
template <typename T>
struct Node {
    T data;
    int nextId; // Store ID instead of MPointer to avoid circular references
    int prevId; // Store ID instead of MPointer to avoid circular references

    // Constructor with explicit full initialization
    Node() : data(T()), nextId(-1), prevId(-1) {
        std::cout << "DEBUG: Creating Node<T> with sizeof(Node<T>)=" << sizeof(Node<T>) 
                 << ", sizeof(data)=" << sizeof(T) << std::endl;
    }
};

// Special version of Node for strings to prevent memory issues
template<>
struct Node<std::string> {
    char stringData[512]; // Fixed-size buffer for string data
    int nextId;
    int prevId;
    
    // Constructor with explicit initialization
    Node() : nextId(-1), prevId(-1) {
        memset(stringData, 0, sizeof(stringData));
    }
    
    // Helper to get the data as a std::string
    std::string getData() const {
        return std::string(stringData);
    }
    
    // Helper to set the data from a std::string
    void setData(const std::string& value) {
        strncpy(stringData, value.c_str(), sizeof(stringData) - 1);
        stringData[sizeof(stringData) - 1] = '\0'; // Ensure null termination
    }
};

#endif // NODE_H