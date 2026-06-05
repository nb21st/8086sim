#ifdef _8086emu_INST_TABLE
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) \
	{op_##mne_in, {in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12}},
#define ALT_INST INST
#endif

#ifdef _8086emu_INST_MNE_ENUM
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) op_##mne_in,
#define ALT_INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12)
#endif

#ifdef _8086emu_INST_MNE_STRING_LITERAL
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) #mne_in,
#define ALT_INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12)
#endif

#define BIN(hex_in, bin_in) {bits_literal, sizeof #bin_in - 1, 0x##hex_in}
#define S         {bits_s,         1}
#define W         {bits_w,         1}
#define D         {bits_d,         1}
#define MOD       {bits_mod,       2}
#define REG       {bits_reg,       3}
#define RM        {bits_rm,        3}
#define DISP_LO   {bits_disp_lo,   8}
#define DISP_HI   {bits_disp_hi,   8}
#define DATA      {bits_data,      8}
#define DATA_IF_W {bits_data_if_w, 8}
#define ADDR_LO   {bits_disp_lo,   8}
#define ADDR_HI   {bits_disp_hi,   8}
#define IP_INC8   {bits_ip_inc_lo, 8}
#define IP_INC_LO {bits_ip_inc_lo, 8}
#define IP_INC_HI {bits_ip+inc_hi, 8}

#define IMP_S(val)   {bits_s,   0, val}
#define IMP_W(val)   {bits_w,   0, val}
#define IMP_D(val)   {bits_d,   0, val}
#define IMP_MOD(val) {bits_mod, 0, val}
#define IMP_REG(val) {bits_reg, 0, val}
#define IMP_RM(val)  {bits_rm,  0, val}

#define _ {0}

    INST(mov, BIN(22, 100010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(mov, BIN(63, 1100011 ), IMP_S(0), IMP_D(0), W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)
ALT_INST(mov, BIN(0b, 1011    ), IMP_S(0), IMP_D(1), W, REG, DATA, DATA_IF_W, _, _, _, _)
ALT_INST(mov, BIN(50, 1010000 ), IMP_MOD(0), IMP_REG(0), IMP_RM(6), W, ADDR_LO, ADDR_HI, IMP_D(1), _, _, _)
ALT_INST(mov, BIN(51, 1010001 ), IMP_MOD(0), IMP_REG(0), IMP_RM(6), W, ADDR_LO, ADDR_HI, IMP_D(0), _, _, _)

    INST(add, BIN(00, 000000  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(add, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)
ALT_INST(add, BIN(02, 0000010 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)

    INST(sub, BIN(0a, 001010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(sub, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(05, 101), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)
ALT_INST(sub, BIN(16, 0010110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)

    INST(cmp, BIN(0e, 001110  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(cmp, BIN(20, 100000  ), S, W, MOD, BIN(07, 111), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0))
ALT_INST(cmp, BIN(1e, 0011110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)

	INST(je,     BIN(74, 01110100), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jl,     BIN(7c, 01111100), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jle,    BIN(7e, 01111110), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jb,     BIN(72, 01110010), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jbe,    BIN(76, 01110110), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jp,     BIN(7a, 01111010), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jo,     BIN(70, 01110000), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(js,     BIN(78, 01111000), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jne,    BIN(75, 01110101), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jnl,    BIN(7d, 01111101), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jg,     BIN(7f, 01111111), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jnb,    BIN(73, 01110011), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(ja,     BIN(77, 01110111), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jnp,    BIN(7b, 01111011), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jno,    BIN(71, 01110001), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jns,    BIN(79, 01111001), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(loop,   BIN(e2, 11100010), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(loopz,  BIN(e1, 11100001), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(loopnz, BIN(e0, 11100000), IP_INC8, _, _, _, _, _, _, _, _, _)
	INST(jcxz,   BIN(e3, 11100011), IP_INC8, _, _, _, _, _, _, _, _, _)
	

#undef _8086emu_INST_ARR
#undef _8086emu_INST_MNE_ENUM
#undef _8086emu_INST_MNE_STRING_LITERAL

#undef INST
#undef ALT_INST

#undef BIN
#undef S
#undef W
#undef D
#undef MOD
#undef REG
#undef RM
#undef DISP_LO
#undef DISP_HI
#undef DATA
#undef DATA_IF_W
#undef ADDR_LO
#undef ADDR_HI
#undef IP_INC8
#undef IP_INC_LO
#undef IP_INC_HI

#undef IMP_S
#undef IMP_W
#undef IMP_D
#undef IMP_MOD
#undef IMP_REG
#undef IMP_RM
	
#undef _
