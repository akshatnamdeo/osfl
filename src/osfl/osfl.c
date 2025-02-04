#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>  // for _getcwd
#define getcwd _getcwd
#include <errno.h>

#include "../../include/osfl.h"
#include "../lexer/lexer.h"
#include "../lexer/token.h"
#include "../parser/parser.h"
#include "../../include/ast.h"
#include "../../include/semantic.h"
#include "../../include/symbol_table.h"
#include "../compiler/compiler.h"
#include "../compiler/bytecode.h"
#include "../vm/vm.h"
#include "../vm/frame.h"    /* For frame_create/destroy if needed */
#include "../runtime/runtime.h" /* If you have a runtime layer */
#include <excpt.h>

/* ------------------------------------------------------------------
    Global error storage
------------------------------------------------------------------ */
static OSFLError g_osfl_last_error = {
    .code = OSFL_SUCCESS,
    .message = "",
    .file = NULL,
    .line = 0,
    .column = 0
};

/* Optionally store the current OSFLConfig globally, or not */
static OSFLConfig g_osfl_current_config;

/* ------------------------------------------------------------------
    Helper for setting an error in g_osfl_last_error
------------------------------------------------------------------ */
static void set_osfl_error(OSFLStatus code, const char* msg, const char* file, size_t line, size_t column) {
    g_osfl_last_error.code = code;
    strncpy(g_osfl_last_error.message, msg, sizeof(g_osfl_last_error.message) - 1);
    g_osfl_last_error.file = file;
    g_osfl_last_error.line = line;
    g_osfl_last_error.column = column;
}

/* ------------------------------------------------------------------
    Core API Implementations
------------------------------------------------------------------ */

/**
 * Initialize the OSFL system
 */
OSFLStatus osfl_init(const OSFLConfig* config) {
    fprintf(stderr, "DEBUG: Entering osfl_init\n");

    /* Clear error */
    osfl_clear_error();

    if (!config) {
        fprintf(stderr, "DEBUG: NULL config in osfl_init\n");
        set_osfl_error(OSFL_ERROR_INVALID_INPUT, "Null config in osfl_init", __FILE__, __LINE__, 0);
        return OSFL_ERROR_INVALID_INPUT;
    }

    fprintf(stderr, "DEBUG: Copying config\n");
    g_osfl_current_config = *config;

    fprintf(stderr, "DEBUG: Verifying config copy\n");
    fprintf(stderr, "DEBUG: Config values after copy:\n");
    fprintf(stderr, "  tab_width: %zu\n", g_osfl_current_config.tab_width);
    fprintf(stderr, "  include_comments: %s\n", g_osfl_current_config.include_comments ? "true" : "false");
    fprintf(stderr, "  input_file: %s\n", g_osfl_current_config.input_file ? g_osfl_current_config.input_file : "NULL");
    fprintf(stderr, "  debug_mode: %s\n", g_osfl_current_config.debug_mode ? "true" : "false");

    fprintf(stderr, "DEBUG: osfl_init completed successfully\n");
    return OSFL_SUCCESS;
}

/**
 * Clean up OSFL
 */
void osfl_cleanup(void) {
    /* If you had any global allocations, free them here. 
       For now, we do nothing. */
}

/**
 * Return the OSFL version string
 */
const char* osfl_version(void) {
    return OSFL_VERSION_STRING; /* e.g. "0.1.0" */
}

/**
 * Return the last error
 */
const OSFLError* osfl_get_last_error(void) {
    return &g_osfl_last_error;
}

/**
 * Clear the last error
 */
void osfl_clear_error(void) {
    g_osfl_last_error.code = OSFL_SUCCESS;
    g_osfl_last_error.message[0] = '\0';
    g_osfl_last_error.file = NULL;
    g_osfl_last_error.line = 0;
    g_osfl_last_error.column = 0;
}

/**
 * The main "run a file" pipeline:
 *  1) read file
 *  2) lex => tokens
 *  3) parse => AST
 *  4) semantic analysis
 *  5) compile => Bytecode
 *  6) vm_create => vm_run
 */
OSFLStatus osfl_run_file(const char* filename) {
    osfl_clear_error();

    if (!filename) {
        set_osfl_error(OSFL_ERROR_INVALID_INPUT, "No filename provided to osfl_run_file", __FILE__, __LINE__, 0);
        return OSFL_ERROR_INVALID_INPUT;
    }

    char* source = NULL;
    Token* tokens = NULL;
    Lexer* lexer = NULL;
    Parser* parser = NULL;
    AstNode* root = NULL;
    Bytecode* bc = NULL;
    VM* vm = NULL;
    OSFLStatus status = OSFL_SUCCESS;

    __try {
        /* 1) Read file into memory */
        FILE* fp = fopen(filename, "rb");
        if (!fp) {
            char errbuf[128];
            snprintf(errbuf, sizeof(errbuf), "Could not open file '%s'", filename);
            set_osfl_error(OSFL_ERROR_FILE_IO, errbuf, __FILE__, __LINE__, 0);
            status = OSFL_ERROR_FILE_IO;
            goto cleanup;
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        source = (char*)malloc(size + 1);
        if (!source) {
            fclose(fp);
            set_osfl_error(OSFL_ERROR_MEMORY_ALLOCATION, "Out of memory reading file", __FILE__, __LINE__, 0);
            status = OSFL_ERROR_MEMORY_ALLOCATION;
            goto cleanup;
        }
        size_t read_size = fread(source, 1, size, fp);
        source[read_size] = '\0';
        fclose(fp);

        /* 2) Lex => tokens */
        LexerConfig lex_cfg = lexer_default_config();
        lex_cfg.include_comments = g_osfl_current_config.include_comments;
        lex_cfg.file_name = filename;
        lexer = lexer_create(source, read_size, lex_cfg);
        if (!lexer) {
            free(source);
            set_osfl_error(OSFL_ERROR_LEXER, "Failed to create lexer", __FILE__, __LINE__, 0);
            status = OSFL_ERROR_LEXER;
            goto cleanup;
        }

        size_t tokens_capacity = 1024;
        tokens = (Token*)malloc(tokens_capacity * sizeof(Token));
        if (!tokens) {
            set_osfl_error(OSFL_ERROR_MEMORY_ALLOCATION, "Failed to allocate token buffer", __FILE__, __LINE__, 0);
            status = OSFL_ERROR_MEMORY_ALLOCATION;
            goto cleanup;
        }

        size_t token_count = 0;
        while (true) {
            if (token_count >= tokens_capacity) {
                tokens_capacity *= 2;
                Token* new_tokens = (Token*)realloc(tokens, tokens_capacity * sizeof(Token));
                if (!new_tokens) {
                    set_osfl_error(OSFL_ERROR_MEMORY_ALLOCATION, "Failed to grow token buffer", __FILE__, __LINE__, 0);
                    status = OSFL_ERROR_MEMORY_ALLOCATION;
                    goto cleanup;
                }
                tokens = new_tokens;
            }

            Token t = lexer_next_token(lexer);
            tokens[token_count++] = t;
            if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR) {
                break;
            }
        }

        LexerError lexError = lexer_get_error(lexer);
        if (lexError.type != LEXER_ERROR_NONE) {
            OSFLStatus sc = OSFL_ERROR_LEXER;
            set_osfl_error(sc, lexError.message, lexError.location.file, lexError.location.line, lexError.location.column);
            lexer_destroy(lexer);
            free(source);
            status = sc;
            goto cleanup;
        }

        /* 3) Parse => AST */
        parser = parser_create(tokens, token_count);
        root = parser_parse(parser);
        parser_destroy(parser);

        /* 4) Semantic analysis */
        SemanticContext sem_ctx;
        semantic_init(&sem_ctx);
        semantic_analyze(root, &sem_ctx);
        if (sem_ctx.error_count > 0) {
            set_osfl_error(OSFL_ERROR_SYNTAX, "Semantic errors occurred", __FILE__, __LINE__, 0);
            semantic_cleanup(&sem_ctx);
            ast_destroy(root);
            lexer_destroy(lexer);
            free(source);
            status = OSFL_ERROR_SYNTAX;
            goto cleanup;
        }
        semantic_cleanup(&sem_ctx);

        /* 5) Compile => bytecode */
        bc = compiler_compile_ast(root);
        if (!bc) {
            set_osfl_error(OSFL_ERROR_COMPILER, "Failed to compile AST", __FILE__, __LINE__, 0);
            ast_destroy(root);
            lexer_destroy(lexer);
            free(source);
            status = OSFL_ERROR_COMPILER;
            goto cleanup;
        }

        /* 6) Create VM */
        vm = vm_create(bc);
        if (!vm) {
            set_osfl_error(OSFL_ERROR_VM, "Failed to create VM", __FILE__, __LINE__, 0);
            bytecode_destroy(bc);
            ast_destroy(root);
            lexer_destroy(lexer);
            free(source);
            status = OSFL_ERROR_VM;
            goto cleanup;
        }

        /* Register native functions */
        vm_register_native(vm, "print", osfl_print);
        vm_register_native(vm, "split", osfl_split);
        vm_register_native(vm, "join", osfl_join);
        vm_register_native(vm, "substring", osfl_substring);
        vm_register_native(vm, "replace", osfl_replace);
        vm_register_native(vm, "to_upper", osfl_to_upper);
        vm_register_native(vm, "to_lower", osfl_to_lower);
        vm_register_native(vm, "len", osfl_len);
        vm_register_native(vm, "append", osfl_append);
        vm_register_native(vm, "pop", osfl_pop);
        vm_register_native(vm, "insert", osfl_insert);
        vm_register_native(vm, "remove", osfl_remove);
        vm_register_native(vm, "sqrt", osfl_sqrt);
        vm_register_native(vm, "pow", osfl_pow);
        vm_register_native(vm, "sin", osfl_sin);
        vm_register_native(vm, "cos", osfl_cos);
        vm_register_native(vm, "tan", osfl_tan);
        vm_register_native(vm, "log", osfl_log);
        vm_register_native(vm, "abs", osfl_abs);
        vm_register_native(vm, "int", osfl_int);
        vm_register_native(vm, "float", osfl_float);
        vm_register_native(vm, "str", osfl_str);
        vm_register_native(vm, "bool", osfl_bool);
        vm_register_native(vm, "open", osfl_open);
        vm_register_native(vm, "read", osfl_read);
        vm_register_native(vm, "write", osfl_write);
        vm_register_native(vm, "close", osfl_close);
        vm_register_native(vm, "exit", osfl_exit);
        vm_register_native(vm, "time", osfl_time);
        vm_register_native(vm, "type", osfl_type);
        vm_register_native(vm, "range", osfl_range);
        vm_register_native(vm, "enumerate", osfl_enumerate);

        vm_run(vm);

    cleanup:
        if (vm) vm_destroy(vm);
        if (bc) bytecode_destroy(bc);
        if (root) ast_destroy(root);
        if (parser) parser_destroy(parser);
        if (lexer) lexer_destroy(lexer);
        free(tokens);
        free(source);
        return status;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        if (vm) vm_destroy(vm);
        if (bc) bytecode_destroy(bc);
        if (root) ast_destroy(root);
        if (parser) parser_destroy(parser);
        if (lexer) lexer_destroy(lexer);
        free(tokens);
        free(source);
        return OSFL_ERROR_RUNTIME;
    }
}

/**
 * Run OSFL on a string of source code directly
 */
OSFLStatus osfl_run_string(const char* source, size_t length) {
    osfl_clear_error();
    if (!source) {
        set_osfl_error(OSFL_ERROR_INVALID_INPUT, "Null source in osfl_run_string", __FILE__, __LINE__, 0);
        return OSFL_ERROR_INVALID_INPUT;
    }

    /* We do the same pipeline as osfl_run_file, but skip file IO. */
    LexerConfig lex_cfg = lexer_default_config();
    /* use optional config from g_osfl_current_config if you want */
    Lexer* lexer = lexer_create(source, length, lex_cfg);
    if (!lexer) {
        set_osfl_error(OSFL_ERROR_LEXER, "Failed to create lexer", __FILE__, __LINE__, 0);
        return OSFL_ERROR_LEXER;
    }

    Token tokens[20000];
    size_t token_count = 0;
    while (true) {
        if (token_count >= 20000) {
            set_osfl_error(OSFL_ERROR_LEXER, "Too many tokens in osfl_run_string", __FILE__, __LINE__, 0);
            lexer_destroy(lexer);
            return OSFL_ERROR_LEXER;
        }
        Token t = lexer_next_token(lexer);
        tokens[token_count++] = t;
        if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR) {
            break;
        }
    }
    LexerError lxErr = lexer_get_error(lexer);
    if (lxErr.type != LEXER_ERROR_NONE) {
        set_osfl_error(OSFL_ERROR_LEXER, lxErr.message, lxErr.location.file, lxErr.location.line, lxErr.location.column);
        lexer_destroy(lexer);
        return OSFL_ERROR_LEXER;
    }

    Parser* parser = parser_create(tokens, token_count);
    AstNode* root = parser_parse(parser);
    parser_destroy(parser);

    /* semantic check */
    SemanticContext sem_ctx;
    semantic_init(&sem_ctx);
    semantic_analyze(root, &sem_ctx);
    if (sem_ctx.error_count > 0) {
        set_osfl_error(OSFL_ERROR_SYNTAX, "Semantic errors in run_string", __FILE__, __LINE__, 0);
        semantic_cleanup(&sem_ctx);
        ast_destroy(root);
        lexer_destroy(lexer);
        return OSFL_ERROR_SYNTAX;
    }
    semantic_cleanup(&sem_ctx);

    /* compile => bytecode => vm_run */
    Bytecode* bc = compiler_compile_ast(root);
    if (!bc) {
        set_osfl_error(OSFL_ERROR_COMPILER, "Failed to compile AST in run_string", __FILE__, __LINE__, 0);
        ast_destroy(root);
        lexer_destroy(lexer);
        return OSFL_ERROR_COMPILER;
    }
    VM* vm = vm_create(bc);
    if (!vm) {
        set_osfl_error(OSFL_ERROR_VM, "Failed to create VM in run_string", __FILE__, __LINE__, 0);
        bytecode_destroy(bc);
        ast_destroy(root);
        lexer_destroy(lexer);
        return OSFL_ERROR_VM;
    }
    vm_run(vm);

    /* cleanup */
    vm_destroy(vm);
    bytecode_destroy(bc);
    ast_destroy(root);
    lexer_destroy(lexer);

    return OSFL_SUCCESS;
}

/**
 * Store config if needed
 */
OSFLStatus osfl_configure(const OSFLConfig* cfg) {
    if (!cfg) {
        set_osfl_error(OSFL_ERROR_INVALID_INPUT, "Null config in osfl_configure", __FILE__, __LINE__, 0);
        return OSFL_ERROR_INVALID_INPUT;
    }
    g_osfl_current_config = *cfg;
    return OSFL_SUCCESS;
}

/**
 * Return the current config
 */
OSFLConfig osfl_get_config(void) {
    return g_osfl_current_config;
}

/**
 * Return a default config
 */
OSFLConfig osfl_default_config(void) {
    OSFLConfig c;
    c.tab_width = 4;
    c.include_comments = false;
    c.input_file = NULL;
    c.output_file = NULL;
    c.debug_mode = false;
    c.optimize = true;
    return c;
}
