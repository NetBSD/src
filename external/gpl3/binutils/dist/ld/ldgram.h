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
     SIZEOF_HEADERS = 297,
     OUTPUT_FORMAT = 298,
     FORCE_COMMON_ALLOCATION = 299,
     OUTPUT_ARCH = 300,
     INHIBIT_COMMON_ALLOCATION = 301,
     SEGMENT_START = 302,
     INCLUDE = 303,
     MEMORY = 304,
     REGION_ALIAS = 305,
     LD_FEATURE = 306,
     NOLOAD = 307,
     DSECT = 308,
     COPY = 309,
     INFO = 310,
     OVERLAY = 311,
     DEFINED = 312,
     TARGET_K = 313,
     SEARCH_DIR = 314,
     MAP = 315,
     ENTRY = 316,
     NEXT = 317,
     SIZEOF = 318,
     ALIGNOF = 319,
     ADDR = 320,
     LOADADDR = 321,
     MAX_K = 322,
     MIN_K = 323,
     STARTUP = 324,
     HLL = 325,
     SYSLIB = 326,
     FLOAT = 327,
     NOFLOAT = 328,
     NOCROSSREFS = 329,
     ORIGIN = 330,
     FILL = 331,
     LENGTH = 332,
     CREATE_OBJECT_SYMBOLS = 333,
     INPUT = 334,
     GROUP = 335,
     OUTPUT = 336,
     CONSTRUCTORS = 337,
     ALIGNMOD = 338,
     AT = 339,
     SUBALIGN = 340,
     PROVIDE = 341,
     PROVIDE_HIDDEN = 342,
     AS_NEEDED = 343,
     CHIP = 344,
     LIST = 345,
     SECT = 346,
     ABSOLUTE = 347,
     LOAD = 348,
     NEWLINE = 349,
     ENDWORD = 350,
     ORDER = 351,
     NAMEWORD = 352,
     ASSERT_K = 353,
     FORMAT = 354,
     PUBLIC = 355,
     DEFSYMEND = 356,
     BASE = 357,
     ALIAS = 358,
     TRUNCATE = 359,
     REL = 360,
     INPUT_SCRIPT = 361,
     INPUT_MRI_SCRIPT = 362,
     INPUT_DEFSYM = 363,
     CASE = 364,
     EXTERN = 365,
     START = 366,
     VERS_TAG = 367,
     VERS_IDENTIFIER = 368,
     GLOBAL = 369,
     LOCAL = 370,
     VERSIONK = 371,
     INPUT_VERSION_SCRIPT = 372,
     KEEP = 373,
     ONLY_IF_RO = 374,
     ONLY_IF_RW = 375,
     SPECIAL = 376,
     EXCLUDE_FILE = 377,
     CONSTANT = 378,
     INPUT_DYNAMIC_LIST = 379
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
#define SIZEOF_HEADERS 297
#define OUTPUT_FORMAT 298
#define FORCE_COMMON_ALLOCATION 299
#define OUTPUT_ARCH 300
#define INHIBIT_COMMON_ALLOCATION 301
#define SEGMENT_START 302
#define INCLUDE 303
#define MEMORY 304
#define REGION_ALIAS 305
#define LD_FEATURE 306
#define NOLOAD 307
#define DSECT 308
#define COPY 309
#define INFO 310
#define OVERLAY 311
#define DEFINED 312
#define TARGET_K 313
#define SEARCH_DIR 314
#define MAP 315
#define ENTRY 316
#define NEXT 317
#define SIZEOF 318
#define ALIGNOF 319
#define ADDR 320
#define LOADADDR 321
#define MAX_K 322
#define MIN_K 323
#define STARTUP 324
#define HLL 325
#define SYSLIB 326
#define FLOAT 327
#define NOFLOAT 328
#define NOCROSSREFS 329
#define ORIGIN 330
#define FILL 331
#define LENGTH 332
#define CREATE_OBJECT_SYMBOLS 333
#define INPUT 334
#define GROUP 335
#define OUTPUT 336
#define CONSTRUCTORS 337
#define ALIGNMOD 338
#define AT 339
#define SUBALIGN 340
#define PROVIDE 341
#define PROVIDE_HIDDEN 342
#define AS_NEEDED 343
#define CHIP 344
#define LIST 345
#define SECT 346
#define ABSOLUTE 347
#define LOAD 348
#define NEWLINE 349
#define ENDWORD 350
#define ORDER 351
#define NAMEWORD 352
#define ASSERT_K 353
#define FORMAT 354
#define PUBLIC 355
#define DEFSYMEND 356
#define BASE 357
#define ALIAS 358
#define TRUNCATE 359
#define REL 360
#define INPUT_SCRIPT 361
#define INPUT_MRI_SCRIPT 362
#define INPUT_DEFSYM 363
#define CASE 364
#define EXTERN 365
#define START 366
#define VERS_TAG 367
#define VERS_IDENTIFIER 368
#define GLOBAL 369
#define LOCAL 370
#define VERSIONK 371
#define INPUT_VERSION_SCRIPT 372
#define KEEP 373
#define ONLY_IF_RO 374
#define ONLY_IF_RW 375
#define SPECIAL 376
#define EXCLUDE_FILE 377
#define CONSTANT 378
#define INPUT_DYNAMIC_LIST 379




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 62 "ldgram.y"
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
#line 326 "ldgram.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

