/* A Bison parser, made by GNU Bison 3.0.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
    SEGMENT_START = 304,
    INCLUDE = 305,
    MEMORY = 306,
    REGION_ALIAS = 307,
    LD_FEATURE = 308,
    NOLOAD = 309,
    DSECT = 310,
    COPY = 311,
    INFO = 312,
    OVERLAY = 313,
    DEFINED = 314,
    TARGET_K = 315,
    SEARCH_DIR = 316,
    MAP = 317,
    ENTRY = 318,
    NEXT = 319,
    SIZEOF = 320,
    ALIGNOF = 321,
    ADDR = 322,
    LOADADDR = 323,
    MAX_K = 324,
    MIN_K = 325,
    STARTUP = 326,
    HLL = 327,
    SYSLIB = 328,
    FLOAT = 329,
    NOFLOAT = 330,
    NOCROSSREFS = 331,
    ORIGIN = 332,
    FILL = 333,
    LENGTH = 334,
    CREATE_OBJECT_SYMBOLS = 335,
    INPUT = 336,
    GROUP = 337,
    OUTPUT = 338,
    CONSTRUCTORS = 339,
    ALIGNMOD = 340,
    AT = 341,
    SUBALIGN = 342,
    HIDDEN = 343,
    PROVIDE = 344,
    PROVIDE_HIDDEN = 345,
    AS_NEEDED = 346,
    CHIP = 347,
    LIST = 348,
    SECT = 349,
    ABSOLUTE = 350,
    LOAD = 351,
    NEWLINE = 352,
    ENDWORD = 353,
    ORDER = 354,
    NAMEWORD = 355,
    ASSERT_K = 356,
    LOG2CEIL = 357,
    FORMAT = 358,
    PUBLIC = 359,
    DEFSYMEND = 360,
    BASE = 361,
    ALIAS = 362,
    TRUNCATE = 363,
    REL = 364,
    INPUT_SCRIPT = 365,
    INPUT_MRI_SCRIPT = 366,
    INPUT_DEFSYM = 367,
    CASE = 368,
    EXTERN = 369,
    START = 370,
    VERS_TAG = 371,
    VERS_IDENTIFIER = 372,
    GLOBAL = 373,
    LOCAL = 374,
    VERSIONK = 375,
    INPUT_VERSION_SCRIPT = 376,
    KEEP = 377,
    ONLY_IF_RO = 378,
    ONLY_IF_RW = 379,
    SPECIAL = 380,
    INPUT_SECTION_FLAGS = 381,
    ALIGN_WITH_INPUT = 382,
    EXCLUDE_FILE = 383,
    CONSTANT = 384,
    INPUT_DYNAMIC_LIST = 385
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
#define SEGMENT_START 304
#define INCLUDE 305
#define MEMORY 306
#define REGION_ALIAS 307
#define LD_FEATURE 308
#define NOLOAD 309
#define DSECT 310
#define COPY 311
#define INFO 312
#define OVERLAY 313
#define DEFINED 314
#define TARGET_K 315
#define SEARCH_DIR 316
#define MAP 317
#define ENTRY 318
#define NEXT 319
#define SIZEOF 320
#define ALIGNOF 321
#define ADDR 322
#define LOADADDR 323
#define MAX_K 324
#define MIN_K 325
#define STARTUP 326
#define HLL 327
#define SYSLIB 328
#define FLOAT 329
#define NOFLOAT 330
#define NOCROSSREFS 331
#define ORIGIN 332
#define FILL 333
#define LENGTH 334
#define CREATE_OBJECT_SYMBOLS 335
#define INPUT 336
#define GROUP 337
#define OUTPUT 338
#define CONSTRUCTORS 339
#define ALIGNMOD 340
#define AT 341
#define SUBALIGN 342
#define HIDDEN 343
#define PROVIDE 344
#define PROVIDE_HIDDEN 345
#define AS_NEEDED 346
#define CHIP 347
#define LIST 348
#define SECT 349
#define ABSOLUTE 350
#define LOAD 351
#define NEWLINE 352
#define ENDWORD 353
#define ORDER 354
#define NAMEWORD 355
#define ASSERT_K 356
#define LOG2CEIL 357
#define FORMAT 358
#define PUBLIC 359
#define DEFSYMEND 360
#define BASE 361
#define ALIAS 362
#define TRUNCATE 363
#define REL 364
#define INPUT_SCRIPT 365
#define INPUT_MRI_SCRIPT 366
#define INPUT_DEFSYM 367
#define CASE 368
#define EXTERN 369
#define START 370
#define VERS_TAG 371
#define VERS_IDENTIFIER 372
#define GLOBAL 373
#define LOCAL 374
#define VERSIONK 375
#define INPUT_VERSION_SCRIPT 376
#define KEEP 377
#define ONLY_IF_RO 378
#define ONLY_IF_RW 379
#define SPECIAL 380
#define INPUT_SECTION_FLAGS 381
#define ALIGN_WITH_INPUT 382
#define EXCLUDE_FILE 383
#define CONSTANT 384
#define INPUT_DYNAMIC_LIST 385

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 60 "ldgram.y" /* yacc.c:1915  */

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

#line 345 "ldgram.h" /* yacc.c:1915  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_LDGRAM_H_INCLUDED  */
