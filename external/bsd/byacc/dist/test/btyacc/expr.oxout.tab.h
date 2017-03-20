/*	$NetBSD: expr.oxout.tab.h,v 1.2.2.2 2017/03/20 06:52:18 pgoyette Exp $	*/

#ifndef _expr.oxout__defines_h_
#define _expr.oxout__defines_h_

#define ID 257
#define CONST 258
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
struct yyyOxAttrbs {
struct yyyStackItem *yyyOxStackItem;
} yyyOxAttrbs;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE expr.oxout_lval;

#endif /* _expr.oxout__defines_h_ */
