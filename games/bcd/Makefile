#	$NetBSD: Makefile,v 1.7 2008/08/29 00:02:22 gmcgarry Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	bcd
MAN=	bcd.6
MLINKS=	bcd.6 morse.6 bcd.6 ppt.6
HIDEGAME=hidegame

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.bcd.c+=	-Wno-pointer-sign
.endif
