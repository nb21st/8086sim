char const *help =
"Usage: 8086sim decode [options...] file\n"
"       8086sim exec   [options...] file\n"
"Options:\n"
"  -o <file>     Write output to file.\n"
"  -d<0-3>       Specify debug printing level.\n"
"Execute options:\n"
"  --dump <file> Write final memory to file.\n";

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

		for (cur_bits = cur_enc->bits; cur_bits->type != bits_end; cur_bits += 1) {
			u8 const bits_remain = 8 - bits_read % 8;
			u8 bits_value;

			if (cur_bits->bit_count == 0) bits_value = cur_bits->value;
			else bits_value = (bytes[bytes_read] >> (bits_remain - cur_bits->bit_count)) & MASK(cur_bits->bit_count);

			/* NOTE: I think this might be a bad design but I just couldn't come up with a good alternative */
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
					bits[bits_mod] == mod_mem_16_disp || bits[bits_force_disp] == 1) {
					bits[bits_disp_lo] = bits_value;
				} else continue;
			break;
			case bits_disp_hi:
				if ((bits[bits_mod] == mod_mem_no_disp && bits[bits_rm] == 6) || bits[bits_mod] == mod_mem_16_disp ||
					bits[bits_force_disp] == 1) {
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
		*flags |= 1 << flags_lock;
		return 0;
	case op_rep:
		*flags |= 1 << flags_rep;
		return 0;
	case op_segment:
		*flags |= 1 << flags_segment;
		*flags |= bits[bits_seg_reg] << flags_segment_register;
		return 0;
	default:
		inst->flags = *flags;
		*flags = 0;
	break;
	}

	{
		b32 const wide_instruction_mode = bits[bits_w] == 1;
		b32 const direction = bits[bits_d];

		b32 const v_exist = bits[bits_v] != -1;
		/* b32 const z_exist = bits[bits_z] != -1; */

		b32 const reg_exist = bits[bits_reg] != -1;
		b32 const wide_reg_mode = wide_instruction_mode;
		b32 const reg_is_src = !direction;

		b32 const seg_reg_exist = bits[bits_seg_reg] != -1;

		b32 const rm_exist = bits[bits_rm] != -1;
		b32 const rm_reg_mode = bits[bits_mod] == mod_reg;
		b32 const wide_rm_mode = wide_reg_mode || (bits[bits_rm_is_w] == 1);
		b32 const direct_address_mode = bits[bits_mod] == mod_mem_no_disp && bits[bits_rm] == 6;
		b32 const disp_mode = bits[bits_mod] == mod_mem_8_disp || bits[bits_mod] == mod_mem_16_disp;
		b32 const wide_disp_mode = bits[bits_mod] == mod_mem_16_disp;
		b32 const rm_is_src = direction;

		b32 const immediate_mode = bits[bits_data] != -1;
		b32 const sign_immediate_mode = bits[bits_s];

		b32 const relative_jump_mode = bits[bits_is_rel_jmp] == 1;
		b32 const far_mode = bits[bits_is_far] == 1;

		inst->flags |= wide_instruction_mode << flags_wide;
		inst->flags |= far_mode << flags_far;

		inst->opcode = cur_enc->opcode;

		if (reg_exist) {
			struct instruction_operand *operand = &inst->operands[reg_is_src];

			operand->type = operand_register;
			operand->value.reg = bits[bits_reg] + 8 * wide_reg_mode;
		} else if (seg_reg_exist) {
			struct instruction_operand *operand = &inst->operands[reg_is_src];

			operand->type = operand_register;
			operand->value.reg = bits[bits_seg_reg] + 8 * 2;
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
			} else {
				operand->type = operand_memory;
				if (direct_address_mode) operand->value.effective_address.rm = 8;
				else operand->value.effective_address.rm = bits[bits_rm];
			}

			operand->value.effective_address.displacement = (bits[bits_disp_hi] << 8) | bits[bits_disp_lo];

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
			struct instruction_operand *operand =
				&inst->operands[!((reg_exist && reg_is_src) || (rm_exist && rm_is_src)) &&
								(reg_exist || rm_exist)];

			if (relative_jump_mode) operand->type = operand_relative_immediate;
			else operand->type = operand_immediate;
			operand->value.data = (bits[bits_data_if_w] << 8) | bits[bits_data];
			/* sign-extension */
			if (sign_immediate_mode && (operand->value.data & 0x80) != 0) {
				operand->value.data |= 0xffffff00;
			}
		}

		if (far_mode && immediate_mode) {
			struct instruction_operand *operands = inst->operands;
			u32 i = 0;

			if (bits[bits_disp_lo] != -1) {
				operands[i].type = operand_immediate;
				operands[i].value.data = (bits[bits_disp_hi] << 8) | bits[bits_disp_lo];
				i += 1;
			}

			operands[i].type = operand_immediate;
			operands[i].value.data = (bits[bits_data_if_w] << 8) | bits[bits_data];
		}
	}

	return 0;
}

err simulate_8086(u8 *memory, i32 const total_bytes, FILE *output, enum sim_mode sim_mode, u32 debug_level) {
	struct asm_buffer asm_buf;
	struct machine_state machine_state;
	i32 bytes_left = total_bytes;
	u8 inst_flags;
	err error;

	switch (sim_mode) {
	case sim_mode_decode:
		error = asm_buffer_initialize(&asm_buf);
		if (error) {
			fprintf(stderr, "ERROR: asm_buffer_initialize failed.\n");
			return 1;
		}
	break;
	case sim_mode_exec:
		memset(&machine_state, 0, sizeof machine_state);
		machine_state.memory = memory;
	break;
	}

	while (bytes_left > 0) {
		u32 const bytes_at = total_bytes - bytes_left;
		struct instruction inst = {0};

		error = decode(memory, bytes_at, &inst, &inst_flags, debug_level);
		if (error) return 1;

		switch (sim_mode) {
		case sim_mode_decode:
			format_instruction(NULL, inst, &asm_buf, bytes_at, debug_level);
			if (inst.opcode != op_none) asm_buf.instruction_count += 1;
			bytes_left -= inst.size;
		break;
		case sim_mode_exec:
			if (debug_level >= 2) {
				fprintf(output, "[0x%04x -> 0x%04x]\n",
						machine_state.instruction_pointer,
						machine_state.instruction_pointer + inst.size);
			}

			if (debug_level >= 1) {
				fprintf(output, "@ ");
				format_instruction(output, inst, NULL, bytes_at, debug_level);
			}

			execute_instruction(output, inst, &machine_state, debug_level);
			bytes_left = total_bytes - machine_state.instruction_pointer;
		break;
		}
	}

	if (bytes_left < 0) {
		fprintf(stderr, "ERROR: Decoder overread.\n");
		return 1;
	}

	switch (sim_mode) {
	case sim_mode_decode:
		assign_all_labels(&asm_buf);
		print_all_instructions(output, asm_buf, memory, debug_level);
		asm_buffer_uninitialize(&asm_buf);
	break;
	case sim_mode_exec:
		print_machine_state(output, &machine_state, debug_level);
	break;
	}

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
	enum sim_mode mode;
	u32 debug_level = 0;

	FILE *output = stdout;
	FILE *dump_output = NULL;
	char *input_filename = NULL;
	char *output_filename = NULL;
	char *dump_output_filename = NULL;

	b32 output_to_file_mode = FALSE;
	b32 dump_to_memory_mode = FALSE;

	u32 bytes_read, i;
	err error;

	void *memory = malloc(MEMORY_SIZE);

	if (arg_count < 2) goto misuse;

	if (strcmp(args[1], "decode") == 0) mode = sim_mode_decode;
	else if (strcmp(args[1], "exec") == 0) mode = sim_mode_exec;
	else goto misuse;

	for (i = 2; i < arg_count; i += 1) {
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
			case '-':
				if (strcmp(args[i] + 2, "dump") == 0) {
					dump_to_memory_mode = TRUE;
				} else goto misuse;
			break;
			default: goto misuse;
			}
		} else if (output_to_file_mode && output_filename == NULL) output_filename = args[i];
		else if (dump_to_memory_mode && dump_output_filename == NULL) dump_output_filename = args[i];
		else if (input_filename == NULL) input_filename = args[i];
		else goto misuse;
	}

	if (input_filename == NULL) goto misuse;
	if (output_filename != NULL) {
		output = fopen(output_filename, "wb");
		if (output == NULL) {
			fprintf(stderr, "ERROR: Unable to open output file\n");
			goto failed_exit;
		}
	}
	if (dump_to_memory_mode) {
		if (dump_output_filename == NULL || mode != sim_mode_exec) goto misuse;
		dump_output = fopen(dump_output_filename, "wb");
		if (dump_output == NULL) {
			fprintf(stderr, "ERROR: Unable to open dump output file\n");
			goto failed_exit;
		}
	}

	error = load_memory_from_file(memory, input_filename, &bytes_read);
	if (error) goto misuse;

	error = simulate_8086((u8 *)memory, bytes_read, output, mode, debug_level);
	if (error) goto failed_exit;

	if (dump_to_memory_mode) fwrite(memory, 1, MEMORY_SIZE, dump_output);

	if (output_filename != NULL) fclose(output);
	free(memory);
	return 0;
misuse:
	if (output_filename != NULL) {
		fclose(output);
		remove(output_filename);
	}
	free(memory);
	fprintf(stderr, help);
	return 1;
failed_exit:
	if (output_filename != NULL && output != NULL) {
		fclose(output);
		remove(output_filename);
	}
	free(memory);
	fprintf(stderr, "Exit Failed\n");
	return 1;
}
