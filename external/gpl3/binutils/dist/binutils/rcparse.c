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
#line 1 "rcparse.y"
 /* rcparse.y -- parser for Windows rc files
   Copyright (C) 1997-2022 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support.
   Extended by Kai Tietz, Onevision.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */


/* This is a parser for Windows rc files.  It is based on the parser
   by Gunther Ebert <gunther.ebert@ixos-leipzig.de>.  */

#include "sysdep.h"
#include "bfd.h"
#include "bucomm.h"
#include "libiberty.h"
#include "windres.h"
#include "safe-ctype.h"

/* The current language.  */

static unsigned short language;

/* The resource information during a sub statement.  */

static rc_res_res_info sub_res_info;

/* Dialog information.  This is built by the nonterminals styles and
   controls.  */

static rc_dialog dialog;

/* This is used when building a style.  It is modified by the
   nonterminal styleexpr.  */

static unsigned long style;

/* These are used when building a control.  They are set before using
   control_params.  */

static rc_uint_type base_style;
static rc_uint_type default_style;
static rc_res_id class;
static rc_res_id res_text_field;
static unichar null_unichar;

/* This is used for COMBOBOX, LISTBOX and EDITTEXT which
   do not allow resource 'text' field in control definition. */
static const rc_res_id res_null_text = { 1, {{0, &null_unichar}}};


#line 137 "rcparse.c"

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
#ifndef YY_YY_RCPARSE_H_INCLUDED
# define YY_YY_RCPARSE_H_INCLUDED
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
    BEG = 258,                     /* BEG  */
    END = 259,                     /* END  */
    ACCELERATORS = 260,            /* ACCELERATORS  */
    VIRTKEY = 261,                 /* VIRTKEY  */
    ASCII = 262,                   /* ASCII  */
    NOINVERT = 263,                /* NOINVERT  */
    SHIFT = 264,                   /* SHIFT  */
    CONTROL = 265,                 /* CONTROL  */
    ALT = 266,                     /* ALT  */
    BITMAP = 267,                  /* BITMAP  */
    CURSOR = 268,                  /* CURSOR  */
    DIALOG = 269,                  /* DIALOG  */
    DIALOGEX = 270,                /* DIALOGEX  */
    EXSTYLE = 271,                 /* EXSTYLE  */
    CAPTION = 272,                 /* CAPTION  */
    CLASS = 273,                   /* CLASS  */
    STYLE = 274,                   /* STYLE  */
    AUTO3STATE = 275,              /* AUTO3STATE  */
    AUTOCHECKBOX = 276,            /* AUTOCHECKBOX  */
    AUTORADIOBUTTON = 277,         /* AUTORADIOBUTTON  */
    CHECKBOX = 278,                /* CHECKBOX  */
    COMBOBOX = 279,                /* COMBOBOX  */
    CTEXT = 280,                   /* CTEXT  */
    DEFPUSHBUTTON = 281,           /* DEFPUSHBUTTON  */
    EDITTEXT = 282,                /* EDITTEXT  */
    GROUPBOX = 283,                /* GROUPBOX  */
    LISTBOX = 284,                 /* LISTBOX  */
    LTEXT = 285,                   /* LTEXT  */
    PUSHBOX = 286,                 /* PUSHBOX  */
    PUSHBUTTON = 287,              /* PUSHBUTTON  */
    RADIOBUTTON = 288,             /* RADIOBUTTON  */
    RTEXT = 289,                   /* RTEXT  */
    SCROLLBAR = 290,               /* SCROLLBAR  */
    STATE3 = 291,                  /* STATE3  */
    USERBUTTON = 292,              /* USERBUTTON  */
    BEDIT = 293,                   /* BEDIT  */
    HEDIT = 294,                   /* HEDIT  */
    IEDIT = 295,                   /* IEDIT  */
    FONT = 296,                    /* FONT  */
    ICON = 297,                    /* ICON  */
    ANICURSOR = 298,               /* ANICURSOR  */
    ANIICON = 299,                 /* ANIICON  */
    DLGINCLUDE = 300,              /* DLGINCLUDE  */
    DLGINIT = 301,                 /* DLGINIT  */
    FONTDIR = 302,                 /* FONTDIR  */
    HTML = 303,                    /* HTML  */
    MANIFEST = 304,                /* MANIFEST  */
    PLUGPLAY = 305,                /* PLUGPLAY  */
    VXD = 306,                     /* VXD  */
    TOOLBAR = 307,                 /* TOOLBAR  */
    BUTTON = 308,                  /* BUTTON  */
    LANGUAGE = 309,                /* LANGUAGE  */
    CHARACTERISTICS = 310,         /* CHARACTERISTICS  */
    VERSIONK = 311,                /* VERSIONK  */
    MENU = 312,                    /* MENU  */
    MENUEX = 313,                  /* MENUEX  */
    MENUITEM = 314,                /* MENUITEM  */
    SEPARATOR = 315,               /* SEPARATOR  */
    POPUP = 316,                   /* POPUP  */
    CHECKED = 317,                 /* CHECKED  */
    GRAYED = 318,                  /* GRAYED  */
    HELP = 319,                    /* HELP  */
    INACTIVE = 320,                /* INACTIVE  */
    OWNERDRAW = 321,               /* OWNERDRAW  */
    MENUBARBREAK = 322,            /* MENUBARBREAK  */
    MENUBREAK = 323,               /* MENUBREAK  */
    MESSAGETABLE = 324,            /* MESSAGETABLE  */
    RCDATA = 325,                  /* RCDATA  */
    STRINGTABLE = 326,             /* STRINGTABLE  */
    VERSIONINFO = 327,             /* VERSIONINFO  */
    FILEVERSION = 328,             /* FILEVERSION  */
    PRODUCTVERSION = 329,          /* PRODUCTVERSION  */
    FILEFLAGSMASK = 330,           /* FILEFLAGSMASK  */
    FILEFLAGS = 331,               /* FILEFLAGS  */
    FILEOS = 332,                  /* FILEOS  */
    FILETYPE = 333,                /* FILETYPE  */
    FILESUBTYPE = 334,             /* FILESUBTYPE  */
    BLOCKSTRINGFILEINFO = 335,     /* BLOCKSTRINGFILEINFO  */
    BLOCKVARFILEINFO = 336,        /* BLOCKVARFILEINFO  */
    VALUE = 337,                   /* VALUE  */
    BLOCK = 338,                   /* BLOCK  */
    MOVEABLE = 339,                /* MOVEABLE  */
    FIXED = 340,                   /* FIXED  */
    PURE = 341,                    /* PURE  */
    IMPURE = 342,                  /* IMPURE  */
    PRELOAD = 343,                 /* PRELOAD  */
    LOADONCALL = 344,              /* LOADONCALL  */
    DISCARDABLE = 345,             /* DISCARDABLE  */
    NOT = 346,                     /* NOT  */
    QUOTEDUNISTRING = 347,         /* QUOTEDUNISTRING  */
    QUOTEDSTRING = 348,            /* QUOTEDSTRING  */
    STRING = 349,                  /* STRING  */
    NUMBER = 350,                  /* NUMBER  */
    SIZEDUNISTRING = 351,          /* SIZEDUNISTRING  */
    SIZEDSTRING = 352,             /* SIZEDSTRING  */
    IGNORED_TOKEN = 353,           /* IGNORED_TOKEN  */
    NEG = 354                      /* NEG  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define BEG 258
#define END 259
#define ACCELERATORS 260
#define VIRTKEY 261
#define ASCII 262
#define NOINVERT 263
#define SHIFT 264
#define CONTROL 265
#define ALT 266
#define BITMAP 267
#define CURSOR 268
#define DIALOG 269
#define DIALOGEX 270
#define EXSTYLE 271
#define CAPTION 272
#define CLASS 273
#define STYLE 274
#define AUTO3STATE 275
#define AUTOCHECKBOX 276
#define AUTORADIOBUTTON 277
#define CHECKBOX 278
#define COMBOBOX 279
#define CTEXT 280
#define DEFPUSHBUTTON 281
#define EDITTEXT 282
#define GROUPBOX 283
#define LISTBOX 284
#define LTEXT 285
#define PUSHBOX 286
#define PUSHBUTTON 287
#define RADIOBUTTON 288
#define RTEXT 289
#define SCROLLBAR 290
#define STATE3 291
#define USERBUTTON 292
#define BEDIT 293
#define HEDIT 294
#define IEDIT 295
#define FONT 296
#define ICON 297
#define ANICURSOR 298
#define ANIICON 299
#define DLGINCLUDE 300
#define DLGINIT 301
#define FONTDIR 302
#define HTML 303
#define MANIFEST 304
#define PLUGPLAY 305
#define VXD 306
#define TOOLBAR 307
#define BUTTON 308
#define LANGUAGE 309
#define CHARACTERISTICS 310
#define VERSIONK 311
#define MENU 312
#define MENUEX 313
#define MENUITEM 314
#define SEPARATOR 315
#define POPUP 316
#define CHECKED 317
#define GRAYED 318
#define HELP 319
#define INACTIVE 320
#define OWNERDRAW 321
#define MENUBARBREAK 322
#define MENUBREAK 323
#define MESSAGETABLE 324
#define RCDATA 325
#define STRINGTABLE 326
#define VERSIONINFO 327
#define FILEVERSION 328
#define PRODUCTVERSION 329
#define FILEFLAGSMASK 330
#define FILEFLAGS 331
#define FILEOS 332
#define FILETYPE 333
#define FILESUBTYPE 334
#define BLOCKSTRINGFILEINFO 335
#define BLOCKVARFILEINFO 336
#define VALUE 337
#define BLOCK 338
#define MOVEABLE 339
#define FIXED 340
#define PURE 341
#define IMPURE 342
#define PRELOAD 343
#define LOADONCALL 344
#define DISCARDABLE 345
#define NOT 346
#define QUOTEDUNISTRING 347
#define QUOTEDSTRING 348
#define STRING 349
#define NUMBER 350
#define SIZEDUNISTRING 351
#define SIZEDSTRING 352
#define IGNORED_TOKEN 353
#define NEG 354

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 68 "rcparse.y"

  rc_accelerator acc;
  rc_accelerator *pacc;
  rc_dialog_control *dialog_control;
  rc_menuitem *menuitem;
  struct
  {
    rc_rcdata_item *first;
    rc_rcdata_item *last;
  } rcdata;
  rc_rcdata_item *rcdata_item;
  rc_fixed_versioninfo *fixver;
  rc_ver_info *verinfo;
  rc_ver_stringtable *verstringtable;
  rc_ver_stringinfo *verstring;
  rc_ver_varinfo *vervar;
  rc_toolbar_item *toobar_item;
  rc_res_id id;
  rc_res_res_info res_info;
  struct
  {
    rc_uint_type on;
    rc_uint_type off;
  } memflags;
  struct
  {
    rc_uint_type val;
    /* Nonzero if this number was explicitly specified as long.  */
    int dword;
  } i;
  rc_uint_type il;
  rc_uint_type is;
  const char *s;
  struct
  {
    rc_uint_type length;
    const char *s;
  } ss;
  unichar *uni;
  struct
  {
    rc_uint_type length;
    const unichar *s;
  } suni;

#line 434 "rcparse.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_RCPARSE_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_BEG = 3,                        /* BEG  */
  YYSYMBOL_END = 4,                        /* END  */
  YYSYMBOL_ACCELERATORS = 5,               /* ACCELERATORS  */
  YYSYMBOL_VIRTKEY = 6,                    /* VIRTKEY  */
  YYSYMBOL_ASCII = 7,                      /* ASCII  */
  YYSYMBOL_NOINVERT = 8,                   /* NOINVERT  */
  YYSYMBOL_SHIFT = 9,                      /* SHIFT  */
  YYSYMBOL_CONTROL = 10,                   /* CONTROL  */
  YYSYMBOL_ALT = 11,                       /* ALT  */
  YYSYMBOL_BITMAP = 12,                    /* BITMAP  */
  YYSYMBOL_CURSOR = 13,                    /* CURSOR  */
  YYSYMBOL_DIALOG = 14,                    /* DIALOG  */
  YYSYMBOL_DIALOGEX = 15,                  /* DIALOGEX  */
  YYSYMBOL_EXSTYLE = 16,                   /* EXSTYLE  */
  YYSYMBOL_CAPTION = 17,                   /* CAPTION  */
  YYSYMBOL_CLASS = 18,                     /* CLASS  */
  YYSYMBOL_STYLE = 19,                     /* STYLE  */
  YYSYMBOL_AUTO3STATE = 20,                /* AUTO3STATE  */
  YYSYMBOL_AUTOCHECKBOX = 21,              /* AUTOCHECKBOX  */
  YYSYMBOL_AUTORADIOBUTTON = 22,           /* AUTORADIOBUTTON  */
  YYSYMBOL_CHECKBOX = 23,                  /* CHECKBOX  */
  YYSYMBOL_COMBOBOX = 24,                  /* COMBOBOX  */
  YYSYMBOL_CTEXT = 25,                     /* CTEXT  */
  YYSYMBOL_DEFPUSHBUTTON = 26,             /* DEFPUSHBUTTON  */
  YYSYMBOL_EDITTEXT = 27,                  /* EDITTEXT  */
  YYSYMBOL_GROUPBOX = 28,                  /* GROUPBOX  */
  YYSYMBOL_LISTBOX = 29,                   /* LISTBOX  */
  YYSYMBOL_LTEXT = 30,                     /* LTEXT  */
  YYSYMBOL_PUSHBOX = 31,                   /* PUSHBOX  */
  YYSYMBOL_PUSHBUTTON = 32,                /* PUSHBUTTON  */
  YYSYMBOL_RADIOBUTTON = 33,               /* RADIOBUTTON  */
  YYSYMBOL_RTEXT = 34,                     /* RTEXT  */
  YYSYMBOL_SCROLLBAR = 35,                 /* SCROLLBAR  */
  YYSYMBOL_STATE3 = 36,                    /* STATE3  */
  YYSYMBOL_USERBUTTON = 37,                /* USERBUTTON  */
  YYSYMBOL_BEDIT = 38,                     /* BEDIT  */
  YYSYMBOL_HEDIT = 39,                     /* HEDIT  */
  YYSYMBOL_IEDIT = 40,                     /* IEDIT  */
  YYSYMBOL_FONT = 41,                      /* FONT  */
  YYSYMBOL_ICON = 42,                      /* ICON  */
  YYSYMBOL_ANICURSOR = 43,                 /* ANICURSOR  */
  YYSYMBOL_ANIICON = 44,                   /* ANIICON  */
  YYSYMBOL_DLGINCLUDE = 45,                /* DLGINCLUDE  */
  YYSYMBOL_DLGINIT = 46,                   /* DLGINIT  */
  YYSYMBOL_FONTDIR = 47,                   /* FONTDIR  */
  YYSYMBOL_HTML = 48,                      /* HTML  */
  YYSYMBOL_MANIFEST = 49,                  /* MANIFEST  */
  YYSYMBOL_PLUGPLAY = 50,                  /* PLUGPLAY  */
  YYSYMBOL_VXD = 51,                       /* VXD  */
  YYSYMBOL_TOOLBAR = 52,                   /* TOOLBAR  */
  YYSYMBOL_BUTTON = 53,                    /* BUTTON  */
  YYSYMBOL_LANGUAGE = 54,                  /* LANGUAGE  */
  YYSYMBOL_CHARACTERISTICS = 55,           /* CHARACTERISTICS  */
  YYSYMBOL_VERSIONK = 56,                  /* VERSIONK  */
  YYSYMBOL_MENU = 57,                      /* MENU  */
  YYSYMBOL_MENUEX = 58,                    /* MENUEX  */
  YYSYMBOL_MENUITEM = 59,                  /* MENUITEM  */
  YYSYMBOL_SEPARATOR = 60,                 /* SEPARATOR  */
  YYSYMBOL_POPUP = 61,                     /* POPUP  */
  YYSYMBOL_CHECKED = 62,                   /* CHECKED  */
  YYSYMBOL_GRAYED = 63,                    /* GRAYED  */
  YYSYMBOL_HELP = 64,                      /* HELP  */
  YYSYMBOL_INACTIVE = 65,                  /* INACTIVE  */
  YYSYMBOL_OWNERDRAW = 66,                 /* OWNERDRAW  */
  YYSYMBOL_MENUBARBREAK = 67,              /* MENUBARBREAK  */
  YYSYMBOL_MENUBREAK = 68,                 /* MENUBREAK  */
  YYSYMBOL_MESSAGETABLE = 69,              /* MESSAGETABLE  */
  YYSYMBOL_RCDATA = 70,                    /* RCDATA  */
  YYSYMBOL_STRINGTABLE = 71,               /* STRINGTABLE  */
  YYSYMBOL_VERSIONINFO = 72,               /* VERSIONINFO  */
  YYSYMBOL_FILEVERSION = 73,               /* FILEVERSION  */
  YYSYMBOL_PRODUCTVERSION = 74,            /* PRODUCTVERSION  */
  YYSYMBOL_FILEFLAGSMASK = 75,             /* FILEFLAGSMASK  */
  YYSYMBOL_FILEFLAGS = 76,                 /* FILEFLAGS  */
  YYSYMBOL_FILEOS = 77,                    /* FILEOS  */
  YYSYMBOL_FILETYPE = 78,                  /* FILETYPE  */
  YYSYMBOL_FILESUBTYPE = 79,               /* FILESUBTYPE  */
  YYSYMBOL_BLOCKSTRINGFILEINFO = 80,       /* BLOCKSTRINGFILEINFO  */
  YYSYMBOL_BLOCKVARFILEINFO = 81,          /* BLOCKVARFILEINFO  */
  YYSYMBOL_VALUE = 82,                     /* VALUE  */
  YYSYMBOL_BLOCK = 83,                     /* BLOCK  */
  YYSYMBOL_MOVEABLE = 84,                  /* MOVEABLE  */
  YYSYMBOL_FIXED = 85,                     /* FIXED  */
  YYSYMBOL_PURE = 86,                      /* PURE  */
  YYSYMBOL_IMPURE = 87,                    /* IMPURE  */
  YYSYMBOL_PRELOAD = 88,                   /* PRELOAD  */
  YYSYMBOL_LOADONCALL = 89,                /* LOADONCALL  */
  YYSYMBOL_DISCARDABLE = 90,               /* DISCARDABLE  */
  YYSYMBOL_NOT = 91,                       /* NOT  */
  YYSYMBOL_QUOTEDUNISTRING = 92,           /* QUOTEDUNISTRING  */
  YYSYMBOL_QUOTEDSTRING = 93,              /* QUOTEDSTRING  */
  YYSYMBOL_STRING = 94,                    /* STRING  */
  YYSYMBOL_NUMBER = 95,                    /* NUMBER  */
  YYSYMBOL_SIZEDUNISTRING = 96,            /* SIZEDUNISTRING  */
  YYSYMBOL_SIZEDSTRING = 97,               /* SIZEDSTRING  */
  YYSYMBOL_IGNORED_TOKEN = 98,             /* IGNORED_TOKEN  */
  YYSYMBOL_99_ = 99,                       /* '|'  */
  YYSYMBOL_100_ = 100,                     /* '^'  */
  YYSYMBOL_101_ = 101,                     /* '&'  */
  YYSYMBOL_102_ = 102,                     /* '+'  */
  YYSYMBOL_103_ = 103,                     /* '-'  */
  YYSYMBOL_104_ = 104,                     /* '*'  */
  YYSYMBOL_105_ = 105,                     /* '/'  */
  YYSYMBOL_106_ = 106,                     /* '%'  */
  YYSYMBOL_107_ = 107,                     /* '~'  */
  YYSYMBOL_NEG = 108,                      /* NEG  */
  YYSYMBOL_109_ = 109,                     /* ','  */
  YYSYMBOL_110_ = 110,                     /* '='  */
  YYSYMBOL_111_ = 111,                     /* '('  */
  YYSYMBOL_112_ = 112,                     /* ')'  */
  YYSYMBOL_YYACCEPT = 113,                 /* $accept  */
  YYSYMBOL_input = 114,                    /* input  */
  YYSYMBOL_accelerator = 115,              /* accelerator  */
  YYSYMBOL_acc_entries = 116,              /* acc_entries  */
  YYSYMBOL_acc_entry = 117,                /* acc_entry  */
  YYSYMBOL_acc_event = 118,                /* acc_event  */
  YYSYMBOL_acc_options = 119,              /* acc_options  */
  YYSYMBOL_acc_option = 120,               /* acc_option  */
  YYSYMBOL_bitmap = 121,                   /* bitmap  */
  YYSYMBOL_cursor = 122,                   /* cursor  */
  YYSYMBOL_dialog = 123,                   /* dialog  */
  YYSYMBOL_124_1 = 124,                    /* $@1  */
  YYSYMBOL_125_2 = 125,                    /* $@2  */
  YYSYMBOL_126_3 = 126,                    /* $@3  */
  YYSYMBOL_exstyle = 127,                  /* exstyle  */
  YYSYMBOL_styles = 128,                   /* styles  */
  YYSYMBOL_controls = 129,                 /* controls  */
  YYSYMBOL_control = 130,                  /* control  */
  YYSYMBOL_131_4 = 131,                    /* $@4  */
  YYSYMBOL_132_5 = 132,                    /* $@5  */
  YYSYMBOL_133_6 = 133,                    /* $@6  */
  YYSYMBOL_134_7 = 134,                    /* $@7  */
  YYSYMBOL_135_8 = 135,                    /* $@8  */
  YYSYMBOL_136_9 = 136,                    /* $@9  */
  YYSYMBOL_137_10 = 137,                   /* $@10  */
  YYSYMBOL_138_11 = 138,                   /* $@11  */
  YYSYMBOL_139_12 = 139,                   /* $@12  */
  YYSYMBOL_140_13 = 140,                   /* $@13  */
  YYSYMBOL_141_14 = 141,                   /* $@14  */
  YYSYMBOL_142_15 = 142,                   /* $@15  */
  YYSYMBOL_143_16 = 143,                   /* $@16  */
  YYSYMBOL_144_17 = 144,                   /* $@17  */
  YYSYMBOL_145_18 = 145,                   /* $@18  */
  YYSYMBOL_146_19 = 146,                   /* $@19  */
  YYSYMBOL_147_20 = 147,                   /* $@20  */
  YYSYMBOL_148_21 = 148,                   /* $@21  */
  YYSYMBOL_149_22 = 149,                   /* $@22  */
  YYSYMBOL_150_23 = 150,                   /* $@23  */
  YYSYMBOL_151_24 = 151,                   /* $@24  */
  YYSYMBOL_control_params = 152,           /* control_params  */
  YYSYMBOL_cresid = 153,                   /* cresid  */
  YYSYMBOL_optresidc = 154,                /* optresidc  */
  YYSYMBOL_resid = 155,                    /* resid  */
  YYSYMBOL_opt_control_data = 156,         /* opt_control_data  */
  YYSYMBOL_control_styleexpr = 157,        /* control_styleexpr  */
  YYSYMBOL_158_25 = 158,                   /* $@25  */
  YYSYMBOL_icon_styleexpr = 159,           /* icon_styleexpr  */
  YYSYMBOL_160_26 = 160,                   /* $@26  */
  YYSYMBOL_control_params_styleexpr = 161, /* control_params_styleexpr  */
  YYSYMBOL_162_27 = 162,                   /* $@27  */
  YYSYMBOL_font = 163,                     /* font  */
  YYSYMBOL_icon = 164,                     /* icon  */
  YYSYMBOL_language = 165,                 /* language  */
  YYSYMBOL_menu = 166,                     /* menu  */
  YYSYMBOL_menuitems = 167,                /* menuitems  */
  YYSYMBOL_menuitem = 168,                 /* menuitem  */
  YYSYMBOL_menuitem_flags = 169,           /* menuitem_flags  */
  YYSYMBOL_menuitem_flag = 170,            /* menuitem_flag  */
  YYSYMBOL_menuex = 171,                   /* menuex  */
  YYSYMBOL_menuexitems = 172,              /* menuexitems  */
  YYSYMBOL_menuexitem = 173,               /* menuexitem  */
  YYSYMBOL_messagetable = 174,             /* messagetable  */
  YYSYMBOL_optrcdata_data = 175,           /* optrcdata_data  */
  YYSYMBOL_176_28 = 176,                   /* $@28  */
  YYSYMBOL_optrcdata_data_int = 177,       /* optrcdata_data_int  */
  YYSYMBOL_rcdata_data = 178,              /* rcdata_data  */
  YYSYMBOL_stringtable = 179,              /* stringtable  */
  YYSYMBOL_180_29 = 180,                   /* $@29  */
  YYSYMBOL_string_data = 181,              /* string_data  */
  YYSYMBOL_rcdata_id = 182,                /* rcdata_id  */
  YYSYMBOL_user = 183,                     /* user  */
  YYSYMBOL_toolbar = 184,                  /* toolbar  */
  YYSYMBOL_toolbar_data = 185,             /* toolbar_data  */
  YYSYMBOL_versioninfo = 186,              /* versioninfo  */
  YYSYMBOL_fixedverinfo = 187,             /* fixedverinfo  */
  YYSYMBOL_verblocks = 188,                /* verblocks  */
  YYSYMBOL_verstringtables = 189,          /* verstringtables  */
  YYSYMBOL_vervals = 190,                  /* vervals  */
  YYSYMBOL_vertrans = 191,                 /* vertrans  */
  YYSYMBOL_id = 192,                       /* id  */
  YYSYMBOL_resname = 193,                  /* resname  */
  YYSYMBOL_resref = 194,                   /* resref  */
  YYSYMBOL_suboptions = 195,               /* suboptions  */
  YYSYMBOL_memflags_move_discard = 196,    /* memflags_move_discard  */
  YYSYMBOL_memflags_move = 197,            /* memflags_move  */
  YYSYMBOL_memflag = 198,                  /* memflag  */
  YYSYMBOL_file_name = 199,                /* file_name  */
  YYSYMBOL_res_unicode_string_concat = 200, /* res_unicode_string_concat  */
  YYSYMBOL_res_unicode_string = 201,       /* res_unicode_string  */
  YYSYMBOL_res_unicode_sizedstring = 202,  /* res_unicode_sizedstring  */
  YYSYMBOL_res_unicode_sizedstring_concat = 203, /* res_unicode_sizedstring_concat  */
  YYSYMBOL_sizedstring = 204,              /* sizedstring  */
  YYSYMBOL_sizedunistring = 205,           /* sizedunistring  */
  YYSYMBOL_styleexpr = 206,                /* styleexpr  */
  YYSYMBOL_parennumber = 207,              /* parennumber  */
  YYSYMBOL_optcnumexpr = 208,              /* optcnumexpr  */
  YYSYMBOL_cnumexpr = 209,                 /* cnumexpr  */
  YYSYMBOL_numexpr = 210,                  /* numexpr  */
  YYSYMBOL_sizednumexpr = 211,             /* sizednumexpr  */
  YYSYMBOL_cposnumexpr = 212,              /* cposnumexpr  */
  YYSYMBOL_posnumexpr = 213,               /* posnumexpr  */
  YYSYMBOL_sizedposnumexpr = 214           /* sizedposnumexpr  */
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
typedef yytype_int16 yy_state_t;

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   842

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  113
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  102
/* YYNRULES -- Number of rules.  */
#define YYNRULES  278
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  522

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   354


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
       2,     2,     2,     2,     2,     2,     2,   106,   101,     2,
     111,   112,   104,   102,   109,   103,     2,   105,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   110,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   100,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    99,     2,   107,     2,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,   108
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   178,   178,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   200,   211,
     214,   235,   240,   252,   272,   282,   286,   291,   298,   302,
     307,   311,   315,   319,   328,   340,   354,   352,   379,   377,
     406,   404,   436,   439,   445,   447,   453,   457,   462,   466,
     470,   483,   498,   513,   528,   532,   536,   540,   546,   548,
     560,   559,   572,   571,   584,   583,   596,   595,   611,   610,
     623,   622,   636,   647,   657,   656,   669,   668,   681,   680,
     693,   692,   705,   704,   719,   724,   730,   736,   743,   742,
     758,   757,   770,   769,   782,   781,   793,   792,   805,   804,
     817,   816,   829,   828,   841,   840,   854,   852,   873,   884,
     895,   907,   918,   921,   925,   930,   940,   943,   953,   952,
     959,   958,   965,   964,   972,   984,   997,  1006,  1017,  1020,
    1037,  1041,  1045,  1053,  1056,  1060,  1067,  1071,  1075,  1079,
    1083,  1087,  1091,  1095,  1104,  1115,  1118,  1135,  1139,  1143,
    1147,  1151,  1155,  1159,  1163,  1173,  1186,  1186,  1198,  1202,
    1209,  1217,  1225,  1233,  1242,  1251,  1260,  1270,  1269,  1274,
    1276,  1281,  1286,  1294,  1298,  1303,  1308,  1313,  1318,  1323,
    1328,  1333,  1338,  1349,  1356,  1366,  1372,  1373,  1392,  1417,
    1428,  1433,  1440,  1447,  1452,  1457,  1462,  1467,  1482,  1485,
    1489,  1497,  1500,  1508,  1511,  1519,  1522,  1531,  1536,  1545,
    1549,  1559,  1564,  1568,  1579,  1585,  1591,  1596,  1601,  1612,
    1617,  1629,  1634,  1646,  1651,  1656,  1661,  1666,  1671,  1676,
    1686,  1690,  1698,  1703,  1718,  1722,  1731,  1735,  1747,  1752,
    1768,  1772,  1784,  1788,  1810,  1814,  1818,  1822,  1829,  1833,
    1843,  1846,  1855,  1864,  1873,  1877,  1881,  1886,  1891,  1896,
    1901,  1906,  1911,  1916,  1921,  1926,  1937,  1946,  1957,  1961,
    1965,  1970,  1975,  1980,  1986,  1991,  1996,  2001,  2006
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
  "\"end of file\"", "error", "\"invalid token\"", "BEG", "END",
  "ACCELERATORS", "VIRTKEY", "ASCII", "NOINVERT", "SHIFT", "CONTROL",
  "ALT", "BITMAP", "CURSOR", "DIALOG", "DIALOGEX", "EXSTYLE", "CAPTION",
  "CLASS", "STYLE", "AUTO3STATE", "AUTOCHECKBOX", "AUTORADIOBUTTON",
  "CHECKBOX", "COMBOBOX", "CTEXT", "DEFPUSHBUTTON", "EDITTEXT", "GROUPBOX",
  "LISTBOX", "LTEXT", "PUSHBOX", "PUSHBUTTON", "RADIOBUTTON", "RTEXT",
  "SCROLLBAR", "STATE3", "USERBUTTON", "BEDIT", "HEDIT", "IEDIT", "FONT",
  "ICON", "ANICURSOR", "ANIICON", "DLGINCLUDE", "DLGINIT", "FONTDIR",
  "HTML", "MANIFEST", "PLUGPLAY", "VXD", "TOOLBAR", "BUTTON", "LANGUAGE",
  "CHARACTERISTICS", "VERSIONK", "MENU", "MENUEX", "MENUITEM", "SEPARATOR",
  "POPUP", "CHECKED", "GRAYED", "HELP", "INACTIVE", "OWNERDRAW",
  "MENUBARBREAK", "MENUBREAK", "MESSAGETABLE", "RCDATA", "STRINGTABLE",
  "VERSIONINFO", "FILEVERSION", "PRODUCTVERSION", "FILEFLAGSMASK",
  "FILEFLAGS", "FILEOS", "FILETYPE", "FILESUBTYPE", "BLOCKSTRINGFILEINFO",
  "BLOCKVARFILEINFO", "VALUE", "BLOCK", "MOVEABLE", "FIXED", "PURE",
  "IMPURE", "PRELOAD", "LOADONCALL", "DISCARDABLE", "NOT",
  "QUOTEDUNISTRING", "QUOTEDSTRING", "STRING", "NUMBER", "SIZEDUNISTRING",
  "SIZEDSTRING", "IGNORED_TOKEN", "'|'", "'^'", "'&'", "'+'", "'-'", "'*'",
  "'/'", "'%'", "'~'", "NEG", "','", "'='", "'('", "')'", "$accept",
  "input", "accelerator", "acc_entries", "acc_entry", "acc_event",
  "acc_options", "acc_option", "bitmap", "cursor", "dialog", "$@1", "$@2",
  "$@3", "exstyle", "styles", "controls", "control", "$@4", "$@5", "$@6",
  "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15",
  "$@16", "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24",
  "control_params", "cresid", "optresidc", "resid", "opt_control_data",
  "control_styleexpr", "$@25", "icon_styleexpr", "$@26",
  "control_params_styleexpr", "$@27", "font", "icon", "language", "menu",
  "menuitems", "menuitem", "menuitem_flags", "menuitem_flag", "menuex",
  "menuexitems", "menuexitem", "messagetable", "optrcdata_data", "$@28",
  "optrcdata_data_int", "rcdata_data", "stringtable", "$@29",
  "string_data", "rcdata_id", "user", "toolbar", "toolbar_data",
  "versioninfo", "fixedverinfo", "verblocks", "verstringtables", "vervals",
  "vertrans", "id", "resname", "resref", "suboptions",
  "memflags_move_discard", "memflags_move", "memflag", "file_name",
  "res_unicode_string_concat", "res_unicode_string",
  "res_unicode_sizedstring", "res_unicode_sizedstring_concat",
  "sizedstring", "sizedunistring", "styleexpr", "parennumber",
  "optcnumexpr", "cnumexpr", "numexpr", "sizednumexpr", "cposnumexpr",
  "posnumexpr", "sizedposnumexpr", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-328)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-233)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -328,    74,  -328,   302,  -328,  -328,  -328,  -328,  -328,  -328,
     302,   302,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,   465,  -328,  -328,  -328,
     513,  -328,   302,   302,   302,   -80,   583,   209,  -328,   654,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,   302,   302,   302,   302,   302,   302,   302,
     302,  -328,  -328,   703,   302,  -328,   302,   302,   302,   302,
     302,   302,   302,   302,  -328,   302,   302,   302,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,   258,   732,   732,
     405,   405,   732,   732,   443,   265,   338,   732,   287,   190,
     619,   234,   327,   181,   181,  -328,  -328,  -328,  -328,  -328,
     619,   234,   327,   181,   181,  -328,  -328,  -328,  -328,   -80,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,   -76,
     104,   104,  -328,  -328,   -80,  -328,  -328,  -328,  -328,   302,
     302,   302,   302,   302,   302,   302,  -328,  -328,    18,  -328,
      23,   302,   -80,   -80,    94,   155,   169,    -1,   -80,   -80,
    -328,  -328,  -328,  -328,  -328,   133,   498,  -328,  -328,   -32,
    -328,  -328,  -328,    56,  -328,  -328,   -80,   -80,  -328,  -328,
     -50,   -17,  -328,  -328,   -25,   -17,  -328,  -328,   132,   171,
     -80,  -328,   -80,  -328,  -328,  -328,  -328,    75,    90,    99,
     583,    -9,  -328,    -9,    90,    99,   104,    81,   -80,   -80,
      13,  -328,    70,  -328,   -17,  -328,    70,    30,  -328,   118,
     -80,   -80,   498,  -328,  -328,    -9,  -328,  -328,   831,  -328,
     -80,  -328,   360,  -328,  -328,  -328,   141,   -80,  -328,     1,
      28,   -17,  -328,  -328,    90,    99,   583,  -328,  -328,  -328,
    -328,  -328,  -328,    45,  -328,  -328,  -328,  -328,  -328,   158,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,   500,
    -328,   -80,   174,  -328,     6,  -328,   198,   -17,   831,  -328,
     485,   566,  -328,   178,  -328,  -328,  -328,   179,  -328,   -80,
    -328,     3,  -328,  -328,   302,   -17,   360,   -67,   302,   302,
     302,   302,   360,  -328,   570,  -328,  -328,   182,   210,    16,
    -328,   -80,   634,  -328,   -17,  -328,   -17,    49,   -33,  -328,
     302,   143,  -328,   139,   -80,  -328,  -328,  -328,   671,  -328,
    -328,  -328,  -328,   -17,  -328,  -328,   389,   389,   389,   389,
     389,  -328,   389,   389,  -328,   389,  -328,   389,   389,   389,
     389,   389,  -328,   389,   360,   389,   389,   389,   360,  -328,
    -328,   137,    80,   -17,  -328,  -328,   708,   194,   164,   302,
     150,   -17,  -328,  -328,  -328,  -328,  -328,   302,  -328,  -328,
     302,  -328,   302,  -328,  -328,  -328,  -328,  -328,   302,  -328,
     151,   302,   160,  -328,  -328,  -328,   302,  -328,   -33,  -328,
      70,  -328,  -328,   -17,   161,  -328,   302,   302,   302,   302,
    -328,   -80,   302,   302,  -328,   302,  -328,   302,   302,   302,
     302,   302,  -328,   302,  -328,   163,  -328,   302,   302,   302,
     -80,  -328,   -80,   -17,   389,   172,  -328,  -328,  -328,  -328,
     -80,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
     302,  -328,  -328,  -328,   -80,   -80,  -328,  -328,   -80,   -80,
     180,    10,  -328,   -67,   -80,   -80,   302,  -328,  -328,   -80,
     143,   -80,    15,   191,   298,    22,   -80,  -328,  -328,   -80,
     302,  -328,  -328,  -328,   -80,   -80,   -67,   300,   -80,   200,
     -67,   300,   -80,   300,   -80,   143,  -328,   300,   302,   143,
    -328,   300,  -328,   300,  -328,   201,  -328,  -328,  -328,   -67,
     -52,  -328
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       2,     0,     1,     0,   214,   234,   235,   210,   268,    17,
       0,     0,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    15,    14,    16,     0,   208,   209,   207,
     267,   254,     0,     0,     0,     0,   253,     0,   270,     0,
     214,   221,   219,   221,   221,   219,   219,   181,   182,   179,
     180,   174,   176,   177,   178,   214,   214,   214,   221,   175,
     190,   214,   173,     0,     0,     0,     0,     0,     0,     0,
       0,   257,   256,     0,     0,   126,     0,     0,     0,     0,
       0,     0,     0,     0,   167,     0,     0,     0,   223,   224,
     225,   226,   227,   228,   229,   215,   269,     0,     0,     0,
      42,    42,     0,     0,     0,     0,     0,     0,     0,     0,
     278,   277,   276,   274,   275,   271,   272,   273,   255,   252,
     265,   264,   263,   261,   262,   258,   259,   260,   169,     0,
     216,   218,    19,   230,   231,   222,    34,   220,    35,     0,
       0,     0,   124,   125,     0,   128,   145,   155,   198,     0,
       0,     0,     0,     0,     0,     0,   156,   184,     0,   217,
       0,     0,     0,     0,     0,     0,     0,     0,   250,   250,
     193,   194,   195,   196,   197,     0,   158,   172,   168,     0,
      18,    23,    20,     0,    24,    43,     0,     0,   186,   127,
       0,     0,   129,   144,     0,     0,   146,   189,     0,     0,
     250,   251,   250,   183,   242,   240,   157,   159,   160,   161,
     162,     0,   238,   170,   237,   236,     0,    21,     0,     0,
       0,   131,     0,   232,   133,   150,   147,     0,   201,     0,
     250,   250,   166,   241,   243,   171,   239,   266,     0,    36,
      38,   185,     0,   188,   233,   133,     0,   148,   145,     0,
       0,     0,   191,   192,   163,   164,   165,    28,    29,    30,
      31,    32,    33,    22,    25,    44,    44,    40,   187,   130,
     128,   142,   136,   137,   138,   139,   143,   140,   141,     0,
     135,   250,     0,   145,     0,   199,     0,   205,     0,    27,
       0,     0,    44,     0,   134,   149,   151,     0,   145,   250,
     203,     0,    26,    58,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    58,     0,   132,   152,     0,     0,     0,
     200,     0,     0,    48,    45,    46,    49,   209,     0,   248,
       0,    47,   244,     0,     0,    55,    57,    54,     0,    58,
     153,   145,   202,     0,   206,    37,   112,   112,   112,   112,
     112,    70,   112,   112,    78,   112,    90,   112,   112,   112,
     112,   112,   102,   112,     0,   112,   112,   112,     0,    59,
     245,     0,     0,     0,    56,    39,     0,     0,     0,     0,
       0,   115,   114,    60,    62,    64,    68,     0,    74,    76,
       0,    80,     0,    92,    94,    96,    98,   100,     0,   104,
     212,     0,     0,    66,    82,    88,     0,   249,     0,   246,
      50,    41,   154,     0,     0,   113,     0,     0,     0,     0,
      71,     0,     0,     0,    79,     0,    91,     0,     0,     0,
       0,     0,   103,     0,   213,     0,   211,     0,     0,     0,
       0,   247,    51,   204,     0,     0,    61,    63,    65,    69,
       0,    75,    77,    81,    93,    95,    97,    99,   101,   105,
       0,    67,    83,    89,     0,    52,   111,   118,     0,     0,
       0,   116,    53,     0,     0,     0,     0,   156,    84,     0,
     119,     0,   116,     0,     0,   116,     0,   122,   108,   250,
       0,   117,   120,    85,   250,   250,     0,   116,   251,     0,
       0,   116,   251,   116,   251,   123,   109,   116,     0,   121,
      86,   116,    72,   116,   110,     0,    87,    73,   106,     0,
     250,   107
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -225,  -328,  -328,
    -328,  -328,  -328,  -328,   214,  -252,  -290,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,   212,
    -328,   434,  -128,   267,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,    48,  -328,    77,    44,  -328,  -198,
    -328,  -328,  -153,  -328,  -328,  -328,  -328,  -328,  -328,  -328,
    -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,  -328,   -24,
    -228,   -43,   345,   131,   208,   729,   268,  -179,     5,  -187,
     119,  -146,  -115,  -101,  -327,  -161,   -30,    -3,    26,  -328,
      20,  -328
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    12,   160,   182,   183,   263,   264,    13,    14,
      15,   265,   266,   292,   140,   290,   322,   369,   416,   417,
     418,   437,   419,   387,   422,   423,   390,   425,   438,   439,
     392,   427,   428,   429,   430,   431,   398,   433,   519,   420,
     445,   379,   380,   478,   468,   473,   494,   500,   489,   496,
      16,    17,    18,    19,   165,   192,   246,   280,    20,   166,
     196,    21,   175,   176,   206,   207,    22,   128,   158,    61,
      23,    24,   220,    25,   108,   167,   250,   319,   301,    26,
      27,   401,    37,    99,    98,    95,   136,   381,   223,   212,
     213,   214,   215,   331,   332,   200,   201,   421,    36,   217,
     382,    30
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      35,   370,    62,   197,   283,    75,    28,   320,   202,   298,
     221,   222,   224,   477,   291,   226,   227,   241,   477,   177,
     342,    29,   178,   338,   328,   477,   236,   180,   329,    74,
     208,    28,   285,   248,   161,   225,    38,    39,   289,   230,
     314,   231,     5,     6,   330,   409,    29,   372,   236,   376,
     282,   257,   258,   259,   260,   261,   262,    74,    71,    72,
      73,   209,   329,   302,   204,   205,   242,     5,     6,   252,
     253,   119,   287,   243,     2,     5,     6,   211,   330,   198,
     199,   441,   129,   130,   131,   297,   254,   204,   205,   110,
     111,   112,   113,   114,   115,   116,   117,   188,   343,   159,
     317,   144,   120,   121,   122,   123,   124,   125,   126,   127,
      74,   286,    74,    31,   164,    74,   181,   255,     8,    74,
     295,    32,     5,     6,   487,    33,   324,   326,     3,    34,
      10,   492,   186,   187,    11,   228,   400,   203,   318,    74,
     400,  -232,  -232,   377,   270,     4,   168,   169,   170,   171,
     172,   173,   174,   271,   288,   179,   218,   219,   185,   189,
     162,   163,     5,     6,   378,   216,     5,     6,     7,     8,
     271,   408,     9,   193,   229,   329,   102,   103,   296,    74,
     184,    10,   315,   316,   232,    11,   340,   233,   239,   240,
     238,   330,   245,   156,   410,   234,   247,   249,   412,     8,
     251,   300,   210,   272,   273,   274,   275,   276,   277,   278,
     267,    10,    84,   341,   190,    11,   191,   281,   268,   284,
     272,   273,   274,   275,   276,   277,   278,   244,   194,   244,
     195,   244,   244,   194,   443,   195,   237,   190,   194,   191,
     195,   194,   372,   195,    85,    86,    87,    28,   373,   407,
     279,   100,   101,   194,   299,   195,     5,     6,   256,   415,
     434,   132,    29,    85,    86,    87,   107,   279,   145,   436,
     444,   321,   460,   413,    88,    89,    90,    91,    92,    93,
      94,   467,   325,   133,   134,    81,    82,    83,   337,   476,
     148,   344,   244,    88,    89,    90,    91,    92,    93,    94,
     490,   323,   491,   477,   374,   333,   334,   335,   336,   508,
     518,   327,    85,    86,    87,   141,   466,    28,   293,    85,
      86,    87,   269,   294,   484,   406,    29,   371,   497,   244,
     235,   244,    29,   501,   503,    78,    79,    80,    81,    82,
      83,   146,    88,    89,    90,    91,    92,    93,    94,    88,
      89,    90,    91,    92,    93,    94,     0,     0,     0,   521,
     149,   150,   151,   152,   153,   154,   155,   138,     0,    28,
     142,   143,   480,    28,     0,   147,   414,   157,     0,     0,
     442,     0,     0,   244,   402,    97,   244,     0,   402,     0,
       0,   450,    85,    86,    87,   505,     0,    31,   435,   509,
     104,   105,   106,   440,     0,    32,   109,     0,     0,    33,
     464,     0,   465,    34,     0,   244,     0,     0,   520,     0,
     469,   139,    88,    89,    90,    91,    92,    93,    94,    79,
      80,    81,    82,    83,   471,   472,     0,     0,   474,   475,
       0,   479,     0,     0,   481,   482,     0,     0,   244,   485,
       0,   486,     5,     6,     7,     8,   495,   470,     0,   498,
       0,     0,     0,     0,   502,   504,     0,    10,   507,     0,
      40,    11,   511,   483,   513,     0,     0,    41,    42,    43,
      44,     5,     6,     0,     8,     0,     0,   499,   303,    88,
      89,    90,    91,    92,    93,    94,    10,    85,    86,    87,
      11,   304,   305,   306,   307,   515,    45,    46,    47,    48,
      49,    50,   271,    51,    52,    53,    54,    55,     0,     0,
       0,     0,    56,    57,     0,     0,   308,    88,    89,    90,
      91,    92,    93,    94,    58,    59,     0,    60,    31,   309,
     310,   311,   312,     0,     0,     0,    32,     0,     0,     0,
      33,     0,     0,     0,    34,     0,     0,     5,     6,     7,
       8,     0,   272,   273,   274,   275,   276,   277,   278,   313,
       0,     0,    10,   339,     0,     0,    11,     0,     0,     0,
       0,     0,   304,   305,   306,   307,   304,   305,   306,   307,
       0,     0,     0,    31,   204,   205,     0,     0,     0,     0,
       0,    32,   424,     0,   426,    33,     0,   308,     0,    34,
     432,   308,    63,    64,    65,    66,    67,    68,    69,    70,
     309,   310,   311,   312,   309,   310,   311,   312,   446,   447,
     448,   449,     0,     0,   451,   452,     0,   453,   345,   454,
     455,   456,   457,   458,   346,   459,     0,     0,     0,   461,
     462,   463,     0,     0,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   375,   368,     0,     0,     0,
       0,   346,    76,    77,    78,    79,    80,    81,    82,    83,
       0,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     366,   367,   411,   368,     0,     0,     0,     0,   346,    77,
      78,    79,    80,    81,    82,    83,     0,     0,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   488,
     368,     0,   493,    76,    77,    78,    79,    80,    81,    82,
      83,     0,     0,     0,   506,     0,    96,     0,   510,     0,
     512,     0,     0,     0,   514,     0,     0,     0,   516,     0,
     517,   383,   384,   385,   386,     0,   388,   389,     0,   391,
       0,   393,   394,   395,   396,   397,     0,   399,     0,   403,
     404,   405,    76,    77,    78,    79,    80,    81,    82,    83,
       0,     0,     0,     0,     0,   118,    88,    89,    90,    91,
      92,    93,    94,     0,     0,   133,   134,   135,   137,   135,
     135,   137,   137,     0,     0,     0,   135,   257,   258,   259,
     260,   261,   262
};

static const yytype_int16 yycheck[] =
{
       3,   328,    26,     4,     3,    35,     1,     4,   169,     3,
      60,   190,   191,     3,   266,   194,   195,     4,     3,     1,
       4,     1,     4,   313,    91,     3,   213,     4,    95,   109,
     176,    26,     4,     3,   110,    60,    10,    11,   263,   200,
     292,   202,    92,    93,   111,   372,    26,    99,   235,   339,
     248,     6,     7,     8,     9,    10,    11,   109,    32,    33,
      34,   176,    95,   288,    96,    97,    53,    92,    93,   230,
     231,    74,   251,    60,     0,    92,    93,   109,   111,    80,
      81,   408,    85,    86,    87,   283,   232,    96,    97,    63,
      64,    65,    66,    67,    68,    69,    70,     3,    82,   129,
     298,   104,    76,    77,    78,    79,    80,    81,    82,    83,
     109,    83,   109,    95,   144,   109,    93,   232,    95,   109,
     281,   103,    92,    93,   109,   107,   305,   306,    54,   111,
     107,   109,   162,   163,   111,     3,   364,     4,   299,   109,
     368,    92,    93,   341,     3,    71,   149,   150,   151,   152,
     153,   154,   155,    12,   109,   158,   186,   187,   161,     4,
     140,   141,    92,    93,   343,   109,    92,    93,    94,    95,
      12,    91,    98,     4,     3,    95,    45,    46,     4,   109,
     160,   107,     4,     4,   109,   111,     4,    97,   218,   219,
     109,   111,   222,     3,   373,    96,   226,   227,     4,    95,
      82,     3,   176,    62,    63,    64,    65,    66,    67,    68,
     240,   107,     3,     3,    59,   111,    61,   247,   242,   249,
      62,    63,    64,    65,    66,    67,    68,   222,    59,   224,
      61,   226,   227,    59,   413,    61,   216,    59,    59,    61,
      61,    59,    99,    61,    54,    55,    56,   242,   109,   112,
     109,    43,    44,    59,   284,    61,    92,    93,   232,   109,
     109,     3,   242,    54,    55,    56,    58,   109,     3,   109,
     109,   301,   109,   109,    84,    85,    86,    87,    88,    89,
      90,   109,   306,    93,    94,   104,   105,   106,   312,   109,
       3,   321,   287,    84,    85,    86,    87,    88,    89,    90,
     109,   304,     4,     3,   334,   308,   309,   310,   311,   109,
     109,   306,    54,    55,    56,   101,   444,   312,   270,    54,
      55,    56,   245,   279,   477,   368,   306,   330,   489,   324,
     211,   326,   312,   494,   495,   101,   102,   103,   104,   105,
     106,     3,    84,    85,    86,    87,    88,    89,    90,    84,
      85,    86,    87,    88,    89,    90,    -1,    -1,    -1,   520,
      73,    74,    75,    76,    77,    78,    79,    99,    -1,   364,
     102,   103,   473,   368,    -1,   107,   379,   109,    -1,    -1,
     410,    -1,    -1,   378,   364,    40,   381,    -1,   368,    -1,
      -1,   421,    54,    55,    56,   496,    -1,    95,   401,   500,
      55,    56,    57,   406,    -1,   103,    61,    -1,    -1,   107,
     440,    -1,   442,   111,    -1,   410,    -1,    -1,   519,    -1,
     450,    16,    84,    85,    86,    87,    88,    89,    90,   102,
     103,   104,   105,   106,   464,   465,    -1,    -1,   468,   469,
      -1,   471,    -1,    -1,   474,   475,    -1,    -1,   443,   479,
      -1,   481,    92,    93,    94,    95,   486,   460,    -1,   489,
      -1,    -1,    -1,    -1,   494,   495,    -1,   107,   498,    -1,
       5,   111,   502,   476,   504,    -1,    -1,    12,    13,    14,
      15,    92,    93,    -1,    95,    -1,    -1,   490,     3,    84,
      85,    86,    87,    88,    89,    90,   107,    54,    55,    56,
     111,    16,    17,    18,    19,   508,    41,    42,    43,    44,
      45,    46,    12,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    58,    -1,    -1,    41,    84,    85,    86,
      87,    88,    89,    90,    69,    70,    -1,    72,    95,    54,
      55,    56,    57,    -1,    -1,    -1,   103,    -1,    -1,    -1,
     107,    -1,    -1,    -1,   111,    -1,    -1,    92,    93,    94,
      95,    -1,    62,    63,    64,    65,    66,    67,    68,     3,
      -1,    -1,   107,     3,    -1,    -1,   111,    -1,    -1,    -1,
      -1,    -1,    16,    17,    18,    19,    16,    17,    18,    19,
      -1,    -1,    -1,    95,    96,    97,    -1,    -1,    -1,    -1,
      -1,   103,   390,    -1,   392,   107,    -1,    41,    -1,   111,
     398,    41,    99,   100,   101,   102,   103,   104,   105,   106,
      54,    55,    56,    57,    54,    55,    56,    57,   416,   417,
     418,   419,    -1,    -1,   422,   423,    -1,   425,     4,   427,
     428,   429,   430,   431,    10,   433,    -1,    -1,    -1,   437,
     438,   439,    -1,    -1,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     4,    42,    -1,    -1,    -1,
      -1,    10,    99,   100,   101,   102,   103,   104,   105,   106,
      -1,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,     4,    42,    -1,    -1,    -1,    -1,    10,   100,
     101,   102,   103,   104,   105,   106,    -1,    -1,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,   482,
      42,    -1,   485,    99,   100,   101,   102,   103,   104,   105,
     106,    -1,    -1,    -1,   497,    -1,   112,    -1,   501,    -1,
     503,    -1,    -1,    -1,   507,    -1,    -1,    -1,   511,    -1,
     513,   347,   348,   349,   350,    -1,   352,   353,    -1,   355,
      -1,   357,   358,   359,   360,   361,    -1,   363,    -1,   365,
     366,   367,    99,   100,   101,   102,   103,   104,   105,   106,
      -1,    -1,    -1,    -1,    -1,   112,    84,    85,    86,    87,
      88,    89,    90,    -1,    -1,    93,    94,    98,    99,   100,
     101,   102,   103,    -1,    -1,    -1,   107,     6,     7,     8,
       9,    10,    11
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   114,     0,    54,    71,    92,    93,    94,    95,    98,
     107,   111,   115,   121,   122,   123,   163,   164,   165,   166,
     171,   174,   179,   183,   184,   186,   192,   193,   201,   213,
     214,    95,   103,   107,   111,   210,   211,   195,   211,   211,
       5,    12,    13,    14,    15,    41,    42,    43,    44,    45,
      46,    48,    49,    50,    51,    52,    57,    58,    69,    70,
      72,   182,   192,    99,   100,   101,   102,   103,   104,   105,
     106,   211,   211,   211,   109,   209,    99,   100,   101,   102,
     103,   104,   105,   106,     3,    54,    55,    56,    84,    85,
      86,    87,    88,    89,    90,   198,   112,   195,   197,   196,
     197,   197,   196,   196,   195,   195,   195,   197,   187,   195,
     211,   211,   211,   211,   211,   211,   211,   211,   112,   210,
     211,   211,   211,   211,   211,   211,   211,   211,   180,   210,
     210,   210,     3,    93,    94,   198,   199,   198,   199,    16,
     127,   127,   199,   199,   210,     3,     3,   199,     3,    73,
      74,    75,    76,    77,    78,    79,     3,   199,   181,   209,
     116,   110,   213,   213,   209,   167,   172,   188,   210,   210,
     210,   210,   210,   210,   210,   175,   176,     1,     4,   210,
       4,    93,   117,   118,   213,   210,   209,   209,     3,     4,
      59,    61,   168,     4,    59,    61,   173,     4,    80,    81,
     208,   209,   208,     4,    96,    97,   177,   178,   204,   205,
     211,   109,   202,   203,   204,   205,   109,   212,   209,   209,
     185,    60,   200,   201,   200,    60,   200,   200,     3,     3,
     208,   208,   109,    97,    96,   203,   202,   213,   109,   209,
     209,     4,    53,    60,   201,   209,   169,   209,     3,   209,
     189,    82,   208,   208,   204,   205,   211,     6,     7,     8,
       9,    10,    11,   119,   120,   124,   125,   209,   192,   169,
       3,    12,    62,    63,    64,    65,    66,    67,    68,   109,
     170,   209,   172,     3,   209,     4,    83,   200,   109,   120,
     128,   128,   126,   167,   170,   208,     4,   172,     3,   209,
       3,   191,   120,     3,    16,    17,    18,    19,    41,    54,
      55,    56,    57,     3,   128,     4,     4,   172,   208,   190,
       4,   209,   129,   210,   200,   192,   200,   201,    91,    95,
     111,   206,   207,   210,   210,   210,   210,   192,   129,     3,
       4,     3,     4,    82,   209,     4,    10,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    42,   130,
     207,   210,    99,   109,   209,     4,   129,   172,   200,   154,
     155,   200,   213,   154,   154,   154,   154,   136,   154,   154,
     139,   154,   143,   154,   154,   154,   154,   154,   149,   154,
     193,   194,   213,   154,   154,   154,   194,   112,    91,   207,
     200,     4,     4,   109,   210,   109,   131,   132,   133,   135,
     152,   210,   137,   138,   152,   140,   152,   144,   145,   146,
     147,   148,   152,   150,   109,   210,   109,   134,   141,   142,
     210,   207,   209,   200,   109,   153,   152,   152,   152,   152,
     209,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     109,   152,   152,   152,   209,   209,   155,   109,   157,   209,
     210,   209,   209,   158,   209,   209,   109,     3,   156,   209,
     206,   209,   209,   210,   175,   209,   209,   109,   156,   161,
     109,     4,   109,   156,   159,   209,   162,   208,   209,   210,
     160,   208,   209,   208,   209,   206,   156,   209,   109,   206,
     156,   209,   156,   209,   156,   210,   156,   156,   109,   151,
     206,   208
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   113,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   115,   116,
     116,   117,   117,   118,   118,   119,   119,   119,   120,   120,
     120,   120,   120,   120,   121,   122,   124,   123,   125,   123,
     126,   123,   127,   127,   128,   128,   128,   128,   128,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   129,   129,
     131,   130,   132,   130,   133,   130,   134,   130,   135,   130,
     136,   130,   130,   130,   137,   130,   138,   130,   139,   130,
     140,   130,   141,   130,   130,   130,   130,   130,   142,   130,
     143,   130,   144,   130,   145,   130,   146,   130,   147,   130,
     148,   130,   149,   130,   150,   130,   151,   130,   152,   152,
     152,   153,   154,   154,   155,   155,   156,   156,   158,   157,
     160,   159,   162,   161,   163,   164,   165,   166,   167,   167,
     168,   168,   168,   169,   169,   169,   170,   170,   170,   170,
     170,   170,   170,   170,   171,   172,   172,   173,   173,   173,
     173,   173,   173,   173,   173,   174,   176,   175,   177,   177,
     178,   178,   178,   178,   178,   178,   178,   180,   179,   181,
     181,   181,   181,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   183,   183,   184,   185,   185,   185,   186,
     187,   187,   187,   187,   187,   187,   187,   187,   188,   188,
     188,   189,   189,   190,   190,   191,   191,   192,   192,   193,
     193,   194,   194,   194,   195,   195,   195,   195,   195,   196,
     196,   197,   197,   198,   198,   198,   198,   198,   198,   198,
     199,   199,   200,   200,   201,   201,   202,   202,   203,   203,
     204,   204,   205,   205,   206,   206,   206,   206,   207,   207,
     208,   208,   209,   210,   211,   211,   211,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   212,   213,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     6,     0,
       2,     2,     4,     1,     1,     1,     3,     2,     1,     1,
       1,     1,     1,     1,     4,     4,     0,    13,     0,    13,
       0,    14,     0,     3,     0,     3,     3,     3,     3,     3,
       5,     6,     7,     8,     3,     3,     4,     3,     0,     2,
       0,     4,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     3,    11,    12,     0,     4,     0,     4,     0,     3,
       0,     4,     0,     4,     6,     8,    10,    11,     0,     4,
       0,     3,     0,     4,     0,     4,     0,     4,     0,     4,
       0,     4,     0,     3,     0,     4,     0,    15,     6,     8,
       9,     2,     0,     2,     1,     1,     0,     3,     0,     3,
       0,     3,     0,     3,     4,     4,     3,     6,     0,     2,
       4,     2,     6,     0,     3,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     6,     0,     2,     2,     3,     5,
       2,     5,     6,     7,     9,     4,     0,     2,     0,     1,
       1,     1,     1,     3,     3,     3,     2,     0,     6,     0,
       3,     4,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     6,     4,     8,     0,     3,     2,     6,
       0,     6,     6,     3,     3,     3,     3,     3,     0,     5,
       7,     0,     5,     0,     5,     0,     3,     1,     1,     1,
       1,     2,     1,     2,     0,     2,     3,     4,     3,     0,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     1,     1,     1,     1,     2,
       1,     2,     1,     2,     1,     2,     3,     4,     1,     3,
       0,     1,     2,     1,     1,     3,     2,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     1,     1,     3,
       2,     3,     3,     3,     3,     3,     3,     3,     3
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
  case 18: /* accelerator: id ACCELERATORS suboptions BEG acc_entries END  */
#line 201 "rcparse.y"
          {
	    define_accelerator ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].pacc));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2090 "rcparse.c"
    break;

  case 19: /* acc_entries: %empty  */
#line 211 "rcparse.y"
          {
	    (yyval.pacc) = NULL;
	  }
#line 2098 "rcparse.c"
    break;

  case 20: /* acc_entries: acc_entries acc_entry  */
#line 215 "rcparse.y"
          {
	    rc_accelerator *a;

	    a = (rc_accelerator *) res_alloc (sizeof *a);
	    *a = (yyvsp[0].acc);
	    if ((yyvsp[-1].pacc) == NULL)
	      (yyval.pacc) = a;
	    else
	      {
		rc_accelerator **pp;

		for (pp = &(yyvsp[-1].pacc)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = a;
		(yyval.pacc) = (yyvsp[-1].pacc);
	      }
	  }
#line 2120 "rcparse.c"
    break;

  case 21: /* acc_entry: acc_event cposnumexpr  */
#line 236 "rcparse.y"
          {
	    (yyval.acc) = (yyvsp[-1].acc);
	    (yyval.acc).id = (yyvsp[0].il);
	  }
#line 2129 "rcparse.c"
    break;

  case 22: /* acc_entry: acc_event cposnumexpr ',' acc_options  */
#line 241 "rcparse.y"
          {
	    (yyval.acc) = (yyvsp[-3].acc);
	    (yyval.acc).id = (yyvsp[-2].il);
	    (yyval.acc).flags |= (yyvsp[0].is);
	    if (((yyval.acc).flags & ACC_VIRTKEY) == 0
		&& ((yyval.acc).flags & (ACC_SHIFT | ACC_CONTROL)) != 0)
	      rcparse_warning (_("inappropriate modifiers for non-VIRTKEY"));
	  }
#line 2142 "rcparse.c"
    break;

  case 23: /* acc_event: QUOTEDSTRING  */
#line 253 "rcparse.y"
          {
	    const char *s = (yyvsp[0].s);
	    char ch;

	    (yyval.acc).next = NULL;
	    (yyval.acc).id = 0;
	    ch = *s;
	    if (ch != '^')
	      (yyval.acc).flags = 0;
	    else
	      {
		(yyval.acc).flags = ACC_CONTROL | ACC_VIRTKEY;
		++s;
		ch = TOUPPER (s[0]);
	      }
	    (yyval.acc).key = ch;
	    if (s[1] != '\0')
	      rcparse_warning (_("accelerator should only be one character"));
	  }
#line 2166 "rcparse.c"
    break;

  case 24: /* acc_event: posnumexpr  */
#line 273 "rcparse.y"
          {
	    (yyval.acc).next = NULL;
	    (yyval.acc).flags = 0;
	    (yyval.acc).id = 0;
	    (yyval.acc).key = (yyvsp[0].il);
	  }
#line 2177 "rcparse.c"
    break;

  case 25: /* acc_options: acc_option  */
#line 283 "rcparse.y"
          {
	    (yyval.is) = (yyvsp[0].is);
	  }
#line 2185 "rcparse.c"
    break;

  case 26: /* acc_options: acc_options ',' acc_option  */
#line 287 "rcparse.y"
          {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
#line 2193 "rcparse.c"
    break;

  case 27: /* acc_options: acc_options acc_option  */
#line 292 "rcparse.y"
          {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
#line 2201 "rcparse.c"
    break;

  case 28: /* acc_option: VIRTKEY  */
#line 299 "rcparse.y"
          {
	    (yyval.is) = ACC_VIRTKEY;
	  }
#line 2209 "rcparse.c"
    break;

  case 29: /* acc_option: ASCII  */
#line 303 "rcparse.y"
          {
	    /* This is just the absence of VIRTKEY.  */
	    (yyval.is) = 0;
	  }
#line 2218 "rcparse.c"
    break;

  case 30: /* acc_option: NOINVERT  */
#line 308 "rcparse.y"
          {
	    (yyval.is) = ACC_NOINVERT;
	  }
#line 2226 "rcparse.c"
    break;

  case 31: /* acc_option: SHIFT  */
#line 312 "rcparse.y"
          {
	    (yyval.is) = ACC_SHIFT;
	  }
#line 2234 "rcparse.c"
    break;

  case 32: /* acc_option: CONTROL  */
#line 316 "rcparse.y"
          {
	    (yyval.is) = ACC_CONTROL;
	  }
#line 2242 "rcparse.c"
    break;

  case 33: /* acc_option: ALT  */
#line 320 "rcparse.y"
          {
	    (yyval.is) = ACC_ALT;
	  }
#line 2250 "rcparse.c"
    break;

  case 34: /* bitmap: id BITMAP memflags_move file_name  */
#line 329 "rcparse.y"
          {
	    define_bitmap ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2261 "rcparse.c"
    break;

  case 35: /* cursor: id CURSOR memflags_move_discard file_name  */
#line 341 "rcparse.y"
          {
	    define_cursor ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2272 "rcparse.c"
    break;

  case 36: /* $@1: %empty  */
#line 354 "rcparse.y"
            {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-3].il);
	      dialog.y = (yyvsp[-2].il);
	      dialog.width = (yyvsp[-1].il);
	      dialog.height = (yyvsp[0].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-4].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = NULL;
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-5].res_info);
	      style = 0;
	    }
#line 2293 "rcparse.c"
    break;

  case 37: /* dialog: id DIALOG memflags_move exstyle posnumexpr cnumexpr cnumexpr cnumexpr $@1 styles BEG controls END  */
#line 371 "rcparse.y"
          {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2304 "rcparse.c"
    break;

  case 38: /* $@2: %empty  */
#line 379 "rcparse.y"
            {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-3].il);
	      dialog.y = (yyvsp[-2].il);
	      dialog.width = (yyvsp[-1].il);
	      dialog.height = (yyvsp[0].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-4].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = ((rc_dialog_ex *)
			   res_alloc (sizeof (rc_dialog_ex)));
	      memset (dialog.ex, 0, sizeof (rc_dialog_ex));
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-5].res_info);
	      style = 0;
	    }
#line 2327 "rcparse.c"
    break;

  case 39: /* dialog: id DIALOGEX memflags_move exstyle posnumexpr cnumexpr cnumexpr cnumexpr $@2 styles BEG controls END  */
#line 398 "rcparse.y"
          {
	    define_dialog ((yyvsp[-12].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2338 "rcparse.c"
    break;

  case 40: /* $@3: %empty  */
#line 406 "rcparse.y"
            {
	      memset (&dialog, 0, sizeof dialog);
	      dialog.x = (yyvsp[-4].il);
	      dialog.y = (yyvsp[-3].il);
	      dialog.width = (yyvsp[-2].il);
	      dialog.height = (yyvsp[-1].il);
	      dialog.style = WS_POPUP | WS_BORDER | WS_SYSMENU;
	      dialog.exstyle = (yyvsp[-5].il);
	      dialog.menu.named = 1;
	      dialog.class.named = 1;
	      dialog.font = NULL;
	      dialog.ex = ((rc_dialog_ex *)
			   res_alloc (sizeof (rc_dialog_ex)));
	      memset (dialog.ex, 0, sizeof (rc_dialog_ex));
	      dialog.ex->help = (yyvsp[0].il);
	      dialog.controls = NULL;
	      sub_res_info = (yyvsp[-6].res_info);
	      style = 0;
	    }
#line 2362 "rcparse.c"
    break;

  case 41: /* dialog: id DIALOGEX memflags_move exstyle posnumexpr cnumexpr cnumexpr cnumexpr cnumexpr $@3 styles BEG controls END  */
#line 426 "rcparse.y"
          {
	    define_dialog ((yyvsp[-13].id), &sub_res_info, &dialog);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 2373 "rcparse.c"
    break;

  case 42: /* exstyle: %empty  */
#line 436 "rcparse.y"
          {
	    (yyval.il) = 0;
	  }
#line 2381 "rcparse.c"
    break;

  case 43: /* exstyle: EXSTYLE '=' numexpr  */
#line 440 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 2389 "rcparse.c"
    break;

  case 45: /* styles: styles CAPTION res_unicode_string_concat  */
#line 448 "rcparse.y"
          {
	    dialog.style |= WS_CAPTION;
	    style |= WS_CAPTION;
	    dialog.caption = (yyvsp[0].uni);
	  }
#line 2399 "rcparse.c"
    break;

  case 46: /* styles: styles CLASS id  */
#line 454 "rcparse.y"
          {
	    dialog.class = (yyvsp[0].id);
	  }
#line 2407 "rcparse.c"
    break;

  case 47: /* styles: styles STYLE styleexpr  */
#line 459 "rcparse.y"
          {
	    dialog.style = style;
	  }
#line 2415 "rcparse.c"
    break;

  case 48: /* styles: styles EXSTYLE numexpr  */
#line 463 "rcparse.y"
          {
	    dialog.exstyle = (yyvsp[0].il);
	  }
#line 2423 "rcparse.c"
    break;

  case 49: /* styles: styles CLASS res_unicode_string_concat  */
#line 467 "rcparse.y"
          {
	    res_unistring_to_id (& dialog.class, (yyvsp[0].uni));
	  }
#line 2431 "rcparse.c"
    break;

  case 50: /* styles: styles FONT numexpr ',' res_unicode_string_concat  */
#line 471 "rcparse.y"
          {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-2].il);
	    dialog.font = (yyvsp[0].uni);
	    if (dialog.ex != NULL)
	      {
		dialog.ex->weight = 0;
		dialog.ex->italic = 0;
		dialog.ex->charset = 1;
	      }
	  }
#line 2448 "rcparse.c"
    break;

  case 51: /* styles: styles FONT numexpr ',' res_unicode_string_concat cnumexpr  */
#line 484 "rcparse.y"
          {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-3].il);
	    dialog.font = (yyvsp[-1].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[0].il);
		dialog.ex->italic = 0;
		dialog.ex->charset = 1;
	      }
	  }
#line 2467 "rcparse.c"
    break;

  case 52: /* styles: styles FONT numexpr ',' res_unicode_string_concat cnumexpr cnumexpr  */
#line 499 "rcparse.y"
          {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-4].il);
	    dialog.font = (yyvsp[-2].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[-1].il);
		dialog.ex->italic = (yyvsp[0].il);
		dialog.ex->charset = 1;
	      }
	  }
#line 2486 "rcparse.c"
    break;

  case 53: /* styles: styles FONT numexpr ',' res_unicode_string_concat cnumexpr cnumexpr cnumexpr  */
#line 514 "rcparse.y"
          {
	    dialog.style |= DS_SETFONT;
	    style |= DS_SETFONT;
	    dialog.pointsize = (yyvsp[-5].il);
	    dialog.font = (yyvsp[-3].uni);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("extended FONT requires DIALOGEX"));
	    else
	      {
		dialog.ex->weight = (yyvsp[-2].il);
		dialog.ex->italic = (yyvsp[-1].il);
		dialog.ex->charset = (yyvsp[0].il);
	      }
	  }
#line 2505 "rcparse.c"
    break;

  case 54: /* styles: styles MENU id  */
#line 529 "rcparse.y"
          {
	    dialog.menu = (yyvsp[0].id);
	  }
#line 2513 "rcparse.c"
    break;

  case 55: /* styles: styles CHARACTERISTICS numexpr  */
#line 533 "rcparse.y"
          {
	    sub_res_info.characteristics = (yyvsp[0].il);
	  }
#line 2521 "rcparse.c"
    break;

  case 56: /* styles: styles LANGUAGE numexpr cnumexpr  */
#line 537 "rcparse.y"
          {
	    sub_res_info.language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 2529 "rcparse.c"
    break;

  case 57: /* styles: styles VERSIONK numexpr  */
#line 541 "rcparse.y"
          {
	    sub_res_info.version = (yyvsp[0].il);
	  }
#line 2537 "rcparse.c"
    break;

  case 59: /* controls: controls control  */
#line 549 "rcparse.y"
          {
	    rc_dialog_control **pp;

	    for (pp = &dialog.controls; *pp != NULL; pp = &(*pp)->next)
	      ;
	    *pp = (yyvsp[0].dialog_control);
	  }
#line 2549 "rcparse.c"
    break;

  case 60: /* $@4: %empty  */
#line 560 "rcparse.y"
            {
	      default_style = BS_AUTO3STATE | WS_TABSTOP;
	      base_style = BS_AUTO3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2561 "rcparse.c"
    break;

  case 61: /* control: AUTO3STATE optresidc $@4 control_params  */
#line 568 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2569 "rcparse.c"
    break;

  case 62: /* $@5: %empty  */
#line 572 "rcparse.y"
            {
	      default_style = BS_AUTOCHECKBOX | WS_TABSTOP;
	      base_style = BS_AUTOCHECKBOX | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2581 "rcparse.c"
    break;

  case 63: /* control: AUTOCHECKBOX optresidc $@5 control_params  */
#line 580 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2589 "rcparse.c"
    break;

  case 64: /* $@6: %empty  */
#line 584 "rcparse.y"
            {
	      default_style = BS_AUTORADIOBUTTON | WS_TABSTOP;
	      base_style = BS_AUTORADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2601 "rcparse.c"
    break;

  case 65: /* control: AUTORADIOBUTTON optresidc $@6 control_params  */
#line 592 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2609 "rcparse.c"
    break;

  case 66: /* $@7: %empty  */
#line 596 "rcparse.y"
            {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2621 "rcparse.c"
    break;

  case 67: /* control: BEDIT optresidc $@7 control_params  */
#line 604 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("BEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "BEDIT");
	  }
#line 2632 "rcparse.c"
    break;

  case 68: /* $@8: %empty  */
#line 611 "rcparse.y"
            {
	      default_style = BS_CHECKBOX | WS_TABSTOP;
	      base_style = BS_CHECKBOX | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2644 "rcparse.c"
    break;

  case 69: /* control: CHECKBOX optresidc $@8 control_params  */
#line 619 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2652 "rcparse.c"
    break;

  case 70: /* $@9: %empty  */
#line 623 "rcparse.y"
            {
	      /* This is as per MSDN documentation.  With some (???)
		 versions of MS rc.exe their is no default style.  */
	      default_style = CBS_SIMPLE | WS_TABSTOP;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_COMBOBOX;
	      res_text_field = res_null_text;
	    }
#line 2666 "rcparse.c"
    break;

  case 71: /* control: COMBOBOX $@9 control_params  */
#line 633 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2674 "rcparse.c"
    break;

  case 72: /* control: CONTROL optresidc numexpr cresid control_styleexpr cnumexpr cnumexpr cnumexpr cnumexpr optcnumexpr opt_control_data  */
#line 638 "rcparse.y"
          {
	    (yyval.dialog_control) = define_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-7].id), style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 2688 "rcparse.c"
    break;

  case 73: /* control: CONTROL optresidc numexpr cresid control_styleexpr cnumexpr cnumexpr cnumexpr cnumexpr cnumexpr cnumexpr opt_control_data  */
#line 649 "rcparse.y"
          {
	    (yyval.dialog_control) = define_control ((yyvsp[-10].id), (yyvsp[-9].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-8].id), style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
#line 2700 "rcparse.c"
    break;

  case 74: /* $@10: %empty  */
#line 657 "rcparse.y"
            {
	      default_style = SS_CENTER | WS_GROUP;
	      base_style = SS_CENTER;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2712 "rcparse.c"
    break;

  case 75: /* control: CTEXT optresidc $@10 control_params  */
#line 665 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2720 "rcparse.c"
    break;

  case 76: /* $@11: %empty  */
#line 669 "rcparse.y"
            {
	      default_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      base_style = BS_DEFPUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2732 "rcparse.c"
    break;

  case 77: /* control: DEFPUSHBUTTON optresidc $@11 control_params  */
#line 677 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2740 "rcparse.c"
    break;

  case 78: /* $@12: %empty  */
#line 681 "rcparse.y"
            {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = res_null_text;
	    }
#line 2752 "rcparse.c"
    break;

  case 79: /* control: EDITTEXT $@12 control_params  */
#line 689 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2760 "rcparse.c"
    break;

  case 80: /* $@13: %empty  */
#line 693 "rcparse.y"
            {
	      default_style = BS_GROUPBOX;
	      base_style = BS_GROUPBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2772 "rcparse.c"
    break;

  case 81: /* control: GROUPBOX optresidc $@13 control_params  */
#line 701 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2780 "rcparse.c"
    break;

  case 82: /* $@14: %empty  */
#line 705 "rcparse.y"
            {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2792 "rcparse.c"
    break;

  case 83: /* control: HEDIT optresidc $@14 control_params  */
#line 713 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "HEDIT");
	  }
#line 2803 "rcparse.c"
    break;

  case 84: /* control: ICON resref numexpr cnumexpr cnumexpr opt_control_data  */
#line 720 "rcparse.y"
          {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-4].id), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-1].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2812 "rcparse.c"
    break;

  case 85: /* control: ICON resref numexpr cnumexpr cnumexpr cnumexpr cnumexpr opt_control_data  */
#line 726 "rcparse.y"
          {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-6].id), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2821 "rcparse.c"
    break;

  case 86: /* control: ICON resref numexpr cnumexpr cnumexpr cnumexpr cnumexpr icon_styleexpr optcnumexpr opt_control_data  */
#line 732 "rcparse.y"
          {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-8].id), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), style, (yyvsp[-1].il), 0, (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2830 "rcparse.c"
    break;

  case 87: /* control: ICON resref numexpr cnumexpr cnumexpr cnumexpr cnumexpr icon_styleexpr cnumexpr cnumexpr opt_control_data  */
#line 738 "rcparse.y"
          {
	    (yyval.dialog_control) = define_icon_control ((yyvsp[-9].id), (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), style, (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].rcdata_item),
				      dialog.ex);
          }
#line 2839 "rcparse.c"
    break;

  case 88: /* $@15: %empty  */
#line 743 "rcparse.y"
            {
	      default_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      base_style = ES_LEFT | WS_BORDER | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_EDIT;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2851 "rcparse.c"
    break;

  case 89: /* control: IEDIT optresidc $@15 control_params  */
#line 751 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	    if (dialog.ex == NULL)
	      rcparse_warning (_("IEDIT requires DIALOGEX"));
	    res_string_to_id (&(yyval.dialog_control)->class, "IEDIT");
	  }
#line 2862 "rcparse.c"
    break;

  case 90: /* $@16: %empty  */
#line 758 "rcparse.y"
            {
	      default_style = LBS_NOTIFY | WS_BORDER;
	      base_style = LBS_NOTIFY | WS_BORDER;
	      class.named = 0;
	      class.u.id = CTL_LISTBOX;
	      res_text_field = res_null_text;
	    }
#line 2874 "rcparse.c"
    break;

  case 91: /* control: LISTBOX $@16 control_params  */
#line 766 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2882 "rcparse.c"
    break;

  case 92: /* $@17: %empty  */
#line 770 "rcparse.y"
            {
	      default_style = SS_LEFT | WS_GROUP;
	      base_style = SS_LEFT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2894 "rcparse.c"
    break;

  case 93: /* control: LTEXT optresidc $@17 control_params  */
#line 778 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2902 "rcparse.c"
    break;

  case 94: /* $@18: %empty  */
#line 782 "rcparse.y"
            {
	      default_style = BS_PUSHBOX | WS_TABSTOP;
	      base_style = BS_PUSHBOX;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	    }
#line 2913 "rcparse.c"
    break;

  case 95: /* control: PUSHBOX optresidc $@18 control_params  */
#line 789 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2921 "rcparse.c"
    break;

  case 96: /* $@19: %empty  */
#line 793 "rcparse.y"
            {
	      default_style = BS_PUSHBUTTON | WS_TABSTOP;
	      base_style = BS_PUSHBUTTON | WS_TABSTOP;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2933 "rcparse.c"
    break;

  case 97: /* control: PUSHBUTTON optresidc $@19 control_params  */
#line 801 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2941 "rcparse.c"
    break;

  case 98: /* $@20: %empty  */
#line 805 "rcparse.y"
            {
	      default_style = BS_RADIOBUTTON | WS_TABSTOP;
	      base_style = BS_RADIOBUTTON;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2953 "rcparse.c"
    break;

  case 99: /* control: RADIOBUTTON optresidc $@20 control_params  */
#line 813 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2961 "rcparse.c"
    break;

  case 100: /* $@21: %empty  */
#line 817 "rcparse.y"
            {
	      default_style = SS_RIGHT | WS_GROUP;
	      base_style = SS_RIGHT;
	      class.named = 0;
	      class.u.id = CTL_STATIC;
	      res_text_field = (yyvsp[0].id);
	    }
#line 2973 "rcparse.c"
    break;

  case 101: /* control: RTEXT optresidc $@21 control_params  */
#line 825 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 2981 "rcparse.c"
    break;

  case 102: /* $@22: %empty  */
#line 829 "rcparse.y"
            {
	      default_style = SBS_HORZ;
	      base_style = 0;
	      class.named = 0;
	      class.u.id = CTL_SCROLLBAR;
	      res_text_field = res_null_text;
	    }
#line 2993 "rcparse.c"
    break;

  case 103: /* control: SCROLLBAR $@22 control_params  */
#line 837 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 3001 "rcparse.c"
    break;

  case 104: /* $@23: %empty  */
#line 841 "rcparse.y"
            {
	      default_style = BS_3STATE | WS_TABSTOP;
	      base_style = BS_3STATE;
	      class.named = 0;
	      class.u.id = CTL_BUTTON;
	      res_text_field = (yyvsp[0].id);
	    }
#line 3013 "rcparse.c"
    break;

  case 105: /* control: STATE3 optresidc $@23 control_params  */
#line 849 "rcparse.y"
          {
	    (yyval.dialog_control) = (yyvsp[0].dialog_control);
	  }
#line 3021 "rcparse.c"
    break;

  case 106: /* $@24: %empty  */
#line 854 "rcparse.y"
            { style = WS_CHILD | WS_VISIBLE; }
#line 3027 "rcparse.c"
    break;

  case 107: /* control: USERBUTTON resref numexpr ',' numexpr ',' numexpr ',' numexpr ',' numexpr ',' $@24 styleexpr optcnumexpr  */
#line 856 "rcparse.y"
          {
	    rc_res_id cid;
	    cid.named = 0;
	    cid.u.id = CTL_BUTTON;
	    (yyval.dialog_control) = define_control ((yyvsp[-13].id), (yyvsp[-12].il), (yyvsp[-10].il), (yyvsp[-8].il), (yyvsp[-6].il), (yyvsp[-4].il), cid,
				 style, (yyvsp[0].il));
	  }
#line 3039 "rcparse.c"
    break;

  case 108: /* control_params: numexpr cnumexpr cnumexpr cnumexpr cnumexpr opt_control_data  */
#line 874 "rcparse.y"
          {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-2].il), (yyvsp[-1].il), class,
				 default_style | WS_CHILD | WS_VISIBLE, 0);
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 3054 "rcparse.c"
    break;

  case 109: /* control_params: numexpr cnumexpr cnumexpr cnumexpr cnumexpr control_params_styleexpr optcnumexpr opt_control_data  */
#line 886 "rcparse.y"
          {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), class, style, (yyvsp[-1].il));
	    if ((yyvsp[0].rcdata_item) != NULL)
	      {
		if (dialog.ex == NULL)
		  rcparse_warning (_("control data requires DIALOGEX"));
		(yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	      }
	  }
#line 3068 "rcparse.c"
    break;

  case 110: /* control_params: numexpr cnumexpr cnumexpr cnumexpr cnumexpr control_params_styleexpr cnumexpr cnumexpr opt_control_data  */
#line 897 "rcparse.y"
          {
	    (yyval.dialog_control) = define_control (res_text_field, (yyvsp[-8].il), (yyvsp[-7].il), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), class, style, (yyvsp[-2].il));
	    if (dialog.ex == NULL)
	      rcparse_warning (_("help ID requires DIALOGEX"));
	    (yyval.dialog_control)->help = (yyvsp[-1].il);
	    (yyval.dialog_control)->data = (yyvsp[0].rcdata_item);
	  }
#line 3080 "rcparse.c"
    break;

  case 111: /* cresid: ',' resid  */
#line 908 "rcparse.y"
          {
	    if ((yyvsp[0].id).named)
	      res_unistring_to_id (&(yyval.id), (yyvsp[0].id).u.n.name);
	    else
	      (yyval.id)=(yyvsp[0].id);
	  }
#line 3091 "rcparse.c"
    break;

  case 112: /* optresidc: %empty  */
#line 918 "rcparse.y"
          {
	    res_string_to_id (&(yyval.id), "");
	  }
#line 3099 "rcparse.c"
    break;

  case 113: /* optresidc: resid ','  */
#line 921 "rcparse.y"
                    { (yyval.id)=(yyvsp[-1].id); }
#line 3105 "rcparse.c"
    break;

  case 114: /* resid: posnumexpr  */
#line 926 "rcparse.y"
          {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
#line 3114 "rcparse.c"
    break;

  case 115: /* resid: res_unicode_string_concat  */
#line 931 "rcparse.y"
          {
	    (yyval.id).named = 1;
	    (yyval.id).u.n.name = (yyvsp[0].uni);
	    (yyval.id).u.n.length = unichar_len ((yyvsp[0].uni));
	  }
#line 3124 "rcparse.c"
    break;

  case 116: /* opt_control_data: %empty  */
#line 940 "rcparse.y"
          {
	    (yyval.rcdata_item) = NULL;
	  }
#line 3132 "rcparse.c"
    break;

  case 117: /* opt_control_data: BEG optrcdata_data END  */
#line 944 "rcparse.y"
          {
	    (yyval.rcdata_item) = (yyvsp[-1].rcdata).first;
	  }
#line 3140 "rcparse.c"
    break;

  case 118: /* $@25: %empty  */
#line 953 "rcparse.y"
          { style = WS_CHILD | WS_VISIBLE; }
#line 3146 "rcparse.c"
    break;

  case 120: /* $@26: %empty  */
#line 959 "rcparse.y"
          { style = SS_ICON | WS_CHILD | WS_VISIBLE; }
#line 3152 "rcparse.c"
    break;

  case 122: /* $@27: %empty  */
#line 965 "rcparse.y"
          { style = base_style | WS_CHILD | WS_VISIBLE; }
#line 3158 "rcparse.c"
    break;

  case 124: /* font: id FONT memflags_move_discard file_name  */
#line 973 "rcparse.y"
          {
	    define_font ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3169 "rcparse.c"
    break;

  case 125: /* icon: id ICON memflags_move_discard file_name  */
#line 985 "rcparse.y"
          {
	    define_icon ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3180 "rcparse.c"
    break;

  case 126: /* language: LANGUAGE numexpr cnumexpr  */
#line 998 "rcparse.y"
          {
	    language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 3188 "rcparse.c"
    break;

  case 127: /* menu: id MENU suboptions BEG menuitems END  */
#line 1007 "rcparse.y"
          {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3199 "rcparse.c"
    break;

  case 128: /* menuitems: %empty  */
#line 1017 "rcparse.y"
          {
	    (yyval.menuitem) = NULL;
	  }
#line 3207 "rcparse.c"
    break;

  case 129: /* menuitems: menuitems menuitem  */
#line 1021 "rcparse.y"
          {
	    if ((yyvsp[-1].menuitem) == NULL)
	      (yyval.menuitem) = (yyvsp[0].menuitem);
	    else
	      {
		rc_menuitem **pp;

		for (pp = &(yyvsp[-1].menuitem)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = (yyvsp[0].menuitem);
		(yyval.menuitem) = (yyvsp[-1].menuitem);
	      }
	  }
#line 3225 "rcparse.c"
    break;

  case 130: /* menuitem: MENUITEM res_unicode_string_concat cnumexpr menuitem_flags  */
#line 1038 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-2].uni), (yyvsp[-1].il), (yyvsp[0].is), 0, 0, NULL);
	  }
#line 3233 "rcparse.c"
    break;

  case 131: /* menuitem: MENUITEM SEPARATOR  */
#line 1042 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
	  }
#line 3241 "rcparse.c"
    break;

  case 132: /* menuitem: POPUP res_unicode_string_concat menuitem_flags BEG menuitems END  */
#line 1046 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), 0, (yyvsp[-3].is), 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3249 "rcparse.c"
    break;

  case 133: /* menuitem_flags: %empty  */
#line 1053 "rcparse.y"
          {
	    (yyval.is) = 0;
	  }
#line 3257 "rcparse.c"
    break;

  case 134: /* menuitem_flags: menuitem_flags ',' menuitem_flag  */
#line 1057 "rcparse.y"
          {
	    (yyval.is) = (yyvsp[-2].is) | (yyvsp[0].is);
	  }
#line 3265 "rcparse.c"
    break;

  case 135: /* menuitem_flags: menuitem_flags menuitem_flag  */
#line 1061 "rcparse.y"
          {
	    (yyval.is) = (yyvsp[-1].is) | (yyvsp[0].is);
	  }
#line 3273 "rcparse.c"
    break;

  case 136: /* menuitem_flag: CHECKED  */
#line 1068 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_CHECKED;
	  }
#line 3281 "rcparse.c"
    break;

  case 137: /* menuitem_flag: GRAYED  */
#line 1072 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_GRAYED;
	  }
#line 3289 "rcparse.c"
    break;

  case 138: /* menuitem_flag: HELP  */
#line 1076 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_HELP;
	  }
#line 3297 "rcparse.c"
    break;

  case 139: /* menuitem_flag: INACTIVE  */
#line 1080 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_INACTIVE;
	  }
#line 3305 "rcparse.c"
    break;

  case 140: /* menuitem_flag: MENUBARBREAK  */
#line 1084 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_MENUBARBREAK;
	  }
#line 3313 "rcparse.c"
    break;

  case 141: /* menuitem_flag: MENUBREAK  */
#line 1088 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_MENUBREAK;
	  }
#line 3321 "rcparse.c"
    break;

  case 142: /* menuitem_flag: BITMAP  */
#line 1092 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_BITMAP;
	  }
#line 3329 "rcparse.c"
    break;

  case 143: /* menuitem_flag: OWNERDRAW  */
#line 1096 "rcparse.y"
          {
	    (yyval.is) = MENUITEM_OWNERDRAW;
	  }
#line 3337 "rcparse.c"
    break;

  case 144: /* menuex: id MENUEX suboptions BEG menuexitems END  */
#line 1105 "rcparse.y"
          {
	    define_menu ((yyvsp[-5].id), &(yyvsp[-3].res_info), (yyvsp[-1].menuitem));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3348 "rcparse.c"
    break;

  case 145: /* menuexitems: %empty  */
#line 1115 "rcparse.y"
          {
	    (yyval.menuitem) = NULL;
	  }
#line 3356 "rcparse.c"
    break;

  case 146: /* menuexitems: menuexitems menuexitem  */
#line 1119 "rcparse.y"
          {
	    if ((yyvsp[-1].menuitem) == NULL)
	      (yyval.menuitem) = (yyvsp[0].menuitem);
	    else
	      {
		rc_menuitem **pp;

		for (pp = &(yyvsp[-1].menuitem)->next; *pp != NULL; pp = &(*pp)->next)
		  ;
		*pp = (yyvsp[0].menuitem);
		(yyval.menuitem) = (yyvsp[-1].menuitem);
	      }
	  }
#line 3374 "rcparse.c"
    break;

  case 147: /* menuexitem: MENUITEM res_unicode_string_concat  */
#line 1136 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[0].uni), 0, 0, 0, 0, NULL);
	  }
#line 3382 "rcparse.c"
    break;

  case 148: /* menuexitem: MENUITEM res_unicode_string_concat cnumexpr  */
#line 1140 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-1].uni), (yyvsp[0].il), 0, 0, 0, NULL);
	  }
#line 3390 "rcparse.c"
    break;

  case 149: /* menuexitem: MENUITEM res_unicode_string_concat cnumexpr cnumexpr optcnumexpr  */
#line 1144 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), (yyvsp[-2].il), (yyvsp[-1].il), (yyvsp[0].il), 0, NULL);
	  }
#line 3398 "rcparse.c"
    break;

  case 150: /* menuexitem: MENUITEM SEPARATOR  */
#line 1148 "rcparse.y"
          {
 	    (yyval.menuitem) = define_menuitem (NULL, 0, 0, 0, 0, NULL);
 	  }
#line 3406 "rcparse.c"
    break;

  case 151: /* menuexitem: POPUP res_unicode_string_concat BEG menuexitems END  */
#line 1152 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-3].uni), 0, 0, 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3414 "rcparse.c"
    break;

  case 152: /* menuexitem: POPUP res_unicode_string_concat cnumexpr BEG menuexitems END  */
#line 1156 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-4].uni), (yyvsp[-3].il), 0, 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3422 "rcparse.c"
    break;

  case 153: /* menuexitem: POPUP res_unicode_string_concat cnumexpr cnumexpr BEG menuexitems END  */
#line 1160 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-5].uni), (yyvsp[-4].il), (yyvsp[-3].il), 0, 0, (yyvsp[-1].menuitem));
	  }
#line 3430 "rcparse.c"
    break;

  case 154: /* menuexitem: POPUP res_unicode_string_concat cnumexpr cnumexpr cnumexpr optcnumexpr BEG menuexitems END  */
#line 1165 "rcparse.y"
          {
	    (yyval.menuitem) = define_menuitem ((yyvsp[-7].uni), (yyvsp[-6].il), (yyvsp[-5].il), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].menuitem));
	  }
#line 3438 "rcparse.c"
    break;

  case 155: /* messagetable: id MESSAGETABLE memflags_move file_name  */
#line 1174 "rcparse.y"
          {
	    define_messagetable ((yyvsp[-3].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3449 "rcparse.c"
    break;

  case 156: /* $@28: %empty  */
#line 1186 "rcparse.y"
          {
	    rcparse_rcdata ();
	  }
#line 3457 "rcparse.c"
    break;

  case 157: /* optrcdata_data: $@28 optrcdata_data_int  */
#line 1190 "rcparse.y"
          {
	    rcparse_normal ();
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
#line 3466 "rcparse.c"
    break;

  case 158: /* optrcdata_data_int: %empty  */
#line 1198 "rcparse.y"
          {
	    (yyval.rcdata).first = NULL;
	    (yyval.rcdata).last = NULL;
	  }
#line 3475 "rcparse.c"
    break;

  case 159: /* optrcdata_data_int: rcdata_data  */
#line 1203 "rcparse.y"
          {
	    (yyval.rcdata) = (yyvsp[0].rcdata);
	  }
#line 3483 "rcparse.c"
    break;

  case 160: /* rcdata_data: sizedstring  */
#line 1210 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3495 "rcparse.c"
    break;

  case 161: /* rcdata_data: sizedunistring  */
#line 1218 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3507 "rcparse.c"
    break;

  case 162: /* rcdata_data: sizednumexpr  */
#line 1226 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3519 "rcparse.c"
    break;

  case 163: /* rcdata_data: rcdata_data ',' sizedstring  */
#line 1234 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_string ((yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3532 "rcparse.c"
    break;

  case 164: /* rcdata_data: rcdata_data ',' sizedunistring  */
#line 1243 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_unistring ((yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3545 "rcparse.c"
    break;

  case 165: /* rcdata_data: rcdata_data ',' sizednumexpr  */
#line 1252 "rcparse.y"
          {
	    rc_rcdata_item *ri;

	    ri = define_rcdata_number ((yyvsp[0].i).val, (yyvsp[0].i).dword);
	    (yyval.rcdata).first = (yyvsp[-2].rcdata).first;
	    (yyvsp[-2].rcdata).last->next = ri;
	    (yyval.rcdata).last = ri;
	  }
#line 3558 "rcparse.c"
    break;

  case 166: /* rcdata_data: rcdata_data ','  */
#line 1261 "rcparse.y"
          {
	    (yyval.rcdata)=(yyvsp[-1].rcdata);
	  }
#line 3566 "rcparse.c"
    break;

  case 167: /* $@29: %empty  */
#line 1270 "rcparse.y"
            { sub_res_info = (yyvsp[-1].res_info); rcparse_rcdata (); }
#line 3572 "rcparse.c"
    break;

  case 168: /* stringtable: STRINGTABLE suboptions BEG $@29 string_data END  */
#line 1271 "rcparse.y"
                            { rcparse_normal (); }
#line 3578 "rcparse.c"
    break;

  case 170: /* string_data: string_data numexpr res_unicode_sizedstring_concat  */
#line 1277 "rcparse.y"
          {
	    define_stringtable (&sub_res_info, (yyvsp[-1].il), (yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    rcparse_discard_strings ();
	  }
#line 3587 "rcparse.c"
    break;

  case 171: /* string_data: string_data numexpr ',' res_unicode_sizedstring_concat  */
#line 1282 "rcparse.y"
          {
	    define_stringtable (&sub_res_info, (yyvsp[-2].il), (yyvsp[0].suni).s, (yyvsp[0].suni).length);
	    rcparse_discard_strings ();
	  }
#line 3596 "rcparse.c"
    break;

  case 172: /* string_data: string_data error  */
#line 1287 "rcparse.y"
          {
	    rcparse_warning (_("invalid stringtable resource."));
	    abort ();
	  }
#line 3605 "rcparse.c"
    break;

  case 173: /* rcdata_id: id  */
#line 1295 "rcparse.y"
          {
	    (yyval.id)=(yyvsp[0].id);
	  }
#line 3613 "rcparse.c"
    break;

  case 174: /* rcdata_id: HTML  */
#line 1299 "rcparse.y"
        {
	  (yyval.id).named = 0;
	  (yyval.id).u.id = 23;
	}
#line 3622 "rcparse.c"
    break;

  case 175: /* rcdata_id: RCDATA  */
#line 1304 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_RCDATA;
        }
#line 3631 "rcparse.c"
    break;

  case 176: /* rcdata_id: MANIFEST  */
#line 1309 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_MANIFEST;
        }
#line 3640 "rcparse.c"
    break;

  case 177: /* rcdata_id: PLUGPLAY  */
#line 1314 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_PLUGPLAY;
        }
#line 3649 "rcparse.c"
    break;

  case 178: /* rcdata_id: VXD  */
#line 1319 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_VXD;
        }
#line 3658 "rcparse.c"
    break;

  case 179: /* rcdata_id: DLGINCLUDE  */
#line 1324 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINCLUDE;
        }
#line 3667 "rcparse.c"
    break;

  case 180: /* rcdata_id: DLGINIT  */
#line 1329 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_DLGINIT;
        }
#line 3676 "rcparse.c"
    break;

  case 181: /* rcdata_id: ANICURSOR  */
#line 1334 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANICURSOR;
        }
#line 3685 "rcparse.c"
    break;

  case 182: /* rcdata_id: ANIICON  */
#line 1339 "rcparse.y"
        {
          (yyval.id).named = 0;
          (yyval.id).u.id = RT_ANIICON;
        }
#line 3694 "rcparse.c"
    break;

  case 183: /* user: id rcdata_id suboptions BEG optrcdata_data END  */
#line 1350 "rcparse.y"
          {
	    define_user_data ((yyvsp[-5].id), (yyvsp[-4].id), &(yyvsp[-3].res_info), (yyvsp[-1].rcdata).first);
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3705 "rcparse.c"
    break;

  case 184: /* user: id rcdata_id suboptions file_name  */
#line 1357 "rcparse.y"
          {
	    define_user_file ((yyvsp[-3].id), (yyvsp[-2].id), &(yyvsp[-1].res_info), (yyvsp[0].s));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3716 "rcparse.c"
    break;

  case 185: /* toolbar: id TOOLBAR suboptions numexpr cnumexpr BEG toolbar_data END  */
#line 1367 "rcparse.y"
        {
	  define_toolbar ((yyvsp[-7].id), &(yyvsp[-5].res_info), (yyvsp[-4].il), (yyvsp[-3].il), (yyvsp[-1].toobar_item));
	}
#line 3724 "rcparse.c"
    break;

  case 186: /* toolbar_data: %empty  */
#line 1372 "rcparse.y"
                          { (yyval.toobar_item)= NULL; }
#line 3730 "rcparse.c"
    break;

  case 187: /* toolbar_data: toolbar_data BUTTON id  */
#line 1374 "rcparse.y"
        {
	  rc_toolbar_item *c,*n;
	  c = (yyvsp[-2].toobar_item);
	  n= (rc_toolbar_item *)
	      res_alloc (sizeof (rc_toolbar_item));
	  if (c != NULL)
	    while (c->next != NULL)
	      c = c->next;
	  n->prev = c;
	  n->next = NULL;
	  if (c != NULL)
	    c->next = n;
	  n->id = (yyvsp[0].id);
	  if ((yyvsp[-2].toobar_item) == NULL)
	    (yyval.toobar_item) = n;
	  else
	    (yyval.toobar_item) = (yyvsp[-2].toobar_item);
	}
#line 3753 "rcparse.c"
    break;

  case 188: /* toolbar_data: toolbar_data SEPARATOR  */
#line 1393 "rcparse.y"
        {
	  rc_toolbar_item *c,*n;
	  c = (yyvsp[-1].toobar_item);
	  n= (rc_toolbar_item *)
	      res_alloc (sizeof (rc_toolbar_item));
	  if (c != NULL)
	    while (c->next != NULL)
	      c = c->next;
	  n->prev = c;
	  n->next = NULL;
	  if (c != NULL)
	    c->next = n;
	  n->id.named = 0;
	  n->id.u.id = 0;
	  if ((yyvsp[-1].toobar_item) == NULL)
	    (yyval.toobar_item) = n;
	  else
	    (yyval.toobar_item) = (yyvsp[-1].toobar_item);
	}
#line 3777 "rcparse.c"
    break;

  case 189: /* versioninfo: id VERSIONINFO fixedverinfo BEG verblocks END  */
#line 1418 "rcparse.y"
          {
	    define_versioninfo ((yyvsp[-5].id), language, (yyvsp[-3].fixver), (yyvsp[-1].verinfo));
	    if (yychar != YYEMPTY)
	      YYERROR;
	    rcparse_discard_strings ();
	  }
#line 3788 "rcparse.c"
    break;

  case 190: /* fixedverinfo: %empty  */
#line 1428 "rcparse.y"
          {
	    (yyval.fixver) = ((rc_fixed_versioninfo *)
		  res_alloc (sizeof (rc_fixed_versioninfo)));
	    memset ((yyval.fixver), 0, sizeof (rc_fixed_versioninfo));
	  }
#line 3798 "rcparse.c"
    break;

  case 191: /* fixedverinfo: fixedverinfo FILEVERSION numexpr optcnumexpr optcnumexpr optcnumexpr  */
#line 1435 "rcparse.y"
          {
	    (yyvsp[-5].fixver)->file_version_ms = ((yyvsp[-3].il) << 16) | ((yyvsp[-2].il) & 0xffff);
	    (yyvsp[-5].fixver)->file_version_ls = ((yyvsp[-1].il) << 16) | ((yyvsp[0].il) & 0xffff);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
#line 3808 "rcparse.c"
    break;

  case 192: /* fixedverinfo: fixedverinfo PRODUCTVERSION numexpr optcnumexpr optcnumexpr optcnumexpr  */
#line 1442 "rcparse.y"
          {
	    (yyvsp[-5].fixver)->product_version_ms = ((yyvsp[-3].il) << 16) | ((yyvsp[-2].il) & 0xffff);
	    (yyvsp[-5].fixver)->product_version_ls = ((yyvsp[-1].il) << 16) | ((yyvsp[0].il) & 0xffff);
	    (yyval.fixver) = (yyvsp[-5].fixver);
	  }
#line 3818 "rcparse.c"
    break;

  case 193: /* fixedverinfo: fixedverinfo FILEFLAGSMASK numexpr  */
#line 1448 "rcparse.y"
          {
	    (yyvsp[-2].fixver)->file_flags_mask = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3827 "rcparse.c"
    break;

  case 194: /* fixedverinfo: fixedverinfo FILEFLAGS numexpr  */
#line 1453 "rcparse.y"
          {
	    (yyvsp[-2].fixver)->file_flags = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3836 "rcparse.c"
    break;

  case 195: /* fixedverinfo: fixedverinfo FILEOS numexpr  */
#line 1458 "rcparse.y"
          {
	    (yyvsp[-2].fixver)->file_os = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3845 "rcparse.c"
    break;

  case 196: /* fixedverinfo: fixedverinfo FILETYPE numexpr  */
#line 1463 "rcparse.y"
          {
	    (yyvsp[-2].fixver)->file_type = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3854 "rcparse.c"
    break;

  case 197: /* fixedverinfo: fixedverinfo FILESUBTYPE numexpr  */
#line 1468 "rcparse.y"
          {
	    (yyvsp[-2].fixver)->file_subtype = (yyvsp[0].il);
	    (yyval.fixver) = (yyvsp[-2].fixver);
	  }
#line 3863 "rcparse.c"
    break;

  case 198: /* verblocks: %empty  */
#line 1482 "rcparse.y"
          {
	    (yyval.verinfo) = NULL;
	  }
#line 3871 "rcparse.c"
    break;

  case 199: /* verblocks: verblocks BLOCKSTRINGFILEINFO BEG verstringtables END  */
#line 1486 "rcparse.y"
          {
	    (yyval.verinfo) = append_ver_stringfileinfo ((yyvsp[-4].verinfo), (yyvsp[-1].verstringtable));
	  }
#line 3879 "rcparse.c"
    break;

  case 200: /* verblocks: verblocks BLOCKVARFILEINFO BEG VALUE res_unicode_string_concat vertrans END  */
#line 1490 "rcparse.y"
          {
	    (yyval.verinfo) = append_ver_varfileinfo ((yyvsp[-6].verinfo), (yyvsp[-2].uni), (yyvsp[-1].vervar));
	  }
#line 3887 "rcparse.c"
    break;

  case 201: /* verstringtables: %empty  */
#line 1497 "rcparse.y"
          {
	    (yyval.verstringtable) = NULL;
	  }
#line 3895 "rcparse.c"
    break;

  case 202: /* verstringtables: verstringtables BLOCK BEG vervals END  */
#line 1501 "rcparse.y"
          {
	    (yyval.verstringtable) = append_ver_stringtable ((yyvsp[-4].verstringtable), (yyvsp[-3].s), (yyvsp[-1].verstring));
	  }
#line 3903 "rcparse.c"
    break;

  case 203: /* vervals: %empty  */
#line 1508 "rcparse.y"
          {
	    (yyval.verstring) = NULL;
	  }
#line 3911 "rcparse.c"
    break;

  case 204: /* vervals: vervals VALUE res_unicode_string_concat ',' res_unicode_string_concat  */
#line 1512 "rcparse.y"
          {
	    (yyval.verstring) = append_verval ((yyvsp[-4].verstring), (yyvsp[-2].uni), (yyvsp[0].uni));
	  }
#line 3919 "rcparse.c"
    break;

  case 205: /* vertrans: %empty  */
#line 1519 "rcparse.y"
          {
	    (yyval.vervar) = NULL;
	  }
#line 3927 "rcparse.c"
    break;

  case 206: /* vertrans: vertrans cnumexpr cnumexpr  */
#line 1523 "rcparse.y"
          {
	    (yyval.vervar) = append_vertrans ((yyvsp[-2].vervar), (yyvsp[-1].il), (yyvsp[0].il));
	  }
#line 3935 "rcparse.c"
    break;

  case 207: /* id: posnumexpr  */
#line 1532 "rcparse.y"
          {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[0].il);
	  }
#line 3944 "rcparse.c"
    break;

  case 208: /* id: resname  */
#line 1537 "rcparse.y"
          {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
#line 3952 "rcparse.c"
    break;

  case 209: /* resname: res_unicode_string  */
#line 1546 "rcparse.y"
          {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
#line 3960 "rcparse.c"
    break;

  case 210: /* resname: STRING  */
#line 1550 "rcparse.y"
          {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
#line 3970 "rcparse.c"
    break;

  case 211: /* resref: posnumexpr ','  */
#line 1560 "rcparse.y"
          {
	    (yyval.id).named = 0;
	    (yyval.id).u.id = (yyvsp[-1].il);
	  }
#line 3979 "rcparse.c"
    break;

  case 212: /* resref: resname  */
#line 1565 "rcparse.y"
          {
	    res_unistring_to_id (&(yyval.id), (yyvsp[0].uni));
	  }
#line 3987 "rcparse.c"
    break;

  case 213: /* resref: resname ','  */
#line 1569 "rcparse.y"
          {
	    res_unistring_to_id (&(yyval.id), (yyvsp[-1].uni));
	  }
#line 3995 "rcparse.c"
    break;

  case 214: /* suboptions: %empty  */
#line 1579 "rcparse.y"
          {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    /* FIXME: Is this the right default?  */
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
#line 4006 "rcparse.c"
    break;

  case 215: /* suboptions: suboptions memflag  */
#line 1586 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 4016 "rcparse.c"
    break;

  case 216: /* suboptions: suboptions CHARACTERISTICS numexpr  */
#line 1592 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).characteristics = (yyvsp[0].il);
	  }
#line 4025 "rcparse.c"
    break;

  case 217: /* suboptions: suboptions LANGUAGE numexpr cnumexpr  */
#line 1597 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-3].res_info);
	    (yyval.res_info).language = (yyvsp[-1].il) | ((yyvsp[0].il) << SUBLANG_SHIFT);
	  }
#line 4034 "rcparse.c"
    break;

  case 218: /* suboptions: suboptions VERSIONK numexpr  */
#line 1602 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-2].res_info);
	    (yyval.res_info).version = (yyvsp[0].il);
	  }
#line 4043 "rcparse.c"
    break;

  case 219: /* memflags_move_discard: %empty  */
#line 1612 "rcparse.y"
          {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_DISCARDABLE;
	  }
#line 4053 "rcparse.c"
    break;

  case 220: /* memflags_move_discard: memflags_move_discard memflag  */
#line 1618 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 4063 "rcparse.c"
    break;

  case 221: /* memflags_move: %empty  */
#line 1629 "rcparse.y"
          {
	    memset (&(yyval.res_info), 0, sizeof (rc_res_res_info));
	    (yyval.res_info).language = language;
	    (yyval.res_info).memflags = MEMFLAG_MOVEABLE | MEMFLAG_PURE | MEMFLAG_DISCARDABLE;
	  }
#line 4073 "rcparse.c"
    break;

  case 222: /* memflags_move: memflags_move memflag  */
#line 1635 "rcparse.y"
          {
	    (yyval.res_info) = (yyvsp[-1].res_info);
	    (yyval.res_info).memflags |= (yyvsp[0].memflags).on;
	    (yyval.res_info).memflags &=~ (yyvsp[0].memflags).off;
	  }
#line 4083 "rcparse.c"
    break;

  case 223: /* memflag: MOVEABLE  */
#line 1647 "rcparse.y"
          {
	    (yyval.memflags).on = MEMFLAG_MOVEABLE;
	    (yyval.memflags).off = 0;
	  }
#line 4092 "rcparse.c"
    break;

  case 224: /* memflag: FIXED  */
#line 1652 "rcparse.y"
          {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_MOVEABLE;
	  }
#line 4101 "rcparse.c"
    break;

  case 225: /* memflag: PURE  */
#line 1657 "rcparse.y"
          {
	    (yyval.memflags).on = MEMFLAG_PURE;
	    (yyval.memflags).off = 0;
	  }
#line 4110 "rcparse.c"
    break;

  case 226: /* memflag: IMPURE  */
#line 1662 "rcparse.y"
          {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PURE;
	  }
#line 4119 "rcparse.c"
    break;

  case 227: /* memflag: PRELOAD  */
#line 1667 "rcparse.y"
          {
	    (yyval.memflags).on = MEMFLAG_PRELOAD;
	    (yyval.memflags).off = 0;
	  }
#line 4128 "rcparse.c"
    break;

  case 228: /* memflag: LOADONCALL  */
#line 1672 "rcparse.y"
          {
	    (yyval.memflags).on = 0;
	    (yyval.memflags).off = MEMFLAG_PRELOAD;
	  }
#line 4137 "rcparse.c"
    break;

  case 229: /* memflag: DISCARDABLE  */
#line 1677 "rcparse.y"
          {
	    (yyval.memflags).on = MEMFLAG_DISCARDABLE;
	    (yyval.memflags).off = 0;
	  }
#line 4146 "rcparse.c"
    break;

  case 230: /* file_name: QUOTEDSTRING  */
#line 1687 "rcparse.y"
          {
	    (yyval.s) = (yyvsp[0].s);
	  }
#line 4154 "rcparse.c"
    break;

  case 231: /* file_name: STRING  */
#line 1691 "rcparse.y"
          {
	    (yyval.s) = (yyvsp[0].s);
	  }
#line 4162 "rcparse.c"
    break;

  case 232: /* res_unicode_string_concat: res_unicode_string  */
#line 1699 "rcparse.y"
          {
	    (yyval.uni) = (yyvsp[0].uni);
	  }
#line 4170 "rcparse.c"
    break;

  case 233: /* res_unicode_string_concat: res_unicode_string_concat res_unicode_string  */
#line 1704 "rcparse.y"
          {
	    rc_uint_type l1 = unichar_len ((yyvsp[-1].uni));
	    rc_uint_type l2 = unichar_len ((yyvsp[0].uni));
	    unichar *h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1 != 0)
	      memcpy (h, (yyvsp[-1].uni), l1 * sizeof (unichar));
	    if (l2 != 0)
	      memcpy (h + l1, (yyvsp[0].uni), l2  * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.uni) = h;
	  }
#line 4186 "rcparse.c"
    break;

  case 234: /* res_unicode_string: QUOTEDUNISTRING  */
#line 1719 "rcparse.y"
          {
	    (yyval.uni) = unichar_dup ((yyvsp[0].uni));
	  }
#line 4194 "rcparse.c"
    break;

  case 235: /* res_unicode_string: QUOTEDSTRING  */
#line 1723 "rcparse.y"
          {
	    unichar *h = NULL;
	    unicode_from_ascii ((rc_uint_type *) NULL, &h, (yyvsp[0].s));
	    (yyval.uni) = h;
	  }
#line 4204 "rcparse.c"
    break;

  case 236: /* res_unicode_sizedstring: sizedunistring  */
#line 1732 "rcparse.y"
          {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4212 "rcparse.c"
    break;

  case 237: /* res_unicode_sizedstring: sizedstring  */
#line 1736 "rcparse.y"
          {
	    unichar *h = NULL;
	    rc_uint_type l = 0;
	    unicode_from_ascii_len (&l, &h, (yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.suni).s = h;
	    (yyval.suni).length = l;
	  }
#line 4224 "rcparse.c"
    break;

  case 238: /* res_unicode_sizedstring_concat: res_unicode_sizedstring  */
#line 1748 "rcparse.y"
          {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4232 "rcparse.c"
    break;

  case 239: /* res_unicode_sizedstring_concat: res_unicode_sizedstring_concat res_unicode_sizedstring  */
#line 1753 "rcparse.y"
          {
	    rc_uint_type l1 = (yyvsp[-1].suni).length;
	    rc_uint_type l2 = (yyvsp[0].suni).length;
	    unichar *h = (unichar *) res_alloc ((l1 + l2 + 1) * sizeof (unichar));
	    if (l1 != 0)
	      memcpy (h, (yyvsp[-1].suni).s, l1 * sizeof (unichar));
	    if (l2 != 0)
	      memcpy (h + l1, (yyvsp[0].suni).s, l2  * sizeof (unichar));
	    h[l1 + l2] = 0;
	    (yyval.suni).length = l1 + l2;
	    (yyval.suni).s = h;
	  }
#line 4249 "rcparse.c"
    break;

  case 240: /* sizedstring: SIZEDSTRING  */
#line 1769 "rcparse.y"
          {
	    (yyval.ss) = (yyvsp[0].ss);
	  }
#line 4257 "rcparse.c"
    break;

  case 241: /* sizedstring: sizedstring SIZEDSTRING  */
#line 1773 "rcparse.y"
          {
	    rc_uint_type l = (yyvsp[-1].ss).length + (yyvsp[0].ss).length;
	    char *h = (char *) res_alloc (l);
	    memcpy (h, (yyvsp[-1].ss).s, (yyvsp[-1].ss).length);
	    memcpy (h + (yyvsp[-1].ss).length, (yyvsp[0].ss).s, (yyvsp[0].ss).length);
	    (yyval.ss).s = h;
	    (yyval.ss).length = l;
	  }
#line 4270 "rcparse.c"
    break;

  case 242: /* sizedunistring: SIZEDUNISTRING  */
#line 1785 "rcparse.y"
          {
	    (yyval.suni) = (yyvsp[0].suni);
	  }
#line 4278 "rcparse.c"
    break;

  case 243: /* sizedunistring: sizedunistring SIZEDUNISTRING  */
#line 1789 "rcparse.y"
          {
	    rc_uint_type l = (yyvsp[-1].suni).length + (yyvsp[0].suni).length;
	    unichar *h = (unichar *) res_alloc (l * sizeof (unichar));
	    memcpy (h, (yyvsp[-1].suni).s, (yyvsp[-1].suni).length * sizeof (unichar));
	    memcpy (h + (yyvsp[-1].suni).length, (yyvsp[0].suni).s, (yyvsp[0].suni).length  * sizeof (unichar));
	    (yyval.suni).s = h;
	    (yyval.suni).length = l;
	  }
#line 4291 "rcparse.c"
    break;

  case 244: /* styleexpr: parennumber  */
#line 1811 "rcparse.y"
          {
	    style |= (yyvsp[0].il);
	  }
#line 4299 "rcparse.c"
    break;

  case 245: /* styleexpr: NOT parennumber  */
#line 1815 "rcparse.y"
          {
	    style &=~ (yyvsp[0].il);
	  }
#line 4307 "rcparse.c"
    break;

  case 246: /* styleexpr: styleexpr '|' parennumber  */
#line 1819 "rcparse.y"
          {
	    style |= (yyvsp[0].il);
	  }
#line 4315 "rcparse.c"
    break;

  case 247: /* styleexpr: styleexpr '|' NOT parennumber  */
#line 1823 "rcparse.y"
          {
	    style &=~ (yyvsp[0].il);
	  }
#line 4323 "rcparse.c"
    break;

  case 248: /* parennumber: NUMBER  */
#line 1830 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4331 "rcparse.c"
    break;

  case 249: /* parennumber: '(' numexpr ')'  */
#line 1834 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[-1].il);
	  }
#line 4339 "rcparse.c"
    break;

  case 250: /* optcnumexpr: %empty  */
#line 1843 "rcparse.y"
          {
	    (yyval.il) = 0;
	  }
#line 4347 "rcparse.c"
    break;

  case 251: /* optcnumexpr: cnumexpr  */
#line 1847 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4355 "rcparse.c"
    break;

  case 252: /* cnumexpr: ',' numexpr  */
#line 1856 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4363 "rcparse.c"
    break;

  case 253: /* numexpr: sizednumexpr  */
#line 1865 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4371 "rcparse.c"
    break;

  case 254: /* sizednumexpr: NUMBER  */
#line 1874 "rcparse.y"
          {
	    (yyval.i) = (yyvsp[0].i);
	  }
#line 4379 "rcparse.c"
    break;

  case 255: /* sizednumexpr: '(' sizednumexpr ')'  */
#line 1878 "rcparse.y"
          {
	    (yyval.i) = (yyvsp[-1].i);
	  }
#line 4387 "rcparse.c"
    break;

  case 256: /* sizednumexpr: '~' sizednumexpr  */
#line 1882 "rcparse.y"
          {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4396 "rcparse.c"
    break;

  case 257: /* sizednumexpr: '-' sizednumexpr  */
#line 1887 "rcparse.y"
          {
	    (yyval.i).val = - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4405 "rcparse.c"
    break;

  case 258: /* sizednumexpr: sizednumexpr '*' sizednumexpr  */
#line 1892 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4414 "rcparse.c"
    break;

  case 259: /* sizednumexpr: sizednumexpr '/' sizednumexpr  */
#line 1897 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val / ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4423 "rcparse.c"
    break;

  case 260: /* sizednumexpr: sizednumexpr '%' sizednumexpr  */
#line 1902 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val % ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4432 "rcparse.c"
    break;

  case 261: /* sizednumexpr: sizednumexpr '+' sizednumexpr  */
#line 1907 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4441 "rcparse.c"
    break;

  case 262: /* sizednumexpr: sizednumexpr '-' sizednumexpr  */
#line 1912 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4450 "rcparse.c"
    break;

  case 263: /* sizednumexpr: sizednumexpr '&' sizednumexpr  */
#line 1917 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4459 "rcparse.c"
    break;

  case 264: /* sizednumexpr: sizednumexpr '^' sizednumexpr  */
#line 1922 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4468 "rcparse.c"
    break;

  case 265: /* sizednumexpr: sizednumexpr '|' sizednumexpr  */
#line 1927 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4477 "rcparse.c"
    break;

  case 266: /* cposnumexpr: ',' posnumexpr  */
#line 1938 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].il);
	  }
#line 4485 "rcparse.c"
    break;

  case 267: /* posnumexpr: sizedposnumexpr  */
#line 1947 "rcparse.y"
          {
	    (yyval.il) = (yyvsp[0].i).val;
	  }
#line 4493 "rcparse.c"
    break;

  case 268: /* sizedposnumexpr: NUMBER  */
#line 1958 "rcparse.y"
          {
	    (yyval.i) = (yyvsp[0].i);
	  }
#line 4501 "rcparse.c"
    break;

  case 269: /* sizedposnumexpr: '(' sizednumexpr ')'  */
#line 1962 "rcparse.y"
          {
	    (yyval.i) = (yyvsp[-1].i);
	  }
#line 4509 "rcparse.c"
    break;

  case 270: /* sizedposnumexpr: '~' sizednumexpr  */
#line 1966 "rcparse.y"
          {
	    (yyval.i).val = ~ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[0].i).dword;
	  }
#line 4518 "rcparse.c"
    break;

  case 271: /* sizedposnumexpr: sizedposnumexpr '*' sizednumexpr  */
#line 1971 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val * (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4527 "rcparse.c"
    break;

  case 272: /* sizedposnumexpr: sizedposnumexpr '/' sizednumexpr  */
#line 1976 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val / ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4536 "rcparse.c"
    break;

  case 273: /* sizedposnumexpr: sizedposnumexpr '%' sizednumexpr  */
#line 1981 "rcparse.y"
          {
	    /* PR 17512: file: 89105a25.  */
	    (yyval.i).val = (yyvsp[-2].i).val % ((yyvsp[0].i).val ? (yyvsp[0].i).val : 1);
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4546 "rcparse.c"
    break;

  case 274: /* sizedposnumexpr: sizedposnumexpr '+' sizednumexpr  */
#line 1987 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val + (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4555 "rcparse.c"
    break;

  case 275: /* sizedposnumexpr: sizedposnumexpr '-' sizednumexpr  */
#line 1992 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val - (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4564 "rcparse.c"
    break;

  case 276: /* sizedposnumexpr: sizedposnumexpr '&' sizednumexpr  */
#line 1997 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val & (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4573 "rcparse.c"
    break;

  case 277: /* sizedposnumexpr: sizedposnumexpr '^' sizednumexpr  */
#line 2002 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val ^ (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4582 "rcparse.c"
    break;

  case 278: /* sizedposnumexpr: sizedposnumexpr '|' sizednumexpr  */
#line 2007 "rcparse.y"
          {
	    (yyval.i).val = (yyvsp[-2].i).val | (yyvsp[0].i).val;
	    (yyval.i).dword = (yyvsp[-2].i).dword || (yyvsp[0].i).dword;
	  }
#line 4591 "rcparse.c"
    break;


#line 4595 "rcparse.c"

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

#line 2013 "rcparse.y"


/* Set the language from the command line.  */

void
rcparse_set_language (int lang)
{
  language = lang;
}
