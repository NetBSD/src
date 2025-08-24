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
#include "libiberty.h"
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

static void yyerror (const char *);

#line 115 "ldgram.c"

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
    XOREQ = 269,                   /* XOREQ  */
    OROR = 270,                    /* OROR  */
    ANDAND = 271,                  /* ANDAND  */
    EQ = 272,                      /* EQ  */
    NE = 273,                      /* NE  */
    LE = 274,                      /* LE  */
    GE = 275,                      /* GE  */
    LSHIFT = 276,                  /* LSHIFT  */
    RSHIFT = 277,                  /* RSHIFT  */
    UNARY = 278,                   /* UNARY  */
    END = 279,                     /* END  */
    ALIGN_K = 280,                 /* ALIGN_K  */
    BLOCK = 281,                   /* BLOCK  */
    BIND = 282,                    /* BIND  */
    QUAD = 283,                    /* QUAD  */
    SQUAD = 284,                   /* SQUAD  */
    LONG = 285,                    /* LONG  */
    SHORT = 286,                   /* SHORT  */
    BYTE = 287,                    /* BYTE  */
    ASCIZ = 288,                   /* ASCIZ  */
    SECTIONS = 289,                /* SECTIONS  */
    PHDRS = 290,                   /* PHDRS  */
    INSERT_K = 291,                /* INSERT_K  */
    AFTER = 292,                   /* AFTER  */
    BEFORE = 293,                  /* BEFORE  */
    LINKER_VERSION = 294,          /* LINKER_VERSION  */
    DATA_SEGMENT_ALIGN = 295,      /* DATA_SEGMENT_ALIGN  */
    DATA_SEGMENT_RELRO_END = 296,  /* DATA_SEGMENT_RELRO_END  */
    DATA_SEGMENT_END = 297,        /* DATA_SEGMENT_END  */
    SORT_BY_NAME = 298,            /* SORT_BY_NAME  */
    SORT_BY_ALIGNMENT = 299,       /* SORT_BY_ALIGNMENT  */
    SORT_NONE = 300,               /* SORT_NONE  */
    SORT_BY_INIT_PRIORITY = 301,   /* SORT_BY_INIT_PRIORITY  */
    REVERSE = 302,                 /* REVERSE  */
    SIZEOF_HEADERS = 303,          /* SIZEOF_HEADERS  */
    OUTPUT_FORMAT = 304,           /* OUTPUT_FORMAT  */
    FORCE_COMMON_ALLOCATION = 305, /* FORCE_COMMON_ALLOCATION  */
    OUTPUT_ARCH = 306,             /* OUTPUT_ARCH  */
    INHIBIT_COMMON_ALLOCATION = 307, /* INHIBIT_COMMON_ALLOCATION  */
    FORCE_GROUP_ALLOCATION = 308,  /* FORCE_GROUP_ALLOCATION  */
    SEGMENT_START = 309,           /* SEGMENT_START  */
    INCLUDE = 310,                 /* INCLUDE  */
    MEMORY = 311,                  /* MEMORY  */
    REGION_ALIAS = 312,            /* REGION_ALIAS  */
    LD_FEATURE = 313,              /* LD_FEATURE  */
    NOLOAD = 314,                  /* NOLOAD  */
    DSECT = 315,                   /* DSECT  */
    COPY = 316,                    /* COPY  */
    INFO = 317,                    /* INFO  */
    OVERLAY = 318,                 /* OVERLAY  */
    READONLY = 319,                /* READONLY  */
    TYPE = 320,                    /* TYPE  */
    DEFINED = 321,                 /* DEFINED  */
    TARGET_K = 322,                /* TARGET_K  */
    SEARCH_DIR = 323,              /* SEARCH_DIR  */
    MAP = 324,                     /* MAP  */
    ENTRY = 325,                   /* ENTRY  */
    NEXT = 326,                    /* NEXT  */
    SIZEOF = 327,                  /* SIZEOF  */
    ALIGNOF = 328,                 /* ALIGNOF  */
    ADDR = 329,                    /* ADDR  */
    LOADADDR = 330,                /* LOADADDR  */
    MAX_K = 331,                   /* MAX_K  */
    MIN_K = 332,                   /* MIN_K  */
    STARTUP = 333,                 /* STARTUP  */
    HLL = 334,                     /* HLL  */
    SYSLIB = 335,                  /* SYSLIB  */
    FLOAT = 336,                   /* FLOAT  */
    NOFLOAT = 337,                 /* NOFLOAT  */
    NOCROSSREFS = 338,             /* NOCROSSREFS  */
    NOCROSSREFS_TO = 339,          /* NOCROSSREFS_TO  */
    ORIGIN = 340,                  /* ORIGIN  */
    FILL = 341,                    /* FILL  */
    LENGTH = 342,                  /* LENGTH  */
    CREATE_OBJECT_SYMBOLS = 343,   /* CREATE_OBJECT_SYMBOLS  */
    INPUT = 344,                   /* INPUT  */
    GROUP = 345,                   /* GROUP  */
    OUTPUT = 346,                  /* OUTPUT  */
    CONSTRUCTORS = 347,            /* CONSTRUCTORS  */
    ALIGNMOD = 348,                /* ALIGNMOD  */
    AT = 349,                      /* AT  */
    SUBALIGN = 350,                /* SUBALIGN  */
    HIDDEN = 351,                  /* HIDDEN  */
    PROVIDE = 352,                 /* PROVIDE  */
    PROVIDE_HIDDEN = 353,          /* PROVIDE_HIDDEN  */
    AS_NEEDED = 354,               /* AS_NEEDED  */
    CHIP = 355,                    /* CHIP  */
    LIST = 356,                    /* LIST  */
    SECT = 357,                    /* SECT  */
    ABSOLUTE = 358,                /* ABSOLUTE  */
    LOAD = 359,                    /* LOAD  */
    NEWLINE = 360,                 /* NEWLINE  */
    ENDWORD = 361,                 /* ENDWORD  */
    ORDER = 362,                   /* ORDER  */
    NAMEWORD = 363,                /* NAMEWORD  */
    ASSERT_K = 364,                /* ASSERT_K  */
    LOG2CEIL = 365,                /* LOG2CEIL  */
    FORMAT = 366,                  /* FORMAT  */
    PUBLIC = 367,                  /* PUBLIC  */
    DEFSYMEND = 368,               /* DEFSYMEND  */
    BASE = 369,                    /* BASE  */
    ALIAS = 370,                   /* ALIAS  */
    TRUNCATE = 371,                /* TRUNCATE  */
    REL = 372,                     /* REL  */
    INPUT_SCRIPT = 373,            /* INPUT_SCRIPT  */
    INPUT_MRI_SCRIPT = 374,        /* INPUT_MRI_SCRIPT  */
    INPUT_DEFSYM = 375,            /* INPUT_DEFSYM  */
    CASE = 376,                    /* CASE  */
    EXTERN = 377,                  /* EXTERN  */
    START = 378,                   /* START  */
    VERS_TAG = 379,                /* VERS_TAG  */
    VERS_IDENTIFIER = 380,         /* VERS_IDENTIFIER  */
    GLOBAL = 381,                  /* GLOBAL  */
    LOCAL = 382,                   /* LOCAL  */
    VERSIONK = 383,                /* VERSIONK  */
    INPUT_VERSION_SCRIPT = 384,    /* INPUT_VERSION_SCRIPT  */
    KEEP = 385,                    /* KEEP  */
    ONLY_IF_RO = 386,              /* ONLY_IF_RO  */
    ONLY_IF_RW = 387,              /* ONLY_IF_RW  */
    SPECIAL = 388,                 /* SPECIAL  */
    INPUT_SECTION_FLAGS = 389,     /* INPUT_SECTION_FLAGS  */
    ALIGN_WITH_INPUT = 390,        /* ALIGN_WITH_INPUT  */
    EXCLUDE_FILE = 391,            /* EXCLUDE_FILE  */
    CONSTANT = 392,                /* CONSTANT  */
    INPUT_DYNAMIC_LIST = 393       /* INPUT_DYNAMIC_LIST  */
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
#define XOREQ 269
#define OROR 270
#define ANDAND 271
#define EQ 272
#define NE 273
#define LE 274
#define GE 275
#define LSHIFT 276
#define RSHIFT 277
#define UNARY 278
#define END 279
#define ALIGN_K 280
#define BLOCK 281
#define BIND 282
#define QUAD 283
#define SQUAD 284
#define LONG 285
#define SHORT 286
#define BYTE 287
#define ASCIZ 288
#define SECTIONS 289
#define PHDRS 290
#define INSERT_K 291
#define AFTER 292
#define BEFORE 293
#define LINKER_VERSION 294
#define DATA_SEGMENT_ALIGN 295
#define DATA_SEGMENT_RELRO_END 296
#define DATA_SEGMENT_END 297
#define SORT_BY_NAME 298
#define SORT_BY_ALIGNMENT 299
#define SORT_NONE 300
#define SORT_BY_INIT_PRIORITY 301
#define REVERSE 302
#define SIZEOF_HEADERS 303
#define OUTPUT_FORMAT 304
#define FORCE_COMMON_ALLOCATION 305
#define OUTPUT_ARCH 306
#define INHIBIT_COMMON_ALLOCATION 307
#define FORCE_GROUP_ALLOCATION 308
#define SEGMENT_START 309
#define INCLUDE 310
#define MEMORY 311
#define REGION_ALIAS 312
#define LD_FEATURE 313
#define NOLOAD 314
#define DSECT 315
#define COPY 316
#define INFO 317
#define OVERLAY 318
#define READONLY 319
#define TYPE 320
#define DEFINED 321
#define TARGET_K 322
#define SEARCH_DIR 323
#define MAP 324
#define ENTRY 325
#define NEXT 326
#define SIZEOF 327
#define ALIGNOF 328
#define ADDR 329
#define LOADADDR 330
#define MAX_K 331
#define MIN_K 332
#define STARTUP 333
#define HLL 334
#define SYSLIB 335
#define FLOAT 336
#define NOFLOAT 337
#define NOCROSSREFS 338
#define NOCROSSREFS_TO 339
#define ORIGIN 340
#define FILL 341
#define LENGTH 342
#define CREATE_OBJECT_SYMBOLS 343
#define INPUT 344
#define GROUP 345
#define OUTPUT 346
#define CONSTRUCTORS 347
#define ALIGNMOD 348
#define AT 349
#define SUBALIGN 350
#define HIDDEN 351
#define PROVIDE 352
#define PROVIDE_HIDDEN 353
#define AS_NEEDED 354
#define CHIP 355
#define LIST 356
#define SECT 357
#define ABSOLUTE 358
#define LOAD 359
#define NEWLINE 360
#define ENDWORD 361
#define ORDER 362
#define NAMEWORD 363
#define ASSERT_K 364
#define LOG2CEIL 365
#define FORMAT 366
#define PUBLIC 367
#define DEFSYMEND 368
#define BASE 369
#define ALIAS 370
#define TRUNCATE 371
#define REL 372
#define INPUT_SCRIPT 373
#define INPUT_MRI_SCRIPT 374
#define INPUT_DEFSYM 375
#define CASE 376
#define EXTERN 377
#define START 378
#define VERS_TAG 379
#define VERS_IDENTIFIER 380
#define GLOBAL 381
#define LOCAL 382
#define VERSIONK 383
#define INPUT_VERSION_SCRIPT 384
#define KEEP 385
#define ONLY_IF_RO 386
#define ONLY_IF_RW 387
#define SPECIAL 388
#define INPUT_SECTION_FLAGS 389
#define ALIGN_WITH_INPUT 390
#define EXCLUDE_FILE 391
#define CONSTANT 392
#define INPUT_DYNAMIC_LIST 393

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 65 "ldgram.y"

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

#line 475 "ldgram.c"

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
  YYSYMBOL_XOREQ = 15,                     /* XOREQ  */
  YYSYMBOL_16_ = 16,                       /* '?'  */
  YYSYMBOL_17_ = 17,                       /* ':'  */
  YYSYMBOL_OROR = 18,                      /* OROR  */
  YYSYMBOL_ANDAND = 19,                    /* ANDAND  */
  YYSYMBOL_20_ = 20,                       /* '|'  */
  YYSYMBOL_21_ = 21,                       /* '^'  */
  YYSYMBOL_22_ = 22,                       /* '&'  */
  YYSYMBOL_EQ = 23,                        /* EQ  */
  YYSYMBOL_NE = 24,                        /* NE  */
  YYSYMBOL_25_ = 25,                       /* '<'  */
  YYSYMBOL_26_ = 26,                       /* '>'  */
  YYSYMBOL_LE = 27,                        /* LE  */
  YYSYMBOL_GE = 28,                        /* GE  */
  YYSYMBOL_LSHIFT = 29,                    /* LSHIFT  */
  YYSYMBOL_RSHIFT = 30,                    /* RSHIFT  */
  YYSYMBOL_31_ = 31,                       /* '+'  */
  YYSYMBOL_32_ = 32,                       /* '-'  */
  YYSYMBOL_33_ = 33,                       /* '*'  */
  YYSYMBOL_34_ = 34,                       /* '/'  */
  YYSYMBOL_35_ = 35,                       /* '%'  */
  YYSYMBOL_UNARY = 36,                     /* UNARY  */
  YYSYMBOL_END = 37,                       /* END  */
  YYSYMBOL_38_ = 38,                       /* '('  */
  YYSYMBOL_ALIGN_K = 39,                   /* ALIGN_K  */
  YYSYMBOL_BLOCK = 40,                     /* BLOCK  */
  YYSYMBOL_BIND = 41,                      /* BIND  */
  YYSYMBOL_QUAD = 42,                      /* QUAD  */
  YYSYMBOL_SQUAD = 43,                     /* SQUAD  */
  YYSYMBOL_LONG = 44,                      /* LONG  */
  YYSYMBOL_SHORT = 45,                     /* SHORT  */
  YYSYMBOL_BYTE = 46,                      /* BYTE  */
  YYSYMBOL_ASCIZ = 47,                     /* ASCIZ  */
  YYSYMBOL_SECTIONS = 48,                  /* SECTIONS  */
  YYSYMBOL_PHDRS = 49,                     /* PHDRS  */
  YYSYMBOL_INSERT_K = 50,                  /* INSERT_K  */
  YYSYMBOL_AFTER = 51,                     /* AFTER  */
  YYSYMBOL_BEFORE = 52,                    /* BEFORE  */
  YYSYMBOL_LINKER_VERSION = 53,            /* LINKER_VERSION  */
  YYSYMBOL_DATA_SEGMENT_ALIGN = 54,        /* DATA_SEGMENT_ALIGN  */
  YYSYMBOL_DATA_SEGMENT_RELRO_END = 55,    /* DATA_SEGMENT_RELRO_END  */
  YYSYMBOL_DATA_SEGMENT_END = 56,          /* DATA_SEGMENT_END  */
  YYSYMBOL_SORT_BY_NAME = 57,              /* SORT_BY_NAME  */
  YYSYMBOL_SORT_BY_ALIGNMENT = 58,         /* SORT_BY_ALIGNMENT  */
  YYSYMBOL_SORT_NONE = 59,                 /* SORT_NONE  */
  YYSYMBOL_SORT_BY_INIT_PRIORITY = 60,     /* SORT_BY_INIT_PRIORITY  */
  YYSYMBOL_REVERSE = 61,                   /* REVERSE  */
  YYSYMBOL_62_ = 62,                       /* '{'  */
  YYSYMBOL_63_ = 63,                       /* '}'  */
  YYSYMBOL_SIZEOF_HEADERS = 64,            /* SIZEOF_HEADERS  */
  YYSYMBOL_OUTPUT_FORMAT = 65,             /* OUTPUT_FORMAT  */
  YYSYMBOL_FORCE_COMMON_ALLOCATION = 66,   /* FORCE_COMMON_ALLOCATION  */
  YYSYMBOL_OUTPUT_ARCH = 67,               /* OUTPUT_ARCH  */
  YYSYMBOL_INHIBIT_COMMON_ALLOCATION = 68, /* INHIBIT_COMMON_ALLOCATION  */
  YYSYMBOL_FORCE_GROUP_ALLOCATION = 69,    /* FORCE_GROUP_ALLOCATION  */
  YYSYMBOL_SEGMENT_START = 70,             /* SEGMENT_START  */
  YYSYMBOL_INCLUDE = 71,                   /* INCLUDE  */
  YYSYMBOL_MEMORY = 72,                    /* MEMORY  */
  YYSYMBOL_REGION_ALIAS = 73,              /* REGION_ALIAS  */
  YYSYMBOL_LD_FEATURE = 74,                /* LD_FEATURE  */
  YYSYMBOL_NOLOAD = 75,                    /* NOLOAD  */
  YYSYMBOL_DSECT = 76,                     /* DSECT  */
  YYSYMBOL_COPY = 77,                      /* COPY  */
  YYSYMBOL_INFO = 78,                      /* INFO  */
  YYSYMBOL_OVERLAY = 79,                   /* OVERLAY  */
  YYSYMBOL_READONLY = 80,                  /* READONLY  */
  YYSYMBOL_TYPE = 81,                      /* TYPE  */
  YYSYMBOL_DEFINED = 82,                   /* DEFINED  */
  YYSYMBOL_TARGET_K = 83,                  /* TARGET_K  */
  YYSYMBOL_SEARCH_DIR = 84,                /* SEARCH_DIR  */
  YYSYMBOL_MAP = 85,                       /* MAP  */
  YYSYMBOL_ENTRY = 86,                     /* ENTRY  */
  YYSYMBOL_NEXT = 87,                      /* NEXT  */
  YYSYMBOL_SIZEOF = 88,                    /* SIZEOF  */
  YYSYMBOL_ALIGNOF = 89,                   /* ALIGNOF  */
  YYSYMBOL_ADDR = 90,                      /* ADDR  */
  YYSYMBOL_LOADADDR = 91,                  /* LOADADDR  */
  YYSYMBOL_MAX_K = 92,                     /* MAX_K  */
  YYSYMBOL_MIN_K = 93,                     /* MIN_K  */
  YYSYMBOL_STARTUP = 94,                   /* STARTUP  */
  YYSYMBOL_HLL = 95,                       /* HLL  */
  YYSYMBOL_SYSLIB = 96,                    /* SYSLIB  */
  YYSYMBOL_FLOAT = 97,                     /* FLOAT  */
  YYSYMBOL_NOFLOAT = 98,                   /* NOFLOAT  */
  YYSYMBOL_NOCROSSREFS = 99,               /* NOCROSSREFS  */
  YYSYMBOL_NOCROSSREFS_TO = 100,           /* NOCROSSREFS_TO  */
  YYSYMBOL_ORIGIN = 101,                   /* ORIGIN  */
  YYSYMBOL_FILL = 102,                     /* FILL  */
  YYSYMBOL_LENGTH = 103,                   /* LENGTH  */
  YYSYMBOL_CREATE_OBJECT_SYMBOLS = 104,    /* CREATE_OBJECT_SYMBOLS  */
  YYSYMBOL_INPUT = 105,                    /* INPUT  */
  YYSYMBOL_GROUP = 106,                    /* GROUP  */
  YYSYMBOL_OUTPUT = 107,                   /* OUTPUT  */
  YYSYMBOL_CONSTRUCTORS = 108,             /* CONSTRUCTORS  */
  YYSYMBOL_ALIGNMOD = 109,                 /* ALIGNMOD  */
  YYSYMBOL_AT = 110,                       /* AT  */
  YYSYMBOL_SUBALIGN = 111,                 /* SUBALIGN  */
  YYSYMBOL_HIDDEN = 112,                   /* HIDDEN  */
  YYSYMBOL_PROVIDE = 113,                  /* PROVIDE  */
  YYSYMBOL_PROVIDE_HIDDEN = 114,           /* PROVIDE_HIDDEN  */
  YYSYMBOL_AS_NEEDED = 115,                /* AS_NEEDED  */
  YYSYMBOL_CHIP = 116,                     /* CHIP  */
  YYSYMBOL_LIST = 117,                     /* LIST  */
  YYSYMBOL_SECT = 118,                     /* SECT  */
  YYSYMBOL_ABSOLUTE = 119,                 /* ABSOLUTE  */
  YYSYMBOL_LOAD = 120,                     /* LOAD  */
  YYSYMBOL_NEWLINE = 121,                  /* NEWLINE  */
  YYSYMBOL_ENDWORD = 122,                  /* ENDWORD  */
  YYSYMBOL_ORDER = 123,                    /* ORDER  */
  YYSYMBOL_NAMEWORD = 124,                 /* NAMEWORD  */
  YYSYMBOL_ASSERT_K = 125,                 /* ASSERT_K  */
  YYSYMBOL_LOG2CEIL = 126,                 /* LOG2CEIL  */
  YYSYMBOL_FORMAT = 127,                   /* FORMAT  */
  YYSYMBOL_PUBLIC = 128,                   /* PUBLIC  */
  YYSYMBOL_DEFSYMEND = 129,                /* DEFSYMEND  */
  YYSYMBOL_BASE = 130,                     /* BASE  */
  YYSYMBOL_ALIAS = 131,                    /* ALIAS  */
  YYSYMBOL_TRUNCATE = 132,                 /* TRUNCATE  */
  YYSYMBOL_REL = 133,                      /* REL  */
  YYSYMBOL_INPUT_SCRIPT = 134,             /* INPUT_SCRIPT  */
  YYSYMBOL_INPUT_MRI_SCRIPT = 135,         /* INPUT_MRI_SCRIPT  */
  YYSYMBOL_INPUT_DEFSYM = 136,             /* INPUT_DEFSYM  */
  YYSYMBOL_CASE = 137,                     /* CASE  */
  YYSYMBOL_EXTERN = 138,                   /* EXTERN  */
  YYSYMBOL_START = 139,                    /* START  */
  YYSYMBOL_VERS_TAG = 140,                 /* VERS_TAG  */
  YYSYMBOL_VERS_IDENTIFIER = 141,          /* VERS_IDENTIFIER  */
  YYSYMBOL_GLOBAL = 142,                   /* GLOBAL  */
  YYSYMBOL_LOCAL = 143,                    /* LOCAL  */
  YYSYMBOL_VERSIONK = 144,                 /* VERSIONK  */
  YYSYMBOL_INPUT_VERSION_SCRIPT = 145,     /* INPUT_VERSION_SCRIPT  */
  YYSYMBOL_KEEP = 146,                     /* KEEP  */
  YYSYMBOL_ONLY_IF_RO = 147,               /* ONLY_IF_RO  */
  YYSYMBOL_ONLY_IF_RW = 148,               /* ONLY_IF_RW  */
  YYSYMBOL_SPECIAL = 149,                  /* SPECIAL  */
  YYSYMBOL_INPUT_SECTION_FLAGS = 150,      /* INPUT_SECTION_FLAGS  */
  YYSYMBOL_ALIGN_WITH_INPUT = 151,         /* ALIGN_WITH_INPUT  */
  YYSYMBOL_EXCLUDE_FILE = 152,             /* EXCLUDE_FILE  */
  YYSYMBOL_CONSTANT = 153,                 /* CONSTANT  */
  YYSYMBOL_INPUT_DYNAMIC_LIST = 154,       /* INPUT_DYNAMIC_LIST  */
  YYSYMBOL_155_ = 155,                     /* ','  */
  YYSYMBOL_156_ = 156,                     /* ';'  */
  YYSYMBOL_157_ = 157,                     /* ')'  */
  YYSYMBOL_158_ = 158,                     /* '['  */
  YYSYMBOL_159_ = 159,                     /* ']'  */
  YYSYMBOL_160_ = 160,                     /* '!'  */
  YYSYMBOL_161_ = 161,                     /* '~'  */
  YYSYMBOL_YYACCEPT = 162,                 /* $accept  */
  YYSYMBOL_file = 163,                     /* file  */
  YYSYMBOL_filename = 164,                 /* filename  */
  YYSYMBOL_defsym_expr = 165,              /* defsym_expr  */
  YYSYMBOL_166_1 = 166,                    /* $@1  */
  YYSYMBOL_mri_script_file = 167,          /* mri_script_file  */
  YYSYMBOL_168_2 = 168,                    /* $@2  */
  YYSYMBOL_mri_script_lines = 169,         /* mri_script_lines  */
  YYSYMBOL_mri_script_command = 170,       /* mri_script_command  */
  YYSYMBOL_171_3 = 171,                    /* $@3  */
  YYSYMBOL_ordernamelist = 172,            /* ordernamelist  */
  YYSYMBOL_mri_load_name_list = 173,       /* mri_load_name_list  */
  YYSYMBOL_mri_abs_name_list = 174,        /* mri_abs_name_list  */
  YYSYMBOL_casesymlist = 175,              /* casesymlist  */
  YYSYMBOL_extern_name_list = 176,         /* extern_name_list  */
  YYSYMBOL_script_file = 177,              /* script_file  */
  YYSYMBOL_178_4 = 178,                    /* $@4  */
  YYSYMBOL_ifile_list = 179,               /* ifile_list  */
  YYSYMBOL_ifile_p1 = 180,                 /* ifile_p1  */
  YYSYMBOL_181_5 = 181,                    /* $@5  */
  YYSYMBOL_182_6 = 182,                    /* $@6  */
  YYSYMBOL_183_7 = 183,                    /* $@7  */
  YYSYMBOL_input_list = 184,               /* input_list  */
  YYSYMBOL_185_8 = 185,                    /* $@8  */
  YYSYMBOL_input_list1 = 186,              /* input_list1  */
  YYSYMBOL_187_9 = 187,                    /* @9  */
  YYSYMBOL_188_10 = 188,                   /* @10  */
  YYSYMBOL_189_11 = 189,                   /* @11  */
  YYSYMBOL_sections = 190,                 /* sections  */
  YYSYMBOL_sec_or_group_p1 = 191,          /* sec_or_group_p1  */
  YYSYMBOL_statement_anywhere = 192,       /* statement_anywhere  */
  YYSYMBOL_193_12 = 193,                   /* $@12  */
  YYSYMBOL_wildcard_name = 194,            /* wildcard_name  */
  YYSYMBOL_wildcard_maybe_exclude = 195,   /* wildcard_maybe_exclude  */
  YYSYMBOL_wildcard_maybe_reverse = 196,   /* wildcard_maybe_reverse  */
  YYSYMBOL_filename_spec = 197,            /* filename_spec  */
  YYSYMBOL_section_name_spec = 198,        /* section_name_spec  */
  YYSYMBOL_sect_flag_list = 199,           /* sect_flag_list  */
  YYSYMBOL_sect_flags = 200,               /* sect_flags  */
  YYSYMBOL_exclude_name_list = 201,        /* exclude_name_list  */
  YYSYMBOL_section_name_list = 202,        /* section_name_list  */
  YYSYMBOL_input_section_spec_no_keep = 203, /* input_section_spec_no_keep  */
  YYSYMBOL_input_section_spec = 204,       /* input_section_spec  */
  YYSYMBOL_205_13 = 205,                   /* $@13  */
  YYSYMBOL_statement = 206,                /* statement  */
  YYSYMBOL_207_14 = 207,                   /* $@14  */
  YYSYMBOL_208_15 = 208,                   /* $@15  */
  YYSYMBOL_statement_list = 209,           /* statement_list  */
  YYSYMBOL_statement_list_opt = 210,       /* statement_list_opt  */
  YYSYMBOL_length = 211,                   /* length  */
  YYSYMBOL_fill_exp = 212,                 /* fill_exp  */
  YYSYMBOL_fill_opt = 213,                 /* fill_opt  */
  YYSYMBOL_assign_op = 214,                /* assign_op  */
  YYSYMBOL_separator = 215,                /* separator  */
  YYSYMBOL_assignment = 216,               /* assignment  */
  YYSYMBOL_opt_comma = 217,                /* opt_comma  */
  YYSYMBOL_memory = 218,                   /* memory  */
  YYSYMBOL_memory_spec_list_opt = 219,     /* memory_spec_list_opt  */
  YYSYMBOL_memory_spec_list = 220,         /* memory_spec_list  */
  YYSYMBOL_memory_spec = 221,              /* memory_spec  */
  YYSYMBOL_222_16 = 222,                   /* $@16  */
  YYSYMBOL_223_17 = 223,                   /* $@17  */
  YYSYMBOL_origin_spec = 224,              /* origin_spec  */
  YYSYMBOL_length_spec = 225,              /* length_spec  */
  YYSYMBOL_attributes_opt = 226,           /* attributes_opt  */
  YYSYMBOL_attributes_list = 227,          /* attributes_list  */
  YYSYMBOL_attributes_string = 228,        /* attributes_string  */
  YYSYMBOL_startup = 229,                  /* startup  */
  YYSYMBOL_high_level_library = 230,       /* high_level_library  */
  YYSYMBOL_high_level_library_NAME_list = 231, /* high_level_library_NAME_list  */
  YYSYMBOL_low_level_library = 232,        /* low_level_library  */
  YYSYMBOL_low_level_library_NAME_list = 233, /* low_level_library_NAME_list  */
  YYSYMBOL_floating_point_support = 234,   /* floating_point_support  */
  YYSYMBOL_nocrossref_list = 235,          /* nocrossref_list  */
  YYSYMBOL_paren_script_name = 236,        /* paren_script_name  */
  YYSYMBOL_237_18 = 237,                   /* $@18  */
  YYSYMBOL_mustbe_exp = 238,               /* mustbe_exp  */
  YYSYMBOL_239_19 = 239,                   /* $@19  */
  YYSYMBOL_exp = 240,                      /* exp  */
  YYSYMBOL_241_20 = 241,                   /* $@20  */
  YYSYMBOL_242_21 = 242,                   /* $@21  */
  YYSYMBOL_memspec_at_opt = 243,           /* memspec_at_opt  */
  YYSYMBOL_opt_at = 244,                   /* opt_at  */
  YYSYMBOL_opt_align = 245,                /* opt_align  */
  YYSYMBOL_opt_align_with_input = 246,     /* opt_align_with_input  */
  YYSYMBOL_opt_subalign = 247,             /* opt_subalign  */
  YYSYMBOL_sect_constraint = 248,          /* sect_constraint  */
  YYSYMBOL_section = 249,                  /* section  */
  YYSYMBOL_250_22 = 250,                   /* $@22  */
  YYSYMBOL_251_23 = 251,                   /* $@23  */
  YYSYMBOL_252_24 = 252,                   /* $@24  */
  YYSYMBOL_253_25 = 253,                   /* $@25  */
  YYSYMBOL_254_26 = 254,                   /* $@26  */
  YYSYMBOL_255_27 = 255,                   /* $@27  */
  YYSYMBOL_256_28 = 256,                   /* $@28  */
  YYSYMBOL_257_29 = 257,                   /* $@29  */
  YYSYMBOL_258_30 = 258,                   /* $@30  */
  YYSYMBOL_259_31 = 259,                   /* $@31  */
  YYSYMBOL_260_32 = 260,                   /* $@32  */
  YYSYMBOL_type = 261,                     /* type  */
  YYSYMBOL_atype = 262,                    /* atype  */
  YYSYMBOL_opt_exp_with_type = 263,        /* opt_exp_with_type  */
  YYSYMBOL_opt_exp_without_type = 264,     /* opt_exp_without_type  */
  YYSYMBOL_opt_nocrossrefs = 265,          /* opt_nocrossrefs  */
  YYSYMBOL_memspec_opt = 266,              /* memspec_opt  */
  YYSYMBOL_phdr_opt = 267,                 /* phdr_opt  */
  YYSYMBOL_overlay_section = 268,          /* overlay_section  */
  YYSYMBOL_269_33 = 269,                   /* $@33  */
  YYSYMBOL_270_34 = 270,                   /* $@34  */
  YYSYMBOL_271_35 = 271,                   /* $@35  */
  YYSYMBOL_phdrs = 272,                    /* phdrs  */
  YYSYMBOL_phdr_list = 273,                /* phdr_list  */
  YYSYMBOL_phdr = 274,                     /* phdr  */
  YYSYMBOL_275_36 = 275,                   /* $@36  */
  YYSYMBOL_276_37 = 276,                   /* $@37  */
  YYSYMBOL_phdr_type = 277,                /* phdr_type  */
  YYSYMBOL_phdr_qualifiers = 278,          /* phdr_qualifiers  */
  YYSYMBOL_phdr_val = 279,                 /* phdr_val  */
  YYSYMBOL_dynamic_list_file = 280,        /* dynamic_list_file  */
  YYSYMBOL_281_38 = 281,                   /* $@38  */
  YYSYMBOL_dynamic_list_nodes = 282,       /* dynamic_list_nodes  */
  YYSYMBOL_dynamic_list_node = 283,        /* dynamic_list_node  */
  YYSYMBOL_dynamic_list_tag = 284,         /* dynamic_list_tag  */
  YYSYMBOL_version_script_file = 285,      /* version_script_file  */
  YYSYMBOL_286_39 = 286,                   /* $@39  */
  YYSYMBOL_version = 287,                  /* version  */
  YYSYMBOL_288_40 = 288,                   /* $@40  */
  YYSYMBOL_vers_nodes = 289,               /* vers_nodes  */
  YYSYMBOL_vers_node = 290,                /* vers_node  */
  YYSYMBOL_verdep = 291,                   /* verdep  */
  YYSYMBOL_vers_tag = 292,                 /* vers_tag  */
  YYSYMBOL_vers_defns = 293,               /* vers_defns  */
  YYSYMBOL_294_41 = 294,                   /* @41  */
  YYSYMBOL_295_42 = 295,                   /* @42  */
  YYSYMBOL_opt_semicolon = 296             /* opt_semicolon  */
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
#define YYLAST   1983

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  162
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  135
/* YYNRULES -- Number of rules.  */
#define YYNRULES  386
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  838

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   393


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
       2,     2,     2,   160,     2,     2,     2,    35,    22,     2,
      38,   157,    33,    31,   155,    32,     2,    34,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    17,   156,
      25,    10,    26,    16,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   158,     2,   159,    21,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,    20,    63,   161,     2,     2,     2,
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
       5,     6,     7,     8,     9,    11,    12,    13,    14,    15,
      18,    19,    23,    24,    27,    28,    29,    30,    36,    37,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   172,   172,   173,   174,   175,   176,   180,   184,   184,
     191,   191,   204,   205,   209,   210,   211,   214,   217,   218,
     219,   221,   223,   225,   227,   229,   231,   233,   235,   237,
     239,   241,   242,   243,   245,   247,   249,   251,   253,   254,
     256,   255,   258,   260,   264,   265,   266,   270,   272,   276,
     278,   283,   284,   285,   289,   291,   293,   298,   298,   304,
     305,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   322,   324,   326,   329,   331,   333,   335,   337,
     339,   341,   340,   344,   347,   346,   349,   353,   357,   357,
     359,   361,   363,   365,   370,   370,   375,   378,   381,   384,
     387,   390,   394,   393,   399,   398,   404,   403,   411,   415,
     416,   417,   421,   423,   424,   424,   430,   437,   445,   456,
     457,   466,   467,   472,   478,   487,   488,   493,   498,   503,
     508,   513,   518,   523,   528,   534,   542,   560,   581,   594,
     603,   614,   623,   634,   643,   652,   656,   665,   669,   677,
     679,   678,   685,   686,   687,   691,   695,   700,   701,   705,
     709,   713,   718,   717,   725,   724,   732,   733,   736,   738,
     742,   744,   746,   748,   750,   755,   762,   764,   768,   770,
     772,   774,   776,   778,   780,   782,   784,   789,   789,   794,
     798,   806,   810,   814,   822,   822,   826,   829,   829,   832,
     833,   838,   837,   843,   842,   848,   855,   868,   869,   873,
     874,   878,   880,   885,   890,   891,   896,   898,   903,   907,
     909,   913,   915,   921,   924,   933,   944,   944,   948,   948,
     954,   956,   958,   960,   962,   964,   967,   969,   971,   973,
     975,   977,   979,   981,   983,   985,   987,   989,   991,   993,
     995,   997,   999,  1001,  1003,  1005,  1007,  1009,  1012,  1014,
    1016,  1018,  1020,  1022,  1024,  1026,  1028,  1030,  1032,  1034,
    1035,  1034,  1044,  1046,  1048,  1050,  1052,  1054,  1056,  1058,
    1064,  1065,  1069,  1070,  1074,  1075,  1079,  1080,  1084,  1085,
    1089,  1090,  1091,  1092,  1096,  1103,  1112,  1114,  1095,  1132,
    1134,  1136,  1142,  1131,  1157,  1159,  1156,  1165,  1164,  1172,
    1173,  1174,  1175,  1176,  1177,  1178,  1179,  1183,  1184,  1185,
    1189,  1190,  1195,  1196,  1201,  1202,  1207,  1208,  1213,  1215,
    1220,  1223,  1236,  1240,  1247,  1249,  1238,  1261,  1264,  1266,
    1270,  1271,  1270,  1280,  1329,  1332,  1345,  1354,  1357,  1364,
    1364,  1376,  1377,  1381,  1385,  1394,  1394,  1408,  1408,  1418,
    1419,  1423,  1427,  1431,  1438,  1442,  1450,  1453,  1457,  1461,
    1465,  1472,  1476,  1480,  1484,  1489,  1488,  1502,  1501,  1511,
    1515,  1519,  1523,  1527,  1531,  1537,  1539
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
  "ANDEQ", "OREQ", "XOREQ", "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'",
  "'&'", "EQ", "NE", "'<'", "'>'", "LE", "GE", "LSHIFT", "RSHIFT", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY", "END", "'('", "ALIGN_K", "BLOCK",
  "BIND", "QUAD", "SQUAD", "LONG", "SHORT", "BYTE", "ASCIZ", "SECTIONS",
  "PHDRS", "INSERT_K", "AFTER", "BEFORE", "LINKER_VERSION",
  "DATA_SEGMENT_ALIGN", "DATA_SEGMENT_RELRO_END", "DATA_SEGMENT_END",
  "SORT_BY_NAME", "SORT_BY_ALIGNMENT", "SORT_NONE",
  "SORT_BY_INIT_PRIORITY", "REVERSE", "'{'", "'}'", "SIZEOF_HEADERS",
  "OUTPUT_FORMAT", "FORCE_COMMON_ALLOCATION", "OUTPUT_ARCH",
  "INHIBIT_COMMON_ALLOCATION", "FORCE_GROUP_ALLOCATION", "SEGMENT_START",
  "INCLUDE", "MEMORY", "REGION_ALIAS", "LD_FEATURE", "NOLOAD", "DSECT",
  "COPY", "INFO", "OVERLAY", "READONLY", "TYPE", "DEFINED", "TARGET_K",
  "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF", "ALIGNOF", "ADDR",
  "LOADADDR", "MAX_K", "MIN_K", "STARTUP", "HLL", "SYSLIB", "FLOAT",
  "NOFLOAT", "NOCROSSREFS", "NOCROSSREFS_TO", "ORIGIN", "FILL", "LENGTH",
  "CREATE_OBJECT_SYMBOLS", "INPUT", "GROUP", "OUTPUT", "CONSTRUCTORS",
  "ALIGNMOD", "AT", "SUBALIGN", "HIDDEN", "PROVIDE", "PROVIDE_HIDDEN",
  "AS_NEEDED", "CHIP", "LIST", "SECT", "ABSOLUTE", "LOAD", "NEWLINE",
  "ENDWORD", "ORDER", "NAMEWORD", "ASSERT_K", "LOG2CEIL", "FORMAT",
  "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE", "REL",
  "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE", "EXTERN",
  "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL", "VERSIONK",
  "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL",
  "INPUT_SECTION_FLAGS", "ALIGN_WITH_INPUT", "EXCLUDE_FILE", "CONSTANT",
  "INPUT_DYNAMIC_LIST", "','", "';'", "')'", "'['", "']'", "'!'", "'~'",
  "$accept", "file", "filename", "defsym_expr", "$@1", "mri_script_file",
  "$@2", "mri_script_lines", "mri_script_command", "$@3", "ordernamelist",
  "mri_load_name_list", "mri_abs_name_list", "casesymlist",
  "extern_name_list", "script_file", "$@4", "ifile_list", "ifile_p1",
  "$@5", "$@6", "$@7", "input_list", "$@8", "input_list1", "@9", "@10",
  "@11", "sections", "sec_or_group_p1", "statement_anywhere", "$@12",
  "wildcard_name", "wildcard_maybe_exclude", "wildcard_maybe_reverse",
  "filename_spec", "section_name_spec", "sect_flag_list", "sect_flags",
  "exclude_name_list", "section_name_list", "input_section_spec_no_keep",
  "input_section_spec", "$@13", "statement", "$@14", "$@15",
  "statement_list", "statement_list_opt", "length", "fill_exp", "fill_opt",
  "assign_op", "separator", "assignment", "opt_comma", "memory",
  "memory_spec_list_opt", "memory_spec_list", "memory_spec", "$@16",
  "$@17", "origin_spec", "length_spec", "attributes_opt",
  "attributes_list", "attributes_string", "startup", "high_level_library",
  "high_level_library_NAME_list", "low_level_library",
  "low_level_library_NAME_list", "floating_point_support",
  "nocrossref_list", "paren_script_name", "$@18", "mustbe_exp", "$@19",
  "exp", "$@20", "$@21", "memspec_at_opt", "opt_at", "opt_align",
  "opt_align_with_input", "opt_subalign", "sect_constraint", "section",
  "$@22", "$@23", "$@24", "$@25", "$@26", "$@27", "$@28", "$@29", "$@30",
  "$@31", "$@32", "type", "atype", "opt_exp_with_type",
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

#define YYPACT_NINF (-767)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-358)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     177,  -767,  -767,  -767,  -767,  -767,   103,  -767,  -767,  -767,
    -767,  -767,    15,  -767,   -29,  -767,    54,  -767,   906,  1697,
    1089,    70,    81,   124,  -767,   102,    68,   -29,  -767,   114,
      54,  -767,    74,   170,    17,   200,  -767,   204,  -767,  -767,
     282,   230,   255,   271,   281,   285,   295,   296,   297,   298,
     306,  -767,  -767,   307,   309,   310,  -767,   312,  -767,   314,
    -767,  -767,  -767,  -767,    95,  -767,  -767,  -767,  -767,  -767,
    -767,  -767,   210,  -767,   351,   282,   352,   721,  -767,   353,
     360,   361,  -767,  -767,   363,   364,   366,   721,   367,   370,
     368,   373,   374,   258,  -767,  -767,  -767,  -767,  -767,  -767,
    -767,  -767,  -767,  -767,  -767,   376,   377,   380,  -767,   381,
    -767,   371,   375,   332,   240,   102,  -767,  -767,  -767,   334,
     245,  -767,  -767,  -767,   398,   402,   408,   409,  -767,  -767,
      43,   410,   416,   417,   282,   282,   418,   282,    13,  -767,
     419,   419,  -767,   386,   282,   391,  -767,  -767,  -767,  -767,
     378,    73,  -767,    92,  -767,  -767,   721,   721,   721,   392,
     394,   395,   399,   400,  -767,  -767,   401,   414,  -767,  -767,
    -767,  -767,   420,   421,  -767,  -767,   422,   423,   432,   433,
     721,   721,  1499,   250,  -767,   299,  -767,   300,    12,  -767,
    -767,   514,  1907,   317,  -767,  -767,   318,  -767,    37,  -767,
    -767,  -767,   721,  -767,   471,   472,   473,   425,   114,   114,
     329,   153,   426,   335,   153,   303,    27,  -767,  -767,   -79,
     336,  -767,  -767,   282,   434,    -3,  -767,   340,   343,   344,
     346,   348,   350,   354,  -767,  -767,   -63,   143,    78,   357,
     358,   359,    20,  -767,   365,   721,   373,   -29,   721,   721,
    -767,   721,   721,  -767,  -767,   868,   721,   721,   721,   721,
     721,   470,   505,   721,  -767,   483,  -767,  -767,  -767,   721,
     721,  -767,  -767,   721,   721,   721,   506,  -767,  -767,   721,
     721,   721,   721,   721,   721,   721,   721,   721,   721,   721,
     721,   721,   721,   721,   721,   721,   721,   721,   721,   721,
     721,  1907,   519,   521,  -767,   523,   721,   721,  1907,   276,
     525,  -767,   526,  1907,  -767,  -767,  -767,  -767,   382,   383,
    -767,  -767,   527,  -767,  -767,  -767,  -100,  -767,  1089,  -767,
     282,  -767,  -767,  -767,  -767,  -767,  -767,  -767,   528,  -767,
    -767,   980,   495,  -767,  -767,  -767,    43,   533,  -767,  -767,
    -767,  -767,  -767,  -767,  -767,   282,  -767,   282,   419,  -767,
    -767,  -767,  -767,  -767,  -767,   503,    46,   403,  -767,  1519,
      45,   -27,  1907,  1907,  1722,  1907,  1907,  -767,   243,  1119,
    1539,  1559,  1139,   539,   407,  1159,   545,  1579,  1599,  1179,
    1637,  1199,   415,  1853,  1935,   918,  1098,  1616,  1948,  1007,
    1007,   670,   670,   670,   670,    90,    90,    98,    98,  -767,
    -767,  -767,  1907,  1907,  1907,  -767,  -767,  -767,  1907,  1907,
    -767,  -767,  -767,  -767,   424,   429,   430,   114,   293,   153,
     489,  -767,  -767,   -97,   589,  -767,   681,   589,   721,   404,
    -767,     4,   540,    43,  -767,   431,  -767,  -767,  -767,  -767,
    -767,  -767,   518,    50,  -767,   554,  -767,  -767,  -767,   721,
    -767,  -767,   721,   721,  -767,  -767,  -767,  -767,   437,   721,
     721,  -767,   561,  -767,  -767,   721,  -767,  -767,  -767,   411,
     549,  -767,  -767,  -767,   387,   535,  1751,   558,   461,  -767,
    -767,  1887,   477,  -767,  1907,    28,   573,  -767,   575,     3,
    -767,   479,   546,  -767,    20,  -767,  -767,  -767,   544,   438,
    1219,  1239,  1259,   435,  -767,  1279,  1299,   440,  1907,   153,
     536,   114,   114,  -767,  -767,  -767,  -767,  -767,   551,   588,
    -767,   443,   721,   369,   591,  -767,   571,   572,   449,  -767,
    -767,   461,   550,   576,   578,  -767,   465,  -767,  -767,  -767,
     608,   468,  -767,    22,    20,  -767,  -767,  -767,  -767,  -767,
     721,  -767,  -767,  -767,  -767,   469,   411,   543,   721,  -767,
    1319,  -767,   721,   594,   475,  -767,   524,  -767,   721,    28,
     721,   478,  -767,  -767,   534,  -767,    34,    20,  1339,   153,
     579,   626,  1907,   270,  1359,   721,  -767,   524,   600,  -767,
    1633,  1379,  -767,  1399,  -767,  -767,   631,  -767,  -767,    80,
    -767,  -767,   721,   609,   632,  -767,  1419,   154,   721,   586,
    -767,  -767,    28,  -767,  -767,  1439,   721,  -767,  -767,  -767,
    -767,  -767,  -767,  1459,  -767,  -767,  -767,  -767,  1479,   590,
    -767,  -767,   612,   812,    33,   634,   921,  -767,  -767,  -767,
    -767,  -767,   650,  -767,   617,   618,   619,   282,   620,  -767,
    -767,  -767,   622,   623,   624,  -767,    82,  -767,  -767,  -767,
     627,    16,  -767,  -767,  -767,   812,   601,   628,    95,  -767,
     642,  -767,  -767,    26,    94,    97,  -767,  -767,   635,  -767,
     666,   668,  -767,   645,   648,   649,   651,   653,  -767,  -767,
    -107,    82,   655,   656,    82,   657,  -767,  -767,  -767,  -767,
     644,   684,   587,   658,   552,   553,   559,   673,   560,   812,
     565,  -767,   721,    38,  -767,     1,  -767,    14,    87,    89,
      94,    94,    96,  -767,    82,   160,    94,   -85,    82,   642,
     566,   812,  -767,   692,  -767,    84,  -767,  -767,  -767,    84,
    -767,   689,  -767,  1657,   570,   574,   724,  -767,   668,  -767,
     694,   695,   577,   700,   701,   583,   584,   597,   705,   706,
    -767,  -767,  -767,   161,   587,  -767,   667,   742,    47,   598,
    -767,   743,  -767,  -767,  -767,    94,    94,  -767,    94,    94,
    -767,  -767,  -767,    84,    84,  -767,  -767,  -767,  -767,  -767,
     744,  -767,   599,   605,   607,   610,   621,   629,   630,   633,
      47,  -767,  -767,  -767,   468,  -767,    95,   636,   637,   638,
     639,   640,   641,  -767,    47,  -767,  -767,  -767,  -767,  -767,
    -767,  -767,  -767,   468,  -767,  -767,   468,  -767
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,    57,    10,     8,   355,   349,     0,     2,    60,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    58,    11,
       0,     0,     0,     0,     9,   366,     0,   356,   359,     0,
     350,   351,     0,     0,     0,     0,    77,     0,    79,    78,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   221,   222,     0,     0,     0,    81,     0,   114,     0,
      70,    59,    62,    68,     0,    61,    64,    65,    66,    67,
      63,    69,     0,    16,     0,     0,     0,     0,    17,     0,
       0,     0,    19,    46,     0,     0,     0,     0,     0,     0,
      51,     0,     0,     0,   178,   179,   180,   181,   228,   182,
     183,   184,   185,   186,   228,     0,     0,     0,   372,   383,
     371,   379,   381,     0,     0,   366,   360,   379,   381,     0,
       0,   352,   111,   338,     0,     0,     0,     0,     7,    84,
     198,     0,     0,     0,     0,     0,     0,     0,     0,   220,
     223,   223,    94,     0,     0,     0,    88,   188,   187,   113,
       0,     0,    40,     0,   256,   273,     0,     0,     0,     0,
       0,     0,     0,     0,   257,   269,     0,     0,   226,   226,
     226,   226,     0,     0,   226,   226,     0,     0,     0,     0,
       0,     0,    14,     0,    49,    31,    47,    32,    18,    33,
      23,     0,    36,     0,    37,    52,    38,    54,    39,    42,
      12,   189,     0,   190,     0,     0,     0,     0,     0,     0,
       0,   367,     0,     0,   354,     0,     0,    90,    91,     0,
       0,    60,   201,     0,     0,   195,   200,     0,     0,     0,
       0,     0,     0,     0,   215,   217,   195,   195,   223,     0,
       0,     0,     0,    94,     0,     0,     0,     0,     0,     0,
      13,     0,     0,   234,   230,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   259,     0,   258,   260,   261,     0,
       0,   277,   278,     0,     0,     0,     0,   233,   235,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    25,     0,     0,    45,     0,     0,     0,    22,     0,
       0,    55,     0,   229,   228,   228,   228,   377,     0,     0,
     361,   374,   384,   373,   380,   382,     0,   353,   294,   108,
       0,   299,   304,   110,   109,   340,   337,   339,     0,    74,
      76,   357,   207,   203,   196,   194,     0,     0,    93,    71,
      72,    83,   112,   213,   214,     0,   218,     0,   223,   224,
      86,    87,    80,    96,    99,     0,    95,     0,    73,     0,
       0,     0,    27,    28,    43,    29,    30,   231,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   254,   253,   251,   250,   249,   243,
     244,   247,   248,   245,   246,   241,   242,   239,   240,   236,
     237,   238,    15,    26,    24,    50,    48,    44,    20,    21,
      35,    34,    53,    56,     0,     0,     0,     0,   368,   369,
       0,   364,   362,     0,   318,   307,     0,   318,     0,     0,
      85,     0,     0,   198,   199,     0,   216,   219,   225,   102,
      98,   101,     0,     0,    82,     0,    89,   358,    41,     0,
     264,   272,     0,     0,   268,   270,   255,   232,     0,     0,
       0,   263,     0,   279,   262,     0,   191,   192,   193,   385,
     382,   375,   365,   363,     0,     0,   318,     0,   283,   111,
     325,     0,   326,   305,   343,   344,     0,   211,     0,     0,
     209,     0,     0,    92,     0,   106,    97,   100,     0,     0,
       0,     0,     0,     0,   227,     0,     0,     0,   252,   386,
       0,     0,     0,   309,   310,   311,   312,   313,   315,     0,
     319,     0,     0,     0,     0,   321,     0,   285,     0,   324,
     327,   283,     0,   347,     0,   341,     0,   212,   208,   210,
       0,   195,   204,     0,     0,   104,   115,   265,   266,   267,
       0,   274,   275,   276,   378,     0,   385,     0,     0,   317,
       0,   320,     0,     0,   287,   308,   289,   111,     0,   344,
       0,     0,    75,   228,     0,   103,     0,     0,     0,   370,
       0,     0,   316,   318,     0,     0,   286,   289,     0,   300,
       0,     0,   345,     0,   342,   205,     0,   202,   107,     0,
     271,   376,     0,     0,     0,   282,     0,   293,     0,     0,
     306,   348,   344,   228,   105,     0,     0,   322,   284,   290,
     291,   292,   295,     0,   301,   346,   206,   314,     0,     0,
     288,   332,   318,   168,     0,     0,   143,   170,   171,   172,
     173,   174,     0,   161,     0,     0,     0,     0,     0,   154,
     155,   162,     0,     0,     0,   152,     0,   117,   119,   121,
       0,     0,   149,   157,   167,   169,     0,     0,     0,   333,
     329,   323,   159,     0,     0,     0,   164,   228,     0,   150,
       0,     0,   116,     0,     0,     0,     0,     0,   125,   142,
     195,     0,   144,     0,     0,     0,   166,   296,   228,   153,
       0,     0,   281,     0,     0,     0,     0,     0,     0,   168,
       0,   175,     0,     0,   136,     0,   140,     0,     0,     0,
       0,     0,     0,   145,     0,   195,     0,   195,     0,   329,
       0,   168,   328,     0,   330,     0,   156,   122,   123,     0,
     120,     0,   160,     0,   116,     0,     0,   138,     0,   139,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     141,   147,   146,   195,   281,   158,     0,     0,   177,     0,
     165,     0,   151,   137,   118,     0,     0,   126,     0,     0,
     127,   128,   133,     0,     0,   148,   330,   334,   280,   228,
       0,   302,     0,     0,     0,     0,     0,     0,     0,     0,
     177,   330,   176,   331,   195,   124,     0,     0,     0,     0,
       0,     0,     0,   297,   177,   303,   163,   130,   129,   131,
     132,   134,   135,   195,   335,   298,   195,   336
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -767,  -767,   -72,  -767,  -767,  -767,  -767,   507,  -767,  -767,
    -767,  -767,  -767,  -767,   512,  -767,  -767,   562,  -767,  -767,
    -767,  -767,   522,  -767,  -475,  -767,  -767,  -767,  -767,  -467,
     -14,  -767,  -638,  -386,  1164,   108,    32,  -767,  -767,  -767,
    -417,    57,  -767,  -767,   106,  -767,  -767,  -767,  -674,  -767,
     -11,  -766,  -767,  -657,   -12,  -223,  -767,   349,  -767,   453,
    -767,  -767,  -767,  -767,  -767,  -767,   290,  -767,  -767,  -767,
    -767,  -767,  -767,  -129,   155,  -767,   -89,  -767,   -76,  -767,
    -767,    30,   260,  -767,  -767,   205,  -767,  -767,  -767,  -767,
    -767,  -767,  -767,  -767,  -767,  -767,  -767,  -767,  -767,  -767,
    -476,   384,  -767,  -767,    66,  -750,  -767,  -767,  -767,  -767,
    -767,  -767,  -767,  -767,  -767,  -767,  -551,  -767,  -767,  -767,
    -767,   785,  -767,  -767,  -767,  -767,  -767,   580,   -22,  -767,
     702,   -23,  -767,  -767,   252
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     6,   129,    11,    12,     9,    10,    19,    93,   250,
     188,   187,   185,   196,   198,     7,     8,    18,    61,   143,
     221,   246,   241,   242,   366,   504,   587,   554,    62,   215,
     333,   145,   667,   668,   669,   670,   699,   725,   671,   727,
     700,   672,   673,   723,   674,   688,   719,   675,   676,   677,
     720,   801,   104,   149,    64,   734,    65,   224,   225,   226,
     342,   443,   551,   607,   442,   499,   500,    66,    67,   236,
      68,   237,    69,   239,   264,   265,   721,   202,   255,   261,
     513,   744,   537,   574,   597,   599,   632,   334,   434,   639,
     739,   833,   436,   619,   641,   814,   437,   542,   489,   531,
     487,   488,   492,   541,   712,   778,   644,   710,   811,   836,
      70,   216,   337,   438,   581,   495,   545,   579,    15,    16,
      30,    31,   119,    13,    14,    71,    72,    27,    28,   433,
     113,   114,   522,   427,   520
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,   182,   346,   152,    63,   116,   120,   497,   497,   201,
     534,   192,   240,   355,   357,   203,   304,   128,   692,    20,
     702,   709,   538,   756,   363,   364,   450,   451,   602,   553,
     692,   335,   543,    25,  -197,    25,   457,   679,   450,   451,
     431,   311,   754,   482,   823,   751,   810,   222,   345,   311,
     450,   451,   733,   726,   506,   507,   432,   799,   834,   483,
    -197,   824,   230,   231,   800,   233,   235,   776,   124,   125,
     345,   635,   244,   703,   772,   655,   338,   656,   339,   586,
     253,   254,   238,   248,   450,   451,   692,   713,   692,   759,
     336,   692,   345,   692,   354,   703,   680,   655,   692,   656,
     692,   692,   251,    17,   277,   278,   108,   301,   105,   359,
     600,    26,   609,    26,   223,   308,    29,   614,   108,   106,
     784,   293,   294,   295,   296,   297,   313,    21,    22,    23,
     115,   295,   296,   297,   714,   365,   122,   452,   544,   693,
     694,   695,   696,   697,   760,   761,   763,   764,   713,   452,
     713,   343,   345,   768,   717,   713,   769,   321,   757,   826,
     548,   452,   107,   498,   498,   508,   645,   305,   664,   369,
     234,   758,   372,   373,   704,   375,   376,   453,   664,   585,
     378,   379,   380,   381,   382,   318,   319,   385,   663,   453,
     664,   608,   312,   387,   388,   452,   666,   389,   390,   391,
     312,   453,   456,   393,   394,   395,   396,   397,   398,   399,
     400,   401,   402,   403,   404,   405,   406,   407,   408,   409,
     410,   411,   412,   413,   414,   424,   425,   426,   249,   448,
     418,   419,   123,   358,   664,   453,   664,   624,   126,   664,
     109,   664,   127,   110,   111,   112,   664,   252,   664,   664,
     147,   148,   109,   154,   155,   110,   117,   118,   435,   279,
     299,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   420,
     421,   156,   157,   446,   735,   447,   128,   737,   158,   159,
     160,   322,   130,   131,   323,   324,   325,   321,   345,   718,
     356,   629,   630,   631,   161,   162,   163,   328,   533,   132,
     613,     1,     2,     3,   164,   345,   345,   771,   795,   133,
     165,   773,     4,   134,   266,   267,   268,    63,   584,   271,
     272,     5,   166,   135,   136,   137,   138,   167,   168,   169,
     170,   171,   172,   173,   139,   140,   718,   141,   142,   116,
     144,   174,   146,   175,   150,   151,   153,   183,   486,   718,
     491,   486,   494,   779,   184,   186,   329,   189,   190,   176,
     191,   193,   195,   194,   330,   177,   178,   197,   199,   200,
     204,   205,   331,   510,   206,   207,   511,   512,   208,    47,
     154,   155,   209,   515,   516,   210,   211,   213,   459,   518,
     460,   214,   217,   179,   479,   300,   218,   808,   809,   332,
     180,   181,   219,   220,   227,    21,    22,    23,   156,   157,
     228,   229,   232,   238,   243,   158,   159,   160,    58,   245,
     256,   322,   257,   258,   323,   324,   480,   259,   260,   262,
     247,   161,   162,   163,   523,   524,   525,   526,   527,   528,
     529,   164,   263,   328,   302,   303,   570,   165,   269,   270,
     273,   274,   523,   524,   525,   526,   527,   528,   529,   166,
     275,   276,   309,   310,   167,   168,   169,   170,   171,   172,
     173,   314,   315,   316,   588,   320,   575,   317,   174,   326,
     175,   327,   592,   340,   605,   347,   594,   344,   565,   566,
     348,   349,   601,   350,   603,   351,   176,   352,   383,   384,
     392,   353,   177,   178,   360,   361,   362,   154,   155,   616,
     330,   386,   368,   415,   306,   416,   530,   417,   331,   422,
     423,   430,   439,   441,   636,    47,   625,   445,   428,   429,
     179,   449,   633,   465,   530,   156,   157,   180,   181,   468,
     638,   481,   158,   159,   160,   332,   505,   501,   509,   496,
     454,    21,    22,    23,   466,   517,   521,   519,   161,   162,
     163,   536,   474,   532,    58,   535,   540,   546,   164,   547,
     550,   476,   555,   552,   165,   686,   477,   478,   503,   567,
     560,   825,   154,   155,   514,   556,   166,   563,   568,   564,
     569,   167,   168,   169,   170,   171,   172,   173,   571,   572,
     835,   573,   577,   837,   578,   174,   580,   175,   583,   740,
     156,   157,   582,   345,   591,   589,   596,   484,   159,   160,
     485,   678,   595,   176,   604,   598,   612,   606,   618,   177,
     178,   623,   611,   161,   162,   163,   753,   626,   634,   627,
     533,   681,   643,   164,   682,   683,   684,   685,   687,   165,
     689,   690,   691,   678,   707,   701,   708,   179,   711,   307,
     724,   166,   692,   722,   180,   181,   167,   168,   169,   170,
     171,   172,   173,   728,   154,   155,   729,   730,   742,   731,
     174,   732,   175,  -116,   736,   738,   745,   743,   490,   291,
     292,   293,   294,   295,   296,   297,   741,   678,   176,   746,
     747,   749,   156,   157,   177,   178,   748,   750,   777,   158,
     159,   160,   752,   775,   154,   155,   780,  -143,   783,   678,
     797,   782,   785,   786,   787,   161,   162,   163,   788,   789,
     790,   791,   179,   793,   794,   164,   798,   803,   813,   180,
     181,   165,   156,   157,   792,   802,   815,   374,   370,   158,
     159,   160,   816,   166,   817,   367,   770,   818,   167,   168,
     169,   170,   171,   172,   173,   161,   162,   163,   819,   705,
     755,   706,   174,   341,   175,   164,   820,   821,   812,   549,
     822,   165,   502,   827,   828,   829,   830,   831,   832,   444,
     176,   576,   617,   166,   796,   774,   177,   178,   167,   168,
     169,   170,   171,   172,   173,   121,   646,   212,   590,     0,
       0,   493,   174,     0,   175,     0,     0,   371,     0,     0,
       0,     0,     0,     0,   179,     0,     0,     0,     0,     0,
     176,   180,   181,     0,     0,     0,   177,   178,     0,     0,
       0,     0,     0,     0,   647,   648,   649,   650,   651,   652,
       0,     0,     0,     0,     0,   653,     0,     0,     0,   654,
       0,   655,     0,   656,   179,     0,     0,     0,     0,     0,
       0,   180,   181,   657,   279,     0,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,     0,     0,     0,     0,     0,     0,
      20,     0,     0,     0,   658,     0,   659,     0,     0,     0,
     660,     0,     0,     0,    21,    22,    23,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   661,   282,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,    32,    33,    34,     0,   662,  -116,
       0,     0,   663,     0,   664,     0,     0,     0,   665,     0,
     666,    35,    36,    37,    38,    39,     0,    40,    41,    42,
      43,     0,     0,     0,    20,     0,     0,     0,     0,    44,
      45,    46,    47,     0,     0,     0,     0,     0,     0,     0,
      48,    49,    50,    51,    52,    53,    54,     0,     0,     0,
       0,    55,    56,    57,     0,     0,     0,   440,    21,    22,
      23,     0,     0,     0,     0,   377,     0,     0,    32,    33,
      34,    58,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,     0,    59,    35,    36,    37,    38,    39,
    -357,    40,    41,    42,    43,     0,     0,     0,     0,     0,
       0,     0,    60,    44,    45,    46,    47,     0,     0,     0,
       0,     0,     0,     0,    48,    49,    50,    51,    52,    53,
      54,     0,     0,     0,     0,    55,    56,    57,     0,     0,
       0,     0,    21,    22,    23,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    58,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    59,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297,     0,   279,    60,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,     0,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   461,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   464,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   467,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   471,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   473,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   557,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   558,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   559,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   561,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   562,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   593,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   610,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   615,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   621,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   622,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   628,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   637,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   279,   640,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,     0,   642,   328,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,     0,   279,   298,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,   455,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,     0,   462,     0,   620,     0,     0,     0,
       0,    73,     0,     0,   330,     0,     0,     0,     0,     0,
       0,     0,   331,     0,   463,     0,     0,     0,     0,    47,
       0,     0,     0,     0,     0,     0,    73,     0,     0,     0,
       0,     0,     0,     0,   469,     0,    74,     0,     0,   332,
       0,     0,     0,     0,     0,    21,    22,    23,     0,     0,
       0,     0,     0,     0,   470,     0,     0,     0,    58,   458,
       0,    74,     0,     0,     0,     0,     0,   279,    75,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   295,   296,   297,     0,     0,   533,
       0,     0,   472,    75,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,     0,     0,     0,
       0,     0,   781,    77,    78,    79,    80,    81,   -43,    82,
      83,    84,     0,     0,    85,    86,     0,    87,    88,    89,
     698,    76,     0,     0,    90,    91,    92,     0,    77,    78,
      79,    80,    81,     0,    82,    83,    84,   715,   716,    85,
      86,     0,    87,    88,    89,     0,     0,     0,     0,    90,
      91,    92,     0,     0,     0,   698,     0,     0,   698,   279,
     475,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,     0,
       0,     0,   762,   765,   766,   767,     0,     0,   698,     0,
     715,     0,   698,   279,   539,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   279,     0,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,     0,     0,     0,     0,     0,     0,   804,
     805,     0,   806,   807,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   285,   286,   287,   288,   289,   290,   291,   292,   293,
     294,   295,   296,   297
};

static const yytype_int16 yycheck[] =
{
      12,    77,   225,    75,    18,    27,    29,     4,     4,    98,
     486,    87,   141,   236,   237,   104,     4,     4,     4,     4,
       4,   678,   489,    22,     4,     5,     4,     5,   579,   504,
       4,     4,     4,    62,    37,    62,    63,     4,     4,     5,
     140,     4,     4,   140,   810,   719,   796,     4,   155,     4,
       4,     5,   159,   691,     4,     5,   156,    10,   824,   156,
      63,   811,   134,   135,    17,   137,   138,   741,    51,    52,
     155,   622,   144,    57,   159,    59,   155,    61,   157,   554,
     156,   157,     4,    10,     4,     5,     4,    61,     4,   727,
      63,     4,   155,     4,   157,    57,    63,    59,     4,    61,
       4,     4,    10,     0,   180,   181,     4,   183,    38,   238,
     577,   140,   587,   140,    71,   191,    62,   593,     4,    38,
     758,    31,    32,    33,    34,    35,   202,   112,   113,   114,
      62,    33,    34,    35,   108,   115,    62,   115,   110,    57,
      58,    59,    60,    61,    57,    58,    57,    58,    61,   115,
      61,   223,   155,    57,    57,    61,    60,     4,   157,   816,
     157,   115,    38,   160,   160,   115,   642,   155,   152,   245,
     157,   157,   248,   249,   158,   251,   252,   155,   152,   157,
     256,   257,   258,   259,   260,   208,   209,   263,   150,   155,
     152,   157,   155,   269,   270,   115,   158,   273,   274,   275,
     155,   155,   157,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   314,   315,   316,   155,   358,
     306,   307,    62,   155,   152,   155,   152,   157,    38,   152,
     138,   152,    38,   141,   142,   143,   152,   155,   152,   152,
     155,   156,   138,     3,     4,   141,   142,   143,   330,    16,
      10,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     3,
       4,    31,    32,   355,   701,   357,     4,   704,    38,    39,
      40,   138,    62,    38,   141,   142,   143,     4,   155,   685,
     157,   147,   148,   149,    54,    55,    56,     4,    38,    38,
      40,   134,   135,   136,    64,   155,   155,   157,   157,    38,
      70,   738,   145,    38,   169,   170,   171,   341,   551,   174,
     175,   154,    82,    38,    38,    38,    38,    87,    88,    89,
      90,    91,    92,    93,    38,    38,   732,    38,    38,   371,
      38,   101,    38,   103,   144,     4,     4,     4,   434,   745,
     436,   437,   438,   749,     4,     4,    63,     4,     4,   119,
       4,     4,     4,     3,    71,   125,   126,     4,     4,   121,
       4,     4,    79,   459,     4,     4,   462,   463,    17,    86,
       3,     4,    17,   469,   470,    63,   156,    63,   155,   475,
     157,   156,     4,   153,   427,   155,     4,   793,   794,   106,
     160,   161,     4,     4,     4,   112,   113,   114,    31,    32,
       4,     4,     4,     4,    38,    38,    39,    40,   125,    38,
      38,   138,    38,    38,   141,   142,   143,    38,    38,    38,
      62,    54,    55,    56,    75,    76,    77,    78,    79,    80,
      81,    64,    38,     4,   155,   155,   532,    70,    38,    38,
      38,    38,    75,    76,    77,    78,    79,    80,    81,    82,
      38,    38,   155,   155,    87,    88,    89,    90,    91,    92,
      93,    10,    10,    10,   560,   156,    37,    62,   101,    63,
     103,   156,   568,   157,   583,   155,   572,    63,   521,   522,
     157,   157,   578,   157,   580,   157,   119,   157,    38,     4,
       4,   157,   125,   126,   157,   157,   157,     3,     4,   595,
      71,    38,   157,     4,    10,     4,   157,     4,    79,     4,
       4,     4,     4,    38,   623,    86,   612,     4,   156,   156,
     153,    38,   618,     4,   157,    31,    32,   160,   161,     4,
     626,    62,    38,    39,    40,   106,    38,    17,     4,   155,
     157,   112,   113,   114,   157,     4,    17,   156,    54,    55,
      56,   110,   157,    38,   125,    17,    99,     4,    64,     4,
     101,   157,    38,    37,    70,   657,   157,   157,   157,    38,
     155,   814,     3,     4,   157,   157,    82,   157,    10,    63,
     157,    87,    88,    89,    90,    91,    92,    93,    17,    38,
     833,    39,    62,   836,    38,   101,    38,   103,    10,   708,
      31,    32,   157,   155,    81,   156,   151,    38,    39,    40,
      41,   643,    38,   119,   156,   111,    10,   103,    38,   125,
     126,    10,    63,    54,    55,    56,   722,    38,    62,    17,
      38,    17,    62,    64,     4,    38,    38,    38,    38,    70,
      38,    38,    38,   675,    63,    38,    38,   153,    26,   155,
       4,    82,     4,    38,   160,   161,    87,    88,    89,    90,
      91,    92,    93,    38,     3,     4,    38,    38,     4,    38,
     101,    38,   103,    38,    38,    38,    38,   110,    17,    29,
      30,    31,    32,    33,    34,    35,    62,   719,   119,   157,
     157,    38,    31,    32,   125,   126,   157,   157,    26,    38,
      39,    40,   157,   157,     3,     4,    37,   157,     4,   741,
      63,   157,    38,    38,   157,    54,    55,    56,    38,    38,
     157,   157,   153,    38,    38,    64,     4,     4,     4,   160,
     161,    70,    31,    32,   157,   157,   157,   250,   246,    38,
      39,    40,   157,    82,   157,   243,   734,   157,    87,    88,
      89,    90,    91,    92,    93,    54,    55,    56,   157,   671,
     723,   675,   101,   221,   103,    64,   157,   157,   799,   499,
     157,    70,   443,   157,   157,   157,   157,   157,   157,   346,
     119,   541,   597,    82,   774,   739,   125,   126,    87,    88,
      89,    90,    91,    92,    93,    30,     4,   115,   566,    -1,
      -1,   437,   101,    -1,   103,    -1,    -1,   247,    -1,    -1,
      -1,    -1,    -1,    -1,   153,    -1,    -1,    -1,    -1,    -1,
     119,   160,   161,    -1,    -1,    -1,   125,   126,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,    47,
      -1,    -1,    -1,    -1,    -1,    53,    -1,    -1,    -1,    57,
      -1,    59,    -1,    61,   153,    -1,    -1,    -1,    -1,    -1,
      -1,   160,   161,    71,    16,    -1,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,
       4,    -1,    -1,    -1,   102,    -1,   104,    -1,    -1,    -1,
     108,    -1,    -1,    -1,   112,   113,   114,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,   125,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    48,    49,    50,    -1,   146,    38,
      -1,    -1,   150,    -1,   152,    -1,    -1,    -1,   156,    -1,
     158,    65,    66,    67,    68,    69,    -1,    71,    72,    73,
      74,    -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    83,
      84,    85,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      94,    95,    96,    97,    98,    99,   100,    -1,    -1,    -1,
      -1,   105,   106,   107,    -1,    -1,    -1,    37,   112,   113,
     114,    -1,    -1,    -1,    -1,   157,    -1,    -1,    48,    49,
      50,   125,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    -1,   138,    65,    66,    67,    68,    69,
     144,    71,    72,    73,    74,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   156,    83,    84,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    94,    95,    96,    97,    98,    99,
     100,    -1,    -1,    -1,    -1,   105,   106,   107,    -1,    -1,
      -1,    -1,   112,   113,   114,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,   125,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   138,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    -1,    16,   156,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,    -1,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    16,   157,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    -1,   157,     4,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    -1,    16,   155,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    16,   155,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    -1,   155,    -1,    63,    -1,    -1,    -1,
      -1,     4,    -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,   155,    -1,    -1,    -1,    -1,    86,
      -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   155,    -1,    39,    -1,    -1,   106,
      -1,    -1,    -1,    -1,    -1,   112,   113,   114,    -1,    -1,
      -1,    -1,    -1,    -1,   155,    -1,    -1,    -1,   125,    37,
      -1,    39,    -1,    -1,    -1,    -1,    -1,    16,    71,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    -1,    -1,    38,
      -1,    -1,   155,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,    -1,    -1,    -1,
      -1,    -1,   155,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,   127,   128,    -1,   130,   131,   132,
     666,   109,    -1,    -1,   137,   138,   139,    -1,   116,   117,
     118,   119,   120,    -1,   122,   123,   124,   683,   684,   127,
     128,    -1,   130,   131,   132,    -1,    -1,    -1,    -1,   137,
     138,   139,    -1,    -1,    -1,   701,    -1,    -1,   704,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    -1,
      -1,    -1,   728,   729,   730,   731,    -1,    -1,   734,    -1,
     736,    -1,   738,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    16,    -1,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,   785,
     786,    -1,   788,   789,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   134,   135,   136,   145,   154,   163,   177,   178,   167,
     168,   165,   166,   285,   286,   280,   281,     0,   179,   169,
       4,   112,   113,   114,   216,    62,   140,   289,   290,    62,
     282,   283,    48,    49,    50,    65,    66,    67,    68,    69,
      71,    72,    73,    74,    83,    84,    85,    86,    94,    95,
      96,    97,    98,    99,   100,   105,   106,   107,   125,   138,
     156,   180,   190,   192,   216,   218,   229,   230,   232,   234,
     272,   287,   288,     4,    39,    71,   109,   116,   117,   118,
     119,   120,   122,   123,   124,   127,   128,   130,   131,   132,
     137,   138,   139,   170,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,   214,    38,    38,    38,     4,   138,
     141,   142,   143,   292,   293,    62,   290,   142,   143,   284,
     293,   283,    62,    62,    51,    52,    38,    38,     4,   164,
      62,    38,    38,    38,    38,    38,    38,    38,    38,    38,
      38,    38,    38,   181,    38,   193,    38,   155,   156,   215,
     144,     4,   164,     4,     3,     4,    31,    32,    38,    39,
      40,    54,    55,    56,    64,    70,    82,    87,    88,    89,
      90,    91,    92,    93,   101,   103,   119,   125,   126,   153,
     160,   161,   240,     4,     4,   174,     4,   173,   172,     4,
       4,     4,   240,     4,     3,     4,   175,     4,   176,     4,
     121,   238,   239,   238,     4,     4,     4,     4,    17,    17,
      63,   156,   292,    63,   156,   191,   273,     4,     4,     4,
       4,   182,     4,    71,   219,   220,   221,     4,     4,     4,
     164,   164,     4,   164,   157,   164,   231,   233,     4,   235,
     235,   184,   185,    38,   164,    38,   183,    62,    10,   155,
     171,    10,   155,   240,   240,   240,    38,    38,    38,    38,
      38,   241,    38,    38,   236,   237,   236,   236,   236,    38,
      38,   236,   236,    38,    38,    38,    38,   240,   240,    16,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,   155,    10,
     155,   240,   155,   155,     4,   155,    10,   155,   240,   155,
     155,     4,   155,   240,    10,    10,    10,    62,   293,   293,
     156,     4,   138,   141,   142,   143,    63,   156,     4,    63,
      71,    79,   106,   192,   249,     4,    63,   274,   155,   157,
     157,   179,   222,   164,    63,   155,   217,   155,   157,   157,
     157,   157,   157,   157,   157,   217,   157,   217,   155,   235,
     157,   157,   157,     4,     5,   115,   186,   184,   157,   240,
     176,   289,   240,   240,   169,   240,   240,   157,   240,   240,
     240,   240,   240,    38,     4,   240,    38,   240,   240,   240,
     240,   240,     4,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,     4,     4,     4,   240,   240,
       3,     4,     4,     4,   238,   238,   238,   295,   156,   156,
       4,   140,   156,   291,   250,   164,   254,   258,   275,     4,
      37,    38,   226,   223,   221,     4,   164,   164,   235,    38,
       4,     5,   115,   155,   157,   155,   157,    63,    37,   155,
     157,   157,   155,   155,   157,     4,   157,   157,     4,   155,
     155,   157,   155,   157,   157,    17,   157,   157,   157,   293,
     143,    62,   140,   156,    38,    41,   240,   262,   263,   260,
      17,   240,   264,   263,   240,   277,   155,     4,   160,   227,
     228,    17,   219,   157,   187,    38,     4,     5,   115,     4,
     240,   240,   240,   242,   157,   240,   240,     4,   240,   156,
     296,    17,   294,    75,    76,    77,    78,    79,    80,    81,
     157,   261,    38,    38,   262,    17,   110,   244,   191,    17,
      99,   265,   259,     4,   110,   278,     4,     4,   157,   228,
     101,   224,    37,   186,   189,    38,   157,   157,   157,   157,
     155,   157,   157,   157,    63,   293,   293,    38,    10,   157,
     240,    17,    38,    39,   245,    37,   244,    62,    38,   279,
      38,   276,   157,    10,   217,   157,   186,   188,   240,   156,
     296,    81,   240,   157,   240,    38,   151,   246,   111,   247,
     191,   240,   278,   240,   156,   238,   103,   225,   157,   186,
     157,    63,    10,    40,   262,   157,   240,   247,    38,   255,
      63,   157,   157,    10,   157,   240,    38,    17,   157,   147,
     148,   149,   248,   240,    62,   278,   238,   157,   240,   251,
     157,   256,   157,    62,   268,   262,     4,    42,    43,    44,
      45,    46,    47,    53,    57,    59,    61,    71,   102,   104,
     108,   125,   146,   150,   152,   156,   158,   194,   195,   196,
     197,   200,   203,   204,   206,   209,   210,   211,   216,     4,
      63,    17,     4,    38,    38,    38,   164,    38,   207,    38,
      38,    38,     4,    57,    58,    59,    60,    61,   196,   198,
     202,    38,     4,    57,   158,   197,   206,    63,    38,   215,
     269,    26,   266,    61,   108,   196,   196,    57,   195,   208,
     212,   238,    38,   205,     4,   199,   194,   201,    38,    38,
      38,    38,    38,   159,   217,   202,    38,   202,    38,   252,
     238,    62,     4,   110,   243,    38,   157,   157,   157,    38,
     157,   210,   157,   240,     4,   203,    22,   157,   157,   194,
      57,    58,   196,    57,    58,   196,   196,   196,    57,    60,
     198,   157,   159,   202,   266,   157,   210,    26,   267,   195,
      37,   155,   157,     4,   194,    38,    38,   157,    38,    38,
     157,   157,   157,    38,    38,   157,   243,    63,     4,    10,
      17,   213,   157,     4,   196,   196,   196,   196,   195,   195,
     267,   270,   212,     4,   257,   157,   157,   157,   157,   157,
     157,   157,   157,   213,   267,   217,   215,   157,   157,   157,
     157,   157,   157,   253,   213,   217,   271,   217
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   162,   163,   163,   163,   163,   163,   164,   166,   165,
     168,   167,   169,   169,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     171,   170,   170,   170,   172,   172,   172,   173,   173,   174,
     174,   175,   175,   175,   176,   176,   176,   178,   177,   179,
     179,   180,   180,   180,   180,   180,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   180,   180,   180,   180,   180,
     180,   181,   180,   180,   182,   180,   180,   180,   183,   180,
     180,   180,   180,   180,   185,   184,   186,   186,   186,   186,
     186,   186,   187,   186,   188,   186,   189,   186,   190,   191,
     191,   191,   192,   192,   193,   192,   194,   195,   195,   196,
     196,   197,   197,   197,   197,   198,   198,   198,   198,   198,
     198,   198,   198,   198,   198,   198,   199,   199,   200,   201,
     201,   202,   202,   203,   203,   203,   203,   203,   203,   204,
     205,   204,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   207,   206,   208,   206,   209,   209,   210,   210,
     211,   211,   211,   211,   211,   212,   213,   213,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   215,   215,   216,
     216,   216,   216,   216,   217,   217,   218,   219,   219,   220,
     220,   222,   221,   223,   221,   224,   225,   226,   226,   227,
     227,   228,   228,   229,   230,   230,   231,   231,   232,   233,
     233,   234,   234,   235,   235,   235,   237,   236,   239,   238,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   241,
     242,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     243,   243,   244,   244,   245,   245,   246,   246,   247,   247,
     248,   248,   248,   248,   250,   251,   252,   253,   249,   254,
     255,   256,   257,   249,   258,   259,   249,   260,   249,   261,
     261,   261,   261,   261,   261,   261,   261,   262,   262,   262,
     263,   263,   263,   263,   264,   264,   265,   265,   266,   266,
     267,   267,   268,   269,   270,   271,   268,   272,   273,   273,
     275,   276,   274,   277,   278,   278,   278,   279,   279,   281,
     280,   282,   282,   283,   284,   286,   285,   288,   287,   289,
     289,   290,   290,   290,   291,   291,   292,   292,   292,   292,
     292,   293,   293,   293,   293,   294,   293,   295,   293,   293,
     293,   293,   293,   293,   293,   296,   296
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
       4,     1,     4,     4,     7,     1,     4,     4,     4,     7,
       7,     7,     7,     4,     7,     7,     1,     3,     4,     2,
       1,     3,     1,     1,     2,     3,     4,     4,     5,     1,
       0,     5,     1,     2,     1,     1,     4,     1,     4,     2,
       4,     1,     0,     8,     0,     5,     2,     1,     0,     1,
       1,     1,     1,     1,     1,     1,     2,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     6,     6,     6,     1,     0,     4,     1,     0,     3,
       1,     0,     7,     0,     5,     3,     3,     0,     3,     1,
       2,     1,     2,     4,     4,     3,     3,     1,     4,     3,
       0,     1,     1,     0,     2,     3,     0,     4,     0,     2,
       2,     3,     4,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     5,     3,     3,     4,     1,     1,     2,     2,
       2,     2,     4,     4,     4,     6,     6,     6,     4,     0,
       0,     8,     4,     1,     6,     6,     6,     2,     2,     4,
       3,     0,     4,     0,     4,     0,     1,     0,     4,     0,
       1,     1,     1,     0,     0,     0,     0,     0,    19,     0,
       0,     0,     0,    17,     0,     0,     7,     0,     5,     1,
       1,     1,     1,     1,     6,     1,     3,     3,     0,     2,
       3,     2,     6,    10,     2,     1,     0,     1,     2,     0,
       0,     3,     0,     0,     0,     0,    11,     4,     0,     2,
       0,     0,     6,     1,     0,     3,     5,     0,     3,     0,
       2,     1,     2,     4,     2,     0,     2,     0,     5,     1,
       2,     4,     5,     6,     1,     2,     0,     2,     4,     4,
       8,     1,     1,     3,     3,     0,     9,     0,     7,     1,
       3,     1,     3,     1,     3,     0,     1
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
#line 184 "ldgram.y"
                { ldlex_expression(); }
#line 2589 "ldgram.c"
    break;

  case 9: /* defsym_expr: $@1 assignment  */
#line 186 "ldgram.y"
                { ldlex_popstate(); }
#line 2595 "ldgram.c"
    break;

  case 10: /* $@2: %empty  */
#line 191 "ldgram.y"
                {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2604 "ldgram.c"
    break;

  case 11: /* mri_script_file: $@2 mri_script_lines  */
#line 196 "ldgram.y"
                {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2614 "ldgram.c"
    break;

  case 16: /* mri_script_command: NAME  */
#line 211 "ldgram.y"
                        {
			einfo(_("%F%P: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
#line 2622 "ldgram.c"
    break;

  case 17: /* mri_script_command: LIST  */
#line 214 "ldgram.y"
                        {
			config.map_filename = "-";
			}
#line 2630 "ldgram.c"
    break;

  case 20: /* mri_script_command: PUBLIC NAME '=' exp  */
#line 220 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2636 "ldgram.c"
    break;

  case 21: /* mri_script_command: PUBLIC NAME ',' exp  */
#line 222 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2642 "ldgram.c"
    break;

  case 22: /* mri_script_command: PUBLIC NAME exp  */
#line 224 "ldgram.y"
                        { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2648 "ldgram.c"
    break;

  case 23: /* mri_script_command: FORMAT NAME  */
#line 226 "ldgram.y"
                        { mri_format((yyvsp[0].name)); }
#line 2654 "ldgram.c"
    break;

  case 24: /* mri_script_command: SECT NAME ',' exp  */
#line 228 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2660 "ldgram.c"
    break;

  case 25: /* mri_script_command: SECT NAME exp  */
#line 230 "ldgram.y"
                        { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2666 "ldgram.c"
    break;

  case 26: /* mri_script_command: SECT NAME '=' exp  */
#line 232 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2672 "ldgram.c"
    break;

  case 27: /* mri_script_command: ALIGN_K NAME '=' exp  */
#line 234 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2678 "ldgram.c"
    break;

  case 28: /* mri_script_command: ALIGN_K NAME ',' exp  */
#line 236 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2684 "ldgram.c"
    break;

  case 29: /* mri_script_command: ALIGNMOD NAME '=' exp  */
#line 238 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2690 "ldgram.c"
    break;

  case 30: /* mri_script_command: ALIGNMOD NAME ',' exp  */
#line 240 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2696 "ldgram.c"
    break;

  case 33: /* mri_script_command: NAMEWORD NAME  */
#line 244 "ldgram.y"
                        { mri_name((yyvsp[0].name)); }
#line 2702 "ldgram.c"
    break;

  case 34: /* mri_script_command: ALIAS NAME ',' NAME  */
#line 246 "ldgram.y"
                        { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2708 "ldgram.c"
    break;

  case 35: /* mri_script_command: ALIAS NAME ',' INT  */
#line 248 "ldgram.y"
                        { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2714 "ldgram.c"
    break;

  case 36: /* mri_script_command: BASE exp  */
#line 250 "ldgram.y"
                        { mri_base((yyvsp[0].etree)); }
#line 2720 "ldgram.c"
    break;

  case 37: /* mri_script_command: TRUNCATE INT  */
#line 252 "ldgram.y"
                { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2726 "ldgram.c"
    break;

  case 40: /* $@3: %empty  */
#line 256 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2732 "ldgram.c"
    break;

  case 42: /* mri_script_command: START NAME  */
#line 259 "ldgram.y"
                { lang_add_entry ((yyvsp[0].name), false); }
#line 2738 "ldgram.c"
    break;

  case 44: /* ordernamelist: ordernamelist ',' NAME  */
#line 264 "ldgram.y"
                                             { mri_order((yyvsp[0].name)); }
#line 2744 "ldgram.c"
    break;

  case 45: /* ordernamelist: ordernamelist NAME  */
#line 265 "ldgram.y"
                                          { mri_order((yyvsp[0].name)); }
#line 2750 "ldgram.c"
    break;

  case 47: /* mri_load_name_list: NAME  */
#line 271 "ldgram.y"
                        { mri_load((yyvsp[0].name)); }
#line 2756 "ldgram.c"
    break;

  case 48: /* mri_load_name_list: mri_load_name_list ',' NAME  */
#line 272 "ldgram.y"
                                            { mri_load((yyvsp[0].name)); }
#line 2762 "ldgram.c"
    break;

  case 49: /* mri_abs_name_list: NAME  */
#line 277 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2768 "ldgram.c"
    break;

  case 50: /* mri_abs_name_list: mri_abs_name_list ',' NAME  */
#line 279 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2774 "ldgram.c"
    break;

  case 51: /* casesymlist: %empty  */
#line 283 "ldgram.y"
                      { (yyval.name) = NULL; }
#line 2780 "ldgram.c"
    break;

  case 54: /* extern_name_list: NAME  */
#line 290 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2786 "ldgram.c"
    break;

  case 55: /* extern_name_list: extern_name_list NAME  */
#line 292 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2792 "ldgram.c"
    break;

  case 56: /* extern_name_list: extern_name_list ',' NAME  */
#line 294 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2798 "ldgram.c"
    break;

  case 57: /* $@4: %empty  */
#line 298 "ldgram.y"
        { ldlex_script (); }
#line 2804 "ldgram.c"
    break;

  case 58: /* script_file: $@4 ifile_list  */
#line 300 "ldgram.y"
        { ldlex_popstate (); }
#line 2810 "ldgram.c"
    break;

  case 71: /* ifile_p1: TARGET_K '(' NAME ')'  */
#line 321 "ldgram.y"
                { lang_add_target((yyvsp[-1].name)); }
#line 2816 "ldgram.c"
    break;

  case 72: /* ifile_p1: SEARCH_DIR '(' filename ')'  */
#line 323 "ldgram.y"
                { ldfile_add_library_path ((yyvsp[-1].name), false); }
#line 2822 "ldgram.c"
    break;

  case 73: /* ifile_p1: OUTPUT '(' filename ')'  */
#line 325 "ldgram.y"
                { lang_add_output((yyvsp[-1].name), 1); }
#line 2828 "ldgram.c"
    break;

  case 74: /* ifile_p1: OUTPUT_FORMAT '(' NAME ')'  */
#line 327 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2835 "ldgram.c"
    break;

  case 75: /* ifile_p1: OUTPUT_FORMAT '(' NAME ',' NAME ',' NAME ')'  */
#line 330 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2841 "ldgram.c"
    break;

  case 76: /* ifile_p1: OUTPUT_ARCH '(' NAME ')'  */
#line 332 "ldgram.y"
                  { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2847 "ldgram.c"
    break;

  case 77: /* ifile_p1: FORCE_COMMON_ALLOCATION  */
#line 334 "ldgram.y"
                { command_line.force_common_definition = true ; }
#line 2853 "ldgram.c"
    break;

  case 78: /* ifile_p1: FORCE_GROUP_ALLOCATION  */
#line 336 "ldgram.y"
                { command_line.force_group_allocation = true ; }
#line 2859 "ldgram.c"
    break;

  case 79: /* ifile_p1: INHIBIT_COMMON_ALLOCATION  */
#line 338 "ldgram.y"
                { link_info.inhibit_common_definition = true ; }
#line 2865 "ldgram.c"
    break;

  case 81: /* $@5: %empty  */
#line 341 "ldgram.y"
                  { lang_enter_group (); }
#line 2871 "ldgram.c"
    break;

  case 82: /* ifile_p1: GROUP $@5 '(' input_list ')'  */
#line 343 "ldgram.y"
                  { lang_leave_group (); }
#line 2877 "ldgram.c"
    break;

  case 83: /* ifile_p1: MAP '(' filename ')'  */
#line 345 "ldgram.y"
                { lang_add_map((yyvsp[-1].name)); }
#line 2883 "ldgram.c"
    break;

  case 84: /* $@6: %empty  */
#line 347 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2889 "ldgram.c"
    break;

  case 86: /* ifile_p1: NOCROSSREFS '(' nocrossref_list ')'  */
#line 350 "ldgram.y"
                {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2897 "ldgram.c"
    break;

  case 87: /* ifile_p1: NOCROSSREFS_TO '(' nocrossref_list ')'  */
#line 354 "ldgram.y"
                {
		  lang_add_nocrossref_to ((yyvsp[-1].nocrossref));
		}
#line 2905 "ldgram.c"
    break;

  case 88: /* $@7: %empty  */
#line 357 "ldgram.y"
                           { ldlex_expression (); }
#line 2911 "ldgram.c"
    break;

  case 89: /* ifile_p1: EXTERN '(' $@7 extern_name_list ')'  */
#line 358 "ldgram.y"
                        { ldlex_popstate (); }
#line 2917 "ldgram.c"
    break;

  case 90: /* ifile_p1: INSERT_K AFTER NAME  */
#line 360 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 0); }
#line 2923 "ldgram.c"
    break;

  case 91: /* ifile_p1: INSERT_K BEFORE NAME  */
#line 362 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 1); }
#line 2929 "ldgram.c"
    break;

  case 92: /* ifile_p1: REGION_ALIAS '(' NAME ',' NAME ')'  */
#line 364 "ldgram.y"
                { lang_memory_region_alias ((yyvsp[-3].name), (yyvsp[-1].name)); }
#line 2935 "ldgram.c"
    break;

  case 93: /* ifile_p1: LD_FEATURE '(' NAME ')'  */
#line 366 "ldgram.y"
                { lang_ld_feature ((yyvsp[-1].name)); }
#line 2941 "ldgram.c"
    break;

  case 94: /* $@8: %empty  */
#line 370 "ldgram.y"
                { ldlex_inputlist(); }
#line 2947 "ldgram.c"
    break;

  case 95: /* input_list: $@8 input_list1  */
#line 372 "ldgram.y"
                { ldlex_popstate(); }
#line 2953 "ldgram.c"
    break;

  case 96: /* input_list1: NAME  */
#line 376 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2960 "ldgram.c"
    break;

  case 97: /* input_list1: input_list1 ',' NAME  */
#line 379 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2967 "ldgram.c"
    break;

  case 98: /* input_list1: input_list1 NAME  */
#line 382 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2974 "ldgram.c"
    break;

  case 99: /* input_list1: LNAME  */
#line 385 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2981 "ldgram.c"
    break;

  case 100: /* input_list1: input_list1 ',' LNAME  */
#line 388 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2988 "ldgram.c"
    break;

  case 101: /* input_list1: input_list1 LNAME  */
#line 391 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2995 "ldgram.c"
    break;

  case 102: /* @9: %empty  */
#line 394 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3002 "ldgram.c"
    break;

  case 103: /* input_list1: AS_NEEDED '(' @9 input_list1 ')'  */
#line 397 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3008 "ldgram.c"
    break;

  case 104: /* @10: %empty  */
#line 399 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3015 "ldgram.c"
    break;

  case 105: /* input_list1: input_list1 ',' AS_NEEDED '(' @10 input_list1 ')'  */
#line 402 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3021 "ldgram.c"
    break;

  case 106: /* @11: %empty  */
#line 404 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3028 "ldgram.c"
    break;

  case 107: /* input_list1: input_list1 AS_NEEDED '(' @11 input_list1 ')'  */
#line 407 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3034 "ldgram.c"
    break;

  case 112: /* statement_anywhere: ENTRY '(' NAME ')'  */
#line 422 "ldgram.y"
                { lang_add_entry ((yyvsp[-1].name), false); }
#line 3040 "ldgram.c"
    break;

  case 114: /* $@12: %empty  */
#line 424 "ldgram.y"
                          {ldlex_expression ();}
#line 3046 "ldgram.c"
    break;

  case 115: /* statement_anywhere: ASSERT_K $@12 '(' exp ',' NAME ')'  */
#line 425 "ldgram.y"
                { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 3053 "ldgram.c"
    break;

  case 116: /* wildcard_name: NAME  */
#line 431 "ldgram.y"
                        {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 3061 "ldgram.c"
    break;

  case 117: /* wildcard_maybe_exclude: wildcard_name  */
#line 438 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			  (yyval.wildcard).reversed = false;
			}
#line 3073 "ldgram.c"
    break;

  case 118: /* wildcard_maybe_exclude: EXCLUDE_FILE '(' exclude_name_list ')' wildcard_name  */
#line 446 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			  (yyval.wildcard).reversed = false;
			}
#line 3085 "ldgram.c"
    break;

  case 120: /* wildcard_maybe_reverse: REVERSE '(' wildcard_maybe_exclude ')'  */
#line 458 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).reversed = true;
			  (yyval.wildcard).sorted = by_name;
			}
#line 3095 "ldgram.c"
    break;

  case 122: /* filename_spec: SORT_BY_NAME '(' wildcard_maybe_reverse ')'  */
#line 468 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3104 "ldgram.c"
    break;

  case 123: /* filename_spec: SORT_NONE '(' wildcard_maybe_reverse ')'  */
#line 473 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			  (yyval.wildcard).reversed = false;
			}
#line 3114 "ldgram.c"
    break;

  case 124: /* filename_spec: REVERSE '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 479 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).reversed = true;
			}
#line 3124 "ldgram.c"
    break;

  case 126: /* section_name_spec: SORT_BY_NAME '(' wildcard_maybe_reverse ')'  */
#line 489 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3133 "ldgram.c"
    break;

  case 127: /* section_name_spec: SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')'  */
#line 494 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3142 "ldgram.c"
    break;

  case 128: /* section_name_spec: SORT_NONE '(' wildcard_maybe_reverse ')'  */
#line 499 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 3151 "ldgram.c"
    break;

  case 129: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')' ')'  */
#line 504 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name_alignment;
			}
#line 3160 "ldgram.c"
    break;

  case 130: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_NAME '(' wildcard_maybe_reverse ')' ')'  */
#line 509 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3169 "ldgram.c"
    break;

  case 131: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_NAME '(' wildcard_maybe_reverse ')' ')'  */
#line 514 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment_name;
			}
#line 3178 "ldgram.c"
    break;

  case 132: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')' ')'  */
#line 519 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3187 "ldgram.c"
    break;

  case 133: /* section_name_spec: SORT_BY_INIT_PRIORITY '(' wildcard_maybe_reverse ')'  */
#line 524 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			}
#line 3196 "ldgram.c"
    break;

  case 134: /* section_name_spec: REVERSE '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 529 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).reversed = true;
			}
#line 3206 "ldgram.c"
    break;

  case 135: /* section_name_spec: REVERSE '(' SORT_BY_INIT_PRIORITY '(' wildcard_maybe_exclude ')' ')'  */
#line 535 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			  (yyval.wildcard).reversed = true;
			}
#line 3216 "ldgram.c"
    break;

  case 136: /* sect_flag_list: NAME  */
#line 543 "ldgram.y"
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
#line 3238 "ldgram.c"
    break;

  case 137: /* sect_flag_list: sect_flag_list '&' NAME  */
#line 561 "ldgram.y"
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
#line 3260 "ldgram.c"
    break;

  case 138: /* sect_flags: INPUT_SECTION_FLAGS '(' sect_flag_list ')'  */
#line 582 "ldgram.y"
                        {
			  struct flag_info *n;
			  n = ((struct flag_info *) xmalloc (sizeof *n));
			  n->flag_list = (yyvsp[-1].flag_info_list);
			  n->flags_initialized = false;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
#line 3274 "ldgram.c"
    break;

  case 139: /* exclude_name_list: exclude_name_list wildcard_name  */
#line 595 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 3286 "ldgram.c"
    break;

  case 140: /* exclude_name_list: wildcard_name  */
#line 604 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 3298 "ldgram.c"
    break;

  case 141: /* section_name_list: section_name_list opt_comma section_name_spec  */
#line 615 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3310 "ldgram.c"
    break;

  case 142: /* section_name_list: section_name_spec  */
#line 624 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3322 "ldgram.c"
    break;

  case 143: /* input_section_spec_no_keep: NAME  */
#line 635 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3335 "ldgram.c"
    break;

  case 144: /* input_section_spec_no_keep: sect_flags NAME  */
#line 644 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-1].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3348 "ldgram.c"
    break;

  case 145: /* input_section_spec_no_keep: '[' section_name_list ']'  */
#line 653 "ldgram.y"
                        {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3356 "ldgram.c"
    break;

  case 146: /* input_section_spec_no_keep: sect_flags '[' section_name_list ']'  */
#line 657 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-3].flag_info);
			  lang_add_wild (&tmp, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3369 "ldgram.c"
    break;

  case 147: /* input_section_spec_no_keep: filename_spec '(' section_name_list ')'  */
#line 666 "ldgram.y"
                        {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3377 "ldgram.c"
    break;

  case 148: /* input_section_spec_no_keep: sect_flags filename_spec '(' section_name_list ')'  */
#line 670 "ldgram.y"
                        {
			  (yyvsp[-3].wildcard).section_flag_list = (yyvsp[-4].flag_info);
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3386 "ldgram.c"
    break;

  case 150: /* $@13: %empty  */
#line 679 "ldgram.y"
                        { ldgram_had_keep = true; }
#line 3392 "ldgram.c"
    break;

  case 151: /* input_section_spec: KEEP '(' $@13 input_section_spec_no_keep ')'  */
#line 681 "ldgram.y"
                        { ldgram_had_keep = false; }
#line 3398 "ldgram.c"
    break;

  case 154: /* statement: CREATE_OBJECT_SYMBOLS  */
#line 688 "ldgram.y"
                {
		  lang_add_attribute (lang_object_symbols_statement_enum);
		}
#line 3406 "ldgram.c"
    break;

  case 155: /* statement: CONSTRUCTORS  */
#line 692 "ldgram.y"
                {
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3414 "ldgram.c"
    break;

  case 156: /* statement: SORT_BY_NAME '(' CONSTRUCTORS ')'  */
#line 696 "ldgram.y"
                {
		  constructors_sorted = true;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3423 "ldgram.c"
    break;

  case 158: /* statement: length '(' mustbe_exp ')'  */
#line 702 "ldgram.y"
                {
		  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
		}
#line 3431 "ldgram.c"
    break;

  case 159: /* statement: ASCIZ NAME  */
#line 706 "ldgram.y"
                {
		  lang_add_string ((yyvsp[0].name));
		}
#line 3439 "ldgram.c"
    break;

  case 160: /* statement: FILL '(' fill_exp ')'  */
#line 710 "ldgram.y"
                {
		  lang_add_fill ((yyvsp[-1].fill));
		}
#line 3447 "ldgram.c"
    break;

  case 161: /* statement: LINKER_VERSION  */
#line 714 "ldgram.y"
                {
		  lang_add_version_string ();
		}
#line 3455 "ldgram.c"
    break;

  case 162: /* $@14: %empty  */
#line 718 "ldgram.y"
                { ldlex_expression (); }
#line 3461 "ldgram.c"
    break;

  case 163: /* statement: ASSERT_K $@14 '(' exp ',' NAME ')' separator  */
#line 720 "ldgram.y"
                {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name)));
		}
#line 3470 "ldgram.c"
    break;

  case 164: /* $@15: %empty  */
#line 725 "ldgram.y"
                {
		  ldfile_open_command_file ((yyvsp[0].name));
		}
#line 3478 "ldgram.c"
    break;

  case 170: /* length: QUAD  */
#line 743 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3484 "ldgram.c"
    break;

  case 171: /* length: SQUAD  */
#line 745 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3490 "ldgram.c"
    break;

  case 172: /* length: LONG  */
#line 747 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3496 "ldgram.c"
    break;

  case 173: /* length: SHORT  */
#line 749 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3502 "ldgram.c"
    break;

  case 174: /* length: BYTE  */
#line 751 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3508 "ldgram.c"
    break;

  case 175: /* fill_exp: mustbe_exp  */
#line 756 "ldgram.y"
                {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, _("fill value"));
		}
#line 3516 "ldgram.c"
    break;

  case 176: /* fill_opt: '=' fill_exp  */
#line 763 "ldgram.y"
                { (yyval.fill) = (yyvsp[0].fill); }
#line 3522 "ldgram.c"
    break;

  case 177: /* fill_opt: %empty  */
#line 764 "ldgram.y"
                { (yyval.fill) = (fill_type *) 0; }
#line 3528 "ldgram.c"
    break;

  case 178: /* assign_op: PLUSEQ  */
#line 769 "ldgram.y"
                        { (yyval.token) = '+'; }
#line 3534 "ldgram.c"
    break;

  case 179: /* assign_op: MINUSEQ  */
#line 771 "ldgram.y"
                        { (yyval.token) = '-'; }
#line 3540 "ldgram.c"
    break;

  case 180: /* assign_op: MULTEQ  */
#line 773 "ldgram.y"
                        { (yyval.token) = '*'; }
#line 3546 "ldgram.c"
    break;

  case 181: /* assign_op: DIVEQ  */
#line 775 "ldgram.y"
                        { (yyval.token) = '/'; }
#line 3552 "ldgram.c"
    break;

  case 182: /* assign_op: LSHIFTEQ  */
#line 777 "ldgram.y"
                        { (yyval.token) = LSHIFT; }
#line 3558 "ldgram.c"
    break;

  case 183: /* assign_op: RSHIFTEQ  */
#line 779 "ldgram.y"
                        { (yyval.token) = RSHIFT; }
#line 3564 "ldgram.c"
    break;

  case 184: /* assign_op: ANDEQ  */
#line 781 "ldgram.y"
                        { (yyval.token) = '&'; }
#line 3570 "ldgram.c"
    break;

  case 185: /* assign_op: OREQ  */
#line 783 "ldgram.y"
                        { (yyval.token) = '|'; }
#line 3576 "ldgram.c"
    break;

  case 186: /* assign_op: XOREQ  */
#line 785 "ldgram.y"
                        { (yyval.token) = '^'; }
#line 3582 "ldgram.c"
    break;

  case 189: /* assignment: NAME '=' mustbe_exp  */
#line 795 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name), (yyvsp[0].etree), false));
		}
#line 3590 "ldgram.c"
    break;

  case 190: /* assignment: NAME assign_op mustbe_exp  */
#line 799 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name),
						   exp_binop ((yyvsp[-1].token),
							      exp_nameop (NAME,
									  (yyvsp[-2].name)),
							      (yyvsp[0].etree)), false));
		}
#line 3602 "ldgram.c"
    break;

  case 191: /* assignment: HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 807 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3610 "ldgram.c"
    break;

  case 192: /* assignment: PROVIDE '(' NAME '=' mustbe_exp ')'  */
#line 811 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), false));
		}
#line 3618 "ldgram.c"
    break;

  case 193: /* assignment: PROVIDE_HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 815 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3626 "ldgram.c"
    break;

  case 201: /* $@16: %empty  */
#line 838 "ldgram.y"
                { region = lang_memory_region_lookup ((yyvsp[0].name), true); }
#line 3632 "ldgram.c"
    break;

  case 202: /* memory_spec: NAME $@16 attributes_opt ':' origin_spec opt_comma length_spec  */
#line 841 "ldgram.y"
                {}
#line 3638 "ldgram.c"
    break;

  case 203: /* $@17: %empty  */
#line 843 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 3644 "ldgram.c"
    break;

  case 205: /* origin_spec: ORIGIN '=' mustbe_exp  */
#line 849 "ldgram.y"
                {
		  region->origin_exp = (yyvsp[0].etree);
		}
#line 3652 "ldgram.c"
    break;

  case 206: /* length_spec: LENGTH '=' mustbe_exp  */
#line 856 "ldgram.y"
                {
		  if (yychar == NAME)
		    {
		      yyclearin;
		      ldlex_backup ();
		    }
		  region->length_exp = (yyvsp[0].etree);
		}
#line 3665 "ldgram.c"
    break;

  case 207: /* attributes_opt: %empty  */
#line 868 "ldgram.y"
                  { /* dummy action to avoid bison 1.25 error message */ }
#line 3671 "ldgram.c"
    break;

  case 211: /* attributes_string: NAME  */
#line 879 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3677 "ldgram.c"
    break;

  case 212: /* attributes_string: '!' NAME  */
#line 881 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3683 "ldgram.c"
    break;

  case 213: /* startup: STARTUP '(' filename ')'  */
#line 886 "ldgram.y"
                { lang_startup((yyvsp[-1].name)); }
#line 3689 "ldgram.c"
    break;

  case 215: /* high_level_library: HLL '(' ')'  */
#line 892 "ldgram.y"
                        { ldemul_hll((char *)NULL); }
#line 3695 "ldgram.c"
    break;

  case 216: /* high_level_library_NAME_list: high_level_library_NAME_list opt_comma filename  */
#line 897 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3701 "ldgram.c"
    break;

  case 217: /* high_level_library_NAME_list: filename  */
#line 899 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3707 "ldgram.c"
    break;

  case 219: /* low_level_library_NAME_list: low_level_library_NAME_list opt_comma filename  */
#line 908 "ldgram.y"
                        { ldemul_syslib((yyvsp[0].name)); }
#line 3713 "ldgram.c"
    break;

  case 221: /* floating_point_support: FLOAT  */
#line 914 "ldgram.y"
                        { lang_float(true); }
#line 3719 "ldgram.c"
    break;

  case 222: /* floating_point_support: NOFLOAT  */
#line 916 "ldgram.y"
                        { lang_float(false); }
#line 3725 "ldgram.c"
    break;

  case 223: /* nocrossref_list: %empty  */
#line 921 "ldgram.y"
                {
		  (yyval.nocrossref) = NULL;
		}
#line 3733 "ldgram.c"
    break;

  case 224: /* nocrossref_list: NAME nocrossref_list  */
#line 925 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3746 "ldgram.c"
    break;

  case 225: /* nocrossref_list: NAME ',' nocrossref_list  */
#line 934 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3759 "ldgram.c"
    break;

  case 226: /* $@18: %empty  */
#line 944 "ldgram.y"
                        { ldlex_script (); }
#line 3765 "ldgram.c"
    break;

  case 227: /* paren_script_name: $@18 '(' NAME ')'  */
#line 946 "ldgram.y"
                        { ldlex_popstate (); (yyval.name) = (yyvsp[-1].name); }
#line 3771 "ldgram.c"
    break;

  case 228: /* $@19: %empty  */
#line 948 "ldgram.y"
                        { ldlex_expression (); }
#line 3777 "ldgram.c"
    break;

  case 229: /* mustbe_exp: $@19 exp  */
#line 950 "ldgram.y"
                        { ldlex_popstate (); (yyval.etree) = (yyvsp[0].etree); }
#line 3783 "ldgram.c"
    break;

  case 230: /* exp: '-' exp  */
#line 955 "ldgram.y"
                        { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3789 "ldgram.c"
    break;

  case 231: /* exp: '(' exp ')'  */
#line 957 "ldgram.y"
                        { (yyval.etree) = (yyvsp[-1].etree); }
#line 3795 "ldgram.c"
    break;

  case 232: /* exp: NEXT '(' exp ')'  */
#line 959 "ldgram.y"
                        { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3801 "ldgram.c"
    break;

  case 233: /* exp: '!' exp  */
#line 961 "ldgram.y"
                        { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3807 "ldgram.c"
    break;

  case 234: /* exp: '+' exp  */
#line 963 "ldgram.y"
                        { (yyval.etree) = (yyvsp[0].etree); }
#line 3813 "ldgram.c"
    break;

  case 235: /* exp: '~' exp  */
#line 965 "ldgram.y"
                        { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3819 "ldgram.c"
    break;

  case 236: /* exp: exp '*' exp  */
#line 968 "ldgram.y"
                        { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3825 "ldgram.c"
    break;

  case 237: /* exp: exp '/' exp  */
#line 970 "ldgram.y"
                        { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3831 "ldgram.c"
    break;

  case 238: /* exp: exp '%' exp  */
#line 972 "ldgram.y"
                        { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3837 "ldgram.c"
    break;

  case 239: /* exp: exp '+' exp  */
#line 974 "ldgram.y"
                        { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3843 "ldgram.c"
    break;

  case 240: /* exp: exp '-' exp  */
#line 976 "ldgram.y"
                        { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3849 "ldgram.c"
    break;

  case 241: /* exp: exp LSHIFT exp  */
#line 978 "ldgram.y"
                        { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3855 "ldgram.c"
    break;

  case 242: /* exp: exp RSHIFT exp  */
#line 980 "ldgram.y"
                        { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3861 "ldgram.c"
    break;

  case 243: /* exp: exp EQ exp  */
#line 982 "ldgram.y"
                        { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3867 "ldgram.c"
    break;

  case 244: /* exp: exp NE exp  */
#line 984 "ldgram.y"
                        { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3873 "ldgram.c"
    break;

  case 245: /* exp: exp LE exp  */
#line 986 "ldgram.y"
                        { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3879 "ldgram.c"
    break;

  case 246: /* exp: exp GE exp  */
#line 988 "ldgram.y"
                        { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3885 "ldgram.c"
    break;

  case 247: /* exp: exp '<' exp  */
#line 990 "ldgram.y"
                        { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3891 "ldgram.c"
    break;

  case 248: /* exp: exp '>' exp  */
#line 992 "ldgram.y"
                        { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3897 "ldgram.c"
    break;

  case 249: /* exp: exp '&' exp  */
#line 994 "ldgram.y"
                        { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3903 "ldgram.c"
    break;

  case 250: /* exp: exp '^' exp  */
#line 996 "ldgram.y"
                        { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3909 "ldgram.c"
    break;

  case 251: /* exp: exp '|' exp  */
#line 998 "ldgram.y"
                        { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3915 "ldgram.c"
    break;

  case 252: /* exp: exp '?' exp ':' exp  */
#line 1000 "ldgram.y"
                        { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3921 "ldgram.c"
    break;

  case 253: /* exp: exp ANDAND exp  */
#line 1002 "ldgram.y"
                        { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3927 "ldgram.c"
    break;

  case 254: /* exp: exp OROR exp  */
#line 1004 "ldgram.y"
                        { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3933 "ldgram.c"
    break;

  case 255: /* exp: DEFINED '(' NAME ')'  */
#line 1006 "ldgram.y"
                        { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3939 "ldgram.c"
    break;

  case 256: /* exp: INT  */
#line 1008 "ldgram.y"
                        { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3945 "ldgram.c"
    break;

  case 257: /* exp: SIZEOF_HEADERS  */
#line 1010 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3951 "ldgram.c"
    break;

  case 258: /* exp: ALIGNOF paren_script_name  */
#line 1013 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ALIGNOF, (yyvsp[0].name)); }
#line 3957 "ldgram.c"
    break;

  case 259: /* exp: SIZEOF paren_script_name  */
#line 1015 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF, (yyvsp[0].name)); }
#line 3963 "ldgram.c"
    break;

  case 260: /* exp: ADDR paren_script_name  */
#line 1017 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ADDR, (yyvsp[0].name)); }
#line 3969 "ldgram.c"
    break;

  case 261: /* exp: LOADADDR paren_script_name  */
#line 1019 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LOADADDR, (yyvsp[0].name)); }
#line 3975 "ldgram.c"
    break;

  case 262: /* exp: CONSTANT '(' NAME ')'  */
#line 1021 "ldgram.y"
                        { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 3981 "ldgram.c"
    break;

  case 263: /* exp: ABSOLUTE '(' exp ')'  */
#line 1023 "ldgram.y"
                        { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 3987 "ldgram.c"
    break;

  case 264: /* exp: ALIGN_K '(' exp ')'  */
#line 1025 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3993 "ldgram.c"
    break;

  case 265: /* exp: ALIGN_K '(' exp ',' exp ')'  */
#line 1027 "ldgram.y"
                        { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 3999 "ldgram.c"
    break;

  case 266: /* exp: DATA_SEGMENT_ALIGN '(' exp ',' exp ')'  */
#line 1029 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 4005 "ldgram.c"
    break;

  case 267: /* exp: DATA_SEGMENT_RELRO_END '(' exp ',' exp ')'  */
#line 1031 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 4011 "ldgram.c"
    break;

  case 268: /* exp: DATA_SEGMENT_END '(' exp ')'  */
#line 1033 "ldgram.y"
                        { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 4017 "ldgram.c"
    break;

  case 269: /* $@20: %empty  */
#line 1034 "ldgram.y"
                              { ldlex_script (); }
#line 4023 "ldgram.c"
    break;

  case 270: /* $@21: %empty  */
#line 1035 "ldgram.y"
                        { ldlex_popstate (); }
#line 4029 "ldgram.c"
    break;

  case 271: /* exp: SEGMENT_START $@20 '(' NAME $@21 ',' exp ')'  */
#line 1036 "ldgram.y"
                        { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-4].name))); }
#line 4042 "ldgram.c"
    break;

  case 272: /* exp: BLOCK '(' exp ')'  */
#line 1045 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 4048 "ldgram.c"
    break;

  case 273: /* exp: NAME  */
#line 1047 "ldgram.y"
                        { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 4054 "ldgram.c"
    break;

  case 274: /* exp: MAX_K '(' exp ',' exp ')'  */
#line 1049 "ldgram.y"
                        { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 4060 "ldgram.c"
    break;

  case 275: /* exp: MIN_K '(' exp ',' exp ')'  */
#line 1051 "ldgram.y"
                        { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 4066 "ldgram.c"
    break;

  case 276: /* exp: ASSERT_K '(' exp ',' NAME ')'  */
#line 1053 "ldgram.y"
                        { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 4072 "ldgram.c"
    break;

  case 277: /* exp: ORIGIN paren_script_name  */
#line 1055 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[0].name)); }
#line 4078 "ldgram.c"
    break;

  case 278: /* exp: LENGTH paren_script_name  */
#line 1057 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[0].name)); }
#line 4084 "ldgram.c"
    break;

  case 279: /* exp: LOG2CEIL '(' exp ')'  */
#line 1059 "ldgram.y"
                        { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[-1].etree)); }
#line 4090 "ldgram.c"
    break;

  case 280: /* memspec_at_opt: AT '>' NAME  */
#line 1064 "ldgram.y"
                            { (yyval.name) = (yyvsp[0].name); }
#line 4096 "ldgram.c"
    break;

  case 281: /* memspec_at_opt: %empty  */
#line 1065 "ldgram.y"
                { (yyval.name) = 0; }
#line 4102 "ldgram.c"
    break;

  case 282: /* opt_at: AT '(' exp ')'  */
#line 1069 "ldgram.y"
                               { (yyval.etree) = (yyvsp[-1].etree); }
#line 4108 "ldgram.c"
    break;

  case 283: /* opt_at: %empty  */
#line 1070 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4114 "ldgram.c"
    break;

  case 284: /* opt_align: ALIGN_K '(' exp ')'  */
#line 1074 "ldgram.y"
                                    { (yyval.etree) = (yyvsp[-1].etree); }
#line 4120 "ldgram.c"
    break;

  case 285: /* opt_align: %empty  */
#line 1075 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4126 "ldgram.c"
    break;

  case 286: /* opt_align_with_input: ALIGN_WITH_INPUT  */
#line 1079 "ldgram.y"
                                 { (yyval.token) = ALIGN_WITH_INPUT; }
#line 4132 "ldgram.c"
    break;

  case 287: /* opt_align_with_input: %empty  */
#line 1080 "ldgram.y"
                { (yyval.token) = 0; }
#line 4138 "ldgram.c"
    break;

  case 288: /* opt_subalign: SUBALIGN '(' exp ')'  */
#line 1084 "ldgram.y"
                                     { (yyval.etree) = (yyvsp[-1].etree); }
#line 4144 "ldgram.c"
    break;

  case 289: /* opt_subalign: %empty  */
#line 1085 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4150 "ldgram.c"
    break;

  case 290: /* sect_constraint: ONLY_IF_RO  */
#line 1089 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RO; }
#line 4156 "ldgram.c"
    break;

  case 291: /* sect_constraint: ONLY_IF_RW  */
#line 1090 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RW; }
#line 4162 "ldgram.c"
    break;

  case 292: /* sect_constraint: SPECIAL  */
#line 1091 "ldgram.y"
                        { (yyval.token) = SPECIAL; }
#line 4168 "ldgram.c"
    break;

  case 293: /* sect_constraint: %empty  */
#line 1092 "ldgram.y"
                { (yyval.token) = 0; }
#line 4174 "ldgram.c"
    break;

  case 294: /* $@22: %empty  */
#line 1096 "ldgram.y"
                        { ldlex_expression(); }
#line 4180 "ldgram.c"
    break;

  case 295: /* $@23: %empty  */
#line 1103 "ldgram.y"
                        {
			  ldlex_popstate ();
			  ldlex_wild ();
			  lang_enter_output_section_statement ((yyvsp[-7].name), (yyvsp[-5].etree), sectype,
					sectype_value, (yyvsp[-3].etree), (yyvsp[-1].etree), (yyvsp[-4].etree), (yyvsp[0].token), (yyvsp[-2].token));
			}
#line 4191 "ldgram.c"
    break;

  case 296: /* $@24: %empty  */
#line 1112 "ldgram.y"
                        { ldlex_popstate (); }
#line 4197 "ldgram.c"
    break;

  case 297: /* $@25: %empty  */
#line 1114 "ldgram.y"
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
#line 4218 "ldgram.c"
    break;

  case 299: /* $@26: %empty  */
#line 1132 "ldgram.y"
                        { ldlex_expression (); }
#line 4224 "ldgram.c"
    break;

  case 300: /* $@27: %empty  */
#line 1134 "ldgram.y"
                        { ldlex_popstate (); }
#line 4230 "ldgram.c"
    break;

  case 301: /* $@28: %empty  */
#line 1136 "ldgram.y"
                        {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 4238 "ldgram.c"
    break;

  case 302: /* $@29: %empty  */
#line 1142 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay ((yyvsp[-10].etree), (int) (yyvsp[-11].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 4252 "ldgram.c"
    break;

  case 304: /* $@30: %empty  */
#line 1157 "ldgram.y"
                        { ldlex_expression (); }
#line 4258 "ldgram.c"
    break;

  case 305: /* $@31: %empty  */
#line 1159 "ldgram.y"
                        {
			  ldlex_popstate ();
			  lang_add_assignment (exp_assign (".", (yyvsp[0].etree), false));
			}
#line 4267 "ldgram.c"
    break;

  case 307: /* $@32: %empty  */
#line 1165 "ldgram.y"
                        {
			  ldfile_open_command_file ((yyvsp[0].name));
			}
#line 4275 "ldgram.c"
    break;

  case 309: /* type: NOLOAD  */
#line 1172 "ldgram.y"
                   { sectype = noload_section; }
#line 4281 "ldgram.c"
    break;

  case 310: /* type: DSECT  */
#line 1173 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4287 "ldgram.c"
    break;

  case 311: /* type: COPY  */
#line 1174 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4293 "ldgram.c"
    break;

  case 312: /* type: INFO  */
#line 1175 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4299 "ldgram.c"
    break;

  case 313: /* type: OVERLAY  */
#line 1176 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4305 "ldgram.c"
    break;

  case 314: /* type: READONLY '(' TYPE '=' exp ')'  */
#line 1177 "ldgram.y"
                                         { sectype = typed_readonly_section; sectype_value = (yyvsp[-1].etree); }
#line 4311 "ldgram.c"
    break;

  case 315: /* type: READONLY  */
#line 1178 "ldgram.y"
                    { sectype = readonly_section; }
#line 4317 "ldgram.c"
    break;

  case 316: /* type: TYPE '=' exp  */
#line 1179 "ldgram.y"
                        { sectype = type_section; sectype_value = (yyvsp[0].etree); }
#line 4323 "ldgram.c"
    break;

  case 318: /* atype: %empty  */
#line 1184 "ldgram.y"
                            { sectype = normal_section; }
#line 4329 "ldgram.c"
    break;

  case 319: /* atype: '(' ')'  */
#line 1185 "ldgram.y"
                        { sectype = normal_section; }
#line 4335 "ldgram.c"
    break;

  case 320: /* opt_exp_with_type: exp atype ':'  */
#line 1189 "ldgram.y"
                                        { (yyval.etree) = (yyvsp[-2].etree); }
#line 4341 "ldgram.c"
    break;

  case 321: /* opt_exp_with_type: atype ':'  */
#line 1190 "ldgram.y"
                                        { (yyval.etree) = (etree_type *)NULL;  }
#line 4347 "ldgram.c"
    break;

  case 322: /* opt_exp_with_type: BIND '(' exp ')' atype ':'  */
#line 1195 "ldgram.y"
                                           { (yyval.etree) = (yyvsp[-3].etree); }
#line 4353 "ldgram.c"
    break;

  case 323: /* opt_exp_with_type: BIND '(' exp ')' BLOCK '(' exp ')' atype ':'  */
#line 1197 "ldgram.y"
                { (yyval.etree) = (yyvsp[-7].etree); }
#line 4359 "ldgram.c"
    break;

  case 324: /* opt_exp_without_type: exp ':'  */
#line 1201 "ldgram.y"
                                { (yyval.etree) = (yyvsp[-1].etree); }
#line 4365 "ldgram.c"
    break;

  case 325: /* opt_exp_without_type: ':'  */
#line 1202 "ldgram.y"
                                { (yyval.etree) = (etree_type *) NULL;  }
#line 4371 "ldgram.c"
    break;

  case 326: /* opt_nocrossrefs: %empty  */
#line 1207 "ldgram.y"
                        { (yyval.integer) = 0; }
#line 4377 "ldgram.c"
    break;

  case 327: /* opt_nocrossrefs: NOCROSSREFS  */
#line 1209 "ldgram.y"
                        { (yyval.integer) = 1; }
#line 4383 "ldgram.c"
    break;

  case 328: /* memspec_opt: '>' NAME  */
#line 1214 "ldgram.y"
                { (yyval.name) = (yyvsp[0].name); }
#line 4389 "ldgram.c"
    break;

  case 329: /* memspec_opt: %empty  */
#line 1215 "ldgram.y"
                { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 4395 "ldgram.c"
    break;

  case 330: /* phdr_opt: %empty  */
#line 1220 "ldgram.y"
                {
		  (yyval.section_phdr) = NULL;
		}
#line 4403 "ldgram.c"
    break;

  case 331: /* phdr_opt: phdr_opt ':' NAME  */
#line 1224 "ldgram.y"
                {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = false;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 4418 "ldgram.c"
    break;

  case 333: /* $@33: %empty  */
#line 1240 "ldgram.y"
                        {
			  ldlex_wild ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 4427 "ldgram.c"
    break;

  case 334: /* $@34: %empty  */
#line 1247 "ldgram.y"
                        { ldlex_popstate (); }
#line 4433 "ldgram.c"
    break;

  case 335: /* $@35: %empty  */
#line 1249 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 4446 "ldgram.c"
    break;

  case 340: /* $@36: %empty  */
#line 1270 "ldgram.y"
                     { ldlex_expression (); }
#line 4452 "ldgram.c"
    break;

  case 341: /* $@37: %empty  */
#line 1271 "ldgram.y"
                                            { ldlex_popstate (); }
#line 4458 "ldgram.c"
    break;

  case 342: /* phdr: NAME $@36 phdr_type phdr_qualifiers $@37 ';'  */
#line 1273 "ldgram.y"
                {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 4467 "ldgram.c"
    break;

  case 343: /* phdr_type: exp  */
#line 1281 "ldgram.y"
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
#line 4516 "ldgram.c"
    break;

  case 344: /* phdr_qualifiers: %empty  */
#line 1329 "ldgram.y"
                {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 4524 "ldgram.c"
    break;

  case 345: /* phdr_qualifiers: NAME phdr_val phdr_qualifiers  */
#line 1333 "ldgram.y"
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
#line 4541 "ldgram.c"
    break;

  case 346: /* phdr_qualifiers: AT '(' exp ')' phdr_qualifiers  */
#line 1346 "ldgram.y"
                {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 4550 "ldgram.c"
    break;

  case 347: /* phdr_val: %empty  */
#line 1354 "ldgram.y"
                {
		  (yyval.etree) = NULL;
		}
#line 4558 "ldgram.c"
    break;

  case 348: /* phdr_val: '(' exp ')'  */
#line 1358 "ldgram.y"
                {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 4566 "ldgram.c"
    break;

  case 349: /* $@38: %empty  */
#line 1364 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 4575 "ldgram.c"
    break;

  case 350: /* dynamic_list_file: $@38 dynamic_list_nodes  */
#line 1369 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4584 "ldgram.c"
    break;

  case 354: /* dynamic_list_tag: vers_defns ';'  */
#line 1386 "ldgram.y"
                {
		  lang_append_dynamic_list (current_dynamic_list_p, (yyvsp[-1].versyms));
		}
#line 4592 "ldgram.c"
    break;

  case 355: /* $@39: %empty  */
#line 1394 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 4601 "ldgram.c"
    break;

  case 356: /* version_script_file: $@39 vers_nodes  */
#line 1399 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4610 "ldgram.c"
    break;

  case 357: /* $@40: %empty  */
#line 1408 "ldgram.y"
                {
		  ldlex_version_script ();
		}
#line 4618 "ldgram.c"
    break;

  case 358: /* version: $@40 VERSIONK '{' vers_nodes '}'  */
#line 1412 "ldgram.y"
                {
		  ldlex_popstate ();
		}
#line 4626 "ldgram.c"
    break;

  case 361: /* vers_node: '{' vers_tag '}' ';'  */
#line 1424 "ldgram.y"
                {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 4634 "ldgram.c"
    break;

  case 362: /* vers_node: VERS_TAG '{' vers_tag '}' ';'  */
#line 1428 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 4642 "ldgram.c"
    break;

  case 363: /* vers_node: VERS_TAG '{' vers_tag '}' verdep ';'  */
#line 1432 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 4650 "ldgram.c"
    break;

  case 364: /* verdep: VERS_TAG  */
#line 1439 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 4658 "ldgram.c"
    break;

  case 365: /* verdep: verdep VERS_TAG  */
#line 1443 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4666 "ldgram.c"
    break;

  case 366: /* vers_tag: %empty  */
#line 1450 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4674 "ldgram.c"
    break;

  case 367: /* vers_tag: vers_defns ';'  */
#line 1454 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4682 "ldgram.c"
    break;

  case 368: /* vers_tag: GLOBAL ':' vers_defns ';'  */
#line 1458 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4690 "ldgram.c"
    break;

  case 369: /* vers_tag: LOCAL ':' vers_defns ';'  */
#line 1462 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4698 "ldgram.c"
    break;

  case 370: /* vers_tag: GLOBAL ':' vers_defns ';' LOCAL ':' vers_defns ';'  */
#line 1466 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4706 "ldgram.c"
    break;

  case 371: /* vers_defns: VERS_IDENTIFIER  */
#line 1473 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4714 "ldgram.c"
    break;

  case 372: /* vers_defns: NAME  */
#line 1477 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4722 "ldgram.c"
    break;

  case 373: /* vers_defns: vers_defns ';' VERS_IDENTIFIER  */
#line 1481 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4730 "ldgram.c"
    break;

  case 374: /* vers_defns: vers_defns ';' NAME  */
#line 1485 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4738 "ldgram.c"
    break;

  case 375: /* @41: %empty  */
#line 1489 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4747 "ldgram.c"
    break;

  case 376: /* vers_defns: vers_defns ';' EXTERN NAME '{' @41 vers_defns opt_semicolon '}'  */
#line 1494 "ldgram.y"
                        {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4759 "ldgram.c"
    break;

  case 377: /* @42: %empty  */
#line 1502 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4768 "ldgram.c"
    break;

  case 378: /* vers_defns: EXTERN NAME '{' @42 vers_defns opt_semicolon '}'  */
#line 1507 "ldgram.y"
                        {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4777 "ldgram.c"
    break;

  case 379: /* vers_defns: GLOBAL  */
#line 1512 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, false);
		}
#line 4785 "ldgram.c"
    break;

  case 380: /* vers_defns: vers_defns ';' GLOBAL  */
#line 1516 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, false);
		}
#line 4793 "ldgram.c"
    break;

  case 381: /* vers_defns: LOCAL  */
#line 1520 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, false);
		}
#line 4801 "ldgram.c"
    break;

  case 382: /* vers_defns: vers_defns ';' LOCAL  */
#line 1524 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, false);
		}
#line 4809 "ldgram.c"
    break;

  case 383: /* vers_defns: EXTERN  */
#line 1528 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, false);
		}
#line 4817 "ldgram.c"
    break;

  case 384: /* vers_defns: vers_defns ';' EXTERN  */
#line 1532 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, false);
		}
#line 4825 "ldgram.c"
    break;


#line 4829 "ldgram.c"

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

#line 1542 "ldgram.y"

static void
yyerror (const char *arg)
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldlex_filename ());
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
    einfo (_("%F%P:%pS: %s in %s\n"), NULL, arg, error_names[error_index - 1]);
  else
    einfo ("%F%P:%pS: %s\n", NULL, arg);
}
