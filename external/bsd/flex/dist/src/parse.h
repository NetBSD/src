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

#ifndef YY_YY_PARSE_H_INCLUDED
# define YY_YY_PARSE_H_INCLUDED
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
    CHAR = 258,
    NUMBER = 259,
    SECTEND = 260,
    SCDECL = 261,
    XSCDECL = 262,
    NAME = 263,
    PREVCCL = 264,
    EOF_OP = 265,
    OPTION_OP = 266,
    OPT_OUTFILE = 267,
    OPT_PREFIX = 268,
    OPT_YYCLASS = 269,
    OPT_HEADER = 270,
    OPT_EXTRA_TYPE = 271,
    OPT_TABLES = 272,
    CCE_ALNUM = 273,
    CCE_ALPHA = 274,
    CCE_BLANK = 275,
    CCE_CNTRL = 276,
    CCE_DIGIT = 277,
    CCE_GRAPH = 278,
    CCE_LOWER = 279,
    CCE_PRINT = 280,
    CCE_PUNCT = 281,
    CCE_SPACE = 282,
    CCE_UPPER = 283,
    CCE_XDIGIT = 284,
    CCE_NEG_ALNUM = 285,
    CCE_NEG_ALPHA = 286,
    CCE_NEG_BLANK = 287,
    CCE_NEG_CNTRL = 288,
    CCE_NEG_DIGIT = 289,
    CCE_NEG_GRAPH = 290,
    CCE_NEG_LOWER = 291,
    CCE_NEG_PRINT = 292,
    CCE_NEG_PUNCT = 293,
    CCE_NEG_SPACE = 294,
    CCE_NEG_UPPER = 295,
    CCE_NEG_XDIGIT = 296,
    CCL_OP_DIFF = 297,
    CCL_OP_UNION = 298,
    BEGIN_REPEAT_POSIX = 299,
    END_REPEAT_POSIX = 300,
    BEGIN_REPEAT_FLEX = 301,
    END_REPEAT_FLEX = 302
  };
#endif
/* Tokens.  */
#define CHAR 258
#define NUMBER 259
#define SECTEND 260
#define SCDECL 261
#define XSCDECL 262
#define NAME 263
#define PREVCCL 264
#define EOF_OP 265
#define OPTION_OP 266
#define OPT_OUTFILE 267
#define OPT_PREFIX 268
#define OPT_YYCLASS 269
#define OPT_HEADER 270
#define OPT_EXTRA_TYPE 271
#define OPT_TABLES 272
#define CCE_ALNUM 273
#define CCE_ALPHA 274
#define CCE_BLANK 275
#define CCE_CNTRL 276
#define CCE_DIGIT 277
#define CCE_GRAPH 278
#define CCE_LOWER 279
#define CCE_PRINT 280
#define CCE_PUNCT 281
#define CCE_SPACE 282
#define CCE_UPPER 283
#define CCE_XDIGIT 284
#define CCE_NEG_ALNUM 285
#define CCE_NEG_ALPHA 286
#define CCE_NEG_BLANK 287
#define CCE_NEG_CNTRL 288
#define CCE_NEG_DIGIT 289
#define CCE_NEG_GRAPH 290
#define CCE_NEG_LOWER 291
#define CCE_NEG_PRINT 292
#define CCE_NEG_PUNCT 293
#define CCE_NEG_SPACE 294
#define CCE_NEG_UPPER 295
#define CCE_NEG_XDIGIT 296
#define CCL_OP_DIFF 297
#define CCL_OP_UNION 298
#define BEGIN_REPEAT_POSIX 299
#define END_REPEAT_POSIX 300
#define BEGIN_REPEAT_FLEX 301
#define END_REPEAT_FLEX 302

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSE_H_INCLUDED  */
