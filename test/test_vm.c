/*
 * test_vm.c
 *
 * A simple test suite for the advanced VM system defined in vm.h / vm.c.
 * It checks basic arithmetic instructions, jump logic, and function calls.
 */

#include <stdio.h>
#include <assert.h>
#include "../src/vm/vm.h"

/* In the upgraded vm.h/vm.c, be sure you have:
 *   Value vm_get_register_value(const VM* vm, int reg_index);
 *   (or some accessor to check a register's contents)
 * Otherwise, these tests rely on internal VM details.
 */

/* Helper: verifies a register is INT and has a specific value. */
static void assert_register_int_value(const VM* vm, int reg_index, int64_t expected) {
    Value v = vm_get_register_value(vm, reg_index);
    assert(v.type == VAL_INT && "Expected VAL_INT in register.");
    assert(v.as.int_val == expected && "Register did not match expected integer value.");
}

/* TEST 1: Simple arithmetic (ADD, SUB, MUL, DIV) */
static void test_arithmetic(void) {
    /* Program:
       R0 = 10
       R1 = 20
       R2 = R0 + R1   => 30
       R3 = R1 - R0   => 10
       R4 = R0 * R1   => 200
       R1 = 2
       R5 = R4 / R1   => 100
       HALT
    */
    Instruction code[] = {
        { OP_LOAD_CONST, 0, 10, 0 },  /* R0 = 10 */
        { OP_LOAD_CONST, 1, 20, 0 },  /* R1 = 20 */
        { OP_ADD,        2, 0, 1 },   /* R2 = R0 + R1 => 10 + 20 = 30 */
        { OP_SUB,        3, 1, 0 },   /* R3 = R1 - R0 => 20 - 10 = 10 */
        { OP_MUL,        4, 0, 1 },   /* R4 = R0 * R1 => 10 * 20 = 200 */
        { OP_LOAD_CONST, 1, 2,  0 },  /* R1 = 2 (overwrite R1) */
        { OP_DIV,        5, 4, 1 },   /* R5 = R4 / R1 => 200 / 2 = 100 */
        { OP_HALT,       0, 0,  0 },
    };
    Bytecode bc = { code, sizeof(code)/sizeof(Instruction) };

    VM* vm = vm_create(&bc);
    vm_run(vm);

    /* Check register values. */
    assert_register_int_value(vm, 0, 10);  /* original R0 = 10 */
    assert_register_int_value(vm, 2, 30);  /* R2 = 30 */
    assert_register_int_value(vm, 3, 10);  /* R3 = 10 */
    assert_register_int_value(vm, 4, 200); /* R4 = 200 */
    assert_register_int_value(vm, 1, 2);   /* R1 was overwritten to 2 */
    assert_register_int_value(vm, 5, 100); /* R5 = 100 */

    vm_destroy(vm);

    printf("[test_arithmetic] PASSED\n");
}

/* TEST 2: Simple jump logic */
static void test_jumps(void) {
    /* Program:
       R0 = 0
       jumpIfZero => Jump to PC=4 if R0 == 0  // Fixed: now jumping to 4 instead of 5
       R1 = 999
       halt
       R1 = 123
       halt
    */
    Instruction code[] = {
        { OP_LOAD_CONST, 0, 0,  0 },    // PC=0: R0 = 0
        { OP_JUMP_IF_ZERO, 4, 0, 0 },   // PC=1: if R0==0 => PC=4 (fixed from 5)
        { OP_LOAD_CONST, 1, 999,0 },    // PC=2: R1 = 999 (should be skipped)
        { OP_HALT,       0, 0,  0 },    // PC=3: HALT
        { OP_LOAD_CONST, 1, 123,0 },    // PC=4: R1 = 123 (jump target)
        { OP_HALT,       0, 0,  0 }     // PC=5: Final HALT
    };
    Bytecode bc = { code, sizeof(code)/sizeof(Instruction) };

    VM* vm = vm_create(&bc);
    
    // Optional: Dump registers before running
    printf("Initial register state:\n");
    vm_dump_registers(vm);
    
    vm_run(vm);
    
    // Optional: Dump registers after running
    printf("Final register state:\n");
    vm_dump_registers(vm);

    /* R1 should be 123 if jump worked. */
    assert_register_int_value(vm, 1, 123);

    vm_destroy(vm);
    printf("[test_jumps] PASSED\n");
}

/* TEST 3: Function call/return */
static void test_function_call(void) {
    /*
       We'll define a small "function" at instruction index 5 that sets R0=99, then returns.

       Program layout:
         0: LOAD_CONST R0, 10   (pre-call test)
         1: CALL 5              (call the function at address=5)
         2: HALT

       Function at index 5:
         5: LOAD_CONST R0, 99
         6: RET
         (execution returns to the instruction after CALL => PC=2 => HALT)

       Expectation: After CALL/RET, R0 should be 99.
    */
    Instruction code[] = {
        /* Main code */
        { OP_LOAD_CONST, 0, 10,  0 }, /* R0 = 10 */
        { OP_CALL,       5,  0,  0 }, /* CALL function at index=5 */
        { OP_HALT,       0,  0,  0 },

        /* Possibly leftover instructions if you want them, or just fill placeholders. */
        { OP_NOP,        0,  0,  0 }, /* index=3 */
        { OP_NOP,        0,  0,  0 }, /* index=4 */

        /* Function (at index=5) */
        { OP_LOAD_CONST, 0, 99, 0 },  /* set R0 = 99 */
        { OP_RET,        0,  0,  0 }
    };
    Bytecode bc = { code, sizeof(code)/sizeof(Instruction) };

    VM* vm = vm_create(&bc);
    vm_run(vm);

    /* After OP_CALL + OP_RET, we expect R0=99. */
    assert_register_int_value(vm, 0, 99);

    vm_destroy(vm);

    printf("[test_function_call] PASSED\n");
}

/* Main test runner */
int main(void) {
    printf("=== VM Test Suite (Advanced) ===\n");

    test_arithmetic();
    test_jumps();
    test_function_call();

    printf("All VM tests passed successfully!\n");
    return 0;
}
