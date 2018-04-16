/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INT = 258,
     NAME = 259,
     LNAME = 260,
     OREQ = 261,
     ANDEQ = 262,
     RSHIFTEQ = 263,
     LSHIFTEQ = 264,
     DIVEQ = 265,
     MULTEQ = 266,
     MINUSEQ = 267,
     PLUSEQ = 268,
     OROR = 269,
     ANDAND = 270,
     NE = 271,
     EQ = 272,
     GE = 273,
     LE = 274,
     RSHIFT = 275,
     LSHIFT = 276,
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
     NOCROSSREFS_TO = 332,
     ORIGIN = 333,
     FILL = 334,
     LENGTH = 335,
     CREATE_OBJECT_SYMBOLS = 336,
     INPUT = 337,
     GROUP = 338,
     OUTPUT = 339,
     CONSTRUCTORS = 340,
     ALIGNMOD = 341,
     AT = 342,
     SUBALIGN = 343,
     HIDDEN = 344,
     PROVIDE = 345,
     PROVIDE_HIDDEN = 346,
     AS_NEEDED = 347,
     CHIP = 348,
     LIST = 349,
     SECT = 350,
     ABSOLUTE = 351,
     LOAD = 352,
     NEWLINE = 353,
     ENDWORD = 354,
     ORDER = 355,
     NAMEWORD = 356,
     ASSERT_K = 357,
     LOG2CEIL = 358,
     FORMAT = 359,
     PUBLIC = 360,
     DEFSYMEND = 361,
     BASE = 362,
     ALIAS = 363,
     TRUNCATE = 364,
     REL = 365,
     INPUT_SCRIPT = 366,
     INPUT_MRI_SCRIPT = 367,
     INPUT_DEFSYM = 368,
     CASE = 369,
     EXTERN = 370,
     START = 371,
     VERS_TAG = 372,
     VERS_IDENTIFIER = 373,
     GLOBAL = 374,
     LOCAL = 375,
     VERSIONK = 376,
     INPUT_VERSION_SCRIPT = 377,
     KEEP = 378,
     ONLY_IF_RO = 379,
     ONLY_IF_RW = 380,
     SPECIAL = 381,
     INPUT_SECTION_FLAGS = 382,
     ALIGN_WITH_INPUT = 383,
     EXCLUDE_FILE = 384,
     CONSTANT = 385,
     INPUT_DYNAMIC_LIST = 386
   };
#endif
/* Tokens.  */
#define INT 258
#define NAME 259
#define LNAME 260
#define OREQ 261
#define ANDEQ 262
#define RSHIFTEQ 263
#define LSHIFTEQ 264
#define DIVEQ 265
#define MULTEQ 266
#define MINUSEQ 267
#define PLUSEQ 268
#define OROR 269
#define ANDAND 270
#define NE 271
#define EQ 272
#define GE 273
#define LE 274
#define RSHIFT 275
#define LSHIFT 276
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
#define NOCROSSREFS_TO 332
#define ORIGIN 333
#define FILL 334
#define LENGTH 335
#define CREATE_OBJECT_SYMBOLS 336
#define INPUT 337
#define GROUP 338
#define OUTPUT 339
#define CONSTRUCTORS 340
#define ALIGNMOD 341
#define AT 342
#define SUBALIGN 343
#define HIDDEN 344
#define PROVIDE 345
#define PROVIDE_HIDDEN 346
#define AS_NEEDED 347
#define CHIP 348
#define LIST 349
#define SECT 350
#define ABSOLUTE 351
#define LOAD 352
#define NEWLINE 353
#define ENDWORD 354
#define ORDER 355
#define NAMEWORD 356
#define ASSERT_K 357
#define LOG2CEIL 358
#define FORMAT 359
#define PUBLIC 360
#define DEFSYMEND 361
#define BASE 362
#define ALIAS 363
#define TRUNCATE 364
#define REL 365
#define INPUT_SCRIPT 366
#define INPUT_MRI_SCRIPT 367
#define INPUT_DEFSYM 368
#define CASE 369
#define EXTERN 370
#define START 371
#define VERS_TAG 372
#define VERS_IDENTIFIER 373
#define GLOBAL 374
#define LOCAL 375
#define VERSIONK 376
#define INPUT_VERSION_SCRIPT 377
#define KEEP 378
#define ONLY_IF_RO 379
#define ONLY_IF_RW 380
#define SPECIAL 381
#define INPUT_SECTION_FLAGS 382
#define ALIGN_WITH_INPUT 383
#define EXCLUDE_FILE 384
#define CONSTANT 385
#define INPUT_DYNAMIC_LIST 386




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 60 "ldgram.y"
{
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
}
/* Line 1529 of yacc.c.  */
#line 342 "ldgram.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

