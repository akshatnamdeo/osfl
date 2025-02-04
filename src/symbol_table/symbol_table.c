#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* st_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

Scope* scope_create(Scope* parent) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    if (!scope) return NULL;
    scope->symbols = NULL;
    scope->symbol_count = 0;
    scope->symbol_capacity = 0;
    scope->parent = parent;
    return scope;
}

void scope_destroy(Scope* scope) {
    if (!scope) return;
    /* free each symbol */
    for (size_t i = 0; i < scope->symbol_count; i++) {
        free(scope->symbols[i].name);
    }
    free(scope->symbols);
    free(scope);
}

static void scope_grow_if_needed(Scope* scope) {
    if (scope->symbol_count >= scope->symbol_capacity) {
        size_t newcap = (scope->symbol_capacity == 0) ? 8 : scope->symbol_capacity * 2;
        Symbol* newarr = (Symbol*)realloc(scope->symbols, newcap * sizeof(Symbol));
        if (!newarr) {
            /* handle allocation failure if you like */
            return;
        }
        scope->symbols = newarr;
        scope->symbol_capacity = newcap;
    }
}

bool scope_add_symbol(Scope* scope, const char* name, SymbolKind kind, int reg) {
    /* Check if already exists in this scope only */
    for (size_t i = 0; i < scope->symbol_count; i++) {
        if (strcmp(scope->symbols[i].name, name) == 0) {
            /* symbol already declared in this scope => error */
            return false;
        }
    }
    scope_grow_if_needed(scope);
    Symbol* s = &scope->symbols[scope->symbol_count++];
    s->name = st_strdup(name);
    s->kind = kind;
    s->reg = reg;   // Store the register number
    return true;
}

Symbol* scope_lookup(Scope* scope, const char* name) {
    /* search current scope */
    Scope* current = scope;
    while (current) {
        for (size_t i = 0; i < current->symbol_count; i++) {
            if (strcmp(current->symbols[i].name, name) == 0) {
                return &current->symbols[i];
            }
        }
        current = current->parent;
    }
    return NULL; /* not found in any parent scope */
}
