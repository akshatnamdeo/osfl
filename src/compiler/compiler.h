#ifndef COMPILER_H
#define COMPILER_H

#include "../../include/ast.h"   /* Adjust the path as needed */
#include "bytecode.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compile the given AST node (e.g. a root program node) into Bytecode.
 *
 * @param root The root of the AST (like a program or block)
 * @return Bytecode* A newly allocated Bytecode struct with instructions
 */
Bytecode* compiler_compile_ast(AstNode* root);

#ifdef __cplusplus
}
#endif

#endif /* COMPILER_H */
