#	$NetBSD: Makefile.inc,v 1.16 2002/08/27 14:46:14 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		2
BINDIR?=	/sbin
WFORMAT?=	2

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
