//  CITS2002 Project 1 2021
//  Name(s):             student-name1   (, student-name2)
//  Student number(s):   student-number1 (, student-number2)

//  compile with:  cc -std=c11 -Wall -Werror -o runcool runcool.c

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//  THE STACK-BASED MACHINE HAS 2^16 (= 65,536) WORDS OF MAIN MEMORY
#define N_MAIN_MEMORY_WORDS (1<<16)

//  EACH WORD OF MEMORY CAN STORE A 16-bit UNSIGNED ADDRESS (0 to 65535)
#define AWORD               uint16_t
//  OR STORE A 16-bit SIGNED INTEGER (-32,768 to 32,767)
#define IWORD               int16_t

//  THE ARRAY OF 65,536 WORDS OF MAIN MEMORY
AWORD                       main_memory[N_MAIN_MEMORY_WORDS];

//  THE SMALL-BUT-FAST CACHE HAS 32 WORDS OF MEMORY
#define N_CACHE_WORDS       32


//  see:  https://teaching.csse.uwa.edu.au/units/CITS2002/projects/coolinstructions.php
enum INSTRUCTION {
	I_HALT       = 0,
	I_NOP,
	I_ADD,
	I_SUB,
	I_MULT,
	I_DIV,
	I_CALL,
	I_RETURN,
	I_JMP,
	I_JEQ,
	I_PRINTI,
	I_PRINTS,
	I_PUSHC,
	I_PUSHA,
	I_PUSHR,
	I_POPA,
	I_POPR
};

//  USE VALUES OF enum INSTRUCTION TO INDEX THE INSTRUCTION_name[] ARRAY
const char *INSTRUCTION_name[] = {
	"halt",
	"nop",
	"add",
	"sub",
	"mult",
	"div",
	"call",
	"return",
	"jmp",
	"jeq",
	"printi",
	"prints",
	"pushc",
	"pusha",
	"pushr",
	"popa",
	"popr"
};

//  ----  IT IS SAFE TO MODIFY ANYTHING BELOW THIS LINE  --------------

//  THE STACK-BASED MACHINE HAS 2^5 (= 32) WORDS OF MAIN MEMORY
#define N_CACHE_MEMORY_WORDS (1<<5)

typedef struct caches {
	bool valid : 1;
	bool dirty : 1;
	unsigned int tag : 11; // 16 - 5, only 11 bits is required
	AWORD data;
} cache_block;

//  THE ARRAY OF 32 WORDS OF CACHE MEMORY
cache_block cache_memory[N_CACHE_MEMORY_WORDS] = {0};

#define MAX_OPERAND_COUNT 1

//  THE STATISTICS TO BE ACCUMULATED AND REPORTED
int n_main_memory_reads     = 0;
int n_main_memory_writes    = 0;
int n_cache_memory_hits     = 0;
int n_cache_memory_misses   = 0;

void report_statistics(void) {
	printf("@number-of-main-memory-reads\t%i\n",    n_main_memory_reads);
	printf("@number-of-main-memory-writes\t%i\n",   n_main_memory_writes);
	printf("@number-of-cache-memory-hits\t%i\n",    n_cache_memory_hits);
	printf("@number-of-cache-memory-misses\t%i\n",  n_cache_memory_misses);
}

//  -------------------------------------------------------------------

//  EVEN THOUGH main_memory[] IS AN ARRAY OF WORDS, IT SHOULD NOT BE ACCESSED DIRECTLY.
//  INSTEAD, USE THESE FUNCTIONS read_memory() and write_memory()
//
//  THIS WILL MAKE THINGS EASIER WHEN WHEN EXTENDING THE CODE TO
//  SUPPORT CACHE MEMORY

AWORD read_memory(int address) {
	int cache_index = address % 32;
	cache_block* block = &cache_memory[cache_index];
	AWORD cache_physical_address = ((block->tag << 5) | cache_index);

	if (address == cache_physical_address && block->valid) {
		n_cache_memory_hits++;
		return block->data;
	}

	n_cache_memory_misses++;

	if (block->dirty && block->valid) {
		n_main_memory_writes++;
		main_memory[cache_physical_address] = block->data;
	}

	n_main_memory_reads++;
	block->tag = address >> 5;
	block->data = main_memory[address];
	block->dirty = false;
	block->valid = true;

	return block->data;
}

void write_memory(AWORD address, AWORD value) {
	int cache_index = address % 32;
	cache_block* block = &cache_memory[cache_index];
	AWORD cache_physical_address = ((block->tag << 5) | cache_index);

	if (address == cache_physical_address && block->valid) {
		n_cache_memory_hits++;
		block->data = value;
		return;
	}

	n_cache_memory_misses++;

	if (block->dirty && block->valid) {
		n_main_memory_writes++;
		main_memory[cache_physical_address] = block->data;
	}

	block->tag = address >> 5;
	block->data = value;
	block->dirty = true;
	block->valid = true;
}

//  -------------------------------------------------------------------

typedef struct cool_machine {
	int PC, SP, FP;
} cool_machine;

AWORD cool_pop_stack(cool_machine* machine) {
	AWORD value = read_memory(machine->SP++);
	return value;
}

void cool_push_stack(cool_machine* machine, AWORD value) {
	write_memory(--machine->SP, value);
}

#define UNUSED(x) (void)(x)

void cool_add(cool_machine* machine, AWORD* _) {
	UNUSED(_);

	int first = cool_pop_stack(machine);
	int second = cool_pop_stack(machine);
	cool_push_stack(machine, second + first);
}

void cool_sub(cool_machine* machine, AWORD* _) {
	UNUSED(_);

	int first = cool_pop_stack(machine);
	int second = cool_pop_stack(machine);
	cool_push_stack(machine, second - first);
}

void cool_mult(cool_machine* machine, AWORD* _) {
	UNUSED(_);

	int first = cool_pop_stack(machine);
	int second = cool_pop_stack(machine);
	cool_push_stack(machine, second * first);
}

void cool_div(cool_machine* machine, AWORD* _) {
	UNUSED(_);

	int first = cool_pop_stack(machine);
	int second = cool_pop_stack(machine);
	cool_push_stack(machine, second / first);
}

void cool_call(cool_machine* machine, AWORD* operands) {
	AWORD function_address = operands[0];

	cool_push_stack(machine, machine->PC);
	cool_push_stack(machine, machine->FP);
	machine->FP = machine->SP;
	machine->PC = function_address;
}

void cool_return(cool_machine* machine, AWORD* operands) {
	AWORD return_offset = operands[0];
	AWORD return_value = cool_pop_stack(machine);

	int current_frame_pointer = machine->FP;
	machine->FP = read_memory(current_frame_pointer);
	machine->PC = read_memory(current_frame_pointer + 1);
	machine->SP = current_frame_pointer + return_offset;

	write_memory(machine->SP, return_value);
}

void cool_jmp(cool_machine* machine, AWORD* operands) {
	AWORD address = operands[0];
	machine->PC = address;
}

void cool_jeq(cool_machine* machine, AWORD* operands) {
	AWORD address = operands[0];

	AWORD condition = cool_pop_stack(machine);
	if (condition == 0) {
		machine->PC = address;
	}
}

void cool_printi(cool_machine* machine, AWORD* _) {
	UNUSED(_);

	IWORD value = cool_pop_stack(machine);
	printf("%hi", value);
}

void cool_prints(cool_machine* machine, AWORD* operands) {
	UNUSED(machine);

	AWORD cstring_address = operands[0];
	//How am I supposed to do this with read_memory?
	printf("%s", (char *)&main_memory[cstring_address]);
}

void cool_pushc(cool_machine* machine, AWORD* operands) {
	AWORD value = operands[0];

	cool_push_stack(machine, value);
}

void cool_pusha(cool_machine* machine, AWORD* operands) {
	AWORD address = operands[0];

	AWORD value = read_memory(address);
	cool_push_stack(machine, value);
}

void cool_pushr(cool_machine* machine, AWORD* operands) {
	AWORD address = machine->FP + operands[0];

	AWORD value = read_memory(address);
	cool_push_stack(machine, value);
}

void cool_popa(cool_machine* machine, AWORD* operands) {
	AWORD address = operands[0];

	AWORD value = cool_pop_stack(machine);
	write_memory(address, value);
}
void cool_popr(cool_machine* machine, AWORD* operands) {
	AWORD address = machine->FP + operands[0];

	AWORD value = cool_pop_stack(machine);
	write_memory(address, value);
}

typedef struct instruction_handler {
	void (*func)(cool_machine*, AWORD*);
	int operand_count;
} instruction_handler;

const instruction_handler instruction_handlers[] = {
	{NULL, 0}, //halt & nop; these should never be reached
	{NULL, 0},
	{cool_add, 0},
	{cool_sub, 0},
	{cool_mult, 0},
	{cool_div, 0},
	{cool_call, 1},
	{cool_return, 1},
	{cool_jmp, 1},
	{cool_jeq, 1},
	{cool_printi, 0},
	{cool_prints, 1},
	{cool_pushc, 1},
	{cool_pusha, 1},
	{cool_pushr, 1},
	{cool_popa, 1},
	{cool_popr, 1},
};


//  EXECUTE THE INSTRUCTIONS IN main_memory[]
int execute_stackmachine(void) {
	//  THE 3 ON-CPU CONTROL REGISTERS:
	cool_machine machine = {
		.PC = 0, // 1st instruction is at address=0
		.SP = N_MAIN_MEMORY_WORDS, // initialised to top-of-stack
		.FP = 0, // frame pointer
	};

	// effectively AWORD operand, but leave like this for extensibility
	AWORD operands[MAX_OPERAND_COUNT];

	while (true) {

		//  FETCH THE NEXT INSTRUCTION TO BE EXECUTED
		IWORD instruction = read_memory(machine.PC++);
		if (instruction == I_HALT) {
			break;
		} else if (instruction == I_NOP) {
			continue;
		} else if (instruction > I_POPR) {
			fprintf(stderr, "Invalid instruction! Halting...");
			break;
		}

		// printf("%d: %s\n", machine.PC - 1, INSTRUCTION_name[instruction]);
		instruction_handler handler = instruction_handlers[instruction];
		for (IWORD i = 0; i < handler.operand_count; i++) {
			operands[i] = read_memory(machine.PC++);
		}

		(*handler.func)(&machine, operands);
	}

	//  THE RESULT OF EXECUTING THE INSTRUCTIONS IS FOUND ON THE TOP-OF-STACK
	return read_memory(machine.SP);
}

//  -------------------------------------------------------------------

//  READ THE PROVIDED coolexe FILE INTO main_memory[]
void read_coolexe_file(char filename[]) {
	memset(main_memory, 0, sizeof main_memory);   //  clear all memory
	//  READ CONTENTS OF coolexe FILE
	FILE* program_file = fopen(filename, "rb");
	if (program_file == NULL) {
		fprintf(stderr, "Invalid file! Exiting...");
		exit(EXIT_FAILURE);
	}

	fread(main_memory, sizeof main_memory, 1, program_file);
}

//  -------------------------------------------------------------------

int main(int argc, char *argv[]) {
	//  CHECK THE NUMBER OF ARGUMENTS
	if (argc != 2) {
		fprintf(stderr, "Usage: %s program.coolexe\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//  READ THE PROVIDED coolexe FILE INTO THE EMULATED MEMORY
	read_coolexe_file(argv[1]);

	//  EXECUTE THE INSTRUCTIONS FOUND IN main_memory[]
	int result = execute_stackmachine();

	report_statistics();

	return result;          // or  exit(result);
}
