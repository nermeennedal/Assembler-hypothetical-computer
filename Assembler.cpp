#include "Assembler.h"
#include <fstream>
#include <sstream>

#include <iomanip>

#include <vector>
#include <algorithm>

#include <iostream>

#include <string>
#include <unordered_map>
#include <cctype> // For isspace()

using std::string;
using std::vector;

struct TripleKey {
	std::string key1;
	uint32_t key2;
	uint32_t key3;

	// Define equality comparison for the struct
	bool operator==(const TripleKey& other) const {
		return key1 == other.key1 && key2 == other.key2 && key3 == other.key3;
	}
};

// Custom hash function for the struct
struct TripleKeyHash {
	std::size_t operator()(const TripleKey& t) const {
		return std::hash<std::string>()(t.key1) ^ std::hash<uint32_t>()(t.key2) ^ std::hash<uint32_t>()(t.key3);
	}
};
std::unordered_map<TripleKey, uint32_t, TripleKeyHash> LITTAP;

std::unordered_map<std::string, uint32_t> labelMap;
uint32_t currentAddress = 0x0000;
uint32_t B;
bool base_flag = false;
std::unordered_map<std::string, uint8_t> opcodeMap = {
	{"ADD", 0x18},{"ADDR", 0x90},{"AND", 0x40},{"CLEAR", 0xB4},{"COMP", 0x28},
	{"COMPR", 0xA0},{"DIV", 0x24},{"DIVR", 0x9C},{"J", 0x3C},{"JEQ", 0x30},
	{"JGT", 0x34},{"JLT", 0x38},{"JSUB", 0x48},{"LDA", 0x00},{"LDB", 0x68},
	{"LDCH",0x50},{"LDL",0x08},{"LDS",0x6C},{"LDT",0x74},{"LDX",0x04},{"MUL",0x20},{"MULR",0x98},{"OR",0x44},{"RD",0xD8},
	{"RMO",0xAC},{"RSUB",0x4C},{"STA",0x0C},{"STB",0x78},{"STCH",0x54},{"STF",0x80},{"STL",0x14},{"STS",0x7C},
	{"STSW",0xE8},{"STT",0x84},{"STX",0x10},{"SUB",0x1C},{"SUBR",0x94},{"TD",0xE0},{"TIX",0x2C},{"TIXR",0xB8},{"WD",0xDC}
};

std::unordered_map<std::string, uint8_t> RegisterMap = {
	{"A",0},{"X",1},{"L",2},{"PC",8},{"SW",9},{"B",3},{"S",4},{"T",5},{"F",6},
};
Assembler::Assembler(const std::string& inputFileName, const std::string& outputFileName)
	: inputFileName(inputFileName), outputFileName(outputFileName), currentAddress(0x0000) {}

void Assembler::assemble() {
	Pass1();  
	Pass2();  
}
void Assembler::Pass1() {
	std::ifstream inputFile(inputFileName); // Open the file
	if (!inputFile) {
		std::cerr << "Error: Could not open the file!" << std::endl;
		return;
	}

	std::string line, ignore, start;
	std::getline(inputFile, line);
	std::istringstream iss2(line);
	iss2 >> ignore;
	iss2 >> start;
	start = trimWhitespace(start);

	// Get the start address
	if (start == "START") {
		std::string start_address;
		iss2 >> start_address;
		start_address = trimWhitespace(start_address);
		currentAddress = static_cast<uint32_t>(std::stoul(start_address, nullptr, 16));
	}

	while (std::getline(inputFile, line)) {
		std::string op;
		if (!line.empty() && std::isspace(line[0])) {
			std::istringstream iss(line);
			iss >> op;
			op = trimWhitespace(op);
			if (op == "BASE") {
				currentAddress = currentAddress;
			}
			else if (op[0] == '+') {
				currentAddress += 4;
			}
			else if (op.back() == 'R' || op == "RMO") {
				currentAddress += 2;
			}
			else if (iss >> op) {
				if (op.empty()) {
					currentAddress += 1;
				}
				else {
					currentAddress += 3;
				}
			}
			continue; // Skip this line
		}

		// Process the line
		if (!line.empty()) {
			std::istringstream iss(line);
			std::string label;
			iss >> label;
			label = trimWhitespace(label);
			iss >> op;
			op = trimWhitespace(op);

			labelMap[label] = currentAddress;

			if (op == "BYTE") {
				std::string operand;
				iss >> operand;
				operand = trimWhitespace(operand);

				if (operand[0] == 'X') {
					operand = operand.substr(2, operand.size() - 3); // Remove "X'"
					for (size_t i = 0; i < operand.size(); i += 2) {
						uint8_t byte = std::stoi(operand.substr(i, 2), nullptr, 16);
						currentAddress++;
					}
				}
				else if (operand[0] == 'C') {
					operand = operand.substr(2, operand.size() - 3); // Remove "C'"
					currentAddress += operand.size();
				}
			}
			else if (op[0] == '+') {
				currentAddress += 4;
			}
			else if (op.back() == 'R' || op == "RMO") {
				currentAddress += 2;
			}
			else if (iss >> op) {
				if (op.empty()) {
					currentAddress += 1;
				}
				else {
					currentAddress += 3;
				}
			}
		}
	}

	inputFile.close();
}


void Assembler::Pass2() {
	std::ifstream inputFile(inputFileName);
	std::ofstream outputFile(outputFileName, std::ios::trunc); // Assembled listing file
	std::ofstream objectFile("ObjectFile.txt", std::ios::trunc);   // Object file
	if (!inputFile.is_open() || !outputFile.is_open() || !objectFile.is_open()) {
		std::cerr << "Error opening files.\n";
		return;
	}

	std::string line, ignore, start;
	std::getline(inputFile, line);
	std::istringstream iss2(line);
	iss2 >> ignore >> start;
	start = trimWhitespace(start);

	uint32_t programLength = 0;
	uint32_t programStart = 0;

	if (start == "START") {
		std::string startAddress;
		iss2 >> startAddress;
		programStart = static_cast<uint32_t>(std::stoul(startAddress, nullptr, 16));
		currentAddress = programStart;
	}

	// Prepare Header record
	std::ostringstream headerStream;
	headerStream << "H" << std::setw(6) << std::left << ignore.substr(0, 6)
		<< std::hex << std::uppercase << std::setw(6) << std::setfill('0') << programStart
		<< std::setw(6) << std::setfill('0') << programLength;
	headerRecord = headerStream.str();

	// Prepare Text records
	std::ostringstream textStream;
	uint32_t textStart = currentAddress;
	std::ostringstream modificationStream;

	while (std::getline(inputFile, line)) {
		if (line.empty()) continue;

		uint32_t machineCode = assembleInstruction(line);
		std::string line2 = line;
		if (machineCode == 0xDEADBEEF) {
			std::cerr << "Error assembling line: " << line << "\n";
		}
		else {
			// Create and store the instruction
			Instruction instr = { currentAddress, machineCode, line };
			writeHexToAssembledFile(instr);

		}
		if (line.substr(0, 3) == "END") {
			break;
		}

		// Update text record
		if (textStream.tellp() > 60) { // If the record exceeds max length, write and start a new one
			textRecord += "T" + writeTextRecord(textStart, textStream.str());
			textStream.str("");
			textStart = currentAddress;
		}

		if (line.substr(0, 4) == "BYTE") {
			std::string operand;
			std::istringstream lineStream(line);
			lineStream >> operand;
			operand = trimWhitespace(operand);

			if (operand[0] == 'X') {  // Hexadecimal byte literal
				operand = operand.substr(2, operand.size() - 3);  // Strip 'X' and quotes
				for (size_t i = 0; i < operand.size(); i += 2) {
					std::string byteStr = operand.substr(i, 2);
					uint8_t byte = std::stoul(byteStr, nullptr, 16);
					textStream << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
				}
			}
			else if (operand[0] == 'C') {  // Character byte literal
				operand = operand.substr(2, operand.size() - 3);  // Strip 'C' and quotes
				for (char c : operand) {
					textStream << std::hex << std::setw(2) << std::setfill('0') << (int)c;
				}
			}
			Instruction instr = { currentAddress, machineCode, line2 };
			writeHexToAssembledFile(instr);
		}
		else if (line.substr(0, 4) == "WORD") {
			std::string operand;
			std::istringstream lineStream(line);
			lineStream >> operand;
			operand = trimWhitespace(operand);
			uint32_t wordValue = std::stoul(operand, nullptr, 10);
			textStream << std::hex << std::setw(6) << std::setfill('0') << wordValue;
			Instruction instr = { currentAddress, machineCode, line2 };
			writeHexToAssembledFile(instr);
		}
		else {
			textStream << std::hex << std::setw(8) << std::setfill('0') << machineCode;
		}

		// Write modification records for relocatable addresses
		if (requiresModification(line)) {
			modificationStream << writeModificationRecord(currentAddress);
		}

		// Update the program length
		programLength += calculateInstructionLength(line);
		currentAddress += calculateInstructionLength(line);
	}

	// Finalize text records
	if (!textStream.str().empty()) {
		textRecord += "T" + writeTextRecord(textStart, textStream.str());
	}

	// Prepare End record
	std::ostringstream endStream;
	endStream << "E" << std::setw(6) << std::setfill('0') << programStart;
	endRecord = endStream.str();

	// Write to the object file
	objectFile << headerRecord << "\n" << textRecord << "\n"
		<< modificationStream.str() << "\n" << endRecord;

	inputFile.close();
	outputFile.close();
	objectFile.close();
}


uint32_t Assembler::assembleInstruction(const std::string& instruction) {
	std::istringstream iss(instruction);
	std::string op;
	if (!instruction.empty() && !(instruction[0] == ' ')) {
		std::string label;
		iss >> label;
		if (label == "BASE") {
			iss >> label;
			base_flag = true;
			B = labelMap[label];
			return 0;
		}
	}

	iss >> op;
	op = trimWhitespace(op);
	if (op == "END") {
		return 0;
	}

	// Handle BYTE and WORD directives
	if (op == "BYTE") {
		std::string operand;
		iss >> operand;
		operand = trimWhitespace(operand);
		handleBYTE(operand); // Adjust the address based on BYTE literal
		return 0; // No machine code generated for BYTE
	}
	else if (op == "WORD") {
		std::string operand;
		iss >> operand;
		operand = trimWhitespace(operand);
		handleWORD(operand); // Adjust the address based on WORD literal
		return 0; // No machine code generated for WORD
	}

	std::string op2 = op;
	if (op[0] == '+') {
		op2 = op2.erase(0, 1);
	}

	auto opcodeIt = opcodeMap.find(op2);
	if (opcodeIt != opcodeMap.end()) {
		uint8_t opcode = opcodeIt->second;
		opcode = opcode >> 2;

		if (op[0] == '+') {
			return Format4(iss, opcode);
		}
		else if (op.back() == 'R' || op == "RMO") {
			return Format2(iss, opcode);
		}
		else {
			return Format3(iss, opcode);
		}
	}
	return 0; // Return 0 for unrecognized opcodes
}

uint32_t Assembler::Format4(std::istringstream& iss, uint8_t opcode){
	std::string address;
	uint8_t nixbpe;
	iss >> address;
	address = trimWhitespace(address);
	
	if (address[0] == '#') {  // Immediate addressing
		nixbpe = 0x10;  // n=0, i=1, x=0, b=0, p=0, e=1
		address.erase(0, 1); // Remove '#' symbol
		return (opcode << 24) |  // Opcode shifted to the highest bits
			(nixbpe << 20) | // NIXBPE
			labelMap[address]; // Target address
	}
	else if (address[0] == '@') {  // Indirect addressing
		nixbpe = 0x20;  // n=1, i=0, x=0, b=0, p=0, e=1
		address.erase(0, 1); // Remove '@' symbol
		return (opcode << 24) |
			(nixbpe << 20) |
			labelMap[address];
	}
	else {  // Simple/direct addressing
		nixbpe = 0x30;  // n=1, i=1, x=0, b=0, p=0, e=1
		return (opcode << 24) |
			(nixbpe << 20) |
			labelMap[address];
	}
}

uint32_t Assembler::Format3(std::istringstream& iss, uint8_t opcode) {
	std::string address;
	uint8_t ni;
	uint8_t x;
	uint8_t bpe;
	iss >> address;

	if (address[0] == '#') {
		ni = 0b01; // edit
		x = 0b0;
		bpe = 0b000;
		address.erase(0, 1);

		uint32_t result = (static_cast<uint32_t>(opcode) << (2 + 1 + 3 + 12)) | // Shift op to its position
			(static_cast<uint32_t>(ni) << (1 + 3 + 12)) |        // Shift ni to its position
			(static_cast<uint32_t>(x) << (3 + 12)) |             // Shift x to its position
			(static_cast<uint32_t>(bpe) << 12) |                 // Shift bpe to its position
			labelMap[address];
		return result;
	}
	else {
		if (address[0] == '@') {
			ni = 0b10;
			address.erase(0, 1);
			uint32_t offset = labelMap[address] - currentAddress - 3;

			if (offset < 0xFFF || offset > 0x000) {
				bpe = 0x010;
			}
			else if (base_flag) {
				bpe = 0x100;
				uint32_t offset = labelMap[address] - B;
			}
			else {
				std::cerr << "Error: can't calculate offset address " << inputFileName << std::endl;
				return 0xDEADBEEF; // Error code
			}

			x = (address.back() == 'x' && address[address.size() - 2] == 'x') ? 0b1 : 0b0;

			uint32_t result = (static_cast<uint32_t>(opcode) << (2 + 1 + 3 + 12)) | // Shift op to its position
				(static_cast<uint32_t>(ni) << (1 + 3 + 12)) |        // Shift ni to its position
				(static_cast<uint32_t>(x) << (3 + 12)) |             // Shift x to its position
				(static_cast<uint32_t>(bpe) << 12) |                 // Shift bpe to its position
				offset;
			return result;
		}
		else {
			ni = 0b11;
			uint16_t offset = labelMap[address] - currentAddress - 3;

			if (offset < 0xFFF || offset > 0x000) {
				bpe = 0b010;
			}
			else if (base_flag) {
				bpe = 0b100;
				uint32_t offset = labelMap[address] - B;
			}
			else {
				std::cerr << "Error: can't calculate offset address " << inputFileName << std::endl;
				return 0xDEADBEEF; // Error code
			}

			x = (address.back() == 'x' && address[address.size() - 2] == 'x') ? 0b1 : 0b0;

			uint32_t result = (static_cast<uint32_t>(opcode) << (2 + 1 + 3 + 12)) | // Shift op to its position
				(static_cast<uint32_t>(ni) << (1 + 3 + 12)) |        // Shift ni to its position
				(static_cast<uint32_t>(x) << (3 + 12)) |             // Shift x to its position
				(static_cast<uint32_t>(bpe) << 12) |                 // Shift bpe to its position
				offset;
			return result;
		}
	}
}

uint32_t Assembler::Format2(std::istringstream& iss, uint8_t opcode) {
	std::string address;
	uint32_t  Register1, Register2;
	iss >> address;
	address = trimWhitespace(address);
	size_t comma_pos = address.find(',');

	if (comma_pos != std::string::npos) {
		// Extract substrings before and after the comma
		Register1 = RegisterMap[address.substr(0, comma_pos)];
		Register2 = RegisterMap[address.substr(comma_pos + 1)];
	}
	else {
		// If no comma, the whole string is "before_comma"
		Register1 = RegisterMap[address];
		Register2 = 0;
	}
	return (opcode << 8) | (Register1 << 4) | Register2;
}

uint32_t Assembler::Format1(std::istringstream& iss, uint8_t opcode) {


	return 0;
}

void Assembler::writeHexToAssembledFile(const Instruction& instr) {
	std::ofstream outputFile(outputFileName, std::ios::app);
	if (outputFile.is_open()) {
		outputFile << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << instr.address << " "
			<< std::setw(8) << instr.machineCode << "  " << instr.basicCode << "\n";
	}
	else {
		std::cerr << "Error writing to output file.\n";
	}
}

std::string Assembler::trimWhitespace(const std::string& str) {
	size_t start = str.find_first_not_of(" \t\n\r\f\v");
	size_t end = str.find_last_not_of(" \t\n\r\f\v");
	return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::string Assembler::writeTextRecord(uint32_t startAddress, const std::string& textData) {
	std::ostringstream textStream;
	textStream << std::hex << std::setw(6) << std::setfill('0') << startAddress
		<< std::setw(2) << (textData.size() / 2) // Record length in bytes
		<< textData;
	return textStream.str();
}

std::string Assembler::writeModificationRecord(uint32_t address) {
	std::ostringstream modStream;
	modStream << "M" << std::hex << std::setw(6) << std::setfill('0') << address
		<< "05"; // Length of the modification in half-bytes
	return modStream.str();
}

bool Assembler::requiresModification(const std::string& line) {
	// Check if the line contains a relocatable address
	return line.find("#") == std::string::npos && line.find("@") == std::string::npos;
}

uint32_t Assembler::calculateInstructionLength(const std::string& line) {
	std::istringstream iss(line);
	std::string label, opcode, operand;

	// Extract label if present
	if (!line.empty() && !(line[0] == ' ')) {
		iss >> label;
	}

	iss >> opcode; // Extract opcode
	opcode = trimWhitespace(opcode);

	if (opcode.empty()) {
		return 0; // No valid opcode
	}

	if (opcode[0] == '+') {
		return 4; // Format 4 instruction
	}
	else if (opcode.back() == 'R' || opcode == "RMO") {
		return 2; // Format 2 instruction
	}
	else if (opcodeMap.find(opcode) != opcodeMap.end()) {
		return 3; // Format 3 instruction
	}
	else if (opcode == "BASE" || opcode == "END" || opcode == "START") {
		return 0; // Directives have no instruction length
	}

	return 0; // Default case for unknown instructions
}

void Assembler::handleBYTE(const std::string& operand) {
	if (operand[0] == 'X') {
		std::string hexValue = operand.substr(2, operand.size() - 3);  // Remove 'X' and quotes
		for (char c : hexValue) {
			currentAddress += 1;
		}
	}
	else if (operand[0] == 'C') {
		std::string charValue = operand.substr(2, operand.size() - 3);  // Remove 'C' and quotes
		currentAddress += charValue.size();
	}
}
void Assembler::handleWORD(const std::string& operand) {
	// Convert the operand (number) to a 3-byte value
	currentAddress += 3; // Each WORD is 3 bytes
}
