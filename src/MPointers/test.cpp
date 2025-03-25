#include "../../include/MPointer.h"
#include <iostream>
#include <string>

// Simple test for MPointer
int main() {
    try {
        // Initialize MPointer to connect to Memory Manager
        MPointer<int>::Init(8080);  // Use the same port as Memory Manager
        std::cout << "Connected to Memory Manager" << std::endl;
        
        // Create a new MPointer
        std::cout << "Creating MPointer<int>..." << std::endl;
        MPointer<int> myPtr = MPointer<int>::New();
        std::cout << "MPointer created with ID: " << myPtr.getId() << std::endl;
        
        // Set a value
        std::cout << "Setting value to 42..." << std::endl;
        *myPtr = 42;
        
        // Get the value
        std::cout << "Value: " << *myPtr << std::endl;
        
        // Create another MPointer
        std::cout << "Creating another MPointer<int>..." << std::endl;
        MPointer<int> anotherPtr = MPointer<int>::New();
        std::cout << "Another MPointer created with ID: " << anotherPtr.getId() << std::endl;
        
        // Set a value
        std::cout << "Setting value to 100..." << std::endl;
        *anotherPtr = 100;
        
        // Get the value
        std::cout << "Value: " << *anotherPtr << std::endl;
        
        // Test assignment
        std::cout << "Assigning first MPointer to the second one..." << std::endl;
        anotherPtr = myPtr;
        std::cout << "Value after assignment: " << *anotherPtr << std::endl;
        
        // Test with different type
        std::cout << "Creating MPointer<std::string>..." << std::endl;
        MPointer<std::string> strPtr = MPointer<std::string>::New();
        std::cout << "String MPointer created with ID: " << strPtr.getId() << std::endl;
        
        // Set a string value
        std::cout << "Setting string value..." << std::endl;
        *strPtr = "Hello, MPointers!";
        
        // Get the string value
        std::cout << "String value: " << *strPtr << std::endl;
        
        // Clean up
        MemoryManagerClient::Cleanup();
        std::cout << "Test completed successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}