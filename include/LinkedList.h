#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "MPointer.h"
#include "Node.h"
#include <iostream>

// LinkedList template class
template <typename T>
class LinkedList {
public:
    // Constructor
    LinkedList() : headId(-1), tailId(-1), size(0) {
        std::cout << "LinkedList created" << std::endl;
    }
    
    // Destructor
    ~LinkedList() {
        // Clear all nodes
        clear();
    }
    
    // Add element to the back of the list
    void pushBack(const T& value) {
        // Create a new node
        MPointer<Node<T>> newNode = MPointer<Node<T>>::New();
        std::cout << "Created new node with ID: " << newNode.getId() << std::endl;
        
        // Debug print
        std::cout << "Before setting data, node.data = " << newNode->data << std::endl;

        // Create a complete Node with all fields properly initialized
        Node<T> tempNode;
        tempNode.data = value;  // Set the value
        tempNode.nextId = -1;   // Initialize links
        tempNode.prevId = -1;
        
        // Directly set the entire node to ensure proper memory layout
        if (!MemoryManagerClient::Set(newNode.getId(), &tempNode, sizeof(Node<T>))) {
            throw std::runtime_error("Failed to set node data");
        }
        
        // Debug verification - retrieve and check the node
        Node<T> verifyNode;
        if (MemoryManagerClient::Get(newNode.getId(), &verifyNode, sizeof(Node<T>))) {
            std::cout << "Verification - node data: " << verifyNode.data << std::endl;
            debugPrintMemory(&verifyNode, sizeof(Node<T>), "Retrieved node");
        }
        
        // First element case
        if (headId == -1) {
            // This is the first element
            headId = newNode.getId();
            tailId = newNode.getId();
        } else {
            // Add to the end of the list
            MPointer<Node<T>> tailNode = getNodeById(tailId);
            if (tailNode.getId() != -1) {
                // Create temp nodes for updating links
                Node<T> updatedNewNode;
                MemoryManagerClient::Get(newNode.getId(), &updatedNewNode, sizeof(Node<T>));
                updatedNewNode.prevId = tailId;
                updatedNewNode.nextId = -1;
                MemoryManagerClient::Set(newNode.getId(), &updatedNewNode, sizeof(Node<T>));
                
                Node<T> updatedTailNode;
                MemoryManagerClient::Get(tailId, &updatedTailNode, sizeof(Node<T>));
                updatedTailNode.nextId = newNode.getId();
                MemoryManagerClient::Set(tailId, &updatedTailNode, sizeof(Node<T>));
                
                tailId = newNode.getId();
            }
        }
        
        size++;
        std::cout << "Added element to back: " << value << ", size now: " << size << std::endl;
    }
    
    // Add element to the front of the list
    void pushFront(const T& value) {
        // Create a new node
        MPointer<Node<T>> newNode = MPointer<Node<T>>::New();
        std::cout << "Created new node with ID: " << newNode.getId() << std::endl;
        
        // Create a complete Node with all fields properly initialized
        Node<T> tempNode;
        tempNode.data = value;  // Set the value
        tempNode.nextId = -1;   // Initialize links
        tempNode.prevId = -1;
        
        // Directly set the entire node to ensure proper memory layout
        if (!MemoryManagerClient::Set(newNode.getId(), &tempNode, sizeof(Node<T>))) {
            throw std::runtime_error("Failed to set node data");
        }
        
        // First element case
        if (headId == -1) {
            // This is the first element
            headId = newNode.getId();
            tailId = newNode.getId();
        } else {
            // Add to the beginning of the list
            MPointer<Node<T>> headNode = getNodeById(headId);
            if (headNode.getId() != -1) {
                // Create temp nodes for updating links
                Node<T> updatedNewNode;
                MemoryManagerClient::Get(newNode.getId(), &updatedNewNode, sizeof(Node<T>));
                updatedNewNode.nextId = headId;
                updatedNewNode.prevId = -1;
                MemoryManagerClient::Set(newNode.getId(), &updatedNewNode, sizeof(Node<T>));
                
                Node<T> updatedHeadNode;
                MemoryManagerClient::Get(headId, &updatedHeadNode, sizeof(Node<T>));
                updatedHeadNode.prevId = newNode.getId();
                MemoryManagerClient::Set(headId, &updatedHeadNode, sizeof(Node<T>));
                
                headId = newNode.getId();
            }
        }
        
        size++;
        std::cout << "Added element to front: " << value << ", size now: " << size << std::endl;
    }
    
    // Remove the first element
    bool popFront() {
        if (headId == -1) {
            return false; // List is empty
        }
        
        // Get a copy of the head node
        Node<T> headNode;
        if (!MemoryManagerClient::Get(headId, &headNode, sizeof(Node<T>))) {
            return false; // Failed to get head node
        }
        
        int nextId = headNode.nextId;
        
        // Update next node's prev pointer if there is a next node
        if (nextId != -1) {
            Node<T> nextNode;
            if (MemoryManagerClient::Get(nextId, &nextNode, sizeof(Node<T>))) {
                nextNode.prevId = -1;
                MemoryManagerClient::Set(nextId, &nextNode, sizeof(Node<T>));
            }
        }
        
        // If this was the only element, update tailId
        if (tailId == headId) {
            tailId = -1;
        }
        
        // Update headId to the next node
        int oldHeadId = headId;
        headId = nextId;
        
        // Decrease reference count for the node being removed
        MemoryManagerClient::DecreaseRefCount(oldHeadId);
        
        size--;
        return true;
    }
    
    // Get value at index
    bool get(int index, T& value) {
        if (index < 0 || index >= size) {
            return false;
        }
        
        // Get a copy of the node at the given index
        Node<T> node;
        int nodeId = headId;
        
        for (int i = 0; i < index; i++) {
            if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<T>))) {
                return false;
            }
            
            // If we reach the end unexpectedly, return false
            if (node.nextId == -1) {
                return false;
            }
            
            nodeId = node.nextId;
        }
        
        // Get the node at the final index
        if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<T>))) {
            return false;
        }
        
        value = node.data;
        return true;
    }
    
    // Set value at index
    bool set(int index, const T& value) {
        if (index < 0 || index >= size) {
            return false;
        }
        
        // Find the node at the given index
        int nodeId = headId;
        Node<T> node;
        
        for (int i = 0; i < index; i++) {
            if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<T>))) {
                return false;
            }
            
            // If we reach the end unexpectedly, return false
            if (node.nextId == -1) {
                return false;
            }
            
            nodeId = node.nextId;
        }
        
        // Get the node, update its data, and set it back
        if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<T>))) {
            return false;
        }
        
        node.data = value;
        
        if (!MemoryManagerClient::Set(nodeId, &node, sizeof(Node<T>))) {
            return false;
        }
        
        return true;
    }
    
    // Clear the list
    void clear() {
        // For safety, handle each node individually
        while (headId != -1) {
            popFront();
        }
        
        // Reset list state
        headId = -1;
        tailId = -1;
        size = 0;
    }
    
    // Print the list
    void print() {
        if (headId == -1) {
            std::cout << "List [empty]" << std::endl;
            return;
        }
        
        std::cout << "List [" << size << "]" << std::endl;
        
        int nodeCount = 0;
        int currentId = headId;
        
        while (currentId != -1 && nodeCount < size) {
            Node<T> node;
            if (!MemoryManagerClient::Get(currentId, &node, sizeof(Node<T>))) {
                std::cerr << "Error: Failed to get node with ID " << currentId << std::endl;
                break;
            }
            
            std::cout << "  " << nodeCount << ": " << node.data << std::endl;
            
            // Move to next node
            currentId = node.nextId;
            nodeCount++;
            
            // Safety check to prevent infinite loops
            if (nodeCount > size) {
                std::cerr << "Warning: List structure corrupted - more nodes than expected" << std::endl;
                break;
            }
        }
    }

private:
    int headId;  // ID of the head node
    int tailId;  // ID of the tail node
    int size;    // Number of elements in the list
    
    // Helper method to get a node by ID
    MPointer<Node<T>> getNodeById(int id) {
        if (id == -1) {
            return MPointer<Node<T>>();  // Return null MPointer
        }
        
        // Create a temporary MPointer with the given ID
        MPointer<Node<T>> nodePtr;
        nodePtr.id = id;  // Direct access to private member via friend declaration
        
        // Increase reference count
        if (!MemoryManagerClient::IsInitialized()) {
            std::cerr << "MemoryManagerClient not initialized" << std::endl;
            return MPointer<Node<T>>(); // Return null MPointer
        }
        
        if (!MemoryManagerClient::IncreaseRefCount(id)) {
            std::cerr << "Failed to increase reference count for node " << id << std::endl;
            return MPointer<Node<T>>(); // Return null MPointer
        }
        
        return nodePtr;
    }
};

// Specialization for std::string
template<>
void LinkedList<std::string>::pushBack(const std::string& value) {
    // Create a new node
    MPointer<Node<std::string>> newNode = MPointer<Node<std::string>>::New();
    std::cout << "Created string node with ID: " << newNode.getId() << std::endl;
    
    // Create a complete Node with all fields properly initialized
    Node<std::string> tempNode;
    tempNode.setData(value);  // Use the helper method
    tempNode.nextId = -1;
    tempNode.prevId = -1;
    
    // Directly set the entire node
    if (!MemoryManagerClient::Set(newNode.getId(), &tempNode, sizeof(Node<std::string>))) {
        throw std::runtime_error("Failed to set string node data");
    }
    
    // Debug verification
    Node<std::string> verifyNode;
    if (MemoryManagerClient::Get(newNode.getId(), &verifyNode, sizeof(Node<std::string>))) {
        std::cout << "Verification - string node data: '" << verifyNode.getData() << "'" << std::endl;
    }
    
    // First element case
    if (headId == -1) {
        // This is the first element
        headId = newNode.getId();
        tailId = newNode.getId();
    } else {
        // Add to the end of the list
        MPointer<Node<std::string>> tailNode = getNodeById(tailId);
        if (tailNode.getId() != -1) {
            // Update links using temp nodes
            Node<std::string> updatedNewNode;
            MemoryManagerClient::Get(newNode.getId(), &updatedNewNode, sizeof(Node<std::string>));
            updatedNewNode.prevId = tailId;
            updatedNewNode.nextId = -1;
            MemoryManagerClient::Set(newNode.getId(), &updatedNewNode, sizeof(Node<std::string>));
            
            Node<std::string> updatedTailNode;
            MemoryManagerClient::Get(tailId, &updatedTailNode, sizeof(Node<std::string>));
            updatedTailNode.nextId = newNode.getId();
            MemoryManagerClient::Set(tailId, &updatedTailNode, sizeof(Node<std::string>));
            
            tailId = newNode.getId();
        }
    }
    
    size++;
    std::cout << "Added string element to back: '" << value << "', size now: " << size << std::endl;
}

// Specialization for std::string to handle proper printing
template<>
void LinkedList<std::string>::print() {
    if (headId == -1) {
        std::cout << "List [empty]" << std::endl;
        return;
    }
    
    std::cout << "List [" << size << "]" << std::endl;
    
    int nodeCount = 0;
    int currentId = headId;
    
    while (currentId != -1 && nodeCount < size) {
        Node<std::string> node;
        if (!MemoryManagerClient::Get(currentId, &node, sizeof(Node<std::string>))) {
            std::cerr << "Error: Failed to get string node with ID " << currentId << std::endl;
            break;
        }
        
        std::cout << "  " << nodeCount << ": \"" << node.getData() << "\"" << std::endl;
        
        // Move to next node
        currentId = node.nextId;
        nodeCount++;
        
        // Safety check to prevent infinite loops
        if (nodeCount > size) {
            std::cerr << "Warning: List structure corrupted - more nodes than expected" << std::endl;
            break;
        }
    }
}

// Additional string specializations for other methods
template<>
bool LinkedList<std::string>::get(int index, std::string& value) {
    if (index < 0 || index >= size) {
        return false;
    }
    
    // Find the node at the given index
    int nodeId = headId;
    Node<std::string> node;
    
    for (int i = 0; i < index; i++) {
        if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<std::string>))) {
            return false;
        }
        
        // If we reach the end unexpectedly, return false
        if (node.nextId == -1) {
            return false;
        }
        
        nodeId = node.nextId;
    }
    
    // Get the node at the final index
    if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<std::string>))) {
        return false;
    }
    
    value = node.getData();
    return true;
}

template<>
bool LinkedList<std::string>::set(int index, const std::string& value) {
    if (index < 0 || index >= size) {
        return false;
    }
    
    // Find the node at the given index
    int nodeId = headId;
    Node<std::string> node;
    
    for (int i = 0; i < index; i++) {
        if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<std::string>))) {
            return false;
        }
        
        // If we reach the end unexpectedly, return false
        if (node.nextId == -1) {
            return false;
        }
        
        nodeId = node.nextId;
    }
    
    // Get the node, update its data, and set it back
    if (!MemoryManagerClient::Get(nodeId, &node, sizeof(Node<std::string>))) {
        return false;
    }
    
    node.setData(value);
    
    if (!MemoryManagerClient::Set(nodeId, &node, sizeof(Node<std::string>))) {
        return false;
    }
    
    return true;
}

#endif // LINKED_LIST_H