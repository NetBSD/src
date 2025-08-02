/*	$NetBSD: expr.oxout.tab.h,v 1.2.28.1 2025/08/02 05:20:59 perseant Exp $	*/

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
