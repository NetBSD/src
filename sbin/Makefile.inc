#	$NetBSD: Makefile.inc,v 1.18 2002/09/27 21:04:46 thorpej Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		2
BINDIR?=	/sbin
WFORMAT?=	2

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
