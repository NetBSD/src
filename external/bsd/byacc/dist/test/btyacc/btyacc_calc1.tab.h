/*	$NetBSD: btyacc_calc1.tab.h,v 1.1.1.3.14.1 2018/12/26 14:01:14 pgoyette Exp $	*/

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
typedef union
{
	int ival;
	double dval;
	INTERVAL vval;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */

#endif /* _calc1__defines_h_ */
