/*	$NetBSD: expr.oxout.tab.h,v 1.1.1.2 2024/09/14 21:25:39 christos Exp $	*/

#ifndef _expr_oxout__defines_h_
#define _expr_oxout__defines_h_

#define ID 257
#define CONST 258
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE {
struct yyyOxAttrbs {
struct yyyStackItem *yyyOxStackItem;
} yyyOxAttrbs;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE expr_oxout_lval;

#endif /* _expr_oxout__defines_h_ */
