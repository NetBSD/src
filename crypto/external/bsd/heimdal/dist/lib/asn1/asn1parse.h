/*	$NetBSD: asn1parse.h,v 1.3 2023/06/19 21:41:42 christos Exp $	*/

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

#ifndef YY_YY_ASN_PARSE_H_INCLUDED
# define YY_YY_ASN_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
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
    kw_ABSENT = 258,               /* kw_ABSENT  */
    kw_ABSTRACT_SYNTAX = 259,      /* kw_ABSTRACT_SYNTAX  */
    kw_ALL = 260,                  /* kw_ALL  */
    kw_APPLICATION = 261,          /* kw_APPLICATION  */
    kw_AUTOMATIC = 262,            /* kw_AUTOMATIC  */
    kw_BEGIN = 263,                /* kw_BEGIN  */
    kw_BIT = 264,                  /* kw_BIT  */
    kw_BMPString = 265,            /* kw_BMPString  */
    kw_BOOLEAN = 266,              /* kw_BOOLEAN  */
    kw_BY = 267,                   /* kw_BY  */
    kw_CHARACTER = 268,            /* kw_CHARACTER  */
    kw_CHOICE = 269,               /* kw_CHOICE  */
    kw_CLASS = 270,                /* kw_CLASS  */
    kw_COMPONENT = 271,            /* kw_COMPONENT  */
    kw_COMPONENTS = 272,           /* kw_COMPONENTS  */
    kw_CONSTRAINED = 273,          /* kw_CONSTRAINED  */
    kw_CONTAINING = 274,           /* kw_CONTAINING  */
    kw_DEFAULT = 275,              /* kw_DEFAULT  */
    kw_DEFINITIONS = 276,          /* kw_DEFINITIONS  */
    kw_EMBEDDED = 277,             /* kw_EMBEDDED  */
    kw_ENCODED = 278,              /* kw_ENCODED  */
    kw_END = 279,                  /* kw_END  */
    kw_ENUMERATED = 280,           /* kw_ENUMERATED  */
    kw_EXCEPT = 281,               /* kw_EXCEPT  */
    kw_EXPLICIT = 282,             /* kw_EXPLICIT  */
    kw_EXPORTS = 283,              /* kw_EXPORTS  */
    kw_EXTENSIBILITY = 284,        /* kw_EXTENSIBILITY  */
    kw_EXTERNAL = 285,             /* kw_EXTERNAL  */
    kw_FALSE = 286,                /* kw_FALSE  */
    kw_FROM = 287,                 /* kw_FROM  */
    kw_GeneralString = 288,        /* kw_GeneralString  */
    kw_GeneralizedTime = 289,      /* kw_GeneralizedTime  */
    kw_GraphicString = 290,        /* kw_GraphicString  */
    kw_IA5String = 291,            /* kw_IA5String  */
    kw_IDENTIFIER = 292,           /* kw_IDENTIFIER  */
    kw_IMPLICIT = 293,             /* kw_IMPLICIT  */
    kw_IMPLIED = 294,              /* kw_IMPLIED  */
    kw_IMPORTS = 295,              /* kw_IMPORTS  */
    kw_INCLUDES = 296,             /* kw_INCLUDES  */
    kw_INSTANCE = 297,             /* kw_INSTANCE  */
    kw_INTEGER = 298,              /* kw_INTEGER  */
    kw_INTERSECTION = 299,         /* kw_INTERSECTION  */
    kw_ISO646String = 300,         /* kw_ISO646String  */
    kw_MAX = 301,                  /* kw_MAX  */
    kw_MIN = 302,                  /* kw_MIN  */
    kw_MINUS_INFINITY = 303,       /* kw_MINUS_INFINITY  */
    kw_NULL = 304,                 /* kw_NULL  */
    kw_NumericString = 305,        /* kw_NumericString  */
    kw_OBJECT = 306,               /* kw_OBJECT  */
    kw_OCTET = 307,                /* kw_OCTET  */
    kw_OF = 308,                   /* kw_OF  */
    kw_OPTIONAL = 309,             /* kw_OPTIONAL  */
    kw_ObjectDescriptor = 310,     /* kw_ObjectDescriptor  */
    kw_PATTERN = 311,              /* kw_PATTERN  */
    kw_PDV = 312,                  /* kw_PDV  */
    kw_PLUS_INFINITY = 313,        /* kw_PLUS_INFINITY  */
    kw_PRESENT = 314,              /* kw_PRESENT  */
    kw_PRIVATE = 315,              /* kw_PRIVATE  */
    kw_PrintableString = 316,      /* kw_PrintableString  */
    kw_REAL = 317,                 /* kw_REAL  */
    kw_RELATIVE_OID = 318,         /* kw_RELATIVE_OID  */
    kw_SEQUENCE = 319,             /* kw_SEQUENCE  */
    kw_SET = 320,                  /* kw_SET  */
    kw_SIZE = 321,                 /* kw_SIZE  */
    kw_STRING = 322,               /* kw_STRING  */
    kw_SYNTAX = 323,               /* kw_SYNTAX  */
    kw_T61String = 324,            /* kw_T61String  */
    kw_TAGS = 325,                 /* kw_TAGS  */
    kw_TRUE = 326,                 /* kw_TRUE  */
    kw_TYPE_IDENTIFIER = 327,      /* kw_TYPE_IDENTIFIER  */
    kw_TeletexString = 328,        /* kw_TeletexString  */
    kw_UNION = 329,                /* kw_UNION  */
    kw_UNIQUE = 330,               /* kw_UNIQUE  */
    kw_UNIVERSAL = 331,            /* kw_UNIVERSAL  */
    kw_UTCTime = 332,              /* kw_UTCTime  */
    kw_UTF8String = 333,           /* kw_UTF8String  */
    kw_UniversalString = 334,      /* kw_UniversalString  */
    kw_VideotexString = 335,       /* kw_VideotexString  */
    kw_VisibleString = 336,        /* kw_VisibleString  */
    kw_WITH = 337,                 /* kw_WITH  */
    RANGE = 338,                   /* RANGE  */
    EEQUAL = 339,                  /* EEQUAL  */
    ELLIPSIS = 340,                /* ELLIPSIS  */
    IDENTIFIER = 341,              /* IDENTIFIER  */
    referencename = 342,           /* referencename  */
    STRING = 343,                  /* STRING  */
    NUMBER = 344                   /* NUMBER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define kw_ABSENT 258
#define kw_ABSTRACT_SYNTAX 259
#define kw_ALL 260
#define kw_APPLICATION 261
#define kw_AUTOMATIC 262
#define kw_BEGIN 263
#define kw_BIT 264
#define kw_BMPString 265
#define kw_BOOLEAN 266
#define kw_BY 267
#define kw_CHARACTER 268
#define kw_CHOICE 269
#define kw_CLASS 270
#define kw_COMPONENT 271
#define kw_COMPONENTS 272
#define kw_CONSTRAINED 273
#define kw_CONTAINING 274
#define kw_DEFAULT 275
#define kw_DEFINITIONS 276
#define kw_EMBEDDED 277
#define kw_ENCODED 278
#define kw_END 279
#define kw_ENUMERATED 280
#define kw_EXCEPT 281
#define kw_EXPLICIT 282
#define kw_EXPORTS 283
#define kw_EXTENSIBILITY 284
#define kw_EXTERNAL 285
#define kw_FALSE 286
#define kw_FROM 287
#define kw_GeneralString 288
#define kw_GeneralizedTime 289
#define kw_GraphicString 290
#define kw_IA5String 291
#define kw_IDENTIFIER 292
#define kw_IMPLICIT 293
#define kw_IMPLIED 294
#define kw_IMPORTS 295
#define kw_INCLUDES 296
#define kw_INSTANCE 297
#define kw_INTEGER 298
#define kw_INTERSECTION 299
#define kw_ISO646String 300
#define kw_MAX 301
#define kw_MIN 302
#define kw_MINUS_INFINITY 303
#define kw_NULL 304
#define kw_NumericString 305
#define kw_OBJECT 306
#define kw_OCTET 307
#define kw_OF 308
#define kw_OPTIONAL 309
#define kw_ObjectDescriptor 310
#define kw_PATTERN 311
#define kw_PDV 312
#define kw_PLUS_INFINITY 313
#define kw_PRESENT 314
#define kw_PRIVATE 315
#define kw_PrintableString 316
#define kw_REAL 317
#define kw_RELATIVE_OID 318
#define kw_SEQUENCE 319
#define kw_SET 320
#define kw_SIZE 321
#define kw_STRING 322
#define kw_SYNTAX 323
#define kw_T61String 324
#define kw_TAGS 325
#define kw_TRUE 326
#define kw_TYPE_IDENTIFIER 327
#define kw_TeletexString 328
#define kw_UNION 329
#define kw_UNIQUE 330
#define kw_UNIVERSAL 331
#define kw_UTCTime 332
#define kw_UTF8String 333
#define kw_UniversalString 334
#define kw_VideotexString 335
#define kw_VisibleString 336
#define kw_WITH 337
#define RANGE 338
#define EEQUAL 339
#define ELLIPSIS 340
#define IDENTIFIER 341
#define referencename 342
#define STRING 343
#define NUMBER 344

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 72 "asn1parse.y"

    int64_t constant;
    struct value *value;
    struct range *range;
    char *name;
    Type *type;
    Member *member;
    struct objid *objid;
    char *defval;
    struct string_list *sl;
    struct tagtype tag;
    struct memhead *members;
    struct constraint_spec *constraint_spec;

#line 260 "asn1parse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_ASN_PARSE_H_INCLUDED  */
