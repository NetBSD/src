#	$NetBSD: Makefile.inc,v 1.17 2002/09/27 15:56:40 thorpej Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

BINDIR?=	/sbin

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		2
WFORMAT?=	2

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
