/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 22 "ldgram.y"

/*

 */

#define DONTDECLARE_MALLOC

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "ctf-api.h"
#include "ld.h"
#include "ldexp.h"
#include "ldver.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldmain.h"
#include "mri.h"
#include "ldctor.h"
#include "ldlex.h"

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

static enum section_type sectype;
static etree_type *sectype_value;
static lang_memory_region_type *region;

static bool ldgram_had_keep = false;
static char *ldgram_vers_current_lang = NULL;

#define ERROR_NAME_MAX 20
static char *error_names[ERROR_NAME_MAX];
static int error_index;
#define PUSH_ERROR(x) if (error_index < ERROR_NAME_MAX) error_names[error_index] = x; error_index++;
#define POP_ERROR()   error_index--;

#line 112 "ldgram.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_LDGRAM_H_INCLUDED
# define YY_YY_LDGRAM_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INT = 258,                     /* INT  */
    NAME = 259,                    /* NAME  */
    LNAME = 260,                   /* LNAME  */
    PLUSEQ = 261,                  /* PLUSEQ  */
    MINUSEQ = 262,                 /* MINUSEQ  */
    MULTEQ = 263,                  /* MULTEQ  */
    DIVEQ = 264,                   /* DIVEQ  */
    LSHIFTEQ = 265,                /* LSHIFTEQ  */
    RSHIFTEQ = 266,                /* RSHIFTEQ  */
    ANDEQ = 267,                   /* ANDEQ  */
    OREQ = 268,                    /* OREQ  */
    OROR = 269,                    /* OROR  */
    ANDAND = 270,                  /* ANDAND  */
    EQ = 271,                      /* EQ  */
    NE = 272,                      /* NE  */
    LE = 273,                      /* LE  */
    GE = 274,                      /* GE  */
    LSHIFT = 275,                  /* LSHIFT  */
    RSHIFT = 276,                  /* RSHIFT  */
    UNARY = 277,                   /* UNARY  */
    END = 278,                     /* END  */
    ALIGN_K = 279,                 /* ALIGN_K  */
    BLOCK = 280,                   /* BLOCK  */
    BIND = 281,                    /* BIND  */
    QUAD = 282,                    /* QUAD  */
    SQUAD = 283,                   /* SQUAD  */
    LONG = 284,                    /* LONG  */
    SHORT = 285,                   /* SHORT  */
    BYTE = 286,                    /* BYTE  */
    SECTIONS = 287,                /* SECTIONS  */
    PHDRS = 288,                   /* PHDRS  */
    INSERT_K = 289,                /* INSERT_K  */
    AFTER = 290,                   /* AFTER  */
    BEFORE = 291,                  /* BEFORE  */
    DATA_SEGMENT_ALIGN = 292,      /* DATA_SEGMENT_ALIGN  */
    DATA_SEGMENT_RELRO_END = 293,  /* DATA_SEGMENT_RELRO_END  */
    DATA_SEGMENT_END = 294,        /* DATA_SEGMENT_END  */
    SORT_BY_NAME = 295,            /* SORT_BY_NAME  */
    SORT_BY_ALIGNMENT = 296,       /* SORT_BY_ALIGNMENT  */
    SORT_NONE = 297,               /* SORT_NONE  */
    SORT_BY_INIT_PRIORITY = 298,   /* SORT_BY_INIT_PRIORITY  */
    SIZEOF_HEADERS = 299,          /* SIZEOF_HEADERS  */
    OUTPUT_FORMAT = 300,           /* OUTPUT_FORMAT  */
    FORCE_COMMON_ALLOCATION = 301, /* FORCE_COMMON_ALLOCATION  */
    OUTPUT_ARCH = 302,             /* OUTPUT_ARCH  */
    INHIBIT_COMMON_ALLOCATION = 303, /* INHIBIT_COMMON_ALLOCATION  */
    FORCE_GROUP_ALLOCATION = 304,  /* FORCE_GROUP_ALLOCATION  */
    SEGMENT_START = 305,           /* SEGMENT_START  */
    INCLUDE = 306,                 /* INCLUDE  */
    MEMORY = 307,                  /* MEMORY  */
    REGION_ALIAS = 308,            /* REGION_ALIAS  */
    LD_FEATURE = 309,              /* LD_FEATURE  */
    NOLOAD = 310,                  /* NOLOAD  */
    DSECT = 311,                   /* DSECT  */
    COPY = 312,                    /* COPY  */
    INFO = 313,                    /* INFO  */
    OVERLAY = 314,                 /* OVERLAY  */
    READONLY = 315,                /* READONLY  */
    TYPE = 316,                    /* TYPE  */
    DEFINED = 317,                 /* DEFINED  */
    TARGET_K = 318,                /* TARGET_K  */
    SEARCH_DIR = 319,              /* SEARCH_DIR  */
    MAP = 320,                     /* MAP  */
    ENTRY = 321,                   /* ENTRY  */
    NEXT = 322,                    /* NEXT  */
    SIZEOF = 323,                  /* SIZEOF  */
    ALIGNOF = 324,                 /* ALIGNOF  */
    ADDR = 325,                    /* ADDR  */
    LOADADDR = 326,                /* LOADADDR  */
    MAX_K = 327,                   /* MAX_K  */
    MIN_K = 328,                   /* MIN_K  */
    STARTUP = 329,                 /* STARTUP  */
    HLL = 330,                     /* HLL  */
    SYSLIB = 331,                  /* SYSLIB  */
    FLOAT = 332,                   /* FLOAT  */
    NOFLOAT = 333,                 /* NOFLOAT  */
    NOCROSSREFS = 334,             /* NOCROSSREFS  */
    NOCROSSREFS_TO = 335,          /* NOCROSSREFS_TO  */
    ORIGIN = 336,                  /* ORIGIN  */
    FILL = 337,                    /* FILL  */
    LENGTH = 338,                  /* LENGTH  */
    CREATE_OBJECT_SYMBOLS = 339,   /* CREATE_OBJECT_SYMBOLS  */
    INPUT = 340,                   /* INPUT  */
    GROUP = 341,                   /* GROUP  */
    OUTPUT = 342,                  /* OUTPUT  */
    CONSTRUCTORS = 343,            /* CONSTRUCTORS  */
    ALIGNMOD = 344,                /* ALIGNMOD  */
    AT = 345,                      /* AT  */
    SUBALIGN = 346,                /* SUBALIGN  */
    HIDDEN = 347,                  /* HIDDEN  */
    PROVIDE = 348,                 /* PROVIDE  */
    PROVIDE_HIDDEN = 349,          /* PROVIDE_HIDDEN  */
    AS_NEEDED = 350,               /* AS_NEEDED  */
    CHIP = 351,                    /* CHIP  */
    LIST = 352,                    /* LIST  */
    SECT = 353,                    /* SECT  */
    ABSOLUTE = 354,                /* ABSOLUTE  */
    LOAD = 355,                    /* LOAD  */
    NEWLINE = 356,                 /* NEWLINE  */
    ENDWORD = 357,                 /* ENDWORD  */
    ORDER = 358,                   /* ORDER  */
    NAMEWORD = 359,                /* NAMEWORD  */
    ASSERT_K = 360,                /* ASSERT_K  */
    LOG2CEIL = 361,                /* LOG2CEIL  */
    FORMAT = 362,                  /* FORMAT  */
    PUBLIC = 363,                  /* PUBLIC  */
    DEFSYMEND = 364,               /* DEFSYMEND  */
    BASE = 365,                    /* BASE  */
    ALIAS = 366,                   /* ALIAS  */
    TRUNCATE = 367,                /* TRUNCATE  */
    REL = 368,                     /* REL  */
    INPUT_SCRIPT = 369,            /* INPUT_SCRIPT  */
    INPUT_MRI_SCRIPT = 370,        /* INPUT_MRI_SCRIPT  */
    INPUT_DEFSYM = 371,            /* INPUT_DEFSYM  */
    CASE = 372,                    /* CASE  */
    EXTERN = 373,                  /* EXTERN  */
    START = 374,                   /* START  */
    VERS_TAG = 375,                /* VERS_TAG  */
    VERS_IDENTIFIER = 376,         /* VERS_IDENTIFIER  */
    GLOBAL = 377,                  /* GLOBAL  */
    LOCAL = 378,                   /* LOCAL  */
    VERSIONK = 379,                /* VERSIONK  */
    INPUT_VERSION_SCRIPT = 380,    /* INPUT_VERSION_SCRIPT  */
    KEEP = 381,                    /* KEEP  */
    ONLY_IF_RO = 382,              /* ONLY_IF_RO  */
    ONLY_IF_RW = 383,              /* ONLY_IF_RW  */
    SPECIAL = 384,                 /* SPECIAL  */
    INPUT_SECTION_FLAGS = 385,     /* INPUT_SECTION_FLAGS  */
    ALIGN_WITH_INPUT = 386,        /* ALIGN_WITH_INPUT  */
    EXCLUDE_FILE = 387,            /* EXCLUDE_FILE  */
    CONSTANT = 388,                /* CONSTANT  */
    INPUT_DYNAMIC_LIST = 389       /* INPUT_DYNAMIC_LIST  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define NAME 259
#define LNAME 260
#define PLUSEQ 261
#define MINUSEQ 262
#define MULTEQ 263
#define DIVEQ 264
#define LSHIFTEQ 265
#define RSHIFTEQ 266
#define ANDEQ 267
#define OREQ 268
#define OROR 269
#define ANDAND 270
#define EQ 271
#define NE 272
#define LE 273
#define GE 274
#define LSHIFT 275
#define RSHIFT 276
#define UNARY 277
#define END 278
#define ALIGN_K 279
#define BLOCK 280
#define BIND 281
#define QUAD 282
#define SQUAD 283
#define LONG 284
#define SHORT 285
#define BYTE 286
#define SECTIONS 287
#define PHDRS 288
#define INSERT_K 289
#define AFTER 290
#define BEFORE 291
#define DATA_SEGMENT_ALIGN 292
#define DATA_SEGMENT_RELRO_END 293
#define DATA_SEGMENT_END 294
#define SORT_BY_NAME 295
#define SORT_BY_ALIGNMENT 296
#define SORT_NONE 297
#define SORT_BY_INIT_PRIORITY 298
#define SIZEOF_HEADERS 299
#define OUTPUT_FORMAT 300
#define FORCE_COMMON_ALLOCATION 301
#define OUTPUT_ARCH 302
#define INHIBIT_COMMON_ALLOCATION 303
#define FORCE_GROUP_ALLOCATION 304
#define SEGMENT_START 305
#define INCLUDE 306
#define MEMORY 307
#define REGION_ALIAS 308
#define LD_FEATURE 309
#define NOLOAD 310
#define DSECT 311
#define COPY 312
#define INFO 313
#define OVERLAY 314
#define READONLY 315
#define TYPE 316
#define DEFINED 317
#define TARGET_K 318
#define SEARCH_DIR 319
#define MAP 320
#define ENTRY 321
#define NEXT 322
#define SIZEOF 323
#define ALIGNOF 324
#define ADDR 325
#define LOADADDR 326
#define MAX_K 327
#define MIN_K 328
#define STARTUP 329
#define HLL 330
#define SYSLIB 331
#define FLOAT 332
#define NOFLOAT 333
#define NOCROSSREFS 334
#define NOCROSSREFS_TO 335
#define ORIGIN 336
#define FILL 337
#define LENGTH 338
#define CREATE_OBJECT_SYMBOLS 339
#define INPUT 340
#define GROUP 341
#define OUTPUT 342
#define CONSTRUCTORS 343
#define ALIGNMOD 344
#define AT 345
#define SUBALIGN 346
#define HIDDEN 347
#define PROVIDE 348
#define PROVIDE_HIDDEN 349
#define AS_NEEDED 350
#define CHIP 351
#define LIST 352
#define SECT 353
#define ABSOLUTE 354
#define LOAD 355
#define NEWLINE 356
#define ENDWORD 357
#define ORDER 358
#define NAMEWORD 359
#define ASSERT_K 360
#define LOG2CEIL 361
#define FORMAT 362
#define PUBLIC 363
#define DEFSYMEND 364
#define BASE 365
#define ALIAS 366
#define TRUNCATE 367
#define REL 368
#define INPUT_SCRIPT 369
#define INPUT_MRI_SCRIPT 370
#define INPUT_DEFSYM 371
#define CASE 372
#define EXTERN 373
#define START 374
#define VERS_TAG 375
#define VERS_IDENTIFIER 376
#define GLOBAL 377
#define LOCAL 378
#define VERSIONK 379
#define INPUT_VERSION_SCRIPT 380
#define KEEP 381
#define ONLY_IF_RO 382
#define ONLY_IF_RW 383
#define SPECIAL 384
#define INPUT_SECTION_FLAGS 385
#define ALIGN_WITH_INPUT 386
#define EXCLUDE_FILE 387
#define CONSTANT 388
#define INPUT_DYNAMIC_LIST 389

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 62 "ldgram.y"

  bfd_vma integer;
  struct big_int
    {
      bfd_vma integer;
      char *str;
    } bigint;
  fill_type *fill;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
  struct wildcard_list *wildcard_list;
  struct name_list *name_list;
  struct flag_info_list *flag_info_list;
  struct flag_info *flag_info;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      bool filehdr;
      bool phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;

#line 464 "ldgram.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_LDGRAM_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_NAME = 4,                       /* NAME  */
  YYSYMBOL_LNAME = 5,                      /* LNAME  */
  YYSYMBOL_PLUSEQ = 6,                     /* PLUSEQ  */
  YYSYMBOL_MINUSEQ = 7,                    /* MINUSEQ  */
  YYSYMBOL_MULTEQ = 8,                     /* MULTEQ  */
  YYSYMBOL_DIVEQ = 9,                      /* DIVEQ  */
  YYSYMBOL_10_ = 10,                       /* '='  */
  YYSYMBOL_LSHIFTEQ = 11,                  /* LSHIFTEQ  */
  YYSYMBOL_RSHIFTEQ = 12,                  /* RSHIFTEQ  */
  YYSYMBOL_ANDEQ = 13,                     /* ANDEQ  */
  YYSYMBOL_OREQ = 14,                      /* OREQ  */
  YYSYMBOL_15_ = 15,                       /* '?'  */
  YYSYMBOL_16_ = 16,                       /* ':'  */
  YYSYMBOL_OROR = 17,                      /* OROR  */
  YYSYMBOL_ANDAND = 18,                    /* ANDAND  */
  YYSYMBOL_19_ = 19,                       /* '|'  */
  YYSYMBOL_20_ = 20,                       /* '^'  */
  YYSYMBOL_21_ = 21,                       /* '&'  */
  YYSYMBOL_EQ = 22,                        /* EQ  */
  YYSYMBOL_NE = 23,                        /* NE  */
  YYSYMBOL_24_ = 24,                       /* '<'  */
  YYSYMBOL_25_ = 25,                       /* '>'  */
  YYSYMBOL_LE = 26,                        /* LE  */
  YYSYMBOL_GE = 27,                        /* GE  */
  YYSYMBOL_LSHIFT = 28,                    /* LSHIFT  */
  YYSYMBOL_RSHIFT = 29,                    /* RSHIFT  */
  YYSYMBOL_30_ = 30,                       /* '+'  */
  YYSYMBOL_31_ = 31,                       /* '-'  */
  YYSYMBOL_32_ = 32,                       /* '*'  */
  YYSYMBOL_33_ = 33,                       /* '/'  */
  YYSYMBOL_34_ = 34,                       /* '%'  */
  YYSYMBOL_UNARY = 35,                     /* UNARY  */
  YYSYMBOL_END = 36,                       /* END  */
  YYSYMBOL_37_ = 37,                       /* '('  */
  YYSYMBOL_ALIGN_K = 38,                   /* ALIGN_K  */
  YYSYMBOL_BLOCK = 39,                     /* BLOCK  */
  YYSYMBOL_BIND = 40,                      /* BIND  */
  YYSYMBOL_QUAD = 41,                      /* QUAD  */
  YYSYMBOL_SQUAD = 42,                     /* SQUAD  */
  YYSYMBOL_LONG = 43,                      /* LONG  */
  YYSYMBOL_SHORT = 44,                     /* SHORT  */
  YYSYMBOL_BYTE = 45,                      /* BYTE  */
  YYSYMBOL_SECTIONS = 46,                  /* SECTIONS  */
  YYSYMBOL_PHDRS = 47,                     /* PHDRS  */
  YYSYMBOL_INSERT_K = 48,                  /* INSERT_K  */
  YYSYMBOL_AFTER = 49,                     /* AFTER  */
  YYSYMBOL_BEFORE = 50,                    /* BEFORE  */
  YYSYMBOL_DATA_SEGMENT_ALIGN = 51,        /* DATA_SEGMENT_ALIGN  */
  YYSYMBOL_DATA_SEGMENT_RELRO_END = 52,    /* DATA_SEGMENT_RELRO_END  */
  YYSYMBOL_DATA_SEGMENT_END = 53,          /* DATA_SEGMENT_END  */
  YYSYMBOL_SORT_BY_NAME = 54,              /* SORT_BY_NAME  */
  YYSYMBOL_SORT_BY_ALIGNMENT = 55,         /* SORT_BY_ALIGNMENT  */
  YYSYMBOL_SORT_NONE = 56,                 /* SORT_NONE  */
  YYSYMBOL_SORT_BY_INIT_PRIORITY = 57,     /* SORT_BY_INIT_PRIORITY  */
  YYSYMBOL_58_ = 58,                       /* '{'  */
  YYSYMBOL_59_ = 59,                       /* '}'  */
  YYSYMBOL_SIZEOF_HEADERS = 60,            /* SIZEOF_HEADERS  */
  YYSYMBOL_OUTPUT_FORMAT = 61,             /* OUTPUT_FORMAT  */
  YYSYMBOL_FORCE_COMMON_ALLOCATION = 62,   /* FORCE_COMMON_ALLOCATION  */
  YYSYMBOL_OUTPUT_ARCH = 63,               /* OUTPUT_ARCH  */
  YYSYMBOL_INHIBIT_COMMON_ALLOCATION = 64, /* INHIBIT_COMMON_ALLOCATION  */
  YYSYMBOL_FORCE_GROUP_ALLOCATION = 65,    /* FORCE_GROUP_ALLOCATION  */
  YYSYMBOL_SEGMENT_START = 66,             /* SEGMENT_START  */
  YYSYMBOL_INCLUDE = 67,                   /* INCLUDE  */
  YYSYMBOL_MEMORY = 68,                    /* MEMORY  */
  YYSYMBOL_REGION_ALIAS = 69,              /* REGION_ALIAS  */
  YYSYMBOL_LD_FEATURE = 70,                /* LD_FEATURE  */
  YYSYMBOL_NOLOAD = 71,                    /* NOLOAD  */
  YYSYMBOL_DSECT = 72,                     /* DSECT  */
  YYSYMBOL_COPY = 73,                      /* COPY  */
  YYSYMBOL_INFO = 74,                      /* INFO  */
  YYSYMBOL_OVERLAY = 75,                   /* OVERLAY  */
  YYSYMBOL_READONLY = 76,                  /* READONLY  */
  YYSYMBOL_TYPE = 77,                      /* TYPE  */
  YYSYMBOL_DEFINED = 78,                   /* DEFINED  */
  YYSYMBOL_TARGET_K = 79,                  /* TARGET_K  */
  YYSYMBOL_SEARCH_DIR = 80,                /* SEARCH_DIR  */
  YYSYMBOL_MAP = 81,                       /* MAP  */
  YYSYMBOL_ENTRY = 82,                     /* ENTRY  */
  YYSYMBOL_NEXT = 83,                      /* NEXT  */
  YYSYMBOL_SIZEOF = 84,                    /* SIZEOF  */
  YYSYMBOL_ALIGNOF = 85,                   /* ALIGNOF  */
  YYSYMBOL_ADDR = 86,                      /* ADDR  */
  YYSYMBOL_LOADADDR = 87,                  /* LOADADDR  */
  YYSYMBOL_MAX_K = 88,                     /* MAX_K  */
  YYSYMBOL_MIN_K = 89,                     /* MIN_K  */
  YYSYMBOL_STARTUP = 90,                   /* STARTUP  */
  YYSYMBOL_HLL = 91,                       /* HLL  */
  YYSYMBOL_SYSLIB = 92,                    /* SYSLIB  */
  YYSYMBOL_FLOAT = 93,                     /* FLOAT  */
  YYSYMBOL_NOFLOAT = 94,                   /* NOFLOAT  */
  YYSYMBOL_NOCROSSREFS = 95,               /* NOCROSSREFS  */
  YYSYMBOL_NOCROSSREFS_TO = 96,            /* NOCROSSREFS_TO  */
  YYSYMBOL_ORIGIN = 97,                    /* ORIGIN  */
  YYSYMBOL_FILL = 98,                      /* FILL  */
  YYSYMBOL_LENGTH = 99,                    /* LENGTH  */
  YYSYMBOL_CREATE_OBJECT_SYMBOLS = 100,    /* CREATE_OBJECT_SYMBOLS  */
  YYSYMBOL_INPUT = 101,                    /* INPUT  */
  YYSYMBOL_GROUP = 102,                    /* GROUP  */
  YYSYMBOL_OUTPUT = 103,                   /* OUTPUT  */
  YYSYMBOL_CONSTRUCTORS = 104,             /* CONSTRUCTORS  */
  YYSYMBOL_ALIGNMOD = 105,                 /* ALIGNMOD  */
  YYSYMBOL_AT = 106,                       /* AT  */
  YYSYMBOL_SUBALIGN = 107,                 /* SUBALIGN  */
  YYSYMBOL_HIDDEN = 108,                   /* HIDDEN  */
  YYSYMBOL_PROVIDE = 109,                  /* PROVIDE  */
  YYSYMBOL_PROVIDE_HIDDEN = 110,           /* PROVIDE_HIDDEN  */
  YYSYMBOL_AS_NEEDED = 111,                /* AS_NEEDED  */
  YYSYMBOL_CHIP = 112,                     /* CHIP  */
  YYSYMBOL_LIST = 113,                     /* LIST  */
  YYSYMBOL_SECT = 114,                     /* SECT  */
  YYSYMBOL_ABSOLUTE = 115,                 /* ABSOLUTE  */
  YYSYMBOL_LOAD = 116,                     /* LOAD  */
  YYSYMBOL_NEWLINE = 117,                  /* NEWLINE  */
  YYSYMBOL_ENDWORD = 118,                  /* ENDWORD  */
  YYSYMBOL_ORDER = 119,                    /* ORDER  */
  YYSYMBOL_NAMEWORD = 120,                 /* NAMEWORD  */
  YYSYMBOL_ASSERT_K = 121,                 /* ASSERT_K  */
  YYSYMBOL_LOG2CEIL = 122,                 /* LOG2CEIL  */
  YYSYMBOL_FORMAT = 123,                   /* FORMAT  */
  YYSYMBOL_PUBLIC = 124,                   /* PUBLIC  */
  YYSYMBOL_DEFSYMEND = 125,                /* DEFSYMEND  */
  YYSYMBOL_BASE = 126,                     /* BASE  */
  YYSYMBOL_ALIAS = 127,                    /* ALIAS  */
  YYSYMBOL_TRUNCATE = 128,                 /* TRUNCATE  */
  YYSYMBOL_REL = 129,                      /* REL  */
  YYSYMBOL_INPUT_SCRIPT = 130,             /* INPUT_SCRIPT  */
  YYSYMBOL_INPUT_MRI_SCRIPT = 131,         /* INPUT_MRI_SCRIPT  */
  YYSYMBOL_INPUT_DEFSYM = 132,             /* INPUT_DEFSYM  */
  YYSYMBOL_CASE = 133,                     /* CASE  */
  YYSYMBOL_EXTERN = 134,                   /* EXTERN  */
  YYSYMBOL_START = 135,                    /* START  */
  YYSYMBOL_VERS_TAG = 136,                 /* VERS_TAG  */
  YYSYMBOL_VERS_IDENTIFIER = 137,          /* VERS_IDENTIFIER  */
  YYSYMBOL_GLOBAL = 138,                   /* GLOBAL  */
  YYSYMBOL_LOCAL = 139,                    /* LOCAL  */
  YYSYMBOL_VERSIONK = 140,                 /* VERSIONK  */
  YYSYMBOL_INPUT_VERSION_SCRIPT = 141,     /* INPUT_VERSION_SCRIPT  */
  YYSYMBOL_KEEP = 142,                     /* KEEP  */
  YYSYMBOL_ONLY_IF_RO = 143,               /* ONLY_IF_RO  */
  YYSYMBOL_ONLY_IF_RW = 144,               /* ONLY_IF_RW  */
  YYSYMBOL_SPECIAL = 145,                  /* SPECIAL  */
  YYSYMBOL_INPUT_SECTION_FLAGS = 146,      /* INPUT_SECTION_FLAGS  */
  YYSYMBOL_ALIGN_WITH_INPUT = 147,         /* ALIGN_WITH_INPUT  */
  YYSYMBOL_EXCLUDE_FILE = 148,             /* EXCLUDE_FILE  */
  YYSYMBOL_CONSTANT = 149,                 /* CONSTANT  */
  YYSYMBOL_INPUT_DYNAMIC_LIST = 150,       /* INPUT_DYNAMIC_LIST  */
  YYSYMBOL_151_ = 151,                     /* ','  */
  YYSYMBOL_152_ = 152,                     /* ';'  */
  YYSYMBOL_153_ = 153,                     /* ')'  */
  YYSYMBOL_154_ = 154,                     /* '['  */
  YYSYMBOL_155_ = 155,                     /* ']'  */
  YYSYMBOL_156_ = 156,                     /* '!'  */
  YYSYMBOL_157_ = 157,                     /* '~'  */
  YYSYMBOL_YYACCEPT = 158,                 /* $accept  */
  YYSYMBOL_file = 159,                     /* file  */
  YYSYMBOL_filename = 160,                 /* filename  */
  YYSYMBOL_defsym_expr = 161,              /* defsym_expr  */
  YYSYMBOL_162_1 = 162,                    /* $@1  */
  YYSYMBOL_mri_script_file = 163,          /* mri_script_file  */
  YYSYMBOL_164_2 = 164,                    /* $@2  */
  YYSYMBOL_mri_script_lines = 165,         /* mri_script_lines  */
  YYSYMBOL_mri_script_command = 166,       /* mri_script_command  */
  YYSYMBOL_167_3 = 167,                    /* $@3  */
  YYSYMBOL_ordernamelist = 168,            /* ordernamelist  */
  YYSYMBOL_mri_load_name_list = 169,       /* mri_load_name_list  */
  YYSYMBOL_mri_abs_name_list = 170,        /* mri_abs_name_list  */
  YYSYMBOL_casesymlist = 171,              /* casesymlist  */
  YYSYMBOL_extern_name_list = 172,         /* extern_name_list  */
  YYSYMBOL_script_file = 173,              /* script_file  */
  YYSYMBOL_174_4 = 174,                    /* $@4  */
  YYSYMBOL_ifile_list = 175,               /* ifile_list  */
  YYSYMBOL_ifile_p1 = 176,                 /* ifile_p1  */
  YYSYMBOL_177_5 = 177,                    /* $@5  */
  YYSYMBOL_178_6 = 178,                    /* $@6  */
  YYSYMBOL_179_7 = 179,                    /* $@7  */
  YYSYMBOL_input_list = 180,               /* input_list  */
  YYSYMBOL_181_8 = 181,                    /* $@8  */
  YYSYMBOL_input_list1 = 182,              /* input_list1  */
  YYSYMBOL_183_9 = 183,                    /* @9  */
  YYSYMBOL_184_10 = 184,                   /* @10  */
  YYSYMBOL_185_11 = 185,                   /* @11  */
  YYSYMBOL_sections = 186,                 /* sections  */
  YYSYMBOL_sec_or_group_p1 = 187,          /* sec_or_group_p1  */
  YYSYMBOL_statement_anywhere = 188,       /* statement_anywhere  */
  YYSYMBOL_189_12 = 189,                   /* $@12  */
  YYSYMBOL_wildcard_name = 190,            /* wildcard_name  */
  YYSYMBOL_wildcard_maybe_exclude = 191,   /* wildcard_maybe_exclude  */
  YYSYMBOL_filename_spec = 192,            /* filename_spec  */
  YYSYMBOL_section_name_spec = 193,        /* section_name_spec  */
  YYSYMBOL_sect_flag_list = 194,           /* sect_flag_list  */
  YYSYMBOL_sect_flags = 195,               /* sect_flags  */
  YYSYMBOL_exclude_name_list = 196,        /* exclude_name_list  */
  YYSYMBOL_section_name_list = 197,        /* section_name_list  */
  YYSYMBOL_input_section_spec_no_keep = 198, /* input_section_spec_no_keep  */
  YYSYMBOL_input_section_spec = 199,       /* input_section_spec  */
  YYSYMBOL_200_13 = 200,                   /* $@13  */
  YYSYMBOL_statement = 201,                /* statement  */
  YYSYMBOL_202_14 = 202,                   /* $@14  */
  YYSYMBOL_203_15 = 203,                   /* $@15  */
  YYSYMBOL_statement_list = 204,           /* statement_list  */
  YYSYMBOL_statement_list_opt = 205,       /* statement_list_opt  */
  YYSYMBOL_length = 206,                   /* length  */
  YYSYMBOL_fill_exp = 207,                 /* fill_exp  */
  YYSYMBOL_fill_opt = 208,                 /* fill_opt  */
  YYSYMBOL_assign_op = 209,                /* assign_op  */
  YYSYMBOL_separator = 210,                /* separator  */
  YYSYMBOL_assignment = 211,               /* assignment  */
  YYSYMBOL_opt_comma = 212,                /* opt_comma  */
  YYSYMBOL_memory = 213,                   /* memory  */
  YYSYMBOL_memory_spec_list_opt = 214,     /* memory_spec_list_opt  */
  YYSYMBOL_memory_spec_list = 215,         /* memory_spec_list  */
  YYSYMBOL_memory_spec = 216,              /* memory_spec  */
  YYSYMBOL_217_16 = 217,                   /* $@16  */
  YYSYMBOL_218_17 = 218,                   /* $@17  */
  YYSYMBOL_origin_spec = 219,              /* origin_spec  */
  YYSYMBOL_length_spec = 220,              /* length_spec  */
  YYSYMBOL_attributes_opt = 221,           /* attributes_opt  */
  YYSYMBOL_attributes_list = 222,          /* attributes_list  */
  YYSYMBOL_attributes_string = 223,        /* attributes_string  */
  YYSYMBOL_startup = 224,                  /* startup  */
  YYSYMBOL_high_level_library = 225,       /* high_level_library  */
  YYSYMBOL_high_level_library_NAME_list = 226, /* high_level_library_NAME_list  */
  YYSYMBOL_low_level_library = 227,        /* low_level_library  */
  YYSYMBOL_low_level_library_NAME_list = 228, /* low_level_library_NAME_list  */
  YYSYMBOL_floating_point_support = 229,   /* floating_point_support  */
  YYSYMBOL_nocrossref_list = 230,          /* nocrossref_list  */
  YYSYMBOL_paren_script_name = 231,        /* paren_script_name  */
  YYSYMBOL_232_18 = 232,                   /* $@18  */
  YYSYMBOL_mustbe_exp = 233,               /* mustbe_exp  */
  YYSYMBOL_234_19 = 234,                   /* $@19  */
  YYSYMBOL_exp = 235,                      /* exp  */
  YYSYMBOL_236_20 = 236,                   /* $@20  */
  YYSYMBOL_237_21 = 237,                   /* $@21  */
  YYSYMBOL_memspec_at_opt = 238,           /* memspec_at_opt  */
  YYSYMBOL_opt_at = 239,                   /* opt_at  */
  YYSYMBOL_opt_align = 240,                /* opt_align  */
  YYSYMBOL_opt_align_with_input = 241,     /* opt_align_with_input  */
  YYSYMBOL_opt_subalign = 242,             /* opt_subalign  */
  YYSYMBOL_sect_constraint = 243,          /* sect_constraint  */
  YYSYMBOL_section = 244,                  /* section  */
  YYSYMBOL_245_22 = 245,                   /* $@22  */
  YYSYMBOL_246_23 = 246,                   /* $@23  */
  YYSYMBOL_247_24 = 247,                   /* $@24  */
  YYSYMBOL_248_25 = 248,                   /* $@25  */
  YYSYMBOL_249_26 = 249,                   /* $@26  */
  YYSYMBOL_250_27 = 250,                   /* $@27  */
  YYSYMBOL_251_28 = 251,                   /* $@28  */
  YYSYMBOL_252_29 = 252,                   /* $@29  */
  YYSYMBOL_253_30 = 253,                   /* $@30  */
  YYSYMBOL_254_31 = 254,                   /* $@31  */
  YYSYMBOL_255_32 = 255,                   /* $@32  */
  YYSYMBOL_type = 256,                     /* type  */
  YYSYMBOL_atype = 257,                    /* atype  */
  YYSYMBOL_opt_exp_with_type = 258,        /* opt_exp_with_type  */
  YYSYMBOL_opt_exp_without_type = 259,     /* opt_exp_without_type  */
  YYSYMBOL_opt_nocrossrefs = 260,          /* opt_nocrossrefs  */
  YYSYMBOL_memspec_opt = 261,              /* memspec_opt  */
  YYSYMBOL_phdr_opt = 262,                 /* phdr_opt  */
  YYSYMBOL_overlay_section = 263,          /* overlay_section  */
  YYSYMBOL_264_33 = 264,                   /* $@33  */
  YYSYMBOL_265_34 = 265,                   /* $@34  */
  YYSYMBOL_266_35 = 266,                   /* $@35  */
  YYSYMBOL_phdrs = 267,                    /* phdrs  */
  YYSYMBOL_phdr_list = 268,                /* phdr_list  */
  YYSYMBOL_phdr = 269,                     /* phdr  */
  YYSYMBOL_270_36 = 270,                   /* $@36  */
  YYSYMBOL_271_37 = 271,                   /* $@37  */
  YYSYMBOL_phdr_type = 272,                /* phdr_type  */
  YYSYMBOL_phdr_qualifiers = 273,          /* phdr_qualifiers  */
  YYSYMBOL_phdr_val = 274,                 /* phdr_val  */
  YYSYMBOL_dynamic_list_file = 275,        /* dynamic_list_file  */
  YYSYMBOL_276_38 = 276,                   /* $@38  */
  YYSYMBOL_dynamic_list_nodes = 277,       /* dynamic_list_nodes  */
  YYSYMBOL_dynamic_list_node = 278,        /* dynamic_list_node  */
  YYSYMBOL_dynamic_list_tag = 279,         /* dynamic_list_tag  */
  YYSYMBOL_version_script_file = 280,      /* version_script_file  */
  YYSYMBOL_281_39 = 281,                   /* $@39  */
  YYSYMBOL_version = 282,                  /* version  */
  YYSYMBOL_283_40 = 283,                   /* $@40  */
  YYSYMBOL_vers_nodes = 284,               /* vers_nodes  */
  YYSYMBOL_vers_node = 285,                /* vers_node  */
  YYSYMBOL_verdep = 286,                   /* verdep  */
  YYSYMBOL_vers_tag = 287,                 /* vers_tag  */
  YYSYMBOL_vers_defns = 288,               /* vers_defns  */
  YYSYMBOL_289_41 = 289,                   /* @41  */
  YYSYMBOL_290_42 = 290,                   /* @42  */
  YYSYMBOL_opt_semicolon = 291             /* opt_semicolon  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2005

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  158
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  134
/* YYNRULES -- Number of rules.  */
#define YYNRULES  378
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  810

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   389


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   156,     2,     2,     2,    34,    21,     2,
      37,   153,    32,    30,   151,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   152,
      24,    10,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   154,     2,   155,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    58,    19,    59,   157,     2,     2,     2,
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
       5,     6,     7,     8,     9,    11,    12,    13,    14,    17,
      18,    22,    23,    26,    27,    28,    29,    35,    36,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   169,   169,   170,   171,   172,   173,   177,   181,   181,
     188,   188,   201,   202,   206,   207,   208,   211,   214,   215,
     216,   218,   220,   222,   224,   226,   228,   230,   232,   234,
     236,   238,   239,   240,   242,   244,   246,   248,   250,   251,
     253,   252,   255,   257,   261,   262,   263,   267,   269,   273,
     275,   280,   281,   282,   286,   288,   290,   295,   295,   301,
     302,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   319,   321,   323,   326,   328,   330,   332,   334,
     336,   338,   337,   341,   344,   343,   346,   350,   354,   354,
     356,   358,   360,   362,   367,   367,   372,   375,   378,   381,
     384,   387,   391,   390,   396,   395,   401,   400,   408,   412,
     413,   414,   418,   420,   421,   421,   427,   434,   441,   451,
     452,   457,   465,   466,   471,   476,   481,   486,   491,   496,
     501,   508,   526,   547,   560,   569,   580,   589,   600,   609,
     618,   622,   631,   635,   643,   645,   644,   651,   652,   653,
     657,   661,   666,   667,   672,   677,   676,   684,   683,   691,
     692,   695,   697,   701,   703,   705,   707,   709,   714,   721,
     723,   727,   729,   731,   733,   735,   737,   739,   741,   746,
     746,   751,   755,   763,   767,   771,   779,   779,   783,   786,
     786,   789,   790,   795,   794,   800,   799,   805,   812,   825,
     826,   830,   831,   835,   837,   842,   847,   848,   853,   855,
     860,   864,   866,   870,   872,   878,   881,   890,   901,   901,
     905,   905,   911,   913,   915,   917,   919,   921,   924,   926,
     928,   930,   932,   934,   936,   938,   940,   942,   944,   946,
     948,   950,   952,   954,   956,   958,   960,   962,   964,   966,
     969,   971,   973,   975,   977,   979,   981,   983,   985,   987,
     989,   991,   992,   991,  1001,  1003,  1005,  1007,  1009,  1011,
    1013,  1015,  1021,  1022,  1026,  1027,  1031,  1032,  1036,  1037,
    1041,  1042,  1046,  1047,  1048,  1049,  1053,  1060,  1069,  1071,
    1052,  1089,  1091,  1093,  1099,  1088,  1114,  1116,  1113,  1122,
    1121,  1129,  1130,  1131,  1132,  1133,  1134,  1135,  1136,  1140,
    1141,  1142,  1146,  1147,  1152,  1153,  1158,  1159,  1164,  1165,
    1170,  1172,  1177,  1180,  1193,  1197,  1204,  1206,  1195,  1218,
    1221,  1223,  1227,  1228,  1227,  1237,  1286,  1289,  1302,  1311,
    1314,  1321,  1321,  1333,  1334,  1338,  1342,  1351,  1351,  1365,
    1365,  1375,  1376,  1380,  1384,  1388,  1395,  1399,  1407,  1410,
    1414,  1418,  1422,  1429,  1433,  1437,  1441,  1446,  1445,  1459,
    1458,  1468,  1472,  1476,  1480,  1484,  1488,  1494,  1496
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "NAME", "LNAME",
  "PLUSEQ", "MINUSEQ", "MULTEQ", "DIVEQ", "'='", "LSHIFTEQ", "RSHIFTEQ",
  "ANDEQ", "OREQ", "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'",
  "EQ", "NE", "'<'", "'>'", "LE", "GE", "LSHIFT", "RSHIFT", "'+'", "'-'",
  "'*'", "'/'", "'%'", "UNARY", "END", "'('", "ALIGN_K", "BLOCK", "BIND",
  "QUAD", "SQUAD", "LONG", "SHORT", "BYTE", "SECTIONS", "PHDRS",
  "INSERT_K", "AFTER", "BEFORE", "DATA_SEGMENT_ALIGN",
  "DATA_SEGMENT_RELRO_END", "DATA_SEGMENT_END", "SORT_BY_NAME",
  "SORT_BY_ALIGNMENT", "SORT_NONE", "SORT_BY_INIT_PRIORITY", "'{'", "'}'",
  "SIZEOF_HEADERS", "OUTPUT_FORMAT", "FORCE_COMMON_ALLOCATION",
  "OUTPUT_ARCH", "INHIBIT_COMMON_ALLOCATION", "FORCE_GROUP_ALLOCATION",
  "SEGMENT_START", "INCLUDE", "MEMORY", "REGION_ALIAS", "LD_FEATURE",
  "NOLOAD", "DSECT", "COPY", "INFO", "OVERLAY", "READONLY", "TYPE",
  "DEFINED", "TARGET_K", "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF",
  "ALIGNOF", "ADDR", "LOADADDR", "MAX_K", "MIN_K", "STARTUP", "HLL",
  "SYSLIB", "FLOAT", "NOFLOAT", "NOCROSSREFS", "NOCROSSREFS_TO", "ORIGIN",
  "FILL", "LENGTH", "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP", "OUTPUT",
  "CONSTRUCTORS", "ALIGNMOD", "AT", "SUBALIGN", "HIDDEN", "PROVIDE",
  "PROVIDE_HIDDEN", "AS_NEEDED", "CHIP", "LIST", "SECT", "ABSOLUTE",
  "LOAD", "NEWLINE", "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K",
  "LOG2CEIL", "FORMAT", "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE",
  "REL", "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE",
  "EXTERN", "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL",
  "VERSIONK", "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW",
  "SPECIAL", "INPUT_SECTION_FLAGS", "ALIGN_WITH_INPUT", "EXCLUDE_FILE",
  "CONSTANT", "INPUT_DYNAMIC_LIST", "','", "';'", "')'", "'['", "']'",
  "'!'", "'~'", "$accept", "file", "filename", "defsym_expr", "$@1",
  "mri_script_file", "$@2", "mri_script_lines", "mri_script_command",
  "$@3", "ordernamelist", "mri_load_name_list", "mri_abs_name_list",
  "casesymlist", "extern_name_list", "script_file", "$@4", "ifile_list",
  "ifile_p1", "$@5", "$@6", "$@7", "input_list", "$@8", "input_list1",
  "@9", "@10", "@11", "sections", "sec_or_group_p1", "statement_anywhere",
  "$@12", "wildcard_name", "wildcard_maybe_exclude", "filename_spec",
  "section_name_spec", "sect_flag_list", "sect_flags", "exclude_name_list",
  "section_name_list", "input_section_spec_no_keep", "input_section_spec",
  "$@13", "statement", "$@14", "$@15", "statement_list",
  "statement_list_opt", "length", "fill_exp", "fill_opt", "assign_op",
  "separator", "assignment", "opt_comma", "memory", "memory_spec_list_opt",
  "memory_spec_list", "memory_spec", "$@16", "$@17", "origin_spec",
  "length_spec", "attributes_opt", "attributes_list", "attributes_string",
  "startup", "high_level_library", "high_level_library_NAME_list",
  "low_level_library", "low_level_library_NAME_list",
  "floating_point_support", "nocrossref_list", "paren_script_name", "$@18",
  "mustbe_exp", "$@19", "exp", "$@20", "$@21", "memspec_at_opt", "opt_at",
  "opt_align", "opt_align_with_input", "opt_subalign", "sect_constraint",
  "section", "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28",
  "$@29", "$@30", "$@31", "$@32", "type", "atype", "opt_exp_with_type",
  "opt_exp_without_type", "opt_nocrossrefs", "memspec_opt", "phdr_opt",
  "overlay_section", "$@33", "$@34", "$@35", "phdrs", "phdr_list", "phdr",
  "$@36", "$@37", "phdr_type", "phdr_qualifiers", "phdr_val",
  "dynamic_list_file", "$@38", "dynamic_list_nodes", "dynamic_list_node",
  "dynamic_list_tag", "version_script_file", "$@39", "version", "$@40",
  "vers_nodes", "vers_node", "verdep", "vers_tag", "vers_defns", "@41",
  "@42", "opt_semicolon", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-750)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-350)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     147,  -750,  -750,  -750,  -750,  -750,    90,  -750,  -750,  -750,
    -750,  -750,    20,  -750,   -16,  -750,   -26,  -750,   915,  1765,
     453,    56,    68,    89,  -750,   106,    59,   -16,  -750,   112,
     -26,  -750,    69,    73,    52,    98,  -750,   114,  -750,  -750,
     184,   176,   198,   204,   205,   210,   211,   215,   232,   233,
     238,  -750,  -750,   243,   252,   256,  -750,   261,  -750,   262,
    -750,  -750,  -750,  -750,   -43,  -750,  -750,  -750,  -750,  -750,
    -750,  -750,   161,  -750,   298,   184,   299,   768,  -750,   300,
     309,   310,  -750,  -750,   311,   312,   321,   768,   322,   325,
     328,   338,   339,   207,  -750,  -750,  -750,  -750,  -750,  -750,
    -750,  -750,  -750,  -750,   340,   341,   343,  -750,   344,  -750,
     313,   342,   304,   236,   106,  -750,  -750,  -750,   330,   245,
    -750,  -750,  -750,   380,   395,   396,   398,  -750,  -750,    30,
     409,   410,   411,   184,   184,   413,   184,    13,  -750,   414,
     414,  -750,   383,   184,   386,  -750,  -750,  -750,  -750,   366,
       2,  -750,    77,  -750,  -750,   768,   768,   768,   388,   389,
     390,   394,   397,  -750,  -750,   399,   400,  -750,  -750,  -750,
    -750,   401,   403,  -750,  -750,   404,   412,   415,   416,   768,
     768,  1550,   391,  -750,   281,  -750,   282,    43,  -750,  -750,
     506,  1820,   284,  -750,  -750,   297,  -750,    44,  -750,  -750,
    -750,   768,  -750,   440,   444,   458,   423,   112,   112,   318,
     120,   425,   319,   120,   337,    22,  -750,  -750,   -94,   329,
    -750,  -750,   184,   426,   -14,  -750,   335,   334,   336,   346,
     349,   351,   352,  -750,  -750,   -30,     7,    49,   354,   355,
     361,    23,  -750,   362,   768,   338,   -16,   768,   768,  -750,
     768,   768,  -750,  -750,   347,   768,   768,   768,   768,   768,
     435,   469,   768,  -750,   455,  -750,  -750,  -750,   768,   768,
    -750,  -750,   768,   768,   768,   490,  -750,  -750,   768,   768,
     768,   768,   768,   768,   768,   768,   768,   768,   768,   768,
     768,   768,   768,   768,   768,   768,   768,   768,   768,   768,
    1820,   492,   513,  -750,   516,   768,   768,  1820,   151,   517,
    -750,   518,  1820,  -750,  -750,  -750,  -750,   371,   372,  -750,
    -750,   521,  -750,  -750,  -750,   -74,  -750,   453,  -750,   184,
    -750,  -750,  -750,  -750,  -750,  -750,  -750,   522,  -750,  -750,
     983,   491,  -750,  -750,  -750,    30,   525,  -750,  -750,  -750,
    -750,  -750,  -750,  -750,   184,  -750,   184,   414,  -750,  -750,
    -750,  -750,  -750,  -750,   493,    39,   378,  -750,  1570,    48,
      -4,  1820,  1820,  1790,  1820,  1820,  -750,   881,  1119,  1590,
    1610,  1139,   528,   385,  1159,   535,  1630,  1685,  1179,  1705,
    1199,   393,   925,  1236,  1099,  1371,  1507,  1643,   674,   674,
     377,   377,   377,   377,   320,   320,    79,    79,  -750,  -750,
    -750,  1820,  1820,  1820,  -750,  -750,  -750,  1820,  1820,  -750,
    -750,  -750,  -750,   402,   417,   421,   112,   129,   120,   494,
    -750,  -750,   -70,   594,  -750,   681,   594,   768,   405,  -750,
       9,   534,    30,  -750,   422,  -750,  -750,  -750,  -750,  -750,
    -750,   514,    46,  -750,   549,  -750,  -750,  -750,   768,  -750,
    -750,   768,   768,  -750,  -750,  -750,  -750,   424,   768,   768,
    -750,   559,  -750,  -750,   768,  -750,  -750,  -750,   427,   548,
    -750,  -750,  -750,   234,   530,  1787,   552,   459,  -750,  -750,
    1916,   474,  -750,  1820,    42,   572,  -750,   574,     3,  -750,
     484,   546,  -750,    23,  -750,  -750,  -750,   550,   430,  1219,
    1256,  1276,   445,  -750,  1296,  1316,   433,  1820,   120,   540,
     112,   112,  -750,  -750,  -750,  -750,  -750,   563,   591,  -750,
     449,   768,   263,   588,  -750,   569,   570,   706,  -750,  -750,
     459,   551,   573,   585,  -750,   454,  -750,  -750,  -750,   613,
     475,  -750,    25,    23,  -750,  -750,  -750,  -750,  -750,   768,
    -750,  -750,  -750,  -750,   477,   427,   560,   768,  -750,  1336,
    -750,   768,   599,   495,  -750,   531,  -750,   768,    42,   768,
     487,  -750,  -750,   541,  -750,    36,    23,  1356,   120,   582,
     634,  1820,   216,  1393,   768,  -750,   531,   611,  -750,   452,
    1413,  -750,  1433,  -750,  -750,   639,  -750,  -750,    80,  -750,
    -750,   768,   614,   636,  -750,  1453,   117,   768,   592,  -750,
    -750,    42,  -750,  -750,  1473,   768,  -750,  -750,  -750,  -750,
    -750,  -750,  1493,  -750,  -750,  -750,  -750,  1530,   595,  -750,
    -750,   619,   818,    32,   643,   606,  -750,  -750,  -750,  -750,
    -750,   624,   627,   184,   628,  -750,  -750,  -750,   629,   630,
     631,  -750,    84,  -750,  -750,   632,    19,  -750,  -750,  -750,
     818,   612,   633,   -43,  -750,   648,  -750,    14,    29,  -750,
    -750,   637,  -750,   671,   672,  -750,   649,   650,   651,   652,
    -750,  -750,   -79,    84,   653,   655,    84,   657,  -750,  -750,
    -750,  -750,   656,   691,   607,   568,   571,   575,   818,   576,
    -750,   768,    15,  -750,    -1,  -750,    17,    88,    91,    29,
      29,  -750,    84,   123,    29,   -55,    84,   648,   577,   818,
    -750,   697,  -750,  -750,  -750,  -750,   687,  -750,  1725,   578,
     583,   721,  -750,   672,  -750,   689,   690,   584,   698,   701,
     586,   587,   593,  -750,  -750,  -750,   131,   607,  -750,   685,
     741,    67,  -750,   744,  -750,  -750,  -750,    29,    29,  -750,
      29,    29,  -750,  -750,  -750,  -750,  -750,  -750,  -750,  -750,
     745,  -750,   600,   601,   602,   603,   604,    67,  -750,  -750,
    -750,   475,   -43,   605,   608,   609,   610,  -750,    67,  -750,
    -750,  -750,  -750,  -750,  -750,   475,  -750,  -750,   475,  -750
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,    57,    10,     8,   347,   341,     0,     2,    60,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    58,    11,
       0,     0,     0,     0,     9,   358,     0,   348,   351,     0,
     342,   343,     0,     0,     0,     0,    77,     0,    79,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   213,   214,     0,     0,     0,    81,     0,   114,     0,
      70,    59,    62,    68,     0,    61,    64,    65,    66,    67,
      63,    69,     0,    16,     0,     0,     0,     0,    17,     0,
       0,     0,    19,    46,     0,     0,     0,     0,     0,     0,
      51,     0,     0,     0,   171,   172,   173,   174,   220,   175,
     176,   177,   178,   220,     0,     0,     0,   364,   375,   363,
     371,   373,     0,     0,   358,   352,   371,   373,     0,     0,
     344,   111,   330,     0,     0,     0,     0,     7,    84,   190,
       0,     0,     0,     0,     0,     0,     0,     0,   212,   215,
     215,    94,     0,     0,     0,    88,   180,   179,   113,     0,
       0,    40,     0,   248,   265,     0,     0,     0,     0,     0,
       0,     0,     0,   249,   261,     0,     0,   218,   218,   218,
     218,     0,     0,   218,   218,     0,     0,     0,     0,     0,
       0,    14,     0,    49,    31,    47,    32,    18,    33,    23,
       0,    36,     0,    37,    52,    38,    54,    39,    42,    12,
     181,     0,   182,     0,     0,     0,     0,     0,     0,     0,
     359,     0,     0,   346,     0,     0,    90,    91,     0,     0,
      60,   193,     0,     0,   187,   192,     0,     0,     0,     0,
       0,     0,     0,   207,   209,   187,   187,   215,     0,     0,
       0,     0,    94,     0,     0,     0,     0,     0,     0,    13,
       0,     0,   226,   222,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   251,     0,   250,   252,   253,     0,     0,
     269,   270,     0,     0,     0,     0,   225,   227,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      25,     0,     0,    45,     0,     0,     0,    22,     0,     0,
      55,     0,   221,   220,   220,   220,   369,     0,     0,   353,
     366,   376,   365,   372,   374,     0,   345,   286,   108,     0,
     291,   296,   110,   109,   332,   329,   331,     0,    74,    76,
     349,   199,   195,   188,   186,     0,     0,    93,    71,    72,
      83,   112,   205,   206,     0,   210,     0,   215,   216,    86,
      87,    80,    96,    99,     0,    95,     0,    73,     0,     0,
       0,    27,    28,    43,    29,    30,   223,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   246,   245,   243,   242,   241,   235,   236,
     239,   240,   237,   238,   233,   234,   231,   232,   228,   229,
     230,    15,    26,    24,    50,    48,    44,    20,    21,    35,
      34,    53,    56,     0,     0,     0,     0,   360,   361,     0,
     356,   354,     0,   310,   299,     0,   310,     0,     0,    85,
       0,     0,   190,   191,     0,   208,   211,   217,   102,    98,
     101,     0,     0,    82,     0,    89,   350,    41,     0,   256,
     264,     0,     0,   260,   262,   247,   224,     0,     0,     0,
     255,     0,   271,   254,     0,   183,   184,   185,   377,   374,
     367,   357,   355,     0,     0,   310,     0,   275,   111,   317,
       0,   318,   297,   335,   336,     0,   203,     0,     0,   201,
       0,     0,    92,     0,   106,    97,   100,     0,     0,     0,
       0,     0,     0,   219,     0,     0,     0,   244,   378,     0,
       0,     0,   301,   302,   303,   304,   305,   307,     0,   311,
       0,     0,     0,     0,   313,     0,   277,     0,   316,   319,
     275,     0,   339,     0,   333,     0,   204,   200,   202,     0,
     187,   196,     0,     0,   104,   115,   257,   258,   259,     0,
     266,   267,   268,   370,     0,   377,     0,     0,   309,     0,
     312,     0,     0,   279,   300,   281,   111,     0,   336,     0,
       0,    75,   220,     0,   103,     0,     0,     0,   362,     0,
       0,   308,   310,     0,     0,   278,   281,     0,   292,     0,
       0,   337,     0,   334,   197,     0,   194,   107,     0,   263,
     368,     0,     0,     0,   274,     0,   285,     0,     0,   298,
     340,   336,   220,   105,     0,     0,   314,   276,   282,   283,
     284,   287,     0,   293,   338,   198,   306,     0,     0,   280,
     324,   310,   161,     0,     0,   138,   163,   164,   165,   166,
     167,     0,     0,     0,     0,   149,   150,   155,     0,     0,
       0,   147,     0,   117,   119,     0,     0,   144,   152,   160,
     162,     0,     0,     0,   325,   321,   315,     0,     0,   157,
     220,     0,   145,     0,     0,   116,     0,     0,     0,     0,
     122,   137,   187,     0,   139,     0,     0,     0,   159,   288,
     220,   148,     0,     0,   273,     0,     0,     0,   161,     0,
     168,     0,     0,   131,     0,   135,     0,     0,     0,     0,
       0,   140,     0,   187,     0,   187,     0,   321,     0,   161,
     320,     0,   322,   151,   120,   121,     0,   154,     0,   116,
       0,     0,   133,     0,   134,     0,     0,     0,     0,     0,
       0,     0,     0,   136,   142,   141,   187,   273,   153,     0,
       0,   170,   158,     0,   146,   132,   118,     0,     0,   123,
       0,     0,   124,   125,   130,   143,   322,   326,   272,   220,
       0,   294,     0,     0,     0,     0,     0,   170,   322,   169,
     323,   187,     0,     0,     0,     0,     0,   289,   170,   295,
     156,   127,   126,   128,   129,   187,   327,   290,   187,   328
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -750,  -750,   -73,  -750,  -750,  -750,  -750,   503,  -750,  -750,
    -750,  -750,  -750,  -750,   515,  -750,  -750,   554,  -750,  -750,
    -750,  -750,   533,  -750,  -464,  -750,  -750,  -750,  -750,  -457,
     -13,  -750,  -649,  1234,   110,    55,  -750,  -750,  -750,  -628,
      70,  -750,  -750,   109,  -750,  -750,  -750,  -671,  -750,     4,
    -749,  -750,  -648,   -12,  -220,  -750,   345,  -750,   439,  -750,
    -750,  -750,  -750,  -750,  -750,   287,  -750,  -750,  -750,  -750,
    -750,  -750,  -130,   122,  -750,   -89,  -750,   -76,  -750,  -750,
      33,   246,  -750,  -750,   193,  -750,  -750,  -750,  -750,  -750,
    -750,  -750,  -750,  -750,  -750,  -750,  -750,  -750,  -750,  -477,
     356,  -750,  -750,    64,  -702,  -750,  -750,  -750,  -750,  -750,
    -750,  -750,  -750,  -750,  -750,  -522,  -750,  -750,  -750,  -750,
     763,  -750,  -750,  -750,  -750,  -750,   555,   -24,  -750,   680,
     -23,  -750,  -750,   230
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     6,   128,    11,    12,     9,    10,    19,    93,   249,
     187,   186,   184,   195,   197,     7,     8,    18,    61,   142,
     220,   245,   240,   241,   365,   503,   586,   553,    62,   214,
     332,   144,   663,   664,   665,   691,   714,   666,   716,   692,
     667,   668,   712,   669,   681,   708,   670,   671,   672,   709,
     781,   103,   148,    64,   722,    65,   223,   224,   225,   341,
     442,   550,   606,   441,   498,   499,    66,    67,   235,    68,
     236,    69,   238,   263,   264,   710,   201,   254,   260,   512,
     732,   536,   573,   596,   598,   631,   333,   433,   638,   727,
     805,   435,   618,   640,   791,   436,   541,   488,   530,   486,
     487,   491,   540,   704,   761,   643,   702,   788,   808,    70,
     215,   336,   437,   580,   494,   544,   578,    15,    16,    30,
      31,   118,    13,    14,    71,    72,    27,    28,   432,   112,
     113,   521,   426,   519
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,   181,   151,   115,   345,    63,   119,   496,   533,   200,
     239,   191,   247,   496,   202,   354,   356,   127,   685,   739,
     741,   685,  -189,   694,    20,   701,   334,   362,   363,   449,
     450,   537,    29,   685,   221,   715,   674,   736,   797,   552,
     449,   450,    25,   449,   450,  -189,   542,   303,   310,   806,
     505,   506,   310,   237,    25,   456,   601,   337,   759,   338,
     229,   230,   430,   232,   234,   723,   481,   744,   725,   695,
     243,   652,   344,   695,   787,   652,   721,   779,   431,   252,
     253,   335,   482,   780,   449,   450,   798,   250,   685,   585,
      17,   675,   685,   104,   766,   685,   344,   222,   756,   634,
     755,   123,   124,   276,   277,   105,   300,   358,   146,   147,
     107,   294,   295,   296,   307,   613,   107,   114,   705,   599,
      26,   344,   608,   353,   320,   312,   106,   121,    21,    22,
      23,   122,    26,   320,   364,   125,   451,   344,   686,   687,
     688,   689,   745,   746,   800,   748,   749,   451,   543,   342,
     451,   126,   742,   248,   419,   420,   547,   507,   344,   497,
     355,   659,   660,   660,   644,   497,   233,   660,   368,   662,
     743,   371,   372,   696,   374,   375,   452,   660,   584,   377,
     378,   379,   380,   381,   317,   318,   384,   452,   127,   607,
     452,   451,   386,   387,   304,   311,   388,   389,   390,   311,
     357,   455,   392,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,   423,   424,   425,   447,   251,   417,
     418,   452,   660,   623,   129,   130,   660,   153,   154,   660,
     108,   131,   132,   109,   110,   111,   108,   133,   134,   109,
     116,   117,   135,   532,   321,   612,   434,   322,   323,   324,
     628,   629,   630,   321,   155,   156,   322,   323,   479,   136,
     137,   157,   158,   159,   344,   138,   754,     1,     2,     3,
     139,   445,   344,   446,   775,   160,   161,   162,     4,   140,
     265,   266,   267,   141,   163,   270,   271,     5,   143,   145,
     164,   149,   150,   152,   182,   522,   523,   524,   525,   526,
     527,   528,   165,   183,   185,   188,   189,   166,   167,   168,
     169,   170,   171,   172,   199,   190,   192,    63,   193,   207,
     583,   173,   194,   174,   522,   523,   524,   525,   526,   527,
     528,   327,   196,   198,   203,   204,   115,   205,   206,   175,
     292,   293,   294,   295,   296,   176,   177,   485,   208,   490,
     485,   493,   278,   209,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   509,   178,   216,   510,   511,   529,   210,   212,
     179,   180,   514,   515,   153,   154,   328,   213,   517,   217,
     218,   298,   219,   478,   329,   290,   291,   292,   293,   294,
     295,   296,   330,   226,   227,   228,   529,   231,   237,    47,
     242,   155,   156,   244,   246,   255,   256,   257,   157,   158,
     159,   258,   301,   302,   259,   308,   261,   262,   268,   331,
     269,   272,   160,   161,   162,    21,    22,    23,   309,   273,
     313,   163,   274,   275,   314,   569,   327,   164,    58,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   315,   165,
     319,   326,   382,   383,   166,   167,   168,   169,   170,   171,
     172,   316,   339,   587,   325,   343,   346,   347,   173,   348,
     174,   591,   385,   604,   391,   593,   414,   564,   565,   349,
     376,   600,   350,   602,   351,   352,   175,   359,   360,   153,
     154,   619,   176,   177,   361,   367,   305,   415,   615,   329,
     416,   421,   422,   427,   428,   429,   438,   330,   440,   444,
     448,   453,   464,   635,    47,   624,   155,   156,   465,   467,
     178,   632,   299,   157,   158,   159,   473,   179,   180,   637,
     500,   504,   480,   508,   331,   475,   495,   160,   161,   162,
      21,    22,    23,   516,   520,   535,   163,   531,   534,   539,
     476,   799,   164,    58,   477,   502,   545,   513,   546,   518,
     679,   549,   551,   555,   165,   807,   562,   554,   809,   166,
     167,   168,   169,   170,   171,   172,   559,   153,   154,   563,
     566,   567,   568,   173,   570,   174,   571,   581,   572,   576,
     577,   728,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   175,   579,   582,   155,   156,   344,   176,   177,   588,
     673,   483,   158,   159,   484,   738,   594,   590,   597,   603,
     605,   610,   595,  -116,   611,   160,   161,   162,   617,   622,
     633,   625,   626,   642,   163,   178,   532,   306,   673,   676,
     164,   677,   179,   180,   678,   680,   682,   683,   684,   693,
     700,   699,   165,   703,   711,   713,   685,   166,   167,   168,
     169,   170,   171,   172,   153,   154,   717,   718,   719,   720,
    -116,   173,   724,   174,   726,   730,   673,   489,   286,   287,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   175,
     327,   155,   156,   731,   729,   176,   177,   673,   157,   158,
     159,   733,   760,   762,   734,   765,   767,   768,   735,   737,
     758,  -138,   160,   161,   162,   770,   764,   769,   771,   772,
     773,   163,   574,   178,   777,   778,   774,   164,   782,   790,
     179,   180,   373,   792,   793,   794,   795,   796,   801,   165,
     369,   802,   803,   804,   166,   167,   168,   169,   170,   171,
     172,   153,   154,   329,   340,   366,   697,   753,   173,   698,
     174,   330,   740,   789,   443,   548,   575,   501,    47,   616,
     776,   757,   492,   120,   211,   589,   175,     0,   155,   156,
       0,   370,   176,   177,     0,   157,   158,   159,   331,     0,
       0,     0,     0,     0,    21,    22,    23,     0,     0,   160,
     161,   162,   645,     0,     0,     0,     0,    58,   163,     0,
     178,     0,     0,     0,   164,     0,     0,   179,   180,     0,
       0,     0,     0,     0,     0,     0,   165,     0,     0,     0,
       0,   166,   167,   168,   169,   170,   171,   172,     0,   646,
     647,   648,   649,   650,     0,   173,     0,   174,     0,     0,
       0,     0,   651,     0,   652,     0,     0,     0,     0,     0,
       0,     0,     0,   175,     0,   653,     0,     0,     0,   176,
     177,     0,     0,     0,     0,     0,   278,     0,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   654,   178,   655,    20,
       0,     0,   656,     0,   179,   180,    21,    22,    23,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   657,
     278,   474,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     658,    32,    33,    34,   659,     0,   660,     0,     0,     0,
     661,     0,   662,     0,     0,     0,    35,    36,    37,    38,
      39,     0,    40,    41,    42,    43,     0,    20,     0,     0,
       0,     0,     0,     0,    44,    45,    46,    47,     0,     0,
       0,     0,     0,     0,     0,    48,    49,    50,    51,    52,
      53,    54,     0,     0,     0,     0,    55,    56,    57,   439,
       0,     0,     0,    21,    22,    23,     0,     0,     0,    32,
      33,    34,   458,     0,   459,     0,    58,     0,     0,     0,
       0,     0,     0,     0,    35,    36,    37,    38,    39,    59,
      40,    41,    42,    43,     0,  -349,     0,     0,     0,     0,
       0,     0,    44,    45,    46,    47,     0,    60,     0,     0,
       0,     0,     0,    48,    49,    50,    51,    52,    53,    54,
       0,     0,     0,     0,    55,    56,    57,     0,     0,     0,
       0,    21,    22,    23,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    58,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,    60,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,     0,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,     0,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,     0,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,     0,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   278,     0,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   460,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   463,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   466,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   470,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   472,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   278,   556,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,     0,     0,   278,   557,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   278,   558,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   278,   560,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   278,   561,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   278,   592,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   278,   609,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,     0,     0,     0,   278,   614,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,   620,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,   621,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,   627,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,   636,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   278,   639,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,     0,     0,
       0,     0,     0,   641,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     278,   297,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     278,   454,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     278,   461,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
       0,   462,     0,     0,     0,     0,     0,     0,     0,    73,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   468,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    73,     0,     0,     0,     0,     0,
       0,     0,   278,    74,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,     0,     0,   532,     0,   457,     0,    74,     0,
       0,     0,    75,     0,     0,   278,   469,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,     0,   471,    75,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      76,     0,     0,     0,     0,     0,   763,    77,    78,    79,
      80,    81,   -43,    82,    83,    84,     0,     0,    85,    86,
       0,    87,    88,    89,     0,    76,   690,     0,    90,    91,
      92,     0,    77,    78,    79,    80,    81,     0,    82,    83,
      84,   706,   707,    85,    86,     0,    87,    88,    89,     0,
       0,     0,     0,    90,    91,    92,     0,   690,     0,     0,
     690,   278,   538,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   747,   750,   751,   752,     0,   690,     0,   706,     0,
     690,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   783,   784,     0,   785,   786
};

static const yytype_int16 yycheck[] =
{
      12,    77,    75,    27,   224,    18,    29,     4,   485,    98,
     140,    87,    10,     4,   103,   235,   236,     4,     4,     4,
      21,     4,    36,     4,     4,   673,     4,     4,     5,     4,
       5,   488,    58,     4,     4,   684,     4,   708,   787,   503,
       4,     5,    58,     4,     5,    59,     4,     4,     4,   798,
       4,     5,     4,     4,    58,    59,   578,   151,   729,   153,
     133,   134,   136,   136,   137,   693,   136,   716,   696,    54,
     143,    56,   151,    54,   776,    56,   155,    10,   152,   155,
     156,    59,   152,    16,     4,     5,   788,    10,     4,   553,
       0,    59,     4,    37,   743,     4,   151,    67,   726,   621,
     155,    49,    50,   179,   180,    37,   182,   237,   151,   152,
       4,    32,    33,    34,   190,   592,     4,    58,   104,   576,
     136,   151,   586,   153,     4,   201,    37,    58,   108,   109,
     110,    58,   136,     4,   111,    37,   111,   151,    54,    55,
      56,    57,    54,    55,   792,    54,    55,   111,   106,   222,
     111,    37,   153,   151,     3,     4,   153,   111,   151,   156,
     153,   146,   148,   148,   641,   156,   153,   148,   244,   154,
     153,   247,   248,   154,   250,   251,   151,   148,   153,   255,
     256,   257,   258,   259,   207,   208,   262,   151,     4,   153,
     151,   111,   268,   269,   151,   151,   272,   273,   274,   151,
     151,   153,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   313,   314,   315,   357,   151,   305,
     306,   151,   148,   153,    58,    37,   148,     3,     4,   148,
     134,    37,    37,   137,   138,   139,   134,    37,    37,   137,
     138,   139,    37,    37,   134,    39,   329,   137,   138,   139,
     143,   144,   145,   134,    30,    31,   137,   138,   139,    37,
      37,    37,    38,    39,   151,    37,   153,   130,   131,   132,
      37,   354,   151,   356,   153,    51,    52,    53,   141,    37,
     168,   169,   170,    37,    60,   173,   174,   150,    37,    37,
      66,   140,     4,     4,     4,    71,    72,    73,    74,    75,
      76,    77,    78,     4,     4,     4,     4,    83,    84,    85,
      86,    87,    88,    89,   117,     4,     4,   340,     3,    16,
     550,    97,     4,    99,    71,    72,    73,    74,    75,    76,
      77,     4,     4,     4,     4,     4,   370,     4,     4,   115,
      30,    31,    32,    33,    34,   121,   122,   433,    16,   435,
     436,   437,    15,    59,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,   458,   149,     4,   461,   462,   153,   152,    59,
     156,   157,   468,   469,     3,     4,    59,   152,   474,     4,
       4,    10,     4,   426,    67,    28,    29,    30,    31,    32,
      33,    34,    75,     4,     4,     4,   153,     4,     4,    82,
      37,    30,    31,    37,    58,    37,    37,    37,    37,    38,
      39,    37,   151,   151,    37,   151,    37,    37,    37,   102,
      37,    37,    51,    52,    53,   108,   109,   110,   151,    37,
      10,    60,    37,    37,    10,   531,     4,    66,   121,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    10,    78,
     152,   152,    37,     4,    83,    84,    85,    86,    87,    88,
      89,    58,   153,   559,    59,    59,   151,   153,    97,   153,
      99,   567,    37,   582,     4,   571,     4,   520,   521,   153,
     153,   577,   153,   579,   153,   153,   115,   153,   153,     3,
       4,    59,   121,   122,   153,   153,    10,     4,   594,    67,
       4,     4,     4,   152,   152,     4,     4,    75,    37,     4,
      37,   153,     4,   622,    82,   611,    30,    31,   153,     4,
     149,   617,   151,    37,    38,    39,   153,   156,   157,   625,
      16,    37,    58,     4,   102,   153,   151,    51,    52,    53,
     108,   109,   110,     4,    16,   106,    60,    37,    16,    95,
     153,   791,    66,   121,   153,   153,     4,   153,     4,   152,
     653,    97,    36,   153,    78,   805,   153,    37,   808,    83,
      84,    85,    86,    87,    88,    89,   151,     3,     4,    59,
      37,    10,   153,    97,    16,    99,    37,   153,    38,    58,
      37,   700,     6,     7,     8,     9,    10,    11,    12,    13,
      14,   115,    37,    10,    30,    31,   151,   121,   122,   152,
     642,    37,    38,    39,    40,   711,    37,    77,   107,   152,
      99,    59,   147,    37,    10,    51,    52,    53,    37,    10,
      58,    37,    16,    58,    60,   149,    37,   151,   670,    16,
      66,    37,   156,   157,    37,    37,    37,    37,    37,    37,
      37,    59,    78,    25,    37,     4,     4,    83,    84,    85,
      86,    87,    88,    89,     3,     4,    37,    37,    37,    37,
      37,    97,    37,    99,    37,     4,   708,    16,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,   115,
       4,    30,    31,   106,    58,   121,   122,   729,    37,    38,
      39,   153,    25,    36,   153,     4,    37,    37,   153,   153,
     153,   153,    51,    52,    53,    37,   153,   153,    37,   153,
     153,    60,    36,   149,    59,     4,   153,    66,     4,     4,
     156,   157,   249,   153,   153,   153,   153,   153,   153,    78,
     245,   153,   153,   153,    83,    84,    85,    86,    87,    88,
      89,     3,     4,    67,   220,   242,   666,   722,    97,   670,
      99,    75,   712,   779,   345,   498,   540,   442,    82,   596,
     757,   727,   436,    30,   114,   565,   115,    -1,    30,    31,
      -1,   246,   121,   122,    -1,    37,    38,    39,   102,    -1,
      -1,    -1,    -1,    -1,   108,   109,   110,    -1,    -1,    51,
      52,    53,     4,    -1,    -1,    -1,    -1,   121,    60,    -1,
     149,    -1,    -1,    -1,    66,    -1,    -1,   156,   157,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    78,    -1,    -1,    -1,
      -1,    83,    84,    85,    86,    87,    88,    89,    -1,    41,
      42,    43,    44,    45,    -1,    97,    -1,    99,    -1,    -1,
      -1,    -1,    54,    -1,    56,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    67,    -1,    -1,    -1,   121,
     122,    -1,    -1,    -1,    -1,    -1,    15,    -1,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    98,   149,   100,     4,
      -1,    -1,   104,    -1,   156,   157,   108,   109,   110,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
     142,    46,    47,    48,   146,    -1,   148,    -1,    -1,    -1,
     152,    -1,   154,    -1,    -1,    -1,    61,    62,    63,    64,
      65,    -1,    67,    68,    69,    70,    -1,     4,    -1,    -1,
      -1,    -1,    -1,    -1,    79,    80,    81,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    90,    91,    92,    93,    94,
      95,    96,    -1,    -1,    -1,    -1,   101,   102,   103,    36,
      -1,    -1,    -1,   108,   109,   110,    -1,    -1,    -1,    46,
      47,    48,   151,    -1,   153,    -1,   121,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    61,    62,    63,    64,    65,   134,
      67,    68,    69,    70,    -1,   140,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    80,    81,    82,    -1,   152,    -1,    -1,
      -1,    -1,    -1,    90,    91,    92,    93,    94,    95,    96,
      -1,    -1,    -1,    -1,   101,   102,   103,    -1,    -1,    -1,
      -1,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   121,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   152,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,    -1,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   153,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    15,   153,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,   153,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    -1,    -1,   153,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,   151,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   151,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      15,   151,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      -1,   151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    15,    38,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    37,    -1,    36,    -1,    38,    -1,
      -1,    -1,    67,    -1,    -1,    15,   151,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,   151,    67,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     105,    -1,    -1,    -1,    -1,    -1,   151,   112,   113,   114,
     115,   116,   117,   118,   119,   120,    -1,    -1,   123,   124,
      -1,   126,   127,   128,    -1,   105,   662,    -1,   133,   134,
     135,    -1,   112,   113,   114,   115,   116,    -1,   118,   119,
     120,   677,   678,   123,   124,    -1,   126,   127,   128,    -1,
      -1,    -1,    -1,   133,   134,   135,    -1,   693,    -1,    -1,
     696,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   717,   718,   719,   720,    -1,   722,    -1,   724,    -1,
     726,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   767,   768,    -1,   770,   771
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   130,   131,   132,   141,   150,   159,   173,   174,   163,
     164,   161,   162,   280,   281,   275,   276,     0,   175,   165,
       4,   108,   109,   110,   211,    58,   136,   284,   285,    58,
     277,   278,    46,    47,    48,    61,    62,    63,    64,    65,
      67,    68,    69,    70,    79,    80,    81,    82,    90,    91,
      92,    93,    94,    95,    96,   101,   102,   103,   121,   134,
     152,   176,   186,   188,   211,   213,   224,   225,   227,   229,
     267,   282,   283,     4,    38,    67,   105,   112,   113,   114,
     115,   116,   118,   119,   120,   123,   124,   126,   127,   128,
     133,   134,   135,   166,     6,     7,     8,     9,    10,    11,
      12,    13,    14,   209,    37,    37,    37,     4,   134,   137,
     138,   139,   287,   288,    58,   285,   138,   139,   279,   288,
     278,    58,    58,    49,    50,    37,    37,     4,   160,    58,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,   177,    37,   189,    37,   151,   152,   210,   140,
       4,   160,     4,     3,     4,    30,    31,    37,    38,    39,
      51,    52,    53,    60,    66,    78,    83,    84,    85,    86,
      87,    88,    89,    97,    99,   115,   121,   122,   149,   156,
     157,   235,     4,     4,   170,     4,   169,   168,     4,     4,
       4,   235,     4,     3,     4,   171,     4,   172,     4,   117,
     233,   234,   233,     4,     4,     4,     4,    16,    16,    59,
     152,   287,    59,   152,   187,   268,     4,     4,     4,     4,
     178,     4,    67,   214,   215,   216,     4,     4,     4,   160,
     160,     4,   160,   153,   160,   226,   228,     4,   230,   230,
     180,   181,    37,   160,    37,   179,    58,    10,   151,   167,
      10,   151,   235,   235,   235,    37,    37,    37,    37,    37,
     236,    37,    37,   231,   232,   231,   231,   231,    37,    37,
     231,   231,    37,    37,    37,    37,   235,   235,    15,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,   151,    10,   151,
     235,   151,   151,     4,   151,    10,   151,   235,   151,   151,
       4,   151,   235,    10,    10,    10,    58,   288,   288,   152,
       4,   134,   137,   138,   139,    59,   152,     4,    59,    67,
      75,   102,   188,   244,     4,    59,   269,   151,   153,   153,
     175,   217,   160,    59,   151,   212,   151,   153,   153,   153,
     153,   153,   153,   153,   212,   153,   212,   151,   230,   153,
     153,   153,     4,     5,   111,   182,   180,   153,   235,   172,
     284,   235,   235,   165,   235,   235,   153,   235,   235,   235,
     235,   235,    37,     4,   235,    37,   235,   235,   235,   235,
     235,     4,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,     4,     4,     4,   235,   235,     3,
       4,     4,     4,   233,   233,   233,   290,   152,   152,     4,
     136,   152,   286,   245,   160,   249,   253,   270,     4,    36,
      37,   221,   218,   216,     4,   160,   160,   230,    37,     4,
       5,   111,   151,   153,   151,   153,    59,    36,   151,   153,
     153,   151,   151,   153,     4,   153,   153,     4,   151,   151,
     153,   151,   153,   153,    16,   153,   153,   153,   288,   139,
      58,   136,   152,    37,    40,   235,   257,   258,   255,    16,
     235,   259,   258,   235,   272,   151,     4,   156,   222,   223,
      16,   214,   153,   183,    37,     4,     5,   111,     4,   235,
     235,   235,   237,   153,   235,   235,     4,   235,   152,   291,
      16,   289,    71,    72,    73,    74,    75,    76,    77,   153,
     256,    37,    37,   257,    16,   106,   239,   187,    16,    95,
     260,   254,     4,   106,   273,     4,     4,   153,   223,    97,
     219,    36,   182,   185,    37,   153,   153,   153,   153,   151,
     153,   153,   153,    59,   288,   288,    37,    10,   153,   235,
      16,    37,    38,   240,    36,   239,    58,    37,   274,    37,
     271,   153,    10,   212,   153,   182,   184,   235,   152,   291,
      77,   235,   153,   235,    37,   147,   241,   107,   242,   187,
     235,   273,   235,   152,   233,    99,   220,   153,   182,   153,
      59,    10,    39,   257,   153,   235,   242,    37,   250,    59,
     153,   153,    10,   153,   235,    37,    16,   153,   143,   144,
     145,   243,   235,    58,   273,   233,   153,   235,   246,   153,
     251,   153,    58,   263,   257,     4,    41,    42,    43,    44,
      45,    54,    56,    67,    98,   100,   104,   121,   142,   146,
     148,   152,   154,   190,   191,   192,   195,   198,   199,   201,
     204,   205,   206,   211,     4,    59,    16,    37,    37,   160,
      37,   202,    37,    37,    37,     4,    54,    55,    56,    57,
     191,   193,   197,    37,     4,    54,   154,   192,   201,    59,
      37,   210,   264,    25,   261,   104,   191,   191,   203,   207,
     233,    37,   200,     4,   194,   190,   196,    37,    37,    37,
      37,   155,   212,   197,    37,   197,    37,   247,   233,    58,
       4,   106,   238,   153,   153,   153,   205,   153,   235,     4,
     198,    21,   153,   153,   190,    54,    55,   191,    54,    55,
     191,   191,   191,   193,   153,   155,   197,   261,   153,   205,
      25,   262,    36,   151,   153,     4,   190,    37,    37,   153,
      37,    37,   153,   153,   153,   153,   238,    59,     4,    10,
      16,   208,     4,   191,   191,   191,   191,   262,   265,   207,
       4,   252,   153,   153,   153,   153,   153,   208,   262,   212,
     210,   153,   153,   153,   153,   248,   208,   212,   266,   212
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   158,   159,   159,   159,   159,   159,   160,   162,   161,
     164,   163,   165,   165,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   166,
     167,   166,   166,   166,   168,   168,   168,   169,   169,   170,
     170,   171,   171,   171,   172,   172,   172,   174,   173,   175,
     175,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   177,   176,   176,   178,   176,   176,   176,   179,   176,
     176,   176,   176,   176,   181,   180,   182,   182,   182,   182,
     182,   182,   183,   182,   184,   182,   185,   182,   186,   187,
     187,   187,   188,   188,   189,   188,   190,   191,   191,   192,
     192,   192,   193,   193,   193,   193,   193,   193,   193,   193,
     193,   194,   194,   195,   196,   196,   197,   197,   198,   198,
     198,   198,   198,   198,   199,   200,   199,   201,   201,   201,
     201,   201,   201,   201,   201,   202,   201,   203,   201,   204,
     204,   205,   205,   206,   206,   206,   206,   206,   207,   208,
     208,   209,   209,   209,   209,   209,   209,   209,   209,   210,
     210,   211,   211,   211,   211,   211,   212,   212,   213,   214,
     214,   215,   215,   217,   216,   218,   216,   219,   220,   221,
     221,   222,   222,   223,   223,   224,   225,   225,   226,   226,
     227,   228,   228,   229,   229,   230,   230,   230,   232,   231,
     234,   233,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   236,   237,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   238,   238,   239,   239,   240,   240,   241,   241,
     242,   242,   243,   243,   243,   243,   245,   246,   247,   248,
     244,   249,   250,   251,   252,   244,   253,   254,   244,   255,
     244,   256,   256,   256,   256,   256,   256,   256,   256,   257,
     257,   257,   258,   258,   258,   258,   259,   259,   260,   260,
     261,   261,   262,   262,   263,   264,   265,   266,   263,   267,
     268,   268,   270,   271,   269,   272,   273,   273,   273,   274,
     274,   276,   275,   277,   277,   278,   279,   281,   280,   283,
     282,   284,   284,   285,   285,   285,   286,   286,   287,   287,
     287,   287,   287,   288,   288,   288,   288,   289,   288,   290,
     288,   288,   288,   288,   288,   288,   288,   291,   291
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     1,     0,     2,
       0,     2,     3,     0,     2,     4,     1,     1,     2,     1,
       4,     4,     3,     2,     4,     3,     4,     4,     4,     4,
       4,     2,     2,     2,     4,     4,     2,     2,     2,     2,
       0,     5,     2,     0,     3,     2,     0,     1,     3,     1,
       3,     0,     1,     3,     1,     2,     3,     0,     2,     2,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     4,     4,     4,     8,     4,     1,     1,     1,
       4,     0,     5,     4,     0,     5,     4,     4,     0,     5,
       3,     3,     6,     4,     0,     2,     1,     3,     2,     1,
       3,     2,     0,     5,     0,     7,     0,     6,     4,     2,
       2,     0,     4,     2,     0,     7,     1,     1,     5,     1,
       4,     4,     1,     4,     4,     4,     7,     7,     7,     7,
       4,     1,     3,     4,     2,     1,     3,     1,     1,     2,
       3,     4,     4,     5,     1,     0,     5,     1,     2,     1,
       1,     4,     1,     4,     4,     0,     8,     0,     5,     2,
       1,     0,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     6,     6,     6,     1,     0,     4,     1,
       0,     3,     1,     0,     7,     0,     5,     3,     3,     0,
       3,     1,     2,     1,     2,     4,     4,     3,     3,     1,
       4,     3,     0,     1,     1,     0,     2,     3,     0,     4,
       0,     2,     2,     3,     4,     2,     2,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     5,     3,     3,     4,     1,     1,
       2,     2,     2,     2,     4,     4,     4,     6,     6,     6,
       4,     0,     0,     8,     4,     1,     6,     6,     6,     2,
       2,     4,     3,     0,     4,     0,     4,     0,     1,     0,
       4,     0,     1,     1,     1,     0,     0,     0,     0,     0,
      19,     0,     0,     0,     0,    17,     0,     0,     7,     0,
       5,     1,     1,     1,     1,     1,     6,     1,     3,     3,
       0,     2,     3,     2,     6,    10,     2,     1,     0,     1,
       2,     0,     0,     3,     0,     0,     0,     0,    11,     4,
       0,     2,     0,     0,     6,     1,     0,     3,     5,     0,
       3,     0,     2,     1,     2,     4,     2,     0,     2,     0,
       5,     1,     2,     4,     5,     6,     1,     2,     0,     2,
       4,     4,     8,     1,     1,     3,     3,     0,     9,     0,
       7,     1,     3,     1,     3,     1,     3,     0,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 8: /* $@1: %empty  */
#line 181 "ldgram.y"
                { ldlex_expression(); }
#line 2562 "ldgram.c"
    break;

  case 9: /* defsym_expr: $@1 assignment  */
#line 183 "ldgram.y"
                { ldlex_popstate(); }
#line 2568 "ldgram.c"
    break;

  case 10: /* $@2: %empty  */
#line 188 "ldgram.y"
                {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2577 "ldgram.c"
    break;

  case 11: /* mri_script_file: $@2 mri_script_lines  */
#line 193 "ldgram.y"
                {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2587 "ldgram.c"
    break;

  case 16: /* mri_script_command: NAME  */
#line 208 "ldgram.y"
                        {
			einfo(_("%F%P: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
#line 2595 "ldgram.c"
    break;

  case 17: /* mri_script_command: LIST  */
#line 211 "ldgram.y"
                        {
			config.map_filename = "-";
			}
#line 2603 "ldgram.c"
    break;

  case 20: /* mri_script_command: PUBLIC NAME '=' exp  */
#line 217 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2609 "ldgram.c"
    break;

  case 21: /* mri_script_command: PUBLIC NAME ',' exp  */
#line 219 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2615 "ldgram.c"
    break;

  case 22: /* mri_script_command: PUBLIC NAME exp  */
#line 221 "ldgram.y"
                        { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2621 "ldgram.c"
    break;

  case 23: /* mri_script_command: FORMAT NAME  */
#line 223 "ldgram.y"
                        { mri_format((yyvsp[0].name)); }
#line 2627 "ldgram.c"
    break;

  case 24: /* mri_script_command: SECT NAME ',' exp  */
#line 225 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2633 "ldgram.c"
    break;

  case 25: /* mri_script_command: SECT NAME exp  */
#line 227 "ldgram.y"
                        { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2639 "ldgram.c"
    break;

  case 26: /* mri_script_command: SECT NAME '=' exp  */
#line 229 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2645 "ldgram.c"
    break;

  case 27: /* mri_script_command: ALIGN_K NAME '=' exp  */
#line 231 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2651 "ldgram.c"
    break;

  case 28: /* mri_script_command: ALIGN_K NAME ',' exp  */
#line 233 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2657 "ldgram.c"
    break;

  case 29: /* mri_script_command: ALIGNMOD NAME '=' exp  */
#line 235 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2663 "ldgram.c"
    break;

  case 30: /* mri_script_command: ALIGNMOD NAME ',' exp  */
#line 237 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2669 "ldgram.c"
    break;

  case 33: /* mri_script_command: NAMEWORD NAME  */
#line 241 "ldgram.y"
                        { mri_name((yyvsp[0].name)); }
#line 2675 "ldgram.c"
    break;

  case 34: /* mri_script_command: ALIAS NAME ',' NAME  */
#line 243 "ldgram.y"
                        { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2681 "ldgram.c"
    break;

  case 35: /* mri_script_command: ALIAS NAME ',' INT  */
#line 245 "ldgram.y"
                        { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2687 "ldgram.c"
    break;

  case 36: /* mri_script_command: BASE exp  */
#line 247 "ldgram.y"
                        { mri_base((yyvsp[0].etree)); }
#line 2693 "ldgram.c"
    break;

  case 37: /* mri_script_command: TRUNCATE INT  */
#line 249 "ldgram.y"
                { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2699 "ldgram.c"
    break;

  case 40: /* $@3: %empty  */
#line 253 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2705 "ldgram.c"
    break;

  case 42: /* mri_script_command: START NAME  */
#line 256 "ldgram.y"
                { lang_add_entry ((yyvsp[0].name), false); }
#line 2711 "ldgram.c"
    break;

  case 44: /* ordernamelist: ordernamelist ',' NAME  */
#line 261 "ldgram.y"
                                             { mri_order((yyvsp[0].name)); }
#line 2717 "ldgram.c"
    break;

  case 45: /* ordernamelist: ordernamelist NAME  */
#line 262 "ldgram.y"
                                          { mri_order((yyvsp[0].name)); }
#line 2723 "ldgram.c"
    break;

  case 47: /* mri_load_name_list: NAME  */
#line 268 "ldgram.y"
                        { mri_load((yyvsp[0].name)); }
#line 2729 "ldgram.c"
    break;

  case 48: /* mri_load_name_list: mri_load_name_list ',' NAME  */
#line 269 "ldgram.y"
                                            { mri_load((yyvsp[0].name)); }
#line 2735 "ldgram.c"
    break;

  case 49: /* mri_abs_name_list: NAME  */
#line 274 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2741 "ldgram.c"
    break;

  case 50: /* mri_abs_name_list: mri_abs_name_list ',' NAME  */
#line 276 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2747 "ldgram.c"
    break;

  case 51: /* casesymlist: %empty  */
#line 280 "ldgram.y"
                      { (yyval.name) = NULL; }
#line 2753 "ldgram.c"
    break;

  case 54: /* extern_name_list: NAME  */
#line 287 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2759 "ldgram.c"
    break;

  case 55: /* extern_name_list: extern_name_list NAME  */
#line 289 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2765 "ldgram.c"
    break;

  case 56: /* extern_name_list: extern_name_list ',' NAME  */
#line 291 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2771 "ldgram.c"
    break;

  case 57: /* $@4: %empty  */
#line 295 "ldgram.y"
        { ldlex_script (); }
#line 2777 "ldgram.c"
    break;

  case 58: /* script_file: $@4 ifile_list  */
#line 297 "ldgram.y"
        { ldlex_popstate (); }
#line 2783 "ldgram.c"
    break;

  case 71: /* ifile_p1: TARGET_K '(' NAME ')'  */
#line 318 "ldgram.y"
                { lang_add_target((yyvsp[-1].name)); }
#line 2789 "ldgram.c"
    break;

  case 72: /* ifile_p1: SEARCH_DIR '(' filename ')'  */
#line 320 "ldgram.y"
                { ldfile_add_library_path ((yyvsp[-1].name), false); }
#line 2795 "ldgram.c"
    break;

  case 73: /* ifile_p1: OUTPUT '(' filename ')'  */
#line 322 "ldgram.y"
                { lang_add_output((yyvsp[-1].name), 1); }
#line 2801 "ldgram.c"
    break;

  case 74: /* ifile_p1: OUTPUT_FORMAT '(' NAME ')'  */
#line 324 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2808 "ldgram.c"
    break;

  case 75: /* ifile_p1: OUTPUT_FORMAT '(' NAME ',' NAME ',' NAME ')'  */
#line 327 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2814 "ldgram.c"
    break;

  case 76: /* ifile_p1: OUTPUT_ARCH '(' NAME ')'  */
#line 329 "ldgram.y"
                  { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2820 "ldgram.c"
    break;

  case 77: /* ifile_p1: FORCE_COMMON_ALLOCATION  */
#line 331 "ldgram.y"
                { command_line.force_common_definition = true ; }
#line 2826 "ldgram.c"
    break;

  case 78: /* ifile_p1: FORCE_GROUP_ALLOCATION  */
#line 333 "ldgram.y"
                { command_line.force_group_allocation = true ; }
#line 2832 "ldgram.c"
    break;

  case 79: /* ifile_p1: INHIBIT_COMMON_ALLOCATION  */
#line 335 "ldgram.y"
                { link_info.inhibit_common_definition = true ; }
#line 2838 "ldgram.c"
    break;

  case 81: /* $@5: %empty  */
#line 338 "ldgram.y"
                  { lang_enter_group (); }
#line 2844 "ldgram.c"
    break;

  case 82: /* ifile_p1: GROUP $@5 '(' input_list ')'  */
#line 340 "ldgram.y"
                  { lang_leave_group (); }
#line 2850 "ldgram.c"
    break;

  case 83: /* ifile_p1: MAP '(' filename ')'  */
#line 342 "ldgram.y"
                { lang_add_map((yyvsp[-1].name)); }
#line 2856 "ldgram.c"
    break;

  case 84: /* $@6: %empty  */
#line 344 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2862 "ldgram.c"
    break;

  case 86: /* ifile_p1: NOCROSSREFS '(' nocrossref_list ')'  */
#line 347 "ldgram.y"
                {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2870 "ldgram.c"
    break;

  case 87: /* ifile_p1: NOCROSSREFS_TO '(' nocrossref_list ')'  */
#line 351 "ldgram.y"
                {
		  lang_add_nocrossref_to ((yyvsp[-1].nocrossref));
		}
#line 2878 "ldgram.c"
    break;

  case 88: /* $@7: %empty  */
#line 354 "ldgram.y"
                           { ldlex_expression (); }
#line 2884 "ldgram.c"
    break;

  case 89: /* ifile_p1: EXTERN '(' $@7 extern_name_list ')'  */
#line 355 "ldgram.y"
                        { ldlex_popstate (); }
#line 2890 "ldgram.c"
    break;

  case 90: /* ifile_p1: INSERT_K AFTER NAME  */
#line 357 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 0); }
#line 2896 "ldgram.c"
    break;

  case 91: /* ifile_p1: INSERT_K BEFORE NAME  */
#line 359 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 1); }
#line 2902 "ldgram.c"
    break;

  case 92: /* ifile_p1: REGION_ALIAS '(' NAME ',' NAME ')'  */
#line 361 "ldgram.y"
                { lang_memory_region_alias ((yyvsp[-3].name), (yyvsp[-1].name)); }
#line 2908 "ldgram.c"
    break;

  case 93: /* ifile_p1: LD_FEATURE '(' NAME ')'  */
#line 363 "ldgram.y"
                { lang_ld_feature ((yyvsp[-1].name)); }
#line 2914 "ldgram.c"
    break;

  case 94: /* $@8: %empty  */
#line 367 "ldgram.y"
                { ldlex_inputlist(); }
#line 2920 "ldgram.c"
    break;

  case 95: /* input_list: $@8 input_list1  */
#line 369 "ldgram.y"
                { ldlex_popstate(); }
#line 2926 "ldgram.c"
    break;

  case 96: /* input_list1: NAME  */
#line 373 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2933 "ldgram.c"
    break;

  case 97: /* input_list1: input_list1 ',' NAME  */
#line 376 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2940 "ldgram.c"
    break;

  case 98: /* input_list1: input_list1 NAME  */
#line 379 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2947 "ldgram.c"
    break;

  case 99: /* input_list1: LNAME  */
#line 382 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2954 "ldgram.c"
    break;

  case 100: /* input_list1: input_list1 ',' LNAME  */
#line 385 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2961 "ldgram.c"
    break;

  case 101: /* input_list1: input_list1 LNAME  */
#line 388 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2968 "ldgram.c"
    break;

  case 102: /* @9: %empty  */
#line 391 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 2975 "ldgram.c"
    break;

  case 103: /* input_list1: AS_NEEDED '(' @9 input_list1 ')'  */
#line 394 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2981 "ldgram.c"
    break;

  case 104: /* @10: %empty  */
#line 396 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 2988 "ldgram.c"
    break;

  case 105: /* input_list1: input_list1 ',' AS_NEEDED '(' @10 input_list1 ')'  */
#line 399 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 2994 "ldgram.c"
    break;

  case 106: /* @11: %empty  */
#line 401 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3001 "ldgram.c"
    break;

  case 107: /* input_list1: input_list1 AS_NEEDED '(' @11 input_list1 ')'  */
#line 404 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3007 "ldgram.c"
    break;

  case 112: /* statement_anywhere: ENTRY '(' NAME ')'  */
#line 419 "ldgram.y"
                { lang_add_entry ((yyvsp[-1].name), false); }
#line 3013 "ldgram.c"
    break;

  case 114: /* $@12: %empty  */
#line 421 "ldgram.y"
                          {ldlex_expression ();}
#line 3019 "ldgram.c"
    break;

  case 115: /* statement_anywhere: ASSERT_K $@12 '(' exp ',' NAME ')'  */
#line 422 "ldgram.y"
                { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 3026 "ldgram.c"
    break;

  case 116: /* wildcard_name: NAME  */
#line 428 "ldgram.y"
                        {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 3034 "ldgram.c"
    break;

  case 117: /* wildcard_maybe_exclude: wildcard_name  */
#line 435 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 3045 "ldgram.c"
    break;

  case 118: /* wildcard_maybe_exclude: EXCLUDE_FILE '(' exclude_name_list ')' wildcard_name  */
#line 442 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			}
#line 3056 "ldgram.c"
    break;

  case 120: /* filename_spec: SORT_BY_NAME '(' wildcard_maybe_exclude ')'  */
#line 453 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3065 "ldgram.c"
    break;

  case 121: /* filename_spec: SORT_NONE '(' wildcard_maybe_exclude ')'  */
#line 458 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 3074 "ldgram.c"
    break;

  case 123: /* section_name_spec: SORT_BY_NAME '(' wildcard_maybe_exclude ')'  */
#line 467 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3083 "ldgram.c"
    break;

  case 124: /* section_name_spec: SORT_BY_ALIGNMENT '(' wildcard_maybe_exclude ')'  */
#line 472 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3092 "ldgram.c"
    break;

  case 125: /* section_name_spec: SORT_NONE '(' wildcard_maybe_exclude ')'  */
#line 477 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 3101 "ldgram.c"
    break;

  case 126: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_exclude ')' ')'  */
#line 482 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name_alignment;
			}
#line 3110 "ldgram.c"
    break;

  case 127: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 487 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3119 "ldgram.c"
    break;

  case 128: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 492 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment_name;
			}
#line 3128 "ldgram.c"
    break;

  case 129: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_exclude ')' ')'  */
#line 497 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3137 "ldgram.c"
    break;

  case 130: /* section_name_spec: SORT_BY_INIT_PRIORITY '(' wildcard_maybe_exclude ')'  */
#line 502 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			}
#line 3146 "ldgram.c"
    break;

  case 131: /* sect_flag_list: NAME  */
#line 509 "ldgram.y"
                        {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[0].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[0].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[0].name);
			    }
			  n->valid = false;
			  n->next = NULL;
			  (yyval.flag_info_list) = n;
			}
#line 3168 "ldgram.c"
    break;

  case 132: /* sect_flag_list: sect_flag_list '&' NAME  */
#line 527 "ldgram.y"
                        {
			  struct flag_info_list *n;
			  n = ((struct flag_info_list *) xmalloc (sizeof *n));
			  if ((yyvsp[0].name)[0] == '!')
			    {
			      n->with = without_flags;
			      n->name = &(yyvsp[0].name)[1];
			    }
			  else
			    {
			      n->with = with_flags;
			      n->name = (yyvsp[0].name);
			    }
			  n->valid = false;
			  n->next = (yyvsp[-2].flag_info_list);
			  (yyval.flag_info_list) = n;
			}
#line 3190 "ldgram.c"
    break;

  case 133: /* sect_flags: INPUT_SECTION_FLAGS '(' sect_flag_list ')'  */
#line 548 "ldgram.y"
                        {
			  struct flag_info *n;
			  n = ((struct flag_info *) xmalloc (sizeof *n));
			  n->flag_list = (yyvsp[-1].flag_info_list);
			  n->flags_initialized = false;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
#line 3204 "ldgram.c"
    break;

  case 134: /* exclude_name_list: exclude_name_list wildcard_name  */
#line 561 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 3216 "ldgram.c"
    break;

  case 135: /* exclude_name_list: wildcard_name  */
#line 570 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 3228 "ldgram.c"
    break;

  case 136: /* section_name_list: section_name_list opt_comma section_name_spec  */
#line 581 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3240 "ldgram.c"
    break;

  case 137: /* section_name_list: section_name_spec  */
#line 590 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3252 "ldgram.c"
    break;

  case 138: /* input_section_spec_no_keep: NAME  */
#line 601 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3265 "ldgram.c"
    break;

  case 139: /* input_section_spec_no_keep: sect_flags NAME  */
#line 610 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-1].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3278 "ldgram.c"
    break;

  case 140: /* input_section_spec_no_keep: '[' section_name_list ']'  */
#line 619 "ldgram.y"
                        {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3286 "ldgram.c"
    break;

  case 141: /* input_section_spec_no_keep: sect_flags '[' section_name_list ']'  */
#line 623 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-3].flag_info);
			  lang_add_wild (&tmp, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3299 "ldgram.c"
    break;

  case 142: /* input_section_spec_no_keep: filename_spec '(' section_name_list ')'  */
#line 632 "ldgram.y"
                        {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3307 "ldgram.c"
    break;

  case 143: /* input_section_spec_no_keep: sect_flags filename_spec '(' section_name_list ')'  */
#line 636 "ldgram.y"
                        {
			  (yyvsp[-3].wildcard).section_flag_list = (yyvsp[-4].flag_info);
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3316 "ldgram.c"
    break;

  case 145: /* $@13: %empty  */
#line 645 "ldgram.y"
                        { ldgram_had_keep = true; }
#line 3322 "ldgram.c"
    break;

  case 146: /* input_section_spec: KEEP '(' $@13 input_section_spec_no_keep ')'  */
#line 647 "ldgram.y"
                        { ldgram_had_keep = false; }
#line 3328 "ldgram.c"
    break;

  case 149: /* statement: CREATE_OBJECT_SYMBOLS  */
#line 654 "ldgram.y"
                {
		  lang_add_attribute (lang_object_symbols_statement_enum);
		}
#line 3336 "ldgram.c"
    break;

  case 150: /* statement: CONSTRUCTORS  */
#line 658 "ldgram.y"
                {
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3344 "ldgram.c"
    break;

  case 151: /* statement: SORT_BY_NAME '(' CONSTRUCTORS ')'  */
#line 662 "ldgram.y"
                {
		  constructors_sorted = true;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3353 "ldgram.c"
    break;

  case 153: /* statement: length '(' mustbe_exp ')'  */
#line 668 "ldgram.y"
                {
		  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
		}
#line 3361 "ldgram.c"
    break;

  case 154: /* statement: FILL '(' fill_exp ')'  */
#line 673 "ldgram.y"
                {
		  lang_add_fill ((yyvsp[-1].fill));
		}
#line 3369 "ldgram.c"
    break;

  case 155: /* $@14: %empty  */
#line 677 "ldgram.y"
                { ldlex_expression (); }
#line 3375 "ldgram.c"
    break;

  case 156: /* statement: ASSERT_K $@14 '(' exp ',' NAME ')' separator  */
#line 679 "ldgram.y"
                {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name)));
		}
#line 3384 "ldgram.c"
    break;

  case 157: /* $@15: %empty  */
#line 684 "ldgram.y"
                {
		  ldfile_open_command_file ((yyvsp[0].name));
		}
#line 3392 "ldgram.c"
    break;

  case 163: /* length: QUAD  */
#line 702 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3398 "ldgram.c"
    break;

  case 164: /* length: SQUAD  */
#line 704 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3404 "ldgram.c"
    break;

  case 165: /* length: LONG  */
#line 706 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3410 "ldgram.c"
    break;

  case 166: /* length: SHORT  */
#line 708 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3416 "ldgram.c"
    break;

  case 167: /* length: BYTE  */
#line 710 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3422 "ldgram.c"
    break;

  case 168: /* fill_exp: mustbe_exp  */
#line 715 "ldgram.y"
                {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, "fill value");
		}
#line 3430 "ldgram.c"
    break;

  case 169: /* fill_opt: '=' fill_exp  */
#line 722 "ldgram.y"
                { (yyval.fill) = (yyvsp[0].fill); }
#line 3436 "ldgram.c"
    break;

  case 170: /* fill_opt: %empty  */
#line 723 "ldgram.y"
                { (yyval.fill) = (fill_type *) 0; }
#line 3442 "ldgram.c"
    break;

  case 171: /* assign_op: PLUSEQ  */
#line 728 "ldgram.y"
                        { (yyval.token) = '+'; }
#line 3448 "ldgram.c"
    break;

  case 172: /* assign_op: MINUSEQ  */
#line 730 "ldgram.y"
                        { (yyval.token) = '-'; }
#line 3454 "ldgram.c"
    break;

  case 173: /* assign_op: MULTEQ  */
#line 732 "ldgram.y"
                        { (yyval.token) = '*'; }
#line 3460 "ldgram.c"
    break;

  case 174: /* assign_op: DIVEQ  */
#line 734 "ldgram.y"
                        { (yyval.token) = '/'; }
#line 3466 "ldgram.c"
    break;

  case 175: /* assign_op: LSHIFTEQ  */
#line 736 "ldgram.y"
                        { (yyval.token) = LSHIFT; }
#line 3472 "ldgram.c"
    break;

  case 176: /* assign_op: RSHIFTEQ  */
#line 738 "ldgram.y"
                        { (yyval.token) = RSHIFT; }
#line 3478 "ldgram.c"
    break;

  case 177: /* assign_op: ANDEQ  */
#line 740 "ldgram.y"
                        { (yyval.token) = '&'; }
#line 3484 "ldgram.c"
    break;

  case 178: /* assign_op: OREQ  */
#line 742 "ldgram.y"
                        { (yyval.token) = '|'; }
#line 3490 "ldgram.c"
    break;

  case 181: /* assignment: NAME '=' mustbe_exp  */
#line 752 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name), (yyvsp[0].etree), false));
		}
#line 3498 "ldgram.c"
    break;

  case 182: /* assignment: NAME assign_op mustbe_exp  */
#line 756 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name),
						   exp_binop ((yyvsp[-1].token),
							      exp_nameop (NAME,
									  (yyvsp[-2].name)),
							      (yyvsp[0].etree)), false));
		}
#line 3510 "ldgram.c"
    break;

  case 183: /* assignment: HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 764 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3518 "ldgram.c"
    break;

  case 184: /* assignment: PROVIDE '(' NAME '=' mustbe_exp ')'  */
#line 768 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), false));
		}
#line 3526 "ldgram.c"
    break;

  case 185: /* assignment: PROVIDE_HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 772 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3534 "ldgram.c"
    break;

  case 193: /* $@16: %empty  */
#line 795 "ldgram.y"
                { region = lang_memory_region_lookup ((yyvsp[0].name), true); }
#line 3540 "ldgram.c"
    break;

  case 194: /* memory_spec: NAME $@16 attributes_opt ':' origin_spec opt_comma length_spec  */
#line 798 "ldgram.y"
                {}
#line 3546 "ldgram.c"
    break;

  case 195: /* $@17: %empty  */
#line 800 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 3552 "ldgram.c"
    break;

  case 197: /* origin_spec: ORIGIN '=' mustbe_exp  */
#line 806 "ldgram.y"
                {
		  region->origin_exp = (yyvsp[0].etree);
		}
#line 3560 "ldgram.c"
    break;

  case 198: /* length_spec: LENGTH '=' mustbe_exp  */
#line 813 "ldgram.y"
                {
		  if (yychar == NAME)
		    {
		      yyclearin;
		      ldlex_backup ();
		    }
		  region->length_exp = (yyvsp[0].etree);
		}
#line 3573 "ldgram.c"
    break;

  case 199: /* attributes_opt: %empty  */
#line 825 "ldgram.y"
                  { /* dummy action to avoid bison 1.25 error message */ }
#line 3579 "ldgram.c"
    break;

  case 203: /* attributes_string: NAME  */
#line 836 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3585 "ldgram.c"
    break;

  case 204: /* attributes_string: '!' NAME  */
#line 838 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3591 "ldgram.c"
    break;

  case 205: /* startup: STARTUP '(' filename ')'  */
#line 843 "ldgram.y"
                { lang_startup((yyvsp[-1].name)); }
#line 3597 "ldgram.c"
    break;

  case 207: /* high_level_library: HLL '(' ')'  */
#line 849 "ldgram.y"
                        { ldemul_hll((char *)NULL); }
#line 3603 "ldgram.c"
    break;

  case 208: /* high_level_library_NAME_list: high_level_library_NAME_list opt_comma filename  */
#line 854 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3609 "ldgram.c"
    break;

  case 209: /* high_level_library_NAME_list: filename  */
#line 856 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3615 "ldgram.c"
    break;

  case 211: /* low_level_library_NAME_list: low_level_library_NAME_list opt_comma filename  */
#line 865 "ldgram.y"
                        { ldemul_syslib((yyvsp[0].name)); }
#line 3621 "ldgram.c"
    break;

  case 213: /* floating_point_support: FLOAT  */
#line 871 "ldgram.y"
                        { lang_float(true); }
#line 3627 "ldgram.c"
    break;

  case 214: /* floating_point_support: NOFLOAT  */
#line 873 "ldgram.y"
                        { lang_float(false); }
#line 3633 "ldgram.c"
    break;

  case 215: /* nocrossref_list: %empty  */
#line 878 "ldgram.y"
                {
		  (yyval.nocrossref) = NULL;
		}
#line 3641 "ldgram.c"
    break;

  case 216: /* nocrossref_list: NAME nocrossref_list  */
#line 882 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3654 "ldgram.c"
    break;

  case 217: /* nocrossref_list: NAME ',' nocrossref_list  */
#line 891 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3667 "ldgram.c"
    break;

  case 218: /* $@18: %empty  */
#line 901 "ldgram.y"
                        { ldlex_script (); }
#line 3673 "ldgram.c"
    break;

  case 219: /* paren_script_name: $@18 '(' NAME ')'  */
#line 903 "ldgram.y"
                        { ldlex_popstate (); (yyval.name) = (yyvsp[-1].name); }
#line 3679 "ldgram.c"
    break;

  case 220: /* $@19: %empty  */
#line 905 "ldgram.y"
                        { ldlex_expression (); }
#line 3685 "ldgram.c"
    break;

  case 221: /* mustbe_exp: $@19 exp  */
#line 907 "ldgram.y"
                        { ldlex_popstate (); (yyval.etree) = (yyvsp[0].etree); }
#line 3691 "ldgram.c"
    break;

  case 222: /* exp: '-' exp  */
#line 912 "ldgram.y"
                        { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3697 "ldgram.c"
    break;

  case 223: /* exp: '(' exp ')'  */
#line 914 "ldgram.y"
                        { (yyval.etree) = (yyvsp[-1].etree); }
#line 3703 "ldgram.c"
    break;

  case 224: /* exp: NEXT '(' exp ')'  */
#line 916 "ldgram.y"
                        { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3709 "ldgram.c"
    break;

  case 225: /* exp: '!' exp  */
#line 918 "ldgram.y"
                        { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3715 "ldgram.c"
    break;

  case 226: /* exp: '+' exp  */
#line 920 "ldgram.y"
                        { (yyval.etree) = (yyvsp[0].etree); }
#line 3721 "ldgram.c"
    break;

  case 227: /* exp: '~' exp  */
#line 922 "ldgram.y"
                        { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3727 "ldgram.c"
    break;

  case 228: /* exp: exp '*' exp  */
#line 925 "ldgram.y"
                        { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3733 "ldgram.c"
    break;

  case 229: /* exp: exp '/' exp  */
#line 927 "ldgram.y"
                        { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3739 "ldgram.c"
    break;

  case 230: /* exp: exp '%' exp  */
#line 929 "ldgram.y"
                        { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3745 "ldgram.c"
    break;

  case 231: /* exp: exp '+' exp  */
#line 931 "ldgram.y"
                        { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3751 "ldgram.c"
    break;

  case 232: /* exp: exp '-' exp  */
#line 933 "ldgram.y"
                        { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3757 "ldgram.c"
    break;

  case 233: /* exp: exp LSHIFT exp  */
#line 935 "ldgram.y"
                        { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3763 "ldgram.c"
    break;

  case 234: /* exp: exp RSHIFT exp  */
#line 937 "ldgram.y"
                        { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3769 "ldgram.c"
    break;

  case 235: /* exp: exp EQ exp  */
#line 939 "ldgram.y"
                        { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3775 "ldgram.c"
    break;

  case 236: /* exp: exp NE exp  */
#line 941 "ldgram.y"
                        { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3781 "ldgram.c"
    break;

  case 237: /* exp: exp LE exp  */
#line 943 "ldgram.y"
                        { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3787 "ldgram.c"
    break;

  case 238: /* exp: exp GE exp  */
#line 945 "ldgram.y"
                        { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3793 "ldgram.c"
    break;

  case 239: /* exp: exp '<' exp  */
#line 947 "ldgram.y"
                        { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3799 "ldgram.c"
    break;

  case 240: /* exp: exp '>' exp  */
#line 949 "ldgram.y"
                        { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3805 "ldgram.c"
    break;

  case 241: /* exp: exp '&' exp  */
#line 951 "ldgram.y"
                        { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3811 "ldgram.c"
    break;

  case 242: /* exp: exp '^' exp  */
#line 953 "ldgram.y"
                        { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3817 "ldgram.c"
    break;

  case 243: /* exp: exp '|' exp  */
#line 955 "ldgram.y"
                        { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3823 "ldgram.c"
    break;

  case 244: /* exp: exp '?' exp ':' exp  */
#line 957 "ldgram.y"
                        { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3829 "ldgram.c"
    break;

  case 245: /* exp: exp ANDAND exp  */
#line 959 "ldgram.y"
                        { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3835 "ldgram.c"
    break;

  case 246: /* exp: exp OROR exp  */
#line 961 "ldgram.y"
                        { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3841 "ldgram.c"
    break;

  case 247: /* exp: DEFINED '(' NAME ')'  */
#line 963 "ldgram.y"
                        { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3847 "ldgram.c"
    break;

  case 248: /* exp: INT  */
#line 965 "ldgram.y"
                        { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3853 "ldgram.c"
    break;

  case 249: /* exp: SIZEOF_HEADERS  */
#line 967 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3859 "ldgram.c"
    break;

  case 250: /* exp: ALIGNOF paren_script_name  */
#line 970 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ALIGNOF, (yyvsp[0].name)); }
#line 3865 "ldgram.c"
    break;

  case 251: /* exp: SIZEOF paren_script_name  */
#line 972 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF, (yyvsp[0].name)); }
#line 3871 "ldgram.c"
    break;

  case 252: /* exp: ADDR paren_script_name  */
#line 974 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ADDR, (yyvsp[0].name)); }
#line 3877 "ldgram.c"
    break;

  case 253: /* exp: LOADADDR paren_script_name  */
#line 976 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LOADADDR, (yyvsp[0].name)); }
#line 3883 "ldgram.c"
    break;

  case 254: /* exp: CONSTANT '(' NAME ')'  */
#line 978 "ldgram.y"
                        { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 3889 "ldgram.c"
    break;

  case 255: /* exp: ABSOLUTE '(' exp ')'  */
#line 980 "ldgram.y"
                        { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 3895 "ldgram.c"
    break;

  case 256: /* exp: ALIGN_K '(' exp ')'  */
#line 982 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3901 "ldgram.c"
    break;

  case 257: /* exp: ALIGN_K '(' exp ',' exp ')'  */
#line 984 "ldgram.y"
                        { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 3907 "ldgram.c"
    break;

  case 258: /* exp: DATA_SEGMENT_ALIGN '(' exp ',' exp ')'  */
#line 986 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 3913 "ldgram.c"
    break;

  case 259: /* exp: DATA_SEGMENT_RELRO_END '(' exp ',' exp ')'  */
#line 988 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 3919 "ldgram.c"
    break;

  case 260: /* exp: DATA_SEGMENT_END '(' exp ')'  */
#line 990 "ldgram.y"
                        { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 3925 "ldgram.c"
    break;

  case 261: /* $@20: %empty  */
#line 991 "ldgram.y"
                              { ldlex_script (); }
#line 3931 "ldgram.c"
    break;

  case 262: /* $@21: %empty  */
#line 992 "ldgram.y"
                        { ldlex_popstate (); }
#line 3937 "ldgram.c"
    break;

  case 263: /* exp: SEGMENT_START $@20 '(' NAME $@21 ',' exp ')'  */
#line 993 "ldgram.y"
                        { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-4].name))); }
#line 3950 "ldgram.c"
    break;

  case 264: /* exp: BLOCK '(' exp ')'  */
#line 1002 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3956 "ldgram.c"
    break;

  case 265: /* exp: NAME  */
#line 1004 "ldgram.y"
                        { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 3962 "ldgram.c"
    break;

  case 266: /* exp: MAX_K '(' exp ',' exp ')'  */
#line 1006 "ldgram.y"
                        { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3968 "ldgram.c"
    break;

  case 267: /* exp: MIN_K '(' exp ',' exp ')'  */
#line 1008 "ldgram.y"
                        { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3974 "ldgram.c"
    break;

  case 268: /* exp: ASSERT_K '(' exp ',' NAME ')'  */
#line 1010 "ldgram.y"
                        { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 3980 "ldgram.c"
    break;

  case 269: /* exp: ORIGIN paren_script_name  */
#line 1012 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[0].name)); }
#line 3986 "ldgram.c"
    break;

  case 270: /* exp: LENGTH paren_script_name  */
#line 1014 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[0].name)); }
#line 3992 "ldgram.c"
    break;

  case 271: /* exp: LOG2CEIL '(' exp ')'  */
#line 1016 "ldgram.y"
                        { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[-1].etree)); }
#line 3998 "ldgram.c"
    break;

  case 272: /* memspec_at_opt: AT '>' NAME  */
#line 1021 "ldgram.y"
                            { (yyval.name) = (yyvsp[0].name); }
#line 4004 "ldgram.c"
    break;

  case 273: /* memspec_at_opt: %empty  */
#line 1022 "ldgram.y"
                { (yyval.name) = 0; }
#line 4010 "ldgram.c"
    break;

  case 274: /* opt_at: AT '(' exp ')'  */
#line 1026 "ldgram.y"
                               { (yyval.etree) = (yyvsp[-1].etree); }
#line 4016 "ldgram.c"
    break;

  case 275: /* opt_at: %empty  */
#line 1027 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4022 "ldgram.c"
    break;

  case 276: /* opt_align: ALIGN_K '(' exp ')'  */
#line 1031 "ldgram.y"
                                    { (yyval.etree) = (yyvsp[-1].etree); }
#line 4028 "ldgram.c"
    break;

  case 277: /* opt_align: %empty  */
#line 1032 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4034 "ldgram.c"
    break;

  case 278: /* opt_align_with_input: ALIGN_WITH_INPUT  */
#line 1036 "ldgram.y"
                                 { (yyval.token) = ALIGN_WITH_INPUT; }
#line 4040 "ldgram.c"
    break;

  case 279: /* opt_align_with_input: %empty  */
#line 1037 "ldgram.y"
                { (yyval.token) = 0; }
#line 4046 "ldgram.c"
    break;

  case 280: /* opt_subalign: SUBALIGN '(' exp ')'  */
#line 1041 "ldgram.y"
                                     { (yyval.etree) = (yyvsp[-1].etree); }
#line 4052 "ldgram.c"
    break;

  case 281: /* opt_subalign: %empty  */
#line 1042 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4058 "ldgram.c"
    break;

  case 282: /* sect_constraint: ONLY_IF_RO  */
#line 1046 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RO; }
#line 4064 "ldgram.c"
    break;

  case 283: /* sect_constraint: ONLY_IF_RW  */
#line 1047 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RW; }
#line 4070 "ldgram.c"
    break;

  case 284: /* sect_constraint: SPECIAL  */
#line 1048 "ldgram.y"
                        { (yyval.token) = SPECIAL; }
#line 4076 "ldgram.c"
    break;

  case 285: /* sect_constraint: %empty  */
#line 1049 "ldgram.y"
                { (yyval.token) = 0; }
#line 4082 "ldgram.c"
    break;

  case 286: /* $@22: %empty  */
#line 1053 "ldgram.y"
                        { ldlex_expression(); }
#line 4088 "ldgram.c"
    break;

  case 287: /* $@23: %empty  */
#line 1060 "ldgram.y"
                        {
			  ldlex_popstate ();
			  ldlex_wild ();
			  lang_enter_output_section_statement ((yyvsp[-7].name), (yyvsp[-5].etree), sectype,
					sectype_value, (yyvsp[-3].etree), (yyvsp[-1].etree), (yyvsp[-4].etree), (yyvsp[0].token), (yyvsp[-2].token));
			}
#line 4099 "ldgram.c"
    break;

  case 288: /* $@24: %empty  */
#line 1069 "ldgram.y"
                        { ldlex_popstate (); }
#line 4105 "ldgram.c"
    break;

  case 289: /* $@25: %empty  */
#line 1071 "ldgram.y"
                        {
			  /* fill_opt may have switched the lexer into
			     expression state, and back again, but in
			     order to find the end of the fill
			     expression the parser must look ahead one
			     token.  If it is a NAME, throw it away as
			     it will have been lexed in the wrong
			     state.  */
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_output_section_statement ((yyvsp[0].fill), (yyvsp[-3].name),
							       (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 4126 "ldgram.c"
    break;

  case 291: /* $@26: %empty  */
#line 1089 "ldgram.y"
                        { ldlex_expression (); }
#line 4132 "ldgram.c"
    break;

  case 292: /* $@27: %empty  */
#line 1091 "ldgram.y"
                        { ldlex_popstate (); }
#line 4138 "ldgram.c"
    break;

  case 293: /* $@28: %empty  */
#line 1093 "ldgram.y"
                        {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 4146 "ldgram.c"
    break;

  case 294: /* $@29: %empty  */
#line 1099 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay ((yyvsp[-10].etree), (int) (yyvsp[-11].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 4160 "ldgram.c"
    break;

  case 296: /* $@30: %empty  */
#line 1114 "ldgram.y"
                        { ldlex_expression (); }
#line 4166 "ldgram.c"
    break;

  case 297: /* $@31: %empty  */
#line 1116 "ldgram.y"
                        {
			  ldlex_popstate ();
			  lang_add_assignment (exp_assign (".", (yyvsp[0].etree), false));
			}
#line 4175 "ldgram.c"
    break;

  case 299: /* $@32: %empty  */
#line 1122 "ldgram.y"
                        {
			  ldfile_open_command_file ((yyvsp[0].name));
			}
#line 4183 "ldgram.c"
    break;

  case 301: /* type: NOLOAD  */
#line 1129 "ldgram.y"
                   { sectype = noload_section; }
#line 4189 "ldgram.c"
    break;

  case 302: /* type: DSECT  */
#line 1130 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4195 "ldgram.c"
    break;

  case 303: /* type: COPY  */
#line 1131 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4201 "ldgram.c"
    break;

  case 304: /* type: INFO  */
#line 1132 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4207 "ldgram.c"
    break;

  case 305: /* type: OVERLAY  */
#line 1133 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4213 "ldgram.c"
    break;

  case 306: /* type: READONLY '(' TYPE '=' exp ')'  */
#line 1134 "ldgram.y"
                                         { sectype = typed_readonly_section; sectype_value = (yyvsp[-1].etree); }
#line 4219 "ldgram.c"
    break;

  case 307: /* type: READONLY  */
#line 1135 "ldgram.y"
                    { sectype = readonly_section; }
#line 4225 "ldgram.c"
    break;

  case 308: /* type: TYPE '=' exp  */
#line 1136 "ldgram.y"
                        { sectype = type_section; sectype_value = (yyvsp[0].etree); }
#line 4231 "ldgram.c"
    break;

  case 310: /* atype: %empty  */
#line 1141 "ldgram.y"
                            { sectype = normal_section; }
#line 4237 "ldgram.c"
    break;

  case 311: /* atype: '(' ')'  */
#line 1142 "ldgram.y"
                        { sectype = normal_section; }
#line 4243 "ldgram.c"
    break;

  case 312: /* opt_exp_with_type: exp atype ':'  */
#line 1146 "ldgram.y"
                                        { (yyval.etree) = (yyvsp[-2].etree); }
#line 4249 "ldgram.c"
    break;

  case 313: /* opt_exp_with_type: atype ':'  */
#line 1147 "ldgram.y"
                                        { (yyval.etree) = (etree_type *)NULL;  }
#line 4255 "ldgram.c"
    break;

  case 314: /* opt_exp_with_type: BIND '(' exp ')' atype ':'  */
#line 1152 "ldgram.y"
                                           { (yyval.etree) = (yyvsp[-3].etree); }
#line 4261 "ldgram.c"
    break;

  case 315: /* opt_exp_with_type: BIND '(' exp ')' BLOCK '(' exp ')' atype ':'  */
#line 1154 "ldgram.y"
                { (yyval.etree) = (yyvsp[-7].etree); }
#line 4267 "ldgram.c"
    break;

  case 316: /* opt_exp_without_type: exp ':'  */
#line 1158 "ldgram.y"
                                { (yyval.etree) = (yyvsp[-1].etree); }
#line 4273 "ldgram.c"
    break;

  case 317: /* opt_exp_without_type: ':'  */
#line 1159 "ldgram.y"
                                { (yyval.etree) = (etree_type *) NULL;  }
#line 4279 "ldgram.c"
    break;

  case 318: /* opt_nocrossrefs: %empty  */
#line 1164 "ldgram.y"
                        { (yyval.integer) = 0; }
#line 4285 "ldgram.c"
    break;

  case 319: /* opt_nocrossrefs: NOCROSSREFS  */
#line 1166 "ldgram.y"
                        { (yyval.integer) = 1; }
#line 4291 "ldgram.c"
    break;

  case 320: /* memspec_opt: '>' NAME  */
#line 1171 "ldgram.y"
                { (yyval.name) = (yyvsp[0].name); }
#line 4297 "ldgram.c"
    break;

  case 321: /* memspec_opt: %empty  */
#line 1172 "ldgram.y"
                { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 4303 "ldgram.c"
    break;

  case 322: /* phdr_opt: %empty  */
#line 1177 "ldgram.y"
                {
		  (yyval.section_phdr) = NULL;
		}
#line 4311 "ldgram.c"
    break;

  case 323: /* phdr_opt: phdr_opt ':' NAME  */
#line 1181 "ldgram.y"
                {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = false;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 4326 "ldgram.c"
    break;

  case 325: /* $@33: %empty  */
#line 1197 "ldgram.y"
                        {
			  ldlex_wild ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 4335 "ldgram.c"
    break;

  case 326: /* $@34: %empty  */
#line 1204 "ldgram.y"
                        { ldlex_popstate (); }
#line 4341 "ldgram.c"
    break;

  case 327: /* $@35: %empty  */
#line 1206 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 4354 "ldgram.c"
    break;

  case 332: /* $@36: %empty  */
#line 1227 "ldgram.y"
                     { ldlex_expression (); }
#line 4360 "ldgram.c"
    break;

  case 333: /* $@37: %empty  */
#line 1228 "ldgram.y"
                                            { ldlex_popstate (); }
#line 4366 "ldgram.c"
    break;

  case 334: /* phdr: NAME $@36 phdr_type phdr_qualifiers $@37 ';'  */
#line 1230 "ldgram.y"
                {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 4375 "ldgram.c"
    break;

  case 335: /* phdr_type: exp  */
#line 1238 "ldgram.y"
                {
		  (yyval.etree) = (yyvsp[0].etree);

		  if ((yyvsp[0].etree)->type.node_class == etree_name
		      && (yyvsp[0].etree)->type.node_code == NAME)
		    {
		      const char *s;
		      unsigned int i;
		      static const char * const phdr_types[] =
			{
			  "PT_NULL", "PT_LOAD", "PT_DYNAMIC",
			  "PT_INTERP", "PT_NOTE", "PT_SHLIB",
			  "PT_PHDR", "PT_TLS"
			};

		      s = (yyvsp[0].etree)->name.name;
		      for (i = 0;
			   i < sizeof phdr_types / sizeof phdr_types[0];
			   i++)
			if (strcmp (s, phdr_types[i]) == 0)
			  {
			    (yyval.etree) = exp_intop (i);
			    break;
			  }
		      if (i == sizeof phdr_types / sizeof phdr_types[0])
			{
			  if (strcmp (s, "PT_GNU_EH_FRAME") == 0)
			    (yyval.etree) = exp_intop (0x6474e550);
			  else if (strcmp (s, "PT_GNU_STACK") == 0)
			    (yyval.etree) = exp_intop (0x6474e551);
			  else if (strcmp (s, "PT_GNU_RELRO") == 0)
			    (yyval.etree) = exp_intop (0x6474e552);
			  else if (strcmp (s, "PT_GNU_PROPERTY") == 0)
			    (yyval.etree) = exp_intop (0x6474e553);
			  else
			    {
			      einfo (_("\
%X%P:%pS: unknown phdr type `%s' (try integer literal)\n"),
				     NULL, s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
#line 4424 "ldgram.c"
    break;

  case 336: /* phdr_qualifiers: %empty  */
#line 1286 "ldgram.y"
                {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 4432 "ldgram.c"
    break;

  case 337: /* phdr_qualifiers: NAME phdr_val phdr_qualifiers  */
#line 1290 "ldgram.y"
                {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  if (strcmp ((yyvsp[-2].name), "FILEHDR") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).filehdr = true;
		  else if (strcmp ((yyvsp[-2].name), "PHDRS") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).phdrs = true;
		  else if (strcmp ((yyvsp[-2].name), "FLAGS") == 0 && (yyvsp[-1].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[-1].etree);
		  else
		    einfo (_("%X%P:%pS: PHDRS syntax error at `%s'\n"),
			   NULL, (yyvsp[-2].name));
		}
#line 4449 "ldgram.c"
    break;

  case 338: /* phdr_qualifiers: AT '(' exp ')' phdr_qualifiers  */
#line 1303 "ldgram.y"
                {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 4458 "ldgram.c"
    break;

  case 339: /* phdr_val: %empty  */
#line 1311 "ldgram.y"
                {
		  (yyval.etree) = NULL;
		}
#line 4466 "ldgram.c"
    break;

  case 340: /* phdr_val: '(' exp ')'  */
#line 1315 "ldgram.y"
                {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 4474 "ldgram.c"
    break;

  case 341: /* $@38: %empty  */
#line 1321 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 4483 "ldgram.c"
    break;

  case 342: /* dynamic_list_file: $@38 dynamic_list_nodes  */
#line 1326 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4492 "ldgram.c"
    break;

  case 346: /* dynamic_list_tag: vers_defns ';'  */
#line 1343 "ldgram.y"
                {
		  lang_append_dynamic_list (current_dynamic_list_p, (yyvsp[-1].versyms));
		}
#line 4500 "ldgram.c"
    break;

  case 347: /* $@39: %empty  */
#line 1351 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 4509 "ldgram.c"
    break;

  case 348: /* version_script_file: $@39 vers_nodes  */
#line 1356 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4518 "ldgram.c"
    break;

  case 349: /* $@40: %empty  */
#line 1365 "ldgram.y"
                {
		  ldlex_version_script ();
		}
#line 4526 "ldgram.c"
    break;

  case 350: /* version: $@40 VERSIONK '{' vers_nodes '}'  */
#line 1369 "ldgram.y"
                {
		  ldlex_popstate ();
		}
#line 4534 "ldgram.c"
    break;

  case 353: /* vers_node: '{' vers_tag '}' ';'  */
#line 1381 "ldgram.y"
                {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 4542 "ldgram.c"
    break;

  case 354: /* vers_node: VERS_TAG '{' vers_tag '}' ';'  */
#line 1385 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 4550 "ldgram.c"
    break;

  case 355: /* vers_node: VERS_TAG '{' vers_tag '}' verdep ';'  */
#line 1389 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 4558 "ldgram.c"
    break;

  case 356: /* verdep: VERS_TAG  */
#line 1396 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 4566 "ldgram.c"
    break;

  case 357: /* verdep: verdep VERS_TAG  */
#line 1400 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4574 "ldgram.c"
    break;

  case 358: /* vers_tag: %empty  */
#line 1407 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4582 "ldgram.c"
    break;

  case 359: /* vers_tag: vers_defns ';'  */
#line 1411 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4590 "ldgram.c"
    break;

  case 360: /* vers_tag: GLOBAL ':' vers_defns ';'  */
#line 1415 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4598 "ldgram.c"
    break;

  case 361: /* vers_tag: LOCAL ':' vers_defns ';'  */
#line 1419 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4606 "ldgram.c"
    break;

  case 362: /* vers_tag: GLOBAL ':' vers_defns ';' LOCAL ':' vers_defns ';'  */
#line 1423 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4614 "ldgram.c"
    break;

  case 363: /* vers_defns: VERS_IDENTIFIER  */
#line 1430 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4622 "ldgram.c"
    break;

  case 364: /* vers_defns: NAME  */
#line 1434 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4630 "ldgram.c"
    break;

  case 365: /* vers_defns: vers_defns ';' VERS_IDENTIFIER  */
#line 1438 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4638 "ldgram.c"
    break;

  case 366: /* vers_defns: vers_defns ';' NAME  */
#line 1442 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4646 "ldgram.c"
    break;

  case 367: /* @41: %empty  */
#line 1446 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4655 "ldgram.c"
    break;

  case 368: /* vers_defns: vers_defns ';' EXTERN NAME '{' @41 vers_defns opt_semicolon '}'  */
#line 1451 "ldgram.y"
                        {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4667 "ldgram.c"
    break;

  case 369: /* @42: %empty  */
#line 1459 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4676 "ldgram.c"
    break;

  case 370: /* vers_defns: EXTERN NAME '{' @42 vers_defns opt_semicolon '}'  */
#line 1464 "ldgram.y"
                        {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4685 "ldgram.c"
    break;

  case 371: /* vers_defns: GLOBAL  */
#line 1469 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, false);
		}
#line 4693 "ldgram.c"
    break;

  case 372: /* vers_defns: vers_defns ';' GLOBAL  */
#line 1473 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, false);
		}
#line 4701 "ldgram.c"
    break;

  case 373: /* vers_defns: LOCAL  */
#line 1477 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, false);
		}
#line 4709 "ldgram.c"
    break;

  case 374: /* vers_defns: vers_defns ';' LOCAL  */
#line 1481 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, false);
		}
#line 4717 "ldgram.c"
    break;

  case 375: /* vers_defns: EXTERN  */
#line 1485 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, false);
		}
#line 4725 "ldgram.c"
    break;

  case 376: /* vers_defns: vers_defns ';' EXTERN  */
#line 1489 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, false);
		}
#line 4733 "ldgram.c"
    break;


#line 4737 "ldgram.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1499 "ldgram.y"

void
yyerror(arg)
     const char *arg;
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldlex_filename ());
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
    einfo ("%F%P:%pS: %s in %s\n", NULL, arg, error_names[error_index - 1]);
  else
    einfo ("%F%P:%pS: %s\n", NULL, arg);
}
