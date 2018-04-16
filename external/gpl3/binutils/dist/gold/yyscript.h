/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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

#ifndef YY_YY_YYSCRIPT_H_INCLUDED
# define YY_YY_YYSCRIPT_H_INCLUDED
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
    PLUSEQ = 258,
    MINUSEQ = 259,
    MULTEQ = 260,
    DIVEQ = 261,
    LSHIFTEQ = 262,
    RSHIFTEQ = 263,
    ANDEQ = 264,
    OREQ = 265,
    OROR = 266,
    ANDAND = 267,
    EQ = 268,
    NE = 269,
    LE = 270,
    GE = 271,
    LSHIFT = 272,
    RSHIFT = 273,
    UNARY = 274,
    STRING = 275,
    QUOTED_STRING = 276,
    INTEGER = 277,
    ABSOLUTE = 278,
    ADDR = 279,
    ALIGN_K = 280,
    ALIGNOF = 281,
    ASSERT_K = 282,
    AS_NEEDED = 283,
    AT = 284,
    BIND = 285,
    BLOCK = 286,
    BYTE = 287,
    CONSTANT = 288,
    CONSTRUCTORS = 289,
    COPY = 290,
    CREATE_OBJECT_SYMBOLS = 291,
    DATA_SEGMENT_ALIGN = 292,
    DATA_SEGMENT_END = 293,
    DATA_SEGMENT_RELRO_END = 294,
    DEFINED = 295,
    DSECT = 296,
    ENTRY = 297,
    EXCLUDE_FILE = 298,
    EXTERN = 299,
    FILL = 300,
    FLOAT = 301,
    FORCE_COMMON_ALLOCATION = 302,
    GLOBAL = 303,
    GROUP = 304,
    HIDDEN = 305,
    HLL = 306,
    INCLUDE = 307,
    INHIBIT_COMMON_ALLOCATION = 308,
    INFO = 309,
    INPUT = 310,
    KEEP = 311,
    LEN = 312,
    LENGTH = 313,
    LOADADDR = 314,
    LOCAL = 315,
    LONG = 316,
    MAP = 317,
    MAX_K = 318,
    MEMORY = 319,
    MIN_K = 320,
    NEXT = 321,
    NOCROSSREFS = 322,
    NOFLOAT = 323,
    NOLOAD = 324,
    ONLY_IF_RO = 325,
    ONLY_IF_RW = 326,
    ORG = 327,
    ORIGIN = 328,
    OUTPUT = 329,
    OUTPUT_ARCH = 330,
    OUTPUT_FORMAT = 331,
    OVERLAY = 332,
    PHDRS = 333,
    PROVIDE = 334,
    PROVIDE_HIDDEN = 335,
    QUAD = 336,
    SEARCH_DIR = 337,
    SECTIONS = 338,
    SEGMENT_START = 339,
    SHORT = 340,
    SIZEOF = 341,
    SIZEOF_HEADERS = 342,
    SORT_BY_ALIGNMENT = 343,
    SORT_BY_INIT_PRIORITY = 344,
    SORT_BY_NAME = 345,
    SPECIAL = 346,
    SQUAD = 347,
    STARTUP = 348,
    SUBALIGN = 349,
    SYSLIB = 350,
    TARGET_K = 351,
    TRUNCATE = 352,
    VERSIONK = 353,
    OPTION = 354,
    PARSING_LINKER_SCRIPT = 355,
    PARSING_VERSION_SCRIPT = 356,
    PARSING_DEFSYM = 357,
    PARSING_DYNAMIC_LIST = 358,
    PARSING_SECTIONS_BLOCK = 359,
    PARSING_SECTION_COMMANDS = 360,
    PARSING_MEMORY_DEF = 361
  };
#endif
/* Tokens.  */
#define PLUSEQ 258
#define MINUSEQ 259
#define MULTEQ 260
#define DIVEQ 261
#define LSHIFTEQ 262
#define RSHIFTEQ 263
#define ANDEQ 264
#define OREQ 265
#define OROR 266
#define ANDAND 267
#define EQ 268
#define NE 269
#define LE 270
#define GE 271
#define LSHIFT 272
#define RSHIFT 273
#define UNARY 274
#define STRING 275
#define QUOTED_STRING 276
#define INTEGER 277
#define ABSOLUTE 278
#define ADDR 279
#define ALIGN_K 280
#define ALIGNOF 281
#define ASSERT_K 282
#define AS_NEEDED 283
#define AT 284
#define BIND 285
#define BLOCK 286
#define BYTE 287
#define CONSTANT 288
#define CONSTRUCTORS 289
#define COPY 290
#define CREATE_OBJECT_SYMBOLS 291
#define DATA_SEGMENT_ALIGN 292
#define DATA_SEGMENT_END 293
#define DATA_SEGMENT_RELRO_END 294
#define DEFINED 295
#define DSECT 296
#define ENTRY 297
#define EXCLUDE_FILE 298
#define EXTERN 299
#define FILL 300
#define FLOAT 301
#define FORCE_COMMON_ALLOCATION 302
#define GLOBAL 303
#define GROUP 304
#define HIDDEN 305
#define HLL 306
#define INCLUDE 307
#define INHIBIT_COMMON_ALLOCATION 308
#define INFO 309
#define INPUT 310
#define KEEP 311
#define LEN 312
#define LENGTH 313
#define LOADADDR 314
#define LOCAL 315
#define LONG 316
#define MAP 317
#define MAX_K 318
#define MEMORY 319
#define MIN_K 320
#define NEXT 321
#define NOCROSSREFS 322
#define NOFLOAT 323
#define NOLOAD 324
#define ONLY_IF_RO 325
#define ONLY_IF_RW 326
#define ORG 327
#define ORIGIN 328
#define OUTPUT 329
#define OUTPUT_ARCH 330
#define OUTPUT_FORMAT 331
#define OVERLAY 332
#define PHDRS 333
#define PROVIDE 334
#define PROVIDE_HIDDEN 335
#define QUAD 336
#define SEARCH_DIR 337
#define SECTIONS 338
#define SEGMENT_START 339
#define SHORT 340
#define SIZEOF 341
#define SIZEOF_HEADERS 342
#define SORT_BY_ALIGNMENT 343
#define SORT_BY_INIT_PRIORITY 344
#define SORT_BY_NAME 345
#define SPECIAL 346
#define SQUAD 347
#define STARTUP 348
#define SUBALIGN 349
#define SYSLIB 350
#define TARGET_K 351
#define TRUNCATE 352
#define VERSIONK 353
#define OPTION 354
#define PARSING_LINKER_SCRIPT 355
#define PARSING_VERSION_SCRIPT 356
#define PARSING_DEFSYM 357
#define PARSING_DYNAMIC_LIST 358
#define PARSING_SECTIONS_BLOCK 359
#define PARSING_SECTION_COMMANDS 360
#define PARSING_MEMORY_DEF 361

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 53 "yyscript.y" /* yacc.c:1909  */

  /* A string.  */
  struct Parser_string string;
  /* A number.  */
  uint64_t integer;
  /* An expression.  */
  Expression_ptr expr;
  /* An output section header.  */
  struct Parser_output_section_header output_section_header;
  /* An output section trailer.  */
  struct Parser_output_section_trailer output_section_trailer;
  /* A section constraint.  */
  enum Section_constraint constraint;
  /* A complete input section specification.  */
  struct Input_section_spec input_section_spec;
  /* A list of wildcard specifications, with exclusions.  */
  struct Wildcard_sections wildcard_sections;
  /* A single wildcard specification.  */
  struct Wildcard_section wildcard_section;
  /* A list of strings.  */
  String_list_ptr string_list;
  /* Information for a program header.  */
  struct Phdr_info phdr_info;
  /* Used for version scripts and within VERSION {}.  */
  struct Version_dependency_list* deplist;
  struct Version_expression_list* versyms;
  struct Version_tree* versnode;
  enum Script_section_type section_type;

#line 296 "yyscript.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (void* closure);

#endif /* !YY_YY_YYSCRIPT_H_INCLUDED  */
