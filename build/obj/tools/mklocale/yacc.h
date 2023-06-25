#ifndef _yy_defines_h_
#define _yy_defines_h_

#define RUNE 257
#define LBRK 258
#define RBRK 259
#define THRU 260
#define MAPLOWER 261
#define MAPUPPER 262
#define DIGITMAP 263
#define LIST 264
#define VARIABLE 265
#define CHARSET 266
#define ENCODING 267
#define INVALID 268
#define STRING 269
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union	{
    __nbrune_t	rune;
    int		i;
    char	*str;

    rune_list	*list;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
