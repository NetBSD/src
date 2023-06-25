#ifndef _yy_defines_h_
#define _yy_defines_h_

#define R_TYPE 257
#define R_NAME 258
#define R_SRC_ZONE 259
#define R_DST_UNIT_BITS 260
#define R_DST_INVALID 261
#define R_DST_ILSEQ 262
#define R_BEGIN_MAP 263
#define R_END_MAP 264
#define R_INVALID 265
#define R_ROWCOL 266
#define R_ILSEQ 267
#define R_OOB_MODE 268
#define R_LN 269
#define L_IMM 270
#define L_STRING 271
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	u_int32_t	i_value;
	char		*s_value;
	linear_zone_t	lz_value;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
