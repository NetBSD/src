#	$NetBSD: Makefile,v 1.5 2006/05/11 23:16:28 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	bcd
MAN=	bcd.6
MLINKS=	bcd.6 morse.6 bcd.6 ppt.6
HIDEGAME=hidegame

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif
