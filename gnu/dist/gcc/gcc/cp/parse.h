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
     IDENTIFIER = 258,
     tTYPENAME = 259,
     SELFNAME = 260,
     PFUNCNAME = 261,
     SCSPEC = 262,
     TYPESPEC = 263,
     CV_QUALIFIER = 264,
     CONSTANT = 265,
     VAR_FUNC_NAME = 266,
     STRING = 267,
     ELLIPSIS = 268,
     SIZEOF = 269,
     ENUM = 270,
     IF = 271,
     ELSE = 272,
     WHILE = 273,
     DO = 274,
     FOR = 275,
     SWITCH = 276,
     CASE = 277,
     DEFAULT = 278,
     BREAK = 279,
     CONTINUE = 280,
     RETURN_KEYWORD = 281,
     GOTO = 282,
     ASM_KEYWORD = 283,
     TYPEOF = 284,
     ALIGNOF = 285,
     SIGOF = 286,
     ATTRIBUTE = 287,
     EXTENSION = 288,
     LABEL = 289,
     REALPART = 290,
     IMAGPART = 291,
     VA_ARG = 292,
     AGGR = 293,
     VISSPEC = 294,
     DELETE = 295,
     NEW = 296,
     THIS = 297,
     OPERATOR = 298,
     CXX_TRUE = 299,
     CXX_FALSE = 300,
     NAMESPACE = 301,
     TYPENAME_KEYWORD = 302,
     USING = 303,
     LEFT_RIGHT = 304,
     TEMPLATE = 305,
     TYPEID = 306,
     DYNAMIC_CAST = 307,
     STATIC_CAST = 308,
     REINTERPRET_CAST = 309,
     CONST_CAST = 310,
     SCOPE = 311,
     EXPORT = 312,
     EMPTY = 313,
     NSNAME = 314,
     PTYPENAME = 315,
     THROW = 316,
     ASSIGN = 317,
     OROR = 318,
     ANDAND = 319,
     MIN_MAX = 320,
     EQCOMPARE = 321,
     ARITHCOMPARE = 322,
     RSHIFT = 323,
     LSHIFT = 324,
     DOT_STAR = 325,
     POINTSAT_STAR = 326,
     MINUSMINUS = 327,
     PLUSPLUS = 328,
     UNARY = 329,
     HYPERUNARY = 330,
     POINTSAT = 331,
     CATCH = 332,
     TRY = 333,
     EXTERN_LANG_STRING = 334,
     ALL = 335,
     PRE_PARSED_CLASS_DECL = 336,
     DEFARG = 337,
     DEFARG_MARKER = 338,
     PRE_PARSED_FUNCTION_DECL = 339,
     TYPENAME_DEFN = 340,
     IDENTIFIER_DEFN = 341,
     PTYPENAME_DEFN = 342,
     END_OF_LINE = 343,
     END_OF_SAVED_INPUT = 344
   };
#endif
#define IDENTIFIER 258
#define tTYPENAME 259
#define SELFNAME 260
#define PFUNCNAME 261
#define SCSPEC 262
#define TYPESPEC 263
#define CV_QUALIFIER 264
#define CONSTANT 265
#define VAR_FUNC_NAME 266
#define STRING 267
#define ELLIPSIS 268
#define SIZEOF 269
#define ENUM 270
#define IF 271
#define ELSE 272
#define WHILE 273
#define DO 274
#define FOR 275
#define SWITCH 276
#define CASE 277
#define DEFAULT 278
#define BREAK 279
#define CONTINUE 280
#define RETURN_KEYWORD 281
#define GOTO 282
#define ASM_KEYWORD 283
#define TYPEOF 284
#define ALIGNOF 285
#define SIGOF 286
#define ATTRIBUTE 287
#define EXTENSION 288
#define LABEL 289
#define REALPART 290
#define IMAGPART 291
#define VA_ARG 292
#define AGGR 293
#define VISSPEC 294
#define DELETE 295
#define NEW 296
#define THIS 297
#define OPERATOR 298
#define CXX_TRUE 299
#define CXX_FALSE 300
#define NAMESPACE 301
#define TYPENAME_KEYWORD 302
#define USING 303
#define LEFT_RIGHT 304
#define TEMPLATE 305
#define TYPEID 306
#define DYNAMIC_CAST 307
#define STATIC_CAST 308
#define REINTERPRET_CAST 309
#define CONST_CAST 310
#define SCOPE 311
#define EXPORT 312
#define EMPTY 313
#define NSNAME 314
#define PTYPENAME 315
#define THROW 316
#define ASSIGN 317
#define OROR 318
#define ANDAND 319
#define MIN_MAX 320
#define EQCOMPARE 321
#define ARITHCOMPARE 322
#define RSHIFT 323
#define LSHIFT 324
#define DOT_STAR 325
#define POINTSAT_STAR 326
#define MINUSMINUS 327
#define PLUSPLUS 328
#define UNARY 329
#define HYPERUNARY 330
#define POINTSAT 331
#define CATCH 332
#define TRY 333
#define EXTERN_LANG_STRING 334
#define ALL 335
#define PRE_PARSED_CLASS_DECL 336
#define DEFARG 337
#define DEFARG_MARKER 338
#define PRE_PARSED_FUNCTION_DECL 339
#define TYPENAME_DEFN 340
#define IDENTIFIER_DEFN 341
#define PTYPENAME_DEFN 342
#define END_OF_LINE 343
#define END_OF_SAVED_INPUT 344




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 271 "parse.y"
typedef union YYSTYPE { GTY(())
  long itype;
  tree ttype;
  char *strtype;
  enum tree_code code;
  flagged_type_tree ftype;
  struct unparsed_text *pi;
} YYSTYPE;
/* Line 1249 of yacc.c.  */
#line 223 "p19357.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



#define YYEMPTY		(-2)
