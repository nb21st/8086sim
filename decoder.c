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

enum opcode {
	op_none,

#define _8086emu_INST_MNE_ENUM
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
	bits_mod,
	bits_reg,
	bits_rm,
	bits_disp_lo,
	bits_disp_hi,
	bits_data,
	bits_data_if_w,

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

const char *_8086emu_mnemonic_arr[] = {
	"",

#define _8086emu_INST_MNE_STRING_LITERAL
#include "instructions.inl"

};

enum operand_type {
	operand_none,
	operand_memory,
	operand_direct_address,
	operand_register,
	operand_immediate
};

struct instruction_encoding instruction_table[] = {

#define _8086emu_INST_TABLE
#include "instructions.inl"

};

/* The design is incredibly unstable */
struct instruction_operand {
	enum operand_type type;
	union {
		struct {
			u8 rm;
			i16 displacement;
		} effective_address;
		u8 reg;
		struct {
			i32 data;
		} immediate;
	} value;
};

struct instruction {
	struct instruction_operand operands[2];
	enum opcode opcode;
	b8 wide_mode;
	u8 size;
};

const char *reg_field_asm_text[8][2] = {
	{"al", "ax"},
	{"cl", "cx"},
	{"dl", "dx"},
	{"bl", "bx"},
	{"ah", "sp"},
	{"ch", "bp"},
	{"dh", "si"},
	{"bh", "di"},
};

const char *mem_field_asm_text[8][3] = {
	{"[bx + si]", "[bx + si + %u]", "[bx + si - %u]"},
	{"[bx + di]", "[bx + di + %u]", "[bx + di - %u]"},
	{"[bp + si]", "[bp + si + %u]", "[bp + si - %u]"},
	{"[bp + di]", "[bp + di + %u]", "[bp + di - %u]"},
	{"[si]"     , "[si + %u]"     , "[si - %u]"     },
	{"[di]"     , "[di + %u]"     , "[di - %u]"     },
	{"[bp]"     , "[bp + %u]"     , "[bp - %u]"     },
	{"[bx]"     , "[bx + %u]"     , "[bx - %u]"     },
};

void debug_binary_string(char *output_byte, u32 byte_count, u8 const *input_byte) {
	u32 i, j;

	for (i = 0; i < 6; ++i) {
		u32 k = i * 9;
		for (j = 0; j < 8; ++j) {
			if (i < byte_count) {
				output_byte[k + j] = '0' + (input_byte[i] >> (7 - j) & 1);
			} else {
				output_byte[k + j] = '.';
			}
		}
		output_byte[k + j] = ' ';
	}
}

err print_instruction8086(struct instruction inst, FILE *output) {
	char const *templates[3] = {"%s\n", "%s %s\n", "%s %s, %s\n"};
	char const *size_templates[3] = {"%s", "byte %s", "word %s"};
	char const *mnemonic = _8086emu_mnemonic_arr[inst.opcode];
	char temp_asm[32];
	char operands_asm[2][32];
	u32 operand_count = 2;
	u32 i;
	
	for (i = 0; i < 2; i += 1) {
		u32 size = 0;
		switch (inst.operands[i].type) {
		case operand_none:
			operand_count -= 1;
			break;
		case operand_register:
			sprintf(temp_asm, reg_field_asm_text[inst.operands[i].value.reg][inst.wide_mode]);
			break;
		case operand_memory: {
			u32 mode = 0;
			if (inst.operands[i].value.effective_address.displacement > 0) {
				mode = 1;
			} else if (inst.operands[i].value.effective_address.displacement < 0) {
				inst.operands[i].value.effective_address.displacement *= -1;
				mode = 2;
			}
			if (inst.opcode != op_mov && inst.operands[1].type == operand_immediate) size = 1 + inst.wide_mode;
			sprintf(temp_asm, mem_field_asm_text[inst.operands[i].value.effective_address.rm][mode],
					inst.operands[i].value.effective_address.displacement);
			break;
		} case operand_direct_address:
			if (inst.opcode != op_mov && inst.operands[1].type == operand_immediate) size = 1 + inst.wide_mode;
			sprintf(temp_asm, "[%u]", inst.operands[i].value.effective_address.displacement);
			break;
		case operand_immediate: {
			if (inst.opcode == op_mov) size = 1 + inst.wide_mode;
			sprintf(temp_asm, "%i", inst.operands[i].value.immediate.data);
			break;
		} default:
			fprintf(stderr, "ERROR: Unknown Operand Type\n");
			return -1;
		}
		sprintf(operands_asm[i], size_templates[size], temp_asm);
	}

	fprintf(output, templates[operand_count], mnemonic, operands_asm[0], operands_asm[1]);

	return 0;
}

err decode8086(u8 const *memory, u32 const at, struct instruction *inst, b32 debug_mode) {
	u8 const *bytes = &memory[at];
	u32 bytes_read;
	u32 bits_read;

	b32 has_reg, has_rm, has_data;
	
	u32 reg, rm, mod;
	b32 w, s, d;
	
	u8 data_lo, data_hi;
	u8 disp_lo, disp_hi;

	struct instruction_encoding *cur_enc;
	struct instruction_bits *cur_bits;

	/* Check if all bits_literal are matched */
	for (cur_enc = instruction_table; cur_enc < instruction_table + LEN(instruction_table); cur_enc += 1) {
		b32 valid = TRUE;
		has_reg = FALSE;
		has_rm = FALSE;
		has_data = FALSE;

		bytes_read = 0;
		bits_read = 0;
		
		for (cur_bits = &cur_enc->bits[0]; cur_bits->type != bits_end; cur_bits += 1) {
			u8 const bits_remain = 8 - bits_read % 8;
			u8 bits_value;

			if (cur_bits->bit_count == 0) {
				bits_value = cur_bits->value;
			} else {
				bits_value = (bytes[bytes_read] >> (bits_remain - cur_bits->bit_count)) & MASK(cur_bits->bit_count);
			}
			
			switch (cur_bits->type) {
			case bits_literal:
				if (bits_value != cur_bits->value) {
					valid = FALSE;
					goto next_encoding;
				}
				break;				
			case bits_s:
				s = bits_value;
				break;
			case bits_w:
				w = bits_value;
				break;
			case bits_d:
				d = bits_value;
				break;
			case bits_mod:
				mod = bits_value;
				break;
			case bits_reg:
				reg = bits_value;
				has_reg = TRUE;
				break;
			case bits_rm:
				rm = bits_value;
				has_rm = TRUE; break;
			case bits_disp_lo:
				if ((mod == mod_mem_no_disp && rm == 0x6) || mod == mod_mem_8_disp || mod == mod_mem_16_disp) {
					disp_lo = bits_value;
				} else {
					disp_lo = 0;
					continue;
				}
				break;
			case bits_disp_hi:
				if ((mod == mod_mem_no_disp && rm == 0x6) || mod == mod_mem_16_disp) {
					disp_hi = bits_value;
				} else {
					disp_hi = 0;
					continue;
				}
				break;
			case bits_data:
				data_lo = bits_value;
				has_data = TRUE;
				break;
			case bits_data_if_w:
				if (w && !s) {
					data_hi = bits_value;
				} else {
					data_hi = 0;
					continue;
				}
				break;
			default:
				fprintf(stderr, "ERROR: Unknown bits type\n");
				return -1;
			}
		
			bits_read += cur_bits->bit_count;
			bytes_read = bits_read / 8;
		}
next_encoding:
		if (valid) break;
	}
	
	if (cur_enc == instruction_table + LEN(instruction_table)) {
		fprintf(stderr, "ERROR: No encoding with valid bits_literal was found\n");
		return -1;
	}
	
	{
		b32 const wide_instruction_mode = w;

		b32 const reg_exist = has_reg;
		b32 const reg_is_src = !d;
		b32 const reg_is_dest = d;
	
		b32 const rm_exist = has_rm;
		b32 const rm_reg_mode = mod == mod_reg;
		b32 const direct_address_mode = mod == mod_mem_no_disp && rm == 0x6;
		b32 const displacement_mode = mod == mod_mem_8_disp || mod == mod_mem_16_disp;
		b32 const wide_displacement_mode = mod == mod_mem_16_disp;
		b32 const rm_is_src = d;
		b32 const rm_is_dest = !d;
		
		b32 const immediate_mode = has_data;
		b32 const sign_immediate_mode = s;
	
		inst->opcode = cur_enc->opcode;
		inst->wide_mode = wide_instruction_mode;
		inst->size = bytes_read;
		
		if (reg_exist) {
			struct instruction_operand *operand = &inst->operands[reg_is_src];
			operand->type = operand_register;
			operand->value.reg = reg;
		}

		if (rm_exist) {
			struct instruction_operand *operand = &inst->operands[rm_is_src];
			if (rm_reg_mode) operand->type = operand_register;
			else if (direct_address_mode) operand->type = operand_direct_address;
			else operand->type = operand_memory;
			
			operand->value.effective_address.rm = rm;
			operand->value.effective_address.displacement = (disp_hi << 8) | disp_lo;
			/* sign-extension */
			if (!wide_displacement_mode && !direct_address_mode) {
				if (operand->value.effective_address.displacement >> 7)
					operand->value.effective_address.displacement |= 0xffffff00;
			}
		}

		if (immediate_mode) {
			struct instruction_operand *operand = &inst->operands[1];
			operand->type = operand_immediate;
			operand->value.immediate.data = (data_hi << 8) | data_lo;
			/* sign_extension */
			if (sign_immediate_mode) {
				if (operand->value.immediate.data >> (7 + 8 * wide_instruction_mode)) {
					operand->value.immediate.data |= 0xffffff00 << (8 * wide_instruction_mode);
				}
			}
		}
	}

	if (debug_mode) {
		char byte_text[8 * 6 + 6];
		debug_binary_string(byte_text, inst->size, bytes);
		fprintf(stderr, "%04x: %s| ", at, byte_text);
	}

	return 0;
}

/*
	if (debug_mode) {
	}
*/

err disasm8086(u8 const *memory, i32 const total_bytes, FILE *output, b32 const debug_mode) {
	i32 bytes_left = total_bytes;
	err error;
	
	fprintf(output, "bits 16\n\n");
	
	while (bytes_left > 0) {
		u32 const bytes_at = total_bytes - bytes_left;
		struct instruction inst = {0};
		error = decode8086(memory, bytes_at, &inst, debug_mode);
		if (error) return error;
		print_instruction8086(inst, output);
		bytes_left -= inst.size;
	}

	if (bytes_left < 0) {
		fprintf(stderr, "ERROR: Decoder overread\n");
		return 1;
	}
	
	return 0;
}

err load_memory_from_file(void *memory, char *input_filename, u32 *bytes_read) {
	FILE *input_file = fopen(input_filename, "rb");
	if (input_file == NULL) return -1;
	*bytes_read = (u32)fread(memory, 1, 1024 * 1024, input_file);
	if (ferror(input_file)) return -1;
	fclose(input_file);
	return 0;
}

int main(int arg_count, char **args) {
	void *memory;
	FILE *output = stdout;
	b32 debug_mode = FALSE;
	char *input_filename = NULL;
	char *output_filename = NULL;
	b32 output_to_file_mode = FALSE;
	
	u32 bytes_read, i;
	err error;
	
	for (i = 1; i < arg_count; i += 1) {
		if (args[i][0] == '-') {
			switch (args[i][1]) {
			case 'd':
				debug_mode = TRUE;
				break;
			case 'o':
				output_to_file_mode = TRUE;
				break;
			default:
				goto misuse;
			}
		} else if (output_to_file_mode && output_filename == NULL) {
			output_filename = args[i];
		} else if (input_filename == NULL) {
			input_filename = args[i];
		} else {
			goto misuse;
		}
	}

	if (output_filename != NULL) output = fopen(output_filename, "wb");
	if (output == NULL) goto misuse;

	memory = malloc(1024 * 1024);
	
	error = load_memory_from_file(memory, input_filename, &bytes_read);
	if (error) goto misuse;

	error = disasm8086((u8 *)memory, (u32)bytes_read, output, debug_mode);
	if (error) goto failed_exit;
	
	if (output != stdout || output != NULL) fclose(output);
	return 0;
	
misuse:
	if (output != stdout || output != NULL) fclose(output);
	printf("usage:\n"
		   "\tdecode [options] filename\n"
		   "options:\n"
		   "\t-o <file>\n\t\twrite output to file\n"
		   "\t-d\n\t\tenable debug mode\n");
	return 1;
failed_exit:
	printf("Exit Failed\n");
	return 1;
}
