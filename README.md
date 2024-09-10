# loaf-lang
Compiler for the Loaf programming language which compiles Loaf code into x86 assembly. 

Taking inspiration from [Pixeled](https://www.youtube.com/@pixeled-yt/videos)'s [Creating a Compiler](https://www.youtube.com/playlist?list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs), Robert Nystrom's [Crafting Interpreters](https://craftinginterpreters.com/), and.. just doing things by the knowledge I have from ECS50 (assembler course)

## Currently implemented
- Printing for integer literals and string literals: print, or println to add newline to end
  ```
  print 12
  println "hello world!"
  ```

## Try it yourself
You'll need a Linux machine with GNU that can assemble ELF for i386.

Simply type ```make``` to run the Makefile, then ```./loaf <filename>``` to compile your Loaf code, then ```./out``` to run the executable.
```bash
make
./loaf test.loaf
./out
```

## Goals
- Python-esque language: whitespace matters
- Static typing
- Main goal is to make a compiler that works correctly :)
