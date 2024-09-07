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
#include <optional>

enum TokenType {
    _int_literal,
    _string_literal,
    _newline,
    _print,
    _println,
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
            if (stringBuffer == "print") {
                tokenBuffer.push_back(Token(TokenType::_print));
            } else if (stringBuffer == "println") {
                tokenBuffer.push_back(Token(TokenType::_println));
            } else {
                std::cerr << "unrecognized symbol " << "\"" << stringBuffer << "\"" << " on line " << currLine << std::endl;
                exit(EXIT_FAILURE);
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

std::string lines_to_asm(const std::vector<Line> lines) {
    int dataId = 0;
    std::stringstream textSection;
    std::stringstream dataSection;
    textSection << "global _start\n\nsection .text\n\n_start:\n";
    dataSection << "section .data\n";

    for (unsigned int lineIdx = 0; lineIdx < lines.size(); lineIdx ++) {
        const std::vector<Token> tokenList = lines[lineIdx].tokenList;
        if (tokenList[0].type == TokenType::_print || tokenList[0].type == TokenType::_println) {
            if (tokenList.size() != 3 || tokenList[2].type != TokenType::_newline || (tokenList[1].type != TokenType::_string_literal && tokenList[1].type != TokenType::_int_literal)) {
                std::cerr << "invalid syntax on line " << lineIdx << ". correct syntax: print <integer literal or string literal>" << std::endl;
                exit(EXIT_FAILURE);
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

            if (lineIdx < lines.size() - 1 && lines[lineIdx + 1].spaces > lines[lineIdx].spaces) {
                std::cerr << "invalid syntax on line " << lines[lineIdx + 1].row << ". unexpected indentation" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
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
    
    std::string asm_string = lines_to_asm(lines);

    std::string outputFileName = "out.asm";
    std::ofstream outputFile(outputFileName);

    outputFile << asm_string;

    outputFile.close();

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
