#include "Interpreter.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

std::vector<std::string> Interpreter::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (ss >> token) {
        if (!token.empty() && token.back() == ',') {
            token.pop_back();
        }
        tokens.push_back(token);
    }
    return tokens;
}

uint8_t Interpreter::parseRegister(const std::string& regStr) {
    std::string cleanReg = regStr;
    if (cleanReg.front() == '[') cleanReg.erase(0, 1);
    if (cleanReg.back() == ']')  cleanReg.pop_back();

    if (cleanReg.size() >= 2 && cleanReg[0] == 'R') {
        int regNum = cleanReg[1] - '0';
        if (regNum >= 0 && regNum < 8) return static_cast<uint8_t>(regNum);
    }
    return 0; 
}

std::vector<uint16_t> Interpreter::parseLine(const std::string& line) {
    size_t commentPos = line.find("//");
    std::string cleanLine = (commentPos != std::string::npos) ? line.substr(0, commentPos) : line;
    
    std::vector<std::string> tokens = tokenize(cleanLine);
    if (tokens.empty()) return {0x0000}; // NOP
    
    std::string mnemonic = tokens[0];
    std::vector<uint16_t> payload;
    
    if (mnemonic == "AARAM" || mnemonic == "NOP") {
        payload.push_back(0x0000);
    }
    else if (mnemonic == "JAMA" || mnemonic == "ADD") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]);
        payload.push_back((0x01 << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "TAFREEK" || mnemonic == "SUB") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]);
        payload.push_back((0x02 << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "ZARAB" || mnemonic == "MUL") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]);
        payload.push_back((0x03 << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "TAQSEEM" || mnemonic == "DIV") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]);
        payload.push_back((0x04 << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "MUWAZANA" || mnemonic == "CMP") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]);
        payload.push_back((0x0A << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "CHHALANG" || mnemonic == "JMP") { 
        uint8_t src  = parseRegister(tokens[1]); 
        payload.push_back((0x10 << 8) | (src & 0x0F));
    }
    else if (mnemonic == "AGAR_SIFAR" || mnemonic == "JZ") { 
        uint8_t src  = parseRegister(tokens[1]);
        payload.push_back((0x11 << 8) | (src & 0x0F));
    }
    else if (mnemonic == "AGAR_MAUJOOD" || mnemonic == "JNZ") { 
        uint8_t src  = parseRegister(tokens[1]);
        payload.push_back((0x12 << 8) | (src & 0x0F));
    }
    else if (mnemonic == "BHARO" || mnemonic == "DAALO" || mnemonic == "LDR_IMM") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint16_t immFull = 0;
        
        bool isNumber = isdigit(tokens[2][0]) || (tokens[2].size() > 1 && tokens[2][1] == 'x');
        if (isNumber) {
            try {
                immFull = std::stoi(tokens[2], nullptr, 0); 
            } catch (...) {
                std::cout << "[COMPILER ERROR] Invalid immediate format: " << tokens[2] << std::endl;
            }
        } else {
            if (labelMap.find(tokens[2]) != labelMap.end()) {
                immFull = labelMap[tokens[2]];
            } else {
                std::cout << "[COMPILER ERROR] Unresolved Label: " << tokens[2] << std::endl;
            }
        }
        
        // If it's a Label, it was tracked as 4 bytes in Pass 1, so we must force Format D
        if (!isNumber || immFull > 0x0F) {
            // Requires 2-word 16-bit payload (Format D)
            payload.push_back((0x1B << 8) | ((dest & 0x0F) << 4)); // Header word
            payload.push_back(immFull);                            // Payload word
        } else {
            // Fits in 4 bits (Format B)
            payload.push_back((0x1A << 8) | ((dest & 0x0F) << 4) | (immFull & 0x0F));
        }
    }
    else if (mnemonic == "PARHO" || mnemonic == "LDR") { 
        uint8_t dest = parseRegister(tokens[1]);
        uint8_t src  = parseRegister(tokens[2]); 
        payload.push_back((0x20 << 8) | ((dest & 0x0F) << 4) | (src & 0x0F));
    }
    else if (mnemonic == "RAKHO" || mnemonic == "STR") { 
        uint8_t srcVal = parseRegister(tokens[1]);
        uint8_t ptrReg = parseRegister(tokens[2]); 
        payload.push_back((0x21 << 8) | ((srcVal & 0x0F) << 4) | (ptrReg & 0x0F));
    }
    else {
        std::cout << "[COMPILER ERROR] Unrecognized Mnemonic: " << mnemonic << std::endl;
    }
    
    return payload;
}

CompilationResult Interpreter::compile(const std::string& sourceCode) {
    CompilationResult result;
    std::stringstream ss(sourceCode);
    std::string line;
    
    // storing pairs of {type, content}
    // type 0 = code, type 1 = data
    std::vector<std::pair<int, std::string>> processLines;
    
    labelMap.clear();
    uint16_t currentCodePC = 0;
    uint16_t currentDataPC = 0x0800;
    bool inDataSection = false;
    
    // Pass 1: PC Tracking and Label Mapping
    while (std::getline(ss, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty() || line.find("//") == 0) continue; 
        
        size_t commentPos = line.find("//");
        std::string cleanLine = (commentPos != std::string::npos) ? line.substr(0, commentPos) : line;
        cleanLine.erase(cleanLine.find_last_not_of(" \t\r\n") + 1);
        
        if (cleanLine == ".MAWAAD" || cleanLine == ".DATA" || cleanLine == ".mawaad" || cleanLine == ".data") {
            inDataSection = true;
            continue;
        }
        if (cleanLine == ".HIDAYAT" || cleanLine == ".CODE" || cleanLine == ".hidayat" || cleanLine == ".code") {
            inDataSection = false;
            continue;
        }
        
        if (inDataSection) {
            size_t colonPos = cleanLine.find(':');
            if (colonPos != std::string::npos) {
                std::string labelName = cleanLine.substr(0, colonPos);
                labelName.erase(labelName.find_last_not_of(" \t") + 1);
                labelMap[labelName] = currentDataPC;
                
                std::string valueStr = cleanLine.substr(colonPos + 1);
                valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                
                processLines.push_back({1, valueStr});
                
                if (!valueStr.empty() && valueStr.front() == '"') {
                    // String literal
                    int chars = 0;
                    for (size_t i = 1; i < valueStr.size() && valueStr[i] != '"'; ++i) {
                        if (valueStr[i] == '\\' && i + 1 < valueStr.size()) {
                            i++; // escape sequence
                        }
                        chars++;
                    }
                    int words = (chars + 1) / 2;
                    currentDataPC += words * 2;
                } else {
                    // Integer array or single int
                    int commas = std::count(valueStr.begin(), valueStr.end(), ',');
                    int words = commas + 1;
                    currentDataPC += words * 2;
                }
            }
        } else {
            if (cleanLine.back() == ':') {
                std::string labelName = cleanLine.substr(0, cleanLine.size() - 1);
                labelMap[labelName] = currentCodePC;
                continue; // Label takes 0 bytes
            }
            
            processLines.push_back({0, cleanLine});
            
            std::vector<std::string> tokens = tokenize(cleanLine);
            if (tokens.empty()) continue;
            
            // Calculate instruction byte size
            std::string mnemonic = tokens[0];
            if (mnemonic == "BHARO" || mnemonic == "DAALO" || mnemonic == "LDR_IMM") {
                if (tokens.size() > 2) {
                    bool isNumber = isdigit(tokens[2][0]) || (tokens[2].size() > 1 && tokens[2][1] == 'x');
                    if (isNumber) {
                        try {
                            uint16_t immFull = std::stoi(tokens[2], nullptr, 0); 
                            if (immFull <= 0x0F) currentCodePC += 2;
                            else currentCodePC += 4; // Format D
                        } catch (...) {
                            currentCodePC += 4; // Fallback
                        }
                    } else {
                        currentCodePC += 4; // Labels unconditionally 2 words
                    }
                } else {
                    currentCodePC += 2;
                }
            } else {
                currentCodePC += 2; // Fixed width defaults
            }
        }
    }
    
    // Pass 2: Instruction Generation
    for (const auto& item : processLines) {
        if (item.first == 1) { // DATA
            std::string valueStr = item.second;
            if (!valueStr.empty() && valueStr.front() == '"') {
                std::vector<uint8_t> bytes;
                for (size_t i = 1; i < valueStr.size() && valueStr[i] != '"'; ++i) {
                    if (valueStr[i] == '\\' && i + 1 < valueStr.size()) {
                        i++;
                        if (valueStr[i] == '0') bytes.push_back(0);
                        else if (valueStr[i] == 'n') bytes.push_back('\n');
                        else bytes.push_back(valueStr[i]);
                    } else {
                        bytes.push_back(valueStr[i]);
                    }
                }
                if (bytes.size() % 2 != 0) bytes.push_back(0); // Word alignment
                for (size_t i = 0; i < bytes.size(); i += 2) {
                    result.dataSegment.push_back((bytes[i] << 8) | bytes[i+1]);
                }
            } else {
                std::stringstream vss(valueStr);
                std::string token;
                while (std::getline(vss, token, ',')) {
                    token.erase(0, token.find_first_not_of(" \t"));
                    token.erase(token.find_last_not_of(" \t") + 1);
                    try {
                        uint16_t val = std::stoi(token, nullptr, 0);
                        result.dataSegment.push_back(val);
                    } catch (...) {
                        result.dataSegment.push_back(0);
                    }
                }
            }
        } else { // CODE
            std::vector<uint16_t> words = parseLine(item.second);
            for (uint16_t word : words) {
                result.codeSegment.push_back(word);
            }
        }
    }
    
    return result;
}

void Interpreter::loadProgramAndFlash(const std::string& filename, MemoryModule& ram) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "[BIOS FAULT] Could not locate OS payload: " << filename << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string targetOS = buffer.str();
    
    CompilationResult compiledOS = compile(targetOS);
    std::cout << "[BIOS] Compiled OS script into " << compiledOS.codeSegment.size() << " native 16-bit instruction words." << std::endl;

    std::cout << "[BOOTLOADER] Flashing Code Execution block to RAM Address 0x0000..." << std::endl;
    uint16_t codeAddress = 0x0000;
    for (uint16_t word : compiledOS.codeSegment) {
        ram.loadRawBinary(codeAddress, word);
        codeAddress += 2;
    }

    std::cout << "[BOOTLOADER] Flashing Data Variables (.MAWAAD) to RAM Address 0x0800..." << std::endl;
    uint16_t dataAddress = 0x0800;
    for (uint16_t word : compiledOS.dataSegment) {
        ram.loadRawBinary(dataAddress, word);
        dataAddress += 2;
    }
    
    std::cout << "[BOOTLOADER] Flash Complete." << std::endl;
    std::cout << "------------------------------------------------------------\n" << std::endl;
}
