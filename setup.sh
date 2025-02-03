#!/bin/bash

# Create main project directories
mkdir -p src/{lexer,parser,vm,compiler}
mkdir -p test
mkdir -p examples/{basic,advanced}
mkdir -p include
mkdir -p lib

# Create source files
touch src/main.c

# Lexer files
touch src/lexer/lexer.h
touch src/lexer/lexer.c
touch src/lexer/token.h

# Parser files
touch src/parser/parser.h
touch src/parser/parser.c
touch src/parser/ast.h

# VM files
touch src/vm/frame.h
touch src/vm/frame.c
touch src/vm/memory.h
touch src/vm/memory.c
touch src/vm/vm.h
touch src/vm/vm.c

# Compiler files
touch src/compiler/compiler.h
touch src/compiler/compiler.c
touch src/compiler/bytecode.h

# Test files
touch test/test_lexer.c
touch test/test_parser.c
touch test/test_vm.c
touch test/test_compiler.c

# Include files
touch include/osfl.h

# Create initial Makefile
touch Makefile

# Set up basic .gitignore
echo "# Build artifacts
*.o
*.a
*.so
*.exe

# Build directories
/lib/
/build/

# Editor files
.vscode/
.idea/
*.swp
*~

# OS-specific files
.DS_Store
Thumbs.db" > .gitignore

# Make the script executable
chmod +x setup.sh

echo "Project structure created successfully!"

# Print the directory structure
echo -e "\nProject structure:"
tree