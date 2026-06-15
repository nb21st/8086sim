char const *get_assembly_reg(u8 reg) {
	char const *result;

	if (reg < 8) result = byte_register_labels[reg];
	else result = word_register_labels[reg - 8];

	return result;
}

void *get_machine_reg(u8 reg, struct machine_state *machine) {
	if (reg < 4) return &machine->data[reg].byte[0];
	else if (reg < 8) return &machine->data[reg - 4].byte[1];
	else if (reg < 12) return &machine->data[reg - 8].word;
	else if (reg < 16) return &machine->pi[reg - 12];
	else if (reg < 20) return &machine->seg[reg - 16];

	ASSERT(FALSE, "Function 'find_machine_reg' shouldn't return NULL");
	return NULL;
}

void arithmetic_update_register_flags(FILE *output, u16 *flags, u32 value0, u32 value1,
						   enum arithmetic_type type, b32 wide_mode) {
	u32 const s_val = wide_mode ? 0x8000 : 0x80;
	u32 const max_val = wide_mode ? 0xffff : 0xff;

	b32 is_signed[3];
	b32 cases[9] = {FALSE};
	u32 flag_count = 0;
	u32 result, half_result;
	u32 i;

	switch (type) {
	case arithmetic_addition:
		half_result = (value0 & 0xf) + (value1 & 0xf);
		result = value0 + value1;
	break;
	case arithmetic_subtraction:
		half_result = (value0 & 0xf) - (value1 & 0xf);
		result = value0 - value1;
	break;
	default:
		ASSERT(FALSE, "Unhandled arithmetic operation");
	break;
	}

	is_signed[0] = (value0 & s_val) != 0;
	is_signed[1] = (value1 & s_val) != 0;
	is_signed[2] = (result & s_val) != 0;

	{
		b32 res;
		switch (type) {
		case arithmetic_addition:
			res = is_signed[0] == is_signed[1] && is_signed[1] != is_signed[2];
		break;
		case arithmetic_subtraction:
			res = is_signed[0] != is_signed[1] && is_signed[1] == is_signed[2];
		break;
		default:
			ASSERT(FALSE, "");
		break;
		}

		cases[register_flags_overflow] = res;
	}
	cases[register_flags_auxiliary_carry] = half_result > 0xf;
	cases[register_flags_carry] = result > max_val;
	cases[register_flags_sign] = is_signed[2];
	cases[register_flags_zero] = result % 0x10000 == 0;
	{
		u32 one_bit_count = 0;
		for (i = 0; i < 8; i += 1) one_bit_count += result >> i & 1;
		cases[register_flags_parity] = one_bit_count % 2 == 0;
	}

	*flags = 0;

	for (i = 0; i < register_flags_count; i += 1) {
		if (cases[i]) {
			*flags |= 1 << i;
			if (flag_count > 0) fprintf(output, ",");
			fprintf(output, " +%s", register_flags_labels[i]);
			flag_count += 1;
		}
	}

	if (flag_count == 0) fprintf(output, " -");
}

u32 read_by_width(void *p, b32 wide) {
	return (u32)(wide ? *(u16 *)p : *(u8 *)p);
}

void write_by_width(void *p, u32 value, b32 wide) {
	if (wide) *(u16 *)p = (u16)value;
	else *(u8 *)p = (u8)value;
}

void execute_instruction(FILE *output,
						 struct instruction inst,
						 struct machine_state *machine_state,
						 u32 debug_level) {
	const b32 wide_mode = inst.flags >> flags_wide & 1;
	void *operands[2];
	char texts[2][16];
	char const *string_index;
	u32 i, lval, rval;

	machine_state->instruction_pointer += inst.size;

	for (i = 0; i < 2; i += 1) {
		switch (inst.operands[i].type) {
		case operand_none: operands[i] = &operands[i]; break;
		case operand_register:
			operands[i] = get_machine_reg(inst.operands[i].value.reg, machine_state);
			string_index = get_assembly_reg(inst.operands[i].value.reg);
			sprintf(texts[i], "%s", string_index);
		break;
		case operand_immediate:
			operands[i] = &inst.operands[i].value.data;
			sprintf(texts[i], wide_mode ? "0x%04x" : "0x%02x", (u16)inst.operands[i].value.data);
		break;
		case operand_relative_immediate:
			operands[i] = &inst.operands[i].value.data;
		break;
		default:
			LOG_VAR(inst.operands[i].type, u);
			ASSERT(FALSE, "Unhandled operand type");
		break;
		}
	}

	lval = read_by_width(operands[0], wide_mode);
	rval = read_by_width(operands[1], wide_mode);

	switch (inst.opcode) {
	case op_mov:
		write_by_width(operands[0], rval, wide_mode);
		fprintf(output, "%s := %s;\n", texts[0], texts[1]);
	break;
	case op_add:
		fprintf(output, "%s := %s + %s;", texts[0], texts[0], texts[1]);
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, rval, arithmetic_addition, wide_mode);
		write_by_width(operands[0], lval + rval, wide_mode);
		fprintf(output, "\n");
	break;
	case op_sub:
		fprintf(output, "%s := %s - %s;", texts[0], texts[0], texts[1]);
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, rval, arithmetic_subtraction, wide_mode);
		write_by_width(operands[0], lval - rval, wide_mode);
		fprintf(output, "\n");
	break;
	case op_cmp:
		fprintf(output, "cmp(%s, %s);", texts[0], texts[1]);
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, rval, arithmetic_subtraction, wide_mode);
		fprintf(output, "\n");
	break;
	default: if (is_conditional_transfer_instruction(inst.opcode)) {
		b32 const OF = machine_state->flags >> register_flags_overflow & 1;
		b32 const SF = machine_state->flags >> register_flags_sign & 1;
		b32 const ZF = machine_state->flags >> register_flags_zero & 1;
		b32 const PF = machine_state->flags >> register_flags_parity & 1;
		b32 const CF = machine_state->flags >> register_flags_carry & 1;
		u32 const cx_index = 9;
		char const *cx_asm = get_assembly_reg((u8)cx_index);
		u16 *cx = get_machine_reg((u8)cx_index, machine_state);
		b32 res;

		switch(inst.opcode) {
		case op_ja  : res = !(CF || ZF);         break;
		case op_jnb : res = CF == 0;             break;
		case op_jb  : res = CF == 1;             break;
		case op_jbe : res = CF || ZF;            break;
		case op_je  : res = ZF;                  break;
		case op_jg  : res = !((SF != OF) || ZF); break;
		case op_jnl : res = !(SF != OF);         break;
		case op_jl  : res = SF || OF;            break;
		case op_jle : res = (SF != OF) || ZF;    break;
		case op_jne : res = !ZF;                 break;
		case op_jno : res = !OF;                 break;
		case op_jnp : res = !PF;                 break;
		case op_jns : res = !SF;                 break;
		case op_jo  : res = OF;                  break;
		case op_jp  : res = PF;                  break;
		case op_js  : res = SF;                  break;
		case op_jcxz: res = *cx == 0;            break;
		case op_loop:
			fprintf(output, "%s := %s - 1;", cx_asm, cx_asm);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)*cx, (u32)*cx - 1U,
											 arithmetic_subtraction, wide_mode);
			*cx -= 1;
			res = *cx != 0;
			fprintf(output, "\n");
		break;
		case op_loopz:
			fprintf(output, "%s := %s - 1;", cx_asm, cx_asm);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)*cx, (u32)*cx - 1U,
											 arithmetic_subtraction, wide_mode);
			*cx -= 1;
			res = *cx != 0 && (machine_state->flags >> register_flags_zero & 1);
			fprintf(output, "\n");
		break;
		case op_loopnz:
			fprintf(output, "%s := %s - 1;", cx_asm, cx_asm);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)*cx, (u32)*cx - 1U,
											 arithmetic_subtraction, wide_mode);
			*cx -= 1;
			res = *cx != 0 && !(machine_state->flags >> register_flags_zero & 1);
			fprintf(output, "\n");
		break;
		default: ASSERT(FALSE, "Unhandled conditional transfer opcode"); break;
		}

		if (res) {
			machine_state->instruction_pointer += *(i32 *)operands[0];
			fprintf(output, "ip := 0x%04x\n", machine_state->instruction_pointer);
		}
	break;
	} else {
		ASSERT(FALSE, "Unhandled opcode");
	} break;
	}

	if (debug_level >= 1) fprintf(output, "\n");
}

void print_machine_state(FILE *output, struct machine_state state, u32 debug_level) {
	/* LEARN: https://retrocomputing.stackexchange.com/q/5121 */
	u32 const pretty_print_order[12] = {
		0, 4, 8,
		3, 5, 9,
		1, 6, 10,
		2, 7, 11
	};
	u32 const linear_print_order[13] = {0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12};
	u32 flag_count = 0;
	u32 i;

	fprintf(output, "Final registers:\n");

	if (debug_level >= 1) {
		for (i = 0; i < 13; i += 1) {
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

	for (i = 0; i < register_flags_count; i += 1) {
		if (state.flags >> i & 1) {
			if (flag_count == 0) fprintf(output, "\tflags: %s", register_flags_labels[i]);
			else fprintf(output, ", %s", register_flags_labels[i]);
			flag_count += 1;
		}
	}

	fprintf(output, "\n");
}
