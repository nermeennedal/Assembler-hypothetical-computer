#pragma once
#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <string>
#include <vector>
#include <unordered_map>

struct Instruction {
	uint32_t address;
	uint32_t machineCode;
	std::string basicCode;
};
class Assembler {
private:
	std::string headerRecord;
	std::string textRecord;
	std::string modificationRecords;
	std::string endRecord;
	uint32_t calculateInstructionLength(const std::string& line);
	void handleBYTE(const std::string& operand);
	void handleWORD(const std::string& operand);

	uint32_t Format1(std::istringstream& iss, uint8_t opcode);
	uint32_t Format2(std::istringstream& iss, uint8_t opcode);
	uint32_t Format3(std::istringstream& iss, uint8_t opcode);
	uint32_t Format4(std::istringstream& iss, uint8_t opcode);
	uint32_t currentAddress;

	void Pass1();
	void Pass2();
	std::string writeTextRecord(uint32_t startAddress, const std::string& textData);
	std::string writeModificationRecord(uint32_t address);
	bool requiresModification(const std::string& line);

	//std::vector<Instruction> instructionSet;
	std::string inputFileName;
	std::string outputFileName;
	void writeHexToAssembledFile(const Instruction& instr);
	std::string trimWhitespace(const std::string& str);
public:
	Assembler(const std::string& inputFileName, const std::string& outputFileName);
	void assemble();
	uint32_t assembleInstruction(const std::string& instruction);

};
#endif
