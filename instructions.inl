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

#define BIN(hex_in, bin_in) {bits_literal, sizeof #bin_in - 1, {0x##hex_in}}
#define S         {bits_s,         1, {0x01}}
#define W         {bits_w,         1, {0x01}}
#define D         {bits_d,         1, {0x01}}
#define MOD       {bits_mod,       2, {0x03}}
#define REG       {bits_reg,       3, {0x07}}
#define RM        {bits_rm,        3, {0x07}}
#define DISP_LO   {bits_disp_lo,   8, {0xff}}
#define DISP_HI   {bits_disp_hi,   8, {0xff}}
#define DATA      {bits_data,      8, {0xff}}
#define DATA_IF_W {bits_data_if_w, 8, {0xff}}
#define ADDR_LO   {bits_disp_lo,   8, {0xff}}
#define ADDR_HI   {bits_disp_hi,   8, {0xff}}

#define IMP_S(val)   {bits_s,   0, {val}}
#define IMP_W(val)   {bits_w,   0, {val}}
#define IMP_D(val)   {bits_d,   0, {val}}
#define IMP_MOD(val) {bits_mod, 0, {val}}
#define IMP_REG(val) {bits_reg, 0, {val}}
#define IMP_RM(val)  {bits_rm,  0, {val}}

#define _ {0}

    INST(mov, BIN(22, 100010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(mov, BIN(63, 1100011 ), W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0), _)
ALT_INST(mov, BIN(0b, 1011    ), W, REG, DATA, DATA_IF_W, IMP_D(1), _, _, _, _, _)
ALT_INST(mov, BIN(50, 1010000 ), W, ADDR_LO, ADDR_HI, IMP_D(1), IMP_MOD(0x0), IMP_REG(0x0), IMP_RM(0x6), _, _, _)
ALT_INST(mov, BIN(51, 1010001 ), W, ADDR_LO, ADDR_HI, IMP_D(0), IMP_MOD(0x0), IMP_REG(0x0), IMP_RM(0x6), _, _, _)

    INST(add, BIN(00, 000000  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(add, BIN(20, 100000  ), S, W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0))
ALT_INST(add, BIN(02, 0000010 ), W, DATA, DATA_IF_W, IMP_D(1), IMP_REG(0x0), _, _, _, _, _)

    INST(sub, BIN(0a, 001010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(sub, BIN(20, 100000  ), S, W, MOD, BIN(05, 101), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0))
ALT_INST(sub, BIN(16, 0010110 ), W, DATA, DATA_IF_W, IMP_D(1), IMP_REG(0x0), _, _, _, _, _)

    INST(cmp, BIN(0e, 001110  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)
ALT_INST(cmp, BIN(20, 100000  ), S, W, MOD, BIN(07, 111), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0))
ALT_INST(cmp, BIN(1e, 0011110 ), W, DATA, DATA_IF_W, IMP_D(1), IMP_REG(0x0), IMP_S(1), _, _, _, _)

#undef _8086emu_INST_ARR
#undef _8086emu_INST_MNE_ENUM
#undef _8086emu_INST_MNE_STRING_LITERAL

#undef INST
#undef ALT_INST

#undef BIN
#undef D
#undef MOD
#undef REG
#undef RM
#undef _