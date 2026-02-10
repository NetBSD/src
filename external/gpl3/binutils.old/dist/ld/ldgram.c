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
    INPUT_SECTION_ORDERING_SCRIPT = 385, /* INPUT_SECTION_ORDERING_SCRIPT  */
    KEEP = 386,                    /* KEEP  */
    ONLY_IF_RO = 387,              /* ONLY_IF_RO  */
    ONLY_IF_RW = 388,              /* ONLY_IF_RW  */
    SPECIAL = 389,                 /* SPECIAL  */
    INPUT_SECTION_FLAGS = 390,     /* INPUT_SECTION_FLAGS  */
    ALIGN_WITH_INPUT = 391,        /* ALIGN_WITH_INPUT  */
    EXCLUDE_FILE = 392,            /* EXCLUDE_FILE  */
    CONSTANT = 393,                /* CONSTANT  */
    INPUT_DYNAMIC_LIST = 394       /* INPUT_DYNAMIC_LIST  */
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
#define INPUT_SECTION_ORDERING_SCRIPT 385
#define KEEP 386
#define ONLY_IF_RO 387
#define ONLY_IF_RW 388
#define SPECIAL 389
#define INPUT_SECTION_FLAGS 390
#define ALIGN_WITH_INPUT 391
#define EXCLUDE_FILE 392
#define CONSTANT 393
#define INPUT_DYNAMIC_LIST 394

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

#line 477 "ldgram.c"

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
  YYSYMBOL_INPUT_SECTION_ORDERING_SCRIPT = 146, /* INPUT_SECTION_ORDERING_SCRIPT  */
  YYSYMBOL_KEEP = 147,                     /* KEEP  */
  YYSYMBOL_ONLY_IF_RO = 148,               /* ONLY_IF_RO  */
  YYSYMBOL_ONLY_IF_RW = 149,               /* ONLY_IF_RW  */
  YYSYMBOL_SPECIAL = 150,                  /* SPECIAL  */
  YYSYMBOL_INPUT_SECTION_FLAGS = 151,      /* INPUT_SECTION_FLAGS  */
  YYSYMBOL_ALIGN_WITH_INPUT = 152,         /* ALIGN_WITH_INPUT  */
  YYSYMBOL_EXCLUDE_FILE = 153,             /* EXCLUDE_FILE  */
  YYSYMBOL_CONSTANT = 154,                 /* CONSTANT  */
  YYSYMBOL_INPUT_DYNAMIC_LIST = 155,       /* INPUT_DYNAMIC_LIST  */
  YYSYMBOL_156_ = 156,                     /* ','  */
  YYSYMBOL_157_ = 157,                     /* ';'  */
  YYSYMBOL_158_ = 158,                     /* ')'  */
  YYSYMBOL_159_ = 159,                     /* '['  */
  YYSYMBOL_160_ = 160,                     /* ']'  */
  YYSYMBOL_161_ = 161,                     /* '!'  */
  YYSYMBOL_162_ = 162,                     /* '~'  */
  YYSYMBOL_YYACCEPT = 163,                 /* $accept  */
  YYSYMBOL_file = 164,                     /* file  */
  YYSYMBOL_filename = 165,                 /* filename  */
  YYSYMBOL_defsym_expr = 166,              /* defsym_expr  */
  YYSYMBOL_167_1 = 167,                    /* $@1  */
  YYSYMBOL_mri_script_file = 168,          /* mri_script_file  */
  YYSYMBOL_169_2 = 169,                    /* $@2  */
  YYSYMBOL_mri_script_lines = 170,         /* mri_script_lines  */
  YYSYMBOL_mri_script_command = 171,       /* mri_script_command  */
  YYSYMBOL_172_3 = 172,                    /* $@3  */
  YYSYMBOL_ordernamelist = 173,            /* ordernamelist  */
  YYSYMBOL_mri_load_name_list = 174,       /* mri_load_name_list  */
  YYSYMBOL_mri_abs_name_list = 175,        /* mri_abs_name_list  */
  YYSYMBOL_casesymlist = 176,              /* casesymlist  */
  YYSYMBOL_extern_name_list = 177,         /* extern_name_list  */
  YYSYMBOL_script_file = 178,              /* script_file  */
  YYSYMBOL_179_4 = 179,                    /* $@4  */
  YYSYMBOL_ifile_list = 180,               /* ifile_list  */
  YYSYMBOL_ifile_p1 = 181,                 /* ifile_p1  */
  YYSYMBOL_182_5 = 182,                    /* $@5  */
  YYSYMBOL_183_6 = 183,                    /* $@6  */
  YYSYMBOL_184_7 = 184,                    /* $@7  */
  YYSYMBOL_input_list = 185,               /* input_list  */
  YYSYMBOL_186_8 = 186,                    /* $@8  */
  YYSYMBOL_input_list1 = 187,              /* input_list1  */
  YYSYMBOL_188_9 = 188,                    /* @9  */
  YYSYMBOL_189_10 = 189,                   /* @10  */
  YYSYMBOL_190_11 = 190,                   /* @11  */
  YYSYMBOL_sections = 191,                 /* sections  */
  YYSYMBOL_sec_or_group_p1 = 192,          /* sec_or_group_p1  */
  YYSYMBOL_statement_anywhere = 193,       /* statement_anywhere  */
  YYSYMBOL_194_12 = 194,                   /* $@12  */
  YYSYMBOL_wildcard_name = 195,            /* wildcard_name  */
  YYSYMBOL_wildcard_maybe_exclude = 196,   /* wildcard_maybe_exclude  */
  YYSYMBOL_wildcard_maybe_reverse = 197,   /* wildcard_maybe_reverse  */
  YYSYMBOL_filename_spec = 198,            /* filename_spec  */
  YYSYMBOL_section_name_spec = 199,        /* section_name_spec  */
  YYSYMBOL_sect_flag_list = 200,           /* sect_flag_list  */
  YYSYMBOL_sect_flags = 201,               /* sect_flags  */
  YYSYMBOL_exclude_name_list = 202,        /* exclude_name_list  */
  YYSYMBOL_section_name_list = 203,        /* section_name_list  */
  YYSYMBOL_input_section_spec_no_keep = 204, /* input_section_spec_no_keep  */
  YYSYMBOL_input_section_spec = 205,       /* input_section_spec  */
  YYSYMBOL_206_13 = 206,                   /* $@13  */
  YYSYMBOL_statement = 207,                /* statement  */
  YYSYMBOL_208_14 = 208,                   /* $@14  */
  YYSYMBOL_209_15 = 209,                   /* $@15  */
  YYSYMBOL_statement_list = 210,           /* statement_list  */
  YYSYMBOL_statement_list_opt = 211,       /* statement_list_opt  */
  YYSYMBOL_length = 212,                   /* length  */
  YYSYMBOL_fill_exp = 213,                 /* fill_exp  */
  YYSYMBOL_fill_opt = 214,                 /* fill_opt  */
  YYSYMBOL_assign_op = 215,                /* assign_op  */
  YYSYMBOL_separator = 216,                /* separator  */
  YYSYMBOL_assignment = 217,               /* assignment  */
  YYSYMBOL_opt_comma = 218,                /* opt_comma  */
  YYSYMBOL_memory = 219,                   /* memory  */
  YYSYMBOL_memory_spec_list_opt = 220,     /* memory_spec_list_opt  */
  YYSYMBOL_memory_spec_list = 221,         /* memory_spec_list  */
  YYSYMBOL_memory_spec = 222,              /* memory_spec  */
  YYSYMBOL_223_16 = 223,                   /* $@16  */
  YYSYMBOL_224_17 = 224,                   /* $@17  */
  YYSYMBOL_origin_spec = 225,              /* origin_spec  */
  YYSYMBOL_length_spec = 226,              /* length_spec  */
  YYSYMBOL_attributes_opt = 227,           /* attributes_opt  */
  YYSYMBOL_attributes_list = 228,          /* attributes_list  */
  YYSYMBOL_attributes_string = 229,        /* attributes_string  */
  YYSYMBOL_startup = 230,                  /* startup  */
  YYSYMBOL_high_level_library = 231,       /* high_level_library  */
  YYSYMBOL_high_level_library_NAME_list = 232, /* high_level_library_NAME_list  */
  YYSYMBOL_low_level_library = 233,        /* low_level_library  */
  YYSYMBOL_low_level_library_NAME_list = 234, /* low_level_library_NAME_list  */
  YYSYMBOL_floating_point_support = 235,   /* floating_point_support  */
  YYSYMBOL_nocrossref_list = 236,          /* nocrossref_list  */
  YYSYMBOL_paren_script_name = 237,        /* paren_script_name  */
  YYSYMBOL_238_18 = 238,                   /* $@18  */
  YYSYMBOL_mustbe_exp = 239,               /* mustbe_exp  */
  YYSYMBOL_240_19 = 240,                   /* $@19  */
  YYSYMBOL_exp = 241,                      /* exp  */
  YYSYMBOL_242_20 = 242,                   /* $@20  */
  YYSYMBOL_243_21 = 243,                   /* $@21  */
  YYSYMBOL_memspec_at_opt = 244,           /* memspec_at_opt  */
  YYSYMBOL_opt_at = 245,                   /* opt_at  */
  YYSYMBOL_opt_align = 246,                /* opt_align  */
  YYSYMBOL_opt_align_with_input = 247,     /* opt_align_with_input  */
  YYSYMBOL_opt_subalign = 248,             /* opt_subalign  */
  YYSYMBOL_sect_constraint = 249,          /* sect_constraint  */
  YYSYMBOL_section = 250,                  /* section  */
  YYSYMBOL_251_22 = 251,                   /* $@22  */
  YYSYMBOL_252_23 = 252,                   /* $@23  */
  YYSYMBOL_253_24 = 253,                   /* $@24  */
  YYSYMBOL_254_25 = 254,                   /* $@25  */
  YYSYMBOL_255_26 = 255,                   /* $@26  */
  YYSYMBOL_256_27 = 256,                   /* $@27  */
  YYSYMBOL_257_28 = 257,                   /* $@28  */
  YYSYMBOL_258_29 = 258,                   /* $@29  */
  YYSYMBOL_259_30 = 259,                   /* $@30  */
  YYSYMBOL_260_31 = 260,                   /* $@31  */
  YYSYMBOL_261_32 = 261,                   /* $@32  */
  YYSYMBOL_type = 262,                     /* type  */
  YYSYMBOL_atype = 263,                    /* atype  */
  YYSYMBOL_opt_exp_with_type = 264,        /* opt_exp_with_type  */
  YYSYMBOL_opt_exp_without_type = 265,     /* opt_exp_without_type  */
  YYSYMBOL_opt_nocrossrefs = 266,          /* opt_nocrossrefs  */
  YYSYMBOL_memspec_opt = 267,              /* memspec_opt  */
  YYSYMBOL_phdr_opt = 268,                 /* phdr_opt  */
  YYSYMBOL_overlay_section = 269,          /* overlay_section  */
  YYSYMBOL_270_33 = 270,                   /* $@33  */
  YYSYMBOL_271_34 = 271,                   /* $@34  */
  YYSYMBOL_272_35 = 272,                   /* $@35  */
  YYSYMBOL_phdrs = 273,                    /* phdrs  */
  YYSYMBOL_phdr_list = 274,                /* phdr_list  */
  YYSYMBOL_phdr = 275,                     /* phdr  */
  YYSYMBOL_276_36 = 276,                   /* $@36  */
  YYSYMBOL_277_37 = 277,                   /* $@37  */
  YYSYMBOL_phdr_type = 278,                /* phdr_type  */
  YYSYMBOL_phdr_qualifiers = 279,          /* phdr_qualifiers  */
  YYSYMBOL_phdr_val = 280,                 /* phdr_val  */
  YYSYMBOL_dynamic_list_file = 281,        /* dynamic_list_file  */
  YYSYMBOL_282_38 = 282,                   /* $@38  */
  YYSYMBOL_dynamic_list_nodes = 283,       /* dynamic_list_nodes  */
  YYSYMBOL_dynamic_list_node = 284,        /* dynamic_list_node  */
  YYSYMBOL_dynamic_list_tag = 285,         /* dynamic_list_tag  */
  YYSYMBOL_version_script_file = 286,      /* version_script_file  */
  YYSYMBOL_287_39 = 287,                   /* $@39  */
  YYSYMBOL_version = 288,                  /* version  */
  YYSYMBOL_289_40 = 289,                   /* $@40  */
  YYSYMBOL_vers_nodes = 290,               /* vers_nodes  */
  YYSYMBOL_vers_node = 291,                /* vers_node  */
  YYSYMBOL_verdep = 292,                   /* verdep  */
  YYSYMBOL_vers_tag = 293,                 /* vers_tag  */
  YYSYMBOL_vers_defns = 294,               /* vers_defns  */
  YYSYMBOL_295_41 = 295,                   /* @41  */
  YYSYMBOL_296_42 = 296,                   /* @42  */
  YYSYMBOL_opt_semicolon = 297,            /* opt_semicolon  */
  YYSYMBOL_section_ordering_script_file = 298, /* section_ordering_script_file  */
  YYSYMBOL_299_43 = 299,                   /* $@43  */
  YYSYMBOL_section_ordering_list = 300,    /* section_ordering_list  */
  YYSYMBOL_section_order = 301,            /* section_order  */
  YYSYMBOL_302_44 = 302,                   /* $@44  */
  YYSYMBOL_303_45 = 303                    /* $@45  */
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
#define YYFINAL  20
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1988

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  163
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  141
/* YYNRULES -- Number of rules.  */
#define YYNRULES  395
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  852

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   394


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
       2,     2,     2,   161,     2,     2,     2,    35,    22,     2,
      38,   158,    33,    31,   156,    32,     2,    34,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    17,   157,
      25,    10,    26,    16,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   159,     2,   160,    21,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    62,    20,    63,   162,     2,     2,     2,
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
     151,   152,   153,   154,   155
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   172,   172,   173,   174,   175,   176,   177,   181,   185,
     185,   192,   192,   205,   206,   210,   211,   212,   215,   218,
     219,   220,   222,   224,   226,   228,   230,   232,   234,   236,
     238,   240,   242,   243,   244,   246,   248,   250,   252,   254,
     255,   257,   256,   259,   261,   265,   266,   267,   271,   273,
     277,   279,   284,   285,   286,   290,   292,   294,   299,   299,
     305,   306,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   323,   325,   327,   330,   332,   334,   336,
     338,   340,   342,   341,   345,   348,   347,   350,   354,   358,
     358,   360,   362,   364,   366,   371,   371,   376,   379,   382,
     385,   388,   391,   395,   394,   400,   399,   405,   404,   412,
     416,   417,   418,   422,   424,   425,   425,   431,   438,   446,
     457,   458,   467,   468,   473,   479,   488,   489,   494,   499,
     504,   509,   514,   519,   524,   529,   535,   543,   561,   582,
     595,   604,   615,   624,   635,   644,   653,   657,   666,   670,
     678,   680,   679,   686,   687,   688,   692,   696,   701,   702,
     706,   710,   714,   719,   718,   726,   725,   733,   734,   737,
     739,   743,   745,   747,   749,   751,   756,   763,   765,   769,
     771,   773,   775,   777,   779,   781,   783,   785,   790,   790,
     795,   799,   807,   811,   815,   823,   823,   827,   830,   830,
     833,   834,   839,   838,   844,   843,   849,   856,   869,   870,
     874,   875,   879,   881,   886,   891,   892,   897,   899,   904,
     908,   910,   914,   916,   922,   925,   934,   945,   945,   949,
     949,   955,   957,   959,   961,   963,   965,   968,   970,   972,
     974,   976,   978,   980,   982,   984,   986,   988,   990,   992,
     994,   996,   998,  1000,  1002,  1004,  1006,  1008,  1010,  1013,
    1015,  1017,  1019,  1021,  1023,  1025,  1027,  1029,  1031,  1033,
    1035,  1036,  1035,  1045,  1047,  1049,  1051,  1053,  1055,  1057,
    1059,  1065,  1066,  1070,  1071,  1075,  1076,  1080,  1081,  1085,
    1086,  1090,  1091,  1092,  1093,  1097,  1104,  1113,  1115,  1096,
    1133,  1135,  1137,  1143,  1132,  1158,  1160,  1157,  1166,  1165,
    1173,  1174,  1175,  1176,  1177,  1178,  1179,  1180,  1184,  1185,
    1186,  1190,  1191,  1196,  1197,  1202,  1203,  1208,  1209,  1214,
    1216,  1221,  1224,  1236,  1240,  1247,  1249,  1238,  1261,  1264,
    1266,  1270,  1271,  1270,  1280,  1329,  1332,  1345,  1354,  1357,
    1364,  1364,  1376,  1377,  1381,  1385,  1394,  1394,  1408,  1408,
    1418,  1419,  1423,  1427,  1431,  1438,  1442,  1450,  1453,  1457,
    1461,  1465,  1472,  1476,  1480,  1484,  1489,  1488,  1502,  1501,
    1511,  1515,  1519,  1523,  1527,  1531,  1537,  1539,  1543,  1543,
    1555,  1556,  1557,  1561,  1569,  1560
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
  "INPUT_VERSION_SCRIPT", "INPUT_SECTION_ORDERING_SCRIPT", "KEEP",
  "ONLY_IF_RO", "ONLY_IF_RW", "SPECIAL", "INPUT_SECTION_FLAGS",
  "ALIGN_WITH_INPUT", "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST",
  "','", "';'", "')'", "'['", "']'", "'!'", "'~'", "$accept", "file",
  "filename", "defsym_expr", "$@1", "mri_script_file", "$@2",
  "mri_script_lines", "mri_script_command", "$@3", "ordernamelist",
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
  "@42", "opt_semicolon", "section_ordering_script_file", "$@43",
  "section_ordering_list", "section_order", "$@44", "$@45", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-782)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-359)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     289,  -782,  -782,  -782,  -782,  -782,  -782,    61,  -782,  -782,
    -782,  -782,  -782,    22,  -782,   -16,  -782,  -782,  -782,     8,
    -782,   246,  1704,   994,    50,    55,    63,  -782,   123,    11,
     -16,  -782,   365,   142,     8,  -782,    47,    57,    54,   103,
    -782,   112,  -782,  -782,   157,   195,   216,   238,   248,   259,
     260,   263,   264,   283,   290,  -782,  -782,   295,   296,   301,
    -782,   311,  -782,   316,  -782,  -782,  -782,  -782,   125,  -782,
    -782,  -782,  -782,  -782,  -782,  -782,   -19,  -782,   352,   157,
     353,   793,  -782,   357,   358,   361,  -782,  -782,   366,   368,
     369,   793,   370,   373,   377,   378,   383,   256,  -782,  -782,
    -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,   384,
     390,   392,  -782,   393,  -782,   385,   387,   338,   249,   123,
    -782,   891,  -782,  -782,  -782,  -782,   344,   252,  -782,  -782,
    -782,   407,   414,   415,   436,  -782,  -782,    32,   437,   438,
     439,   157,   157,   441,   157,     4,  -782,   443,   443,  -782,
     410,   157,   411,  -782,  -782,  -782,  -782,   388,    84,  -782,
      87,  -782,  -782,   793,   793,   793,   416,   417,   418,   427,
     428,  -782,  -782,   429,   430,  -782,  -782,  -782,  -782,   442,
     444,  -782,  -782,   446,   450,   451,   453,   793,   793,  1504,
     514,  -782,   320,  -782,   325,    25,  -782,  -782,   617,  1893,
     331,  -782,  -782,   336,  -782,    27,  -782,  -782,  -782,   793,
    -782,   483,   484,   485,   434,   142,   142,   340,   237,   440,
    -782,   343,   237,   435,    35,  -782,  -782,   -30,   346,  -782,
    -782,   157,   448,    -7,  -782,   349,   351,   354,   355,   364,
     367,   371,  -782,  -782,   116,   121,    77,   372,   374,   375,
      23,  -782,   376,   793,   378,   -16,   793,   793,  -782,   793,
     793,  -782,  -782,  1101,   793,   793,   793,   793,   793,   464,
     511,   793,  -782,   478,  -782,  -782,  -782,   793,   793,  -782,
    -782,   793,   793,   793,   515,  -782,  -782,   793,   793,   793,
     793,   793,   793,   793,   793,   793,   793,   793,   793,   793,
     793,   793,   793,   793,   793,   793,   793,   793,   793,  1893,
     522,   523,  -782,   527,   793,   793,  1893,   288,   533,  -782,
     538,  1893,  -782,  -782,  -782,  -782,   399,   400,  -782,  -782,
     546,  -782,  -782,  -782,   -66,   496,  -782,   994,  -782,   157,
    -782,  -782,  -782,  -782,  -782,  -782,  -782,   555,  -782,  -782,
     980,   526,  -782,  -782,  -782,    32,   561,  -782,  -782,  -782,
    -782,  -782,  -782,  -782,   157,  -782,   157,   443,  -782,  -782,
    -782,  -782,  -782,  -782,   528,    39,   413,  -782,  1524,    12,
     -28,  1893,  1893,  1729,  1893,  1893,  -782,   941,  1122,  1545,
    1565,  1142,   568,   419,  1162,   569,  1585,  1605,  1182,  1643,
    1202,   421,  1853,  1910,  1926,  1623,  1940,  1953,  1081,  1081,
     275,   275,   275,   275,   292,   292,   234,   234,  -782,  -782,
    -782,  1893,  1893,  1893,  -782,  -782,  -782,  1893,  1893,  -782,
    -782,  -782,  -782,   422,   423,   424,   142,   274,   237,   513,
    -782,  -782,   -55,   885,   657,  -782,   753,   657,   793,   420,
    -782,    21,   566,    32,  -782,   432,  -782,  -782,  -782,  -782,
    -782,  -782,   547,    33,  -782,   582,  -782,  -782,  -782,   793,
    -782,  -782,   793,   793,  -782,  -782,  -782,  -782,   452,   793,
     793,  -782,   584,  -782,  -782,   793,  -782,  -782,  -782,   454,
     575,  -782,  -782,  -782,  1012,  -782,  -782,  -782,  -782,  -782,
     589,  -782,   556,   557,   560,   157,   562,  -782,  -782,  -782,
     570,   571,   576,  -782,    19,  -782,  -782,  -782,   580,    83,
    -782,  -782,  -782,   885,   550,   585,   125,   382,   586,  1758,
     605,   516,  -782,  -782,  1873,   529,  -782,  1893,    29,   621,
    -782,   625,    17,  -782,   531,   593,  -782,    23,  -782,  -782,
    -782,   596,   477,  1222,  1242,  1263,   480,  -782,  1283,  1303,
     479,  1893,   237,   578,   142,   142,  -782,   255,    43,   107,
    -782,  -782,   600,  -782,   638,   639,  -782,   606,   608,   609,
     612,   613,  -782,  -782,   -97,    19,   614,   615,    19,   616,
    -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,   620,
     649,  -782,   504,   793,   -27,   646,  -782,   626,   627,   449,
    -782,  -782,   516,   603,   629,   631,  -782,   520,  -782,  -782,
    -782,   664,   524,  -782,    36,    23,  -782,  -782,  -782,  -782,
    -782,   793,  -782,  -782,  -782,  -782,   525,   454,   645,   532,
     534,   535,   648,   536,   885,   542,  -782,   793,    86,  -782,
      93,  -782,    16,    95,   242,    43,    43,   109,  -782,    19,
     131,    43,   -74,    19,   524,   544,   610,   793,  -782,  1323,
    -782,   793,   665,   563,  -782,   573,  -782,   793,    29,   793,
     559,  -782,  -782,   611,  -782,    91,    23,  1343,   237,   654,
      18,  -782,  -782,  -782,    18,  -782,   682,  -782,  1663,   564,
     565,   720,  -782,   639,  -782,   687,   688,   572,   690,   691,
     574,   577,   579,   693,   695,  -782,  -782,  -782,   192,  -782,
    -782,   724,  1893,    92,  1363,   793,  -782,   573,   700,  -782,
    1639,  1383,  -782,  1404,  -782,  -782,   730,  -782,  -782,   117,
    -782,  -782,   583,  -782,   747,  -782,  -782,  -782,    43,    43,
    -782,    43,    43,  -782,  -782,  -782,    18,    18,  -782,   793,
     714,   736,  -782,  1424,   188,   793,   692,  -782,  -782,    29,
    -782,  -782,   597,   601,   604,   607,   619,   622,   623,   628,
    1444,   793,  -782,  -782,  -782,  -782,  -782,  -782,  1464,  -782,
    -782,  -782,  -782,   125,   630,   632,   636,   637,   640,   641,
    -782,  1484,   699,  -782,  -782,  -782,  -782,  -782,  -782,  -782,
    -782,  -782,   725,   885,    53,   749,   701,  -782,   741,  -782,
    -782,   706,   765,   662,   741,   885,  -782,   748,  -782,   662,
     712,   783,    45,  -782,  -782,  -782,  -782,   785,  -782,    45,
    -782,  -782,  -782,   524,  -782,    45,  -782,   524,  -782,  -782,
     524,  -782
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,    58,    11,     9,   356,   388,   350,     0,     2,    61,
       3,    14,     7,     0,     4,     0,     5,   392,     6,     0,
       1,    59,    12,     0,     0,     0,     0,    10,   367,     0,
     357,   360,   389,     0,   351,   352,     0,     0,     0,     0,
      78,     0,    80,    79,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   222,   223,     0,     0,     0,
      82,     0,   115,     0,    71,    60,    63,    69,     0,    62,
      65,    66,    67,    68,    64,    70,     0,    17,     0,     0,
       0,     0,    18,     0,     0,     0,    20,    47,     0,     0,
       0,     0,     0,     0,    52,     0,     0,     0,   179,   180,
     181,   182,   229,   183,   184,   185,   186,   187,   229,     0,
       0,     0,   373,   384,   372,   380,   382,     0,     0,   367,
     361,     0,   391,   390,   380,   382,     0,     0,   353,   112,
     339,     0,     0,     0,     0,     8,    85,   199,     0,     0,
       0,     0,     0,     0,     0,     0,   221,   224,   224,    95,
       0,     0,     0,    89,   189,   188,   114,     0,     0,    41,
       0,   257,   274,     0,     0,     0,     0,     0,     0,     0,
       0,   258,   270,     0,     0,   227,   227,   227,   227,     0,
       0,   227,   227,     0,     0,     0,     0,     0,     0,    15,
       0,    50,    32,    48,    33,    19,    34,    24,     0,    37,
       0,    38,    53,    39,    55,    40,    43,    13,   190,     0,
     191,     0,     0,     0,     0,     0,     0,     0,   368,     0,
     393,     0,   355,     0,     0,    91,    92,     0,     0,    61,
     202,     0,     0,   196,   201,     0,     0,     0,     0,     0,
       0,     0,   216,   218,   196,   196,   224,     0,     0,     0,
       0,    95,     0,     0,     0,     0,     0,     0,    14,     0,
       0,   235,   231,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   260,     0,   259,   261,   262,     0,     0,   278,
     279,     0,     0,     0,     0,   234,   236,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    26,
       0,     0,    46,     0,     0,     0,    23,     0,     0,    56,
       0,   230,   229,   229,   229,   378,     0,     0,   362,   375,
     385,   374,   381,   383,     0,     0,   354,   295,   109,     0,
     300,   305,   111,   110,   341,   338,   340,     0,    75,    77,
     358,   208,   204,   197,   195,     0,     0,    94,    72,    73,
      84,   113,   214,   215,     0,   219,     0,   224,   225,    87,
      88,    81,    97,   100,     0,    96,     0,    74,     0,     0,
       0,    28,    29,    44,    30,    31,   232,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   255,   254,   252,   251,   250,   244,   245,
     248,   249,   246,   247,   242,   243,   240,   241,   237,   238,
     239,    16,    27,    25,    51,    49,    45,    21,    22,    36,
      35,    54,    57,     0,     0,     0,     0,   369,   370,     0,
     365,   363,     0,   169,   319,   308,     0,   319,     0,     0,
      86,     0,     0,   199,   200,     0,   217,   220,   226,   103,
      99,   102,     0,     0,    83,     0,    90,   359,    42,     0,
     265,   273,     0,     0,   269,   271,   256,   233,     0,     0,
       0,   264,     0,   280,   263,     0,   192,   193,   194,   386,
     383,   376,   366,   364,   144,   171,   172,   173,   174,   175,
       0,   162,     0,     0,     0,     0,     0,   155,   156,   163,
       0,     0,     0,   153,     0,   118,   120,   122,     0,     0,
     150,   158,   168,   170,     0,     0,     0,     0,     0,   319,
       0,   284,   112,   326,     0,   327,   306,   344,   345,     0,
     212,     0,     0,   210,     0,     0,    93,     0,   107,    98,
     101,     0,     0,     0,     0,     0,     0,   228,     0,     0,
       0,   253,   387,     0,     0,     0,   160,     0,     0,     0,
     165,   229,     0,   151,     0,     0,   117,     0,     0,     0,
       0,     0,   126,   143,   196,     0,   145,     0,     0,     0,
     167,   394,   229,   154,   310,   311,   312,   313,   314,   316,
       0,   320,     0,     0,     0,     0,   322,     0,   286,     0,
     325,   328,   284,     0,   348,     0,   342,     0,   213,   209,
     211,     0,   196,   205,     0,     0,   105,   116,   266,   267,
     268,     0,   275,   276,   277,   379,     0,   386,     0,     0,
       0,     0,     0,     0,   169,     0,   176,     0,     0,   137,
       0,   141,     0,     0,     0,     0,     0,     0,   146,     0,
     196,     0,   196,     0,   196,     0,     0,     0,   318,     0,
     321,     0,     0,   288,   309,   290,   112,     0,   345,     0,
       0,    76,   229,     0,   104,     0,     0,     0,   371,     0,
       0,   157,   123,   124,     0,   121,     0,   161,     0,   117,
       0,     0,   139,     0,   140,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   142,   148,   147,   196,   395,
     159,     0,   317,   319,     0,     0,   287,   290,     0,   301,
       0,     0,   346,     0,   343,   206,     0,   203,   108,     0,
     272,   377,     0,   166,     0,   152,   138,   119,     0,     0,
     127,     0,     0,   128,   129,   134,     0,     0,   149,     0,
       0,     0,   283,     0,   294,     0,     0,   307,   349,   345,
     229,   106,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   323,   285,   291,   292,   293,   296,     0,   302,
     347,   207,   125,     0,     0,     0,     0,     0,     0,     0,
     315,     0,     0,   289,   333,   164,   131,   130,   132,   133,
     135,   136,   319,   169,     0,     0,     0,   334,   330,   324,
     297,     0,     0,   282,   330,   169,   329,     0,   331,   282,
       0,     0,   178,   331,   335,   281,   229,     0,   303,   178,
     331,   177,   332,   196,   298,   178,   304,   196,   336,   299,
     196,   337
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -782,  -782,   -76,  -782,  -782,  -782,  -782,   543,  -782,  -782,
    -782,  -782,  -782,  -782,   548,  -782,  -782,   581,  -782,  -782,
    -782,  -782,   552,  -782,  -303,  -782,  -782,  -782,  -782,  -418,
     -15,  -782,  -305,  -557,  -496,   285,   146,  -782,  -782,  -782,
    -543,   158,  -782,  -782,   291,  -782,  -782,  -782,  -620,  -782,
     -23,  -781,  -782,  -522,   -13,  -231,  -782,   362,  -782,   461,
    -782,  -782,  -782,  -782,  -782,  -782,   278,  -782,  -782,  -782,
    -782,  -782,  -782,  -129,   250,  -782,   -93,  -782,   -80,  -782,
    -782,    -8,   210,  -782,  -782,    99,  -782,  -782,  -782,  -782,
    -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,  -782,
    -519,   380,  -782,  -782,     5,  -773,  -782,  -782,  -782,  -782,
    -782,  -782,  -782,  -782,  -782,  -782,  -646,  -782,  -782,  -782,
    -782,   794,  -782,  -782,  -782,  -782,  -782,   595,   -25,  -782,
     711,   -26,  -782,  -782,   197,  -782,  -782,  -782,  -782,  -782,
    -782
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     7,   136,    12,    13,    10,    11,    22,    97,   258,
     195,   194,   192,   203,   205,     8,     9,    21,    65,   150,
     229,   254,   249,   250,   375,   547,   686,   625,    66,   223,
     342,   152,   515,   516,   517,   518,   583,   650,   519,   652,
     584,   520,   521,   648,   522,   572,   644,   523,   524,   525,
     645,   838,   108,   156,    68,   659,    69,   232,   233,   234,
     351,   453,   622,   737,   452,   542,   543,    70,    71,   244,
      72,   245,    73,   247,   272,   273,   646,   209,   263,   269,
     556,   828,   608,   673,   727,   729,   787,   343,   444,   802,
     824,   847,   446,   766,   804,   843,   447,   613,   532,   602,
     530,   531,   535,   612,   823,   832,   814,   821,   840,   850,
      74,   224,   346,   448,   680,   538,   616,   678,    18,    19,
      34,    35,   126,    14,    15,    75,    76,    30,    31,   442,
     117,   118,   565,   436,   563,    16,    17,    32,   123,   335,
     664
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      27,   189,   355,   159,   593,   120,    67,   127,   135,   208,
     605,   199,   643,   364,   366,   210,   319,   122,   582,   248,
     576,   540,   576,   576,   696,   540,    23,   372,   373,   312,
    -198,   319,   732,   614,    28,   467,   230,   549,   550,   344,
     460,   461,   660,   460,   461,   662,    28,   576,   594,   595,
     596,   597,   598,   599,   600,   836,  -198,   817,   844,   354,
     839,    20,   837,   658,   848,   238,   239,   845,   241,   243,
      33,   640,   641,   119,   440,   252,   577,   578,   579,   580,
     581,   246,   354,   261,   262,   492,   717,   586,   109,   582,
     699,   441,   582,   110,   256,   460,   461,   259,   345,   576,
     643,   111,   493,   231,   638,   131,   132,   285,   286,   129,
     309,   576,    29,   576,   609,   701,   818,   368,   316,   130,
     718,   460,   461,   790,    29,   157,   347,   112,   348,   321,
     604,   601,   760,   643,    24,    25,    26,   742,   374,   615,
     587,   133,   503,   587,   504,   503,   112,   504,   551,   354,
     134,   462,   705,   706,   462,   352,   638,   707,   710,   711,
     712,   135,   242,   582,   642,   640,   713,   582,   320,   714,
     466,   512,   512,   378,   703,   619,   381,   382,   541,   384,
     385,   313,   541,   320,   387,   388,   389,   390,   391,   326,
     327,   394,   463,   816,   684,   463,   512,   396,   397,   778,
     779,   398,   399,   400,   761,   830,   462,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   433,
     434,   435,   462,   367,   427,   428,   512,   511,   458,   512,
     257,   329,   588,   260,   624,   514,   576,   463,   512,   738,
      23,   702,   774,   775,   138,   776,   777,   137,   730,   576,
     512,   113,   512,   445,   114,   115,   116,   303,   304,   305,
     651,   805,   354,   463,   363,   771,   139,   354,   329,   365,
     113,   154,   155,   114,   124,   125,   140,   354,   456,   716,
     457,   429,   430,   815,    36,    37,    38,   141,   142,   708,
     709,   143,   144,   638,   299,   300,   301,   302,   303,   304,
     305,    39,    40,    41,    42,    43,   638,    44,    45,    46,
      47,   145,   685,   301,   302,   303,   304,   305,   146,    48,
      49,    50,    51,   147,   148,    67,   784,   785,   786,   149,
      52,    53,    54,    55,    56,    57,    58,   704,   354,   151,
     758,    59,    60,    61,   153,   120,   158,   160,    24,    25,
      26,   190,   191,   639,   529,   193,   534,   529,   537,   121,
     196,    62,   197,   198,   200,   330,   201,   207,   331,   332,
     333,   202,   204,   739,    63,   161,   162,   206,   211,   553,
    -358,   683,   554,   555,   212,   512,   213,   214,   747,   558,
     559,   217,   215,    64,   216,   561,   218,   221,   512,   222,
     489,   225,   330,   163,   164,   331,   332,   490,   226,   227,
     165,   166,   167,     1,     2,     3,   274,   275,   276,   570,
     526,   279,   280,   719,     4,     5,   168,   169,   170,   337,
     228,   235,   236,   237,     6,   240,   171,   246,   251,   253,
     255,    51,   172,   337,   264,   265,   266,   594,   595,   596,
     597,   598,   599,   600,   173,   267,   268,   270,   271,   174,
     175,   176,   177,   178,   179,   180,   310,    24,    25,    26,
     277,   311,   278,   181,   281,   182,   674,   317,   282,   283,
      62,   284,   318,   322,   323,   324,   325,   328,   338,   665,
     336,   183,   392,   334,   349,   356,   339,   184,   185,   357,
     526,   353,   358,   359,   340,   393,   395,   161,   162,   401,
     339,    51,   360,   669,   307,   361,   424,   425,   340,   362,
     369,   426,   370,   371,   377,    51,   186,   431,   636,   637,
     601,   341,   432,   187,   188,   163,   164,    24,    25,    26,
     439,   687,   165,   166,   167,   341,   437,   438,   443,   449,
      62,    24,    25,    26,   451,   455,   459,   698,   168,   169,
     170,   464,   475,   478,    62,   491,   539,   476,   171,   484,
     486,   487,   488,   544,   172,   548,   552,   722,   560,   735,
     546,   724,   564,   566,   567,   568,   173,   731,   569,   733,
     571,   174,   175,   176,   177,   178,   179,   180,   573,   574,
     557,   562,   846,   591,   575,   181,   849,   182,   585,   851,
     161,   162,   606,   592,   603,   617,   607,   314,   611,   618,
     623,   526,   621,   183,   626,   627,   631,   634,   647,   184,
     185,   635,   649,   576,   653,   763,   654,   655,   163,   164,
     656,   657,  -117,   661,   663,   165,   166,   167,   666,   667,
     161,   162,   668,   670,   671,   676,   672,   677,   186,   679,
     308,   168,   169,   170,   682,   187,   188,   791,   681,   780,
     354,   171,   688,   690,   728,   788,   694,   172,   163,   164,
     691,   721,   692,   693,   695,   527,   166,   167,   528,   173,
     697,   801,   720,   725,   174,   175,   176,   177,   178,   179,
     180,   168,   169,   170,   736,   726,   734,   741,   181,   743,
     182,   171,  -144,   745,   746,   748,   749,   172,   751,   752,
     750,   756,   753,   757,   759,   754,   183,   755,   765,   173,
     770,   772,   184,   185,   174,   175,   176,   177,   178,   179,
     180,   773,   781,   782,   789,   792,   161,   162,   181,   793,
     182,   813,   794,   604,   820,   795,   819,   822,   825,   826,
     533,   186,   827,   315,   831,   834,   183,   796,   187,   188,
     797,   798,   184,   185,   163,   164,   799,   835,   806,   842,
     807,   165,   166,   167,   808,   809,   161,   162,   810,   811,
     526,   383,   379,   376,   589,   715,   700,   168,   169,   170,
     350,   186,   526,   841,   590,   545,   454,   171,   187,   188,
     620,   833,   675,   172,   163,   164,   764,   536,   128,   829,
     219,   165,   166,   167,   689,   173,     0,     0,     0,     0,
     174,   175,   176,   177,   178,   179,   180,   168,   169,   170,
     380,     0,     0,     0,   181,     0,   182,   171,     0,     0,
       0,     0,     0,   172,     0,     0,     0,     0,     0,     0,
       0,     0,   183,     0,     0,   173,     0,     0,   184,   185,
     174,   175,   176,   177,   178,   179,   180,     0,     0,   494,
       0,     0,     0,     0,   181,     0,   182,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   186,   220,     0,
       0,     0,   183,     0,   187,   188,     0,     0,   184,   185,
       0,     0,     0,     0,     0,     0,     0,   495,   496,   497,
     498,   499,   500,     0,     0,     0,     0,     0,   501,     0,
       0,     0,   502,     0,   503,     0,   504,   186,     0,     0,
       0,     0,     0,     0,   187,   188,   505,   287,     0,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,     0,     0,     0,
       0,     0,     0,     0,    23,     0,     0,   506,     0,   507,
       0,     0,     0,   508,     0,     0,     0,    24,    25,    26,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     509,     0,     0,     0,     0,     0,     0,   450,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,    36,    37,
      38,     0,   510,     0,     0,     0,   511,     0,   512,     0,
       0,     0,   513,     0,   514,    39,    40,    41,    42,    43,
    -117,    44,    45,    46,    47,     0,     0,     0,     0,     0,
       0,     0,     0,    48,    49,    50,    51,     0,     0,     0,
       0,     0,     0,     0,    52,    53,    54,    55,    56,    57,
      58,     0,     0,     0,     0,    59,    60,    61,     0,     0,
       0,     0,    24,    25,    26,     0,     0,   469,     0,   470,
       0,     0,     0,     0,     0,    62,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   287,    63,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,    64,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,     0,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,   287,   386,
     288,   289,   290,   291,   292,   293,   294,   295,   296,   297,
     298,   299,   300,   301,   302,   303,   304,   305,     0,   287,
     471,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     474,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     477,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     481,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     483,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     628,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     629,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,     0,
     287,   630,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   632,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   633,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   723,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   740,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   762,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     287,   768,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
       0,   287,   769,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   287,   783,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   287,   800,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   287,   803,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,     0,   812,   337,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     306,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     465,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,     0,
       0,   472,   767,     0,     0,     0,     0,     0,    77,     0,
     339,     0,     0,     0,     0,     0,     0,     0,   340,     0,
       0,   473,     0,     0,     0,    51,     0,     0,     0,     0,
       0,     0,     0,    77,     0,     0,     0,     0,     0,     0,
       0,   479,     0,    78,     0,   341,     0,     0,     0,     0,
       0,    24,    25,    26,     0,     0,     0,     0,     0,     0,
       0,   480,     0,     0,    62,     0,   468,     0,    78,     0,
       0,     0,     0,     0,   287,    79,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,     0,     0,   604,     0,     0,   482,
      79,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    80,     0,     0,     0,     0,     0,   744,
      81,    82,    83,    84,    85,   -44,    86,    87,    88,     0,
       0,    89,    90,     0,    91,    92,    93,     0,    80,     0,
       0,    94,    95,    96,     0,    81,    82,    83,    84,    85,
       0,    86,    87,    88,     0,     0,    89,    90,     0,    91,
      92,    93,     0,     0,     0,     0,    94,    95,    96,   287,
     485,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
     610,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   287,
       0,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,   300,   301,   302,   303,
     304,   305,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305
};

static const yytype_int16 yycheck[] =
{
      13,    81,   233,    79,   526,    30,    21,    33,     4,   102,
     529,    91,   569,   244,   245,   108,     4,    32,   514,   148,
       4,     4,     4,     4,   644,     4,     4,     4,     5,     4,
      37,     4,   678,     4,    62,    63,     4,     4,     5,     4,
       4,     5,   585,     4,     5,   588,    62,     4,    75,    76,
      77,    78,    79,    80,    81,    10,    63,     4,   839,   156,
     833,     0,    17,   160,   845,   141,   142,   840,   144,   145,
      62,   567,   568,    62,   140,   151,    57,    58,    59,    60,
      61,     4,   156,   163,   164,   140,   160,     4,    38,   585,
       4,   157,   588,    38,    10,     4,     5,    10,    63,     4,
     657,    38,   157,    71,    61,    51,    52,   187,   188,    62,
     190,     4,   140,     4,   532,    22,    63,   246,   198,    62,
     663,     4,     5,   769,   140,   144,   156,     4,   158,   209,
      38,   158,    40,   690,   112,   113,   114,   694,   115,   110,
      57,    38,    59,    57,    61,    59,     4,    61,   115,   156,
      38,   115,    57,    58,   115,   231,    61,   653,   654,   655,
     656,     4,   158,   659,    57,   661,    57,   663,   156,    60,
     158,   153,   153,   253,   158,   158,   256,   257,   161,   259,
     260,   156,   161,   156,   264,   265,   266,   267,   268,   215,
     216,   271,   156,   813,   158,   156,   153,   277,   278,   756,
     757,   281,   282,   283,   723,   825,   115,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   322,
     323,   324,   115,   156,   314,   315,   153,   151,   367,   153,
     156,     4,   159,   156,   547,   159,     4,   156,   153,   158,
       4,   158,   748,   749,    38,   751,   752,    62,   676,     4,
     153,   138,   153,   339,   141,   142,   143,    33,    34,    35,
     575,   793,   156,   156,   158,   158,    38,   156,     4,   158,
     138,   156,   157,   141,   142,   143,    38,   156,   364,   158,
     366,     3,     4,   812,    48,    49,    50,    38,    38,    57,
      58,    38,    38,    61,    29,    30,    31,    32,    33,    34,
      35,    65,    66,    67,    68,    69,    61,    71,    72,    73,
      74,    38,   625,    31,    32,    33,    34,    35,    38,    83,
      84,    85,    86,    38,    38,   350,   148,   149,   150,    38,
      94,    95,    96,    97,    98,    99,   100,   652,   156,    38,
     158,   105,   106,   107,    38,   380,     4,     4,   112,   113,
     114,     4,     4,   108,   444,     4,   446,   447,   448,     4,
       4,   125,     4,     4,     4,   138,     3,   121,   141,   142,
     143,     4,     4,   686,   138,     3,     4,     4,     4,   469,
     144,   622,   472,   473,     4,   153,     4,     4,   703,   479,
     480,    63,    17,   157,    17,   485,   157,    63,   153,   157,
     436,     4,   138,    31,    32,   141,   142,   143,     4,     4,
      38,    39,    40,   134,   135,   136,   176,   177,   178,   505,
     443,   181,   182,   664,   145,   146,    54,    55,    56,     4,
       4,     4,     4,     4,   155,     4,    64,     4,    38,    38,
      62,    86,    70,     4,    38,    38,    38,    75,    76,    77,
      78,    79,    80,    81,    82,    38,    38,    38,    38,    87,
      88,    89,    90,    91,    92,    93,   156,   112,   113,   114,
      38,   156,    38,   101,    38,   103,    37,   156,    38,    38,
     125,    38,   156,    10,    10,    10,    62,   157,    63,   592,
     157,   119,    38,    63,   158,   156,    71,   125,   126,   158,
     523,    63,   158,   158,    79,     4,    38,     3,     4,     4,
      71,    86,   158,   603,    10,   158,     4,     4,    79,   158,
     158,     4,   158,   158,   158,    86,   154,     4,   564,   565,
     158,   106,     4,   161,   162,    31,    32,   112,   113,   114,
       4,   631,    38,    39,    40,   106,   157,   157,    62,     4,
     125,   112,   113,   114,    38,     4,    38,   647,    54,    55,
      56,   158,     4,     4,   125,    62,   156,   158,    64,   158,
     158,   158,   158,    17,    70,    38,     4,   667,     4,   682,
     158,   671,    17,     4,    38,    38,    82,   677,    38,   679,
      38,    87,    88,    89,    90,    91,    92,    93,    38,    38,
     158,   157,   843,    63,    38,   101,   847,   103,    38,   850,
       3,     4,    17,    38,    38,     4,   110,    10,    99,     4,
      37,   644,   101,   119,    38,   158,   156,   158,    38,   125,
     126,    63,     4,     4,    38,   725,    38,    38,    31,    32,
      38,    38,    38,    38,    38,    38,    39,    40,    38,    10,
       3,     4,   158,    17,    38,    62,    39,    38,   154,    38,
     156,    54,    55,    56,    10,   161,   162,   770,   158,   759,
     156,    64,   157,    38,   111,   765,    38,    70,    31,    32,
     158,    81,   158,   158,   158,    38,    39,    40,    41,    82,
     158,   781,   158,    38,    87,    88,    89,    90,    91,    92,
      93,    54,    55,    56,   103,   152,   157,    63,   101,    37,
     103,    64,   158,   158,     4,    38,    38,    70,    38,    38,
     158,    38,   158,    38,    10,   158,   119,   158,    38,    82,
      10,   158,   125,   126,    87,    88,    89,    90,    91,    92,
      93,     4,    38,    17,    62,   158,     3,     4,   101,   158,
     103,    62,   158,    38,    63,   158,    17,    26,    62,     4,
      17,   154,   110,   156,    26,    63,   119,   158,   161,   162,
     158,   158,   125,   126,    31,    32,   158,     4,   158,     4,
     158,    38,    39,    40,   158,   158,     3,     4,   158,   158,
     813,   258,   254,   251,   519,   659,   648,    54,    55,    56,
     229,   154,   825,   836,   523,   453,   355,    64,   161,   162,
     542,   829,   612,    70,    31,    32,   727,   447,    34,   824,
     119,    38,    39,    40,   637,    82,    -1,    -1,    -1,    -1,
      87,    88,    89,    90,    91,    92,    93,    54,    55,    56,
     255,    -1,    -1,    -1,   101,    -1,   103,    64,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   119,    -1,    -1,    82,    -1,    -1,   125,   126,
      87,    88,    89,    90,    91,    92,    93,    -1,    -1,     4,
      -1,    -1,    -1,    -1,   101,    -1,   103,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,   154,    17,    -1,
      -1,    -1,   119,    -1,   161,   162,    -1,    -1,   125,   126,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    53,    -1,
      -1,    -1,    57,    -1,    59,    -1,    61,   154,    -1,    -1,
      -1,    -1,    -1,    -1,   161,   162,    71,    16,    -1,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     4,    -1,    -1,   102,    -1,   104,
      -1,    -1,    -1,   108,    -1,    -1,    -1,   112,   113,   114,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
     125,    -1,    -1,    -1,    -1,    -1,    -1,    37,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    48,    49,
      50,    -1,   147,    -1,    -1,    -1,   151,    -1,   153,    -1,
      -1,    -1,   157,    -1,   159,    65,    66,    67,    68,    69,
      38,    71,    72,    73,    74,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    83,    84,    85,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    94,    95,    96,    97,    98,    99,
     100,    -1,    -1,    -1,    -1,   105,   106,   107,    -1,    -1,
      -1,    -1,   112,   113,   114,    -1,    -1,   156,    -1,   158,
      -1,    -1,    -1,    -1,    -1,   125,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    16,   138,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,   157,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,    -1,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    16,   158,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    -1,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     158,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    -1,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      16,   158,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      -1,    16,   158,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    16,   158,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    16,   158,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    16,   158,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    -1,   158,     4,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     156,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
     156,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    -1,
      -1,   156,    63,    -1,    -1,    -1,    -1,    -1,     4,    -1,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,   156,    -1,    -1,    -1,    86,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   156,    -1,    39,    -1,   106,    -1,    -1,    -1,    -1,
      -1,   112,   113,   114,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   156,    -1,    -1,   125,    -1,    37,    -1,    39,    -1,
      -1,    -1,    -1,    -1,    16,    71,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    -1,    -1,    38,    -1,    -1,   156,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   109,    -1,    -1,    -1,    -1,    -1,   156,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,   127,   128,    -1,   130,   131,   132,    -1,   109,    -1,
      -1,   137,   138,   139,    -1,   116,   117,   118,   119,   120,
      -1,   122,   123,   124,    -1,    -1,   127,   128,    -1,   130,
     131,   132,    -1,    -1,    -1,    -1,   137,   138,   139,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    16,
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   134,   135,   136,   145,   146,   155,   164,   178,   179,
     168,   169,   166,   167,   286,   287,   298,   299,   281,   282,
       0,   180,   170,     4,   112,   113,   114,   217,    62,   140,
     290,   291,   300,    62,   283,   284,    48,    49,    50,    65,
      66,    67,    68,    69,    71,    72,    73,    74,    83,    84,
      85,    86,    94,    95,    96,    97,    98,    99,   100,   105,
     106,   107,   125,   138,   157,   181,   191,   193,   217,   219,
     230,   231,   233,   235,   273,   288,   289,     4,    39,    71,
     109,   116,   117,   118,   119,   120,   122,   123,   124,   127,
     128,   130,   131,   132,   137,   138,   139,   171,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,   215,    38,
      38,    38,     4,   138,   141,   142,   143,   293,   294,    62,
     291,     4,   193,   301,   142,   143,   285,   294,   284,    62,
      62,    51,    52,    38,    38,     4,   165,    62,    38,    38,
      38,    38,    38,    38,    38,    38,    38,    38,    38,    38,
     182,    38,   194,    38,   156,   157,   216,   144,     4,   165,
       4,     3,     4,    31,    32,    38,    39,    40,    54,    55,
      56,    64,    70,    82,    87,    88,    89,    90,    91,    92,
      93,   101,   103,   119,   125,   126,   154,   161,   162,   241,
       4,     4,   175,     4,   174,   173,     4,     4,     4,   241,
       4,     3,     4,   176,     4,   177,     4,   121,   239,   240,
     239,     4,     4,     4,     4,    17,    17,    63,   157,   293,
      17,    63,   157,   192,   274,     4,     4,     4,     4,   183,
       4,    71,   220,   221,   222,     4,     4,     4,   165,   165,
       4,   165,   158,   165,   232,   234,     4,   236,   236,   185,
     186,    38,   165,    38,   184,    62,    10,   156,   172,    10,
     156,   241,   241,   241,    38,    38,    38,    38,    38,   242,
      38,    38,   237,   238,   237,   237,   237,    38,    38,   237,
     237,    38,    38,    38,    38,   241,   241,    16,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,   156,    10,   156,   241,
     156,   156,     4,   156,    10,   156,   241,   156,   156,     4,
     156,   241,    10,    10,    10,    62,   294,   294,   157,     4,
     138,   141,   142,   143,    63,   302,   157,     4,    63,    71,
      79,   106,   193,   250,     4,    63,   275,   156,   158,   158,
     180,   223,   165,    63,   156,   218,   156,   158,   158,   158,
     158,   158,   158,   158,   218,   158,   218,   156,   236,   158,
     158,   158,     4,     5,   115,   187,   185,   158,   241,   177,
     290,   241,   241,   170,   241,   241,   158,   241,   241,   241,
     241,   241,    38,     4,   241,    38,   241,   241,   241,   241,
     241,     4,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   241,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   241,   241,   241,     4,     4,     4,   241,   241,     3,
       4,     4,     4,   239,   239,   239,   296,   157,   157,     4,
     140,   157,   292,    62,   251,   165,   255,   259,   276,     4,
      37,    38,   227,   224,   222,     4,   165,   165,   236,    38,
       4,     5,   115,   156,   158,   156,   158,    63,    37,   156,
     158,   158,   156,   156,   158,     4,   158,   158,     4,   156,
     156,   158,   156,   158,   158,    17,   158,   158,   158,   294,
     143,    62,   140,   157,     4,    42,    43,    44,    45,    46,
      47,    53,    57,    59,    61,    71,   102,   104,   108,   125,
     147,   151,   153,   157,   159,   195,   196,   197,   198,   201,
     204,   205,   207,   210,   211,   212,   217,    38,    41,   241,
     263,   264,   261,    17,   241,   265,   264,   241,   278,   156,
       4,   161,   228,   229,    17,   220,   158,   188,    38,     4,
       5,   115,     4,   241,   241,   241,   243,   158,   241,   241,
       4,   241,   157,   297,    17,   295,     4,    38,    38,    38,
     165,    38,   208,    38,    38,    38,     4,    57,    58,    59,
      60,    61,   197,   199,   203,    38,     4,    57,   159,   198,
     207,    63,    38,   216,    75,    76,    77,    78,    79,    80,
      81,   158,   262,    38,    38,   263,    17,   110,   245,   192,
      17,    99,   266,   260,     4,   110,   279,     4,     4,   158,
     229,   101,   225,    37,   187,   190,    38,   158,   158,   158,
     158,   156,   158,   158,   158,    63,   294,   294,    61,   108,
     197,   197,    57,   196,   209,   213,   239,    38,   206,     4,
     200,   195,   202,    38,    38,    38,    38,    38,   160,   218,
     203,    38,   203,    38,   303,   239,    38,    10,   158,   241,
      17,    38,    39,   246,    37,   245,    62,    38,   280,    38,
     277,   158,    10,   218,   158,   187,   189,   241,   157,   297,
      38,   158,   158,   158,    38,   158,   211,   158,   241,     4,
     204,    22,   158,   158,   195,    57,    58,   197,    57,    58,
     197,   197,   197,    57,    60,   199,   158,   160,   203,   218,
     158,    81,   241,   158,   241,    38,   152,   247,   111,   248,
     192,   241,   279,   241,   157,   239,   103,   226,   158,   187,
     158,    63,   196,    37,   156,   158,     4,   195,    38,    38,
     158,    38,    38,   158,   158,   158,    38,    38,   158,    10,
      40,   263,   158,   241,   248,    38,   256,    63,   158,   158,
      10,   158,   158,     4,   197,   197,   197,   197,   196,   196,
     241,    38,    17,   158,   148,   149,   150,   249,   241,    62,
     279,   239,   158,   158,   158,   158,   158,   158,   158,   158,
     158,   241,   252,   158,   257,   216,   158,   158,   158,   158,
     158,   158,   158,    62,   269,   263,   211,     4,    63,    17,
      63,   270,    26,   267,   253,    62,     4,   110,   244,   267,
     211,    26,   268,   244,    63,     4,    10,    17,   214,   268,
     271,   213,     4,   258,   214,   268,   218,   254,   214,   218,
     272,   218
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   163,   164,   164,   164,   164,   164,   164,   165,   167,
     166,   169,   168,   170,   170,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   172,   171,   171,   171,   173,   173,   173,   174,   174,
     175,   175,   176,   176,   176,   177,   177,   177,   179,   178,
     180,   180,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   182,   181,   181,   183,   181,   181,   181,   184,
     181,   181,   181,   181,   181,   186,   185,   187,   187,   187,
     187,   187,   187,   188,   187,   189,   187,   190,   187,   191,
     192,   192,   192,   193,   193,   194,   193,   195,   196,   196,
     197,   197,   198,   198,   198,   198,   199,   199,   199,   199,
     199,   199,   199,   199,   199,   199,   199,   200,   200,   201,
     202,   202,   203,   203,   204,   204,   204,   204,   204,   204,
     205,   206,   205,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   208,   207,   209,   207,   210,   210,   211,
     211,   212,   212,   212,   212,   212,   213,   214,   214,   215,
     215,   215,   215,   215,   215,   215,   215,   215,   216,   216,
     217,   217,   217,   217,   217,   218,   218,   219,   220,   220,
     221,   221,   223,   222,   224,   222,   225,   226,   227,   227,
     228,   228,   229,   229,   230,   231,   231,   232,   232,   233,
     234,   234,   235,   235,   236,   236,   236,   238,   237,   240,
     239,   241,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   241,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   241,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   241,   241,   241,   241,   241,   241,   241,   241,   241,
     242,   243,   241,   241,   241,   241,   241,   241,   241,   241,
     241,   244,   244,   245,   245,   246,   246,   247,   247,   248,
     248,   249,   249,   249,   249,   251,   252,   253,   254,   250,
     255,   256,   257,   258,   250,   259,   260,   250,   261,   250,
     262,   262,   262,   262,   262,   262,   262,   262,   263,   263,
     263,   264,   264,   264,   264,   265,   265,   266,   266,   267,
     267,   268,   268,   269,   270,   271,   272,   269,   273,   274,
     274,   276,   277,   275,   278,   279,   279,   279,   280,   280,
     282,   281,   283,   283,   284,   285,   287,   286,   289,   288,
     290,   290,   291,   291,   291,   292,   292,   293,   293,   293,
     293,   293,   294,   294,   294,   294,   295,   294,   296,   294,
     294,   294,   294,   294,   294,   294,   297,   297,   299,   298,
     300,   300,   300,   302,   303,   301
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     1,     0,
       2,     0,     2,     3,     0,     2,     4,     1,     1,     2,
       1,     4,     4,     3,     2,     4,     3,     4,     4,     4,
       4,     4,     2,     2,     2,     4,     4,     2,     2,     2,
       2,     0,     5,     2,     0,     3,     2,     0,     1,     3,
       1,     3,     0,     1,     3,     1,     2,     3,     0,     2,
       2,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     4,     4,     4,     4,     8,     4,     1,     1,
       1,     4,     0,     5,     4,     0,     5,     4,     4,     0,
       5,     3,     3,     6,     4,     0,     2,     1,     3,     2,
       1,     3,     2,     0,     5,     0,     7,     0,     6,     4,
       2,     2,     0,     4,     2,     0,     7,     1,     1,     5,
       1,     4,     1,     4,     4,     7,     1,     4,     4,     4,
       7,     7,     7,     7,     4,     7,     7,     1,     3,     4,
       2,     1,     3,     1,     1,     2,     3,     4,     4,     5,
       1,     0,     5,     1,     2,     1,     1,     4,     1,     4,
       2,     4,     1,     0,     8,     0,     5,     2,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     6,     6,     6,     1,     0,     4,     1,     0,
       3,     1,     0,     7,     0,     5,     3,     3,     0,     3,
       1,     2,     1,     2,     4,     4,     3,     3,     1,     4,
       3,     0,     1,     1,     0,     2,     3,     0,     4,     0,
       2,     2,     3,     4,     2,     2,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     3,     3,     4,     1,     1,     2,
       2,     2,     2,     4,     4,     4,     6,     6,     6,     4,
       0,     0,     8,     4,     1,     6,     6,     6,     2,     2,
       4,     3,     0,     4,     0,     4,     0,     1,     0,     4,
       0,     1,     1,     1,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,    17,     0,     0,     7,     0,     5,
       1,     1,     1,     1,     1,     6,     1,     3,     3,     0,
       2,     3,     2,     6,    10,     2,     1,     0,     1,     2,
       0,     0,     3,     0,     0,     0,     0,    11,     4,     0,
       2,     0,     0,     6,     1,     0,     3,     5,     0,     3,
       0,     2,     1,     2,     4,     2,     0,     2,     0,     5,
       1,     2,     4,     5,     6,     1,     2,     0,     2,     4,
       4,     8,     1,     1,     3,     3,     0,     9,     0,     7,
       1,     3,     1,     3,     1,     3,     0,     1,     0,     2,
       2,     2,     0,     0,     0,     8
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
  case 9: /* $@1: %empty  */
#line 185 "ldgram.y"
                { ldlex_expression(); }
#line 2611 "ldgram.c"
    break;

  case 10: /* defsym_expr: $@1 assignment  */
#line 187 "ldgram.y"
                { ldlex_popstate(); }
#line 2617 "ldgram.c"
    break;

  case 11: /* $@2: %empty  */
#line 192 "ldgram.y"
                {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2626 "ldgram.c"
    break;

  case 12: /* mri_script_file: $@2 mri_script_lines  */
#line 197 "ldgram.y"
                {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2636 "ldgram.c"
    break;

  case 17: /* mri_script_command: NAME  */
#line 212 "ldgram.y"
                        {
			fatal (_("%P: unrecognised keyword in MRI style script '%s'\n"), (yyvsp[0].name));
			}
#line 2644 "ldgram.c"
    break;

  case 18: /* mri_script_command: LIST  */
#line 215 "ldgram.y"
                        {
			config.map_filename = "-";
			}
#line 2652 "ldgram.c"
    break;

  case 21: /* mri_script_command: PUBLIC NAME '=' exp  */
#line 221 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2658 "ldgram.c"
    break;

  case 22: /* mri_script_command: PUBLIC NAME ',' exp  */
#line 223 "ldgram.y"
                        { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2664 "ldgram.c"
    break;

  case 23: /* mri_script_command: PUBLIC NAME exp  */
#line 225 "ldgram.y"
                        { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2670 "ldgram.c"
    break;

  case 24: /* mri_script_command: FORMAT NAME  */
#line 227 "ldgram.y"
                        { mri_format((yyvsp[0].name)); }
#line 2676 "ldgram.c"
    break;

  case 25: /* mri_script_command: SECT NAME ',' exp  */
#line 229 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2682 "ldgram.c"
    break;

  case 26: /* mri_script_command: SECT NAME exp  */
#line 231 "ldgram.y"
                        { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2688 "ldgram.c"
    break;

  case 27: /* mri_script_command: SECT NAME '=' exp  */
#line 233 "ldgram.y"
                        { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2694 "ldgram.c"
    break;

  case 28: /* mri_script_command: ALIGN_K NAME '=' exp  */
#line 235 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2700 "ldgram.c"
    break;

  case 29: /* mri_script_command: ALIGN_K NAME ',' exp  */
#line 237 "ldgram.y"
                        { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2706 "ldgram.c"
    break;

  case 30: /* mri_script_command: ALIGNMOD NAME '=' exp  */
#line 239 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2712 "ldgram.c"
    break;

  case 31: /* mri_script_command: ALIGNMOD NAME ',' exp  */
#line 241 "ldgram.y"
                        { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2718 "ldgram.c"
    break;

  case 34: /* mri_script_command: NAMEWORD NAME  */
#line 245 "ldgram.y"
                        { mri_name((yyvsp[0].name)); }
#line 2724 "ldgram.c"
    break;

  case 35: /* mri_script_command: ALIAS NAME ',' NAME  */
#line 247 "ldgram.y"
                        { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2730 "ldgram.c"
    break;

  case 36: /* mri_script_command: ALIAS NAME ',' INT  */
#line 249 "ldgram.y"
                        { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2736 "ldgram.c"
    break;

  case 37: /* mri_script_command: BASE exp  */
#line 251 "ldgram.y"
                        { mri_base((yyvsp[0].etree)); }
#line 2742 "ldgram.c"
    break;

  case 38: /* mri_script_command: TRUNCATE INT  */
#line 253 "ldgram.y"
                { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2748 "ldgram.c"
    break;

  case 41: /* $@3: %empty  */
#line 257 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2754 "ldgram.c"
    break;

  case 43: /* mri_script_command: START NAME  */
#line 260 "ldgram.y"
                { lang_add_entry ((yyvsp[0].name), false); }
#line 2760 "ldgram.c"
    break;

  case 45: /* ordernamelist: ordernamelist ',' NAME  */
#line 265 "ldgram.y"
                                             { mri_order((yyvsp[0].name)); }
#line 2766 "ldgram.c"
    break;

  case 46: /* ordernamelist: ordernamelist NAME  */
#line 266 "ldgram.y"
                                          { mri_order((yyvsp[0].name)); }
#line 2772 "ldgram.c"
    break;

  case 48: /* mri_load_name_list: NAME  */
#line 272 "ldgram.y"
                        { mri_load((yyvsp[0].name)); }
#line 2778 "ldgram.c"
    break;

  case 49: /* mri_load_name_list: mri_load_name_list ',' NAME  */
#line 273 "ldgram.y"
                                            { mri_load((yyvsp[0].name)); }
#line 2784 "ldgram.c"
    break;

  case 50: /* mri_abs_name_list: NAME  */
#line 278 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2790 "ldgram.c"
    break;

  case 51: /* mri_abs_name_list: mri_abs_name_list ',' NAME  */
#line 280 "ldgram.y"
                        { mri_only_load((yyvsp[0].name)); }
#line 2796 "ldgram.c"
    break;

  case 52: /* casesymlist: %empty  */
#line 284 "ldgram.y"
                      { (yyval.name) = NULL; }
#line 2802 "ldgram.c"
    break;

  case 55: /* extern_name_list: NAME  */
#line 291 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2808 "ldgram.c"
    break;

  case 56: /* extern_name_list: extern_name_list NAME  */
#line 293 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2814 "ldgram.c"
    break;

  case 57: /* extern_name_list: extern_name_list ',' NAME  */
#line 295 "ldgram.y"
                        { ldlang_add_undef ((yyvsp[0].name), false); }
#line 2820 "ldgram.c"
    break;

  case 58: /* $@4: %empty  */
#line 299 "ldgram.y"
        { ldlex_script (); }
#line 2826 "ldgram.c"
    break;

  case 59: /* script_file: $@4 ifile_list  */
#line 301 "ldgram.y"
        { ldlex_popstate (); }
#line 2832 "ldgram.c"
    break;

  case 72: /* ifile_p1: TARGET_K '(' NAME ')'  */
#line 322 "ldgram.y"
                { lang_add_target((yyvsp[-1].name)); }
#line 2838 "ldgram.c"
    break;

  case 73: /* ifile_p1: SEARCH_DIR '(' filename ')'  */
#line 324 "ldgram.y"
                { ldfile_add_library_path ((yyvsp[-1].name), false); }
#line 2844 "ldgram.c"
    break;

  case 74: /* ifile_p1: OUTPUT '(' filename ')'  */
#line 326 "ldgram.y"
                { lang_add_output((yyvsp[-1].name), 1); }
#line 2850 "ldgram.c"
    break;

  case 75: /* ifile_p1: OUTPUT_FORMAT '(' NAME ')'  */
#line 328 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2857 "ldgram.c"
    break;

  case 76: /* ifile_p1: OUTPUT_FORMAT '(' NAME ',' NAME ',' NAME ')'  */
#line 331 "ldgram.y"
                  { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2863 "ldgram.c"
    break;

  case 77: /* ifile_p1: OUTPUT_ARCH '(' NAME ')'  */
#line 333 "ldgram.y"
                  { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2869 "ldgram.c"
    break;

  case 78: /* ifile_p1: FORCE_COMMON_ALLOCATION  */
#line 335 "ldgram.y"
                { command_line.force_common_definition = true ; }
#line 2875 "ldgram.c"
    break;

  case 79: /* ifile_p1: FORCE_GROUP_ALLOCATION  */
#line 337 "ldgram.y"
                { command_line.force_group_allocation = true ; }
#line 2881 "ldgram.c"
    break;

  case 80: /* ifile_p1: INHIBIT_COMMON_ALLOCATION  */
#line 339 "ldgram.y"
                { link_info.inhibit_common_definition = true ; }
#line 2887 "ldgram.c"
    break;

  case 82: /* $@5: %empty  */
#line 342 "ldgram.y"
                  { lang_enter_group (); }
#line 2893 "ldgram.c"
    break;

  case 83: /* ifile_p1: GROUP $@5 '(' input_list ')'  */
#line 344 "ldgram.y"
                  { lang_leave_group (); }
#line 2899 "ldgram.c"
    break;

  case 84: /* ifile_p1: MAP '(' filename ')'  */
#line 346 "ldgram.y"
                { lang_add_map((yyvsp[-1].name)); }
#line 2905 "ldgram.c"
    break;

  case 85: /* $@6: %empty  */
#line 348 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 2911 "ldgram.c"
    break;

  case 87: /* ifile_p1: NOCROSSREFS '(' nocrossref_list ')'  */
#line 351 "ldgram.y"
                {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2919 "ldgram.c"
    break;

  case 88: /* ifile_p1: NOCROSSREFS_TO '(' nocrossref_list ')'  */
#line 355 "ldgram.y"
                {
		  lang_add_nocrossref_to ((yyvsp[-1].nocrossref));
		}
#line 2927 "ldgram.c"
    break;

  case 89: /* $@7: %empty  */
#line 358 "ldgram.y"
                           { ldlex_expression (); }
#line 2933 "ldgram.c"
    break;

  case 90: /* ifile_p1: EXTERN '(' $@7 extern_name_list ')'  */
#line 359 "ldgram.y"
                        { ldlex_popstate (); }
#line 2939 "ldgram.c"
    break;

  case 91: /* ifile_p1: INSERT_K AFTER NAME  */
#line 361 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 0); }
#line 2945 "ldgram.c"
    break;

  case 92: /* ifile_p1: INSERT_K BEFORE NAME  */
#line 363 "ldgram.y"
                { lang_add_insert ((yyvsp[0].name), 1); }
#line 2951 "ldgram.c"
    break;

  case 93: /* ifile_p1: REGION_ALIAS '(' NAME ',' NAME ')'  */
#line 365 "ldgram.y"
                { lang_memory_region_alias ((yyvsp[-3].name), (yyvsp[-1].name)); }
#line 2957 "ldgram.c"
    break;

  case 94: /* ifile_p1: LD_FEATURE '(' NAME ')'  */
#line 367 "ldgram.y"
                { lang_ld_feature ((yyvsp[-1].name)); }
#line 2963 "ldgram.c"
    break;

  case 95: /* $@8: %empty  */
#line 371 "ldgram.y"
                { ldlex_inputlist(); }
#line 2969 "ldgram.c"
    break;

  case 96: /* input_list: $@8 input_list1  */
#line 373 "ldgram.y"
                { ldlex_popstate(); }
#line 2975 "ldgram.c"
    break;

  case 97: /* input_list1: NAME  */
#line 377 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2982 "ldgram.c"
    break;

  case 98: /* input_list1: input_list1 ',' NAME  */
#line 380 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2989 "ldgram.c"
    break;

  case 99: /* input_list1: input_list1 NAME  */
#line 383 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2996 "ldgram.c"
    break;

  case 100: /* input_list1: LNAME  */
#line 386 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 3003 "ldgram.c"
    break;

  case 101: /* input_list1: input_list1 ',' LNAME  */
#line 389 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 3010 "ldgram.c"
    break;

  case 102: /* input_list1: input_list1 LNAME  */
#line 392 "ldgram.y"
                { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 3017 "ldgram.c"
    break;

  case 103: /* @9: %empty  */
#line 395 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3024 "ldgram.c"
    break;

  case 104: /* input_list1: AS_NEEDED '(' @9 input_list1 ')'  */
#line 398 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3030 "ldgram.c"
    break;

  case 105: /* @10: %empty  */
#line 400 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3037 "ldgram.c"
    break;

  case 106: /* input_list1: input_list1 ',' AS_NEEDED '(' @10 input_list1 ')'  */
#line 403 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3043 "ldgram.c"
    break;

  case 107: /* @11: %empty  */
#line 405 "ldgram.y"
                  { (yyval.integer) = input_flags.add_DT_NEEDED_for_regular;
		    input_flags.add_DT_NEEDED_for_regular = true; }
#line 3050 "ldgram.c"
    break;

  case 108: /* input_list1: input_list1 AS_NEEDED '(' @11 input_list1 ')'  */
#line 408 "ldgram.y"
                  { input_flags.add_DT_NEEDED_for_regular = (yyvsp[-2].integer); }
#line 3056 "ldgram.c"
    break;

  case 113: /* statement_anywhere: ENTRY '(' NAME ')'  */
#line 423 "ldgram.y"
                { lang_add_entry ((yyvsp[-1].name), false); }
#line 3062 "ldgram.c"
    break;

  case 115: /* $@12: %empty  */
#line 425 "ldgram.y"
                          {ldlex_expression ();}
#line 3068 "ldgram.c"
    break;

  case 116: /* statement_anywhere: ASSERT_K $@12 '(' exp ',' NAME ')'  */
#line 426 "ldgram.y"
                { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 3075 "ldgram.c"
    break;

  case 117: /* wildcard_name: NAME  */
#line 432 "ldgram.y"
                        {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 3083 "ldgram.c"
    break;

  case 118: /* wildcard_maybe_exclude: wildcard_name  */
#line 439 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			  (yyval.wildcard).section_flag_list = NULL;
			  (yyval.wildcard).reversed = false;
			}
#line 3095 "ldgram.c"
    break;

  case 119: /* wildcard_maybe_exclude: EXCLUDE_FILE '(' exclude_name_list ')' wildcard_name  */
#line 447 "ldgram.y"
                        {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			  (yyval.wildcard).section_flag_list = NULL;
			  (yyval.wildcard).reversed = false;
			}
#line 3107 "ldgram.c"
    break;

  case 121: /* wildcard_maybe_reverse: REVERSE '(' wildcard_maybe_exclude ')'  */
#line 459 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).reversed = true;
			  (yyval.wildcard).sorted = by_name;
			}
#line 3117 "ldgram.c"
    break;

  case 123: /* filename_spec: SORT_BY_NAME '(' wildcard_maybe_reverse ')'  */
#line 469 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3126 "ldgram.c"
    break;

  case 124: /* filename_spec: SORT_NONE '(' wildcard_maybe_reverse ')'  */
#line 474 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			  (yyval.wildcard).reversed = false;
			}
#line 3136 "ldgram.c"
    break;

  case 125: /* filename_spec: REVERSE '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 480 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).reversed = true;
			}
#line 3146 "ldgram.c"
    break;

  case 127: /* section_name_spec: SORT_BY_NAME '(' wildcard_maybe_reverse ')'  */
#line 490 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3155 "ldgram.c"
    break;

  case 128: /* section_name_spec: SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')'  */
#line 495 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3164 "ldgram.c"
    break;

  case 129: /* section_name_spec: SORT_NONE '(' wildcard_maybe_reverse ')'  */
#line 500 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_none;
			}
#line 3173 "ldgram.c"
    break;

  case 130: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')' ')'  */
#line 505 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name_alignment;
			}
#line 3182 "ldgram.c"
    break;

  case 131: /* section_name_spec: SORT_BY_NAME '(' SORT_BY_NAME '(' wildcard_maybe_reverse ')' ')'  */
#line 510 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			}
#line 3191 "ldgram.c"
    break;

  case 132: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_NAME '(' wildcard_maybe_reverse ')' ')'  */
#line 515 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment_name;
			}
#line 3200 "ldgram.c"
    break;

  case 133: /* section_name_spec: SORT_BY_ALIGNMENT '(' SORT_BY_ALIGNMENT '(' wildcard_maybe_reverse ')' ')'  */
#line 520 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_alignment;
			}
#line 3209 "ldgram.c"
    break;

  case 134: /* section_name_spec: SORT_BY_INIT_PRIORITY '(' wildcard_maybe_reverse ')'  */
#line 525 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-1].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			}
#line 3218 "ldgram.c"
    break;

  case 135: /* section_name_spec: REVERSE '(' SORT_BY_NAME '(' wildcard_maybe_exclude ')' ')'  */
#line 530 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).reversed = true;
			}
#line 3228 "ldgram.c"
    break;

  case 136: /* section_name_spec: REVERSE '(' SORT_BY_INIT_PRIORITY '(' wildcard_maybe_exclude ')' ')'  */
#line 536 "ldgram.y"
                        {
			  (yyval.wildcard) = (yyvsp[-2].wildcard);
			  (yyval.wildcard).sorted = by_init_priority;
			  (yyval.wildcard).reversed = true;
			}
#line 3238 "ldgram.c"
    break;

  case 137: /* sect_flag_list: NAME  */
#line 544 "ldgram.y"
                        {
			  struct flag_info_list *n;
			  n = stat_alloc (sizeof *n);
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
#line 3260 "ldgram.c"
    break;

  case 138: /* sect_flag_list: sect_flag_list '&' NAME  */
#line 562 "ldgram.y"
                        {
			  struct flag_info_list *n;
			  n = stat_alloc (sizeof *n);
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
#line 3282 "ldgram.c"
    break;

  case 139: /* sect_flags: INPUT_SECTION_FLAGS '(' sect_flag_list ')'  */
#line 583 "ldgram.y"
                        {
			  struct flag_info *n;
			  n = stat_alloc (sizeof *n);
			  n->flag_list = (yyvsp[-1].flag_info_list);
			  n->flags_initialized = false;
			  n->not_with_flags = 0;
			  n->only_with_flags = 0;
			  (yyval.flag_info) = n;
			}
#line 3296 "ldgram.c"
    break;

  case 140: /* exclude_name_list: exclude_name_list wildcard_name  */
#line 596 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = stat_alloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 3308 "ldgram.c"
    break;

  case 141: /* exclude_name_list: wildcard_name  */
#line 605 "ldgram.y"
                        {
			  struct name_list *tmp;
			  tmp = stat_alloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 3320 "ldgram.c"
    break;

  case 142: /* section_name_list: section_name_list opt_comma section_name_spec  */
#line 616 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = stat_alloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3332 "ldgram.c"
    break;

  case 143: /* section_name_list: section_name_spec  */
#line 625 "ldgram.y"
                        {
			  struct wildcard_list *tmp;
			  tmp = stat_alloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 3344 "ldgram.c"
    break;

  case 144: /* input_section_spec_no_keep: NAME  */
#line 636 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = NULL;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3357 "ldgram.c"
    break;

  case 145: /* input_section_spec_no_keep: sect_flags NAME  */
#line 645 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-1].flag_info);
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 3370 "ldgram.c"
    break;

  case 146: /* input_section_spec_no_keep: '[' section_name_list ']'  */
#line 654 "ldgram.y"
                        {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3378 "ldgram.c"
    break;

  case 147: /* input_section_spec_no_keep: sect_flags '[' section_name_list ']'  */
#line 658 "ldgram.y"
                        {
			  struct wildcard_spec tmp;
			  tmp.name = NULL;
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  tmp.section_flag_list = (yyvsp[-3].flag_info);
			  lang_add_wild (&tmp, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3391 "ldgram.c"
    break;

  case 148: /* input_section_spec_no_keep: filename_spec '(' section_name_list ')'  */
#line 667 "ldgram.y"
                        {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3399 "ldgram.c"
    break;

  case 149: /* input_section_spec_no_keep: sect_flags filename_spec '(' section_name_list ')'  */
#line 671 "ldgram.y"
                        {
			  (yyvsp[-3].wildcard).section_flag_list = (yyvsp[-4].flag_info);
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 3408 "ldgram.c"
    break;

  case 151: /* $@13: %empty  */
#line 680 "ldgram.y"
                        { ldgram_had_keep = true; }
#line 3414 "ldgram.c"
    break;

  case 152: /* input_section_spec: KEEP '(' $@13 input_section_spec_no_keep ')'  */
#line 682 "ldgram.y"
                        { ldgram_had_keep = false; }
#line 3420 "ldgram.c"
    break;

  case 155: /* statement: CREATE_OBJECT_SYMBOLS  */
#line 689 "ldgram.y"
                {
		  lang_add_attribute (lang_object_symbols_statement_enum);
		}
#line 3428 "ldgram.c"
    break;

  case 156: /* statement: CONSTRUCTORS  */
#line 693 "ldgram.y"
                {
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3436 "ldgram.c"
    break;

  case 157: /* statement: SORT_BY_NAME '(' CONSTRUCTORS ')'  */
#line 697 "ldgram.y"
                {
		  constructors_sorted = true;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 3445 "ldgram.c"
    break;

  case 159: /* statement: length '(' mustbe_exp ')'  */
#line 703 "ldgram.y"
                {
		  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
		}
#line 3453 "ldgram.c"
    break;

  case 160: /* statement: ASCIZ NAME  */
#line 707 "ldgram.y"
                {
		  lang_add_string ((yyvsp[0].name));
		}
#line 3461 "ldgram.c"
    break;

  case 161: /* statement: FILL '(' fill_exp ')'  */
#line 711 "ldgram.y"
                {
		  lang_add_fill ((yyvsp[-1].fill));
		}
#line 3469 "ldgram.c"
    break;

  case 162: /* statement: LINKER_VERSION  */
#line 715 "ldgram.y"
                {
		  lang_add_version_string ();
		}
#line 3477 "ldgram.c"
    break;

  case 163: /* $@14: %empty  */
#line 719 "ldgram.y"
                { ldlex_expression (); }
#line 3483 "ldgram.c"
    break;

  case 164: /* statement: ASSERT_K $@14 '(' exp ',' NAME ')' separator  */
#line 721 "ldgram.y"
                {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-4].etree), (yyvsp[-2].name)));
		}
#line 3492 "ldgram.c"
    break;

  case 165: /* $@15: %empty  */
#line 726 "ldgram.y"
                {
		  ldfile_open_command_file ((yyvsp[0].name));
		}
#line 3500 "ldgram.c"
    break;

  case 171: /* length: QUAD  */
#line 744 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3506 "ldgram.c"
    break;

  case 172: /* length: SQUAD  */
#line 746 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3512 "ldgram.c"
    break;

  case 173: /* length: LONG  */
#line 748 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3518 "ldgram.c"
    break;

  case 174: /* length: SHORT  */
#line 750 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3524 "ldgram.c"
    break;

  case 175: /* length: BYTE  */
#line 752 "ldgram.y"
                        { (yyval.integer) = (yyvsp[0].token); }
#line 3530 "ldgram.c"
    break;

  case 176: /* fill_exp: mustbe_exp  */
#line 757 "ldgram.y"
                {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, _("fill value"));
		}
#line 3538 "ldgram.c"
    break;

  case 177: /* fill_opt: '=' fill_exp  */
#line 764 "ldgram.y"
                { (yyval.fill) = (yyvsp[0].fill); }
#line 3544 "ldgram.c"
    break;

  case 178: /* fill_opt: %empty  */
#line 765 "ldgram.y"
                { (yyval.fill) = (fill_type *) 0; }
#line 3550 "ldgram.c"
    break;

  case 179: /* assign_op: PLUSEQ  */
#line 770 "ldgram.y"
                        { (yyval.token) = '+'; }
#line 3556 "ldgram.c"
    break;

  case 180: /* assign_op: MINUSEQ  */
#line 772 "ldgram.y"
                        { (yyval.token) = '-'; }
#line 3562 "ldgram.c"
    break;

  case 181: /* assign_op: MULTEQ  */
#line 774 "ldgram.y"
                        { (yyval.token) = '*'; }
#line 3568 "ldgram.c"
    break;

  case 182: /* assign_op: DIVEQ  */
#line 776 "ldgram.y"
                        { (yyval.token) = '/'; }
#line 3574 "ldgram.c"
    break;

  case 183: /* assign_op: LSHIFTEQ  */
#line 778 "ldgram.y"
                        { (yyval.token) = LSHIFT; }
#line 3580 "ldgram.c"
    break;

  case 184: /* assign_op: RSHIFTEQ  */
#line 780 "ldgram.y"
                        { (yyval.token) = RSHIFT; }
#line 3586 "ldgram.c"
    break;

  case 185: /* assign_op: ANDEQ  */
#line 782 "ldgram.y"
                        { (yyval.token) = '&'; }
#line 3592 "ldgram.c"
    break;

  case 186: /* assign_op: OREQ  */
#line 784 "ldgram.y"
                        { (yyval.token) = '|'; }
#line 3598 "ldgram.c"
    break;

  case 187: /* assign_op: XOREQ  */
#line 786 "ldgram.y"
                        { (yyval.token) = '^'; }
#line 3604 "ldgram.c"
    break;

  case 190: /* assignment: NAME '=' mustbe_exp  */
#line 796 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name), (yyvsp[0].etree), false));
		}
#line 3612 "ldgram.c"
    break;

  case 191: /* assignment: NAME assign_op mustbe_exp  */
#line 800 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-2].name),
						   exp_binop ((yyvsp[-1].token),
							      exp_nameop (NAME,
									  (yyvsp[-2].name)),
							      (yyvsp[0].etree)), false));
		}
#line 3624 "ldgram.c"
    break;

  case 192: /* assignment: HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 808 "ldgram.y"
                {
		  lang_add_assignment (exp_assign ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3632 "ldgram.c"
    break;

  case 193: /* assignment: PROVIDE '(' NAME '=' mustbe_exp ')'  */
#line 812 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), false));
		}
#line 3640 "ldgram.c"
    break;

  case 194: /* assignment: PROVIDE_HIDDEN '(' NAME '=' mustbe_exp ')'  */
#line 816 "ldgram.y"
                {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), true));
		}
#line 3648 "ldgram.c"
    break;

  case 202: /* $@16: %empty  */
#line 839 "ldgram.y"
                { region = lang_memory_region_lookup ((yyvsp[0].name), true); }
#line 3654 "ldgram.c"
    break;

  case 203: /* memory_spec: NAME $@16 attributes_opt ':' origin_spec opt_comma length_spec  */
#line 842 "ldgram.y"
                {}
#line 3660 "ldgram.c"
    break;

  case 204: /* $@17: %empty  */
#line 844 "ldgram.y"
                { ldfile_open_command_file ((yyvsp[0].name)); }
#line 3666 "ldgram.c"
    break;

  case 206: /* origin_spec: ORIGIN '=' mustbe_exp  */
#line 850 "ldgram.y"
                {
		  region->origin_exp = (yyvsp[0].etree);
		}
#line 3674 "ldgram.c"
    break;

  case 207: /* length_spec: LENGTH '=' mustbe_exp  */
#line 857 "ldgram.y"
                {
		  if (yychar == NAME)
		    {
		      yyclearin;
		      ldlex_backup ();
		    }
		  region->length_exp = (yyvsp[0].etree);
		}
#line 3687 "ldgram.c"
    break;

  case 208: /* attributes_opt: %empty  */
#line 869 "ldgram.y"
                  { /* dummy action to avoid bison 1.25 error message */ }
#line 3693 "ldgram.c"
    break;

  case 212: /* attributes_string: NAME  */
#line 880 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3699 "ldgram.c"
    break;

  case 213: /* attributes_string: '!' NAME  */
#line 882 "ldgram.y"
                  { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3705 "ldgram.c"
    break;

  case 214: /* startup: STARTUP '(' filename ')'  */
#line 887 "ldgram.y"
                { lang_startup((yyvsp[-1].name)); }
#line 3711 "ldgram.c"
    break;

  case 216: /* high_level_library: HLL '(' ')'  */
#line 893 "ldgram.y"
                        { ldemul_hll((char *)NULL); }
#line 3717 "ldgram.c"
    break;

  case 217: /* high_level_library_NAME_list: high_level_library_NAME_list opt_comma filename  */
#line 898 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3723 "ldgram.c"
    break;

  case 218: /* high_level_library_NAME_list: filename  */
#line 900 "ldgram.y"
                        { ldemul_hll((yyvsp[0].name)); }
#line 3729 "ldgram.c"
    break;

  case 220: /* low_level_library_NAME_list: low_level_library_NAME_list opt_comma filename  */
#line 909 "ldgram.y"
                        { ldemul_syslib((yyvsp[0].name)); }
#line 3735 "ldgram.c"
    break;

  case 222: /* floating_point_support: FLOAT  */
#line 915 "ldgram.y"
                        { lang_float(true); }
#line 3741 "ldgram.c"
    break;

  case 223: /* floating_point_support: NOFLOAT  */
#line 917 "ldgram.y"
                        { lang_float(false); }
#line 3747 "ldgram.c"
    break;

  case 224: /* nocrossref_list: %empty  */
#line 922 "ldgram.y"
                {
		  (yyval.nocrossref) = NULL;
		}
#line 3755 "ldgram.c"
    break;

  case 225: /* nocrossref_list: NAME nocrossref_list  */
#line 926 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = stat_alloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3768 "ldgram.c"
    break;

  case 226: /* nocrossref_list: NAME ',' nocrossref_list  */
#line 935 "ldgram.y"
                {
		  struct lang_nocrossref *n;

		  n = stat_alloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3781 "ldgram.c"
    break;

  case 227: /* $@18: %empty  */
#line 945 "ldgram.y"
                        { ldlex_script (); }
#line 3787 "ldgram.c"
    break;

  case 228: /* paren_script_name: $@18 '(' NAME ')'  */
#line 947 "ldgram.y"
                        { ldlex_popstate (); (yyval.name) = (yyvsp[-1].name); }
#line 3793 "ldgram.c"
    break;

  case 229: /* $@19: %empty  */
#line 949 "ldgram.y"
                        { ldlex_expression (); }
#line 3799 "ldgram.c"
    break;

  case 230: /* mustbe_exp: $@19 exp  */
#line 951 "ldgram.y"
                        { ldlex_popstate (); (yyval.etree) = (yyvsp[0].etree); }
#line 3805 "ldgram.c"
    break;

  case 231: /* exp: '-' exp  */
#line 956 "ldgram.y"
                        { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3811 "ldgram.c"
    break;

  case 232: /* exp: '(' exp ')'  */
#line 958 "ldgram.y"
                        { (yyval.etree) = (yyvsp[-1].etree); }
#line 3817 "ldgram.c"
    break;

  case 233: /* exp: NEXT '(' exp ')'  */
#line 960 "ldgram.y"
                        { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3823 "ldgram.c"
    break;

  case 234: /* exp: '!' exp  */
#line 962 "ldgram.y"
                        { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3829 "ldgram.c"
    break;

  case 235: /* exp: '+' exp  */
#line 964 "ldgram.y"
                        { (yyval.etree) = (yyvsp[0].etree); }
#line 3835 "ldgram.c"
    break;

  case 236: /* exp: '~' exp  */
#line 966 "ldgram.y"
                        { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3841 "ldgram.c"
    break;

  case 237: /* exp: exp '*' exp  */
#line 969 "ldgram.y"
                        { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3847 "ldgram.c"
    break;

  case 238: /* exp: exp '/' exp  */
#line 971 "ldgram.y"
                        { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3853 "ldgram.c"
    break;

  case 239: /* exp: exp '%' exp  */
#line 973 "ldgram.y"
                        { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3859 "ldgram.c"
    break;

  case 240: /* exp: exp '+' exp  */
#line 975 "ldgram.y"
                        { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3865 "ldgram.c"
    break;

  case 241: /* exp: exp '-' exp  */
#line 977 "ldgram.y"
                        { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3871 "ldgram.c"
    break;

  case 242: /* exp: exp LSHIFT exp  */
#line 979 "ldgram.y"
                        { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3877 "ldgram.c"
    break;

  case 243: /* exp: exp RSHIFT exp  */
#line 981 "ldgram.y"
                        { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3883 "ldgram.c"
    break;

  case 244: /* exp: exp EQ exp  */
#line 983 "ldgram.y"
                        { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3889 "ldgram.c"
    break;

  case 245: /* exp: exp NE exp  */
#line 985 "ldgram.y"
                        { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3895 "ldgram.c"
    break;

  case 246: /* exp: exp LE exp  */
#line 987 "ldgram.y"
                        { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3901 "ldgram.c"
    break;

  case 247: /* exp: exp GE exp  */
#line 989 "ldgram.y"
                        { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3907 "ldgram.c"
    break;

  case 248: /* exp: exp '<' exp  */
#line 991 "ldgram.y"
                        { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3913 "ldgram.c"
    break;

  case 249: /* exp: exp '>' exp  */
#line 993 "ldgram.y"
                        { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3919 "ldgram.c"
    break;

  case 250: /* exp: exp '&' exp  */
#line 995 "ldgram.y"
                        { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3925 "ldgram.c"
    break;

  case 251: /* exp: exp '^' exp  */
#line 997 "ldgram.y"
                        { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3931 "ldgram.c"
    break;

  case 252: /* exp: exp '|' exp  */
#line 999 "ldgram.y"
                        { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3937 "ldgram.c"
    break;

  case 253: /* exp: exp '?' exp ':' exp  */
#line 1001 "ldgram.y"
                        { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3943 "ldgram.c"
    break;

  case 254: /* exp: exp ANDAND exp  */
#line 1003 "ldgram.y"
                        { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3949 "ldgram.c"
    break;

  case 255: /* exp: exp OROR exp  */
#line 1005 "ldgram.y"
                        { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3955 "ldgram.c"
    break;

  case 256: /* exp: DEFINED '(' NAME ')'  */
#line 1007 "ldgram.y"
                        { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3961 "ldgram.c"
    break;

  case 257: /* exp: INT  */
#line 1009 "ldgram.y"
                        { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3967 "ldgram.c"
    break;

  case 258: /* exp: SIZEOF_HEADERS  */
#line 1011 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3973 "ldgram.c"
    break;

  case 259: /* exp: ALIGNOF paren_script_name  */
#line 1014 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ALIGNOF, (yyvsp[0].name)); }
#line 3979 "ldgram.c"
    break;

  case 260: /* exp: SIZEOF paren_script_name  */
#line 1016 "ldgram.y"
                        { (yyval.etree) = exp_nameop (SIZEOF, (yyvsp[0].name)); }
#line 3985 "ldgram.c"
    break;

  case 261: /* exp: ADDR paren_script_name  */
#line 1018 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ADDR, (yyvsp[0].name)); }
#line 3991 "ldgram.c"
    break;

  case 262: /* exp: LOADADDR paren_script_name  */
#line 1020 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LOADADDR, (yyvsp[0].name)); }
#line 3997 "ldgram.c"
    break;

  case 263: /* exp: CONSTANT '(' NAME ')'  */
#line 1022 "ldgram.y"
                        { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 4003 "ldgram.c"
    break;

  case 264: /* exp: ABSOLUTE '(' exp ')'  */
#line 1024 "ldgram.y"
                        { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 4009 "ldgram.c"
    break;

  case 265: /* exp: ALIGN_K '(' exp ')'  */
#line 1026 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 4015 "ldgram.c"
    break;

  case 266: /* exp: ALIGN_K '(' exp ',' exp ')'  */
#line 1028 "ldgram.y"
                        { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 4021 "ldgram.c"
    break;

  case 267: /* exp: DATA_SEGMENT_ALIGN '(' exp ',' exp ')'  */
#line 1030 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 4027 "ldgram.c"
    break;

  case 268: /* exp: DATA_SEGMENT_RELRO_END '(' exp ',' exp ')'  */
#line 1032 "ldgram.y"
                        { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 4033 "ldgram.c"
    break;

  case 269: /* exp: DATA_SEGMENT_END '(' exp ')'  */
#line 1034 "ldgram.y"
                        { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 4039 "ldgram.c"
    break;

  case 270: /* $@20: %empty  */
#line 1035 "ldgram.y"
                              { ldlex_script (); }
#line 4045 "ldgram.c"
    break;

  case 271: /* $@21: %empty  */
#line 1036 "ldgram.y"
                        { ldlex_popstate (); }
#line 4051 "ldgram.c"
    break;

  case 272: /* exp: SEGMENT_START $@20 '(' NAME $@21 ',' exp ')'  */
#line 1037 "ldgram.y"
                        { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-4].name))); }
#line 4064 "ldgram.c"
    break;

  case 273: /* exp: BLOCK '(' exp ')'  */
#line 1046 "ldgram.y"
                        { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 4070 "ldgram.c"
    break;

  case 274: /* exp: NAME  */
#line 1048 "ldgram.y"
                        { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 4076 "ldgram.c"
    break;

  case 275: /* exp: MAX_K '(' exp ',' exp ')'  */
#line 1050 "ldgram.y"
                        { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 4082 "ldgram.c"
    break;

  case 276: /* exp: MIN_K '(' exp ',' exp ')'  */
#line 1052 "ldgram.y"
                        { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 4088 "ldgram.c"
    break;

  case 277: /* exp: ASSERT_K '(' exp ',' NAME ')'  */
#line 1054 "ldgram.y"
                        { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 4094 "ldgram.c"
    break;

  case 278: /* exp: ORIGIN paren_script_name  */
#line 1056 "ldgram.y"
                        { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[0].name)); }
#line 4100 "ldgram.c"
    break;

  case 279: /* exp: LENGTH paren_script_name  */
#line 1058 "ldgram.y"
                        { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[0].name)); }
#line 4106 "ldgram.c"
    break;

  case 280: /* exp: LOG2CEIL '(' exp ')'  */
#line 1060 "ldgram.y"
                        { (yyval.etree) = exp_unop (LOG2CEIL, (yyvsp[-1].etree)); }
#line 4112 "ldgram.c"
    break;

  case 281: /* memspec_at_opt: AT '>' NAME  */
#line 1065 "ldgram.y"
                            { (yyval.name) = (yyvsp[0].name); }
#line 4118 "ldgram.c"
    break;

  case 282: /* memspec_at_opt: %empty  */
#line 1066 "ldgram.y"
                { (yyval.name) = 0; }
#line 4124 "ldgram.c"
    break;

  case 283: /* opt_at: AT '(' exp ')'  */
#line 1070 "ldgram.y"
                               { (yyval.etree) = (yyvsp[-1].etree); }
#line 4130 "ldgram.c"
    break;

  case 284: /* opt_at: %empty  */
#line 1071 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4136 "ldgram.c"
    break;

  case 285: /* opt_align: ALIGN_K '(' exp ')'  */
#line 1075 "ldgram.y"
                                    { (yyval.etree) = (yyvsp[-1].etree); }
#line 4142 "ldgram.c"
    break;

  case 286: /* opt_align: %empty  */
#line 1076 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4148 "ldgram.c"
    break;

  case 287: /* opt_align_with_input: ALIGN_WITH_INPUT  */
#line 1080 "ldgram.y"
                                 { (yyval.token) = ALIGN_WITH_INPUT; }
#line 4154 "ldgram.c"
    break;

  case 288: /* opt_align_with_input: %empty  */
#line 1081 "ldgram.y"
                { (yyval.token) = 0; }
#line 4160 "ldgram.c"
    break;

  case 289: /* opt_subalign: SUBALIGN '(' exp ')'  */
#line 1085 "ldgram.y"
                                     { (yyval.etree) = (yyvsp[-1].etree); }
#line 4166 "ldgram.c"
    break;

  case 290: /* opt_subalign: %empty  */
#line 1086 "ldgram.y"
                { (yyval.etree) = 0; }
#line 4172 "ldgram.c"
    break;

  case 291: /* sect_constraint: ONLY_IF_RO  */
#line 1090 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RO; }
#line 4178 "ldgram.c"
    break;

  case 292: /* sect_constraint: ONLY_IF_RW  */
#line 1091 "ldgram.y"
                           { (yyval.token) = ONLY_IF_RW; }
#line 4184 "ldgram.c"
    break;

  case 293: /* sect_constraint: SPECIAL  */
#line 1092 "ldgram.y"
                        { (yyval.token) = SPECIAL; }
#line 4190 "ldgram.c"
    break;

  case 294: /* sect_constraint: %empty  */
#line 1093 "ldgram.y"
                { (yyval.token) = 0; }
#line 4196 "ldgram.c"
    break;

  case 295: /* $@22: %empty  */
#line 1097 "ldgram.y"
                        { ldlex_expression(); }
#line 4202 "ldgram.c"
    break;

  case 296: /* $@23: %empty  */
#line 1104 "ldgram.y"
                        {
			  ldlex_popstate ();
			  ldlex_wild ();
			  lang_enter_output_section_statement ((yyvsp[-7].name), (yyvsp[-5].etree), sectype,
					sectype_value, (yyvsp[-3].etree), (yyvsp[-1].etree), (yyvsp[-4].etree), (yyvsp[0].token), (yyvsp[-2].token));
			}
#line 4213 "ldgram.c"
    break;

  case 297: /* $@24: %empty  */
#line 1113 "ldgram.y"
                        { ldlex_popstate (); }
#line 4219 "ldgram.c"
    break;

  case 298: /* $@25: %empty  */
#line 1115 "ldgram.y"
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
#line 4240 "ldgram.c"
    break;

  case 300: /* $@26: %empty  */
#line 1133 "ldgram.y"
                        { ldlex_expression (); }
#line 4246 "ldgram.c"
    break;

  case 301: /* $@27: %empty  */
#line 1135 "ldgram.y"
                        { ldlex_popstate (); }
#line 4252 "ldgram.c"
    break;

  case 302: /* $@28: %empty  */
#line 1137 "ldgram.y"
                        {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 4260 "ldgram.c"
    break;

  case 303: /* $@29: %empty  */
#line 1143 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay ((yyvsp[-10].etree), (int) (yyvsp[-11].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 4274 "ldgram.c"
    break;

  case 305: /* $@30: %empty  */
#line 1158 "ldgram.y"
                        { ldlex_expression (); }
#line 4280 "ldgram.c"
    break;

  case 306: /* $@31: %empty  */
#line 1160 "ldgram.y"
                        {
			  ldlex_popstate ();
			  lang_add_assignment (exp_assign (".", (yyvsp[0].etree), false));
			}
#line 4289 "ldgram.c"
    break;

  case 308: /* $@32: %empty  */
#line 1166 "ldgram.y"
                        {
			  ldfile_open_command_file ((yyvsp[0].name));
			}
#line 4297 "ldgram.c"
    break;

  case 310: /* type: NOLOAD  */
#line 1173 "ldgram.y"
                   { sectype = noload_section; }
#line 4303 "ldgram.c"
    break;

  case 311: /* type: DSECT  */
#line 1174 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4309 "ldgram.c"
    break;

  case 312: /* type: COPY  */
#line 1175 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4315 "ldgram.c"
    break;

  case 313: /* type: INFO  */
#line 1176 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4321 "ldgram.c"
    break;

  case 314: /* type: OVERLAY  */
#line 1177 "ldgram.y"
                   { sectype = noalloc_section; }
#line 4327 "ldgram.c"
    break;

  case 315: /* type: READONLY '(' TYPE '=' exp ')'  */
#line 1178 "ldgram.y"
                                         { sectype = typed_readonly_section; sectype_value = (yyvsp[-1].etree); }
#line 4333 "ldgram.c"
    break;

  case 316: /* type: READONLY  */
#line 1179 "ldgram.y"
                    { sectype = readonly_section; }
#line 4339 "ldgram.c"
    break;

  case 317: /* type: TYPE '=' exp  */
#line 1180 "ldgram.y"
                        { sectype = type_section; sectype_value = (yyvsp[0].etree); }
#line 4345 "ldgram.c"
    break;

  case 319: /* atype: %empty  */
#line 1185 "ldgram.y"
                            { sectype = normal_section; }
#line 4351 "ldgram.c"
    break;

  case 320: /* atype: '(' ')'  */
#line 1186 "ldgram.y"
                        { sectype = normal_section; }
#line 4357 "ldgram.c"
    break;

  case 321: /* opt_exp_with_type: exp atype ':'  */
#line 1190 "ldgram.y"
                                        { (yyval.etree) = (yyvsp[-2].etree); }
#line 4363 "ldgram.c"
    break;

  case 322: /* opt_exp_with_type: atype ':'  */
#line 1191 "ldgram.y"
                                        { (yyval.etree) = (etree_type *)NULL;  }
#line 4369 "ldgram.c"
    break;

  case 323: /* opt_exp_with_type: BIND '(' exp ')' atype ':'  */
#line 1196 "ldgram.y"
                                           { (yyval.etree) = (yyvsp[-3].etree); }
#line 4375 "ldgram.c"
    break;

  case 324: /* opt_exp_with_type: BIND '(' exp ')' BLOCK '(' exp ')' atype ':'  */
#line 1198 "ldgram.y"
                { (yyval.etree) = (yyvsp[-7].etree); }
#line 4381 "ldgram.c"
    break;

  case 325: /* opt_exp_without_type: exp ':'  */
#line 1202 "ldgram.y"
                                { (yyval.etree) = (yyvsp[-1].etree); }
#line 4387 "ldgram.c"
    break;

  case 326: /* opt_exp_without_type: ':'  */
#line 1203 "ldgram.y"
                                { (yyval.etree) = (etree_type *) NULL;  }
#line 4393 "ldgram.c"
    break;

  case 327: /* opt_nocrossrefs: %empty  */
#line 1208 "ldgram.y"
                        { (yyval.integer) = 0; }
#line 4399 "ldgram.c"
    break;

  case 328: /* opt_nocrossrefs: NOCROSSREFS  */
#line 1210 "ldgram.y"
                        { (yyval.integer) = 1; }
#line 4405 "ldgram.c"
    break;

  case 329: /* memspec_opt: '>' NAME  */
#line 1215 "ldgram.y"
                { (yyval.name) = (yyvsp[0].name); }
#line 4411 "ldgram.c"
    break;

  case 330: /* memspec_opt: %empty  */
#line 1216 "ldgram.y"
                { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 4417 "ldgram.c"
    break;

  case 331: /* phdr_opt: %empty  */
#line 1221 "ldgram.y"
                {
		  (yyval.section_phdr) = NULL;
		}
#line 4425 "ldgram.c"
    break;

  case 332: /* phdr_opt: phdr_opt ':' NAME  */
#line 1225 "ldgram.y"
                {
		  struct lang_output_section_phdr_list *n;

		  n = stat_alloc (sizeof *n);
		  n->name = (yyvsp[0].name);
		  n->used = false;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 4439 "ldgram.c"
    break;

  case 334: /* $@33: %empty  */
#line 1240 "ldgram.y"
                        {
			  ldlex_wild ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 4448 "ldgram.c"
    break;

  case 335: /* $@34: %empty  */
#line 1247 "ldgram.y"
                        { ldlex_popstate (); }
#line 4454 "ldgram.c"
    break;

  case 336: /* $@35: %empty  */
#line 1249 "ldgram.y"
                        {
			  if (yychar == NAME)
			    {
			      yyclearin;
			      ldlex_backup ();
			    }
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 4467 "ldgram.c"
    break;

  case 341: /* $@36: %empty  */
#line 1270 "ldgram.y"
                     { ldlex_expression (); }
#line 4473 "ldgram.c"
    break;

  case 342: /* $@37: %empty  */
#line 1271 "ldgram.y"
                                            { ldlex_popstate (); }
#line 4479 "ldgram.c"
    break;

  case 343: /* phdr: NAME $@36 phdr_type phdr_qualifiers $@37 ';'  */
#line 1273 "ldgram.y"
                {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 4488 "ldgram.c"
    break;

  case 344: /* phdr_type: exp  */
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
#line 4537 "ldgram.c"
    break;

  case 345: /* phdr_qualifiers: %empty  */
#line 1329 "ldgram.y"
                {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 4545 "ldgram.c"
    break;

  case 346: /* phdr_qualifiers: NAME phdr_val phdr_qualifiers  */
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
#line 4562 "ldgram.c"
    break;

  case 347: /* phdr_qualifiers: AT '(' exp ')' phdr_qualifiers  */
#line 1346 "ldgram.y"
                {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 4571 "ldgram.c"
    break;

  case 348: /* phdr_val: %empty  */
#line 1354 "ldgram.y"
                {
		  (yyval.etree) = NULL;
		}
#line 4579 "ldgram.c"
    break;

  case 349: /* phdr_val: '(' exp ')'  */
#line 1358 "ldgram.y"
                {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 4587 "ldgram.c"
    break;

  case 350: /* $@38: %empty  */
#line 1364 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 4596 "ldgram.c"
    break;

  case 351: /* dynamic_list_file: $@38 dynamic_list_nodes  */
#line 1369 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4605 "ldgram.c"
    break;

  case 355: /* dynamic_list_tag: vers_defns ';'  */
#line 1386 "ldgram.y"
                {
		  lang_append_dynamic_list (current_dynamic_list_p, (yyvsp[-1].versyms));
		}
#line 4613 "ldgram.c"
    break;

  case 356: /* $@39: %empty  */
#line 1394 "ldgram.y"
                {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 4622 "ldgram.c"
    break;

  case 357: /* version_script_file: $@39 vers_nodes  */
#line 1399 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4631 "ldgram.c"
    break;

  case 358: /* $@40: %empty  */
#line 1408 "ldgram.y"
                {
		  ldlex_version_script ();
		}
#line 4639 "ldgram.c"
    break;

  case 359: /* version: $@40 VERSIONK '{' vers_nodes '}'  */
#line 1412 "ldgram.y"
                {
		  ldlex_popstate ();
		}
#line 4647 "ldgram.c"
    break;

  case 362: /* vers_node: '{' vers_tag '}' ';'  */
#line 1424 "ldgram.y"
                {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 4655 "ldgram.c"
    break;

  case 363: /* vers_node: VERS_TAG '{' vers_tag '}' ';'  */
#line 1428 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 4663 "ldgram.c"
    break;

  case 364: /* vers_node: VERS_TAG '{' vers_tag '}' verdep ';'  */
#line 1432 "ldgram.y"
                {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 4671 "ldgram.c"
    break;

  case 365: /* verdep: VERS_TAG  */
#line 1439 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 4679 "ldgram.c"
    break;

  case 366: /* verdep: verdep VERS_TAG  */
#line 1443 "ldgram.y"
                {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4687 "ldgram.c"
    break;

  case 367: /* vers_tag: %empty  */
#line 1450 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4695 "ldgram.c"
    break;

  case 368: /* vers_tag: vers_defns ';'  */
#line 1454 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4703 "ldgram.c"
    break;

  case 369: /* vers_tag: GLOBAL ':' vers_defns ';'  */
#line 1458 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4711 "ldgram.c"
    break;

  case 370: /* vers_tag: LOCAL ':' vers_defns ';'  */
#line 1462 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4719 "ldgram.c"
    break;

  case 371: /* vers_tag: GLOBAL ':' vers_defns ';' LOCAL ':' vers_defns ';'  */
#line 1466 "ldgram.y"
                {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4727 "ldgram.c"
    break;

  case 372: /* vers_defns: VERS_IDENTIFIER  */
#line 1473 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4735 "ldgram.c"
    break;

  case 373: /* vers_defns: NAME  */
#line 1477 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4743 "ldgram.c"
    break;

  case 374: /* vers_defns: vers_defns ';' VERS_IDENTIFIER  */
#line 1481 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, false);
		}
#line 4751 "ldgram.c"
    break;

  case 375: /* vers_defns: vers_defns ';' NAME  */
#line 1485 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, true);
		}
#line 4759 "ldgram.c"
    break;

  case 376: /* @41: %empty  */
#line 1489 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4768 "ldgram.c"
    break;

  case 377: /* vers_defns: vers_defns ';' EXTERN NAME '{' @41 vers_defns opt_semicolon '}'  */
#line 1494 "ldgram.y"
                        {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4780 "ldgram.c"
    break;

  case 378: /* @42: %empty  */
#line 1502 "ldgram.y"
                        {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4789 "ldgram.c"
    break;

  case 379: /* vers_defns: EXTERN NAME '{' @42 vers_defns opt_semicolon '}'  */
#line 1507 "ldgram.y"
                        {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4798 "ldgram.c"
    break;

  case 380: /* vers_defns: GLOBAL  */
#line 1512 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, false);
		}
#line 4806 "ldgram.c"
    break;

  case 381: /* vers_defns: vers_defns ';' GLOBAL  */
#line 1516 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, false);
		}
#line 4814 "ldgram.c"
    break;

  case 382: /* vers_defns: LOCAL  */
#line 1520 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, false);
		}
#line 4822 "ldgram.c"
    break;

  case 383: /* vers_defns: vers_defns ';' LOCAL  */
#line 1524 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, false);
		}
#line 4830 "ldgram.c"
    break;

  case 384: /* vers_defns: EXTERN  */
#line 1528 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, false);
		}
#line 4838 "ldgram.c"
    break;

  case 385: /* vers_defns: vers_defns ';' EXTERN  */
#line 1532 "ldgram.y"
                {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, false);
		}
#line 4846 "ldgram.c"
    break;

  case 388: /* $@43: %empty  */
#line 1543 "ldgram.y"
                {
		  ldlex_script ();
		  PUSH_ERROR (_("section-ordering-file script"));
		}
#line 4855 "ldgram.c"
    break;

  case 389: /* section_ordering_script_file: $@43 section_ordering_list  */
#line 1548 "ldgram.y"
                {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 4864 "ldgram.c"
    break;

  case 393: /* $@44: %empty  */
#line 1561 "ldgram.y"
                {
		  ldlex_wild ();
		  lang_enter_output_section_statement
		    ((yyvsp[-1].name), NULL, 0, NULL, NULL, NULL, NULL, 0, 0);
		}
#line 4874 "ldgram.c"
    break;

  case 394: /* $@45: %empty  */
#line 1569 "ldgram.y"
                {
		  ldlex_popstate ();
		  lang_leave_output_section_statement (NULL, NULL, NULL, NULL);
		}
#line 4883 "ldgram.c"
    break;


#line 4887 "ldgram.c"

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

#line 1575 "ldgram.y"

static void
yyerror (const char *arg)
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldlex_filename ());
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
    fatal (_("%P:%pS: %s in %s\n"), NULL, arg, error_names[error_index - 1]);
  else
    fatal ("%P:%pS: %s\n", NULL, arg);
}
