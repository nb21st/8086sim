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

/* There is no guard/safety for overflow */
#define MAX_READ 1024 * 1024
#define MAX_INSTRUCTIONS 256 * 256
#define MAX_INSTRUCTION_ASM_SIZE 48

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
	bits_mod,
	bits_reg,
	bits_rm,
	bits_disp_lo,
	bits_disp_hi,
	bits_data,
	bits_data_if_w,
	bits_ip_inc_lo,
	bits_ip_inc_hi,

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
	operand_immediate,
	operand_ip_inc
};

struct instruction_encoding instruction_table[] = {

#define INST_TABLE
#include "instructions.inl"

};

struct instruction_operand {
	enum operand_type type;
	union {
		struct {
			u8 rm; /* is also correspond to reg union below if mod == 0b00 */
			i16 displacement;
		} effective_address;
		u8 reg;
		i16 immediate_data;
		i16 ip_inc;
	} value;
};

struct instruction {
	struct instruction_operand operands[2];
	enum opcode opcode;
	b8 wide_mode;
	u8 size;
};

struct asm_buffer {
	void *memory_block;
	
	char *texts;

	/* For printing control transfer assembly but seems */
	u16 *label_numbers;
	b8 *is_jumps;
	i16 *ip_incs;
	u8 *instruction_sizes;
	u32 label_count;
	
	u32 bytes_per_text;
	u32 instruction_count;
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

	for (i = 0; i < byte_count; ++i) {
		for (j = 0; j < 8; ++j) {
			output_byte[i * 9 + j] = '0' + (input_byte[i] >> (7 - j) & 1);
		}
		output_byte[i * 9 + j] = ' ';
	}
	
	output_byte[i * 9 - 1] = '\0';
}

void print_all_instructions(FILE *output, struct asm_buffer const asm_buf, u8 const *bytes, b32 const debug_mode) {
	char const *label = "\nlabel%u:\n";
	int i, at = 0;
	
	fprintf(output, "\tbits 16\n\n");
	
	for (i = 0; i < asm_buf.instruction_count; i += 1) {
		if (asm_buf.label_numbers[i]) {
			fprintf(output, label, asm_buf.label_numbers[i] - 1);
		}
		if (debug_mode) {
			char debug_text[8 * 6 + 7];
			debug_binary_string(debug_text, asm_buf.instruction_sizes[i], bytes + at);
			fprintf(output, "\n;;; %04x: %s\n", at, debug_text);
			at += asm_buf.instruction_sizes[i];
		}

		fprintf(output, asm_buf.texts + asm_buf.bytes_per_text * i);
		fprintf(output, "\n");
	}

	if (asm_buf.label_numbers[i]) {
		fprintf(output, label, asm_buf.label_numbers[i] - 1);
	}
}

err get_and_mark_label_number(struct asm_buffer *asm_buf, i32 const ip_inc, u32 const instruction_number, u32 *out_label_number) {
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

	if (displacement < 0) {
		fprintf(stderr, "ERROR: get_label's instruction pointer increment is not aligned\n");
		return 1;
	}

	if (asm_buf->label_numbers[label_line] == 0) {
		asm_buf->label_count += 1;
		asm_buf->label_numbers[label_line] = asm_buf->label_count;
	}

	*out_label_number = (u32)asm_buf->label_numbers[label_line];
	
	return 0;
}

err assign_all_labels(struct asm_buffer *asm_buf) {
	char temp[MAX_INSTRUCTION_ASM_SIZE];
	u32 i, label_number;
	err error;

	for (i = 0; i < asm_buf->instruction_count; i += 1) {
		if (asm_buf->is_jumps[i]) {
			error = get_and_mark_label_number(asm_buf, (i32)asm_buf->ip_incs[i], i, &label_number);
			if (error) return error;
			
			sprintf(temp, asm_buf->texts + asm_buf->bytes_per_text * i, label_number - 1);
			sprintf(asm_buf->texts + asm_buf->bytes_per_text * i, temp);
		}
	}

	return 0;
}

err asm_buffer_initialize(struct asm_buffer *result) {
	result->memory_block =
		calloc(sizeof *result->texts * MAX_INSTRUCTION_ASM_SIZE +
			   sizeof *result->label_numbers +
			   sizeof *result->is_jumps +
			   sizeof *result->ip_incs +
			   sizeof *result->instruction_sizes,
			   MAX_INSTRUCTIONS);
	
	if (result->memory_block == NULL) return 1;

	result->texts = result->memory_block;
	result->label_numbers     = (u16 *)(result->texts         + MAX_INSTRUCTIONS);
	result->is_jumps          = (b8  *)(result->label_numbers + MAX_INSTRUCTIONS);
	result->ip_incs           = (i16 *)(result->is_jumps      + MAX_INSTRUCTIONS);
	result->instruction_sizes = (u8  *)(result->ip_incs       + MAX_INSTRUCTIONS);
	
	result->label_count = 0;
	result->bytes_per_text = MAX_INSTRUCTION_ASM_SIZE;
	result->instruction_count = 0;
	
	return 0;
}

void asm_buffer_uninitialize(struct asm_buffer *asm_buf) {
	free(asm_buf->memory_block);
}

err format_instruction(struct instruction inst, struct asm_buffer *asm_buf) {
	char const *templates[3] = {"\t%s", "\t%s %s", "\t%s %s, %s"};
	char const *size_templates[3] = {"%s", "byte %s", "word %s"};
	char const *mnemonic = mnemonic_arr[inst.opcode];
	
	char temp_operand_asm[32];
	char operands_asm[2][32];
	u32 operand_count = 2;
	u32 i, mode, label_number;
	
	for (i = 0; i < 2; i += 1) {
		u32 size = 0;
		
		switch (inst.operands[i].type) {
		case operand_none:
			operand_count -= 1;
			break;
		case operand_register:
			sprintf(temp_operand_asm, reg_field_asm_text[inst.operands[i].value.reg][inst.wide_mode]);
			break;
		case operand_memory:
			mode = 0;
			if (inst.operands[i].value.effective_address.displacement > 0) {
				mode = 1;
			} else if (inst.operands[i].value.effective_address.displacement < 0) {
				inst.operands[i].value.effective_address.displacement *= -1;
				mode = 2;
			}
			if (inst.opcode != op_mov && inst.operands[1].type == operand_immediate) size = 1 + inst.wide_mode;
			sprintf(temp_operand_asm, mem_field_asm_text[inst.operands[i].value.effective_address.rm][mode],
					inst.operands[i].value.effective_address.displacement);
			break;
		case operand_direct_address:
			if (inst.opcode != op_mov && inst.operands[1].type == operand_immediate) size = 1 + inst.wide_mode;
			sprintf(temp_operand_asm, "[%u]", inst.operands[i].value.effective_address.displacement);
			break;
		case operand_immediate:
			if (inst.opcode == op_mov) size = 1 + inst.wide_mode;
			sprintf(temp_operand_asm, "%i", inst.operands[i].value.immediate_data);
			break;
		case operand_ip_inc:
			asm_buf->is_jumps[asm_buf->instruction_count] = TRUE;
			asm_buf->ip_incs[asm_buf->instruction_count] = inst.operands[i].value.ip_inc;
			sprintf(temp_operand_asm, "label%%u ; %i", inst.operands[i].value.ip_inc);
			break;
		default:
			fprintf(stderr, "ERROR: Unknown Operand Type\n");
			return 1;
		}
		
		sprintf(operands_asm[i], size_templates[size], temp_operand_asm);
	}

	sprintf(asm_buf->texts + asm_buf->instruction_count * asm_buf->bytes_per_text,
			templates[operand_count], mnemonic, operands_asm[0], operands_asm[1]);
	asm_buf->instruction_sizes[asm_buf->instruction_count] = inst.size;		

	return 0;
}

err decode(u8 const *memory, u32 const at, struct instruction *inst, b32 const debug_mode) {
	u8 const *bytes = &memory[at];
	u32 bytes_read;
	u32 bits_read;

	b32 has_reg, has_rm, has_data;
	b32 has_ip_inc_lo, has_ip_inc_hi;
	
	u32 reg, rm, mod;
	b32 w, s, d;
	
	u8 data_lo, data_hi;
	u8 disp_lo, disp_hi;
	u8 ip_inc_lo, ip_inc_hi;

	struct instruction_encoding *cur_enc;
	struct instruction_bits *cur_bits;

	/* Check if all bits_literal are matched */
	for (cur_enc = instruction_table; cur_enc < instruction_table + LEN(instruction_table); cur_enc += 1) {
		b32 valid = TRUE;
		has_reg = FALSE;
		has_rm = FALSE;
		has_data = FALSE;
		has_ip_inc_lo = FALSE;
		has_ip_inc_hi = FALSE;

		bytes_read = 0;
		bits_read = 0;
		
		for (cur_bits = &cur_enc->bits[0]; cur_bits->type != bits_end; cur_bits += 1) {
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
				disp_lo = 0;
				disp_hi = 0;
				if ((mod == mod_mem_no_disp && rm == 0x6) || mod == mod_mem_8_disp || mod == mod_mem_16_disp) disp_lo = bits_value;
				else continue;
				break;
			case bits_disp_hi:
				if ((mod == mod_mem_no_disp && rm == 0x6) || mod == mod_mem_16_disp) disp_hi = bits_value;
				else continue;
				break;
			case bits_data:
				data_lo = bits_value;
				data_hi = 0;
				has_data = TRUE;
				break;
			case bits_data_if_w:
				if (w && !s) data_hi = bits_value;
				else continue;
				break;
			case bits_ip_inc_lo:
				ip_inc_lo = bits_value;
				ip_inc_hi = 0;
				has_ip_inc_lo = TRUE;
				break;
			case bits_ip_inc_hi:
				ip_inc_hi = bits_value;
				has_ip_inc_hi = TRUE;
			default:
				fprintf(stderr, "ERROR: Unknown bits type\n");
				return 1;
			}
		
			bits_read += cur_bits->bit_count;
			bytes_read = bits_read / 8;
		}
next_encoding:
		if (valid) break;
	}
	
	if (cur_enc == instruction_table + LEN(instruction_table)) {
		fprintf(stderr, "ERROR: No encoding with valid bits_literal was found\n");
		return 1;
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

		b32 const control_transfer_mode = has_ip_inc_lo;
		b32 const wide_control_transfer_mode = has_ip_inc_hi;
	
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
				if (operand->value.effective_address.displacement >> 7) {
					operand->value.effective_address.displacement |= 0xff00;
				}
			}
		}

		if (immediate_mode) {
			struct instruction_operand *operand = &inst->operands[1];
			operand->type = operand_immediate;
			operand->value.immediate_data = (data_hi << 8) | data_lo;
			/* sign_extension */
			if (!wide_displacement_mode && sign_immediate_mode) {
				if (operand->value.immediate_data >> 7) {
					operand->value.immediate_data |= 0xff00;
				}
			}
		}

		if (control_transfer_mode) {
			struct instruction_operand *operand = &inst->operands[0]; /* maybe */
			operand->type = operand_ip_inc;
			operand->value.ip_inc = (ip_inc_hi << 8) | ip_inc_lo;
			/* sign_extension */
			if (!wide_control_transfer_mode) {
				if (operand->value.ip_inc >> 7) {
					operand->value.ip_inc |= 0xff00;
				}
			}
		}
	}

	return 0;
}

err disasm(u8 const *memory, i32 const total_bytes, FILE *output, b32 const debug_mode) {
	struct asm_buffer asm_buf;
	i32 bytes_left = total_bytes;
	err error;
	
	error = asm_buffer_initialize(&asm_buf);
	if (error) return error;
		
	while (bytes_left > 0) {
		u32 const bytes_at = total_bytes - bytes_left;
		struct instruction inst = {0};
		
		error = decode(memory, bytes_at, &inst, debug_mode);
		if (error) return error;
		
		error = format_instruction(inst, &asm_buf);
		if (error) return error;
		
		bytes_left -= inst.size;
		asm_buf.instruction_count += 1;
	}

	if (bytes_left < 0) {
		fprintf(stderr, "ERROR: Decoder overread\n");
		asm_buffer_uninitialize(&asm_buf);
		return 1;
	}

	error = assign_all_labels(&asm_buf);
	if (error) return error;
	
	print_all_instructions(output, asm_buf, memory, debug_mode);
	
	asm_buffer_uninitialize(&asm_buf);
	return 0;
}

err load_memory_from_file(void *memory, char *input_filename, u32 *bytes_read) {
	FILE *input_file = fopen(input_filename, "rb");
	if (input_file == NULL) return 1;
	
	*bytes_read = (u32)fread(memory, 1, 1024 * 1024, input_file);
	if (ferror(input_file)) return 1;
	
	fclose(input_file);
	return 0;
}

int main(int arg_count, char **args) {
	FILE *output = stdout;
	b32 debug_mode = FALSE;
	char *input_filename = NULL;
	char *output_filename = NULL;
	b32 output_to_file_mode = FALSE;
	
	u32 bytes_read, i;
	err error;
	
	void *memory = malloc(MAX_READ);	
	
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

	error = load_memory_from_file(memory, input_filename, &bytes_read);
	if (error) goto misuse;

	error = disasm((u8 *)memory, (u32)bytes_read, output, debug_mode);
	if (error) goto failed_exit;
	
	if (output != stdout || output != NULL) fclose(output);
	free(memory);
	return 0;
	
misuse:
	if (output != stdout || output != NULL) fclose(output);
	printf("usage:\n"
		   "\tdecode [options] filename\n"
		   "options:\n"
		   "\t-o <file>\n\t\twrite output to file\n"
		   "\t-d\n\t\tenable debug mode\n");
	free(memory);
	return 1;
failed_exit:
	free(memory);
	fprintf(stderr, "Exit Failed\n");
	return 1;
}
