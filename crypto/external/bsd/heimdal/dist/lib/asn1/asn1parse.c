/*	$NetBSD: asn1parse.c,v 1.3 2023/06/19 21:41:42 christos Exp $	*/

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 38 "asn1parse.y"


#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "symbol.h"
#include "lex.h"
#include "gen_locl.h"
#include <krb5/der.h>

static Type *new_type (Typetype t);
static struct constraint_spec *new_constraint_spec(enum ctype);
static Type *new_tag(int tagclass, int tagvalue, int tagenv, Type *oldtype);
void yyerror (const char *);
static struct objid *new_objid(const char *label, int value);
static void add_oid_to_tail(struct objid *, struct objid *);
static void fix_labels(Symbol *s);

struct string_list {
    char *string;
    struct string_list *next;
};

static int default_tag_env = TE_EXPLICIT;

/* Declarations for Bison */
#define YYMALLOC malloc
#define YYFREE   free


#line 105 "asn1parse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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

#line 351 "asn1parse.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_ASN_PARSE_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_kw_ABSENT = 3,                  /* kw_ABSENT  */
  YYSYMBOL_kw_ABSTRACT_SYNTAX = 4,         /* kw_ABSTRACT_SYNTAX  */
  YYSYMBOL_kw_ALL = 5,                     /* kw_ALL  */
  YYSYMBOL_kw_APPLICATION = 6,             /* kw_APPLICATION  */
  YYSYMBOL_kw_AUTOMATIC = 7,               /* kw_AUTOMATIC  */
  YYSYMBOL_kw_BEGIN = 8,                   /* kw_BEGIN  */
  YYSYMBOL_kw_BIT = 9,                     /* kw_BIT  */
  YYSYMBOL_kw_BMPString = 10,              /* kw_BMPString  */
  YYSYMBOL_kw_BOOLEAN = 11,                /* kw_BOOLEAN  */
  YYSYMBOL_kw_BY = 12,                     /* kw_BY  */
  YYSYMBOL_kw_CHARACTER = 13,              /* kw_CHARACTER  */
  YYSYMBOL_kw_CHOICE = 14,                 /* kw_CHOICE  */
  YYSYMBOL_kw_CLASS = 15,                  /* kw_CLASS  */
  YYSYMBOL_kw_COMPONENT = 16,              /* kw_COMPONENT  */
  YYSYMBOL_kw_COMPONENTS = 17,             /* kw_COMPONENTS  */
  YYSYMBOL_kw_CONSTRAINED = 18,            /* kw_CONSTRAINED  */
  YYSYMBOL_kw_CONTAINING = 19,             /* kw_CONTAINING  */
  YYSYMBOL_kw_DEFAULT = 20,                /* kw_DEFAULT  */
  YYSYMBOL_kw_DEFINITIONS = 21,            /* kw_DEFINITIONS  */
  YYSYMBOL_kw_EMBEDDED = 22,               /* kw_EMBEDDED  */
  YYSYMBOL_kw_ENCODED = 23,                /* kw_ENCODED  */
  YYSYMBOL_kw_END = 24,                    /* kw_END  */
  YYSYMBOL_kw_ENUMERATED = 25,             /* kw_ENUMERATED  */
  YYSYMBOL_kw_EXCEPT = 26,                 /* kw_EXCEPT  */
  YYSYMBOL_kw_EXPLICIT = 27,               /* kw_EXPLICIT  */
  YYSYMBOL_kw_EXPORTS = 28,                /* kw_EXPORTS  */
  YYSYMBOL_kw_EXTENSIBILITY = 29,          /* kw_EXTENSIBILITY  */
  YYSYMBOL_kw_EXTERNAL = 30,               /* kw_EXTERNAL  */
  YYSYMBOL_kw_FALSE = 31,                  /* kw_FALSE  */
  YYSYMBOL_kw_FROM = 32,                   /* kw_FROM  */
  YYSYMBOL_kw_GeneralString = 33,          /* kw_GeneralString  */
  YYSYMBOL_kw_GeneralizedTime = 34,        /* kw_GeneralizedTime  */
  YYSYMBOL_kw_GraphicString = 35,          /* kw_GraphicString  */
  YYSYMBOL_kw_IA5String = 36,              /* kw_IA5String  */
  YYSYMBOL_kw_IDENTIFIER = 37,             /* kw_IDENTIFIER  */
  YYSYMBOL_kw_IMPLICIT = 38,               /* kw_IMPLICIT  */
  YYSYMBOL_kw_IMPLIED = 39,                /* kw_IMPLIED  */
  YYSYMBOL_kw_IMPORTS = 40,                /* kw_IMPORTS  */
  YYSYMBOL_kw_INCLUDES = 41,               /* kw_INCLUDES  */
  YYSYMBOL_kw_INSTANCE = 42,               /* kw_INSTANCE  */
  YYSYMBOL_kw_INTEGER = 43,                /* kw_INTEGER  */
  YYSYMBOL_kw_INTERSECTION = 44,           /* kw_INTERSECTION  */
  YYSYMBOL_kw_ISO646String = 45,           /* kw_ISO646String  */
  YYSYMBOL_kw_MAX = 46,                    /* kw_MAX  */
  YYSYMBOL_kw_MIN = 47,                    /* kw_MIN  */
  YYSYMBOL_kw_MINUS_INFINITY = 48,         /* kw_MINUS_INFINITY  */
  YYSYMBOL_kw_NULL = 49,                   /* kw_NULL  */
  YYSYMBOL_kw_NumericString = 50,          /* kw_NumericString  */
  YYSYMBOL_kw_OBJECT = 51,                 /* kw_OBJECT  */
  YYSYMBOL_kw_OCTET = 52,                  /* kw_OCTET  */
  YYSYMBOL_kw_OF = 53,                     /* kw_OF  */
  YYSYMBOL_kw_OPTIONAL = 54,               /* kw_OPTIONAL  */
  YYSYMBOL_kw_ObjectDescriptor = 55,       /* kw_ObjectDescriptor  */
  YYSYMBOL_kw_PATTERN = 56,                /* kw_PATTERN  */
  YYSYMBOL_kw_PDV = 57,                    /* kw_PDV  */
  YYSYMBOL_kw_PLUS_INFINITY = 58,          /* kw_PLUS_INFINITY  */
  YYSYMBOL_kw_PRESENT = 59,                /* kw_PRESENT  */
  YYSYMBOL_kw_PRIVATE = 60,                /* kw_PRIVATE  */
  YYSYMBOL_kw_PrintableString = 61,        /* kw_PrintableString  */
  YYSYMBOL_kw_REAL = 62,                   /* kw_REAL  */
  YYSYMBOL_kw_RELATIVE_OID = 63,           /* kw_RELATIVE_OID  */
  YYSYMBOL_kw_SEQUENCE = 64,               /* kw_SEQUENCE  */
  YYSYMBOL_kw_SET = 65,                    /* kw_SET  */
  YYSYMBOL_kw_SIZE = 66,                   /* kw_SIZE  */
  YYSYMBOL_kw_STRING = 67,                 /* kw_STRING  */
  YYSYMBOL_kw_SYNTAX = 68,                 /* kw_SYNTAX  */
  YYSYMBOL_kw_T61String = 69,              /* kw_T61String  */
  YYSYMBOL_kw_TAGS = 70,                   /* kw_TAGS  */
  YYSYMBOL_kw_TRUE = 71,                   /* kw_TRUE  */
  YYSYMBOL_kw_TYPE_IDENTIFIER = 72,        /* kw_TYPE_IDENTIFIER  */
  YYSYMBOL_kw_TeletexString = 73,          /* kw_TeletexString  */
  YYSYMBOL_kw_UNION = 74,                  /* kw_UNION  */
  YYSYMBOL_kw_UNIQUE = 75,                 /* kw_UNIQUE  */
  YYSYMBOL_kw_UNIVERSAL = 76,              /* kw_UNIVERSAL  */
  YYSYMBOL_kw_UTCTime = 77,                /* kw_UTCTime  */
  YYSYMBOL_kw_UTF8String = 78,             /* kw_UTF8String  */
  YYSYMBOL_kw_UniversalString = 79,        /* kw_UniversalString  */
  YYSYMBOL_kw_VideotexString = 80,         /* kw_VideotexString  */
  YYSYMBOL_kw_VisibleString = 81,          /* kw_VisibleString  */
  YYSYMBOL_kw_WITH = 82,                   /* kw_WITH  */
  YYSYMBOL_RANGE = 83,                     /* RANGE  */
  YYSYMBOL_EEQUAL = 84,                    /* EEQUAL  */
  YYSYMBOL_ELLIPSIS = 85,                  /* ELLIPSIS  */
  YYSYMBOL_IDENTIFIER = 86,                /* IDENTIFIER  */
  YYSYMBOL_referencename = 87,             /* referencename  */
  YYSYMBOL_STRING = 88,                    /* STRING  */
  YYSYMBOL_NUMBER = 89,                    /* NUMBER  */
  YYSYMBOL_90_ = 90,                       /* ';'  */
  YYSYMBOL_91_ = 91,                       /* ','  */
  YYSYMBOL_92_ = 92,                       /* '('  */
  YYSYMBOL_93_ = 93,                       /* ')'  */
  YYSYMBOL_94_ = 94,                       /* '{'  */
  YYSYMBOL_95_ = 95,                       /* '}'  */
  YYSYMBOL_96_ = 96,                       /* '['  */
  YYSYMBOL_97_ = 97,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 98,                  /* $accept  */
  YYSYMBOL_ModuleDefinition = 99,          /* ModuleDefinition  */
  YYSYMBOL_TagDefault = 100,               /* TagDefault  */
  YYSYMBOL_ExtensionDefault = 101,         /* ExtensionDefault  */
  YYSYMBOL_ModuleBody = 102,               /* ModuleBody  */
  YYSYMBOL_Imports = 103,                  /* Imports  */
  YYSYMBOL_SymbolsImported = 104,          /* SymbolsImported  */
  YYSYMBOL_SymbolsFromModuleList = 105,    /* SymbolsFromModuleList  */
  YYSYMBOL_SymbolsFromModule = 106,        /* SymbolsFromModule  */
  YYSYMBOL_Exports = 107,                  /* Exports  */
  YYSYMBOL_AssignmentList = 108,           /* AssignmentList  */
  YYSYMBOL_Assignment = 109,               /* Assignment  */
  YYSYMBOL_referencenames = 110,           /* referencenames  */
  YYSYMBOL_TypeAssignment = 111,           /* TypeAssignment  */
  YYSYMBOL_Type = 112,                     /* Type  */
  YYSYMBOL_BuiltinType = 113,              /* BuiltinType  */
  YYSYMBOL_BooleanType = 114,              /* BooleanType  */
  YYSYMBOL_range = 115,                    /* range  */
  YYSYMBOL_IntegerType = 116,              /* IntegerType  */
  YYSYMBOL_NamedNumberList = 117,          /* NamedNumberList  */
  YYSYMBOL_NamedNumber = 118,              /* NamedNumber  */
  YYSYMBOL_EnumeratedType = 119,           /* EnumeratedType  */
  YYSYMBOL_Enumerations = 120,             /* Enumerations  */
  YYSYMBOL_BitStringType = 121,            /* BitStringType  */
  YYSYMBOL_ObjectIdentifierType = 122,     /* ObjectIdentifierType  */
  YYSYMBOL_OctetStringType = 123,          /* OctetStringType  */
  YYSYMBOL_NullType = 124,                 /* NullType  */
  YYSYMBOL_size = 125,                     /* size  */
  YYSYMBOL_SequenceType = 126,             /* SequenceType  */
  YYSYMBOL_SequenceOfType = 127,           /* SequenceOfType  */
  YYSYMBOL_SetType = 128,                  /* SetType  */
  YYSYMBOL_SetOfType = 129,                /* SetOfType  */
  YYSYMBOL_ChoiceType = 130,               /* ChoiceType  */
  YYSYMBOL_ReferencedType = 131,           /* ReferencedType  */
  YYSYMBOL_DefinedType = 132,              /* DefinedType  */
  YYSYMBOL_UsefulType = 133,               /* UsefulType  */
  YYSYMBOL_ConstrainedType = 134,          /* ConstrainedType  */
  YYSYMBOL_Constraint = 135,               /* Constraint  */
  YYSYMBOL_ConstraintSpec = 136,           /* ConstraintSpec  */
  YYSYMBOL_GeneralConstraint = 137,        /* GeneralConstraint  */
  YYSYMBOL_ContentsConstraint = 138,       /* ContentsConstraint  */
  YYSYMBOL_UserDefinedConstraint = 139,    /* UserDefinedConstraint  */
  YYSYMBOL_TaggedType = 140,               /* TaggedType  */
  YYSYMBOL_Tag = 141,                      /* Tag  */
  YYSYMBOL_Class = 142,                    /* Class  */
  YYSYMBOL_tagenv = 143,                   /* tagenv  */
  YYSYMBOL_ValueAssignment = 144,          /* ValueAssignment  */
  YYSYMBOL_CharacterStringType = 145,      /* CharacterStringType  */
  YYSYMBOL_RestrictedCharactedStringType = 146, /* RestrictedCharactedStringType  */
  YYSYMBOL_ComponentTypeList = 147,        /* ComponentTypeList  */
  YYSYMBOL_NamedType = 148,                /* NamedType  */
  YYSYMBOL_ComponentType = 149,            /* ComponentType  */
  YYSYMBOL_NamedBitList = 150,             /* NamedBitList  */
  YYSYMBOL_NamedBit = 151,                 /* NamedBit  */
  YYSYMBOL_objid_opt = 152,                /* objid_opt  */
  YYSYMBOL_objid = 153,                    /* objid  */
  YYSYMBOL_objid_list = 154,               /* objid_list  */
  YYSYMBOL_objid_element = 155,            /* objid_element  */
  YYSYMBOL_Value = 156,                    /* Value  */
  YYSYMBOL_BuiltinValue = 157,             /* BuiltinValue  */
  YYSYMBOL_ReferencedValue = 158,          /* ReferencedValue  */
  YYSYMBOL_DefinedValue = 159,             /* DefinedValue  */
  YYSYMBOL_Valuereference = 160,           /* Valuereference  */
  YYSYMBOL_CharacterStringValue = 161,     /* CharacterStringValue  */
  YYSYMBOL_BooleanValue = 162,             /* BooleanValue  */
  YYSYMBOL_IntegerValue = 163,             /* IntegerValue  */
  YYSYMBOL_SignedNumber = 164,             /* SignedNumber  */
  YYSYMBOL_NullValue = 165,                /* NullValue  */
  YYSYMBOL_ObjectIdentifierValue = 166     /* ObjectIdentifierValue  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   203

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  98
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  69
/* YYNRULES -- Number of rules.  */
#define YYNRULES  140
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  220

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   344


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      92,    93,     2,     2,    91,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    90,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,    97,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    94,     2,    95,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   240,   240,   247,   249,   251,   253,   256,   258,   261,
     262,   265,   266,   269,   270,   273,   274,   277,   289,   295,
     296,   299,   300,   303,   304,   307,   313,   321,   331,   332,
     333,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   352,   359,   369,   377,   385,
     396,   401,   407,   415,   421,   426,   430,   443,   451,   454,
     461,   469,   475,   489,   497,   498,   503,   509,   517,   532,
     538,   546,   554,   561,   562,   565,   576,   581,   588,   604,
     610,   613,   614,   617,   623,   631,   641,   647,   665,   674,
     677,   681,   685,   692,   695,   699,   706,   717,   720,   725,
     730,   735,   740,   745,   750,   755,   763,   769,   774,   785,
     796,   802,   808,   816,   822,   829,   842,   843,   846,   853,
     856,   867,   871,   882,   888,   889,   892,   893,   894,   895,
     896,   899,   902,   905,   916,   924,   930,   938,   946,   949,
     954
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "kw_ABSENT",
  "kw_ABSTRACT_SYNTAX", "kw_ALL", "kw_APPLICATION", "kw_AUTOMATIC",
  "kw_BEGIN", "kw_BIT", "kw_BMPString", "kw_BOOLEAN", "kw_BY",
  "kw_CHARACTER", "kw_CHOICE", "kw_CLASS", "kw_COMPONENT", "kw_COMPONENTS",
  "kw_CONSTRAINED", "kw_CONTAINING", "kw_DEFAULT", "kw_DEFINITIONS",
  "kw_EMBEDDED", "kw_ENCODED", "kw_END", "kw_ENUMERATED", "kw_EXCEPT",
  "kw_EXPLICIT", "kw_EXPORTS", "kw_EXTENSIBILITY", "kw_EXTERNAL",
  "kw_FALSE", "kw_FROM", "kw_GeneralString", "kw_GeneralizedTime",
  "kw_GraphicString", "kw_IA5String", "kw_IDENTIFIER", "kw_IMPLICIT",
  "kw_IMPLIED", "kw_IMPORTS", "kw_INCLUDES", "kw_INSTANCE", "kw_INTEGER",
  "kw_INTERSECTION", "kw_ISO646String", "kw_MAX", "kw_MIN",
  "kw_MINUS_INFINITY", "kw_NULL", "kw_NumericString", "kw_OBJECT",
  "kw_OCTET", "kw_OF", "kw_OPTIONAL", "kw_ObjectDescriptor", "kw_PATTERN",
  "kw_PDV", "kw_PLUS_INFINITY", "kw_PRESENT", "kw_PRIVATE",
  "kw_PrintableString", "kw_REAL", "kw_RELATIVE_OID", "kw_SEQUENCE",
  "kw_SET", "kw_SIZE", "kw_STRING", "kw_SYNTAX", "kw_T61String", "kw_TAGS",
  "kw_TRUE", "kw_TYPE_IDENTIFIER", "kw_TeletexString", "kw_UNION",
  "kw_UNIQUE", "kw_UNIVERSAL", "kw_UTCTime", "kw_UTF8String",
  "kw_UniversalString", "kw_VideotexString", "kw_VisibleString", "kw_WITH",
  "RANGE", "EEQUAL", "ELLIPSIS", "IDENTIFIER", "referencename", "STRING",
  "NUMBER", "';'", "','", "'('", "')'", "'{'", "'}'", "'['", "']'",
  "$accept", "ModuleDefinition", "TagDefault", "ExtensionDefault",
  "ModuleBody", "Imports", "SymbolsImported", "SymbolsFromModuleList",
  "SymbolsFromModule", "Exports", "AssignmentList", "Assignment",
  "referencenames", "TypeAssignment", "Type", "BuiltinType", "BooleanType",
  "range", "IntegerType", "NamedNumberList", "NamedNumber",
  "EnumeratedType", "Enumerations", "BitStringType",
  "ObjectIdentifierType", "OctetStringType", "NullType", "size",
  "SequenceType", "SequenceOfType", "SetType", "SetOfType", "ChoiceType",
  "ReferencedType", "DefinedType", "UsefulType", "ConstrainedType",
  "Constraint", "ConstraintSpec", "GeneralConstraint",
  "ContentsConstraint", "UserDefinedConstraint", "TaggedType", "Tag",
  "Class", "tagenv", "ValueAssignment", "CharacterStringType",
  "RestrictedCharactedStringType", "ComponentTypeList", "NamedType",
  "ComponentType", "NamedBitList", "NamedBit", "objid_opt", "objid",
  "objid_list", "objid_element", "Value", "BuiltinValue",
  "ReferencedValue", "DefinedValue", "Valuereference",
  "CharacterStringValue", "BooleanValue", "IntegerValue", "SignedNumber",
  "NullValue", "ObjectIdentifierValue", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-119)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-11)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -43,   -56,    47,   -65,    29,  -119,  -119,   -31,  -119,   -25,
     -65,     4,    -1,  -119,  -119,    17,    20,    26,    50,    13,
    -119,  -119,  -119,    63,    24,  -119,  -119,   104,     8,    -2,
      89,    74,  -119,    33,    25,  -119,    34,    39,    34,  -119,
      37,    34,  -119,    98,    58,  -119,    39,  -119,  -119,  -119,
    -119,  -119,    52,    66,  -119,  -119,    51,    53,  -119,  -119,
    -119,   -79,  -119,   109,    81,  -119,   -60,   -48,  -119,  -119,
    -119,  -119,  -119,   107,  -119,     2,   -74,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,   -18,  -119,  -119,  -119,   -56,
      55,    65,    67,   -12,    67,  -119,  -119,    86,    68,   -70,
     102,   107,   -69,    69,  -119,  -119,  -119,    73,    40,    10,
    -119,  -119,  -119,   107,  -119,    71,   107,   -47,   -13,  -119,
      72,    75,  -119,    70,  -119,    80,  -119,  -119,  -119,  -119,
    -119,  -119,   -71,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,   -46,  -119,  -119,  -119,   -39,   107,    69,
    -119,   -38,    76,  -119,   155,   107,   157,    77,  -119,  -119,
    -119,    69,    82,   -10,  -119,    69,   -22,  -119,    40,  -119,
      87,    19,  -119,    40,     9,  -119,  -119,  -119,    69,  -119,
    -119,    83,   -19,    40,  -119,    90,    71,  -119,  -119,  -119,
    -119,    85,  -119,  -119,    88,    94,    96,    95,   163,  -119,
      99,  -119,  -119,  -119,  -119,  -119,  -119,    40,  -119,  -119
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   117,     0,   119,     0,   116,     1,   122,   123,     0,
     119,     6,     0,   118,   120,     0,     0,     0,     8,     0,
       5,     3,     4,     0,     0,   121,     7,     0,    20,     0,
       0,    12,    19,    26,     0,     2,    14,     0,     0,    18,
       0,    13,    15,     0,     0,     9,    21,    23,    24,    25,
      11,    16,     0,     0,   104,    45,     0,     0,    98,    76,
     103,    50,    63,     0,     0,   101,    64,     0,    99,    77,
     100,   105,   102,     0,    75,    89,     0,    28,    32,    36,
      35,    31,    38,    39,    37,    40,    41,    42,    43,    34,
      29,    73,    74,    30,    44,    93,    33,    97,    22,   117,
      59,     0,     0,     0,     0,    51,    61,    64,     0,     0,
       0,     0,     0,    27,    91,    92,    90,     0,     0,     0,
      78,    94,    95,     0,    17,     0,     0,     0,   110,   106,
       0,    58,    53,     0,   136,     0,   139,   135,   133,   134,
     138,   140,     0,   124,   125,   131,   132,   127,   126,   128,
     137,   130,   129,     0,    62,    65,    67,     0,     0,    71,
      70,     0,     0,    96,     0,     0,     0,     0,    80,    81,
      82,    87,     0,     0,   113,   109,     0,    72,     0,   111,
       0,     0,    57,     0,     0,    49,    52,    66,    68,    69,
      88,     0,    83,     0,    79,     0,     0,    60,   108,   107,
     112,     0,    55,    54,     0,     0,     0,     0,     0,    84,
       0,   114,    56,    48,    47,    46,    86,     0,   115,    85
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,   141,  -119,
     137,  -119,   -15,  -119,   -72,  -119,  -119,    91,  -119,    92,
      14,  -119,  -119,  -119,  -119,  -119,  -119,    84,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,  -119,   -82,
    -119,    18,  -119,     5,   101,     1,   187,  -119,  -118,  -119,
    -119,  -119,  -119,  -119,  -119,  -119,    22,  -119,  -119
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     2,    18,    24,    30,    37,    40,    41,    42,    31,
      45,    46,    43,    47,    76,    77,    78,   105,    79,   131,
     132,    80,   133,    81,    82,    83,    84,   110,    85,    86,
      87,    88,    89,    90,    91,    92,    93,   120,   167,   168,
     169,   170,    94,    95,   117,   123,    48,    96,    97,   127,
     128,   129,   173,   174,     4,   141,     9,    10,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     163,   113,     5,    32,   208,   111,   108,   178,   114,   121,
     118,    15,   184,   103,    34,   104,   126,   126,   119,   134,
     122,     7,   185,    49,     8,   156,   160,   157,   164,   165,
     161,    16,   -10,   166,   109,   135,    29,   136,     3,   159,
     134,   179,    17,     1,   176,   181,   112,     6,   177,   186,
      11,   171,   176,   176,   175,   205,   187,   189,   136,   137,
     200,    12,   115,   198,   126,   204,   206,    53,    54,    55,
      13,   134,    56,   119,   138,   209,   139,   140,   116,    23,
     137,   196,     3,    57,    33,   197,   188,    20,    19,   136,
      21,    58,    59,   192,    60,   138,    22,   139,   140,   219,
       5,    61,    26,     3,   202,   130,    25,    62,    27,    63,
      64,   137,    28,    35,    36,    39,    53,    54,    55,    65,
      33,    56,    66,    67,    38,    44,   138,    50,   139,   140,
      52,    68,    57,   100,     3,    69,    70,    71,    99,    72,
      58,    59,    73,    60,    74,   101,   106,   102,   107,   125,
      61,   126,   108,   130,    75,   158,    62,   172,    63,    64,
     103,   119,   162,   183,   180,   182,   181,   191,    65,   193,
     194,    66,    67,   190,   195,   217,   140,   207,   212,   210,
      68,   213,    51,    98,    69,    70,    71,   214,    72,   215,
     216,   154,   218,    74,   199,   203,   153,    14,     0,   155,
     124,   211,   201,    75
};

static const yytype_int16 yycheck[] =
{
     118,    73,     1,     5,    23,    53,    66,    20,     6,    27,
      84,     7,    83,    92,    29,    94,    86,    86,    92,    31,
      38,    86,    93,    38,    89,    95,    95,   109,    18,    19,
     112,    27,    24,    23,    94,    47,    28,    49,    94,   111,
      31,    54,    38,    86,    91,    91,    94,     0,    95,    95,
      21,   123,    91,    91,   126,    46,    95,    95,    49,    71,
     178,    92,    60,    85,    86,   183,   184,     9,    10,    11,
      95,    31,    14,    92,    86,   193,    88,    89,    76,    29,
      71,    91,    94,    25,    86,    95,   158,    70,    89,    49,
      70,    33,    34,   165,    36,    86,    70,    88,    89,   217,
      99,    43,    39,    94,    85,    86,    93,    49,    84,    51,
      52,    71,     8,    24,    40,    90,     9,    10,    11,    61,
      86,    14,    64,    65,    91,    86,    86,    90,    88,    89,
      32,    73,    25,    67,    94,    77,    78,    79,    86,    81,
      33,    34,    84,    36,    86,    94,    37,    94,    67,    94,
      43,    86,    66,    86,    96,    53,    49,    86,    51,    52,
      92,    92,    89,    83,    92,    95,    91,    12,    61,    12,
      93,    64,    65,    97,    92,    12,    89,    94,    93,    89,
      73,    93,    41,    46,    77,    78,    79,    93,    81,    93,
      95,   107,    93,    86,   176,   181,   104,    10,    -1,   108,
      99,   196,   180,    96
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    86,    99,    94,   152,   153,     0,    86,    89,   154,
     155,    21,    92,    95,   154,     7,    27,    38,   100,    89,
      70,    70,    70,    29,   101,    93,    39,    84,     8,    28,
     102,   107,     5,    86,   110,    24,    40,   103,    91,    90,
     104,   105,   106,   110,    86,   108,   109,   111,   144,   110,
      90,   106,    32,     9,    10,    11,    14,    25,    33,    34,
      36,    43,    49,    51,    52,    61,    64,    65,    73,    77,
      78,    79,    81,    84,    86,    96,   112,   113,   114,   116,
     119,   121,   122,   123,   124,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   140,   141,   145,   146,   108,    86,
      67,    94,    94,    92,    94,   115,    37,    67,    66,    94,
     125,    53,    94,   112,     6,    60,    76,   142,    84,    92,
     135,    27,    38,   143,   152,    94,    86,   147,   148,   149,
      86,   117,   118,   120,    31,    47,    49,    71,    86,    88,
      89,   153,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   117,   125,   115,    95,   147,    53,   112,
      95,   147,    89,   156,    18,    19,    23,   136,   137,   138,
     139,   112,    86,   150,   151,   112,    91,    95,    20,    54,
      92,    91,    95,    83,    83,    93,    95,    95,   112,    95,
      97,    12,   112,    12,    93,    92,    91,    95,    85,   149,
     156,   164,    85,   118,   156,    46,   156,    94,    23,   156,
      89,   151,    93,    93,    93,    93,    95,    12,    93,   156
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    98,    99,   100,   100,   100,   100,   101,   101,   102,
     102,   103,   103,   104,   104,   105,   105,   106,   107,   107,
     107,   108,   108,   109,   109,   110,   110,   111,   112,   112,
     112,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   113,   113,   113,   114,   115,   115,   115,   115,
     116,   116,   116,   117,   117,   117,   118,   119,   120,   121,
     121,   122,   123,   124,   125,   125,   126,   126,   127,   128,
     128,   129,   130,   131,   131,   132,   133,   133,   134,   135,
     136,   137,   137,   138,   138,   138,   139,   140,   141,   142,
     142,   142,   142,   143,   143,   143,   144,   145,   146,   146,
     146,   146,   146,   146,   146,   146,   147,   147,   147,   148,
     149,   149,   149,   150,   150,   151,   152,   152,   153,   154,
     154,   155,   155,   155,   156,   156,   157,   157,   157,   157,
     157,   158,   159,   160,   161,   162,   162,   163,   164,   165,
     166
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     9,     2,     2,     2,     0,     2,     0,     3,
       0,     3,     0,     1,     0,     1,     2,     4,     3,     2,
       0,     1,     2,     1,     1,     3,     1,     3,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     5,     5,     3,
       1,     2,     4,     1,     3,     3,     4,     4,     1,     2,
       5,     2,     3,     1,     0,     2,     4,     3,     4,     4,
       3,     3,     4,     1,     1,     1,     1,     1,     2,     3,
       1,     1,     1,     2,     3,     5,     4,     3,     4,     0,
       1,     1,     1,     0,     1,     1,     4,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     2,
       1,     2,     3,     1,     3,     4,     1,     0,     3,     0,
       2,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* ModuleDefinition: IDENTIFIER objid_opt kw_DEFINITIONS TagDefault ExtensionDefault EEQUAL kw_BEGIN ModuleBody kw_END  */
#line 242 "asn1parse.y"
                {
			checkundefined();
		}
#line 1687 "asn1parse.c"
    break;

  case 3: /* TagDefault: kw_EXPLICIT kw_TAGS  */
#line 248 "asn1parse.y"
                        { default_tag_env = TE_EXPLICIT; }
#line 1693 "asn1parse.c"
    break;

  case 4: /* TagDefault: kw_IMPLICIT kw_TAGS  */
#line 250 "asn1parse.y"
                        { default_tag_env = TE_IMPLICIT; }
#line 1699 "asn1parse.c"
    break;

  case 5: /* TagDefault: kw_AUTOMATIC kw_TAGS  */
#line 252 "asn1parse.y"
                      { lex_error_message("automatic tagging is not supported"); }
#line 1705 "asn1parse.c"
    break;

  case 7: /* ExtensionDefault: kw_EXTENSIBILITY kw_IMPLIED  */
#line 257 "asn1parse.y"
                      { lex_error_message("no extensibility options supported"); }
#line 1711 "asn1parse.c"
    break;

  case 17: /* SymbolsFromModule: referencenames kw_FROM IDENTIFIER objid_opt  */
#line 278 "asn1parse.y"
                {
		    struct string_list *sl;
		    for(sl = (yyvsp[-3].sl); sl != NULL; sl = sl->next) {
			Symbol *s = addsym(sl->string);
			s->stype = Stype;
			gen_template_import(s);
		    }
		    add_import((yyvsp[-1].name));
		}
#line 1725 "asn1parse.c"
    break;

  case 18: /* Exports: kw_EXPORTS referencenames ';'  */
#line 290 "asn1parse.y"
                {
		    struct string_list *sl;
		    for(sl = (yyvsp[-1].sl); sl != NULL; sl = sl->next)
			add_export(sl->string);
		}
#line 1735 "asn1parse.c"
    break;

  case 25: /* referencenames: IDENTIFIER ',' referencenames  */
#line 308 "asn1parse.y"
                {
		    (yyval.sl) = emalloc(sizeof(*(yyval.sl)));
		    (yyval.sl)->string = (yyvsp[-2].name);
		    (yyval.sl)->next = (yyvsp[0].sl);
		}
#line 1745 "asn1parse.c"
    break;

  case 26: /* referencenames: IDENTIFIER  */
#line 314 "asn1parse.y"
                {
		    (yyval.sl) = emalloc(sizeof(*(yyval.sl)));
		    (yyval.sl)->string = (yyvsp[0].name);
		    (yyval.sl)->next = NULL;
		}
#line 1755 "asn1parse.c"
    break;

  case 27: /* TypeAssignment: IDENTIFIER EEQUAL Type  */
#line 322 "asn1parse.y"
                {
		    Symbol *s = addsym ((yyvsp[-2].name));
		    s->stype = Stype;
		    s->type = (yyvsp[0].type);
		    fix_labels(s);
		    generate_type (s);
		}
#line 1767 "asn1parse.c"
    break;

  case 45: /* BooleanType: kw_BOOLEAN  */
#line 353 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_Boolean,
				     TE_EXPLICIT, new_type(TBoolean));
		}
#line 1776 "asn1parse.c"
    break;

  case 46: /* range: '(' Value RANGE Value ')'  */
#line 360 "asn1parse.y"
                {
		    if((yyvsp[-3].value)->type != integervalue)
			lex_error_message("Non-integer used in first part of range");
		    if((yyvsp[-3].value)->type != integervalue)
			lex_error_message("Non-integer in second part of range");
		    (yyval.range) = ecalloc(1, sizeof(*(yyval.range)));
		    (yyval.range)->min = (yyvsp[-3].value)->u.integervalue;
		    (yyval.range)->max = (yyvsp[-1].value)->u.integervalue;
		}
#line 1790 "asn1parse.c"
    break;

  case 47: /* range: '(' Value RANGE kw_MAX ')'  */
#line 370 "asn1parse.y"
                {
		    if((yyvsp[-3].value)->type != integervalue)
			lex_error_message("Non-integer in first part of range");
		    (yyval.range) = ecalloc(1, sizeof(*(yyval.range)));
		    (yyval.range)->min = (yyvsp[-3].value)->u.integervalue;
		    (yyval.range)->max = INT_MAX;
		}
#line 1802 "asn1parse.c"
    break;

  case 48: /* range: '(' kw_MIN RANGE Value ')'  */
#line 378 "asn1parse.y"
                {
		    if((yyvsp[-1].value)->type != integervalue)
			lex_error_message("Non-integer in second part of range");
		    (yyval.range) = ecalloc(1, sizeof(*(yyval.range)));
		    (yyval.range)->min = INT_MIN;
		    (yyval.range)->max = (yyvsp[-1].value)->u.integervalue;
		}
#line 1814 "asn1parse.c"
    break;

  case 49: /* range: '(' Value ')'  */
#line 386 "asn1parse.y"
                {
		    if((yyvsp[-1].value)->type != integervalue)
			lex_error_message("Non-integer used in limit");
		    (yyval.range) = ecalloc(1, sizeof(*(yyval.range)));
		    (yyval.range)->min = (yyvsp[-1].value)->u.integervalue;
		    (yyval.range)->max = (yyvsp[-1].value)->u.integervalue;
		}
#line 1826 "asn1parse.c"
    break;

  case 50: /* IntegerType: kw_INTEGER  */
#line 397 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_Integer,
				     TE_EXPLICIT, new_type(TInteger));
		}
#line 1835 "asn1parse.c"
    break;

  case 51: /* IntegerType: kw_INTEGER range  */
#line 402 "asn1parse.y"
                {
			(yyval.type) = new_type(TInteger);
			(yyval.type)->range = (yyvsp[0].range);
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_Integer, TE_EXPLICIT, (yyval.type));
		}
#line 1845 "asn1parse.c"
    break;

  case 52: /* IntegerType: kw_INTEGER '{' NamedNumberList '}'  */
#line 408 "asn1parse.y"
                {
		  (yyval.type) = new_type(TInteger);
		  (yyval.type)->members = (yyvsp[-1].members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Integer, TE_EXPLICIT, (yyval.type));
		}
#line 1855 "asn1parse.c"
    break;

  case 53: /* NamedNumberList: NamedNumber  */
#line 416 "asn1parse.y"
                {
			(yyval.members) = emalloc(sizeof(*(yyval.members)));
			ASN1_TAILQ_INIT((yyval.members));
			ASN1_TAILQ_INSERT_HEAD((yyval.members), (yyvsp[0].member), members);
		}
#line 1865 "asn1parse.c"
    break;

  case 54: /* NamedNumberList: NamedNumberList ',' NamedNumber  */
#line 422 "asn1parse.y"
                {
			ASN1_TAILQ_INSERT_TAIL((yyvsp[-2].members), (yyvsp[0].member), members);
			(yyval.members) = (yyvsp[-2].members);
		}
#line 1874 "asn1parse.c"
    break;

  case 55: /* NamedNumberList: NamedNumberList ',' ELLIPSIS  */
#line 427 "asn1parse.y"
                        { (yyval.members) = (yyvsp[-2].members); }
#line 1880 "asn1parse.c"
    break;

  case 56: /* NamedNumber: IDENTIFIER '(' SignedNumber ')'  */
#line 431 "asn1parse.y"
                {
			(yyval.member) = emalloc(sizeof(*(yyval.member)));
			(yyval.member)->name = (yyvsp[-3].name);
			(yyval.member)->gen_name = estrdup((yyvsp[-3].name));
			output_name ((yyval.member)->gen_name);
			(yyval.member)->val = (yyvsp[-1].constant);
			(yyval.member)->optional = 0;
			(yyval.member)->ellipsis = 0;
			(yyval.member)->type = NULL;
		}
#line 1895 "asn1parse.c"
    break;

  case 57: /* EnumeratedType: kw_ENUMERATED '{' Enumerations '}'  */
#line 444 "asn1parse.y"
                {
		  (yyval.type) = new_type(TInteger);
		  (yyval.type)->members = (yyvsp[-1].members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Enumerated, TE_EXPLICIT, (yyval.type));
		}
#line 1905 "asn1parse.c"
    break;

  case 59: /* BitStringType: kw_BIT kw_STRING  */
#line 455 "asn1parse.y"
                {
		  (yyval.type) = new_type(TBitString);
		  (yyval.type)->members = emalloc(sizeof(*(yyval.type)->members));
		  ASN1_TAILQ_INIT((yyval.type)->members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_BitString, TE_EXPLICIT, (yyval.type));
		}
#line 1916 "asn1parse.c"
    break;

  case 60: /* BitStringType: kw_BIT kw_STRING '{' NamedBitList '}'  */
#line 462 "asn1parse.y"
                {
		  (yyval.type) = new_type(TBitString);
		  (yyval.type)->members = (yyvsp[-1].members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_BitString, TE_EXPLICIT, (yyval.type));
		}
#line 1926 "asn1parse.c"
    break;

  case 61: /* ObjectIdentifierType: kw_OBJECT kw_IDENTIFIER  */
#line 470 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_OID,
				     TE_EXPLICIT, new_type(TOID));
		}
#line 1935 "asn1parse.c"
    break;

  case 62: /* OctetStringType: kw_OCTET kw_STRING size  */
#line 476 "asn1parse.y"
                {
		    Type *t = new_type(TOctetString);
		    t->range = (yyvsp[0].range);
		    if (t->range) {
			if (t->range->min < 0)
			    lex_error_message("can't use a negative SIZE range "
					      "length for OCTET STRING");
		    }
		    (yyval.type) = new_tag(ASN1_C_UNIV, UT_OctetString,
				 TE_EXPLICIT, t);
		}
#line 1951 "asn1parse.c"
    break;

  case 63: /* NullType: kw_NULL  */
#line 490 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_Null,
				     TE_EXPLICIT, new_type(TNull));
		}
#line 1960 "asn1parse.c"
    break;

  case 64: /* size: %empty  */
#line 497 "asn1parse.y"
                { (yyval.range) = NULL; }
#line 1966 "asn1parse.c"
    break;

  case 65: /* size: kw_SIZE range  */
#line 499 "asn1parse.y"
                { (yyval.range) = (yyvsp[0].range); }
#line 1972 "asn1parse.c"
    break;

  case 66: /* SequenceType: kw_SEQUENCE '{' ComponentTypeList '}'  */
#line 504 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSequence);
		  (yyval.type)->members = (yyvsp[-1].members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Sequence, default_tag_env, (yyval.type));
		}
#line 1982 "asn1parse.c"
    break;

  case 67: /* SequenceType: kw_SEQUENCE '{' '}'  */
#line 510 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSequence);
		  (yyval.type)->members = NULL;
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Sequence, default_tag_env, (yyval.type));
		}
#line 1992 "asn1parse.c"
    break;

  case 68: /* SequenceOfType: kw_SEQUENCE size kw_OF Type  */
#line 518 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSequenceOf);
		  (yyval.type)->range = (yyvsp[-2].range);
		  if ((yyval.type)->range) {
		      if ((yyval.type)->range->min < 0)
			  lex_error_message("can't use a negative SIZE range "
					    "length for SEQUENCE OF");
		    }

		  (yyval.type)->subtype = (yyvsp[0].type);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Sequence, default_tag_env, (yyval.type));
		}
#line 2009 "asn1parse.c"
    break;

  case 69: /* SetType: kw_SET '{' ComponentTypeList '}'  */
#line 533 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSet);
		  (yyval.type)->members = (yyvsp[-1].members);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Set, default_tag_env, (yyval.type));
		}
#line 2019 "asn1parse.c"
    break;

  case 70: /* SetType: kw_SET '{' '}'  */
#line 539 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSet);
		  (yyval.type)->members = NULL;
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Set, default_tag_env, (yyval.type));
		}
#line 2029 "asn1parse.c"
    break;

  case 71: /* SetOfType: kw_SET kw_OF Type  */
#line 547 "asn1parse.y"
                {
		  (yyval.type) = new_type(TSetOf);
		  (yyval.type)->subtype = (yyvsp[0].type);
		  (yyval.type) = new_tag(ASN1_C_UNIV, UT_Set, default_tag_env, (yyval.type));
		}
#line 2039 "asn1parse.c"
    break;

  case 72: /* ChoiceType: kw_CHOICE '{' ComponentTypeList '}'  */
#line 555 "asn1parse.y"
                {
		  (yyval.type) = new_type(TChoice);
		  (yyval.type)->members = (yyvsp[-1].members);
		}
#line 2048 "asn1parse.c"
    break;

  case 75: /* DefinedType: IDENTIFIER  */
#line 566 "asn1parse.y"
                {
		  Symbol *s = addsym((yyvsp[0].name));
		  (yyval.type) = new_type(TType);
		  if(s->stype != Stype && s->stype != SUndefined)
		    lex_error_message ("%s is not a type\n", (yyvsp[0].name));
		  else
		    (yyval.type)->symbol = s;
		}
#line 2061 "asn1parse.c"
    break;

  case 76: /* UsefulType: kw_GeneralizedTime  */
#line 577 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_GeneralizedTime,
				     TE_EXPLICIT, new_type(TGeneralizedTime));
		}
#line 2070 "asn1parse.c"
    break;

  case 77: /* UsefulType: kw_UTCTime  */
#line 582 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_UTCTime,
				     TE_EXPLICIT, new_type(TUTCTime));
		}
#line 2079 "asn1parse.c"
    break;

  case 78: /* ConstrainedType: Type Constraint  */
#line 589 "asn1parse.y"
                {
		    /* if (Constraint.type == contentConstrant) {
		       assert(Constraint.u.constraint.type == octetstring|bitstring-w/o-NamedBitList); // remember to check type reference too
		       if (Constraint.u.constraint.type) {
		         assert((Constraint.u.constraint.type.length % 8) == 0);
		       }
		      }
		      if (Constraint.u.constraint.encoding) {
		        type == der-oid|ber-oid
		      }
		    */
		}
#line 2096 "asn1parse.c"
    break;

  case 79: /* Constraint: '(' ConstraintSpec ')'  */
#line 605 "asn1parse.y"
                {
		    (yyval.constraint_spec) = (yyvsp[-1].constraint_spec);
		}
#line 2104 "asn1parse.c"
    break;

  case 83: /* ContentsConstraint: kw_CONTAINING Type  */
#line 618 "asn1parse.y"
                {
		    (yyval.constraint_spec) = new_constraint_spec(CT_CONTENTS);
		    (yyval.constraint_spec)->u.content.type = (yyvsp[0].type);
		    (yyval.constraint_spec)->u.content.encoding = NULL;
		}
#line 2114 "asn1parse.c"
    break;

  case 84: /* ContentsConstraint: kw_ENCODED kw_BY Value  */
#line 624 "asn1parse.y"
                {
		    if ((yyvsp[0].value)->type != objectidentifiervalue)
			lex_error_message("Non-OID used in ENCODED BY constraint");
		    (yyval.constraint_spec) = new_constraint_spec(CT_CONTENTS);
		    (yyval.constraint_spec)->u.content.type = NULL;
		    (yyval.constraint_spec)->u.content.encoding = (yyvsp[0].value);
		}
#line 2126 "asn1parse.c"
    break;

  case 85: /* ContentsConstraint: kw_CONTAINING Type kw_ENCODED kw_BY Value  */
#line 632 "asn1parse.y"
                {
		    if ((yyvsp[0].value)->type != objectidentifiervalue)
			lex_error_message("Non-OID used in ENCODED BY constraint");
		    (yyval.constraint_spec) = new_constraint_spec(CT_CONTENTS);
		    (yyval.constraint_spec)->u.content.type = (yyvsp[-3].type);
		    (yyval.constraint_spec)->u.content.encoding = (yyvsp[0].value);
		}
#line 2138 "asn1parse.c"
    break;

  case 86: /* UserDefinedConstraint: kw_CONSTRAINED kw_BY '{' '}'  */
#line 642 "asn1parse.y"
                {
		    (yyval.constraint_spec) = new_constraint_spec(CT_USER);
		}
#line 2146 "asn1parse.c"
    break;

  case 87: /* TaggedType: Tag tagenv Type  */
#line 648 "asn1parse.y"
                {
			(yyval.type) = new_type(TTag);
			(yyval.type)->tag = (yyvsp[-2].tag);
			(yyval.type)->tag.tagenv = (yyvsp[-1].constant);
			if (template_flag) {
			    (yyval.type)->subtype = (yyvsp[0].type);
			} else {
			    if((yyvsp[0].type)->type == TTag && (yyvsp[-1].constant) == TE_IMPLICIT) {
				(yyval.type)->subtype = (yyvsp[0].type)->subtype;
				free((yyvsp[0].type));
			    } else {
				(yyval.type)->subtype = (yyvsp[0].type);
			    }
			}
		}
#line 2166 "asn1parse.c"
    break;

  case 88: /* Tag: '[' Class NUMBER ']'  */
#line 666 "asn1parse.y"
                {
			(yyval.tag).tagclass = (yyvsp[-2].constant);
			(yyval.tag).tagvalue = (yyvsp[-1].constant);
			(yyval.tag).tagenv = default_tag_env;
		}
#line 2176 "asn1parse.c"
    break;

  case 89: /* Class: %empty  */
#line 674 "asn1parse.y"
                {
			(yyval.constant) = ASN1_C_CONTEXT;
		}
#line 2184 "asn1parse.c"
    break;

  case 90: /* Class: kw_UNIVERSAL  */
#line 678 "asn1parse.y"
                {
			(yyval.constant) = ASN1_C_UNIV;
		}
#line 2192 "asn1parse.c"
    break;

  case 91: /* Class: kw_APPLICATION  */
#line 682 "asn1parse.y"
                {
			(yyval.constant) = ASN1_C_APPL;
		}
#line 2200 "asn1parse.c"
    break;

  case 92: /* Class: kw_PRIVATE  */
#line 686 "asn1parse.y"
                {
			(yyval.constant) = ASN1_C_PRIVATE;
		}
#line 2208 "asn1parse.c"
    break;

  case 93: /* tagenv: %empty  */
#line 692 "asn1parse.y"
                {
			(yyval.constant) = default_tag_env;
		}
#line 2216 "asn1parse.c"
    break;

  case 94: /* tagenv: kw_EXPLICIT  */
#line 696 "asn1parse.y"
                {
			(yyval.constant) = default_tag_env;
		}
#line 2224 "asn1parse.c"
    break;

  case 95: /* tagenv: kw_IMPLICIT  */
#line 700 "asn1parse.y"
                {
			(yyval.constant) = TE_IMPLICIT;
		}
#line 2232 "asn1parse.c"
    break;

  case 96: /* ValueAssignment: IDENTIFIER Type EEQUAL Value  */
#line 707 "asn1parse.y"
                {
			Symbol *s;
			s = addsym ((yyvsp[-3].name));

			s->stype = SValue;
			s->value = (yyvsp[0].value);
			generate_constant (s);
		}
#line 2245 "asn1parse.c"
    break;

  case 98: /* RestrictedCharactedStringType: kw_GeneralString  */
#line 721 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_GeneralString,
				     TE_EXPLICIT, new_type(TGeneralString));
		}
#line 2254 "asn1parse.c"
    break;

  case 99: /* RestrictedCharactedStringType: kw_TeletexString  */
#line 726 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_TeletexString,
				     TE_EXPLICIT, new_type(TTeletexString));
		}
#line 2263 "asn1parse.c"
    break;

  case 100: /* RestrictedCharactedStringType: kw_UTF8String  */
#line 731 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_UTF8String,
				     TE_EXPLICIT, new_type(TUTF8String));
		}
#line 2272 "asn1parse.c"
    break;

  case 101: /* RestrictedCharactedStringType: kw_PrintableString  */
#line 736 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_PrintableString,
				     TE_EXPLICIT, new_type(TPrintableString));
		}
#line 2281 "asn1parse.c"
    break;

  case 102: /* RestrictedCharactedStringType: kw_VisibleString  */
#line 741 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_VisibleString,
				     TE_EXPLICIT, new_type(TVisibleString));
		}
#line 2290 "asn1parse.c"
    break;

  case 103: /* RestrictedCharactedStringType: kw_IA5String  */
#line 746 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_IA5String,
				     TE_EXPLICIT, new_type(TIA5String));
		}
#line 2299 "asn1parse.c"
    break;

  case 104: /* RestrictedCharactedStringType: kw_BMPString  */
#line 751 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_BMPString,
				     TE_EXPLICIT, new_type(TBMPString));
		}
#line 2308 "asn1parse.c"
    break;

  case 105: /* RestrictedCharactedStringType: kw_UniversalString  */
#line 756 "asn1parse.y"
                {
			(yyval.type) = new_tag(ASN1_C_UNIV, UT_UniversalString,
				     TE_EXPLICIT, new_type(TUniversalString));
		}
#line 2317 "asn1parse.c"
    break;

  case 106: /* ComponentTypeList: ComponentType  */
#line 764 "asn1parse.y"
                {
			(yyval.members) = emalloc(sizeof(*(yyval.members)));
			ASN1_TAILQ_INIT((yyval.members));
			ASN1_TAILQ_INSERT_HEAD((yyval.members), (yyvsp[0].member), members);
		}
#line 2327 "asn1parse.c"
    break;

  case 107: /* ComponentTypeList: ComponentTypeList ',' ComponentType  */
#line 770 "asn1parse.y"
                {
			ASN1_TAILQ_INSERT_TAIL((yyvsp[-2].members), (yyvsp[0].member), members);
			(yyval.members) = (yyvsp[-2].members);
		}
#line 2336 "asn1parse.c"
    break;

  case 108: /* ComponentTypeList: ComponentTypeList ',' ELLIPSIS  */
#line 775 "asn1parse.y"
                {
		        struct member *m = ecalloc(1, sizeof(*m));
			m->name = estrdup("...");
			m->gen_name = estrdup("asn1_ellipsis");
			m->ellipsis = 1;
			ASN1_TAILQ_INSERT_TAIL((yyvsp[-2].members), m, members);
			(yyval.members) = (yyvsp[-2].members);
		}
#line 2349 "asn1parse.c"
    break;

  case 109: /* NamedType: IDENTIFIER Type  */
#line 786 "asn1parse.y"
                {
		  (yyval.member) = emalloc(sizeof(*(yyval.member)));
		  (yyval.member)->name = (yyvsp[-1].name);
		  (yyval.member)->gen_name = estrdup((yyvsp[-1].name));
		  output_name ((yyval.member)->gen_name);
		  (yyval.member)->type = (yyvsp[0].type);
		  (yyval.member)->ellipsis = 0;
		}
#line 2362 "asn1parse.c"
    break;

  case 110: /* ComponentType: NamedType  */
#line 797 "asn1parse.y"
                {
			(yyval.member) = (yyvsp[0].member);
			(yyval.member)->optional = 0;
			(yyval.member)->defval = NULL;
		}
#line 2372 "asn1parse.c"
    break;

  case 111: /* ComponentType: NamedType kw_OPTIONAL  */
#line 803 "asn1parse.y"
                {
			(yyval.member) = (yyvsp[-1].member);
			(yyval.member)->optional = 1;
			(yyval.member)->defval = NULL;
		}
#line 2382 "asn1parse.c"
    break;

  case 112: /* ComponentType: NamedType kw_DEFAULT Value  */
#line 809 "asn1parse.y"
                {
			(yyval.member) = (yyvsp[-2].member);
			(yyval.member)->optional = 0;
			(yyval.member)->defval = (yyvsp[0].value);
		}
#line 2392 "asn1parse.c"
    break;

  case 113: /* NamedBitList: NamedBit  */
#line 817 "asn1parse.y"
                {
			(yyval.members) = emalloc(sizeof(*(yyval.members)));
			ASN1_TAILQ_INIT((yyval.members));
			ASN1_TAILQ_INSERT_HEAD((yyval.members), (yyvsp[0].member), members);
		}
#line 2402 "asn1parse.c"
    break;

  case 114: /* NamedBitList: NamedBitList ',' NamedBit  */
#line 823 "asn1parse.y"
                {
			ASN1_TAILQ_INSERT_TAIL((yyvsp[-2].members), (yyvsp[0].member), members);
			(yyval.members) = (yyvsp[-2].members);
		}
#line 2411 "asn1parse.c"
    break;

  case 115: /* NamedBit: IDENTIFIER '(' NUMBER ')'  */
#line 830 "asn1parse.y"
                {
		  (yyval.member) = emalloc(sizeof(*(yyval.member)));
		  (yyval.member)->name = (yyvsp[-3].name);
		  (yyval.member)->gen_name = estrdup((yyvsp[-3].name));
		  output_name ((yyval.member)->gen_name);
		  (yyval.member)->val = (yyvsp[-1].constant);
		  (yyval.member)->optional = 0;
		  (yyval.member)->ellipsis = 0;
		  (yyval.member)->type = NULL;
		}
#line 2426 "asn1parse.c"
    break;

  case 117: /* objid_opt: %empty  */
#line 843 "asn1parse.y"
                              { (yyval.objid) = NULL; }
#line 2432 "asn1parse.c"
    break;

  case 118: /* objid: '{' objid_list '}'  */
#line 847 "asn1parse.y"
                {
			(yyval.objid) = (yyvsp[-1].objid);
		}
#line 2440 "asn1parse.c"
    break;

  case 119: /* objid_list: %empty  */
#line 853 "asn1parse.y"
                {
			(yyval.objid) = NULL;
		}
#line 2448 "asn1parse.c"
    break;

  case 120: /* objid_list: objid_element objid_list  */
#line 857 "asn1parse.y"
                {
		        if ((yyvsp[0].objid)) {
				(yyval.objid) = (yyvsp[0].objid);
				add_oid_to_tail((yyvsp[0].objid), (yyvsp[-1].objid));
			} else {
				(yyval.objid) = (yyvsp[-1].objid);
			}
		}
#line 2461 "asn1parse.c"
    break;

  case 121: /* objid_element: IDENTIFIER '(' NUMBER ')'  */
#line 868 "asn1parse.y"
                {
			(yyval.objid) = new_objid((yyvsp[-3].name), (yyvsp[-1].constant));
		}
#line 2469 "asn1parse.c"
    break;

  case 122: /* objid_element: IDENTIFIER  */
#line 872 "asn1parse.y"
                {
		    Symbol *s = addsym((yyvsp[0].name));
		    if(s->stype != SValue ||
		       s->value->type != objectidentifiervalue) {
			lex_error_message("%s is not an object identifier\n",
				      s->name);
			exit(1);
		    }
		    (yyval.objid) = s->value->u.objectidentifiervalue;
		}
#line 2484 "asn1parse.c"
    break;

  case 123: /* objid_element: NUMBER  */
#line 883 "asn1parse.y"
                {
		    (yyval.objid) = new_objid(NULL, (yyvsp[0].constant));
		}
#line 2492 "asn1parse.c"
    break;

  case 133: /* Valuereference: IDENTIFIER  */
#line 906 "asn1parse.y"
                {
			Symbol *s = addsym((yyvsp[0].name));
			if(s->stype != SValue)
				lex_error_message ("%s is not a value\n",
						s->name);
			else
				(yyval.value) = s->value;
		}
#line 2505 "asn1parse.c"
    break;

  case 134: /* CharacterStringValue: STRING  */
#line 917 "asn1parse.y"
                {
			(yyval.value) = emalloc(sizeof(*(yyval.value)));
			(yyval.value)->type = stringvalue;
			(yyval.value)->u.stringvalue = (yyvsp[0].name);
		}
#line 2515 "asn1parse.c"
    break;

  case 135: /* BooleanValue: kw_TRUE  */
#line 925 "asn1parse.y"
                {
			(yyval.value) = emalloc(sizeof(*(yyval.value)));
			(yyval.value)->type = booleanvalue;
			(yyval.value)->u.booleanvalue = 0;
		}
#line 2525 "asn1parse.c"
    break;

  case 136: /* BooleanValue: kw_FALSE  */
#line 931 "asn1parse.y"
                {
			(yyval.value) = emalloc(sizeof(*(yyval.value)));
			(yyval.value)->type = booleanvalue;
			(yyval.value)->u.booleanvalue = 0;
		}
#line 2535 "asn1parse.c"
    break;

  case 137: /* IntegerValue: SignedNumber  */
#line 939 "asn1parse.y"
                {
			(yyval.value) = emalloc(sizeof(*(yyval.value)));
			(yyval.value)->type = integervalue;
			(yyval.value)->u.integervalue = (yyvsp[0].constant);
		}
#line 2545 "asn1parse.c"
    break;

  case 139: /* NullValue: kw_NULL  */
#line 950 "asn1parse.y"
                {
		}
#line 2552 "asn1parse.c"
    break;

  case 140: /* ObjectIdentifierValue: objid  */
#line 955 "asn1parse.y"
                {
			(yyval.value) = emalloc(sizeof(*(yyval.value)));
			(yyval.value)->type = objectidentifiervalue;
			(yyval.value)->u.objectidentifiervalue = (yyvsp[0].objid);
		}
#line 2562 "asn1parse.c"
    break;


#line 2566 "asn1parse.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 962 "asn1parse.y"


void
yyerror (const char *s)
{
     lex_error_message ("%s\n", s);
}

static Type *
new_tag(int tagclass, int tagvalue, int tagenv, Type *oldtype)
{
    Type *t;
    if(oldtype->type == TTag && oldtype->tag.tagenv == TE_IMPLICIT) {
	t = oldtype;
	oldtype = oldtype->subtype; /* XXX */
    } else
	t = new_type (TTag);

    t->tag.tagclass = tagclass;
    t->tag.tagvalue = tagvalue;
    t->tag.tagenv = tagenv;
    t->subtype = oldtype;
    return t;
}

static struct objid *
new_objid(const char *label, int value)
{
    struct objid *s;
    s = emalloc(sizeof(*s));
    s->label = label;
    s->value = value;
    s->next = NULL;
    return s;
}

static void
add_oid_to_tail(struct objid *head, struct objid *tail)
{
    struct objid *o;
    o = head;
    while (o->next)
	o = o->next;
    o->next = tail;
}

static unsigned long idcounter;

static Type *
new_type (Typetype tt)
{
    Type *t = ecalloc(1, sizeof(*t));
    t->type = tt;
    t->id = idcounter++;
    return t;
}

static struct constraint_spec *
new_constraint_spec(enum ctype ct)
{
    struct constraint_spec *c = ecalloc(1, sizeof(*c));
    c->ctype = ct;
    return c;
}

static void fix_labels2(Type *t, const char *prefix);
static void fix_labels1(struct memhead *members, const char *prefix)
{
    Member *m;

    if(members == NULL)
	return;
    ASN1_TAILQ_FOREACH(m, members, members) {
	if (asprintf(&m->label, "%s_%s", prefix, m->gen_name) < 0)
	    errx(1, "malloc");
	if (m->label == NULL)
	    errx(1, "malloc");
	if(m->type != NULL)
	    fix_labels2(m->type, m->label);
    }
}

static void fix_labels2(Type *t, const char *prefix)
{
    for(; t; t = t->subtype)
	fix_labels1(t->members, prefix);
}

static void
fix_labels(Symbol *s)
{
    char *p = NULL;
    if (asprintf(&p, "choice_%s", s->gen_name) < 0 || p == NULL)
	errx(1, "malloc");
    fix_labels2(s->type, p);
    free(p);
}
