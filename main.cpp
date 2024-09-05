/**
 * compiles loaf code into x86_64 assembly.
 * currently only works for return <int>; instructions haha
 * uses system() to call nasm with elf64 format so you need nasm on linux
 */

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <optional>

enum TokenType {
    _exit,
    _int_literal,
    _string_literal,
    _newline,
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
    
    Line(std::vector<Token> tokenList, int spaces) : tokenList(tokenList), spaces(spaces) {}
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

    file.close();  // close the file
    return fileContent;
}

std::vector<Line> getLines(std::string fileName) {
    std::vector<Token> tokenBuffer;
    std::vector<Line> lines;

    std::string fileContent = getFileContent(fileName);
    std::string stringBuffer;
    int currLine = 1;
    int i = 0;

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
            if (stringBuffer == "exit") {
                tokenBuffer.push_back(Token(TokenType::_exit));
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
            tokenBuffer.push_back(Token(TokenType::_newline));
            lines.push_back(Line(tokenBuffer, 0));
            tokenBuffer.clear();
            currLine ++;
            i ++;
        }

        else if (std::isspace(c)) {  // TODO: count spaces before getting other tokens
            // other whitespace ignored for now
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
            std::cout << i << std::endl;
        }
    }

    if (tokenBuffer.size() != 0) {
        lines.push_back(Line(tokenBuffer, 0));
        tokenBuffer.clear();
    }

    return lines;
}

std::string lines_to_asm(const std::vector<Line> lines) {
    std::stringstream output;
    output << "global _start\n_start:\n";

    for (int lineIdx = 0; lineIdx < lines.size(); lineIdx ++) {
        const std::vector<Token> tokenList = lines[lineIdx].tokenList;
        if (tokenList[0].type == TokenType::_exit) {
            if (tokenList.size() != 3 || tokenList[2].type != TokenType::_newline || tokenList[1].type != TokenType::_int_literal) {
                std::cerr << "invalid syntax, correct syntax: exit <exit value>;" << std::endl;
                exit(EXIT_FAILURE);
            }
            output << "    mov rax, 60\n";
            output << "    mov rdi, " << tokenList[1].value << "\n";
            output << "    syscall\n";
        }
    }

    return output.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: ./loaf <file.loaf>" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "hi" << std::endl;

    std::string fileName = argv[1];
    std::vector<Line> lines = getLines(fileName);

    for (int i = 0; i < lines.size(); i ++) {
        const std::vector<Token> tokenList = lines[i].tokenList;
        for (int j = 0; j < tokenList.size(); j ++) {
            std::cout << tokenList[j].type << " " << tokenList[j].value << ", ";
        }
        std::cout << std::endl;
    }
    
    std::string asm_string = lines_to_asm(lines);

    std::string outputFileName = "out.asm";
    std::ofstream outputFile(outputFileName);

    outputFile << asm_string;

    outputFile.close();

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
