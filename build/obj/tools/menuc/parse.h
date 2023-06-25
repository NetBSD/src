#ifndef _yy_defines_h_
#define _yy_defines_h_

#define X 257
#define Y 258
#define W 259
#define H 260
#define NO 261
#define BOX 262
#define SUB 263
#define HELP 264
#define MENU 265
#define NEXT 266
#define EXIT 267
#define ACTION 268
#define ENDWIN 269
#define OPTION 270
#define TITLE 271
#define DEFAULT 272
#define DISPLAY 273
#define ERROR 274
#define EXITSTRING 275
#define EXPAND 276
#define ALLOW 277
#define DYNAMIC 278
#define MENUS 279
#define SCROLLABLE 280
#define SHORTCUT 281
#define CLEAR 282
#define MESSAGES 283
#define ALWAYS 284
#define SCROLL 285
#define CONTINUOUS 286
#define STRING 287
#define NAME 288
#define CODE 289
#define INT_CONST 290
#define CHAR_CONST 291
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	char *s_value;
	int   i_value;
	optn_info *optn_value;
	action a_value;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
