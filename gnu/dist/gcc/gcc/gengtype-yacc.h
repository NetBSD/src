/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ENT_TYPEDEF_STRUCT = 258,
     ENT_STRUCT = 259,
     ENT_EXTERNSTATIC = 260,
     ENT_YACCUNION = 261,
     GTY_TOKEN = 262,
     UNION = 263,
     STRUCT = 264,
     ENUM = 265,
     ALIAS = 266,
     PARAM_IS = 267,
     NUM = 268,
     PERCENTPERCENT = 269,
     SCALAR = 270,
     ID = 271,
     STRING = 272,
     ARRAY = 273,
     PERCENT_ID = 274,
     CHAR = 275
   };
#endif
#define ENT_TYPEDEF_STRUCT 258
#define ENT_STRUCT 259
#define ENT_EXTERNSTATIC 260
#define ENT_YACCUNION 261
#define GTY_TOKEN 262
#define UNION 263
#define STRUCT 264
#define ENUM 265
#define ALIAS 266
#define PARAM_IS 267
#define NUM 268
#define PERCENTPERCENT 269
#define SCALAR 270
#define ID 271
#define STRING 272
#define ARRAY 273
#define PERCENT_ID 274
#define CHAR 275




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 29 "gengtype-yacc.y"
typedef union YYSTYPE {
  type_p t;
  pair_p p;
  options_p o;
  const char *s;
} YYSTYPE;
/* Line 1249 of yacc.c.  */
#line 83 "gengtype-yacc.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



