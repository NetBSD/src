#ifndef _yy_defines_h_
#define _yy_defines_h_

#define R_NAME 257
#define R_ENCODING 258
#define R_VARIABLE 259
#define R_DEFCSID 260
#define R_INVALID 261
#define R_LN 262
#define L_IMM 263
#define L_STRING 264
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	u_int32_t	i_value;
	char		*s_value;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
