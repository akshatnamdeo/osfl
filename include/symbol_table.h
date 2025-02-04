#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Kinds of symbols in the language
 */
typedef enum {
    SYMBOL_VAR,
    SYMBOL_CONST,
    SYMBOL_FUNC,
    SYMBOL_CLASS
} SymbolKind;

/**
 * @brief Symbol information structure
 */
typedef struct Symbol {
    char* name;
    SymbolKind kind;
    int reg;    // <-- New: the register number assigned to this symbol
    /* optionally store type info, or pointer to AST node, etc. */
} Symbol;

/**
 * @brief Represents a scope with local symbols
 */
typedef struct Scope {
    Symbol* symbols;
    size_t symbol_count;
    size_t symbol_capacity;
    struct Scope* parent; /* for nested scopes */
} Scope;

/**
 * Create a new scope with the given parent
 */
Scope* scope_create(Scope* parent);

/**
 * Destroy a scope (recursively frees all symbols).
 */
void scope_destroy(Scope* scope);

/**
 * Add a symbol to the current scope
 * @return true if success, false if symbol already exists
 */
bool scope_add_symbol(Scope* scope, const char* name, SymbolKind kind, int reg);

/**
 * Lookup a symbol in the current scope or any parent
 * @return pointer to Symbol, or NULL if not found
 */
Symbol* scope_lookup(Scope* scope, const char* name);

#endif /* SYMBOL_TABLE_H */
