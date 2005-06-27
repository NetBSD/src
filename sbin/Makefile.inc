#	$NetBSD: Makefile.inc,v 1.19 2005/06/27 01:00:04 christos Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		3
BINDIR?=	/sbin
WFORMAT?=	2

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
