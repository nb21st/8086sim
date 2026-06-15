enum arithmetic_type {
	arithmetic_addition,
	arithmetic_subtraction,
	arithmetic_multiplication,
	arithmetic_division
};

enum register_flags {
	/* Control Flags */
	register_flags_trap,
	register_flags_direction,
	register_flags_interrupt_enable,
	/* Status Flags */
	register_flags_overflow,
	register_flags_sign,
	register_flags_zero,
	register_flags_auxiliary_carry,
	register_flags_parity,
	register_flags_carry,

	register_flags_count
};

char const *register_flags_labels[9] = {
	"TF", "DF", "IF",
	"OF", "SF", "ZF", "AF", "PF", "CF",
};

struct machine_state {
	union {
		u8 byte[2];
		u16 word;
	} data[4];
	u16 pi[4];
	u16 seg[4];
	u16 instruction_pointer;
	u16 flags;
};

char const *byte_register_labels[8] = {
	"al", "cl", "dl", "bl",
	"ah", "ch", "dh", "bh",
};

char const *word_register_labels[13] = {
	"ax", "cx", "dx", "bx",
	"sp", "bp", "si", "di",
	"es", "cs", "ss", "ds",
	"ip",
};
