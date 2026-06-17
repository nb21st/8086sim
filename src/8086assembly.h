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

char const *reg_field_asm_text[3][8] = {
	{"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
	{"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"},
	{"es", "cs", "ss", "ds",                       },
};

char const *mem_field_asm_text[9][3] = {
	{"[bx + si]", "[bx + si + %u]", "[bx + si - %u]"},
	{"[bx + di]", "[bx + di + %u]", "[bx + di - %u]"},
	{"[bp + si]", "[bp + si + %u]", "[bp + si - %u]"},
	{"[bp + di]", "[bp + di + %u]", "[bp + di - %u]"},
	{"[si]"     , "[si + %u]"     , "[si - %u]"     },
	{"[di]"     , "[di + %u]"     , "[di - %u]"     },
	{"[bp]"     , "[bp + %u]"     , "[bp - %u]"     },
	{"[bx]"     , "[bx + %u]"     , "[bx - %u]"     },
	{"[%u]"     , "[%u]"          , "[%u]"          },
};

const char *mnemonic_arr[] = {
	"",

#define INST_MNE_STRING_LITERAL
#include "instructions.inl"

};

b32 is_shift_instruction(enum opcode opcode);
b32 is_string_instruction(enum opcode opcode);
b32 is_conditional_transfer_instruction(enum opcode opcode);
