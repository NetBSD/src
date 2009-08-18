/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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
     NL = 258,
     MCIDENT = 259,
     MCFILENAME = 260,
     MCLINE = 261,
     MCCOMMENT = 262,
     MCTOKEN = 263,
     MCENDLINE = 264,
     MCLANGUAGENAMES = 265,
     MCFACILITYNAMES = 266,
     MCSEVERITYNAMES = 267,
     MCOUTPUTBASE = 268,
     MCMESSAGEIDTYPEDEF = 269,
     MCLANGUAGE = 270,
     MCMESSAGEID = 271,
     MCSEVERITY = 272,
     MCFACILITY = 273,
     MCSYMBOLICNAME = 274,
     MCNUMBER = 275
   };
#endif
/* Tokens.  */
#define NL 258
#define MCIDENT 259
#define MCFILENAME 260
#define MCLINE 261
#define MCCOMMENT 262
#define MCTOKEN 263
#define MCENDLINE 264
#define MCLANGUAGENAMES 265
#define MCFACILITYNAMES 266
#define MCSEVERITYNAMES 267
#define MCOUTPUTBASE 268
#define MCMESSAGEIDTYPEDEF 269
#define MCLANGUAGE 270
#define MCMESSAGEID 271
#define MCSEVERITY 272
#define MCFACILITY 273
#define MCSYMBOLICNAME 274
#define MCNUMBER 275




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 45 "mcparse.y"
typedef union YYSTYPE {
  rc_uint_type ival;
  unichar *ustr;
  const mc_keyword *tok;
  mc_node *nod;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 85 "mcparse.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



