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

#line 366 "ldgram.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_LDGRAM_H_INCLUDED  */
