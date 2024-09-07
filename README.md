# loaf-lang
Compiler for the Loaf programming language, compiling Loaf code to x86 assembly. 

Taking inspiration from [Pixeled](https://www.youtube.com/@pixeled-yt/videos)'s [Creating a Compiler](https://www.youtube.com/playlist?list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs), Robert Nystrom's [Crafting Interpreters](https://craftinginterpreters.com/), and just doing things by the seat of the pants

## Currently implemented
- Printing for integer literals and string literals
  ```python
  print 12
  print "hello world!"
  ```

## Try it yourself
You'll need a Linux machine with NASM that can write ELF64 object files.

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
