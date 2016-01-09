/*	$NetBSD: calc1.tab.h,v 1.1.1.3 2016/01/09 21:59:45 christos Exp $	*/

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
extern YYSTYPE calc1_lval;
