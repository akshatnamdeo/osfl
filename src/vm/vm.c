#include "vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "frame.h"
#include "../include/vm_common.h"
#include "../compiler/bytecode.h"

/* forward declarations */
static void vm_init_registers(VM* vm);
static void vm_execute_instruction(VM* vm, Instruction inst);
static void vm_push_frame(VM* vm, Frame* frame, size_t return_address);
static void vm_pop_frame(VM* vm);
static void vm_grow_object_array(VM* vm);
static VMValue vmvalue_from_int(int64_t n);
static void destroy_object(VMObject* obj);

VM* vm_create(Bytecode* bytecode) {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (!vm) {
        fprintf(stderr, "Failed to allocate VM\n");
        exit(1);
    }

    vm->bytecode = bytecode;
    vm->pc = 0;
    vm->running = 1;
    vm->call_stack_top = 0;

    vm_init_registers(vm);

    for (size_t i = 0; i < 1024; i++) {
        vm->call_stack[i] = NULL;
        vm->return_addresses[i] = 0;
    }

    vm->objects = NULL;
    vm->object_count = 0;
    vm->object_capacity = 0;

    for (size_t i = 0; i < MAX_COROUTINES; i++) {
        vm->coroutines[i].active = false;
    }
    vm->current_coro = 0;

    vm->native_count = 0;
    for (size_t i = 0; i < 64; i++) {
        vm->native_registry[i].name = NULL;
        vm->native_registry[i].func = NULL;
    }

#ifdef ENABLE_JIT
    vm->jit_context = NULL;
#endif

    return vm;
}

void vm_destroy(VM* vm) {
    if (!vm) return;

    while (vm->call_stack_top > 0) {
        vm_pop_frame(vm);
    }

    for (size_t i = 0; i < vm->object_count; i++) {
        destroy_object(vm->objects[i]);
    }
    free(vm->objects);

#ifdef ENABLE_JIT
    if (vm->jit_context) {
        vm->jit_context = NULL;
    }
#endif

    free(vm);
}

static void vm_init_registers(VM* vm) {
    for (int i = 0; i < 16; i++) {
        vm->registers[i].type = VAL_NULL;
        vm->registers[i].refcount = 0;
        vm->registers[i].as.int_val = 0;
    }
}

void vm_run(VM* vm) {
#ifdef ENABLE_JIT
    vm_jit_compile(vm);
#endif

    while (vm->running && vm->pc < vm->bytecode->instruction_count) {
        Instruction inst = vm->bytecode->instructions[vm->pc];
        vm_execute_instruction(vm, inst);
    }
}

static void vm_execute_instruction(VM* vm, Instruction inst) {
    fprintf(stderr, "[DEBUG] PC: %zu, Opcode: %d\n", vm->pc, inst.opcode);
    switch (inst.opcode) {
        case OP_NOP:
            vm->pc++;
            break;
        case OP_LOAD_CONST: {
            int r = inst.operand1;
            int val = inst.operand2;
            if (r < 0 || r >= 16) {
                    fprintf(stderr, "Invalid register index %d\n", r);
                    vm->running = 0;
                    return;
            }
            vm->registers[r].type = VAL_INT;
            vm->registers[r].as.int_val = val;
            vm->registers[r].refcount = 0;
            vm->pc++;
        } break;
        case OP_LOAD_CONST_FLOAT: {
            int r = inst.operand1;
            if (r < 0 || r >= 16) {
                fprintf(stderr, "Invalid register index %d for float constant\n", r);
                vm->running = 0;
                return;
            }
            vm->registers[r].type = VAL_FLOAT;
            vm->registers[r].as.float_val = 0.0; // Placeholder for float constants
            vm->registers[r].refcount = 0;
            vm->pc++;
        } break;
        case OP_LOAD_CONST_STR: {
            int r = inst.operand1;
            if (r < 0 || r >= 16) {
                    fprintf(stderr, "Invalid register index %d for string constant\n", r);
                    vm->running = 0;
                    return;
            }
            vm->registers[r].type = VAL_STRING;
            // Here we assume the compiler loaded a raw pointer;
            // with our new constant pool, you might want to change this
            // so that OP_LOAD_CONST_STR gets its string from the constant pool.
            vm->registers[r].as.str_val = (char*)(intptr_t)inst.operand2;
            vm->registers[r].refcount = 0;
            vm->pc++;
        } break;
        case OP_ADD: {
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "OP_ADD invalid register index.\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[rd].type = VAL_INT;
                vm->registers[rd].as.int_val = vm->registers[rs1].as.int_val + vm->registers[rs2].as.int_val;
                vm->registers[rd].refcount = 0;
            } else {
                fprintf(stderr, "OP_ADD type mismatch.\n");
                vm->running = 0;
            }
            vm->pc++;
        } break;
        case OP_SUB:
        case OP_MUL:
        case OP_DIV: {
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "OP_%d invalid register index\n", inst.opcode);
                vm->running = 0;
                return;
            }
            if (vm->registers[rs1].type != VAL_INT || vm->registers[rs2].type != VAL_INT) {
                fprintf(stderr, "OP_%d type mismatch (must be int)\n", inst.opcode);
                vm->running = 0;
                return;
            }
            long long a = vm->registers[rs1].as.int_val;
            long long b = vm->registers[rs2].as.int_val;
            long long r;
            if (inst.opcode == OP_SUB) {
                r = a - b;
            } else if (inst.opcode == OP_MUL) {
                r = a * b;
            } else {
                if (b == 0) {
                    fprintf(stderr, "Division by zero!\n");
                    vm->running = 0;
                    return;
                }
                r = a / b;
            }
            vm->registers[rd].type = VAL_INT;
            vm->registers[rd].as.int_val = r;
            vm->registers[rd].refcount = 0;
            vm->pc++;
        } break;
        case OP_EQ: {
            int dest = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (dest < 0 || dest >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "OP_EQ invalid register index.\n");
                vm->running = 0;
                return;
            }
            // Compare integer values (for simplicity; you may extend this to floats etc.)
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[dest].type = VAL_INT;
                vm->registers[dest].as.int_val = (vm->registers[rs1].as.int_val == vm->registers[rs2].as.int_val) ? 1 : 0;
                vm->registers[dest].refcount = 0;
            } else {
                fprintf(stderr, "OP_EQ type mismatch: expected ints.\n");
                vm->running = 0;
                return;
            }
            vm->pc++;
        } break;
        case OP_NEQ: {
            int dest = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (dest < 0 || dest >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "OP_NEQ invalid register index.\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[dest].type = VAL_INT;
                vm->registers[dest].as.int_val = (vm->registers[rs1].as.int_val != vm->registers[rs2].as.int_val) ? 1 : 0;
                vm->registers[dest].refcount = 0;
            } else {
                fprintf(stderr, "OP_NEQ type mismatch: expected ints.\n");
                vm->running = 0;
                return;
            }
            vm->pc++;
        } break;
        case OP_MOVE: {
            int dest = inst.operand1;
            int src = inst.operand2;
            if (dest < 0 || dest >= 16 || src < 0 || src >= 16) {
                fprintf(stderr, "OP_MOVE: invalid register index (dest=%d, src=%d).\n", dest, src);
                vm->running = 0;
                return;
            }
            vm->registers[dest] = vm->registers[src];
            vm->pc++;
        } break;
        case OP_JUMP:
            vm->pc = (size_t)inst.operand1;
            break;
        case OP_JUMP_IF_ZERO: {
            int r = inst.operand2;
            if (r < 0 || r >= 16) {
                fprintf(stderr, "OP_JUMP_IF_ZERO invalid register index\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[r].type != VAL_INT) {
                fprintf(stderr, "OP_JUMP_IF_ZERO requires an int register\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[r].as.int_val == 0) {
                vm->pc = (size_t)inst.operand1;
            } else {
                vm->pc++;
            }
        } break;
        case OP_CALL: {
            size_t func_addr = (size_t)inst.operand1;
            if (func_addr >= vm->bytecode->instruction_count) {
                fprintf(stderr, "OP_CALL: function addr out of range %zu\n", func_addr);
                vm->running = 0;
                return;
            }
            Frame* f = frame_create(8, vm->call_stack_top > 0 ? vm->call_stack[vm->call_stack_top - 1] : NULL);
            vm_push_frame(vm, f, vm->pc + 1);
            vm->pc = func_addr;
        } break;
        case OP_CALL_NATIVE: {
            int dest = inst.operand1;
            int cp_index = inst.operand2;  // constant pool index
            fprintf(stderr, "[DEBUG] OP_CALL_NATIVE: About to look up constant pool index %d (pool count: %zu).\n",
                    cp_index, vm->bytecode->constant_pool.count);
            if (cp_index < 0 || cp_index >= (int)vm->bytecode->constant_pool.count) {
                fprintf(stderr, "OP_CALL_NATIVE: constant pool index %d out of range\n", cp_index);
                vm->running = 0;
                return;
            }
            const char* native_name = vm->bytecode->constant_pool.strings[cp_index];
            fprintf(stderr, "[DEBUG] OP_CALL_NATIVE: Retrieved native function name '%s' from constant pool index %d.\n",
                    native_name, cp_index);
            int arg_count = inst.operand3;
            int base_reg = inst.operand4;
            if (!native_name) {
                fprintf(stderr, "ERROR: NULL native function name\n");
                vm->running = 0;
                return;
            }
            VMValue* args = (VMValue*)malloc(arg_count * sizeof(VMValue));
            if (!args) {
                fprintf(stderr, "Failed to allocate memory for native call arguments.\n");
                vm->running = 0;
                return;
            }
            for (int i = 0; i < arg_count; i++) {
                if (base_reg + i >= 16) {
                    fprintf(stderr, "ERROR: Register index out of bounds in native call\n");
                    free(args);
                    vm->running = 0;
                    return;
                }
                args[i] = vm->registers[base_reg + i];
            }
            VMValue result = vm_call_native(vm, native_name, arg_count, args);
            free(args);
            if (dest >= 0 && dest < 16) {
                vm->registers[dest] = result;
            } else {
                fprintf(stderr, "ERROR: Invalid destination register in native call\n");
                vm->running = 0;
                return;
            }
            vm->pc++;
        } break;
        case OP_RET:
            if (vm->call_stack_top == 0) {
                // No caller frame exists (e.g. main returned).
                vm->running = 0;
            } else {
                vm_pop_frame(vm);
            }
            break;
        case OP_HALT:
            vm->running = 0;
            break;
        case OP_NEWOBJ: {
            int rd = inst.operand1;
            if (rd < 0 || rd >= 16) {
                fprintf(stderr, "OP_NEWOBJ invalid register index\n");
                vm->running = 0;
                return;
            }
            VMObject* obj = vm_create_object(vm);
            vm->registers[rd].type = VAL_OBJ;
            vm->registers[rd].as.obj_ref = obj;
            vm->registers[rd].refcount = 1;
            vm->pc++;
        } break;
        case OP_SETPROP: {
            int ro = inst.operand1;
            int rk = inst.operand2;
            int rv = inst.operand3;
            if (ro < 0 || ro >= 16 || rk < 0 || rk >= 16 || rv < 0 || rv >= 16) {
                fprintf(stderr, "OP_SETPROP invalid register index\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[ro].type != VAL_OBJ) {
                fprintf(stderr, "OP_SETPROP: not an object.\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rk].type != VAL_INT) {
                fprintf(stderr, "OP_SETPROP: key is not an int\n");
                vm->running = 0;
                return;
            }
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%" PRId64, vm->registers[rk].as.int_val);
            VMValue val_to_set = vm->registers[rv];
            vm_set_property(vm, (VMObject*)vm->registers[ro].as.obj_ref, buffer, val_to_set);
            vm->pc++;
        } break;
        case OP_GETPROP: {
            int rd = inst.operand1;
            int ro = inst.operand2;
            int rk = inst.operand3;
            if (rd < 0 || rd >= 16 || ro < 0 || ro >= 16 || rk < 0 || rk >= 16) {
                fprintf(stderr, "OP_GETPROP invalid register index\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[ro].type != VAL_OBJ) {
                fprintf(stderr, "OP_GETPROP: not an object.\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rk].type != VAL_INT) {
                fprintf(stderr, "OP_GETPROP: key not int.\n");
                vm->running = 0;
                return;
            }
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%" PRId64, vm->registers[rk].as.int_val);
            VMObject* obj = (VMObject*)vm->registers[ro].as.obj_ref;
            VMValue val = vm_get_property(vm, obj, buffer);
            vm->registers[rd] = val;
            if (val.type == VAL_OBJ) {
                val.refcount++;
            }
            vm->pc++;
        } break;
        case OP_CORO_INIT: {
            size_t idx = (size_t)inst.operand1;
            if (idx >= MAX_COROUTINES) {
                fprintf(stderr, "OP_CORO_INIT: index out of range\n");
                vm->running = 0;
                return;
            }
            size_t cindex = vm_create_coroutine(vm);
            if (cindex != idx) {
                /* Simplistic handling */
            }
            vm->pc++;
        } break;
        case OP_CORO_YIELD:
            vm_coroutine_yield(vm);
            vm->pc++;
            break;
        case OP_CORO_RESUME: {
            size_t idx = (size_t)inst.operand1;
            vm_coroutine_resume(vm, idx);
            vm->pc++;
        } break;
        default:
            fprintf(stderr, "Unknown opcode %d at PC %zu\n", inst.opcode, vm->pc);
            vm->running = 0;
            break;
    }
}

static void vm_push_frame(VM* vm, Frame* frame, size_t return_address) {
    if (vm->call_stack_top >= 1024) {
        fprintf(stderr, "Call stack overflow!\n");
        vm->running = 0;
        return;
    }
    vm->call_stack[vm->call_stack_top] = frame;
    vm->return_addresses[vm->call_stack_top] = return_address;
    vm->call_stack_top++;
}

static void vm_pop_frame(VM* vm) {
    if (vm->call_stack_top == 0) {
        fprintf(stderr, "Call stack underflow!\n");
        vm->running = 0;
        return;
    }
    vm->call_stack_top--;
    Frame* top = vm->call_stack[vm->call_stack_top];
    size_t ret_addr = vm->return_addresses[vm->call_stack_top];
    frame_destroy(top);
    vm->call_stack[vm->call_stack_top] = NULL;
    vm->pc = ret_addr;
}

void vm_retain_object(VM* vm, VMObject* obj) {
    if (!obj) return;
    obj->refcount++;
}

void vm_release_object(VM* vm, VMObject* obj) {
    if (!obj) return;
    obj->refcount--;
    if (obj->refcount <= 0) {
        for (size_t i = 0; i < vm->object_count; i++) {
            if (vm->objects[i] == obj) {
                destroy_object(vm->objects[i]);
                for (size_t j = i; j < vm->object_count - 1; j++) {
                    vm->objects[j] = vm->objects[j+1];
                }
                vm->object_count--;
                return;
            }
        }
    }
}

void vm_gc_collect(VM* vm) {
    (void)vm;
}

VMObject* vm_create_object(VM* vm) {
    VMObject* obj = (VMObject*)malloc(sizeof(VMObject));
    memset(obj, 0, sizeof(VMObject));
    obj->refcount = 1;
    vm_grow_object_array(vm);
    vm->objects[vm->object_count++] = obj;
    return obj;
}

bool vm_set_property(VM* vm, VMObject* obj, const char* key, VMValue val) {
    (void)vm;
    for (size_t i = 0; i < obj->fields.count; i++) {
        if (strcmp(obj->fields.keys[i], key) == 0) {
            obj->fields.values[i] = val;
            return true;
        }
    }
    if (obj->fields.count >= obj->fields.capacity) {
        size_t new_cap = (obj->fields.capacity == 0) ? 4 : obj->fields.capacity * 2;
        obj->fields.keys = (char**)realloc(obj->fields.keys, new_cap * sizeof(char*));
        obj->fields.values = (VMValue*)realloc(obj->fields.values, new_cap * sizeof(VMValue));
        obj->fields.capacity = new_cap;
    }
    size_t idx = obj->fields.count++;
    obj->fields.keys[idx] = strdup(key);
    obj->fields.values[idx] = val;
    return true;
}

VMValue vm_get_property(VM* vm, VMObject* obj, const char* key) {
    (void)vm;
    for (size_t i = 0; i < obj->fields.count; i++) {
        if (strcmp(obj->fields.keys[i], key) == 0) {
            return obj->fields.values[i];
        }
    }
    VMValue none;
    none.type = VAL_NULL;
    none.refcount = 0;
    none.as.int_val = 0;
    return none;
}

size_t vm_create_coroutine(VM* vm) {
    for (size_t i = 0; i < MAX_COROUTINES; i++) {
        if (!vm->coroutines[i].active) {
            vm->coroutines[i].active = true;
            vm->coroutines[i].pc = 0;
            vm->coroutines[i].frame = NULL;
            for (int r = 0; r < 16; r++) {
                vm->coroutines[i].registers[r].type = VAL_NULL;
                vm->coroutines[i].registers[r].refcount = 0;
            }
            return i;
        }
    }
    return 0;
}

void vm_coroutine_yield(VM* vm) {
    size_t c = vm->current_coro;
    if (!vm->coroutines[c].active) {
        fprintf(stderr, "Yield from inactive coroutine?\n");
        return;
    }
    vm->coroutines[c].pc = vm->pc;
    size_t next = (c + 1) % MAX_COROUTINES;
    for (size_t i = 0; i < MAX_COROUTINES; i++) {
        size_t idx = (c + 1 + i) % MAX_COROUTINES;
        if (vm->coroutines[idx].active) {
            next = idx;
            break;
        }
    }
    vm->current_coro = next;
    vm->pc = vm->coroutines[next].pc;
}

void vm_coroutine_resume(VM* vm, size_t coro_index) {
    if (coro_index >= MAX_COROUTINES || !vm->coroutines[coro_index].active) {
        fprintf(stderr, "Coroutine %zu is invalid or inactive.\n", coro_index);
        return;
    }
    vm_coroutine_yield(vm);
    vm->coroutines[vm->current_coro].pc = vm->pc;
    vm->current_coro = coro_index;
    vm->pc = vm->coroutines[coro_index].pc;
}

bool vm_register_native(VM* vm, const char* name, VMValue(*func)(int, VMValue*)) {
    if (!vm) {
        fprintf(stderr, "ERROR: NULL VM passed to vm_register_native\n");
        return false;
    }
    
    if (!name) {
        fprintf(stderr, "ERROR: NULL name passed to vm_register_native\n");
        return false;
    }
    
    if (!func) {
        fprintf(stderr, "ERROR: NULL function pointer passed to vm_register_native\n");
        return false;
    }

    if (vm->native_count >= 64) {
        fprintf(stderr, "ERROR: Native registry full (max 64 functions)\n");
        return false;
    }

    for (size_t i = 0; i < vm->native_count; i++) {
        if (strcmp(vm->native_registry[i].name, name) == 0) {
            vm->native_registry[i].func = func;
            return true;
        }
    }

    vm->native_registry[vm->native_count].name = name;
    vm->native_registry[vm->native_count].func = func;
    vm->native_count++;
    
    return true;
}

VMValue vm_call_native(VM* vm, const char* name, int arg_count, VMValue* args) {
    for (size_t i = 0; i < vm->native_count; i++) {
        if (strcmp(vm->native_registry[i].name, name) == 0) {
            return vm->native_registry[i].func(arg_count, args);
        }
    }
    VMValue v;
    v.type = VAL_NULL;
    v.refcount = 0;
    v.as.int_val = 0;
    return v;
}

#ifdef ENABLE_JIT
void vm_jit_compile(VM* vm) {
    printf("JIT compilation stub: no real code generated.\n");
    (void)vm;
}
#endif

static VMValue vmvalue_from_int(int64_t n) {
    VMValue v;
    v.type = VAL_INT;
    v.refcount = 0;
    v.as.int_val = n;
    return v;
}

static void vm_grow_object_array(VM* vm) {
    if (vm->object_count >= vm->object_capacity) {
        size_t new_cap = (vm->object_capacity == 0) ? 8 : vm->object_capacity * 2;
        vm->objects = (VMObject**)realloc(vm->objects, new_cap * sizeof(VMObject*));
        vm->object_capacity = new_cap;
    }
}

static void destroy_object(VMObject* obj) {
    if (!obj) return;
    for (size_t i = 0; i < obj->fields.count; i++) {
        free(obj->fields.keys[i]);
    }
    free(obj->fields.keys);
    free(obj->fields.values);
    free(obj);
}

void vm_dump_registers(const VM* vm) {
    for (int i = 0; i < 16; i++) {
        const VMValue* v = &vm->registers[i];
        switch (v->type) {
            case VAL_INT:
                printf("R%d: INT(%" PRId64 "), refcount=%d\n", i, v->as.int_val, v->refcount);
                break;
            case VAL_FLOAT:
                printf("R%d: FLOAT(%f), refcount=%d\n", i, v->as.float_val, v->refcount);
                break;
            case VAL_BOOL:
                printf("R%d: BOOL(%s), refcount=%d\n", i, v->as.bool_val ? "true" : "false", v->refcount);
                break;
            case VAL_NULL:
                printf("R%d: NULL, refcount=%d\n", i, v->refcount);
                break;
            case VAL_OBJ:
                printf("R%d: OBJ(%p), refcount=%d\n", i, v->as.obj_ref, v->refcount);
                break;
            case VAL_STRING:
                printf("R%d: STRING(%s), refcount=%d\n", i, v->as.str_val, v->refcount);
                break;
            default:
                printf("R%d: Unknown type, refcount=%d\n", i, v->refcount);
                break;
        }
    }
}

VMValue vm_get_register_value(const VM* vm, int reg_index) {
    VMValue none;
    none.type = VAL_NULL;
    none.refcount = 0;
    if (!vm || reg_index < 0 || reg_index >= 16) {
        return none;
    }
    return vm->registers[reg_index];
}
