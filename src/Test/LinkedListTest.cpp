#include "../../include/LinkedList.h"
#include <iostream>
#include <string>

// Test the LinkedList implementation
int main() {
    try {
        std::cout << "Initializing MPointer..." << std::endl;
        // Initialize MPointer to connect to Memory Manager
        MPointer<int>::Init(8080);  // Use the same port as Memory Manager
        std::cout << "Connected to Memory Manager" << std::endl;
        
        {
            // Create a simple test first - just one element
            std::cout << "\n=== TEST 1: SINGLE ELEMENT ===\n";
            std::cout << "Creating LinkedList<int>..." << std::endl;
            LinkedList<int> testList1;
            
            std::cout << "Adding one element (10)..." << std::endl;
            testList1.pushBack(10);
            std::cout << "Current list: ";
            testList1.print();
            
            // Test with multiple elements if the first test works
            std::cout << "\n=== TEST 2: MULTIPLE ELEMENTS ===\n";
            std::cout << "Creating LinkedList<int>..." << std::endl;
            LinkedList<int> testList2;
            
            std::cout << "Adding elements one by one..." << std::endl;
            
            std::cout << "Adding 10..." << std::endl;
            testList2.pushBack(10);
            testList2.print();
            
            std::cout << "Adding 20..." << std::endl;
            testList2.pushBack(20);
            testList2.print();
            
            std::cout << "Adding 30..." << std::endl;
            testList2.pushBack(30);
            testList2.print();
            
            // Only continue if previous tests succeeded
            std::cout << "\n=== TEST 3: ADVANCED OPERATIONS ===\n";
            std::cout << "Creating test list..." << std::endl;
            LinkedList<int> intList;
            
            std::cout << "Adding elements..." << std::endl;
            intList.pushBack(10);
            intList.pushBack(20);
            intList.pushBack(30);
            intList.pushFront(5);
            intList.pushFront(1);
            
            std::cout << "Final list: ";
            intList.print();
            
            // Get element at index 2
            int value;
            if (intList.get(2, value)) {
                std::cout << "Element at index 2: " << value << std::endl;
            }
            
            // Modify element at index 2
            std::cout << "Modifying element at index 2 to 15..." << std::endl;
            intList.set(2, 15);
            std::cout << "After modifying element at index 2: ";
            intList.print();
            
            // Remove elements
            std::cout << "Removing the first element..." << std::endl;
            intList.popFront();
            std::cout << "After removing the first element: ";
            intList.print();
            
            // Clear the list
            std::cout << "Clearing the list..." << std::endl;
            intList.clear();
            std::cout << "After clearing the list: ";
            intList.print();
            
            // Test with strings
            std::cout << "\n=== TEST 4: STRING LIST ===\n";
            std::cout << "Creating LinkedList<std::string>..." << std::endl;
            {
                LinkedList<std::string> stringList;
                
                std::cout << "Adding elements..." << std::endl;
                stringList.pushBack("Hello");
                stringList.pushBack("World");
                stringList.pushBack("MPointers");
                
                std::cout << "String list: ";
                stringList.print();
            } // Asegura que stringList se destruya antes de Cleanup()
        } // Asegura que todas las listas se destruyan antes de Cleanup()
        
        // Cleanup - llamado después de que todas las listas se han destruido
        std::cout << "\nCleaning up..." << std::endl;
        MemoryManagerClient::Cleanup();
        std::cout << "Test completed successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}