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

/* There is no guard/safety for overflow */
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
	bits_ip_inc,

	bits_rm_always_w,

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

const char *mnemonic_arr[] = {
	"",

#define INST_MNE_STRING_LITERAL
#include "instructions.inl"

};

enum operand_type {
	operand_none,
	operand_memory,
	operand_direct_address,
	operand_register,
	operand_segment_register,
	operand_immediate,
	operand_ip_inc
};

struct instruction_encoding instruction_table[] = {

#define INST_TABLE
#include "instructions.inl"

};

enum flags {
	flags_wide    = 1 << 0,
	flags_lock    = 1 << 1,
	flags_rep     = 1 << 2,
	flags_segment = 1 << 3,
	flags_segment_register = 3 << 4
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
		i16 ip_inc;
	} value;
};

struct instruction {
	struct instruction_operand operands[2];
	enum opcode opcode;
	u8 flags;
	u8 size;
};

struct asm_buffer {
	void *memory_block;
	
	char *texts;

	/* For printing control transfer assembly
	   but it really feels clunky. Maybe improvement can be made here.*/
	u16 *label_numbers;
	b8 *is_cond_jumps;
	i16 *ip_incs;
	u8 *instruction_sizes;
	u32 label_count;
	
	u32 bytes_per_text;
	u32 instruction_count;
};

char const *reg_field_asm_text[8][2] = {
	{"al", "ax"},
	{"cl", "cx"},
	{"dl", "dx"},
	{"bl", "bx"},
	{"ah", "sp"},
	{"ch", "bp"},
	{"dh", "si"},
	{"bh", "di"},
};

char const *seg_reg_field_asm_text[4] = {"es", "cs", "ss", "ds"};

char const *mem_field_asm_text[8][3] = {
	{"[bx + si]", "[bx + si + %u]", "[bx + si - %u]"},
	{"[bx + di]", "[bx + di + %u]", "[bx + di - %u]"},
	{"[bp + si]", "[bp + si + %u]", "[bp + si - %u]"},
	{"[bp + di]", "[bp + di + %u]", "[bp + di - %u]"},
	{"[si]"     , "[si + %u]"     , "[si - %u]"     },
	{"[di]"     , "[di + %u]"     , "[di - %u]"     },
	{"[bp]"     , "[bp + %u]"     , "[bp - %u]"     },
	{"[bx]"     , "[bx + %u]"     , "[bx - %u]"     },
};

void debug_output_binary_instruction(FILE *pipe, i32 at, u32 byte_count, u8 const *input) {
	char output[8 * 6 + 7];
	u32 i, j;
	for (i = 0; i < byte_count; ++i) {
		for (j = 0; j < 8; ++j) {
			output[i * 9 + j] = '0' + (input[i] >> (7 - j) & 1);
		}
		output[i * 9 + j] = ' ';
	}
	output[i * 9 - 1] = '\0';
	
	if (at < 0) {
		fprintf(pipe, "%s", output);
	} else {
		fprintf(pipe, "%04x: %s\n", at, output);
	}
}

void print_all_instructions(FILE *output, struct asm_buffer const asm_buf, u8 const *bytes, u32 const debug_level) {
	char const *label = "\nlabel%u:\n";
	u32 at = 0;
	u32 i;
	
	fprintf(output, "bits 16\nCPU 8086\n\n");
	
	for (i = 0; i < asm_buf.instruction_count; i += 1) {
		if (asm_buf.label_numbers[i]) {
			fprintf(output, label, asm_buf.label_numbers[i] - 1);
		}
		if (debug_level >= 1) {
			fprintf(output, ";;; ");
			debug_output_binary_instruction(output, at, asm_buf.instruction_sizes[i], bytes + at);
			at += asm_buf.instruction_sizes[i];
		}

		fprintf(output, asm_buf.texts + asm_buf.bytes_per_text * i);
		fprintf(output, "\n");

		if (debug_level >= 1) fprintf(output, "\n");
	}

	if (asm_buf.label_numbers[i]) {
		fprintf(output, label, asm_buf.label_numbers[i] - 1);
	}
}

void get_and_mark_label_number(struct asm_buffer *asm_buf,
							  i32 const ip_inc,
							  u32 const instruction_number,
							  u32 *out_label_number) {
	i32 direction = 1;
	i32 displacement = ip_inc;
	u32 label_line = instruction_number + 1;
	
	if (displacement < 0) {
		direction = -1;
		displacement *= -1;
	}
	
	while (displacement > 0) {
		displacement -= asm_buf->instruction_sizes[label_line - 1];
		label_line += direction;
	}

	if (asm_buf->label_numbers[label_line] == 0) {
		asm_buf->label_count += 1;
		asm_buf->label_numbers[label_line] = asm_buf->label_count;
	}

	*out_label_number = (u32)asm_buf->label_numbers[label_line];
}

void assign_all_labels(struct asm_buffer *asm_buf) {
	char temp[MAX_INSTRUCTION_ASM_SIZE];
	u32 i, label_number;

	for (i = 0; i < asm_buf->instruction_count; i += 1) {
		if (asm_buf->is_cond_jumps[i]) {
			get_and_mark_label_number(asm_buf, (i32)asm_buf->ip_incs[i], i, &label_number);

			sprintf(temp, asm_buf->texts + asm_buf->bytes_per_text * i, label_number - 1);
			sprintf(asm_buf->texts + asm_buf->bytes_per_text * i, temp);
		}
	}
}

err asm_buffer_initialize(struct asm_buffer *result) {
	result->memory_block =
		calloc(sizeof *result->texts * MAX_INSTRUCTION_ASM_SIZE +
			   sizeof *result->label_numbers +
			   sizeof *result->is_cond_jumps +
			   sizeof *result->ip_incs +
			   sizeof *result->instruction_sizes,
			   MAX_INSTRUCTIONS);
	
	if (result->memory_block == NULL) return 1;

	result->texts             = result->memory_block;
	result->label_numbers     = (u16 *)(result->texts         + MAX_INSTRUCTIONS * MAX_INSTRUCTION_ASM_SIZE);
	result->is_cond_jumps     = (b8  *)(result->label_numbers + MAX_INSTRUCTIONS);
	result->ip_incs           = (i16 *)(result->is_cond_jumps + MAX_INSTRUCTIONS);
	result->instruction_sizes = (u8  *)(result->ip_incs       + MAX_INSTRUCTIONS);
	
	result->label_count = 0;
	result->bytes_per_text = MAX_INSTRUCTION_ASM_SIZE;
	result->instruction_count = 0;
	
	return 0;
}

void asm_buffer_uninitialize(struct asm_buffer *asm_buf) {
	free(asm_buf->memory_block);
}

b32 is_shift_instruction(enum opcode opcode) {
	if (opcode >= op_shl && opcode <= op_rcr) return TRUE;
	return FALSE;
}

b32 is_string_instruction(enum opcode opcode) {
	if (opcode >= op_movs && opcode <= op_stos) return TRUE;
	return FALSE;
}

void format_instruction(struct instruction inst, struct asm_buffer *asm_buf, u32 const debug_level) {
	char *target_asm = asm_buf->texts + asm_buf->bytes_per_text * asm_buf->instruction_count;
	char temp_operand_asm[MAX_OPERAND_ASM_SIZE];
	u32 operand_count = 2;
	u32 i, ea_mode;

	if (inst.opcode == op_none) return;

	target_asm[0] = '\t';

	if (inst.flags & flags_lock) {
		strcat(target_asm, "lock ");
	} else if (inst.flags & flags_rep) {
		strcat(target_asm, "rep ");
	}

	strcat(target_asm, mnemonic_arr[inst.opcode]);

	if (is_string_instruction(inst.opcode)) {
		strcat(target_asm, inst.flags & flags_wide ? "w" : "b");
	}

	for (i = 0; i < 2; i += 1) {
		if (inst.operands[i].type == operand_none) {
			operand_count -= 1;
			continue;
		}

		if (i == 0) strcat(target_asm, " ");
		else strcat(target_asm, ", ");
		
		switch (inst.operands[i].type) {
		case operand_register:
			sprintf(temp_operand_asm,
					reg_field_asm_text[inst.operands[i].value.reg % 8][inst.operands[i].value.reg / 8]);
		break;
		case operand_segment_register:
			sprintf(temp_operand_asm, seg_reg_field_asm_text[inst.operands[i].value.reg]);
		break;
		/* NOTE: Maybe we should merge `operand_memory` with
		         `operand_direct_address` to reduce redundancy here */
		case operand_memory:
			ea_mode = 0;
			if (inst.operands[i].value.effective_address.displacement > 0) {
				ea_mode = 1;
			} else if (inst.operands[i].value.effective_address.displacement < 0) {
				inst.operands[i].value.effective_address.displacement *= -1;
				ea_mode = 2;
			}
			
			if (inst.opcode != op_mov && (inst.operands[1].type == operand_immediate ||
										  inst.operands[1].type == operand_none ||
										  is_shift_instruction(inst.opcode))) {
				if (inst.flags & flags_wide) strcat(target_asm, "word ");
				else strcat(target_asm, "byte ");
			}

			if (inst.flags & flags_segment) {
				strcat(target_asm, seg_reg_field_asm_text[(inst.flags & flags_segment_register) >> 4]);
				strcat(target_asm, ":");
			}

			sprintf(temp_operand_asm,
					mem_field_asm_text[inst.operands[i].value.effective_address.rm][ea_mode],
					inst.operands[i].value.effective_address.displacement);
		break;
		case operand_direct_address:
			if (inst.opcode != op_mov && (inst.operands[1].type == operand_immediate ||
										  inst.operands[1].type == operand_none ||
										  is_shift_instruction(inst.opcode))) {
				if (inst.flags & flags_wide) strcat(target_asm, "word ");
				else strcat(target_asm, "byte ");
			}

			if (inst.flags & flags_segment) {
				strcat(target_asm, seg_reg_field_asm_text[(inst.flags & flags_segment_register) >> 4]);
				strcat(target_asm, ":");
			}
			
			sprintf(temp_operand_asm, "[%u]", inst.operands[i].value.effective_address.displacement);
		break;
		case operand_immediate:
			if (inst.opcode == op_mov) {
				if (inst.flags & flags_wide) strcat(target_asm, "word ");
				else strcat(target_asm, "byte ");
			}
			
			sprintf(temp_operand_asm, "%i", inst.operands[i].value.data);
		break;
		case operand_ip_inc:
			asm_buf->is_cond_jumps[asm_buf->instruction_count] = TRUE;
			asm_buf->ip_incs[asm_buf->instruction_count] = inst.operands[i].value.ip_inc;
			
			sprintf(temp_operand_asm, "label%%u ; %i", inst.operands[i].value.ip_inc);
		break;
		default:
			ASSERT(FALSE, "Unknown operand.");
		break;
		}
		
		strcat(target_asm, temp_operand_asm);
	}

	asm_buf->instruction_sizes[asm_buf->instruction_count] = inst.size;

	if (debug_level >= 2) {
		fprintf(stderr, target_asm);
		fprintf(stderr, "\n");
		if (debug_level >= 3) {
			fprintf(stderr, "\n");
		}
	}
}

err decode(u8 const *memory, u32 const at, struct instruction *inst, u8 *flags, u32 const debug_level) {
	u8 const *bytes = memory + at;
	u32 bytes_read;
	u32 bits_read;

	u32 bits[bits_count];

	struct instruction_encoding *cur_enc;
	struct instruction_bits *cur_bits;

	for (cur_enc = instruction_table;
		 cur_enc < instruction_table + LEN(instruction_table);
		 cur_enc += 1) {
		b32 valid = TRUE;
		bytes_read = 0;
		bits_read = 0;
		memset(bits, 0xff, sizeof *bits * bits_count);
		bits[bits_rm_always_w] = FALSE;
		
		for (cur_bits = cur_enc->bits; cur_bits->type != bits_end; cur_bits += 1) {
			u8 const bits_remain = 8 - bits_read % 8;
			u8 bits_value;

			if (cur_bits->bit_count == 0) bits_value = cur_bits->value;
			else bits_value = (bytes[bytes_read] >> (bits_remain - cur_bits->bit_count)) & MASK(cur_bits->bit_count);
			
			switch (cur_bits->type) {
			case bits_literal:
				if (bits_value != cur_bits->value) {
					valid = FALSE;
					goto next_encoding;
				}
			break;				
			case bits_disp_lo:
				bits[bits_disp_lo] = 0;
				bits[bits_disp_hi] = 0;
				if ((bits[bits_mod] == mod_mem_no_disp && bits[bits_rm] == 6) || bits[bits_mod] == mod_mem_8_disp ||
					bits[bits_mod] == mod_mem_16_disp) {
					bits[bits_disp_lo] = bits_value;
				} else continue;
			break;
			case bits_disp_hi:
				if ((bits[bits_mod] == mod_mem_no_disp && bits[bits_rm] == 6) || bits[bits_mod] == mod_mem_16_disp) {
					bits[bits_disp_hi] = bits_value;
				} else continue;
			break;
			case bits_data:
				bits[bits_data] = bits_value;
				bits[bits_data_if_w] = 0;
			break;
			case bits_data_if_w:
				if (bits[bits_w] && !bits[bits_s]) bits[bits_data_if_w] = bits_value;
				else continue;
			break;
			default:
				bits[cur_bits->type] = bits_value;
			break;
			}
		
			bits_read += cur_bits->bit_count;
			bytes_read = bits_read / 8;
		}
next_encoding:
		if (valid) break;
	}

	if (cur_enc == instruction_table + LEN(instruction_table)) {
		debug_output_binary_instruction(stderr, at, 6, bytes);
		fprintf(stderr, "ERROR: No encoding with valid bits_literal was found.\n");
		return 1;
	} else if (debug_level >= 3) {
		fprintf(stderr, "Encoding[%u]\n", (u32)(cur_enc - instruction_table));
		debug_output_binary_instruction(stderr, at, bytes_read, bytes);
	}
	
	inst->size = bytes_read;

	/* Check for flags type of instructions or transfer flags to the instruction */
	switch (cur_enc->opcode) {
	case op_lock:
		*flags |= flags_lock;
		return 0;
	case op_rep:
		*flags |= flags_rep;
		return 0;
	case op_segment:
		*flags |= flags_segment;
		*flags |= bits[bits_seg_reg] << 4;
		return 0;
	default:
		inst->flags = *flags;
		*flags = 0;
	break;
	}
	
	{
		b32 const wide_instruction_mode = bits[bits_w];
		b32 const direction = bits[bits_d];
		
		b32 const v_exist = bits[bits_v] != -1;
		/* b32 const z_exist = bits[bits_z] != -1; */

		b32 const reg_exist = bits[bits_reg] != -1;
		b32 const wide_reg_mode = wide_instruction_mode;
		b32 const reg_is_src = !direction;

		b32 const seg_reg_exist = bits[bits_seg_reg] != -1;
		
		b32 const rm_exist = bits[bits_rm] != -1;
		b32 const rm_reg_mode = bits[bits_mod] == mod_reg;
		b32 const wide_rm_mode = wide_reg_mode || bits[bits_rm_always_w];
		b32 const direct_address_mode = bits[bits_mod] == mod_mem_no_disp && bits[bits_rm] == 6;
		b32 const disp_mode = bits[bits_mod] == mod_mem_8_disp || bits[bits_mod] == mod_mem_16_disp;
		b32 const wide_disp_mode = bits[bits_mod] == mod_mem_16_disp;
		b32 const rm_is_src = direction;
		
		b32 const immediate_mode = bits[bits_data] != -1;
		b32 const wide_immediate_mode = wide_instruction_mode;
		b32 const sign_immediate_mode = bits[bits_s];

		b32 const control_transfer_mode = bits[bits_ip_inc] != -1;
	
		if (wide_instruction_mode) inst->flags |= flags_wide;
		else inst->flags &= inst->flags ^ flags_wide;
		inst->opcode = cur_enc->opcode;
		
		if (reg_exist) {
			struct instruction_operand *operand = &inst->operands[reg_is_src];
			
			operand->type = operand_register;
			operand->value.reg = bits[bits_reg] + 8 * wide_reg_mode;
		} else if (seg_reg_exist) {
			struct instruction_operand *operand = &inst->operands[reg_is_src];
			
			operand->type = operand_segment_register;
			operand->value.reg = bits[bits_seg_reg];
		} else if (v_exist) {
			struct instruction_operand *operand = &inst->operands[1];

			if (bits[bits_v]) {
				operand->type = operand_register;
				operand->value.reg = 1;
			} else {
				operand->type = operand_immediate;
				operand->value.data = 1;
			}
		}

		if (rm_exist) {
			struct instruction_operand *operand = &inst->operands[rm_is_src];
			
			if (rm_reg_mode) {
				operand->type = operand_register;
				operand->value.reg = bits[bits_rm] + 8 * wide_rm_mode;
			} else if (direct_address_mode) {
				operand->type = operand_direct_address;
			} else {
				operand->type = operand_memory;
				operand->value.effective_address.rm = bits[bits_rm];
			}
			
			operand->value.effective_address.displacement = (u16)((bits[bits_disp_hi] << 8) | bits[bits_disp_lo]);
			
			/* sign-extension */
			if (disp_mode && !wide_disp_mode) {
				if (operand->value.effective_address.displacement >> 7) {
					operand->value.effective_address.displacement |= 0xffffff00;
				}
			} else if (!direct_address_mode && operand->value.effective_address.displacement >> 15) {
				operand->value.effective_address.displacement |= 0xffff0000;
			}
		}

		if (immediate_mode) {
			/* be the opposite operand of whatever is already assigned */
			struct instruction_operand *operand =
				&inst->operands[!((reg_exist && reg_is_src) || (rm_exist && rm_is_src)) &&
								(reg_exist || rm_exist)];
			
			operand->type = operand_immediate;
			operand->value.data = (bits[bits_data_if_w] << 8) | bits[bits_data];
			/* sign_extension */
			if (!wide_immediate_mode) {
				if (sign_immediate_mode && operand->value.data >> 7) {
					operand->value.data |= 0xffffff00;
				}
			} else if (operand->value.data >> 15) {
				operand->value.data |= 0xffff0000;
			}
		}

		if (control_transfer_mode) {
			struct instruction_operand *operand = &inst->operands[0];
			
			operand->type = operand_ip_inc;
			operand->value.ip_inc = bits[bits_ip_inc];
			/* sign_extension */
			if (operand->value.ip_inc >> 7) {
				operand->value.ip_inc |= 0xff00;
			}
		}
	}

	return 0;
}

err disasm(u8 const *memory, i32 const total_bytes, FILE *output, u32 const debug_level) {
	struct asm_buffer asm_buf;
	i32 bytes_left = total_bytes;
	u8 flags;
	err error;
	
	error = asm_buffer_initialize(&asm_buf);
	if (error) {
		fprintf(stderr, "ERROR: asm_buffer_initialize failed.\n");
		return 1;
	}
		
	while (bytes_left > 0) {
		u32 const bytes_at = total_bytes - bytes_left;
		struct instruction inst = {0};
		
		error = decode(memory, bytes_at, &inst, &flags, debug_level);
		if (error) return 1;
		
		format_instruction(inst, &asm_buf, debug_level);
		
		bytes_left -= inst.size;
		if (inst.opcode != op_none) asm_buf.instruction_count += 1;
	}

	if (bytes_left < 0) {
		fprintf(stderr, "ERROR: Decoder overread.\n");
		return 1;
	}

	assign_all_labels(&asm_buf);
	print_all_instructions(output, asm_buf, memory, debug_level);
	
	asm_buffer_uninitialize(&asm_buf);
	return 0;
}

err load_memory_from_file(void *memory, char *input_filename, u32 *bytes_read) {
	FILE *input_file = fopen(input_filename, "rb");
	if (input_file == NULL) {
		fprintf(stderr, "ERROR: Unable to open input file.\n");
		return 1;
	}
	
	*bytes_read = (u32)fread(memory, 1, 1024 * 1024, input_file);

	fclose(input_file);
	return 0;
}

int main(int arg_count, char **args) {
	FILE *output = stdout;
	u32 debug_level = 0;
	char *input_filename = NULL;
	char *output_filename = NULL;
	b32 output_to_file_mode = FALSE;
	
	u32 bytes_read, i;
	err error;
	
	void *memory = malloc(MAX_BYTES_READ);	

	for (i = 1; i < arg_count; i += 1) {
		if (args[i][0] == '-') {
			switch (args[i][1]) {
			case 'd':
				if (args[i][2] >= '0' && args[i][2] <= '3' && args[i][3] == '\0') {
					debug_level = args[i][2] - '0';
				} else goto misuse;
			break;
			case 'o':
				if (args[i][2] != '\0') goto misuse;
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

	if (input_filename == NULL) goto misuse;

	if (output_filename != NULL) {
		output = fopen(output_filename, "wb");
	}
	
	if (output == NULL) {
		fprintf(stderr, "ERROR: Unable to open output file\n");
		goto failed_exit;
	}

	error = load_memory_from_file(memory, input_filename, &bytes_read);
	if (error) goto misuse;

	error = disasm((u8 *)memory, (u32)bytes_read, output, debug_level);
	if (error) goto failed_exit;
	
	if (output != stdout) fclose(output);
	free(memory);
	return 0;
misuse:
	if (output != stdout) {
		fclose(output);
		remove(output_filename);
	}
	free(memory);
	fprintf(stderr, "\nUsage:\n"
			"\tdecode8086 [options] filename\n"
			"Options:\n"
			"\t-o <file>\n\t\tWrite output to file.\n\n"
			"\t-d0, -d1, -d2, -d3\n"
			"\t\tSpecify debug printing level.\n"
			"\t\t   -d0 No debug printing.\n\n"
			"\t\t   -d1 Comment binary instruction to output if exit success.\n\n"
			"\t\t   -d2 Print assembly instruction as the program is decoding/formatting. "
			           "However, Labels will not be displayed correctly.\n\n"
			"\t\t   -d3 Print both binary and assembly instruction as the program"
			           "is decoding/formatting.\n\n");
	return 1;
failed_exit:
	if (output != stdout && output != NULL) {
		fclose(output);
		remove(output_filename);
	}
	free(memory);
	fprintf(stderr, "Exit Failed\n");
	return 1;
}
