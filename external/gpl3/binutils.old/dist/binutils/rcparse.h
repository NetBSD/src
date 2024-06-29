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

#line 311 "rcparse.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_RCPARSE_H_INCLUDED  */
