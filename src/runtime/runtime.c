#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* -----------------------------
 * Internal Helper: create a VAL_STRING from a C-string
 * ----------------------------- */
static OSFL_Value make_string(const char* s) {
    OSFL_Value v;
    v.type = VAL_STRING;
    /* In a production system, handle memory carefully or use a GC. */
    v.as.str_val = strdup(s);
    return v;
}

/* -----------------------------
 * Internal Helper: create an empty list
 * ----------------------------- */
static OSFL_Value make_list(void) {
    OSFL_Value v;
    v.type = VAL_LIST;
    v.as.list_val.data = NULL;
    v.as.list_val.length = 0;
    v.as.list_val.capacity = 0;
    return v;
}

/* -----------------------------
 * Internal Helper: push a OSFL_Value onto a dynamic list
 * ----------------------------- */
static void list_push(OSFL_Value* list_value, OSFL_Value item) {
    if (list_value->type != VAL_LIST) return; /* or assert */
    size_t len = list_value->as.list_val.length;
    size_t cap = list_value->as.list_val.capacity;
    if (len >= cap) {
        size_t new_cap = (cap == 0) ? 8 : cap * 2;
        list_value->as.list_val.data = realloc(list_value->as.list_val.data, new_cap * sizeof(OSFL_Value));
        list_value->as.list_val.capacity = new_cap;
    }
    list_value->as.list_val.data[list_value->as.list_val.length++] = item;
}

/* -----------------------------
 * Internal Helper: convert a OSFL_Value to string (rudimentary)
 *   - not safe for large data, returns a static buffer
 * ----------------------------- */
static char* value_to_string(const OSFL_Value* v) {
    static char buffer[128]; /* expanded a bit for safety */
    switch (v->type) {
        case VAL_INT:
            snprintf(buffer, sizeof(buffer), "%lld", (long long)v->as.int_val);
            return buffer;
        case VAL_FLOAT:
            snprintf(buffer, sizeof(buffer), "%f", v->as.float_val);
            return buffer;
        case VAL_BOOL:
            return (v->as.bool_val) ? "true" : "false";
        case VAL_STRING:
            return v->as.str_val; /* caution: returns the pointer directly */
        case VAL_LIST:
            return "[list]";
        case VAL_FILE:
            return "[file]";
        case VAL_NULL:
            return "null";
        default:
            return "[unknown]";
    }
}

/* -----------------------------
 * NEW: Print Function Implementation
 * -----------------------------
 * This function prints each argument (converted to a string) separated by a space,
 * then prints a newline. It returns VALUE_NULL.
 */
OSFL_Value osfl_print(int arg_count, OSFL_Value* args) {
    printf("[DEBUG] osfl_print called with %d arguments\n", arg_count);
    for (int i = 0; i < arg_count; i++) {
        char* s = value_to_string(&args[i]);
        printf("%s", s);
        if (i < arg_count - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return VALUE_NULL;
}

/* -----------------------------
 * STRING FUNCTIONS
 * ----------------------------- */
OSFL_Value osfl_split(int arg_count, OSFL_Value* args) {
    if (arg_count < 2) {
        return VALUE_NULL;
    }
    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return VALUE_NULL;
    }
    const char* str   = args[0].as.str_val;
    const char* delim = args[1].as.str_val;

    OSFL_Value result = make_list();

    /* Copy str for strtok usage */
    char* temp = strdup(str);
    char* token = strtok(temp, delim);
    while (token != NULL) {
        OSFL_Value piece = make_string(token);
        list_push(&result, piece);
        token = strtok(NULL, delim);
    }
    free(temp);
    return result;
}

OSFL_Value osfl_join(int arg_count, OSFL_Value* args) {
    if (arg_count < 2) {
        return VALUE_NULL;
    }
    if (args[0].type != VAL_LIST || args[1].type != VAL_STRING) {
        return VALUE_NULL;
    }
    OSFL_Value listval = args[0];
    const char* delim = args[1].as.str_val;

    /* We'll build up in a dynamic buffer. */
    size_t buf_size = 64;
    char* buffer = malloc(buf_size);
    buffer[0] = '\0';

    for (size_t i = 0; i < listval.as.list_val.length; i++) {
        char* piece = value_to_string(&listval.as.list_val.data[i]);
        size_t need = strlen(buffer)
                    + strlen(piece)
                    + (i > 0 ? strlen(delim) : 0)
                    + 1;
        /* expand if needed */
        while (need > buf_size) {
            buf_size *= 2;
            buffer = realloc(buffer, buf_size);
        }
        if (i > 0) strcat(buffer, delim);
        strcat(buffer, piece);
    }

    OSFL_Value result = make_string(buffer);
    free(buffer);
    return result;
}

OSFL_Value osfl_substring(int arg_count, OSFL_Value* args) {
    if (arg_count < 3) return VALUE_NULL;
    if (args[0].type != VAL_STRING ||
        args[1].type != VAL_INT ||
        args[2].type != VAL_INT) {
        return VALUE_NULL;
    }
    const char* str = args[0].as.str_val;
    long long start  = args[1].as.int_val;
    long long length = args[2].as.int_val;

    size_t str_len = strlen(str);
    if (start < 0) start = 0;
    if (start + length > (long long)str_len) {
        length = (long long)str_len - start;
    }
    if (length < 0) length = 0;

    char* buffer = malloc(length + 1);
    memcpy(buffer, str + start, length);
    buffer[length] = '\0';

    OSFL_Value result = make_string(buffer);
    free(buffer);
    return result;
}

OSFL_Value osfl_replace(int arg_count, OSFL_Value* args) {
    if (arg_count < 3) return VALUE_NULL;
    if (args[0].type != VAL_STRING ||
        args[1].type != VAL_STRING ||
        args[2].type != VAL_STRING) {
        return VALUE_NULL;
    }
    const char* original = args[0].as.str_val;
    const char* target   = args[1].as.str_val;
    const char* repl     = args[2].as.str_val;

    size_t orig_len = strlen(original);
    size_t tgt_len  = strlen(target);
    size_t repl_len = strlen(repl);

    /* We'll build result in a dynamic buffer. */
    size_t res_cap = orig_len * 2 + 1;
    char* result_buffer = malloc(res_cap);
    result_buffer[0] = '\0';

    const char* scan = original;
    while (*scan) {
        if (strncmp(scan, target, tgt_len) == 0) {
            /* Append repl */
            size_t need = strlen(result_buffer) + repl_len + 1;
            while (need > res_cap) {
                res_cap *= 2;
                result_buffer = realloc(result_buffer, res_cap);
            }
            strcat(result_buffer, repl);
            scan += tgt_len;
        } else {
            /* copy one char */
            size_t old_len = strlen(result_buffer);
            if (old_len + 2 > res_cap) {
                while (old_len + 2 > res_cap) {
                    res_cap *= 2;
                }
                result_buffer = realloc(result_buffer, res_cap);
            }
            result_buffer[old_len] = *scan;
            result_buffer[old_len + 1] = '\0';
            scan++;
        }
    }

    OSFL_Value result = make_string(result_buffer);
    free(result_buffer);
    return result;
}

OSFL_Value osfl_to_upper(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return VALUE_NULL;
    }
    const char* s = args[0].as.str_val;
    char* buffer = strdup(s);
    for (char* p = buffer; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = (char)(*p - 32);
        }
    }
    OSFL_Value result = make_string(buffer);
    free(buffer);
    return result;
}

OSFL_Value osfl_to_lower(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return VALUE_NULL;
    }
    const char* s = args[0].as.str_val;
    char* buffer = strdup(s);
    for (char* p = buffer; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = (char)(*p + 32);
        }
    }
    OSFL_Value result = make_string(buffer);
    free(buffer);
    return result;
}

/* -----------------------------
 * LIST/ARRAY FUNCTIONS
 * ----------------------------- */
OSFL_Value osfl_len(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    OSFL_Value result;
    result.type = VAL_INT;

    switch (args[0].type) {
        case VAL_STRING:
            result.as.int_val = (long long)strlen(args[0].as.str_val);
            break;
        case VAL_LIST:
            result.as.int_val = (long long)args[0].as.list_val.length;
            break;
        default:
            result.as.int_val = 0;
            break;
    }
    return result;
}

OSFL_Value osfl_append(int arg_count, OSFL_Value* args) {
    if (arg_count < 2 || args[0].type != VAL_LIST) {
        return VALUE_NULL;
    }
    list_push(&args[0], args[1]);
    return args[0];
}

OSFL_Value osfl_pop(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_LIST) {
        return VALUE_NULL;
    }
    size_t len = args[0].as.list_val.length;
    if (len == 0) {
        return VALUE_NULL;
    }
    OSFL_Value item = args[0].as.list_val.data[len - 1];
    args[0].as.list_val.length--;
    return item;
}

OSFL_Value osfl_insert(int arg_count, OSFL_Value* args) {
    /*
     * insert(list, index, value)
     */
    if (arg_count < 3 ||
        args[0].type != VAL_LIST ||
        args[1].type != VAL_INT) {
        return VALUE_NULL;
    }
    OSFL_Value* listv = &args[0];
    long long index = args[1].as.int_val;
    OSFL_Value val = args[2];

    if (index < 0) index = 0;
    if (index > (long long)listv->as.list_val.length) {
        index = listv->as.list_val.length;
    }
    /* Expand by pushing a dummy. */
    list_push(listv, VALUE_NULL);

    /* shift elements right from the end to index */
    for (long long i = (long long)listv->as.list_val.length - 1; i > index; i--) {
        listv->as.list_val.data[i] = listv->as.list_val.data[i - 1];
    }
    listv->as.list_val.data[index] = val;
    return *listv;
}

OSFL_Value osfl_remove(int arg_count, OSFL_Value* args) {
    /*
     * remove(list, value) => removes first occurrence
     */
    if (arg_count < 2 || args[0].type != VAL_LIST) {
        return VALUE_NULL;
    }
    OSFL_Value* listv = &args[0];
    OSFL_Value val = args[1];
    for (size_t i = 0; i < listv->as.list_val.length; i++) {
        bool match = false;
        if (val.type == VAL_INT && listv->as.list_val.data[i].type == VAL_INT) {
            match = (val.as.int_val == listv->as.list_val.data[i].as.int_val);
        } else if (val.type == VAL_STRING && listv->as.list_val.data[i].type == VAL_STRING) {
            if (strcmp(val.as.str_val, listv->as.list_val.data[i].as.str_val) == 0) {
                match = true;
            }
        }
        /* Add more type comparisons if needed. */
        if (match) {
            /* shift everything left by 1 */
            for (size_t j = i; j < listv->as.list_val.length - 1; j++) {
                listv->as.list_val.data[j] = listv->as.list_val.data[j + 1];
            }
            listv->as.list_val.length--;
            break;
        }
    }
    return *listv;
}

/* -----------------------------
 * MATH FUNCTIONS
 * ----------------------------- */

OSFL_Value osfl_sqrt(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    double val = 0;
    if (args[0].type == VAL_INT)   val = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) val = args[0].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = sqrt(val);
    return r;
}

OSFL_Value osfl_pow(int arg_count, OSFL_Value* args) {
    if (arg_count < 2) return VALUE_NULL;
    double base = 0;
    double exp  = 0;
    if (args[0].type == VAL_INT)   base = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) base = args[0].as.float_val;

    if (args[1].type == VAL_INT)   exp = (double)args[1].as.int_val;
    if (args[1].type == VAL_FLOAT) exp = args[1].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = pow(base, exp);
    return r;
}

OSFL_Value osfl_sin(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    double d = 0;
    if (args[0].type == VAL_INT)   d = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) d = args[0].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = sin(d);
    return r;
}

OSFL_Value osfl_cos(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    double d = 0;
    if (args[0].type == VAL_INT)   d = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) d = args[0].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = cos(d);
    return r;
}

OSFL_Value osfl_tan(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    double d = 0;
    if (args[0].type == VAL_INT)   d = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) d = args[0].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = tan(d);
    return r;
}

OSFL_Value osfl_log(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    double d = 0;
    if (args[0].type == VAL_INT)   d = (double)args[0].as.int_val;
    if (args[0].type == VAL_FLOAT) d = args[0].as.float_val;

    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = log(d);
    return r;
}

OSFL_Value osfl_abs(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    if (args[0].type == VAL_INT) {
        long long x = args[0].as.int_val;
        OSFL_Value r;
        r.type = VAL_INT;
        r.as.int_val = (x < 0) ? -x : x;
        return r;
    } else if (args[0].type == VAL_FLOAT) {
        double d = args[0].as.float_val;
        OSFL_Value r;
        r.type = VAL_FLOAT;
        r.as.float_val = fabs(d);
        return r;
    }
    return VALUE_NULL;
}

/* -----------------------------
 * CONVERSION FUNCTIONS
 * ----------------------------- */
OSFL_Value osfl_int(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    OSFL_Value r;
    r.type = VAL_INT;
    switch (args[0].type) {
        case VAL_INT:
            r.as.int_val = args[0].as.int_val;
            break;
        case VAL_FLOAT:
            r.as.int_val = (long long)args[0].as.float_val;
            break;
        case VAL_BOOL:
            r.as.int_val = args[0].as.bool_val ? 1 : 0;
            break;
        case VAL_STRING:
            r.as.int_val = atoll(args[0].as.str_val);
            break;
        default:
            r.as.int_val = 0;
            break;
    }
    return r;
}

OSFL_Value osfl_float(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    OSFL_Value r;
    r.type = VAL_FLOAT;
    switch (args[0].type) {
        case VAL_INT:
            r.as.float_val = (double)args[0].as.int_val;
            break;
        case VAL_FLOAT:
            r.as.float_val = args[0].as.float_val;
            break;
        case VAL_BOOL:
            r.as.float_val = args[0].as.bool_val ? 1.0 : 0.0;
            break;
        case VAL_STRING:
            r.as.float_val = atof(args[0].as.str_val);
            break;
        default:
            r.as.float_val = 0.0;
            break;
    }
    return r;
}

OSFL_Value osfl_str(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    /* Convert the first arg to string using value_to_string. */
    char* tmp = strdup(value_to_string(&args[0]));
    OSFL_Value r = make_string(tmp);
    free(tmp);
    return r;
}

OSFL_Value osfl_bool(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) return VALUE_NULL;
    OSFL_Value r;
    r.type = VAL_BOOL;
    switch (args[0].type) {
        case VAL_INT:
            r.as.bool_val = (args[0].as.int_val != 0);
            break;
        case VAL_FLOAT:
            r.as.bool_val = (args[0].as.float_val != 0.0);
            break;
        case VAL_BOOL:
            r.as.bool_val = args[0].as.bool_val;
            break;
        case VAL_STRING:
            r.as.bool_val = (strlen(args[0].as.str_val) > 0);
            break;
        case VAL_NULL:
            r.as.bool_val = false;
            break;
        default:
            r.as.bool_val = true;
            break;
    }
    return r;
}

/* -----------------------------
 * FILE I/O FUNCTIONS
 * ----------------------------- */

OSFL_Value osfl_open(int arg_count, OSFL_Value* args) {
    if (arg_count < 2) return VALUE_NULL;
    if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return VALUE_NULL;
    }
    const char* filename = args[0].as.str_val;
    const char* mode     = args[1].as.str_val;

    FILE* fp = fopen(filename, mode);
    if (!fp) {
        return VALUE_NULL;
    }
    OSFL_Value v;
    v.type = VAL_FILE;
    v.as.file_val.native_file = fp;
    return v;
}

OSFL_Value osfl_read(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_FILE) {
        return VALUE_NULL;
    }
    FILE* fp = (FILE*)args[0].as.file_val.native_file;
    if (!fp) return VALUE_NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    size_t readn = fread(buffer, 1, size, fp);
    buffer[readn] = '\0';

    OSFL_Value v = make_string(buffer);
    free(buffer);
    return v;
}

OSFL_Value osfl_write(int arg_count, OSFL_Value* args) {
    if (arg_count < 2 || args[0].type != VAL_FILE || args[1].type != VAL_STRING) {
        return VALUE_NULL;
    }
    FILE* fp = (FILE*)args[0].as.file_val.native_file;
    if (!fp) return VALUE_NULL;

    fputs(args[1].as.str_val, fp);
    /* Return number of chars written? */
    OSFL_Value r;
    r.type = VAL_INT;
    r.as.int_val = (long long)strlen(args[1].as.str_val);
    return r;
}

OSFL_Value osfl_close(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_FILE) {
        return VALUE_NULL;
    }
    FILE* fp = (FILE*)args[0].as.file_val.native_file;
    if (fp) {
        fclose(fp);
        args[0].as.file_val.native_file = NULL;
    }
    return VALUE_NULL;
}

/* -----------------------------
 * SYSTEM/OS FUNCTIONS
 * ----------------------------- */
OSFL_Value osfl_exit(int arg_count, OSFL_Value* args) {
    int code = 0;
    if (arg_count >= 1 && args[0].type == VAL_INT) {
        code = (int)args[0].as.int_val;
    }
    exit(code);
    return VALUE_NULL; /* unreachable */
}

OSFL_Value osfl_time(int arg_count, OSFL_Value* args) {
    (void)arg_count; (void)args;
    OSFL_Value r;
    r.type = VAL_FLOAT;
    r.as.float_val = (double)time(NULL);
    return r;
}

/* -----------------------------
 * MISCELLANEOUS
 * ----------------------------- */
OSFL_Value osfl_type(int arg_count, OSFL_Value* args) {
    if (arg_count < 1) {
        return make_string("null");
    }
    switch (args[0].type) {
        case VAL_INT:    return make_string("int");
        case VAL_FLOAT:  return make_string("float");
        case VAL_BOOL:   return make_string("bool");
        case VAL_STRING: return make_string("string");
        case VAL_LIST:   return make_string("list");
        case VAL_FILE:   return make_string("file");
        case VAL_NULL:   return make_string("null");
        default:         return make_string("unknown");
    }
}

/**
 * range(start, end, step): Return a list of numbers for iteration,
 * similar to Python's range. step can be optional. 
 */
OSFL_Value osfl_range(int arg_count, OSFL_Value* args) {
    long long start = 0, end = 0, step = 1;
    if (arg_count >= 1 && args[0].type == VAL_INT) {
        start = args[0].as.int_val;
    }
    if (arg_count >= 2 && args[1].type == VAL_INT) {
        end = args[1].as.int_val;
    }
    if (arg_count >= 3 && args[2].type == VAL_INT) {
        step = args[2].as.int_val;
        if (step == 0) step = 1;
    }
    OSFL_Value result = make_list();
    for (long long i = start; (step > 0) ? (i < end) : (i > end); i += step) {
        OSFL_Value num;
        num.type = VAL_INT;
        num.as.int_val = i;
        list_push(&result, num);
    }
    return result;
}

/**
 * enumerate(list): Return a list of (index, item) pairs
 *  => [ [0, item0], [1, item1], ... ]
 */
OSFL_Value osfl_enumerate(int arg_count, OSFL_Value* args) {
    if (arg_count < 1 || args[0].type != VAL_LIST) {
        return VALUE_NULL;
    }
    OSFL_Value input_list = args[0];
    OSFL_Value result = make_list();
    for (size_t i = 0; i < input_list.as.list_val.length; i++) {
        OSFL_Value pair = make_list();
        /* index */
        OSFL_Value idx;
        idx.type = VAL_INT;
        idx.as.int_val = (long long)i;
        list_push(&pair, idx);
        /* item */
        list_push(&pair, input_list.as.list_val.data[i]);
        /* push pair into result list */
        list_push(&result, pair);
    }
    return result;
}