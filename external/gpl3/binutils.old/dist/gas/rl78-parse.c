/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         rl78_parse
#define yylex           rl78_lex
#define yyerror         rl78_error
#define yydebug         rl78_debug
#define yynerrs         rl78_nerrs

#define yylval          rl78_lval
#define yychar          rl78_char

/* Copy the first part of user declarations.  */
#line 20 "./config/rl78-parse.y" /* yacc.c:339  */


#include "as.h"
#include "safe-ctype.h"
#include "rl78-defs.h"

static int rl78_lex (void);

/* Ok, here are the rules for using these macros...

   B*() is used to specify the base opcode bytes.  Fields to be filled
        in later, leave zero.  Call this first.

   F() and FE() are used to fill in fields within the base opcode bytes.  You MUST
        call B*() before any F() or FE().

   [UN]*O*(), PC*() appends operands to the end of the opcode.  You
        must call P() and B*() before any of these, so that the fixups
        have the right byte location.
        O = signed, UO = unsigned, NO = negated, PC = pcrel

   IMM() adds an immediate and fills in the field for it.
   NIMM() same, but negates the immediate.
   NBIMM() same, but negates the immediate, for sbb.
   DSP() adds a displacement, and fills in the field for it.

   Note that order is significant for the O, IMM, and DSP macros, as
   they append their data to the operand buffer in the order that you
   call them.

   Use "disp" for displacements whenever possible; this handles the
   "0" case properly.  */

#define B1(b1)             rl78_base1 (b1)
#define B2(b1, b2)         rl78_base2 (b1, b2)
#define B3(b1, b2, b3)     rl78_base3 (b1, b2, b3)
#define B4(b1, b2, b3, b4) rl78_base4 (b1, b2, b3, b4)

/* POS is bits from the MSB of the first byte to the LSB of the last byte.  */
#define F(val,pos,sz)      rl78_field (val, pos, sz)
#define FE(exp,pos,sz)	   rl78_field (exp_val (exp), pos, sz);

#define O1(v)              rl78_op (v, 1, RL78REL_DATA)
#define O2(v)              rl78_op (v, 2, RL78REL_DATA)
#define O3(v)              rl78_op (v, 3, RL78REL_DATA)
#define O4(v)              rl78_op (v, 4, RL78REL_DATA)

#define PC1(v)             rl78_op (v, 1, RL78REL_PCREL)
#define PC2(v)             rl78_op (v, 2, RL78REL_PCREL)
#define PC3(v)             rl78_op (v, 3, RL78REL_PCREL)

#define IMM(v,pos)	   F (immediate (v, RL78REL_SIGNED, pos), pos, 2); \
			   if (v.X_op != O_constant && v.X_op != O_big) rl78_linkrelax_imm (pos)
#define NIMM(v,pos)	   F (immediate (v, RL78REL_NEGATIVE, pos), pos, 2)
#define NBIMM(v,pos)	   F (immediate (v, RL78REL_NEGATIVE_BORROW, pos), pos, 2)
#define DSP(v,pos,msz)	   if (!v.X_md) rl78_relax (RL78_RELAX_DISP, pos); \
			   else rl78_linkrelax_dsp (pos); \
			   F (displacement (v, msz), pos, 2)

#define id24(a,b2,b3)	   B3 (0xfb+a, b2, b3)

static int         expr_is_sfr (expressionS);
static int         expr_is_saddr (expressionS);
static int         expr_is_word_aligned (expressionS);
static int         exp_val (expressionS exp);

static int    need_flag = 0;
static int    rl78_in_brackets = 0;
static int    rl78_last_token = 0;
static char * rl78_init_start;
static char * rl78_last_exp_start = 0;
static int    rl78_bit_insn = 0;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#define NOT_SADDR  rl78_error ("Expression not 0xFFE20 to 0xFFF1F")
#define SA(e) if (!expr_is_saddr (e)) NOT_SADDR;

#define SET_SA(e) e.X_md = BFD_RELOC_RL78_SADDR

#define NOT_SFR  rl78_error ("Expression not 0xFFF00 to 0xFFFFF")
#define SFR(e) if (!expr_is_sfr (e)) NOT_SFR;

#define NOT_SFR_OR_SADDR  rl78_error ("Expression not 0xFFE20 to 0xFFFFF")

#define NOT_ES if (rl78_has_prefix()) rl78_error ("ES: prefix not allowed here");

#define WA(x) if (!expr_is_word_aligned (x)) rl78_error ("Expression not word-aligned");

#define ISA_G10(s) if (!rl78_isa_g10()) rl78_error (s " is only supported on the G10")
#define ISA_G13(s) if (!rl78_isa_g13()) rl78_error (s " is only supported on the G13")
#define ISA_G14(s) if (!rl78_isa_g14()) rl78_error (s " is only supported on the G14")

static void check_expr_is_bit_index (expressionS);
#define Bit(e) check_expr_is_bit_index (e);

/* Returns TRUE (non-zero) if the expression is a constant in the
   given range.  */
static int check_expr_is_const (expressionS, int vmin, int vmax);

/* Convert a "regb" value to a "reg_xbc" value.  Error if other
   registers are passed.  Needed to avoid reduce-reduce conflicts.  */
static int
reg_xbc (int reg)
{
  switch (reg)
    {
      case 0: /* X */
        return 0x10;
      case 3: /* B */
        return 0x20;
      case 2: /* C */
        return 0x30;
      default:
        rl78_error ("Only X, B, or C allowed here");
	return 0;
    }
}


#line 196 "rl78-parse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_RL78_RL_PARSE_H_INCLUDED
# define YY_RL78_RL_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int rl78_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    A = 258,
    X = 259,
    B = 260,
    C = 261,
    D = 262,
    E = 263,
    H = 264,
    L = 265,
    AX = 266,
    BC = 267,
    DE = 268,
    HL = 269,
    SPL = 270,
    SPH = 271,
    PSW = 272,
    CS = 273,
    ES = 274,
    PMC = 275,
    MEM = 276,
    FLAG = 277,
    SP = 278,
    CY = 279,
    RB0 = 280,
    RB1 = 281,
    RB2 = 282,
    RB3 = 283,
    EXPR = 284,
    UNKNOWN_OPCODE = 285,
    IS_OPCODE = 286,
    DOT_S = 287,
    DOT_B = 288,
    DOT_W = 289,
    DOT_L = 290,
    DOT_A = 291,
    DOT_UB = 292,
    DOT_UW = 293,
    ADD = 294,
    ADDC = 295,
    ADDW = 296,
    AND_ = 297,
    AND1 = 298,
    BF = 299,
    BH = 300,
    BNC = 301,
    BNH = 302,
    BNZ = 303,
    BR = 304,
    BRK = 305,
    BRK1 = 306,
    BT = 307,
    BTCLR = 308,
    BZ = 309,
    CALL = 310,
    CALLT = 311,
    CLR1 = 312,
    CLRB = 313,
    CLRW = 314,
    CMP = 315,
    CMP0 = 316,
    CMPS = 317,
    CMPW = 318,
    DEC = 319,
    DECW = 320,
    DI = 321,
    DIVHU = 322,
    DIVWU = 323,
    EI = 324,
    HALT = 325,
    INC = 326,
    INCW = 327,
    MACH = 328,
    MACHU = 329,
    MOV = 330,
    MOV1 = 331,
    MOVS = 332,
    MOVW = 333,
    MULH = 334,
    MULHU = 335,
    MULU = 336,
    NOP = 337,
    NOT1 = 338,
    ONEB = 339,
    ONEW = 340,
    OR = 341,
    OR1 = 342,
    POP = 343,
    PUSH = 344,
    RET = 345,
    RETI = 346,
    RETB = 347,
    ROL = 348,
    ROLC = 349,
    ROLWC = 350,
    ROR = 351,
    RORC = 352,
    SAR = 353,
    SARW = 354,
    SEL = 355,
    SET1 = 356,
    SHL = 357,
    SHLW = 358,
    SHR = 359,
    SHRW = 360,
    SKC = 361,
    SKH = 362,
    SKNC = 363,
    SKNH = 364,
    SKNZ = 365,
    SKZ = 366,
    STOP = 367,
    SUB = 368,
    SUBC = 369,
    SUBW = 370,
    XCH = 371,
    XCHW = 372,
    XOR = 373,
    XOR1 = 374
  };
#endif
/* Tokens.  */
#define A 258
#define X 259
#define B 260
#define C 261
#define D 262
#define E 263
#define H 264
#define L 265
#define AX 266
#define BC 267
#define DE 268
#define HL 269
#define SPL 270
#define SPH 271
#define PSW 272
#define CS 273
#define ES 274
#define PMC 275
#define MEM 276
#define FLAG 277
#define SP 278
#define CY 279
#define RB0 280
#define RB1 281
#define RB2 282
#define RB3 283
#define EXPR 284
#define UNKNOWN_OPCODE 285
#define IS_OPCODE 286
#define DOT_S 287
#define DOT_B 288
#define DOT_W 289
#define DOT_L 290
#define DOT_A 291
#define DOT_UB 292
#define DOT_UW 293
#define ADD 294
#define ADDC 295
#define ADDW 296
#define AND_ 297
#define AND1 298
#define BF 299
#define BH 300
#define BNC 301
#define BNH 302
#define BNZ 303
#define BR 304
#define BRK 305
#define BRK1 306
#define BT 307
#define BTCLR 308
#define BZ 309
#define CALL 310
#define CALLT 311
#define CLR1 312
#define CLRB 313
#define CLRW 314
#define CMP 315
#define CMP0 316
#define CMPS 317
#define CMPW 318
#define DEC 319
#define DECW 320
#define DI 321
#define DIVHU 322
#define DIVWU 323
#define EI 324
#define HALT 325
#define INC 326
#define INCW 327
#define MACH 328
#define MACHU 329
#define MOV 330
#define MOV1 331
#define MOVS 332
#define MOVW 333
#define MULH 334
#define MULHU 335
#define MULU 336
#define NOP 337
#define NOT1 338
#define ONEB 339
#define ONEW 340
#define OR 341
#define OR1 342
#define POP 343
#define PUSH 344
#define RET 345
#define RETI 346
#define RETB 347
#define ROL 348
#define ROLC 349
#define ROLWC 350
#define ROR 351
#define RORC 352
#define SAR 353
#define SARW 354
#define SEL 355
#define SET1 356
#define SHL 357
#define SHLW 358
#define SHR 359
#define SHRW 360
#define SKC 361
#define SKH 362
#define SKNC 363
#define SKNH 364
#define SKNZ 365
#define SKZ 366
#define STOP 367
#define SUB 368
#define SUBC 369
#define SUBW 370
#define XCH 371
#define XCHW 372
#define XOR 373
#define XOR1 374

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 144 "./config/rl78-parse.y" /* yacc.c:355  */

  int regno;
  expressionS exp;

#line 479 "rl78-parse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE rl78_lval;

int rl78_parse (void);

#endif /* !YY_RL78_RL_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 496 "rl78-parse.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  180
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   844

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  129
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  56
/* YYNRULES -- Number of rules.  */
#define YYNRULES  324
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  744

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   374

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   122,     2,   121,   127,     2,     2,     2,
       2,     2,     2,   125,   120,     2,   126,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   128,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   123,     2,   124,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   187,   187,   208,   211,   211,   214,   217,   220,   223,
     223,   226,   229,   232,   235,   238,   241,   250,   253,   256,
     256,   259,   262,   265,   268,   276,   276,   279,   279,   288,
     288,   291,   291,   296,   299,   302,   305,   308,   311,   316,
     319,   328,   331,   336,   339,   342,   345,   348,   353,   356,
     361,   364,   367,   370,   373,   394,   397,   400,   409,   412,
     415,   420,   422,   424,   426,   429,   429,   432,   437,   439,
     444,   447,   450,   453,   456,   456,   459,   464,   469,   472,
     472,   474,   476,   478,   480,   485,   488,   488,   491,   494,
     499,   502,   507,   507,   510,   510,   513,   516,   516,   524,
     524,   527,   527,   530,   530,   535,   543,   545,   548,   555,
     555,   564,   567,   570,   573,   576,   576,   585,   588,   591,
     594,   597,   597,   606,   606,   606,   609,   612,   619,   619,
     619,   626,   629,   632,   635,   638,   641,   644,   647,   650,
     653,   656,   659,   662,   665,   668,   671,   674,   677,   680,
     683,   686,   689,   692,   695,   698,   701,   704,   704,   707,
     707,   710,   710,   713,   713,   716,   716,   719,   719,   724,
     733,   736,   739,   742,   751,   754,   757,   762,   767,   770,
     773,   773,   782,   782,   791,   791,   800,   803,   806,   809,
     812,   815,   818,   821,   824,   827,   830,   833,   836,   839,
     842,   845,   848,   851,   854,   857,   860,   860,   863,   863,
     866,   866,   869,   869,   872,   872,   875,   878,   881,   884,
     887,   892,   897,   902,   905,   908,   911,   916,   919,   922,
     927,   932,   937,   942,   947,   952,   959,   964,   971,   974,
     977,   980,   985,   990,   995,  1000,  1005,  1012,  1017,  1024,
    1027,  1030,  1033,  1036,  1039,  1044,  1049,  1056,  1059,  1062,
    1065,  1068,  1071,  1074,  1077,  1088,  1097,  1098,  1102,  1103,
    1104,  1105,  1106,  1107,  1108,  1109,  1112,  1113,  1114,  1115,
    1116,  1117,  1118,  1121,  1122,  1123,  1124,  1127,  1128,  1129,
    1132,  1133,  1134,  1135,  1136,  1137,  1138,  1144,  1145,  1146,
    1147,  1148,  1149,  1150,  1151,  1154,  1155,  1156,  1159,  1160,
    1161,  1164,  1165,  1166,  1169,  1170,  1173,  1174,  1177,  1178,
    1181,  1182,  1185,  1186,  1189
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "A", "X", "B", "C", "D", "E", "H", "L",
  "AX", "BC", "DE", "HL", "SPL", "SPH", "PSW", "CS", "ES", "PMC", "MEM",
  "FLAG", "SP", "CY", "RB0", "RB1", "RB2", "RB3", "EXPR", "UNKNOWN_OPCODE",
  "IS_OPCODE", "DOT_S", "DOT_B", "DOT_W", "DOT_L", "DOT_A", "DOT_UB",
  "DOT_UW", "ADD", "ADDC", "ADDW", "AND_", "AND1", "BF", "BH", "BNC",
  "BNH", "BNZ", "BR", "BRK", "BRK1", "BT", "BTCLR", "BZ", "CALL", "CALLT",
  "CLR1", "CLRB", "CLRW", "CMP", "CMP0", "CMPS", "CMPW", "DEC", "DECW",
  "DI", "DIVHU", "DIVWU", "EI", "HALT", "INC", "INCW", "MACH", "MACHU",
  "MOV", "MOV1", "MOVS", "MOVW", "MULH", "MULHU", "MULU", "NOP", "NOT1",
  "ONEB", "ONEW", "OR", "OR1", "POP", "PUSH", "RET", "RETI", "RETB", "ROL",
  "ROLC", "ROLWC", "ROR", "RORC", "SAR", "SARW", "SEL", "SET1", "SHL",
  "SHLW", "SHR", "SHRW", "SKC", "SKH", "SKNC", "SKNH", "SKNZ", "SKZ",
  "STOP", "SUB", "SUBC", "SUBW", "XCH", "XCHW", "XOR", "XOR1", "','",
  "'#'", "'!'", "'['", "']'", "'+'", "'.'", "'$'", "':'", "$accept",
  "statement", "$@1", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7", "$@8",
  "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15", "$@16", "$@17",
  "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "$@26",
  "$@27", "$@28", "$@29", "$@30", "$@31", "$@32", "$@33", "$@34", "$@35",
  "$@36", "$@37", "$@38", "opt_es", "regb", "regb_na", "regw", "regw_na",
  "sfr", "addsub", "addsubw", "andor1", "bt_bf", "setclr1", "oneclrb",
  "oneclrw", "incdec", "incdecw", "mov1", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
      44,    35,    33,    91,    93,    43,    46,    36,    58
};
# endif

#define YYPACT_NINF -212

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-212)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     226,   -44,  -212,  -212,  -212,  -212,  -212,  -212,  -212,   -34,
     -15,    14,    20,    42,  -212,  -212,  -212,  -212,    45,    91,
     -80,  -212,  -212,  -212,  -212,   347,   134,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,    53,
    -212,   136,   430,  -212,  -212,   153,  -212,   150,  -212,  -212,
    -212,  -212,   413,   451,  -212,  -212,  -212,   181,   186,   198,
     192,   212,   214,   225,   232,  -212,   368,   210,   236,   231,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
     242,   244,  -212,  -212,   261,   221,   107,   325,   162,   343,
     403,   222,    10,   235,   362,   328,   341,   385,   394,   402,
    -212,    38,    81,   411,  -212,  -212,  -212,  -212,    82,   263,
    -212,   428,  -212,  -212,  -212,  -212,   300,  -212,   273,   351,
    -212,  -212,  -212,  -212,   367,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,   376,  -212,  -212,   457,
      11,   372,   383,   384,   386,  -212,  -212,  -212,   388,    36,
     389,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,   390,
     391,   392,   393,   395,   396,   397,   398,  -212,  -212,  -212,
    -212,   401,   404,   405,   406,   407,   408,   409,   410,   412,
    -212,   414,  -212,   415,   416,   418,   419,   420,   379,   300,
     417,   421,   422,   423,  -212,   424,     0,   425,  -212,  -212,
    -212,  -212,  -212,   431,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,   426,  -212,   485,   505,  -212,  -212,
     233,  -212,   432,   427,   434,   436,   435,  -212,  -212,  -212,
    -212,  -212,  -212,   491,  -212,   493,  -212,  -212,   494,   502,
     439,  -212,  -212,   504,   136,    80,   440,   437,   -79,   184,
      29,    72,   521,   123,    41,    79,   512,   239,   102,   513,
     516,   517,   523,   526,   527,   535,   536,   537,   538,   539,
     540,   541,   542,   543,   429,   399,    18,   453,   545,   572,
      95,   455,   381,   548,   549,   565,   551,   552,   553,   554,
     570,   556,  -212,   557,   267,  -212,  -212,   462,  -212,   559,
     575,   561,   400,   562,   578,   564,  -212,  -212,  -212,  -212,
    -212,  -212,   471,   566,    63,  -212,  -212,   567,     5,     6,
     161,   473,   268,   280,   330,  -212,   569,    86,   571,   573,
    -212,   474,  -212,   574,    65,  -212,  -212,   576,    55,   342,
     481,   480,   335,   345,   348,  -212,  -212,  -212,   577,   486,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,   344,  -212,  -212,  -212,
    -212,   580,   352,  -212,   489,   487,  -212,  -212,   582,   354,
    -212,   583,   488,   490,   492,   495,   497,   498,   496,   499,
    -212,  -212,   500,   501,  -212,  -212,   584,   608,   594,  -212,
     503,   507,   506,   508,   510,   509,   511,   514,   519,   610,
    -212,   518,   600,   433,   520,  -212,   601,  -212,   607,   522,
     524,   525,   530,   531,   613,   532,   111,   533,   614,  -212,
    -212,   615,  -212,  -212,   616,  -212,   534,   618,   438,  -212,
    -212,   625,   544,   546,   547,   626,   555,   558,   627,   560,
     629,   563,   630,  -212,  -212,   631,   632,   465,  -212,  -212,
     633,   641,   634,   568,  -212,  -212,   635,   651,  -212,   637,
     638,   655,   643,   550,   579,   581,   585,   644,   587,  -212,
     586,   590,   645,   652,   650,   653,   667,   656,   660,   589,
     662,   591,   363,  -212,  -212,   593,   356,   358,   360,     7,
    -212,  -212,  -212,   588,   598,   599,     8,   684,   596,   685,
     597,   602,   603,    27,   604,  -212,  -212,  -212,   605,   382,
    -212,  -212,   606,   364,   366,   369,  -212,  -212,   611,   612,
     617,  -212,   679,   680,   609,   681,   619,   682,   620,  -212,
    -212,   371,   373,  -212,   375,  -212,   665,  -212,   377,  -212,
    -212,   621,  -212,   666,   668,   669,   670,  -212,   671,   672,
    -212,   622,  -212,  -212,  -212,   623,  -212,  -212,   673,  -212,
     674,   624,   628,   636,  -212,  -212,   675,  -212,   114,  -212,
     676,  -212,   693,  -212,    28,    30,    31,  -212,   694,  -212,
     639,  -212,   642,   646,   647,  -212,   695,   648,  -212,  -212,
     649,   640,   654,   657,  -212,  -212,   696,  -212,   705,  -212,
     706,  -212,   698,   699,   725,  -212,  -212,   659,  -212,   663,
    -212,   664,  -212,   709,  -212,   139,  -212,   165,  -212,  -212,
     710,  -212,  -212,   661,  -212,  -212,  -212,   677,  -212,  -212,
     658,  -212,   678,   683,   686,  -212,  -212,  -212,   687,   688,
     689,   690,  -212,   691,  -212,  -212,   711,  -212,   712,  -212,
     713,  -212,    32,   746,   747,    33,  -212,  -212,    35,   692,
    -212,  -212,  -212,   697,   700,  -212,   701,  -212,  -212,  -212,
     740,   742,  -212,   743,   702,   703,   704,   707,   708,   714,
     715,   716,   726,   717,  -212,   727,   733,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,   729,  -212,  -212,
    -212,   732,  -212,  -212,   734,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,   736,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     2,   297,   298,   305,   302,   308,   312,     0,
       0,     0,     0,     0,    48,    49,   311,   313,     0,     0,
       0,   315,   317,   319,   301,   266,     0,   307,   321,   323,
      90,    97,    99,    91,   105,   320,   322,   103,   101,   266,
     324,   266,   266,    94,    92,     0,   221,     0,   316,   318,
     303,   309,     0,     0,   227,   228,   229,     0,     0,     0,
       0,     0,     0,     0,     0,   314,     0,     0,     0,     0,
     249,   250,   251,   252,   253,   254,   255,   299,   300,   306,
       0,     0,   304,   310,     0,   266,     0,     0,   266,   266,
     266,     0,     0,   266,   266,     0,     0,     0,     0,     0,
      43,     0,     0,     0,   283,   284,   285,   286,     0,     0,
      50,     0,    70,    71,    72,    73,     0,    74,     0,     0,
      98,   100,   104,   102,     0,   276,   278,   277,   280,   279,
     282,   281,   290,   291,   292,   293,   294,   295,   296,     0,
       0,     0,     0,     0,     0,   287,   288,   289,     0,     0,
       0,    95,    93,    96,   222,   224,   223,   226,   225,     0,
       0,     0,     0,     0,     0,     0,     0,   238,   239,   240,
     241,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       1,     0,     4,     0,     0,     0,     0,     0,     0,   294,
       0,     0,     0,     0,    55,     0,     0,     0,    61,    62,
      63,    64,    65,     0,    68,    69,   269,   268,   271,   270,
     273,   272,   275,   274,     0,    79,     0,     0,    78,    86,
       0,    85,     0,     0,     0,     0,     0,    33,    37,    34,
      38,    36,    46,     0,    44,     0,    35,    52,     0,     0,
       0,   267,    75,     0,   266,   266,   267,     0,     0,     0,
     266,   266,     0,   266,     0,     0,     0,     0,   266,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   266,     0,   266,     0,     0,     0,
     266,     0,   266,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    66,     0,     0,    80,    81,     0,    87,     0,
       0,     0,   266,     0,     0,     0,    47,    45,    53,    51,
      54,    76,     0,     0,     0,   114,   126,     0,     0,     0,
       0,     0,     0,     0,     0,   113,     0,     0,     0,     0,
     127,     0,   219,     0,     0,   186,   218,     0,     0,     0,
       0,     0,     0,     0,     0,   187,   220,   214,     0,     0,
     230,   231,   232,   233,   234,   235,   236,   237,   242,   243,
     244,   245,   246,   247,   248,   264,     0,   256,   265,     6,
       9,     0,     0,     7,     0,     0,     8,    19,     0,     0,
      18,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      58,    57,     0,     0,    56,    67,     0,     0,     0,    88,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     106,   121,     0,     0,     0,   118,     0,   115,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
     123,     0,   108,   128,     0,   178,   182,     0,     0,   217,
     184,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   215,   179,     0,     0,     0,    10,     3,
       0,     0,     0,     0,    20,    17,     0,     0,    24,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    82,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   122,   117,     0,     0,     0,     0,     0,
     111,   116,   109,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   124,   120,   129,     0,     0,
     183,   188,     0,     0,     0,     0,   185,   180,     0,     0,
       0,   189,     0,     0,     0,     0,     0,     0,     0,   216,
     257,     0,     0,    11,     0,     5,     0,    21,     0,    29,
      27,     0,    25,     0,     0,     0,     0,    59,     0,     0,
      83,     0,   174,   170,   169,     0,   171,   173,     0,   175,
       0,     0,     0,     0,   154,   131,     0,   136,     0,   163,
       0,   119,     0,   110,     0,     0,     0,   156,     0,   132,
       0,   137,     0,     0,     0,   167,     0,     0,   125,   130,
       0,     0,     0,     0,   203,   190,     0,   194,     0,   208,
       0,   181,     0,     0,     0,   205,   191,     0,   195,     0,
     212,     0,   258,     0,   260,     0,    12,     0,    16,    23,
       0,    30,    28,     0,    26,    41,    40,     0,    39,    60,
       0,    89,     0,     0,     0,   146,   149,   153,     0,     0,
       0,     0,   164,     0,   112,   147,     0,   150,     0,   155,
       0,   152,     0,     0,     0,     0,   168,   159,     0,     0,
     198,   200,   202,     0,     0,   209,     0,   199,   201,   204,
       0,     0,   213,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    84,     0,     0,    77,   134,   141,
     143,   139,   161,   145,   148,   151,   135,     0,   142,   144,
     140,     0,   160,   165,     0,   177,   192,   196,   206,   193,
     197,   210,   259,   262,   263,   261,    14,    15,    13,    22,
      31,     0,   172,   176,   162,   133,   138,   166,   157,   207,
     211,    32,    42,   158
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,  -212,
     -39,   529,   -84,   -48,  -211,   -82,  -212,  -212,  -212,  -212,
    -212,  -212,  -212,  -212,  -212,  -212
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    84,   277,   458,   464,   634,   632,   631,   741,   292,
     242,   295,   298,   152,   151,   120,   121,   123,   122,   583,
     501,   493,   515,   598,   517,   599,   743,   712,   734,   652,
     737,   666,   611,   520,   526,   739,   675,   740,   682,   453,
     118,   218,   141,   110,   150,   142,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint16 yytable[] =
{
     140,   184,   143,   149,   156,   158,   192,   197,   415,   417,
     581,   587,   226,   206,   207,   208,   209,   210,   211,   212,
     213,   369,   125,   126,   127,   128,   129,   130,   131,   214,
     595,   655,   325,   657,   659,   706,   710,   116,   713,   215,
     248,   319,   335,   111,   320,   221,   183,   370,   116,   191,
     196,   203,   336,   100,   220,   225,   124,   125,   126,   127,
     128,   129,   130,   131,   368,   255,   440,   232,   132,   133,
     134,   135,   136,   137,   138,   206,   207,   208,   209,   210,
     211,   212,   213,    95,   125,   126,   127,   128,   129,   130,
     131,   116,   411,    96,   436,   132,   133,   134,   135,   189,
     137,   138,   104,   105,   106,   107,   104,   105,   106,   107,
     234,   237,    97,   345,   116,   430,   510,   511,   185,   649,
     650,   116,   289,   290,   377,   346,   416,   418,   582,   588,
     186,   347,   216,   217,   249,   145,   146,   147,   119,   371,
     512,    98,   116,   651,   685,   686,   332,    99,   596,   656,
     326,   658,   660,   707,   711,   116,   714,   153,   256,   257,
     233,   315,   337,   316,   101,   188,   419,   420,   687,   102,
     688,   689,   103,   421,   154,   139,   441,   132,   133,   134,
     135,   189,   137,   138,   159,   412,   413,   437,   438,   160,
     367,   190,   373,   328,   690,   163,   321,   322,   323,   338,
     385,   313,   339,   235,   238,   312,   314,   324,   431,   161,
     162,   327,   329,   108,   334,   164,   378,   165,   109,   349,
     405,   174,   175,   348,   181,   125,   126,   127,   128,   129,
     130,   131,   380,   204,   205,   366,   166,   372,     1,   176,
     116,   379,   177,   384,   333,   178,   104,   105,   106,   107,
     182,   341,   342,   343,   116,   179,     2,   167,   168,   169,
     170,   180,   344,   404,   219,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   193,   442,   443,   187,
     112,   113,   114,   115,   444,   299,   300,   227,   132,   133,
     134,   135,   189,   137,   138,   222,   116,   194,   571,   572,
     228,   171,   195,   172,   173,   573,   117,   132,   133,   134,
     135,   189,   137,   138,   382,   239,   223,   601,   602,   396,
     397,   224,   423,   424,   603,   243,   132,   133,   134,   135,
     189,   137,   138,   402,   425,   426,   198,   199,   200,   201,
     383,   145,   146,   147,   229,   132,   133,   134,   135,   189,
     137,   138,   116,   230,   104,   105,   106,   107,   241,   403,
     155,   231,   202,   125,   126,   127,   128,   129,   130,   131,
     236,   144,   145,   146,   147,   495,   496,   497,   116,   116,
     522,   523,   524,   148,   427,   428,   498,   240,   365,   447,
     448,   525,   104,   105,   106,   107,   456,   457,   157,   449,
     450,   244,   451,   452,   460,   461,   466,   467,   541,   542,
     575,   576,   577,   578,   579,   580,   247,   245,   605,   606,
     607,   608,   250,   609,   610,   622,   623,   624,   625,   626,
     627,   629,   630,   251,   246,   283,   253,   252,   254,   258,
     259,   260,   261,   262,   296,   263,   264,   265,   266,   297,
     306,   267,   307,   308,   268,   269,   270,   271,   272,   273,
     274,   309,   275,   311,   276,   331,   279,   278,   280,   281,
     282,   340,   350,   284,   285,   351,   352,   302,   286,   287,
     288,   291,   353,   293,   294,   354,   355,   318,   301,   304,
     303,   305,   317,   310,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   374,   375,   376,   381,   386,   387,   388,
     389,   390,   391,   392,   393,   394,   395,   398,   399,   400,
     401,   406,   407,   408,   409,   410,   414,   422,   429,   434,
     432,   445,   433,   435,   446,   439,   454,   463,   455,   459,
     462,   465,   468,   479,   469,   471,   470,   473,   474,   476,
     475,   472,   480,   481,   491,   478,   477,   483,   482,   494,
     500,   488,   484,   486,   485,   487,   502,   531,   489,   490,
     499,   492,   508,   514,   516,   518,   503,   521,   504,   505,
     506,   507,   509,   513,   527,   544,   534,   519,   536,   538,
     539,   540,   543,   545,   547,   548,   549,   550,   528,   551,
     529,   530,   552,   557,   561,   532,   562,   553,   533,   563,
     535,   565,   564,   537,   567,   566,   569,   589,   591,   546,
     615,   616,   618,   620,   628,   635,   715,   636,   637,   638,
     639,   640,   643,   644,   648,   653,   554,   555,   584,   677,
     678,   559,   556,   558,   560,   568,   570,   574,   585,   586,
     590,   592,   654,   661,   667,   673,   593,   594,   597,   600,
     604,   612,   613,   617,   674,   676,   679,   614,   684,   691,
     703,   704,   705,   619,   621,   633,   641,   642,   645,   708,
     709,   719,   646,   720,   721,   730,   732,   733,   735,   662,
     647,   736,   663,   738,   670,   742,   664,   665,   668,   669,
       0,     0,     0,     0,     0,     0,     0,     0,   671,   680,
     330,   672,   694,   681,   683,     0,     0,   692,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   693,     0,     0,
       0,     0,     0,   696,   695,     0,     0,     0,     0,     0,
     697,   698,   699,   700,   701,   702,     0,     0,     0,     0,
       0,   716,     0,     0,   717,   718,   722,   723,   724,     0,
       0,   725,   726,     0,     0,     0,     0,     0,   727,   728,
     729,     0,     0,     0,   731
};

static const yytype_int16 yycheck[] =
{
      39,    85,    41,    42,    52,    53,    88,    89,     3,     3,
       3,     3,    94,     3,     4,     5,     6,     7,     8,     9,
      10,     3,     4,     5,     6,     7,     8,     9,    10,    19,
       3,     3,     3,     3,     3,     3,     3,    19,     3,    29,
      29,   120,   253,   123,   123,    93,    85,    29,    19,    88,
      89,    90,    11,    11,    93,    94,     3,     4,     5,     6,
       7,     8,     9,    10,   275,    29,    11,    29,    15,    16,
      17,    18,    19,    20,    21,     3,     4,     5,     6,     7,
       8,     9,    10,   127,     4,     5,     6,     7,     8,     9,
      10,    19,    29,   127,    29,    15,    16,    17,    18,    19,
      20,    21,    11,    12,    13,    14,    11,    12,    13,    14,
      29,    29,   127,    11,    19,    29,     5,     6,    11,     5,
       6,    19,   122,   123,    29,    23,   121,   121,   121,   121,
      23,    29,   122,   123,   123,    12,    13,    14,     4,   121,
      29,   127,    19,    29,     5,     6,    23,   127,   121,   121,
     121,   121,   121,   121,   121,    19,   121,     4,   122,   123,
     122,   245,   121,   245,   122,     3,     5,     6,    29,   127,
       5,     6,   127,    12,    24,   122,   121,    15,    16,    17,
      18,    19,    20,    21,     3,   122,   123,   122,   123,     3,
     274,    29,   276,   121,    29,     3,    12,    13,    14,   120,
     282,   121,   123,   122,   122,   244,   245,    23,   122,    11,
      12,   250,   251,   122,   253,     3,   121,     3,   127,   258,
     302,    11,    12,   121,     3,     4,     5,     6,     7,     8,
       9,    10,   280,    11,    12,   274,    11,   276,    12,     3,
      19,   280,    11,   282,   121,     3,    11,    12,    13,    14,
      29,    12,    13,    14,    19,    11,    30,    25,    26,    27,
      28,     0,    23,   302,    29,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,     3,     5,     6,    24,
       3,     4,     5,     6,    12,   122,   123,    29,    15,    16,
      17,    18,    19,    20,    21,     3,    19,    24,     5,     6,
      29,     3,    29,     5,     6,    12,    29,    15,    16,    17,
      18,    19,    20,    21,     3,   122,    24,     5,     6,   122,
     123,    29,   124,   125,    12,   122,    15,    16,    17,    18,
      19,    20,    21,     3,   124,   125,     3,     4,     5,     6,
      29,    12,    13,    14,    29,    15,    16,    17,    18,    19,
      20,    21,    19,    29,    11,    12,    13,    14,   128,    29,
      17,    29,    29,     4,     5,     6,     7,     8,     9,    10,
      29,    11,    12,    13,    14,    12,    13,    14,    19,    19,
      12,    13,    14,    23,   124,   125,    23,    29,    29,   124,
     125,    23,    11,    12,    13,    14,   122,   123,    17,   124,
     125,   120,   124,   125,   122,   123,   122,   123,    13,    14,
     124,   125,   124,   125,   124,   125,    29,   120,   124,   125,
     124,   125,   120,   124,   125,   124,   125,   124,   125,   124,
     125,   124,   125,   120,   128,   126,   120,   123,   120,   120,
     120,   120,   120,   120,    29,   120,   120,   120,   120,    14,
      29,   120,    29,    29,   120,   120,   120,   120,   120,   120,
     120,    29,   120,    29,   120,    14,   120,   122,   120,   120,
     120,    29,    29,   126,   123,    29,    29,   120,   126,   126,
     126,   126,    29,   122,   128,    29,    29,   120,   126,   123,
     126,   126,   122,   124,    29,    29,    29,    29,    29,    29,
      29,    29,    29,   120,    29,     3,   121,    29,    29,    14,
      29,    29,    29,    29,    14,    29,    29,   125,    29,    14,
      29,    29,    14,    29,   123,    29,    29,   124,    29,   125,
      29,   120,    29,    29,   124,    29,    29,   120,   122,    29,
     121,    29,    29,    29,   126,   123,   126,   120,   120,   120,
     124,   126,    14,    29,    14,   124,   126,   120,   125,    29,
      29,   120,   126,   123,   126,   126,    29,    11,   124,   120,
     120,   123,    29,    29,    29,    29,   124,    29,   124,   124,
     120,   120,   120,   120,    29,    14,    29,   123,    29,    29,
      29,    29,    29,    29,    29,    14,    29,    29,   124,    14,
     124,   124,    29,    29,    29,   120,    24,   127,   120,    29,
     120,    14,    29,   120,    24,    29,    24,     3,     3,   121,
      11,    11,    11,    11,    29,    29,     4,    29,    29,    29,
      29,    29,    29,    29,    29,    29,   127,   126,   120,    11,
      11,   125,   127,   126,   124,   126,   125,   124,   120,   120,
     124,   124,    29,    29,    29,    29,   124,   124,   124,   124,
     124,   120,   120,   124,    29,    29,    11,   120,    29,    29,
      29,    29,    29,   124,   124,   124,   124,   124,   124,     3,
       3,    11,   124,    11,    11,    29,    29,    24,    29,   120,
     124,    29,   120,    29,   124,    29,   120,   120,   120,   120,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   120,
     251,   124,   124,   120,   120,    -1,    -1,   126,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   120,    -1,    -1,
      -1,    -1,    -1,   120,   126,    -1,    -1,    -1,    -1,    -1,
     124,   124,   124,   124,   124,   124,    -1,    -1,    -1,    -1,
      -1,   124,    -1,    -1,   124,   124,   124,   124,   124,    -1,
      -1,   124,   124,    -1,    -1,    -1,    -1,    -1,   124,   124,
     124,    -1,    -1,    -1,   127
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    12,    30,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   130,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   127,   127,   127,   127,   127,
      11,   122,   127,   127,    11,    12,    13,    14,   122,   127,
     172,   123,     3,     4,     5,     6,    19,    29,   169,     4,
     144,   145,   147,   146,     3,     4,     5,     6,     7,     8,
       9,    10,    15,    16,    17,    18,    19,    20,    21,   122,
     169,   171,   174,   169,    11,    12,    13,    14,    23,   169,
     173,   143,   142,     4,    24,    17,   172,    17,   172,     3,
       3,    11,    12,     3,     3,     3,    11,    25,    26,    27,
      28,     3,     5,     6,    11,    12,     3,    11,     3,    11,
       0,     3,    29,   169,   171,    11,    23,    24,     3,    19,
      29,   169,   174,     3,    24,    29,   169,   174,     3,     4,
       5,     6,    29,   169,    11,    12,     3,     4,     5,     6,
       7,     8,     9,    10,    19,    29,   122,   123,   170,    29,
     169,   172,     3,    24,    29,   169,   174,    29,    29,    29,
      29,    29,    29,   122,    29,   122,    29,    29,   122,   122,
      29,   128,   139,   122,   120,   120,   128,    29,    29,   123,
     120,   120,   123,   120,   120,    29,   122,   123,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   131,   122,   120,
     120,   120,   120,   126,   126,   123,   126,   126,   126,   122,
     123,   126,   138,   122,   128,   140,    29,    14,   141,   122,
     123,   126,   120,   126,   123,   126,    29,    29,    29,    29,
     124,    29,   169,   121,   169,   171,   174,   122,   120,   120,
     123,    12,    13,    14,    23,     3,   121,   169,   121,   169,
     170,    14,    23,   121,   169,   173,    11,   121,   120,   123,
      29,    12,    13,    14,    23,    11,    23,    29,   121,   169,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,   169,   171,   173,     3,
      29,   121,   169,   171,   120,    29,     3,    29,   121,   169,
     172,   121,     3,    29,   169,   174,    29,    29,    14,    29,
      29,    29,    29,    14,    29,    29,   122,   123,   125,    29,
      14,    29,     3,    29,   169,   174,    29,    14,    29,   123,
      29,    29,   122,   123,    29,     3,   121,     3,   121,     5,
       6,    12,   124,   124,   125,   124,   125,   124,   125,    29,
      29,   122,    29,    29,   125,    29,    29,   122,   123,    29,
      11,   121,     5,     6,    12,   120,   124,   124,   125,   124,
     125,   124,   125,   168,    29,   122,   122,   123,   132,    29,
     122,   123,   121,   120,   133,    29,   122,   123,    29,   126,
     126,   123,   126,   120,   120,   124,   120,   126,   124,    29,
      14,    29,   125,   120,   126,   126,   123,   126,   120,   124,
     120,    14,   123,   150,    29,    12,    13,    14,    23,   120,
      29,   149,    29,   124,   124,   124,   120,   120,    29,   120,
       5,     6,    29,   120,    29,   151,    29,   153,    29,   123,
     162,    29,    12,    13,    14,    23,   163,    29,   124,   124,
     124,    11,   120,   120,    29,   120,    29,   120,    29,    29,
      29,    13,    14,    29,    14,    29,   121,    29,    14,    29,
      29,    14,    29,   127,   127,   126,   127,    29,   126,   125,
     124,    29,    24,    29,    29,    14,    29,    24,   126,    24,
     125,     5,     6,    12,   124,   124,   125,   124,   125,   124,
     125,     3,   121,   148,   120,   120,   120,     3,   121,     3,
     124,     3,   124,   124,   124,     3,   121,   124,   152,   154,
     124,     5,     6,    12,   124,   124,   125,   124,   125,   124,
     125,   161,   120,   120,   120,    11,    11,   124,    11,   124,
      11,   124,   124,   125,   124,   125,   124,   125,    29,   124,
     125,   136,   135,   124,   134,    29,    29,    29,    29,    29,
      29,   124,   124,    29,    29,   124,   124,   124,    29,     5,
       6,    29,   158,    29,    29,     3,   121,     3,   121,     3,
     121,    29,   120,   120,   120,   120,   160,    29,   120,   120,
     124,   124,   124,    29,    29,   165,    29,    11,    11,    11,
     120,   120,   167,   120,    29,     5,     6,    29,     5,     6,
      29,    29,   126,   120,   124,   126,   120,   124,   124,   124,
     124,   124,   124,    29,    29,    29,     3,   121,     3,     3,
       3,   121,   156,     3,   121,     4,   124,   124,   124,    11,
      11,    11,   124,   124,   124,   124,   124,   124,   124,   124,
      29,   127,    29,    24,   157,    29,    29,   159,    29,   164,
     166,   137,    29,   155
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   129,   130,   130,   131,   130,   130,   130,   130,   132,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   133,
     130,   130,   130,   130,   130,   134,   130,   135,   130,   136,
     130,   137,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   138,   130,   130,   130,   130,
     130,   130,   130,   130,   139,   130,   130,   130,   130,   140,
     130,   130,   130,   130,   130,   130,   141,   130,   130,   130,
     130,   130,   142,   130,   143,   130,   130,   144,   130,   145,
     130,   146,   130,   147,   130,   130,   130,   130,   130,   148,
     130,   130,   130,   130,   130,   149,   130,   130,   130,   130,
     130,   150,   130,   151,   152,   130,   130,   130,   153,   154,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   155,   130,   156,
     130,   157,   130,   158,   130,   159,   130,   160,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     161,   130,   162,   130,   163,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   164,   130,   165,   130,
     166,   130,   167,   130,   168,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     130,   130,   130,   130,   130,   130,   169,   169,   170,   170,
     170,   170,   170,   170,   170,   170,   171,   171,   171,   171,
     171,   171,   171,   172,   172,   172,   172,   173,   173,   173,
     174,   174,   174,   174,   174,   174,   174,   175,   175,   175,
     175,   175,   175,   175,   175,   176,   176,   176,   177,   177,
     177,   178,   178,   178,   179,   179,   180,   180,   181,   181,
     182,   182,   183,   183,   184
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     5,     0,     6,     4,     4,     4,     0,
       5,     6,     7,     9,     9,     9,     7,     5,     4,     0,
       5,     6,     9,     7,     5,     0,     7,     0,     7,     0,
       7,     0,    10,     3,     3,     3,     3,     3,     3,     7,
       7,     7,    10,     2,     3,     4,     3,     4,     1,     1,
       2,     4,     3,     4,     4,     2,     4,     4,     4,     6,
       7,     2,     2,     2,     2,     0,     3,     4,     2,     2,
       2,     2,     2,     2,     0,     3,     4,     9,     2,     0,
       3,     3,     5,     6,     8,     2,     0,     3,     4,     7,
       1,     1,     0,     2,     0,     2,     2,     0,     2,     0,
       2,     0,     2,     0,     2,     1,     5,     5,     5,     0,
       7,     6,     8,     4,     4,     0,     6,     6,     5,     7,
       6,     0,     6,     0,     0,     7,     4,     4,     0,     0,
       7,     7,     7,    10,     9,     9,     7,     7,    10,     9,
       9,     9,     9,     9,     9,     9,     8,     8,     9,     8,
       8,     9,     8,     8,     7,     8,     7,     0,    11,     0,
       9,     0,    10,     0,     8,     0,    10,     0,     8,     6,
       6,     6,     9,     6,     6,     6,     9,     9,     5,     5,
       0,     7,     0,     6,     0,     6,     4,     4,     6,     6,
       7,     7,     9,     9,     7,     7,     9,     9,     8,     8,
       8,     8,     8,     7,     8,     7,     0,    10,     0,     8,
       0,    10,     0,     8,     0,     5,     6,     5,     4,     4,
       4,     1,     2,     2,     2,     2,     2,     1,     1,     1,
       4,     4,     4,     4,     4,     4,     4,     4,     2,     2,
       2,     2,     4,     4,     4,     4,     4,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     4,     6,     7,     9,
       7,     9,     9,     9,     4,     4,     0,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 188 "./config/rl78-parse.y" /* yacc.c:1646  */
    { as_bad (_("Unknown opcode: %s"), rl78_init_start); }
#line 2100 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 3:
#line 209 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0c|(yyvsp[-4].regno)); O1 ((yyvsp[0].exp)); }
#line 2106 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 4:
#line 211 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2112 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 5:
#line 212 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0a|(yyvsp[-5].regno)); SET_SA ((yyvsp[-4].exp)); O1 ((yyvsp[-4].exp)); O1 ((yyvsp[0].exp)); }
#line 2118 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 6:
#line 215 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x01|(yyvsp[-3].regno)); }
#line 2124 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 7:
#line 218 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x08|(yyvsp[-3].regno)); F ((yyvsp[0].regno), 13, 3); }
#line 2130 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 8:
#line 221 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x00|(yyvsp[-3].regno)); F ((yyvsp[-2].regno), 13, 3); }
#line 2136 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 9:
#line 223 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2142 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 224 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0b|(yyvsp[-4].regno)); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2148 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 227 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0f|(yyvsp[-5].regno)); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2154 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 230 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0d|(yyvsp[-6].regno)); }
#line 2160 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 233 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x0e|(yyvsp[-8].regno)); O1 ((yyvsp[-1].exp)); }
#line 2166 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 236 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x80|(yyvsp[-8].regno)); }
#line 2172 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 239 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x82|(yyvsp[-8].regno)); }
#line 2178 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 242 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-6].regno) != 0x40)
	      { rl78_error ("Only CMP takes these operands"); }
	    else
	      { B1 (0x00|(yyvsp[-6].regno)); O2 ((yyvsp[-3].exp)); O1 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
	  }
#line 2188 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 251 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x04|(yyvsp[-4].regno)); O2 ((yyvsp[0].exp)); }
#line 2194 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 254 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x01|(yyvsp[-3].regno)); F ((yyvsp[0].regno), 5, 2); }
#line 2200 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 256 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2206 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 257 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x06|(yyvsp[-4].regno)); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2212 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 260 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x02|(yyvsp[-5].regno)); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2218 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 263 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x09|(yyvsp[-8].regno)); O1 ((yyvsp[-1].exp)); }
#line 2224 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 266 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x61, 0x09|(yyvsp[-6].regno), 0); }
#line 2230 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 24:
#line 269 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 ((yyvsp[-4].regno) ? 0x20 : 0x10); O1 ((yyvsp[0].exp));
	    if ((yyvsp[-4].regno) == 0x40)
	      rl78_error ("CMPW SP,#imm not allowed");
	  }
#line 2239 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 276 "./config/rl78-parse.y" /* yacc.c:1646  */
    {Bit((yyvsp[0].exp))}
#line 2245 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 26:
#line 277 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x08|(yyvsp[-6].regno), (yyvsp[-3].regno)); FE ((yyvsp[-1].exp), 9, 3); }
#line 2251 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 279 "./config/rl78-parse.y" /* yacc.c:1646  */
    {Bit((yyvsp[0].exp))}
#line 2257 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 28:
#line 280 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[-3].exp)))
	      { B2 (0x71, 0x08|(yyvsp[-6].regno)); FE ((yyvsp[-1].exp), 9, 3); O1 ((yyvsp[-3].exp)); }
	    else if (expr_is_saddr ((yyvsp[-3].exp)))
	      { B2 (0x71, 0x00|(yyvsp[-6].regno)); FE ((yyvsp[-1].exp), 9, 3); SET_SA ((yyvsp[-3].exp)); O1 ((yyvsp[-3].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2269 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 288 "./config/rl78-parse.y" /* yacc.c:1646  */
    {Bit((yyvsp[0].exp))}
#line 2275 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 289 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x88|(yyvsp[-6].regno));  FE ((yyvsp[-1].exp), 9, 3); }
#line 2281 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 31:
#line 291 "./config/rl78-parse.y" /* yacc.c:1646  */
    {Bit((yyvsp[0].exp))}
#line 2287 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 292 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x80|(yyvsp[-9].regno));  FE ((yyvsp[-1].exp), 9, 3); }
#line 2293 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 297 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xdc); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2299 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 300 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xde); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2305 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 303 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xdd); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2311 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 306 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xdf); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2317 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 309 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xc3); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2323 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 312 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xd3); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2329 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 317 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x31, 0x80|(yyvsp[-6].regno), (yyvsp[-5].regno)); FE ((yyvsp[-3].exp), 9, 3); PC1 ((yyvsp[0].exp)); }
#line 2335 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 320 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[-5].exp)))
	      { B2 (0x31, 0x80|(yyvsp[-6].regno)); FE ((yyvsp[-3].exp), 9, 3); O1 ((yyvsp[-5].exp)); PC1 ((yyvsp[0].exp)); }
	    else if (expr_is_saddr ((yyvsp[-5].exp)))
	      { B2 (0x31, 0x00|(yyvsp[-6].regno)); FE ((yyvsp[-3].exp), 9, 3); SET_SA ((yyvsp[-5].exp)); O1 ((yyvsp[-5].exp)); PC1 ((yyvsp[0].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2347 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 329 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x31, 0x01|(yyvsp[-6].regno)); FE ((yyvsp[-3].exp), 9, 3); PC1 ((yyvsp[0].exp)); }
#line 2353 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 332 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x31, 0x81|(yyvsp[-9].regno)); FE ((yyvsp[-3].exp), 9, 3); PC1 ((yyvsp[0].exp)); }
#line 2359 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 337 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xcb); }
#line 2365 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 340 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xef); PC1 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2371 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 343 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xee); PC2 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2377 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 346 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xed); O2 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2383 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 349 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xec); O3 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2389 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 354 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xcc); }
#line 2395 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 49:
#line 357 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xff); }
#line 2401 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 362 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xca); F ((yyvsp[0].regno), 10, 2); }
#line 2407 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 365 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xfe); PC2 ((yyvsp[0].exp)); }
#line 2413 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 368 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xfd); O2 ((yyvsp[0].exp)); }
#line 2419 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 371 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xfc); O3 ((yyvsp[0].exp)); rl78_linkrelax_branch (); }
#line 2425 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 374 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-1].exp).X_op != O_constant)
	      rl78_error ("CALLT requires a numeric address");
	    else
	      {
	        int i = (yyvsp[-1].exp).X_add_number;
		if (i < 0x80 || i > 0xbe)
		  rl78_error ("CALLT address not 0x80..0xbe");
		else if (i & 1)
		  rl78_error ("CALLT address not even");
		else
		  {
		    B2 (0x61, 0x84);
	    	    F ((i >> 1) & 7, 9, 3);
	    	    F ((i >> 4) & 7, 14, 2);
		  }
	      }
	  }
#line 2447 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 395 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, (yyvsp[-1].regno) ? 0x88 : 0x80); }
#line 2453 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 398 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x0a|(yyvsp[-3].regno), (yyvsp[-2].regno)); FE ((yyvsp[0].exp), 9, 3); }
#line 2459 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 401 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[-2].exp)))
	      { B2 (0x71, 0x0a|(yyvsp[-3].regno)); FE ((yyvsp[0].exp), 9, 3); O1 ((yyvsp[-2].exp)); }
	    else if (expr_is_saddr ((yyvsp[-2].exp)))
	      { B2 (0x71, 0x02|(yyvsp[-3].regno)); FE ((yyvsp[0].exp), 9, 3); SET_SA ((yyvsp[-2].exp)); O1 ((yyvsp[-2].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2471 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 410 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x8a|(yyvsp[-3].regno));  FE ((yyvsp[0].exp), 9, 3); }
#line 2477 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 59:
#line 413 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x00+(yyvsp[-5].regno)*0x08); FE ((yyvsp[0].exp), 9, 3); O2 ((yyvsp[-2].exp)); rl78_linkrelax_addr16 (); }
#line 2483 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 416 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x82|(yyvsp[-6].regno)); FE ((yyvsp[0].exp), 9, 3); }
#line 2489 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 421 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe1|(yyvsp[-1].regno)); }
#line 2495 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 62:
#line 423 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe0|(yyvsp[-1].regno)); }
#line 2501 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 63:
#line 425 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe3|(yyvsp[-1].regno)); }
#line 2507 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 427 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe2|(yyvsp[-1].regno)); }
#line 2513 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 429 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2519 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 430 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe4|(yyvsp[-2].regno)); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2525 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 67:
#line 433 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe5|(yyvsp[-3].regno)); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2531 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 438 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe6|(yyvsp[-1].regno)); }
#line 2537 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 440 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xe7|(yyvsp[-1].regno)); }
#line 2543 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 445 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd1); }
#line 2549 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 448 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd0); }
#line 2555 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 451 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd3); }
#line 2561 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 454 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd2); }
#line 2567 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 456 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2573 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 457 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd4); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2579 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 460 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd5); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2585 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 465 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xde); O1 ((yyvsp[-1].exp)); }
#line 2591 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 470 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x80|(yyvsp[-1].regno)); F ((yyvsp[0].regno), 5, 3); }
#line 2597 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 472 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2603 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 473 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa4|(yyvsp[-2].regno)); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2609 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 475 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa0|(yyvsp[-2].regno)); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2615 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 477 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x11, 0xa0|(yyvsp[-4].regno)); O2 ((yyvsp[0].exp)); }
#line 2621 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 479 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x59+(yyvsp[-5].regno)); O1 ((yyvsp[-1].exp)); }
#line 2627 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 481 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x11, 0x61, 0x59+(yyvsp[-7].regno)); O1 ((yyvsp[-1].exp)); }
#line 2633 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 486 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa1|(yyvsp[-1].regno)); F ((yyvsp[0].regno), 5, 2); }
#line 2639 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 488 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2645 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 489 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa6|(yyvsp[-2].regno)); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
#line 2651 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 492 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa2|(yyvsp[-3].regno)); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2657 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 495 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0x79+(yyvsp[-6].regno)); O1 ((yyvsp[-1].exp)); }
#line 2663 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 500 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x7b, 0xfa); }
#line 2669 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 503 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x7a, 0xfa); }
#line 2675 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 507 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("MULHU"); }
#line 2681 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 508 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x01); }
#line 2687 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 94:
#line 510 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("MULH"); }
#line 2693 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 511 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x02); }
#line 2699 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 96:
#line 514 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd6); }
#line 2705 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 97:
#line 516 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("DIVHU"); }
#line 2711 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 98:
#line 517 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x03); }
#line 2717 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 99:
#line 524 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("DIVWU"); }
#line 2723 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 100:
#line 525 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x0b); }
#line 2729 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 101:
#line 527 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("MACHU"); }
#line 2735 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 102:
#line 528 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x05); }
#line 2741 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 103:
#line 530 "./config/rl78-parse.y" /* yacc.c:1646  */
    { ISA_G14 ("MACH"); }
#line 2747 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 104:
#line 531 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xce, 0xfb, 0x06); }
#line 2753 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 105:
#line 536 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xed); }
#line 2759 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 106:
#line 544 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x51); O1 ((yyvsp[0].exp)); }
#line 2765 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 107:
#line 546 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x50); F((yyvsp[-3].regno), 5, 3); O1 ((yyvsp[0].exp)); }
#line 2771 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 108:
#line 549 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-3].regno) != 0xfd)
	      { B2 (0xce, (yyvsp[-3].regno)); O1 ((yyvsp[0].exp)); }
	    else
	      { B1 (0x41); O1 ((yyvsp[0].exp)); }
	  }
#line 2781 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 109:
#line 555 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 2787 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 110:
#line 556 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[-4].exp)))
	      { B1 (0xce); O1 ((yyvsp[-4].exp)); O1 ((yyvsp[-1].exp)); }
	    else if (expr_is_saddr ((yyvsp[-4].exp)))
	      { B1 (0xcd); SET_SA ((yyvsp[-4].exp)); O1 ((yyvsp[-4].exp)); O1 ((yyvsp[-1].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2799 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 111:
#line 565 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xcf); O2 ((yyvsp[-3].exp)); O1 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2805 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 112:
#line 568 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x11, 0xcf); O2 ((yyvsp[-3].exp)); O1 ((yyvsp[0].exp)); }
#line 2811 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 113:
#line 571 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x70); F ((yyvsp[-2].regno), 5, 3); }
#line 2817 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 114:
#line 574 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x60); F ((yyvsp[0].regno), 5, 3); }
#line 2823 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 115:
#line 576 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 2829 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 116:
#line 577 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[-3].exp)))
	      { B1 (0x9e); O1 ((yyvsp[-3].exp)); }
	    else if (expr_is_saddr ((yyvsp[-3].exp)))
	      { B1 (0x9d); SET_SA ((yyvsp[-3].exp)); O1 ((yyvsp[-3].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2841 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 117:
#line 586 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x8f); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2847 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 118:
#line 589 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x9f); O2 ((yyvsp[-2].exp)); rl78_linkrelax_addr16 (); }
#line 2853 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 119:
#line 592 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x11, 0x9f); O2 ((yyvsp[-2].exp)); }
#line 2859 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 120:
#line 595 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xc9|reg_xbc((yyvsp[-4].regno))); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 2865 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 121:
#line 597 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 2871 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 122:
#line 598 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-1].exp)))
	      { B1 (0x8d); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); }
	    else if (expr_is_sfr ((yyvsp[-1].exp)))
	      { B1 (0x8e); O1 ((yyvsp[-1].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 2883 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 123:
#line 606 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2889 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 124:
#line 606 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 2895 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 125:
#line 607 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xc8|reg_xbc((yyvsp[-5].regno))); SET_SA ((yyvsp[-2].exp)); O1 ((yyvsp[-2].exp)); }
#line 2901 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 126:
#line 610 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x8e, (yyvsp[0].regno)); }
#line 2907 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 127:
#line 613 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) != 1)
	      rl78_error ("Only A allowed here");
	    else
	      { B2 (0x9e, (yyvsp[-2].regno)); }
	  }
#line 2917 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 128:
#line 619 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 2923 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 129:
#line 619 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 2929 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 130:
#line 620 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[-5].regno) != 0xfd)
	      rl78_error ("Only ES allowed here");
	    else
	      { B2 (0x61, 0xb8); SET_SA ((yyvsp[-2].exp)); O1 ((yyvsp[-2].exp)); }
	  }
#line 2939 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 131:
#line 627 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x89); }
#line 2945 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 132:
#line 630 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x99); }
#line 2951 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 133:
#line 633 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xca); O1 ((yyvsp[-4].exp)); O1 ((yyvsp[0].exp)); }
#line 2957 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 134:
#line 636 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x8a); O1 ((yyvsp[-1].exp)); }
#line 2963 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 135:
#line 639 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x9a); O1 ((yyvsp[-3].exp)); }
#line 2969 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 136:
#line 642 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x8b); }
#line 2975 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 137:
#line 645 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x9b); }
#line 2981 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 138:
#line 648 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xcc); O1 ((yyvsp[-4].exp)); O1 ((yyvsp[0].exp)); }
#line 2987 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 139:
#line 651 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x8c); O1 ((yyvsp[-1].exp)); }
#line 2993 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 140:
#line 654 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x9c); O1 ((yyvsp[-3].exp)); }
#line 2999 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 141:
#line 657 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xc9); }
#line 3005 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 142:
#line 660 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xd9); }
#line 3011 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 143:
#line 663 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xe9); }
#line 3017 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 144:
#line 666 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xf9); }
#line 3023 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 145:
#line 669 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x19); O2 ((yyvsp[-6].exp)); O1 ((yyvsp[0].exp)); }
#line 3029 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 146:
#line 672 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x09); O2 ((yyvsp[-3].exp)); }
#line 3035 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 147:
#line 675 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x18); O2 ((yyvsp[-5].exp)); }
#line 3041 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 148:
#line 678 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x38); O2 ((yyvsp[-6].exp)); O1 ((yyvsp[0].exp)); }
#line 3047 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 149:
#line 681 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x29); O2 ((yyvsp[-3].exp)); }
#line 3053 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 150:
#line 684 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x28); O2 ((yyvsp[-5].exp)); }
#line 3059 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 151:
#line 687 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x39); O2 ((yyvsp[-6].exp)); O1 ((yyvsp[0].exp)); }
#line 3065 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 152:
#line 690 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x39, 0, 0); O1 ((yyvsp[0].exp)); }
#line 3071 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 153:
#line 693 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x49); O2 ((yyvsp[-3].exp)); }
#line 3077 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 154:
#line 696 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x49, 0, 0); }
#line 3083 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 155:
#line 699 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x48); O2 ((yyvsp[-5].exp)); }
#line 3089 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 156:
#line 702 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x48, 0, 0); }
#line 3095 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 157:
#line 704 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3101 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 158:
#line 705 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xc8); O1 ((yyvsp[-5].exp)); O1 ((yyvsp[-1].exp)); }
#line 3107 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 159:
#line 707 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3113 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 160:
#line 708 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xc8, 0); O1 ((yyvsp[-1].exp)); }
#line 3119 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 161:
#line 710 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3125 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 162:
#line 711 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x88); O1 ((yyvsp[-2].exp)); }
#line 3131 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 163:
#line 713 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3137 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 164:
#line 714 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x88, 0); }
#line 3143 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 165:
#line 716 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3149 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 166:
#line 717 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x98); O1 ((yyvsp[-4].exp)); }
#line 3155 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 167:
#line 719 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3161 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 168:
#line 720 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x98, 0); }
#line 3167 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 169:
#line 725 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-2].exp)))
	      { B2 (0x71, 0x04); FE ((yyvsp[0].exp), 9, 3); SET_SA ((yyvsp[-2].exp)); O1 ((yyvsp[-2].exp)); }
	    else if (expr_is_sfr ((yyvsp[-2].exp)))
	      { B2 (0x71, 0x0c); FE ((yyvsp[0].exp), 9, 3); O1 ((yyvsp[-2].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3179 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 170:
#line 734 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x8c); FE ((yyvsp[0].exp), 9, 3); }
#line 3185 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 171:
#line 737 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x0c, (yyvsp[-2].regno)); FE ((yyvsp[0].exp), 9, 3); }
#line 3191 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 172:
#line 740 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x84); FE ((yyvsp[0].exp), 9, 3); }
#line 3197 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 173:
#line 743 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-4].exp)))
	      { B2 (0x71, 0x01); FE ((yyvsp[-2].exp), 9, 3); SET_SA ((yyvsp[-4].exp)); O1 ((yyvsp[-4].exp)); }
	    else if (expr_is_sfr ((yyvsp[-4].exp)))
	      { B2 (0x71, 0x09); FE ((yyvsp[-2].exp), 9, 3); O1 ((yyvsp[-4].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3209 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 174:
#line 752 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x89); FE ((yyvsp[-2].exp), 9, 3); }
#line 3215 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 175:
#line 755 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x71, 0x09, (yyvsp[-4].regno)); FE ((yyvsp[-2].exp), 9, 3); }
#line 3221 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 176:
#line 758 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0x81); FE ((yyvsp[-2].exp), 9, 3); }
#line 3227 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 177:
#line 763 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xce); O1 ((yyvsp[-3].exp)); }
#line 3233 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 178:
#line 768 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x30); O2 ((yyvsp[0].exp)); }
#line 3239 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 179:
#line 771 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x30); F ((yyvsp[-3].regno), 5, 2); O2 ((yyvsp[0].exp)); }
#line 3245 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 180:
#line 773 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3251 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 181:
#line 774 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-4].exp)))
	      { B1 (0xc9); SET_SA ((yyvsp[-4].exp)); O1 ((yyvsp[-4].exp)); O2 ((yyvsp[-1].exp)); }
	    else if (expr_is_sfr ((yyvsp[-4].exp)))
	      { B1 (0xcb); O1 ((yyvsp[-4].exp)); O2 ((yyvsp[-1].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3263 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 182:
#line 782 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3269 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 183:
#line 783 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-1].exp)))
	      { B1 (0xad); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); WA((yyvsp[-1].exp)); }
	    else if (expr_is_sfr ((yyvsp[-1].exp)))
	      { B1 (0xae); O1 ((yyvsp[-1].exp)); WA((yyvsp[-1].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3281 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 184:
#line 791 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3287 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 185:
#line 792 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_saddr ((yyvsp[-3].exp)))
	      { B1 (0xbd); SET_SA ((yyvsp[-3].exp)); O1 ((yyvsp[-3].exp)); WA((yyvsp[-3].exp)); }
	    else if (expr_is_sfr ((yyvsp[-3].exp)))
	      { B1 (0xbe); O1 ((yyvsp[-3].exp)); WA((yyvsp[-3].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3299 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 186:
#line 801 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x11); F ((yyvsp[0].regno), 5, 2); }
#line 3305 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 187:
#line 804 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x10); F ((yyvsp[-2].regno), 5, 2); }
#line 3311 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 188:
#line 807 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xaf); O2 ((yyvsp[0].exp)); WA((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 3317 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 189:
#line 810 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xbf); O2 ((yyvsp[-2].exp)); WA((yyvsp[-2].exp)); rl78_linkrelax_addr16 (); }
#line 3323 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 190:
#line 813 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa9); }
#line 3329 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 191:
#line 816 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xb9); }
#line 3335 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 192:
#line 819 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xaa); O1 ((yyvsp[-1].exp)); }
#line 3341 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 193:
#line 822 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xba); O1 ((yyvsp[-3].exp)); }
#line 3347 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 194:
#line 825 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xab); }
#line 3353 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 195:
#line 828 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xbb); }
#line 3359 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 196:
#line 831 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xac); O1 ((yyvsp[-1].exp)); }
#line 3365 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 197:
#line 834 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xbc); O1 ((yyvsp[-3].exp)); }
#line 3371 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 198:
#line 837 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x59); O2 ((yyvsp[-3].exp)); }
#line 3377 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 199:
#line 840 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x58); O2 ((yyvsp[-5].exp)); }
#line 3383 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 200:
#line 843 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x69); O2 ((yyvsp[-3].exp)); }
#line 3389 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 201:
#line 846 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x68); O2 ((yyvsp[-5].exp)); }
#line 3395 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 202:
#line 849 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x79); O2 ((yyvsp[-3].exp)); }
#line 3401 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 203:
#line 852 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x79, 0, 0); }
#line 3407 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 204:
#line 855 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x78); O2 ((yyvsp[-5].exp)); }
#line 3413 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 205:
#line 858 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0x78, 0, 0); }
#line 3419 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 206:
#line 860 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3425 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 207:
#line 861 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xa8); O1 ((yyvsp[-2].exp));  WA((yyvsp[-2].exp));}
#line 3431 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 208:
#line 863 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3437 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 209:
#line 864 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xa8, 0); }
#line 3443 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 210:
#line 866 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3449 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 211:
#line 867 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xb8); O1 ((yyvsp[-4].exp)); WA((yyvsp[-4].exp)); }
#line 3455 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 212:
#line 869 "./config/rl78-parse.y" /* yacc.c:1646  */
    {NOT_ES}
#line 3461 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 213:
#line 870 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xb8, 0); }
#line 3467 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 214:
#line 872 "./config/rl78-parse.y" /* yacc.c:1646  */
    {SA((yyvsp[0].exp))}
#line 3473 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 215:
#line 873 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xca); F ((yyvsp[-3].regno), 2, 2); SET_SA ((yyvsp[-1].exp)); O1 ((yyvsp[-1].exp)); WA((yyvsp[-1].exp)); }
#line 3479 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 216:
#line 876 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xcb); F ((yyvsp[-4].regno), 2, 2); O2 ((yyvsp[0].exp)); WA((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 3485 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 217:
#line 879 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xcb, 0xf8); O2 ((yyvsp[0].exp)); }
#line 3491 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 218:
#line 882 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xbe, 0xf8); }
#line 3497 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 219:
#line 885 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0xae, 0xf8); }
#line 3503 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 220:
#line 888 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B3 (0xcb, 0xf8, 0xff); F ((yyvsp[-2].regno), 2, 2); }
#line 3509 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 221:
#line 893 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x00); }
#line 3515 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 222:
#line 898 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x71, 0xc0); }
#line 3521 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 223:
#line 903 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xc0); F ((yyvsp[0].regno), 5, 2); }
#line 3527 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 224:
#line 906 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xcd); }
#line 3533 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 225:
#line 909 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xc1); F ((yyvsp[0].regno), 5, 2); }
#line 3539 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 226:
#line 912 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xdd); }
#line 3545 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 227:
#line 917 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0xd7); }
#line 3551 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 228:
#line 920 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xfc); }
#line 3557 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 229:
#line 923 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xec); }
#line 3563 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 230:
#line 928 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xeb); }
	  }
#line 3571 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 231:
#line 933 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xdc); }
	  }
#line 3579 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 232:
#line 938 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xee); }
	  }
#line 3587 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 233:
#line 943 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xfe); }
	  }
#line 3595 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 234:
#line 948 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xdb); }
	  }
#line 3603 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 235:
#line 953 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 1))
	      { B2 (0x61, 0xfb);}
	  }
#line 3611 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 236:
#line 960 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 7))
	      { B2 (0x31, 0x0b); FE ((yyvsp[0].exp), 9, 3); }
	  }
#line 3619 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 237:
#line 965 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 15))
	      { B2 (0x31, 0x0f); FE ((yyvsp[0].exp), 8, 4); }
	  }
#line 3627 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 238:
#line 972 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xcf); }
#line 3633 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 239:
#line 975 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xdf); }
#line 3639 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 240:
#line 978 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xef); }
#line 3645 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 241:
#line 981 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xff); }
#line 3651 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 242:
#line 986 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 7))
	      { B2 (0x31, 0x09); FE ((yyvsp[0].exp), 9, 3); }
	  }
#line 3659 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 243:
#line 991 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 7))
	      { B2 (0x31, 0x08); FE ((yyvsp[0].exp), 9, 3); }
	  }
#line 3667 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 244:
#line 996 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 7))
	      { B2 (0x31, 0x07); FE ((yyvsp[0].exp), 9, 3); }
	  }
#line 3675 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 245:
#line 1001 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 15))
	      { B2 (0x31, 0x0d); FE ((yyvsp[0].exp), 8, 4); }
	  }
#line 3683 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 246:
#line 1006 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 15))
	      { B2 (0x31, 0x0c); FE ((yyvsp[0].exp), 8, 4); }
	  }
#line 3691 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 247:
#line 1013 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 7))
	      { B2 (0x31, 0x0a); FE ((yyvsp[0].exp), 9, 3); }
	  }
#line 3699 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 248:
#line 1018 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (check_expr_is_const ((yyvsp[0].exp), 1, 15))
	      { B2 (0x31, 0x0e); FE ((yyvsp[0].exp), 8, 4); }
	  }
#line 3707 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 249:
#line 1025 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xc8); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3713 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 250:
#line 1028 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xe3); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3719 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 251:
#line 1031 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xd8); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3725 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 252:
#line 1034 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xf3); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3731 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 253:
#line 1037 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xf8); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3737 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 254:
#line 1040 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xe8); rl78_relax (RL78_RELAX_BRANCH, 0); }
#line 3743 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 255:
#line 1045 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xfd); }
#line 3749 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 256:
#line 1050 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if ((yyvsp[0].regno) == 0) /* X */
	      { B1 (0x08); }
	    else
	      { B2 (0x61, 0x88); F ((yyvsp[0].regno), 13, 3); }
	  }
#line 3759 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 257:
#line 1057 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xaa); O2 ((yyvsp[0].exp)); rl78_linkrelax_addr16 (); }
#line 3765 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 258:
#line 1060 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xae); }
#line 3771 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 259:
#line 1063 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xaf); O1 ((yyvsp[-1].exp)); }
#line 3777 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 260:
#line 1066 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xac); }
#line 3783 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 261:
#line 1069 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xad); O1 ((yyvsp[-1].exp)); }
#line 3789 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 262:
#line 1072 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xb9); }
#line 3795 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 263:
#line 1075 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B2 (0x61, 0xa9); }
#line 3801 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 264:
#line 1078 "./config/rl78-parse.y" /* yacc.c:1646  */
    { if (expr_is_sfr ((yyvsp[0].exp)))
	      { B2 (0x61, 0xab); O1 ((yyvsp[0].exp)); }
	    else if (expr_is_saddr ((yyvsp[0].exp)))
	      { B2 (0x61, 0xa8); SET_SA ((yyvsp[0].exp)); O1 ((yyvsp[0].exp)); }
	    else
	      NOT_SFR_OR_SADDR;
	  }
#line 3813 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1089 "./config/rl78-parse.y" /* yacc.c:1646  */
    { B1 (0x31); F ((yyvsp[0].regno), 5, 2); }
#line 3819 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1099 "./config/rl78-parse.y" /* yacc.c:1646  */
    { rl78_prefix (0x11); }
#line 3825 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 268:
#line 1102 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; }
#line 3831 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 269:
#line 1103 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3837 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 270:
#line 1104 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3843 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 271:
#line 1105 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 3; }
#line 3849 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 272:
#line 1106 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 4; }
#line 3855 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 273:
#line 1107 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 5; }
#line 3861 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 274:
#line 1108 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 6; }
#line 3867 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1109 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 7; }
#line 3873 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1112 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; }
#line 3879 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1113 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3885 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1114 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 3; }
#line 3891 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1115 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 4; }
#line 3897 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 280:
#line 1116 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 5; }
#line 3903 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1117 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 6; }
#line 3909 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1118 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 7; }
#line 3915 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1121 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; }
#line 3921 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1122 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3927 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1123 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3933 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1124 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 3; }
#line 3939 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1127 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; }
#line 3945 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1128 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 2; }
#line 3951 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1129 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 3; }
#line 3957 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1132 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xf8; }
#line 3963 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1133 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xf9; }
#line 3969 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 292:
#line 1134 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xfa; }
#line 3975 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 293:
#line 1135 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xfc; }
#line 3981 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 294:
#line 1136 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xfd; }
#line 3987 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 295:
#line 1137 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xfe; }
#line 3993 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 296:
#line 1138 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0xff; }
#line 3999 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 297:
#line 1144 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4005 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1145 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x10; }
#line 4011 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 299:
#line 1146 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x20; }
#line 4017 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 300:
#line 1147 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x30; }
#line 4023 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1148 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x40; }
#line 4029 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 302:
#line 1149 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x50; }
#line 4035 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 303:
#line 1150 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x60; }
#line 4041 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1151 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x70; }
#line 4047 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1154 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4053 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 306:
#line 1155 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x20; }
#line 4059 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 307:
#line 1156 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x40; }
#line 4065 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 308:
#line 1159 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x05; rl78_bit_insn = 1; }
#line 4071 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 309:
#line 1160 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x06; rl78_bit_insn = 1; }
#line 4077 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 310:
#line 1161 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x07; rl78_bit_insn = 1; }
#line 4083 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 311:
#line 1164 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x02;    rl78_bit_insn = 1; rl78_linkrelax_branch (); }
#line 4089 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 312:
#line 1165 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x04;    rl78_bit_insn = 1; rl78_linkrelax_branch (); }
#line 4095 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 313:
#line 1166 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; rl78_bit_insn = 1; }
#line 4101 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 314:
#line 1169 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0; rl78_bit_insn = 1; }
#line 4107 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 315:
#line 1170 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 1; rl78_bit_insn = 1; }
#line 4113 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 316:
#line 1173 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4119 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 317:
#line 1174 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x10; }
#line 4125 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 318:
#line 1177 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4131 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 319:
#line 1178 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x10; }
#line 4137 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 320:
#line 1181 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4143 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 321:
#line 1182 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x10; }
#line 4149 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 322:
#line 1185 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x00; }
#line 4155 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 323:
#line 1186 "./config/rl78-parse.y" /* yacc.c:1646  */
    { (yyval.regno) = 0x10; }
#line 4161 "rl78-parse.c" /* yacc.c:1646  */
    break;

  case 324:
#line 1189 "./config/rl78-parse.y" /* yacc.c:1646  */
    { rl78_bit_insn = 1; }
#line 4167 "rl78-parse.c" /* yacc.c:1646  */
    break;


#line 4171 "rl78-parse.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 1192 "./config/rl78-parse.y" /* yacc.c:1906  */

/* ====================================================================== */

static struct
{
  const char * string;
  int          token;
  int          val;
}
token_table[] =
{
  { "r0", X, 0 },
  { "r1", A, 1 },
  { "r2", C, 2 },
  { "r3", B, 3 },
  { "r4", E, 4 },
  { "r5", D, 5 },
  { "r6", L, 6 },
  { "r7", H, 7 },
  { "x", X, 0 },
  { "a", A, 1 },
  { "c", C, 2 },
  { "b", B, 3 },
  { "e", E, 4 },
  { "d", D, 5 },
  { "l", L, 6 },
  { "h", H, 7 },

  { "rp0", AX, 0 },
  { "rp1", BC, 1 },
  { "rp2", DE, 2 },
  { "rp3", HL, 3 },
  { "ax", AX, 0 },
  { "bc", BC, 1 },
  { "de", DE, 2 },
  { "hl", HL, 3 },

  { "RB0", RB0, 0 },
  { "RB1", RB1, 1 },
  { "RB2", RB2, 2 },
  { "RB3", RB3, 3 },

  { "sp", SP, 0 },
  { "cy", CY, 0 },

  { "spl", SPL, 0xf8 },
  { "sph", SPH, 0xf9 },
  { "psw", PSW, 0xfa },
  { "cs", CS, 0xfc },
  { "es", ES, 0xfd },
  { "pmc", PMC, 0xfe },
  { "mem", MEM, 0xff },

  { ".s", DOT_S, 0 },
  { ".b", DOT_B, 0 },
  { ".w", DOT_W, 0 },
  { ".l", DOT_L, 0 },
  { ".a", DOT_A , 0},
  { ".ub", DOT_UB, 0 },
  { ".uw", DOT_UW , 0},

  { "c", FLAG, 0 },
  { "z", FLAG, 1 },
  { "s", FLAG, 2 },
  { "o", FLAG, 3 },
  { "i", FLAG, 8 },
  { "u", FLAG, 9 },

#define OPC(x) { #x, x, IS_OPCODE }

  OPC(ADD),
  OPC(ADDC),
  OPC(ADDW),
  { "and", AND_, IS_OPCODE },
  OPC(AND1),
  OPC(BC),
  OPC(BF),
  OPC(BH),
  OPC(BNC),
  OPC(BNH),
  OPC(BNZ),
  OPC(BR),
  OPC(BRK),
  OPC(BRK1),
  OPC(BT),
  OPC(BTCLR),
  OPC(BZ),
  OPC(CALL),
  OPC(CALLT),
  OPC(CLR1),
  OPC(CLRB),
  OPC(CLRW),
  OPC(CMP),
  OPC(CMP0),
  OPC(CMPS),
  OPC(CMPW),
  OPC(DEC),
  OPC(DECW),
  OPC(DI),
  OPC(DIVHU),
  OPC(DIVWU),
  OPC(EI),
  OPC(HALT),
  OPC(INC),
  OPC(INCW),
  OPC(MACH),
  OPC(MACHU),
  OPC(MOV),
  OPC(MOV1),
  OPC(MOVS),
  OPC(MOVW),
  OPC(MULH),
  OPC(MULHU),
  OPC(MULU),
  OPC(NOP),
  OPC(NOT1),
  OPC(ONEB),
  OPC(ONEW),
  OPC(OR),
  OPC(OR1),
  OPC(POP),
  OPC(PUSH),
  OPC(RET),
  OPC(RETI),
  OPC(RETB),
  OPC(ROL),
  OPC(ROLC),
  OPC(ROLWC),
  OPC(ROR),
  OPC(RORC),
  OPC(SAR),
  OPC(SARW),
  OPC(SEL),
  OPC(SET1),
  OPC(SHL),
  OPC(SHLW),
  OPC(SHR),
  OPC(SHRW),
  OPC(SKC),
  OPC(SKH),
  OPC(SKNC),
  OPC(SKNH),
  OPC(SKNZ),
  OPC(SKZ),
  OPC(STOP),
  OPC(SUB),
  OPC(SUBC),
  OPC(SUBW),
  OPC(XCH),
  OPC(XCHW),
  OPC(XOR),
  OPC(XOR1),
};

#define NUM_TOKENS (sizeof (token_table) / sizeof (token_table[0]))

void
rl78_lex_init (char * beginning, char * ending)
{
  rl78_init_start = beginning;
  rl78_lex_start = beginning;
  rl78_lex_end = ending;
  rl78_in_brackets = 0;
  rl78_last_token = 0;

  rl78_bit_insn = 0;

  setbuf (stdout, 0);
}

/* Return a pointer to the '.' in a bit index expression (like
   foo.5), or NULL if none is found.  */
static char *
find_bit_index (char *tok)
{
  char *last_dot = NULL;
  char *last_digit = NULL;
  while (*tok && *tok != ',')
    {
      if (*tok == '.')
	{
	  last_dot = tok;
	  last_digit = NULL;
	}
      else if (*tok >= '0' && *tok <= '7'
	       && last_dot != NULL
	       && last_digit == NULL)
	{
	  last_digit = tok;
	}
      else if (ISSPACE (*tok))
	{
	  /* skip */
	}
      else
	{
	  last_dot = NULL;
	  last_digit = NULL;
	}
      tok ++;
    }
  if (last_dot != NULL
      && last_digit != NULL)
    return last_dot;
  return NULL;
}

static int
rl78_lex (void)
{
  /*unsigned int ci;*/
  char * save_input_pointer;
  char * bit = NULL;

  while (ISSPACE (*rl78_lex_start)
	 && rl78_lex_start != rl78_lex_end)
    rl78_lex_start ++;

  rl78_last_exp_start = rl78_lex_start;

  if (rl78_lex_start == rl78_lex_end)
    return 0;

  if (ISALPHA (*rl78_lex_start)
      || (*rl78_lex_start == '.' && ISALPHA (rl78_lex_start[1])))
    {
      unsigned int i;
      char * e;
      char save;

      for (e = rl78_lex_start + 1;
	   e < rl78_lex_end && ISALNUM (*e);
	   e ++)
	;
      save = *e;
      *e = 0;

      for (i = 0; i < NUM_TOKENS; i++)
	if (strcasecmp (rl78_lex_start, token_table[i].string) == 0
	    && !(token_table[i].val == IS_OPCODE && rl78_last_token != 0)
	    && !(token_table[i].token == FLAG && !need_flag))
	  {
	    rl78_lval.regno = token_table[i].val;
	    *e = save;
	    rl78_lex_start = e;
	    rl78_last_token = token_table[i].token;
	    return token_table[i].token;
	  }
      *e = save;
    }

  if (rl78_last_token == 0)
    {
      rl78_last_token = UNKNOWN_OPCODE;
      return UNKNOWN_OPCODE;
    }

  if (rl78_last_token == UNKNOWN_OPCODE)
    return 0;

  if (*rl78_lex_start == '[')
    rl78_in_brackets = 1;
  if (*rl78_lex_start == ']')
    rl78_in_brackets = 0;

  /* '.' is funny - the syntax includes it for bitfields, but only for
      bitfields.  We check for it specially so we can allow labels
      with '.' in them.  */

  if (rl78_bit_insn
      && *rl78_lex_start == '.'
      && find_bit_index (rl78_lex_start) == rl78_lex_start)
    {
      rl78_last_token = *rl78_lex_start;
      return *rl78_lex_start ++;
    }

  if ((rl78_in_brackets && *rl78_lex_start == '+')
      || strchr ("[],#!$:", *rl78_lex_start))
    {
      rl78_last_token = *rl78_lex_start;
      return *rl78_lex_start ++;
    }

  /* Again, '.' is funny.  Look for '.<digit>' at the end of the line
     or before a comma, which is a bitfield, not an expression.  */

  if (rl78_bit_insn)
    {
      bit = find_bit_index (rl78_lex_start);
      if (bit)
	*bit = 0;
      else
	bit = NULL;
    }

  save_input_pointer = input_line_pointer;
  input_line_pointer = rl78_lex_start;
  rl78_lval.exp.X_md = 0;
  expression (&rl78_lval.exp);

  if (bit)
    *bit = '.';

  rl78_lex_start = input_line_pointer;
  input_line_pointer = save_input_pointer;
  rl78_last_token = EXPR;
  return EXPR;
}

int
rl78_error (const char * str)
{
  int len;

  len = rl78_last_exp_start - rl78_init_start;

  as_bad ("%s", rl78_init_start);
  as_bad ("%*s^ %s", len, "", str);
  return 0;
}

static int
expr_is_sfr (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 0;

  v = exp.X_add_number;
  if (0xFFF00 <= v && v <= 0xFFFFF)
    return 1;
  return 0;
}

static int
expr_is_saddr (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 1;

  v = exp.X_add_number;
  if (0xFFE20 <= v && v <= 0xFFF1F)
    return 1;
  return 0;
}

static int
expr_is_word_aligned (expressionS exp)
{
  unsigned long v;

  if (exp.X_op != O_constant)
    return 1;

  v = exp.X_add_number;
  if (v & 1)
    return 0;
  return 1;

}

static void
check_expr_is_bit_index (expressionS exp)
{
  int val;

  if (exp.X_op != O_constant)
    {
      rl78_error (_("bit index must be a constant"));
      return;
    }
  val = exp.X_add_number;

  if (val < 0 || val > 7)
    rl78_error (_("rtsd size must be 0..7"));
}

static int
exp_val (expressionS exp)
{
  if (exp.X_op != O_constant)
  {
    rl78_error (_("constant expected"));
    return 0;
  }
  return exp.X_add_number;
}

static int
check_expr_is_const (expressionS e, int vmin, int vmax)
{
  static char buf[100];
  if (e.X_op != O_constant
      || e.X_add_number < vmin
      || e.X_add_number > vmax)
    {
      if (vmin == vmax)
	sprintf (buf, "%d expected here", vmin);
      else
	sprintf (buf, "%d..%d expected here", vmin, vmax);
      rl78_error(buf);
      return 0;
    }
  return 1;
}
