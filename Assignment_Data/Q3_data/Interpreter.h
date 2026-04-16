#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include "motherboard/MemoryModule.h"

struct CompilationResult {
    std::vector<uint16_t> codeSegment;
    std::vector<uint16_t> dataSegment;
};

class Interpreter {
public:
    Interpreter() {}
    
    CompilationResult compile(const std::string& sourceCode);
    void loadProgramAndFlash(const std::string& filename, MemoryModule& ram);
    
private:
    std::map<std::string, uint16_t> labelMap;
    std::vector<uint16_t> parseLine(const std::string& line);
    uint8_t parseRegister(const std::string& regStr);
    
    std::vector<std::string> tokenize(const std::string& line);
};
