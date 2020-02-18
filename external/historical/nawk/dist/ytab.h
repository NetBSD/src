/* A Bison parser, made by GNU Bison 3.4.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_AWKGRAM_TAB_H_INCLUDED
# define YY_YY_AWKGRAM_TAB_H_INCLUDED
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
    FIRSTTOKEN = 258,
    PROGRAM = 259,
    PASTAT = 260,
    PASTAT2 = 261,
    XBEGIN = 262,
    XEND = 263,
    NL = 264,
    ARRAY = 265,
    MATCH = 266,
    NOTMATCH = 267,
    MATCHOP = 268,
    FINAL = 269,
    DOT = 270,
    ALL = 271,
    CCL = 272,
    NCCL = 273,
    CHAR = 274,
    OR = 275,
    STAR = 276,
    QUEST = 277,
    PLUS = 278,
    EMPTYRE = 279,
    ZERO = 280,
    AND = 281,
    BOR = 282,
    APPEND = 283,
    EQ = 284,
    GE = 285,
    GT = 286,
    LE = 287,
    LT = 288,
    NE = 289,
    IN = 290,
    ARG = 291,
    BLTIN = 292,
    BREAK = 293,
    CLOSE = 294,
    CONTINUE = 295,
    DELETE = 296,
    DO = 297,
    EXIT = 298,
    FOR = 299,
    FUNC = 300,
    GENSUB = 301,
    SUB = 302,
    GSUB = 303,
    IF = 304,
    INDEX = 305,
    LSUBSTR = 306,
    MATCHFCN = 307,
    NEXT = 308,
    NEXTFILE = 309,
    ADD = 310,
    MINUS = 311,
    MULT = 312,
    DIVIDE = 313,
    MOD = 314,
    ASSIGN = 315,
    ASGNOP = 316,
    ADDEQ = 317,
    SUBEQ = 318,
    MULTEQ = 319,
    DIVEQ = 320,
    MODEQ = 321,
    POWEQ = 322,
    PRINT = 323,
    PRINTF = 324,
    SPRINTF = 325,
    ELSE = 326,
    INTEST = 327,
    CONDEXPR = 328,
    POSTINCR = 329,
    PREINCR = 330,
    POSTDECR = 331,
    PREDECR = 332,
    VAR = 333,
    IVAR = 334,
    VARNF = 335,
    CALL = 336,
    NUMBER = 337,
    STRING = 338,
    REGEXPR = 339,
    GETLINE = 340,
    RETURN = 341,
    SPLIT = 342,
    SUBSTR = 343,
    WHILE = 344,
    CAT = 345,
    NOT = 346,
    UMINUS = 347,
    UPLUS = 348,
    POWER = 349,
    DECR = 350,
    INCR = 351,
    INDIRECT = 352,
    LASTTOKEN = 353
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 45 "awkgram.y"

	Node	*p;
	Cell	*cp;
	int	i;
	char	*s;

#line 163 "awkgram.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_AWKGRAM_TAB_H_INCLUDED  */
