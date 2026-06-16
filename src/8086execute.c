u32 read_by_width(void const *in_p, b32 const wide) {
	u32 result;
	u8 const *p = in_p;

	result = p[0];
	if (wide) result |= p[1] << 8;

	return result;
}

void write_by_width(void *in_p, u32 const value, b32 const wide) {
	u8 *p = in_p;

	p[0] = value & 0x00ff;
	if (wide) p[1] = (value & 0xff00) >> 8;
}

char const *get_assembly_reg(u8 reg) {
	char const *result;

	if (reg < 8) result = byte_register_labels[reg];
	else result = word_register_labels[reg - 8];

	return result;
}

void *get_machine_reg(u8 reg, struct machine_state *machine) {
	void *result = NULL;

	if (reg < 4) result = &machine->data[reg].byte.lo;
	else if (reg < 8) result =  &machine->data[reg - 4].byte.hi;
	else if (reg < 12) result = &machine->data[reg - 8].word;
	else if (reg < 16) result = &machine->pi[reg - 12];
	else if (reg < 20) result = &machine->seg[reg - 16];
	ASSERT(result != NULL, "Function 'find_machine_reg' shouldn't return NULL");

	return result;
}

u32 calculate_total_displacement(struct machine_state *state, u32 rm, u32 displacement) {
	u32 res = 0;

	if (rm == 8) return displacement;
	if (rm % 7 == 0 || rm == 1) res += read_by_width(get_machine_reg(8 + 3, state), TRUE);
	if (rm % 3 == 0 && rm != 7) res += read_by_width(get_machine_reg(12 + 1, state), TRUE);
	if (rm % 2 == 0 && rm != 6) res += read_by_width(get_machine_reg(12 + 2, state), TRUE);
	if (rm % 2 == 1 && rm != 7) res += read_by_width(get_machine_reg(12 + 3, state), TRUE);

	return res + displacement;
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
		half_result = 0;
		result = 0;
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
			ASSERT(FALSE, "Unhandled arithmetic operation");
			res = FALSE;
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
			if (flag_count == 0) fprintf(output, "+");
			fprintf(output, "%c", register_flags_labels[i][0]);
			flag_count += 1;
		}
	}

	if (flag_count == 0) fprintf(output, "-");
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
		case operand_memory:
			operands[i] = machine_state->memory + calculate_total_displacement(machine_state, inst.operands[i].value.effective_address.rm, inst.operands[i].value.effective_address.displacement);
			sprintf(texts[i], "[%lu]", (u8 *)operands[i] - machine_state->memory);
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
		fprintf(output, wide_mode ? "%s := 0x%04x; \n" : "%s := 0x%02x; \n", texts[0], rval);
	break;
	case op_add:
		fprintf(output, wide_mode ? "%s := 0x%04x; " : "%s := 0x%02x; ", texts[0], (u16)(lval + rval));
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, rval, arithmetic_addition, wide_mode);
		write_by_width(operands[0], lval + rval, wide_mode);
		fprintf(output, "\n");
	break;
	case op_inc:
		fprintf(output, wide_mode ? "%s := 0x%04x; " : "%s := 0x%02x; ", texts[0], (u16)(lval + 1));
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, 1, arithmetic_addition, wide_mode);
		write_by_width(operands[0], lval + 1, wide_mode);
		fprintf(output, "\n");
	break;
	case op_sub:
		fprintf(output, wide_mode ? "%s := 0x%04x; " : "%s := 0x%02x; ", texts[0], (u16)(lval - rval));
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, rval, arithmetic_subtraction, wide_mode);
		write_by_width(operands[0], lval - rval, wide_mode);
		fprintf(output, "\n");
	break;
	case op_dec:
		fprintf(output, wide_mode ? "%s := 0x%04x; " : "%s := 0x%02x; ", texts[0], (u16)(lval - 1));
		arithmetic_update_register_flags(output, &machine_state->flags,
										 lval, 1, arithmetic_subtraction, wide_mode);
		write_by_width(operands[0], lval - 1, wide_mode);
		fprintf(output, "\n");
	break;
	case op_cmp:
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
		void *cx = get_machine_reg((u8)cx_index, machine_state);
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
		case op_jcxz: res = read_by_width(cx, TRUE)  == 0; break;
		case op_loop:
			fprintf(output, "%s := 0x%04x; ", cx_asm, read_by_width(cx, TRUE) - 1U);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)read_by_width(cx, TRUE), (u32)read_by_width(cx, TRUE) - 1U,
											 arithmetic_subtraction, wide_mode);
			write_by_width(cx, read_by_width(cx, TRUE) - 1, TRUE);
			res = read_by_width(cx, TRUE) != 0;
			fprintf(output, "\n");
		break;
		case op_loopz:
			fprintf(output, "%s := 0x%04x; ", cx_asm, read_by_width(cx, TRUE) - 1U);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)read_by_width(cx, TRUE), (u32)read_by_width(cx, TRUE) - 1U,
											 arithmetic_subtraction, wide_mode);
			write_by_width(cx, read_by_width(cx, TRUE) - 1, TRUE);
			res = read_by_width(cx, TRUE) != 0 && (machine_state->flags >> register_flags_zero & 1);
			fprintf(output, "\n");
		break;
		case op_loopnz:
			fprintf(output, "%s := 0x%04x; ", cx_asm, read_by_width(cx, TRUE) - 1U);
			arithmetic_update_register_flags(output, &machine_state->flags, (u32)read_by_width(cx, TRUE), (u32)read_by_width(cx, TRUE) - 1U,
											 arithmetic_subtraction, wide_mode);
			write_by_width(cx, read_by_width(cx, TRUE) - 1, TRUE);
			res = read_by_width(cx, TRUE) != 0 && !(machine_state->flags >> register_flags_zero & 1);
			fprintf(output, "\n");
		break;
		default: ASSERT(FALSE, "Unhandled conditional transfer opcode"); break;
		}

		if (res) {
			machine_state->instruction_pointer += *(i32 *)operands[0];
			fprintf(output, "ip := 0x%04x; \n", machine_state->instruction_pointer);
		}
	break;
	} else {
		ASSERT(FALSE, "Unhandled opcode");
	} break;
	}

	if (debug_level >= 2) fprintf(output, "\n");
}

void print_machine_state(FILE *output, struct machine_state *state, u32 debug_level) {
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
			u16 const value = state->data[linear_print_order[i]].word;

			if (value == 0 && debug_level <= 1) continue;
			fprintf(output, "\t%s: 0x%04x (%u)\n", label, value, value);
		}
	} else {
		for (i = 0; i < 12; i += 3) {
			char const **labels;
			u16 const *values;
			u32 const *order;

			order = pretty_print_order + i;
			labels = word_register_labels;
			values = (u16 *)state->data;

			fprintf(output, "\t%s: 0x%04x | %s: 0x%04x | %s: 0x%04x\n",
					labels[order[0]], read_by_width(values + order[0], TRUE),
					labels[order[1]], read_by_width(values + order[1], TRUE),
					labels[order[2]], read_by_width(values + order[2], TRUE));
		}
	}

	for (i = 0; i < register_flags_count; i += 1) {
		if (state->flags >> i & 1) {
			if (flag_count == 0) fprintf(output, "\tflags: ");
			fprintf(output, "%c", register_flags_labels[i][0]);
			flag_count += 1;
		}
	}

	fprintf(output, "\n");
}
