#	$NetBSD: Makefile.inc,v 1.20.32.1 2009/08/27 22:48:36 matt Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		3
BINDIR?=	/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC+=	-static
.endif
