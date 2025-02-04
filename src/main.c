// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include "../include/osfl.h"

static void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s [options] <input_file>\n\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help           Display this help message\n");
    fprintf(stderr, "  -v, --version        Display version information\n");
    fprintf(stderr, "  -o <file>           Specify output file\n");
    fprintf(stderr, "  -d, --debug         Enable debug output\n");
    fprintf(stderr, "  --no-optimize       Disable optimizations\n");
}

static OSFLStatus parse_args(int argc, char* argv[], OSFLConfig* config) {
    if (argc < 2) {
        print_usage(argv[0]);
        return OSFL_ERROR_INVALID_INPUT;
    }

    /* Start with default configuration */
    *config = osfl_default_config();
    config->debug_mode = true;  // Always enable debug mode for now

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            fprintf(stderr, "OSFL Version %s\n", osfl_version());
            exit(0);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config->output_file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            config->debug_mode = true;
        } else if (strcmp(argv[i], "--no-optimize") == 0) {
            config->optimize = false;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return OSFL_ERROR_INVALID_INPUT;
        } else {
            if (config->input_file) {
                fprintf(stderr, "Multiple input files not supported\n");
                return OSFL_ERROR_INVALID_INPUT;
            }
            config->input_file = argv[i];
        }
    }

    if (!config->input_file) {
        fprintf(stderr, "No input file specified\n");
        return OSFL_ERROR_INVALID_INPUT;
    }

    return OSFL_SUCCESS;
}

static void handle_error(void) {
    const OSFLError* error = osfl_get_last_error();
    if (error) {
        if (error->file) {
            fprintf(stderr, "Error in %s at line %zu, column %zu:\n",
                    error->file, error->line, error->column);
        }
        fprintf(stderr, "Error: %s\n", error->message);
    }
}

LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo) {
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    fprintf(stderr, "Unhandled exception. Code: 0x%lx\n", code);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char* argv[]) {
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    OSFLConfig config;
    OSFLStatus status;

    status = parse_args(argc, argv, &config);
    if (status != OSFL_SUCCESS) {
        return EXIT_FAILURE;
    }

    if (osfl_init(&config) != OSFL_SUCCESS) {
        handle_error();
        return EXIT_FAILURE;
    }

    status = osfl_run_file(config.input_file);
    if (status != OSFL_SUCCESS) {
        handle_error();
        osfl_cleanup();
        return EXIT_FAILURE;
    }

    osfl_cleanup();
    return EXIT_SUCCESS;
}
