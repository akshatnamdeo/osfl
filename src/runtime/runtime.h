// src/runtime/runtime.h
#ifndef RUNTIME_H
#define RUNTIME_H

#include "../../include/value.h"

Value osfl_print(int arg_count, Value* args);
Value osfl_split(int arg_count, Value* args);
Value osfl_join(int arg_count, Value* args);
Value osfl_substring(int arg_count, Value* args);
Value osfl_replace(int arg_count, Value* args);
Value osfl_to_upper(int arg_count, Value* args);
Value osfl_to_lower(int arg_count, Value* args);
Value osfl_len(int arg_count, Value* args);
Value osfl_append(int arg_count, Value* args);
Value osfl_pop(int arg_count, Value* args);
Value osfl_insert(int arg_count, Value* args);
Value osfl_remove(int arg_count, Value* args);
Value osfl_sqrt(int arg_count, Value* args);
Value osfl_pow(int arg_count, Value* args);
Value osfl_sin(int arg_count, Value* args);
Value osfl_cos(int arg_count, Value* args);
Value osfl_tan(int arg_count, Value* args);
Value osfl_log(int arg_count, Value* args);
Value osfl_abs(int arg_count, Value* args);
Value osfl_int(int arg_count, Value* args);
Value osfl_float(int arg_count, Value* args);
Value osfl_str(int arg_count, Value* args);
Value osfl_bool(int arg_count, Value* args);
Value osfl_open(int arg_count, Value* args);
Value osfl_read(int arg_count, Value* args);
Value osfl_write(int arg_count, Value* args);
Value osfl_close(int arg_count, Value* args);
Value osfl_exit(int arg_count, Value* args);
Value osfl_time(int arg_count, Value* args);
Value osfl_type(int arg_count, Value* args);
Value osfl_range(int arg_count, Value* args);
Value osfl_enumerate(int arg_count, Value* args);

#endif /* RUNTIME_H */