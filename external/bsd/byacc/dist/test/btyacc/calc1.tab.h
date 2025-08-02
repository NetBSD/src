/*	$NetBSD: calc1.tab.h,v 1.1.1.3.28.1 2025/08/02 05:20:56 perseant Exp $	*/

#ifndef _calc1__defines_h_
#define _calc1__defines_h_

#define DREG 257
#define VREG 258
#define CONST 259
#define UMINUS 260
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE
{
	int ival;
	double dval;
	INTERVAL vval;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE calc1_lval;

#endif /* _calc1__defines_h_ */
