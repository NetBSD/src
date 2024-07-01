/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

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

#line 374 "ldgram.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_LDGRAM_H_INCLUDED  */
