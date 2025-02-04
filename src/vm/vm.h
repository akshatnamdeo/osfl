// src/vm/vm.h
#ifndef VM_H
#define VM_H

#include <stdbool.h>
#include "../../include/vm_common.h"
#include "../../include/value.h"
#include "../compiler/bytecode.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct VM VM;
typedef struct Frame Frame;

/**
    A minimal "object" structure for the new object model.
*/
typedef struct VMObject {
    int refcount;
    struct {
        char** keys;
        Value* values;  // Using Value instead of VMValue
        size_t count;
        size_t capacity;
    } fields;
} VMObject;

/**
    A coroutine structure.
*/
typedef struct Coro {
    bool active;
    size_t pc;
    Frame* frame;
    Value registers[16];  // Using Value instead of VMValue
} Coro;

#define MAX_COROUTINES 64

/**
    The main VM structure.
*/
typedef struct VM {
    Bytecode* bytecode;
    size_t pc;
    Value registers[16];  // Using Value instead of VMValue
    int running;
    Frame* call_stack[1024];
    size_t call_stack_top;
    size_t return_addresses[1024];
    VMObject** objects;
    size_t object_count;
    size_t object_capacity;
    Coro coroutines[MAX_COROUTINES];
    size_t current_coro;
    struct {
        const char* name;
        Value (*func)(int arg_count, Value* args);  // Using Value instead of VMValue
    } native_registry[64];
    size_t native_count;
    void* jit_context;
} VM;

/* PUBLIC FUNCTIONS */
VM* vm_create(Bytecode* bytecode);
void vm_destroy(VM* vm);
void vm_run(VM* vm);
void vm_dump_registers(const VM* vm);
Value vm_get_register_value(const VM* vm, int reg_index);  // Using Value instead of VMValue
void vm_retain_object(VM* vm, VMObject* obj);
void vm_release_object(VM* vm, VMObject* obj);
void vm_gc_collect(VM* vm);
VMObject* vm_create_object(VM* vm);
bool vm_set_property(VM* vm, VMObject* obj, const char* key, Value val);  // Using Value instead of VMValue
Value vm_get_property(VM* vm, VMObject* obj, const char* key);  // Using Value instead of VMValue
size_t vm_create_coroutine(VM* vm);
void vm_coroutine_yield(VM* vm);
void vm_coroutine_resume(VM* vm, size_t coro_index);
bool vm_register_native(VM* vm, const char* name, Value(*func)(int, Value*));  // Using Value instead of VMValue
Value vm_call_native(VM* vm, const char* name, int arg_count, Value* args);  // Using Value instead of VMValue

#ifdef ENABLE_JIT
void vm_jit_compile(VM* vm);
#endif

#ifdef __cplusplus
}
#endif

#endif // VM_H