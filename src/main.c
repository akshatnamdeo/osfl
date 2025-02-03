#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/osfl.h"

/* Display usage information */
static void print_usage(const char* program_name) {
    printf("Usage: %s [options] <input_file>\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help           Display this help message\n");
    printf("  -v, --version        Display version information\n");
    printf("  -o <file>           Specify output file\n");
    printf("  -d, --debug         Enable debug output\n");
    printf("  --no-optimize       Disable optimizations\n");
    printf("\nFor more information, visit: [project-url]\n");
}

/* Parse command line arguments and configure OSFL */
static OSFLStatus parse_args(int argc, char* argv[], OSFLConfig* config) {
    if (argc < 2) {
        print_usage(argv[0]);
        return OSFL_ERROR_INVALID_INPUT;
    }

    /* Start with default configuration */
    *config = osfl_default_config();

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("OSFL Version %s\n", osfl_version());
            exit(0);
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config->output_file = argv[++i];
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            config->debug_mode = true;
        }
        else if (strcmp(argv[i], "--no-optimize") == 0) {
            config->optimize = false;
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return OSFL_ERROR_INVALID_INPUT;
        }
        else {
            /* Assume it's the input file */
            if (config->input_file) {
                fprintf(stderr, "Multiple input files not supported\n");
                return OSFL_ERROR_INVALID_INPUT;
            }
            config->input_file = argv[i];
        }
    }

    /* Verify we have an input file */
    if (!config->input_file) {
        fprintf(stderr, "No input file specified\n");
        return OSFL_ERROR_INVALID_INPUT;
    }

    return OSFL_SUCCESS;
}

/* Handle errors and display appropriate messages */
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

int main(int argc, char* argv[]) {
    OSFLConfig config;
    OSFLStatus status;

    /* Parse command line arguments */
    status = parse_args(argc, argv, &config);
    if (status != OSFL_SUCCESS) {
        return EXIT_FAILURE;
    }

    /* Initialize OSFL */
    status = osfl_init(&config);
    if (status != OSFL_SUCCESS) {
        handle_error();
        return EXIT_FAILURE;
    }

    /* Run the compiler on the input file */
    status = osfl_run_file(config.input_file);
    if (status != OSFL_SUCCESS) {
        handle_error();
        osfl_cleanup();
        return EXIT_FAILURE;
    }

    /* Clean up and exit */
    osfl_cleanup();
    return EXIT_SUCCESS;
}