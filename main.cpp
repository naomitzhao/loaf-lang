/**
 * compiles loaf code into x86_64 assembly.
 * currently only works for printing integer literals and string literals.
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <optional>

enum TokenType {
    _int_literal,
    _string_literal,
    _newline,
    _equal,
    _equalequal,
    _print,
    _println,
    _int,
    _symbol,
};

const std::unordered_map<std::string, TokenType> stringToToken = {
    {"print", TokenType::_print},
    {"println", TokenType::_println},
    {"int", TokenType::_int},
};

// token types that should increase the indentation for the next line
const std::unordered_set<TokenType> indentationIncreasers = {
      
};

class Token {
    public:
        TokenType type;
        std::string value;

    Token(TokenType type) : type(type), value("") {}
    Token(TokenType type, std::string value) : type(type), value(value) {}
};

class Line {
    public:
        std::vector<Token> tokenList;
        int spaces;  // the number of spaces the line is indented with (scope)
        int row;  // the row number of the line in the plaintext of the file
    
    Line(std::vector<Token> tokenList, int spaces, int row) : tokenList(tokenList), spaces(spaces), row(row) {}
};

enum DataType {
    _int_type,
    _string_type,
    _null_type,
};

class Symbol {
    public:
        DataType type;
        std::string name;
        std::string value;
    
    Symbol(DataType type, std::string name, std::string value) : type(type), name(name), value(value) {}
    Symbol(DataType type, std::string name) : type(type), name(name), value("") {}
};

std::string getFileContent(std::string fileName) {
    std::ifstream file;
    file.open(fileName); // open file

    std::string currLine;

    if (!file.is_open()) {
        throw std::invalid_argument("The file " + fileName + " does not exist.");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string fileContent = buffer.str();
    if (fileContent[fileContent.size() - 1] != '\n') {
        fileContent += '\n';
    }

    file.close();  // close the file
    return fileContent;
}

std::vector<Line> getLines(std::string fileName) {
    std::vector<Token> tokenBuffer;
    std::vector<Line> lines;

    std::string fileContent = getFileContent(fileName);
    std::string stringBuffer;
    int currLine = 1;
    unsigned int i = 0;
    int spaces = 0;

    // iterate over all characters in file content
    while (i < fileContent.length()) {
        char c = fileContent.at(i);

        // beginning of identifier or keyword
        if (std::isalpha(c)) {
            stringBuffer.push_back(c);
            i ++;
            while (std::isalnum(fileContent.at(i))) {
                stringBuffer.push_back(fileContent.at(i));
                i ++;
            }
            if (stringToToken.contains(stringBuffer)) {
                tokenBuffer.push_back(Token(stringToToken.at(stringBuffer)));
            } else {
                tokenBuffer.push_back(Token(TokenType::_symbol, stringBuffer));
            }
            stringBuffer.clear();
        }

        // beginning of int literal
        else if (std::isdigit(c)) {
            while (std::isdigit(fileContent.at(i))) {
                stringBuffer.push_back(fileContent.at(i));
                i ++;
            }
            tokenBuffer.push_back(Token(TokenType::_int_literal, stringBuffer));
            stringBuffer.clear();
        }

        // loaf recognizes newline as end command
        else if (c == '\n') {
            if (tokenBuffer.size() != 0) {
                tokenBuffer.push_back(Token(TokenType::_newline));
                lines.push_back(Line(tokenBuffer, spaces, currLine));
            }
            spaces = 0;
            tokenBuffer.clear();
            currLine ++;
            i ++;
        }

        else if (std::isspace(c)) { 
            if (tokenBuffer.size() == 0) {
                spaces ++;
            }
            i ++;
        }

        // not alphanumeric or whitespace
        else {
            switch (c) {
                case '\"':
                    i ++;
                    while (fileContent.at(i) != '\"') {
                        stringBuffer.push_back(fileContent.at(i));
                        i ++;
                    }
                    tokenBuffer.push_back(Token(TokenType::_string_literal, stringBuffer));
                    stringBuffer.clear();
                    break;
                case '=':
                    i ++;
                    if (fileContent.at(i) == '=') {
                        tokenBuffer.push_back(Token(TokenType::_equalequal));
                        i ++;
                    } else {
                        tokenBuffer.push_back(Token(TokenType::_equal));
                    }
                    break;
                default:
                    std::cerr << "don't recognize token " << '\"' << c << '\"' << std::endl;
                    exit(EXIT_FAILURE);
            }
            i ++;
        }
    }

    if (tokenBuffer.size() != 0) {
        if (tokenBuffer.size() != 0) {
            lines.push_back(Line(tokenBuffer, spaces, currLine));
        }
        spaces = 0;
        tokenBuffer.clear();
    }

    return lines;
}

// check if the current line has the correct indentation by comparing to the last line
bool has_correct_indentation(const std::vector<Line> lines, const int lineIdx, const int lastIndentation) {
    if (lineIdx > 0) {
        const int currSpaces = lines[lineIdx].spaces;
        if (indentationIncreasers.contains(lines[lineIdx].tokenList[0].type)) {
            if (lines[lineIdx].spaces <= lastIndentation) {
                std::cerr << "invalid syntax on line " << lines[lineIdx].row << ". indentation expected" << std::endl;
                return false;
            }
        } else if (currSpaces > lastIndentation) {
            std::cerr << "invalid syntax on line " << lines[lineIdx].row << ". unexpected indentation" << std::endl;
        } 
    } else if (lines[lineIdx].spaces != 0) {
        std::cerr << "invalid syntax on line " << lines[lineIdx].row << ". unexpected indentation" << std::endl;
        return false;
    }
    return true;
}

std::string lines_to_asm(const std::vector<Line> lines) {
    bool foundError = false;
    int dataId = 0;
    std::stringstream textSection;
    std::stringstream dataSection;

    std::stack<int> indentations;  // indentations we are within
    indentations.push(0);  // program starts with 0 indentation
    
    std::unordered_map<std::string, Symbol> symbols;  // key: symbol name, value: symbol
    std::unordered_map<std::string, std::string> registers = {  // key: register name, value: symbol name
        {"eax", ""}, 
        {"ebx", ""}, 
        {"ecx", ""}, 
        {"edx", ""}, 
        {"esi", ""}, 
        {"edi", ""}, 
    };  

    std::stack<std::string> availableRegisters;
    for (std::unordered_map<std::string, std::string>::iterator iter = registers.begin(); iter != registers.end(); iter ++) {
        availableRegisters.push(iter->first);
    }

    textSection << "global _start\n\nsection .text\n\n_start:\n";
    dataSection << "section .data\n";

    for (unsigned int lineIdx = 0; lineIdx < lines.size(); lineIdx ++) {
        // check for correct indentation
        if (!has_correct_indentation(lines, lineIdx, indentations.top())) {
            foundError = true;
        }

        if (lines[lineIdx].spaces > indentations.top()) {
            indentations.push(lines[lineIdx].spaces);
        }
        while (lines[lineIdx].spaces < indentations.top()) {
            indentations.pop();
        }

        const std::vector<Token> tokenList = lines[lineIdx].tokenList;

        // print or println
        if (tokenList[0].type == TokenType::_print || tokenList[0].type == TokenType::_println) {
            if (tokenList.size() != 3) {
                std::cerr << "invalid syntax on line " << lineIdx << ". correct syntax: print <integer literal or string literal>" << std::endl;
                foundError = true;
            }

            std::string stringValue;
            stringValue = tokenList[1].value;

            textSection << "    mov eax, 4\n";
            textSection << "    mov ebx, 1\n";
            textSection << "    mov ecx, item" << dataId << "\n";
            textSection << "    mov edx, item" << dataId << "_length\n";
            textSection << "    int 0x80\n";

            dataSection << "    item" << dataId << ": db \"" << tokenList[1].value << "\"";

            if (tokenList[0].type == TokenType::_println) {
                dataSection << ", 0xA";
            }

            dataSection << "\n";

            dataSection << "    item" << dataId << "_length equ $-item" << dataId << "\n";
            dataId ++;
        }

        // declaration of integer variable
        else if (tokenList[0].type == TokenType::_int) {
            Token symbolToken = tokenList[1];

            if (tokenList.size() != 5 || tokenList[1].type != TokenType::_symbol || tokenList[2].type != TokenType::_equal || tokenList[3].type != TokenType::_int_literal) {
                std::cerr << "invalid syntax on line " << lineIdx << ". correct syntax: int <variable_name> = <int literal>" << std::endl;
                foundError = true;
                continue;
            }

            if (symbols.contains(symbolToken.value)) {
                std::cerr << "error on line " << lineIdx << ": symbol " << symbolToken.value << " already declared previously" << std::endl;
                foundError = true;
                continue;
            }

            if (availableRegisters.empty()) {
                std::cerr << "error on line " << lineIdx << ": not enough registers. current max number of variables is 6" << std::endl;
                foundError = true;
                continue;
            }

            std::string chosenRegister = availableRegisters.top();
            availableRegisters.pop();

            symbols.insert({symbolToken.value, Symbol(DataType::_int_type, symbolToken.value, tokenList[3].value)});
            registers.insert({chosenRegister, symbolToken.value});
            textSection << "    mov " << chosenRegister << ", " << tokenList[3].value << "\n";
        }
    }

    if (foundError) {
        exit(EXIT_FAILURE);
    }

    textSection << "    mov eax, 1\n";
    textSection << "    mov ebx, 0\n";
    textSection << "    int 0x80\n";
    return textSection.str() + "\n" + dataSection.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: ./loaf <file.loaf>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string fileName = argv[1];
    std::vector<Line> lines = getLines(fileName);

    // for (Line line : lines) {
    //     for (Token token : line.tokenList) {
    //         std::cout << "(" << token.type << ", " << token.value << ") ";
    //     }
    //     std::cout << std::endl;
    // }
    
    std::string asm_string = lines_to_asm(lines);

    std::string outputFileName = "out.asm";
    std::ofstream outputFile(outputFileName);

    outputFile << asm_string;

    outputFile.close();

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
