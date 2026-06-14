/* NOTE: C89 standard didn't provide binary literal or macro with variable number of argument */

#ifdef INST_TABLE
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) \
	{op_##mne_in, {in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12}},
#define ALT_INST INST
#endif

#ifdef INST_MNE_ENUM
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) op_##mne_in,
#define ALT_INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12)
#endif

#ifdef INST_MNE_STRING_LITERAL
#define INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12) #mne_in,
#define ALT_INST(mne_in, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12)
#endif

#define BIN(hex_in, bin_in) {bits_literal, sizeof #bin_in - 1, 0x##hex_in}
#define S         {bits_s,         1}
#define W         {bits_w,         1}
#define D         {bits_d,         1}
#define V         {bits_v,         1}
#define Z         {bits_z,         1}
#define MOD       {bits_mod,       2}
#define REG       {bits_reg,       3}
#define SR        {bits_seg_reg,   2}
#define RM        {bits_rm,        3}
#define DISP_LO   {bits_disp_lo,   8}
#define DISP_HI   {bits_disp_hi,   8}
#define DATA_8    {bits_data,      8}
#define DATA      {bits_data,      8}
#define DATA_IF_W {bits_data_if_w, 8}
#define ADDR_LO   {bits_disp_lo,   8}
#define ADDR_HI   {bits_disp_hi,   8}
#define IP_INC_8  {bits_data,      8}
#define IP_INC_LO {bits_data,      8}
#define IP_INC_HI {bits_data_if_w, 8}
#define IP_LO     {bits_data,      8}
#define IP_HI     {bits_data_if_w, 8}
#define CS_LO     {bits_disp_lo,   8}
#define CS_HI     {bits_disp_hi,   8}

#define IMP_S(val)   {bits_s,   0, val}
#define IMP_W(val)   {bits_w,   0, val}
#define IMP_D(val)   {bits_d,   0, val}
#define IMP_MOD(val) {bits_mod, 0, val}
#define IMP_REG(val) {bits_reg, 0, val}
#define IMP_RM(val)  {bits_rm,  0, val}

#define IMP_DATA_8(val) {bits_data, 0, val}

#define IMP_RM_IS_W    {bits_rm_is_w,     0, 1}
#define IMP_IS_REL_JMP {bits_is_rel_jmp,  0, 1}
#define IMP_FORCE_DISP {bits_force_disp,  0, 1}
#define IMP_IS_FAR     {bits_is_far,      0, 1}

#define _ {0}


/* DATA TRANSFER */
                                                                                                                    /* mov = Move:                                */
    INST(mov, BIN(22, 100010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Register|memory to|from register         */
ALT_INST(mov, BIN(63, 1100011 ), IMP_S(0), IMP_D(0), W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)   /*   Immediate to register|memory             */
ALT_INST(mov, BIN(0b, 1011    ), IMP_S(0), IMP_D(1), W, REG, DATA, DATA_IF_W, _, _, _, _)                           /*   Immediate to register                    */
ALT_INST(mov, BIN(50, 1010000 ), IMP_MOD(0), IMP_REG(0), IMP_RM(6), W, ADDR_LO, ADDR_HI, IMP_D(1), _, _, _)         /*   Memory to accumulator                    */
ALT_INST(mov, BIN(51, 1010001 ), IMP_MOD(0), IMP_REG(0), IMP_RM(6), W, ADDR_LO, ADDR_HI, IMP_D(0), _, _, _)         /*   Accumulator to memory                    */
ALT_INST(mov, BIN(23, 100011  ), IMP_W(1), D, BIN(00, 0), MOD, BIN(00, 0), SR, RM, DISP_LO, DISP_HI, _)             /*   Register|memory to|from segment register */

                                                                                                                    /* push = Push:       */
    INST(push, BIN(ff, 11111111), IMP_W(1), IMP_D(0), MOD, BIN(06, 110), RM, DISP_LO, DISP_HI, _, _, _)             /*   Register|memory  */
ALT_INST(push, BIN(0a, 01010   ), IMP_W(1), IMP_D(1), REG, _, _, _, _, _, _, _)                                     /*   Register         */
ALT_INST(push, BIN(00, 000     ), IMP_D(1), SR, BIN(06, 110), _, _, _, _, _, _, _)                                  /*   Segment register */

                                                                                                                    /* pop = Pop:         */
    INST(pop, BIN(8f, 10001111), IMP_W(1), IMP_D(0), MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, _, _, _)              /*   Register|memory  */
ALT_INST(pop, BIN(0b, 01011   ), IMP_W(1), IMP_D(1), REG, _, _, _, _, _, _, _)                                      /*   Register         */
ALT_INST(pop, BIN(00, 000     ), IMP_D(1), SR, BIN(07, 111), _, _, _, _, _, _, _)                                   /*   Segment register */

                                                                                                                    /* xchg = Exchange:                */
    INST(xchg, BIN(43, 1000011 ), IMP_D(0), W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                             /*   Register|memory with register */
ALT_INST(xchg, BIN(12, 10010   ), IMP_D(0), IMP_W(1), IMP_MOD(3), IMP_RM(0), REG, _, _, _, _, _)                    /*   Register with accumulator     */

                                                                                                                    /* in = Input from */
    INST(in, BIN(72, 1110010 ), IMP_D(1), IMP_REG(0), W, DATA_8, _, _, _, _, _, _)                                  /*   Fixed port    */
ALT_INST(in, BIN(76, 1110110 ), IMP_RM_IS_W, W, IMP_D(1), IMP_MOD(3), IMP_REG(0), IMP_RM(2), _, _, _, _)            /*   Variable port */

                                                                                                                    /* out = Output from */
    INST(out, BIN(73, 1110011 ), IMP_D(0), IMP_REG(0), W, DATA_8, _, _, _, _, _, _)                                 /*   Fixed port      */
ALT_INST(out, BIN(77, 1110111 ), IMP_RM_IS_W, W, IMP_D(0), IMP_MOD(3), IMP_REG(0), IMP_RM(2), _, _, _, _)           /*   Variable port   */

    INST(xlat,  BIN(d7, 11010111), _, _, _, _, _, _, _, _, _, _)                                                    /* xlat = Translate byte to AL */
    INST(lea,   BIN(8d, 10001101), IMP_W(1), IMP_D(1), MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                     /* lea = Load EA to register   */
    INST(lds,   BIN(c5, 11000101), IMP_W(1), IMP_D(1), MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                     /* lds = Load pointer to DS    */
    INST(les,   BIN(c4, 11000100), IMP_W(1), IMP_D(1), MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                     /* les = Load pointer to ES    */
    INST(lahf,  BIN(9f, 10011111), _, _, _, _, _, _, _, _, _, _)                                                    /* lahf = Load AH with flags   */
    INST(sahf,  BIN(9e, 10011110), _, _, _, _, _, _, _, _, _, _)                                                    /* sahf = Store AH into flags  */
    INST(pushf, BIN(9c, 10011100), _, _, _, _, _, _, _, _, _, _)                                                    /* pushf = Push flags          */
    INST(popf,  BIN(9d, 10011101), _, _, _, _, _, _, _, _, _, _)                                                    /* popf = Pop flags            */

                                                                                                                    /* add = Add:                           */
    INST(add, BIN(00, 000000  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory with register to either */
ALT_INST(add, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)          /*   Immediate to register|memory       */
ALT_INST(add, BIN(02, 0000010 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator           */

                                                                                                                    /* adc = Add with carry:                */
    INST(adc, BIN(04, 000100  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory with register to either */
ALT_INST(adc, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(02, 010), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)          /*   Immediate to register|memory       */
ALT_INST(adc, BIN(0a, 0001010 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator           */

                                                                                                                    /* inc = Increment   */
    INST(inc, BIN(7f, 1111111 ), IMP_D(0), W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, _, _, _)                     /*   Register|memory */
ALT_INST(inc, BIN(08, 01000   ), IMP_W(1), IMP_D(1), REG, _, _, _, _, _, _, _)                                      /*   Register        */

    INST(aaa, BIN(37, 00110111), _, _, _, _, _, _, _, _, _, _)                                                      /* aaa = ASCII adjust for add   */
    INST(daa, BIN(27, 00100111), _, _, _, _, _, _, _, _, _, _)                                                      /* daa = Decimal adjust for add */

                                                                                                                    /* sub = Subtract:                      */
    INST(sub, BIN(0a, 001010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory with register to either */
ALT_INST(sub, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(05, 101), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)          /*   Immediate to register|memory       */
ALT_INST(sub, BIN(16, 0010110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator           */

                                                                                                                    /* sbb = Subtract with borrow:          */
    INST(sbb, BIN(06, 000110  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory with register to either */
ALT_INST(sbb, BIN(20, 100000  ), IMP_D(0), S, W, MOD, BIN(03, 011), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)          /*   Immediate to register|memory       */
ALT_INST(sbb, BIN(0e, 0001110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator           */

                                                                                                                    /* dec = Decrement:  */
    INST(dec, BIN(7f, 1111111 ), IMP_D(0), W, MOD, BIN(01, 001), RM, DISP_LO, DISP_HI, _, _, _)                     /*   Register|memory */
ALT_INST(dec, BIN(09, 01001   ), IMP_W(1), IMP_D(1), REG, _, _, _, _, _, _, _)                                      /*   Register        */

    INST(neg, BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(03, 011), RM, DISP_LO, DISP_HI, _, _, _)                     /* neg = Change sign */

                                                                                                                    /* cmp = Compare:                 */
    INST(cmp, BIN(0e, 001110  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory and register      */
ALT_INST(cmp, BIN(20, 100000  ), S, W, MOD, BIN(07, 111), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W, IMP_D(0))          /*   Immediate to register|memory */
ALT_INST(cmp, BIN(1e, 0011110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator     */

    INST(aas,  BIN(3f, 00111111), _, _, _, _, _, _, _, _, _, _)                                                     /* aas = ASCII adjust for subtract   */
    INST(das,  BIN(2f, 00101111), _, _, _, _, _, _, _, _, _, _)                                                     /* das = Decimal adjust for subtract */
    INST(mul,  BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(04, 100), RM, DISP_LO, DISP_HI, _, _, _)                    /* mul = Multiply (unsigned)         */
    INST(imul, BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(05, 101), RM, DISP_LO, DISP_HI, _, _, _)                    /* imul = Integer multiply (signed)  */
    INST(aam,  BIN(d4, 11010100), BIN(0a, 00001010), _, _, _, _, _, _, _, _, _)                                     /* aam = ASCII adjust for multiply   */
    INST(div,  BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(06, 110), RM, DISP_LO, DISP_HI, _, _, _)                    /* div = Divide (unsigned)           */
    INST(idiv, BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(07, 111), RM, DISP_LO, DISP_HI, _, _, _)                    /* idiv = Integer divide (signed)    */
    INST(aad,  BIN(d5, 11010101), BIN(0a, 00001010), _, _, _, _, _, _, _, _, _)                                     /* aad = ASCII adjust for divide     */
    INST(cbw,  BIN(98, 10011000), _, _, _, _, _, _, _, _, _, _)                                                     /* cbw = Convert byte to word        */
    INST(cwd,  BIN(99, 10011001), _, _, _, _, _, _, _, _, _, _)                                                     /* cwd = Convert word to double word */

/* LOGIC */

    INST(not, BIN(7b, 1111011 ), IMP_D(0), W, MOD, BIN(02, 010), RM, DISP_LO, DISP_HI, _, _, _)                     /* not = Invert                        */
    INST(shl, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(04, 100), RM, DISP_LO, DISP_HI, _, _)                     /* shl = Shift logical|arithmetic left */
    INST(shr, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(05, 101), RM, DISP_LO, DISP_HI, _, _)                     /* sar = Shift logical right           */
    INST(sar, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(07, 111), RM, DISP_LO, DISP_HI, _, _)                     /* imul = Integer multiply (signed)    */
    INST(rol, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, _, _)                     /* aam = ASCII adjust for multiply     */
    INST(ror, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(01, 001), RM, DISP_LO, DISP_HI, _, _)                     /* div = Divide (unsigned)             */
    INST(rcl, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(02, 010), RM, DISP_LO, DISP_HI, _, _)                     /* idiv = Integer divide (signed)      */
    INST(rcr, BIN(34, 110100  ), IMP_D(0), V, W, MOD, BIN(03, 011), RM, DISP_LO, DISP_HI, _, _)                     /* aad = ASCII adjust for divide       */

                                                                                                                    /* and = And:                           */
    INST(and, BIN(08, 001000  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory with register to either */
ALT_INST(and, BIN(40, 1000000 ), IMP_S(0), IMP_D(0), W, MOD, BIN(04, 100), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)   /*   Immediate to register|memory       */
ALT_INST(and, BIN(12, 0010010 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator           */

                                                                                                                    /* test = and function to flags no result: */
    INST(test, BIN(21, 100001  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                    /*   Register|memory and register          */
ALT_INST(test, BIN(7b, 1111011 ), IMP_S(0), IMP_D(0), W, MOD, BIN(00, 000), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)  /*   Immediate data and register|memory    */
ALT_INST(test, BIN(54, 1010100 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                   /*   Immediate data and accumulator        */

                                                                                                                    /* or = Or:                            */
    INST(or, BIN(02, 000010  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                      /*   Reg|memory and register to either */
ALT_INST(or, BIN(40, 1000000 ), IMP_S(0), IMP_D(0), W, MOD, BIN(01, 001), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)    /*   Immediate to register|memory      */
ALT_INST(or, BIN(06, 0000110 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                     /*   Immediate to accumulator          */

                                                                                                                    /* xor = Exclusive or:                 */
    INST(xor, BIN(0c, 001100  ), D, W, MOD, REG, RM, DISP_LO, DISP_HI, _, _, _)                                     /*   Reg|memory and register to either */
ALT_INST(xor, BIN(40, 1000000 ), IMP_S(0), IMP_D(0), W, MOD, BIN(6, 110), RM, DISP_LO, DISP_HI, DATA, DATA_IF_W)    /*   Immediate to register|memory      */
ALT_INST(xor, BIN(1a, 0011010 ), IMP_S(0), IMP_D(1), IMP_REG(0), W, DATA, DATA_IF_W, _, _, _, _)                    /*   Immediate to accumulator          */

/* STRING MANIPULATION */

    INST(rep,  BIN(79, 1111001 ), Z, _, _, _, _, _, _, _, _, _)                                                     /* rep = Repeat                      */
    INST(movs, BIN(52, 1010010 ), W, _, _, _, _, _, _, _, _, _)                                                     /* movs = Move byte|word             */
    INST(cmps, BIN(53, 1010011 ), W, _, _, _, _, _, _, _, _, _)                                                     /* cmps = Compare byte|word          */
    INST(scas, BIN(57, 1010111 ), W, _, _, _, _, _, _, _, _, _)                                                     /* scas = Scan byte|word             */
    INST(lods, BIN(56, 1010110 ), W, _, _, _, _, _, _, _, _, _)                                                     /* lods = Load byte|word to AL|AX    */
    INST(stos, BIN(55, 1010101 ), W, _, _, _, _, _, _, _, _, _)                                                     /* stos = Store byte|word from AL|AX */

/* CONTROL TRANSFER */
                                                                                                                    /* call = Call:              */
    INST(call, BIN(e8, 11101000), IMP_S(0), IMP_W(1), IP_INC_LO, IP_INC_HI, IMP_IS_REL_JMP, _, _, _, _, _)          /*   Direct within segment   */
ALT_INST(call, BIN(ff, 11111111), IMP_D(0), IMP_W(1), MOD, BIN(02, 010), RM, DISP_LO, DISP_HI, _, _, _)             /*   Indirect within segment */
ALT_INST(call, BIN(9a, 10011010), IMP_S(0), IMP_W(1), IMP_FORCE_DISP, IP_LO, IP_HI, CS_LO, CS_HI, IMP_IS_FAR, _, _) /*   Direct intersegment     */
ALT_INST(call, BIN(ff, 11111111), IMP_W(1), IMP_D(0), MOD, BIN(3, 011), RM, DISP_LO, DISP_HI, IMP_IS_FAR, _, _)     /*   Indirect intersegment   */

                                                                                                                    /* jmp = Unconditional Jump:      */
    INST(jmp, BIN(e9, 11101001), IMP_S(0), IMP_W(1), IP_INC_LO, IP_INC_HI, IMP_IS_REL_JMP, _, _, _, _, _)           /*    Direct within segment       */
ALT_INST(jmp, BIN(eb, 11101011), IMP_S(0), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                    /*    Direct within segment-short */
ALT_INST(jmp, BIN(ff, 11111111), IMP_D(0), IMP_W(1), MOD, BIN(04, 100), RM, DISP_LO, DISP_HI, _, _, _)              /*    Indirect within segment     */
ALT_INST(jmp, BIN(ea, 11101010), IMP_S(0), IMP_W(1), IMP_FORCE_DISP, IP_LO, IP_HI, CS_LO, CS_HI, IMP_IS_FAR, _, _)  /*    Direct intersegment         */
ALT_INST(jmp, BIN(ff, 11111111), IMP_W(1), IMP_D(0), MOD, BIN(5, 101), RM, DISP_LO, DISP_HI, IMP_IS_FAR, _, _)      /*    Indirect intersegment       */
                                                                                                                    /* ret = Return from CALL:                 */
    INST(ret,  BIN(c3, 11000011), _, _, _, _, _, _, _, _, _, _)                                                     /*   Within segment                        */
ALT_INST(ret,  BIN(c2, 11000010), IMP_S(0), IMP_W(1), DATA, DATA_IF_W, _, _, _, _, _, _)                            /*   Within segment adding immediate to SP */
    INST(retf, BIN(cb, 11001011), IMP_IS_FAR, _, _, _, _, _, _, _, _, _)                                            /*   Intersegment                          */
ALT_INST(retf, BIN(ca, 11001010), IMP_S(0), IMP_W(1), DATA, DATA_IF_W, IMP_IS_FAR, _, _, _, _, _)                   /*   Intersegment adding immediate to SP   */

    INST(je,     BIN(74, 01110100), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* je = Jump on equal|zero                 */
    INST(jl,     BIN(7c, 01111100), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jl = Jump on less|not greater or equal  */
    INST(jle,    BIN(7e, 01111110), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jle = Jump on less or equal|not greater */
    INST(jb,     BIN(72, 01110010), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jb = Jump on below|not above or equal   */
    INST(jbe,    BIN(76, 01110110), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jbe = Jump on below or equal|not above  */
    INST(jp,     BIN(7a, 01111010), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jp = Jump on parity|parity even         */
    INST(jo,     BIN(70, 01110000), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jo = Jump on overflow                   */
    INST(js,     BIN(78, 01111000), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* js = Jump on sign                       */
    INST(jne,    BIN(75, 01110101), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jne = Jump on not equal|zero            */
    INST(jnl,    BIN(7d, 01111101), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jnl = Jump on not less|greater or equal */
    INST(jg,     BIN(7f, 01111111), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jg = Jump on not less or equal|greater  */
    INST(jnb,    BIN(73, 01110011), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jnb = Jump on not below|above or equal  */
    INST(ja,     BIN(77, 01110111), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* ja = Jump on not below or equal|above   */
    INST(jnp,    BIN(7b, 01111011), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jnp = Jump on not par|par odd           */
    INST(jno,    BIN(71, 01110001), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jno = Jump on not overflow              */
    INST(jns,    BIN(79, 01111001), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jns = Jump on not sign                  */
    INST(loop,   BIN(e2, 11100010), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* loop = Loop CX times                    */
    INST(loopz,  BIN(e1, 11100001), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* loopz = Loop while zero|equal           */
    INST(loopnz, BIN(e0, 11100000), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* loopnz = Loop while not zero|equal      */
    INST(jcxz,   BIN(e3, 11100011), IMP_S(1), IMP_W(0), IP_INC_8, IMP_IS_REL_JMP, _, _, _, _, _, _)                 /* jcxz = Jump on CX zero                  */

                                                                                                                    /* int = Interrupt: */
    INST(int,  BIN(cd, 11001101), DATA_8, _, _, _, _, _, _, _, _, _)                                                /*   Type specified */
    INST(int3, BIN(cc, 11001100), _, _, _, _, _, _, _, _, _, _)                                                     /*   Type 3         */

    INST(into, BIN(ce, 11001110), _, _, _, _, _, _, _, _, _, _)                                                     /* into = Interrupt on overflow */
    INST(iret, BIN(cf, 11001111), _, _, _, _, _, _, _, _, _, _)                                                     /* iret = Interrupt return      */

/* PROCESSOR CONTROL */

    INST(clc,  BIN(f8, 11111000), _, _, _, _, _, _, _, _, _, _)                                                     /* clc = Clear carry                 */
    INST(cmc,  BIN(f5, 11110101), _, _, _, _, _, _, _, _, _, _)                                                     /* cmc = Complement carry            */
    INST(stc,  BIN(f9, 11111001), _, _, _, _, _, _, _, _, _, _)                                                     /* stc = Set carry                   */
    INST(cld,  BIN(fc, 11111100), _, _, _, _, _, _, _, _, _, _)                                                     /* cld = Clear direction             */
    INST(std,  BIN(fd, 11111101), _, _, _, _, _, _, _, _, _, _)                                                     /* std = Set direction               */
    INST(cli,  BIN(fa, 11111010), _, _, _, _, _, _, _, _, _, _)                                                     /* cli = Clear interrupt             */
    INST(sti,  BIN(fb, 11111011), _, _, _, _, _, _, _, _, _, _)                                                     /* sti = Set interrupt               */
    INST(hlt,  BIN(f4, 11110100), _, _, _, _, _, _, _, _, _, _)                                                     /* hlt = Halt                        */
    INST(wait, BIN(9b, 10011011), _, _, _, _, _, _, _, _, _, _)                                                     /* wait = Wait                       */
/*  INST(esc,  BIN(1b, 11011   ), XXX, MOD, YYY, RM, DISP_LO, DISP_HI, _, _, _, _)                                     esc = Escape (to external device) */
    INST(lock, BIN(f0, 11110000), _, _, _, _, _, _, _, _, _, _)                                                     /* lock = Bus lock prefix            */
    INST(segment, BIN(01, 001), SR, BIN(6, 110), _, _, _, _, _, _, _, _)                                            /* segment = Override prefix         */


#undef INST_TABLE
#undef INST_MNE_ENUM
#undef INST_MNE_STRING_LITERAL

#undef INST
#undef ALT_INST

#undef BIN
#undef S
#undef W
#undef D
#undef V
#undef Z
#undef MOD
#undef REG
#undef SR
#undef RM
#undef DISP_LO
#undef DISP_HI
#undef DATA_8
#undef DATA
#undef DATA_IF_W
#undef ADDR_LO
#undef ADDR_HI
#undef IP_INC_8
#undef IP_INC_LO
#undef IP_INC_HI
#undef IP_LO
#undef IP_HI
#undef CS_LO
#undef CS_HI

#undef IMP_S
#undef IMP_W
#undef IMP_D
#undef IMP_MOD
#undef IMP_REG
#undef IMP_RM

#undef IMP_DATA_8

#undef IMP_RM_IS_W
#undef IMP_IS_REL_JMP
#undef IMP_FORCE_DISP
#undef IMP_IS_FAR

#undef _
