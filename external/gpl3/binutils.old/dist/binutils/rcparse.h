/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

#ifndef YY_YY_RCPARSE_H_INCLUDED
# define YY_YY_RCPARSE_H_INCLUDED
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
    BEG = 258,
    END = 259,
    ACCELERATORS = 260,
    VIRTKEY = 261,
    ASCII = 262,
    NOINVERT = 263,
    SHIFT = 264,
    CONTROL = 265,
    ALT = 266,
    BITMAP = 267,
    CURSOR = 268,
    DIALOG = 269,
    DIALOGEX = 270,
    EXSTYLE = 271,
    CAPTION = 272,
    CLASS = 273,
    STYLE = 274,
    AUTO3STATE = 275,
    AUTOCHECKBOX = 276,
    AUTORADIOBUTTON = 277,
    CHECKBOX = 278,
    COMBOBOX = 279,
    CTEXT = 280,
    DEFPUSHBUTTON = 281,
    EDITTEXT = 282,
    GROUPBOX = 283,
    LISTBOX = 284,
    LTEXT = 285,
    PUSHBOX = 286,
    PUSHBUTTON = 287,
    RADIOBUTTON = 288,
    RTEXT = 289,
    SCROLLBAR = 290,
    STATE3 = 291,
    USERBUTTON = 292,
    BEDIT = 293,
    HEDIT = 294,
    IEDIT = 295,
    FONT = 296,
    ICON = 297,
    ANICURSOR = 298,
    ANIICON = 299,
    DLGINCLUDE = 300,
    DLGINIT = 301,
    FONTDIR = 302,
    HTML = 303,
    MANIFEST = 304,
    PLUGPLAY = 305,
    VXD = 306,
    TOOLBAR = 307,
    BUTTON = 308,
    LANGUAGE = 309,
    CHARACTERISTICS = 310,
    VERSIONK = 311,
    MENU = 312,
    MENUEX = 313,
    MENUITEM = 314,
    SEPARATOR = 315,
    POPUP = 316,
    CHECKED = 317,
    GRAYED = 318,
    HELP = 319,
    INACTIVE = 320,
    MENUBARBREAK = 321,
    MENUBREAK = 322,
    MESSAGETABLE = 323,
    RCDATA = 324,
    STRINGTABLE = 325,
    VERSIONINFO = 326,
    FILEVERSION = 327,
    PRODUCTVERSION = 328,
    FILEFLAGSMASK = 329,
    FILEFLAGS = 330,
    FILEOS = 331,
    FILETYPE = 332,
    FILESUBTYPE = 333,
    BLOCKSTRINGFILEINFO = 334,
    BLOCKVARFILEINFO = 335,
    VALUE = 336,
    BLOCK = 337,
    MOVEABLE = 338,
    FIXED = 339,
    PURE = 340,
    IMPURE = 341,
    PRELOAD = 342,
    LOADONCALL = 343,
    DISCARDABLE = 344,
    NOT = 345,
    QUOTEDUNISTRING = 346,
    QUOTEDSTRING = 347,
    STRING = 348,
    NUMBER = 349,
    SIZEDUNISTRING = 350,
    SIZEDSTRING = 351,
    IGNORED_TOKEN = 352,
    NEG = 353
  };
#endif
/* Tokens.  */
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
#define MENUBARBREAK 321
#define MENUBREAK 322
#define MESSAGETABLE 323
#define RCDATA 324
#define STRINGTABLE 325
#define VERSIONINFO 326
#define FILEVERSION 327
#define PRODUCTVERSION 328
#define FILEFLAGSMASK 329
#define FILEFLAGS 330
#define FILEOS 331
#define FILETYPE 332
#define FILESUBTYPE 333
#define BLOCKSTRINGFILEINFO 334
#define BLOCKVARFILEINFO 335
#define VALUE 336
#define BLOCK 337
#define MOVEABLE 338
#define FIXED 339
#define PURE 340
#define IMPURE 341
#define PRELOAD 342
#define LOADONCALL 343
#define DISCARDABLE 344
#define NOT 345
#define QUOTEDUNISTRING 346
#define QUOTEDSTRING 347
#define STRING 348
#define NUMBER 349
#define SIZEDUNISTRING 350
#define SIZEDSTRING 351
#define IGNORED_TOKEN 352
#define NEG 353

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 68 "rcparse.y" /* yacc.c:1909  */

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

#line 296 "rcparse.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_RCPARSE_H_INCLUDED  */
