#include <iostream>
#include <Windows.h>

#include "Assembler.h"


int main() {
   
    // Define the temporary file name
    
    std::cout << "Opening file for editing: " <<  std::endl;
    std::cout << "Edit the file and then save it. Press Enter when done...";
    std::cin.get(); // Wait for user input before proceeding

    // Create Assembler instance
    std::string inputFileName = "Input_File.txt";
    std::string outputFileName = "output_File.txt";

    Assembler assembler(inputFileName, outputFileName);
    assembler.assemble();
    std::cout << "Assembling completed. Check the file: " << outputFileName << std::endl;
    return 0;
}