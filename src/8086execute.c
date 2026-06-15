char const *get_assembly_reg(u8 reg) {
	char const *result;
	
	if (reg < 8) result = byte_register_labels[reg];
	else result = word_register_labels[reg - 8];

	return result;
}

void *get_machine_reg(u8 reg, struct machine_state *machine) {
	if (reg < 4) {
		return &machine->data[reg].byte[0];
	} else if (reg < 8) {
		return &machine->data[reg - 4].byte[1];
	} else if (reg < 12) {
		return &machine->data[reg - 8].word;
	} else if (reg < 16) {
		return &machine->pi[reg - 12];
	} else if (reg < 20) {
		return &machine->seg[reg - 16];
	}

	ASSERT(FALSE, "Function 'find_machine_reg' shouldn't return NULL");
	return NULL;
}

void execute_instruction(FILE *output,
						 struct instruction inst,
						 struct machine_state *machine_state,
						 u32 debug_level) {
	const b32 wide_instruction_mode = inst.flags >> flags_wide & 1;
	void *operands[2];
	char texts[2][16], temp_text[16];
	char const *string_index;
	u32 i, temp_values[2];

	for (i = 0; i < 2; i += 1) {
		switch (inst.operands[i].type) {
		case operand_register:
			operands[i] = get_machine_reg(inst.operands[i].value.reg, machine_state);
			string_index = get_assembly_reg(inst.operands[i].value.reg);
				sprintf(texts[i], "%s", string_index);
				
			if (i == 1 && debug_level >= 1) {
				if (wide_instruction_mode) sprintf(temp_text, " (0x%04x)", *(u16 *)operands[1]);
				else sprintf(temp_text, " (0x%02x)", *(u8 *)operands[1]);
				strcat(texts[1], temp_text);
			}
		break;
		case operand_immediate:
			temp_values[i] = inst.operands[i].value.data;
			operands[i] = &temp_values[i];
			sprintf(texts[i], wide_instruction_mode ? "0x%04x" : "0x%02x", (u16)temp_values[i]);
		break;
		default:
			ASSERT(FALSE, "Unhandled operand type");
		break;
		}
	}

	switch (inst.opcode) {
	case op_mov:
		if (wide_instruction_mode) *(u16 *)operands[0] = *(u16 *)operands[1];
		else *(u8 *)operands[0] = *(u8 *)operands[1];
		fprintf(output, "%s := %s;", texts[0], texts[1]);
	break;
	case op_add:
		if (wide_instruction_mode) {
		} else {
		}
	break;
	default:
		ASSERT(FALSE, "Unhandled opcode");
	break;
	}

	fprintf(output, "\n\n");
}

void print_machine_state(FILE *output, struct machine_state state, u32 debug_level) {
	
	/* LEARN: https://retrocomputing.stackexchange.com/q/5121 */
	u32 const pretty_print_order[12] = {
		0, 4, 8,
		3, 5, 9,
		1, 6, 10,
		2, 7, 11
	};
	u32 const linear_print_order[12] = {0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11};
	u32 i;

	fprintf(output, "Final registers:\n");

	if (debug_level >= 1) {
		for (i = 0; i < 12; i += 1) {
			char const *label = word_register_labels[linear_print_order[i]];
			u16 const value = state.data[linear_print_order[i]].word;

			fprintf(output, "\t%s: 0x%04x (%u)\n", label, value, value);
		}
	} else {
		for (i = 0; i < 12; i += 3) {
			char const **labels;
			u16 const *values;
			u32 const *order;
			
			order = pretty_print_order + i;
			labels = word_register_labels;
			values = (u16 *)state.data;
			
			fprintf(output, "\t%s: 0x%04x | %s: 0x%04x | %s: 0x%04x\n",
					labels[order[0]], values[order[0]],
					labels[order[1]], values[order[1]],
					labels[order[2]], values[order[2]]);
		}
	}
}
