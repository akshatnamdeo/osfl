#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdbool.h>
#include "frame.h"
#include "memory.h"
#include "../../include/value.h"

/**
    VMOpcode: all operation codes recognized by the VM.
*/
typedef enum {
    OP_NOP,
    OP_LOAD_CONST,          // load integer constant
    OP_LOAD_CONST_FLOAT,    // load float constant
    OP_LOAD_CONST_STR,      // load string constant
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_JUMP,
    OP_JUMP_IF_ZERO,
    OP_CALL,                // regular (bytecode) function call
    OP_CALL_NATIVE,         // native function call (extended instruction)
    OP_RET,
    OP_HALT,
    OP_NEWOBJ,
    OP_SETPROP,
    OP_GETPROP,
    OP_CORO_INIT,
    OP_CORO_YIELD,
    OP_CORO_RESUME
} VMOpcode;

/* An instruction for the VM with 4 operands */
typedef struct {
    VMOpcode opcode;
    int operand1;
    int operand2;
    int operand3;
    int operand4;  // extra operand for extended instructions (e.g. native calls)
} Instruction;

/* Bytecode: an array of instructions plus a count. */
typedef struct {
    Instruction* instructions;
    size_t instruction_count;
} Bytecode;

/**
    Use the unified Value type as our VMValue.
*/
typedef Value VMValue;

/**
    A minimal “object” structure for the new object model.
*/
typedef struct VMObject {
    int refcount;
    struct {
        char** keys;
        VMValue* values;
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
    VMValue registers[16];
} Coro;

#define MAX_COROUTINES 64

/**
    The main VM structure.
*/
typedef struct VM {
    Bytecode* bytecode;
    size_t pc;
    VMValue registers[16];
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
        VMValue (*func)(int arg_count, VMValue* args);
    } native_registry[64];
    size_t native_count;
    void* jit_context;
} VM;

/* PUBLIC FUNCTIONS */
VM* vm_create(Bytecode* bytecode);
void vm_destroy(VM* vm);
void vm_run(VM* vm);
void vm_dump_registers(const VM* vm);
VMValue vm_get_register_value(const VM* vm, int reg_index);
void vm_retain_object(VM* vm, VMObject* obj);
void vm_release_object(VM* vm, VMObject* obj);
void vm_gc_collect(VM* vm);
VMObject* vm_create_object(VM* vm);
bool vm_set_property(VM* vm, VMObject* obj, const char* key, VMValue val);
VMValue vm_get_property(VM* vm, VMObject* obj, const char* key);
size_t vm_create_coroutine(VM* vm);
void vm_coroutine_yield(VM* vm);
void vm_coroutine_resume(VM* vm, size_t coro_index);
bool vm_register_native(VM* vm, const char* name, VMValue(*func)(int, VMValue*));
VMValue vm_call_native(VM* vm, const char* name, int arg_count, VMValue* args);

#ifdef ENABLE_JIT
void vm_jit_compile(VM* vm);
#endif

#endif // VM_H
