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

char const *word_register_labels[12] = {
	"ax", "cx", "dx", "bx",
	"sp", "bp", "si", "di",
	"es", "cs", "ss", "ds",
};
