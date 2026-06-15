void print_all_instructions(FILE *output,
							struct asm_buffer const asm_buf,
							u8 const *bytes,
							u32 const debug_level) {
	char const *label = "\nlabel_%u:\n";
	u32 at = 0;
	u32 i;
	
	fprintf(output, "CPU 8086\n\n");
	
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
		displacement -= asm_buf->instruction_sizes[label_line - (direction == -1)];
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
	return opcode >= op_shl && opcode <= op_rcr;
}
b32 is_string_instruction(enum opcode opcode) {
	return opcode >= op_movs && opcode <= op_stos;
}
b32 is_conditional_transfer_instruction(enum opcode opcode) {
	return opcode >= op_je && opcode <= op_jcxz;
}

void format_instruction(FILE *output,
						struct instruction inst,
						struct asm_buffer *asm_buf,
						u32 bytes_at,
						u32 const debug_level) {
	char *target_asm;
	char temp_asm[MAX_INSTRUCTION_ASM_SIZE];
	char temp_operand_asm[MAX_OPERAND_ASM_SIZE];
	u32 operand_count = 2;
	u32 i, ea_mode;

	if (inst.opcode == op_none) return;

	if (asm_buf == NULL) target_asm = temp_asm;
	else {
		target_asm = asm_buf->texts + asm_buf->bytes_per_text * asm_buf->instruction_count;
		target_asm[0] = '\t';
	}

	if (inst.flags >> flags_lock & 1) strcat(target_asm, "lock ");
	if (inst.flags >> flags_rep & 1) strcat(target_asm, "rep ");

	strcat(target_asm, mnemonic_arr[inst.opcode]);

	if (is_string_instruction(inst.opcode)) {
		strcat(target_asm, inst.flags >> flags_wide & 1 ? "w" : "b");
	}

	for (i = 0; i < 2; i += 1) {
		if (inst.operands[i].type != operand_none) {
			if (i == 0) strcat(target_asm, " ");
			else if ((inst.flags >> flags_far & 1) && inst.operands[i].type == operand_immediate) {
				strcat(target_asm, ":");
			} else strcat(target_asm, ", ");
		}
		
		switch (inst.operands[i].type) {
		case operand_none:
			operand_count -= 1;
			continue;
		case operand_register:
			sprintf(temp_operand_asm,
					reg_field_asm_text[inst.operands[i].value.reg / 8][inst.operands[i].value.reg % 8]);
		break;
		case operand_memory:
			ea_mode = 0;
			if (inst.operands[i].value.effective_address.displacement > 0) {
				ea_mode = 1;
			} else if (inst.operands[i].value.effective_address.displacement < 0) {
				inst.operands[i].value.effective_address.displacement *= -1;
				ea_mode = 2;
			}

			if (inst.opcode == op_call || inst.opcode == op_jmp) {
				if (inst.flags >> flags_far & 1) strcat(target_asm, "far ");
			} else if (inst.opcode != op_mov && (inst.operands[1].type == operand_immediate ||
												 inst.operands[1].type == operand_none ||
												 is_shift_instruction(inst.opcode))) {
				if (inst.flags >> flags_wide & 1) strcat(target_asm, "word ");
				else strcat(target_asm, "byte ");
			}

			if (inst.flags >> flags_segment & 1) {
				strcat(target_asm, reg_field_asm_text[2][inst.flags >> flags_segment_register & 3]);
				strcat(target_asm, ":");
			}

			sprintf(temp_operand_asm,
					mem_field_asm_text[inst.operands[i].value.effective_address.rm][ea_mode],
					inst.operands[i].value.effective_address.displacement);
		break;
		case operand_immediate:
			if (inst.opcode == op_mov) {
				if (inst.flags >> flags_wide & 1) strcat(target_asm, "word ");
				else strcat(target_asm, "byte ");
			}
			
			sprintf(temp_operand_asm, "%i", inst.operands[i].value.data);
		break;
		case operand_relative_immediate:
			if (asm_buf == NULL) {
				sprintf(temp_operand_asm, "$%+i", inst.operands[i].value.data);
				break;
			}
			
			asm_buf->ip_incs[asm_buf->instruction_count] = inst.operands[i].value.data;
			if ((asm_buf->is_cond_jumps[asm_buf->instruction_count] =
				 is_conditional_transfer_instruction(inst.opcode))) {
				sprintf(temp_operand_asm, "label_%%u ; %+i", inst.operands[i].value.data);
			} else {
				if (inst.opcode == op_jmp && !(inst.flags >> flags_wide & 1)) strcat(target_asm, "short "); /* nasm aligned */
				
				sprintf(temp_operand_asm, "%i", inst.operands[i].value.data + bytes_at + inst.size);
			}
		break;
		}
		
		strcat(target_asm, temp_operand_asm);
	}
	
	if (asm_buf == NULL) {
		fprintf(output, target_asm);
		fprintf(output, "\n");
	} else {
		asm_buf->instruction_sizes[asm_buf->instruction_count] = inst.size;
		if (debug_level >= 2) {
			fprintf(stderr, target_asm);
			fprintf(stderr, "\n");
			if (debug_level >= 3) fprintf(stderr, "\n");
		}
	}
}
