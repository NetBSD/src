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

#ifndef YY_YY_DEFFILEP_H_INCLUDED
# define YY_YY_DEFFILEP_H_INCLUDED
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
    NAME = 258,                    /* NAME  */
    LIBRARY = 259,                 /* LIBRARY  */
    DESCRIPTION = 260,             /* DESCRIPTION  */
    STACKSIZE_K = 261,             /* STACKSIZE_K  */
    HEAPSIZE = 262,                /* HEAPSIZE  */
    CODE = 263,                    /* CODE  */
    DATAU = 264,                   /* DATAU  */
    DATAL = 265,                   /* DATAL  */
    SECTIONS = 266,                /* SECTIONS  */
    EXPORTS = 267,                 /* EXPORTS  */
    IMPORTS = 268,                 /* IMPORTS  */
    VERSIONK = 269,                /* VERSIONK  */
    BASE = 270,                    /* BASE  */
    CONSTANTU = 271,               /* CONSTANTU  */
    CONSTANTL = 272,               /* CONSTANTL  */
    PRIVATEU = 273,                /* PRIVATEU  */
    PRIVATEL = 274,                /* PRIVATEL  */
    ALIGNCOMM = 275,               /* ALIGNCOMM  */
    READ = 276,                    /* READ  */
    WRITE = 277,                   /* WRITE  */
    EXECUTE = 278,                 /* EXECUTE  */
    SHARED_K = 279,                /* SHARED_K  */
    NONAMEU = 280,                 /* NONAMEU  */
    NONAMEL = 281,                 /* NONAMEL  */
    DIRECTIVE = 282,               /* DIRECTIVE  */
    EQUAL = 283,                   /* EQUAL  */
    ID = 284,                      /* ID  */
    DIGITS = 285                   /* DIGITS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define NAME 258
#define LIBRARY 259
#define DESCRIPTION 260
#define STACKSIZE_K 261
#define HEAPSIZE 262
#define CODE 263
#define DATAU 264
#define DATAL 265
#define SECTIONS 266
#define EXPORTS 267
#define IMPORTS 268
#define VERSIONK 269
#define BASE 270
#define CONSTANTU 271
#define CONSTANTL 272
#define PRIVATEU 273
#define PRIVATEL 274
#define ALIGNCOMM 275
#define READ 276
#define WRITE 277
#define EXECUTE 278
#define SHARED_K 279
#define NONAMEU 280
#define NONAMEL 281
#define DIRECTIVE 282
#define EQUAL 283
#define ID 284
#define DIGITS 285

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 114 "deffilep.y"

  char *id;
  const char *id_const;
  int number;
  bfd_vma vma;
  char *digits;

#line 135 "deffilep.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_DEFFILEP_H_INCLUDED  */
