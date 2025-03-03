/*
** $Id: ljumptab.h $
** Jump Table for the Lua interpreter
** See Copyright Notice in lua.h
*/

#undef vmdispatch
#undef vmcase
#undef vmbreak

#define vmdispatch(x)     goto *disptab[x];

#define vmcase(l)     L_##l:

#define vmbreak		vmfetch(); vmdispatch(GET_OPCODE(i));

static const void* const disptab[NUM_OPCODES] = {
#if 0
* *you can update the following list with this command:
**
**sed - n '/^OP_/\!d; s/OP_/\&\&L_OP_/ ; s/,.*/,/ ; s/\/.*// ; p'  lopcodes.h
* *
#endif

#define L_OP_MOVE 0
#define L_OP_LOADI 1
#define L_OP_LOADF 2
#define L_OP_LOADK 3
#define L_OP_LOADKX 4
#define L_OP_LOADFALSE 5
#define L_OP_LFALSESKIP 6
#define L_OP_LOADTRUE 7
#define L_OP_LOADNIL 8
#define L_OP_GETUPVAL 9
#define L_OP_SETUPVAL 10
#define L_OP_GETTABUP 11
#define L_OP_GETTABLE 12
#define L_OP_GETI 13
#define L_OP_GETFIELD 14
#define L_OP_SETTABUP 15
#define L_OP_SETTABLE 16
#define L_OP_SETI 17
#define L_OP_SETFIELD 18
#define L_OP_NEWTABLE 19
#define L_OP_SELF 20
#define L_OP_ADDI 21
#define L_OP_ADDK 22
#define L_OP_SUBK 23
#define L_OP_MULK 24
#define L_OP_MODK 25
#define L_OP_POWK 26
#define L_OP_DIVK 27
#define L_OP_IDIVK 28
#define L_OP_BANDK 29
#define L_OP_BORK 30
#define L_OP_BXORK 31
#define L_OP_SHRI 32
#define L_OP_SHLI 33
#define L_OP_ADD 34
#define L_OP_SUB 35
#define L_OP_MUL 36
#define L_OP_MOD 37
#define L_OP_POW 38
#define L_OP_DIV 39
#define L_OP_IDIV 40
#define L_OP_BAND 41
#define L_OP_BOR 42
#define L_OP_BXOR 43
#define L_OP_SHL 44
#define L_OP_SHR 45
#define L_OP_MMBIN 46
#define L_OP_MMBINI 47
#define L_OP_MMBINK 48
#define L_OP_UNM 49
#define L_OP_BNOT 50
#define L_OP_NOT 51
#define L_OP_LEN 52
#define L_OP_CONCAT 53
#define L_OP_CLOSE 54
#define L_OP_TBC 55
#define L_OP_JMP 56
#define L_OP_EQ 57
#define L_OP_LT 58
#define L_OP_LE 59
#define L_OP_EQK 60
#define L_OP_EQI 61
#define L_OP_LTI 62
#define L_OP_LEI 63
#define L_OP_GTI 64
#define L_OP_GEI 65
#define L_OP_TEST 66
#define L_OP_TESTSET 67
#define L_OP_CALL 68
#define L_OP_TAILCALL 69
#define L_OP_RETURN 70
#define L_OP_RETURN0 71
#define L_OP_RETURN1 72
#define L_OP_FORLOOP 73
#define L_OP_FORPREP 74
#define L_OP_TFORPREP 75
#define L_OP_TFORCALL 76
#define L_OP_TFORLOOP 77
#define L_OP_SETLIST 78
#define L_OP_CLOSURE 79
#define L_OP_VARARG 80
#define L_OP_VARARGPREP 81
#define L_OP_EXTRAARG 82
};
