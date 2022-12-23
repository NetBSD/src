/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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

#ifndef YY_YY_LDGRAM_H_INCLUDED
# define YY_YY_LDGRAM_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    NAME = 259,
    LNAME = 260,
    PLUSEQ = 261,
    MINUSEQ = 262,
    MULTEQ = 263,
    DIVEQ = 264,
    LSHIFTEQ = 265,
    RSHIFTEQ = 266,
    ANDEQ = 267,
    OREQ = 268,
    OROR = 269,
    ANDAND = 270,
    EQ = 271,
    NE = 272,
    LE = 273,
    GE = 274,
    LSHIFT = 275,
    RSHIFT = 276,
    UNARY = 277,
    END = 278,
    ALIGN_K = 279,
    BLOCK = 280,
    BIND = 281,
    QUAD = 282,
    SQUAD = 283,
    LONG = 284,
    SHORT = 285,
    BYTE = 286,
    SECTIONS = 287,
    PHDRS = 288,
    INSERT_K = 289,
    AFTER = 290,
    BEFORE = 291,
    DATA_SEGMENT_ALIGN = 292,
    DATA_SEGMENT_RELRO_END = 293,
    DATA_SEGMENT_END = 294,
    SORT_BY_NAME = 295,
    SORT_BY_ALIGNMENT = 296,
    SORT_NONE = 297,
    SORT_BY_INIT_PRIORITY = 298,
    SIZEOF_HEADERS = 299,
    OUTPUT_FORMAT = 300,
    FORCE_COMMON_ALLOCATION = 301,
    OUTPUT_ARCH = 302,
    INHIBIT_COMMON_ALLOCATION = 303,
    FORCE_GROUP_ALLOCATION = 304,
    SEGMENT_START = 305,
    INCLUDE = 306,
    MEMORY = 307,
    REGION_ALIAS = 308,
    LD_FEATURE = 309,
    NOLOAD = 310,
    DSECT = 311,
    COPY = 312,
    INFO = 313,
    OVERLAY = 314,
    DEFINED = 315,
    TARGET_K = 316,
    SEARCH_DIR = 317,
    MAP = 318,
    ENTRY = 319,
    NEXT = 320,
    SIZEOF = 321,
    ALIGNOF = 322,
    ADDR = 323,
    LOADADDR = 324,
    MAX_K = 325,
    MIN_K = 326,
    STARTUP = 327,
    HLL = 328,
    SYSLIB = 329,
    FLOAT = 330,
    NOFLOAT = 331,
    NOCROSSREFS = 332,
    NOCROSSREFS_TO = 333,
    ORIGIN = 334,
    FILL = 335,
    LENGTH = 336,
    CREATE_OBJECT_SYMBOLS = 337,
    INPUT = 338,
    GROUP = 339,
    OUTPUT = 340,
    CONSTRUCTORS = 341,
    ALIGNMOD = 342,
    AT = 343,
    SUBALIGN = 344,
    HIDDEN = 345,
    PROVIDE = 346,
    PROVIDE_HIDDEN = 347,
    AS_NEEDED = 348,
    CHIP = 349,
    LIST = 350,
    SECT = 351,
    ABSOLUTE = 352,
    LOAD = 353,
    NEWLINE = 354,
    ENDWORD = 355,
    ORDER = 356,
    NAMEWORD = 357,
    ASSERT_K = 358,
    LOG2CEIL = 359,
    FORMAT = 360,
    PUBLIC = 361,
    DEFSYMEND = 362,
    BASE = 363,
    ALIAS = 364,
    TRUNCATE = 365,
    REL = 366,
    INPUT_SCRIPT = 367,
    INPUT_MRI_SCRIPT = 368,
    INPUT_DEFSYM = 369,
    CASE = 370,
    EXTERN = 371,
    START = 372,
    VERS_TAG = 373,
    VERS_IDENTIFIER = 374,
    GLOBAL = 375,
    LOCAL = 376,
    VERSIONK = 377,
    INPUT_VERSION_SCRIPT = 378,
    KEEP = 379,
    ONLY_IF_RO = 380,
    ONLY_IF_RW = 381,
    SPECIAL = 382,
    INPUT_SECTION_FLAGS = 383,
    ALIGN_WITH_INPUT = 384,
    EXCLUDE_FILE = 385,
    CONSTANT = 386,
    INPUT_DYNAMIC_LIST = 387
  };
#endif
/* Tokens.  */
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
#define DEFINED 315
#define TARGET_K 316
#define SEARCH_DIR 317
#define MAP 318
#define ENTRY 319
#define NEXT 320
#define SIZEOF 321
#define ALIGNOF 322
#define ADDR 323
#define LOADADDR 324
#define MAX_K 325
#define MIN_K 326
#define STARTUP 327
#define HLL 328
#define SYSLIB 329
#define FLOAT 330
#define NOFLOAT 331
#define NOCROSSREFS 332
#define NOCROSSREFS_TO 333
#define ORIGIN 334
#define FILL 335
#define LENGTH 336
#define CREATE_OBJECT_SYMBOLS 337
#define INPUT 338
#define GROUP 339
#define OUTPUT 340
#define CONSTRUCTORS 341
#define ALIGNMOD 342
#define AT 343
#define SUBALIGN 344
#define HIDDEN 345
#define PROVIDE 346
#define PROVIDE_HIDDEN 347
#define AS_NEEDED 348
#define CHIP 349
#define LIST 350
#define SECT 351
#define ABSOLUTE 352
#define LOAD 353
#define NEWLINE 354
#define ENDWORD 355
#define ORDER 356
#define NAMEWORD 357
#define ASSERT_K 358
#define LOG2CEIL 359
#define FORMAT 360
#define PUBLIC 361
#define DEFSYMEND 362
#define BASE 363
#define ALIAS 364
#define TRUNCATE 365
#define REL 366
#define INPUT_SCRIPT 367
#define INPUT_MRI_SCRIPT 368
#define INPUT_DEFSYM 369
#define CASE 370
#define EXTERN 371
#define START 372
#define VERS_TAG 373
#define VERS_IDENTIFIER 374
#define GLOBAL 375
#define LOCAL 376
#define VERSIONK 377
#define INPUT_VERSION_SCRIPT 378
#define KEEP 379
#define ONLY_IF_RO 380
#define ONLY_IF_RW 381
#define SPECIAL 382
#define INPUT_SECTION_FLAGS 383
#define ALIGN_WITH_INPUT 384
#define EXCLUDE_FILE 385
#define CONSTANT 386
#define INPUT_DYNAMIC_LIST 387

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 61 "ldgram.y" /* yacc.c:1910  */

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
      bfd_boolean filehdr;
      bfd_boolean phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;

#line 349 "ldgram.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_LDGRAM_H_INCLUDED  */
