#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef u8  b8;
typedef u32 b32;
#define TRUE 1
#define FALSE 0

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

struct instruction_property {
	enum instruction_bits_type type;
	u8 bit_count;
	union {
		u8 unshifted_mask;
		u8 forced_value;
	} u;
};

struct instruction_encoding {
	enum opcode opcode;
	struct instruction_property properties[16];
};

struct instruction_table {
	u32 encoding_count;
	struct instruction_encoding *encoding_arr;
};

struct x86_assembly_instruction_text {
	const char *format[3];
	u8 argument_count;

	const char *mnemonic;
	char destination[32];
	char source[32];
};

const char *_8086emu_mnemonic_arr[] = {
	"",

#define _8086emu_INST_MNE_STRING_LITERAL
#include "instructions.inl"

};

struct instruction_encoding _8086emu_instruction_table[] = {

#define _8086emu_INST_TABLE
#include "instructions.inl"

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

i32 debug_binary_string(char *output_byte, u32 byte_count, u8 *input_byte) {
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

	return EXIT_SUCCESS;
}

int main(int main_arg_count, char **main_arg_arr) {
	int main_arg_i;

	b32 debug_mode = FALSE;

	FILE *input_file, *output_file;
	const char *input_filename = NULL, *output_filename = NULL;

	size_t input_buffer_count, input_buffer_i, instruction_byte_count;
	u8 input_buffer_arr[1024];

	u32 instruction_encoding_i;
	struct instruction_table instruction_table = {
		sizeof _8086emu_instruction_table / sizeof *_8086emu_instruction_table,
		_8086emu_instruction_table,
	};

	for (main_arg_i = 1; main_arg_i < main_arg_count; ++main_arg_i) {
		static b32 is_expecting_output_filename;

		if (main_arg_arr[main_arg_i][0] == '-')
			switch (main_arg_arr[main_arg_i][1]) {
			case 'd':
				debug_mode = TRUE;
				break;
			case 'o':
				is_expecting_output_filename = TRUE;
				break;
			default:
				goto argument_failure_exit;
			} else if (is_expecting_output_filename) {
			output_filename = main_arg_arr[main_arg_i];
			is_expecting_output_filename = FALSE;
		} else if (input_filename == NULL) {
			input_filename = main_arg_arr[main_arg_i];
		}
		continue;

		goto argument_failure_exit;
	}
	
	input_file = fopen(input_filename, "rb");
	if (input_file == NULL)
		goto argument_failure_exit;

	if (output_filename == NULL) {
		output_file = stdout;
	} else {
		output_file = fopen(output_filename, "w");
		if (output_file == NULL) {
			fclose(input_file);
			goto argument_failure_exit;
		}
	}

	fprintf(output_file, "bits 16\n\n");

	input_buffer_count = fread(input_buffer_arr, 1, 1024, input_file);
	for (input_buffer_i = 0, instruction_byte_count = 0;
	     input_buffer_i < input_buffer_count;
	     input_buffer_i += instruction_byte_count) {
		u8 *cur_inst_read = &input_buffer_arr[input_buffer_i];
		u8 bit_read = 0;

		struct instruction_encoding *cur_encoding;
		struct instruction_property *cur_property;

		u8 cur_inst_property_arr[bits_count];
		b8 immediate_context = FALSE;

		struct x86_assembly_instruction_text asm_text =
			{{"%s\n", "%s %s\n", "%s %s, %s\n"}, 0, "", "", ""};

		char reg_asm_text[32], rm_asm_text[32], data_asm_text[32];

		memset(cur_inst_property_arr, 0xff, bits_count);

		/* OPCODE matching */
		for (instruction_encoding_i = 0;
		     instruction_encoding_i < instruction_table.encoding_count;
		     ++instruction_encoding_i) {
			u8 cur_opcode_bit_count;
			u8 cur_opcode_unshifted_mask;

			cur_encoding = &instruction_table.encoding_arr[instruction_encoding_i];
			cur_opcode_bit_count = cur_encoding->properties[0].bit_count;
			cur_opcode_unshifted_mask = cur_encoding->properties[0].u.unshifted_mask;

			if (cur_inst_read[0] >> (8 - cur_opcode_bit_count) ==
				cur_opcode_unshifted_mask)
				break;
		}

		if (instruction_encoding_i >= instruction_table.encoding_count) {
			fprintf(stderr, "ERROR: OPCODE not found.\n");
			goto exit_failure;
		}

		cur_property = &cur_encoding->properties[0];

		/* note: this design need to handle every special case like bits_data
		         consider rewriting this section to reduce redundancy         */

		/* properties assignment */
		while (cur_property->type != bits_end) {
			u8 bit_remain = 8 - bit_read % 8;
			u8 *cur_inst_byte = &cur_inst_read[instruction_byte_count];

			/* special case: dynamic data byte read */
			if (cur_property->type == bits_data) {
				immediate_context = TRUE;
				if (cur_inst_property_arr[bits_mod] != 0xff) {
					u8 backtrace = 0;
					switch (cur_inst_property_arr[bits_mod]) {
					case 0:
						if (cur_inst_property_arr[bits_rm] == 0x6) {
							backtrace = 0;
						} else {
							backtrace = 2;
						}
						break;
					case 1:
						backtrace = 1;
						break;
					case 2:
						backtrace = 0;
						break;
					case 3:
						backtrace = 2;
						break;
					}
					cur_inst_property_arr[bits_data] =
						*(cur_inst_byte - backtrace) >>
						(bit_remain - cur_property->bit_count) &
						cur_property->u.unshifted_mask;
					goto skip_normal_read;
				}
			}

			/* normal read */
			if (cur_property->bit_count > 0) {

				cur_inst_property_arr[cur_property->type] =
					*cur_inst_byte >> (bit_remain - cur_property->bit_count) &
					cur_property->u.unshifted_mask;
			} else {
				cur_inst_property_arr[cur_property->type] =
					cur_property->u.forced_value;
			}

skip_normal_read:
			bit_read += cur_property->bit_count;
			instruction_byte_count = bit_read / 8;

			++cur_property;
		}

		/* x86 assembly text processing */

		asm_text.mnemonic = _8086emu_mnemonic_arr[cur_encoding->opcode];

		if (cur_inst_property_arr[bits_reg] != 0xff) {
			strcpy(reg_asm_text,
			       reg_field_asm_text[cur_inst_property_arr[bits_reg]]
			       [cur_inst_property_arr[bits_w]]);

			switch (cur_inst_property_arr[bits_d]) {
			case 0:
				strcpy(asm_text.source, reg_asm_text);
				break;
			case 1:
				strcpy(asm_text.destination, reg_asm_text);
				break;
			}
		}

		if (cur_inst_property_arr[bits_rm] != 0xff)
		{
			u8 disp_lo = cur_inst_property_arr[bits_disp_lo];
			u8 disp_hi = cur_inst_property_arr[bits_disp_hi];
			u16 disp_16 = disp_lo + (disp_hi << 8);
			u8 sign_mode_8, sign_mode_16;

			if ((i8)disp_lo < 0) {
				sign_mode_8 = 2;
				disp_lo = -disp_lo;
			} else if (disp_lo == 0) {
				sign_mode_8  = 0;
			} else {
				sign_mode_8  = 1;
			}

			if ((i16)disp_16 < 0) {
				sign_mode_16 = 2;
				disp_16 = -disp_16;
			} else if (disp_16 == 0) {
				sign_mode_16 = 0;
			} else {
				sign_mode_16 = 1;
			}

			switch (cur_inst_property_arr[bits_mod]) {
			case mod_mem_no_disp:
				if (cur_inst_property_arr[bits_rm] != 0x6) {
					strcpy(rm_asm_text,
					       mem_field_asm_text[cur_inst_property_arr[bits_rm]][0]);
					bit_read -= 16;
				} else {
					sprintf(rm_asm_text, "[%u]", disp_16);
				}
				break;
			case mod_mem_8_disp:
				sprintf(rm_asm_text,
					mem_field_asm_text[cur_inst_property_arr[bits_rm]][sign_mode_8],
					disp_lo);
				bit_read -= 8;
				break;
			case mod_mem_16_disp:
				sprintf(rm_asm_text,
					mem_field_asm_text[cur_inst_property_arr[bits_rm]][sign_mode_16],
					disp_16);
				break;
			case mod_reg:
				strcpy(rm_asm_text, reg_field_asm_text[cur_inst_property_arr[bits_rm]]
				       [cur_inst_property_arr[bits_w]]);
				bit_read -= 16;
				break;
			default:
				fprintf(stderr, "ERROR: invalid bits_mod value for defined bits_rm route.\n");
				goto exit_failure;
			}

			switch (cur_inst_property_arr[bits_d]) {
			case 0:
				strcpy(asm_text.destination, rm_asm_text);
				break;
			case 1:
				strcpy(asm_text.source, rm_asm_text);
				break;
			}
		}

		/* we need to find a way to manage sign data fr */
		if (immediate_context) {
			const char *data_size_text;
			u8 data_lo, data_hi;
			u16 data_16;
			i32 data_output;

			data_lo = cur_inst_property_arr[bits_data];
			data_hi = cur_inst_property_arr[bits_data_if_w];
			data_16 = data_lo + (data_hi << 8);

			if (cur_inst_property_arr[bits_w] && cur_inst_property_arr[bits_s] != TRUE) {
				data_size_text = "word";
				data_output = data_16;
			} else {
				data_size_text = "byte";
				bit_read -= 8;

				if (cur_inst_property_arr[bits_s] == 1) {
					data_output = (i8)data_lo;
				} else {
					data_output = data_lo;
				}
			}


			sprintf(data_asm_text, "%s %i", data_size_text, data_output);
			strcpy(asm_text.source, data_asm_text);

			/* special case: immediate opcode is stated in the second byte */
			/* note: I attempted to use bit_literal as it should be the latest read
			         but the result is unexpected so I just hard code the exact bits */
			if (cur_inst_property_arr[bits_rm] != 0xff) {
				switch ((cur_inst_read[1] >> 3) & 0x7) {
				case 0x0:
					asm_text.mnemonic = _8086emu_mnemonic_arr[op_add];
					break;
				case 0x5:
					asm_text.mnemonic = _8086emu_mnemonic_arr[op_sub];
					break;
				case 0x7:
					asm_text.mnemonic = _8086emu_mnemonic_arr[op_cmp];
					break;
				}
			}
		}

		asm_text.argument_count =
			(asm_text.destination[0] != '\0') + (asm_text.source[0] != '\0');
			
		instruction_byte_count = bit_read / 8;

		if (debug_mode) {
			char byte_text[8 * 6 + 6];
			debug_binary_string(byte_text, instruction_byte_count, cur_inst_read);
			fprintf(output_file, "%08lx: %s| ", input_buffer_i, byte_text);
		}

		fprintf(output_file, asm_text.format[asm_text.argument_count],
			asm_text.mnemonic, asm_text.destination, asm_text.source);
	}

exit_success:
	fclose(input_file);
	fclose(output_file);
	if (debug_mode)
		fprintf(stderr, "[EXIT SUCCESS]\n");
	return EXIT_SUCCESS;
exit_failure:
	fclose(input_file);
	fclose(output_file);
	fprintf(stderr, "[EXIT FAILURE]\n");
	return EXIT_FAILURE;
argument_failure_exit:
	fprintf(stderr, "[ARGUMENT FAILURE EXIT]\n");
	return EXIT_FAILURE;
file_access_failure_exit:
	fprintf(stderr, "[FILE ACCESS FAILURE EXIT]\n");
	return EXIT_FAILURE;
}
