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
    _return,
    _int_literal,
    _string_literal,
    _semicolon,
};

class Token {
    public:
        TokenType type;
        std::string value;
    

    Token(TokenType type) : type(type), value("") {}
    Token(TokenType type, std::string value) : type(type), value(value) {}
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

std::vector<Token> getTokens(std::string fileName) {
    std::vector<Token> tokenList;

    std::string fileContent = getFileContent(fileName);
    std::string tokenBuffer;
    int currLine = 1;
    int i = 0;

    // iterate over all characters in file content
    while (i < fileContent.length()) {
        char c = fileContent.at(i);

        // beginning of identifier or keyword
        if (std::isalpha(c)) {
            tokenBuffer.push_back(c);
            i ++;
            while (std::isalnum(fileContent.at(i))) {
                tokenBuffer.push_back(fileContent.at(i));
                i ++;
            }
            if (tokenBuffer == "return") {
                tokenList.push_back(Token(TokenType::_return));
            } else {
                std::cerr << "unrecognized symbol " << "\"" << tokenBuffer << "\"" << " on line " << currLine << std::endl;
                exit(EXIT_FAILURE);
            }
            tokenBuffer.clear();
        }

        // beginning of int literal
        else if (std::isdigit(c)) {
            while (std::isdigit(fileContent.at(i))) {
                tokenBuffer.push_back(fileContent.at(i));
                i ++;
            }
            tokenList.push_back(Token(TokenType::_int_literal, tokenBuffer));
            tokenBuffer.clear();
        }

        else if (std::isspace(c)) {
            // newline increments line count
            // other whitespace ignored
            if (c == '\n') {
                currLine ++;
            }
            i ++;
        }

        // not alphanumeric or whitespace
        else {
            switch (c) {
                case ';':
                    tokenList.push_back(TokenType::_semicolon);
                    break;
                case '\"':
                    i ++;
                    while (fileContent.at(i) != '\"') {
                        tokenBuffer.push_back(fileContent.at(i));
                        i ++;
                    }
                    tokenList.push_back(Token(TokenType::_string_literal, tokenBuffer));
                    tokenBuffer.clear();
                    break;
                default:
                    std::cerr << "don't recognize token " << '\"' << c << '\"' << std::endl;
                    exit(EXIT_FAILURE);
            }
            i ++;
        }
    }

    return tokenList;
}

std::string tokens_to_asm(const std::vector<Token> tokens) {
    std::stringstream output;
    output << "global _start\n_start:\n";

    for (int i = 0; i < tokens.size(); i ++) {
        const Token token = tokens[i];

        if (token.type == TokenType::_return) {
            if (i + 2 >= tokens.size() || tokens[i + 2].type != TokenType::_semicolon || tokens[i + 1].type != TokenType::_int_literal) {
                std::cerr << "invalid syntax, correct syntax: return <return value>;" << std::endl;
                exit(EXIT_FAILURE);
            }
            output << "    mov rax, 60\n";
            output << "    mov rdi, " << tokens[i + 1].value << "\n";
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

    std::string fileName = argv[1];
    std::vector<Token> tokenList = getTokens(fileName);
    
    std::string asm_string = tokens_to_asm(tokenList);

    std::string outputFileName = "out.asm";
    std::ofstream outputFile(outputFileName);

    outputFile << asm_string;

    outputFile.close();

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}
