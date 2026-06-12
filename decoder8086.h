#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int err;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef u8 b8;
typedef u32 b32;
#define TRUE 1
#define FALSE 0

#define LEN(array) (sizeof array / sizeof *array)
#define MASK(in) ((in >= 1 ? 1 << 0 : 0) | (in >= 2 ? 1 << 1 : 0) | \
				  (in >= 3 ? 1 << 2 : 0) | (in >= 4 ? 1 << 3 : 0) | \
				  (in >= 5 ? 1 << 4 : 0) | (in >= 6 ? 1 << 5 : 0) | \
				  (in >= 7 ? 1 << 6 : 0) | (in >= 8 ? 1 << 7 : 0))

#ifdef _DEBUG
#define ASSERT(exp, msg) if (!exp) {fprintf(stderr, "ASSERTION AT LINE %u: " msg "\n", __LINE__); *(int *)0 = 0; }
#define LOG_MSG(msg) fprintf(stderr, "LOG: " msg "\n")
#define LOG_VAR(var, conversion_specifier) fprintf(stderr, "LOG: " #var " = %" #conversion_specifier "\n", var)
#else
#define ASSERT(exp, msg) {}
#define LOG_MSG(msg) {}
#define LOG_VAR(var, conversion_specifier) {}
#endif

/* TODO: Make decoder capable of disassemble binary larger than 1MiB and take more than 64 KiB instructions */
#define MAX_BYTES_READ 1024 * 1024
#define MAX_INSTRUCTIONS 256 * 256
#define MAX_INSTRUCTION_ASM_SIZE 48
#define MAX_OPERAND_ASM_SIZE 32

enum opcode {
	op_none,

#define INST_MNE_ENUM
#include "instructions.inl"

	op_count
};

enum mod_field {
	mod_mem_no_disp,
	mod_mem_8_disp,
	mod_mem_16_disp,
	mod_reg
};

enum instruction_bits_type {
	bits_end,

	bits_literal,
	bits_s,
	bits_w,
	bits_d,
	bits_v,
	bits_z,
	bits_mod,
	bits_reg,
	bits_seg_reg,
	bits_rm,
	bits_disp_lo,
	bits_disp_hi,
	bits_data,
	bits_data_if_w,

	bits_rm_is_w,
	bits_is_rel_jmp,
	bits_force_disp,
	bits_is_far,

	bits_count
};

struct instruction_bits {
	enum instruction_bits_type type;
	u8 bit_count;
	u8 value;
};

struct instruction_encoding {
	enum opcode opcode;
	struct instruction_bits bits[12];
};

enum operand_type {
	operand_none,
	operand_memory,
	operand_register,
	operand_immediate,
	operand_relative_immediate
};

enum flags {
	flags_wide,
	flags_lock,
	flags_rep,
	flags_segment,
	flags_far,
	flags_segment_register /* 2 bits */
};

struct instruction_operand {
	enum operand_type type;
	union {
		u8 reg;
		struct {
			u8 rm;
			i32 displacement;
		} effective_address;
		i32 data;
	} value;
};

struct instruction {
	struct instruction_operand operands[2];
	enum opcode opcode;
	u8 flags;
	u8 size;
};

/* TODO: Implement a way to label jump that consumes less space, O(1) ideally */
struct asm_buffer {
	void *memory_block;
	
	char *texts;

	u16 *label_numbers;
	b8 *is_cond_jumps;
	i16 *ip_incs;
	u8 *instruction_sizes;
	u32 label_count;
	
	u32 bytes_per_text;
	u32 instruction_count;
};
