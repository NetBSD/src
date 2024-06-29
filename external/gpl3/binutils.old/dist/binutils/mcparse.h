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

#ifndef YY_YY_MCPARSE_H_INCLUDED
# define YY_YY_MCPARSE_H_INCLUDED
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
    NL = 258,                      /* NL  */
    MCIDENT = 259,                 /* MCIDENT  */
    MCFILENAME = 260,              /* MCFILENAME  */
    MCLINE = 261,                  /* MCLINE  */
    MCCOMMENT = 262,               /* MCCOMMENT  */
    MCTOKEN = 263,                 /* MCTOKEN  */
    MCENDLINE = 264,               /* MCENDLINE  */
    MCLANGUAGENAMES = 265,         /* MCLANGUAGENAMES  */
    MCFACILITYNAMES = 266,         /* MCFACILITYNAMES  */
    MCSEVERITYNAMES = 267,         /* MCSEVERITYNAMES  */
    MCOUTPUTBASE = 268,            /* MCOUTPUTBASE  */
    MCMESSAGEIDTYPEDEF = 269,      /* MCMESSAGEIDTYPEDEF  */
    MCLANGUAGE = 270,              /* MCLANGUAGE  */
    MCMESSAGEID = 271,             /* MCMESSAGEID  */
    MCSEVERITY = 272,              /* MCSEVERITY  */
    MCFACILITY = 273,              /* MCFACILITY  */
    MCSYMBOLICNAME = 274,          /* MCSYMBOLICNAME  */
    MCNUMBER = 275                 /* MCNUMBER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 44 "mcparse.y"

  rc_uint_type ival;
  unichar *ustr;
  const mc_keyword *tok;
  mc_node *nod;

#line 114 "mcparse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MCPARSE_H_INCLUDED  */
