#	$NetBSD: Makefile.inc,v 1.13 2002/09/27 15:56:39 thorpej Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

BINDIR?=	/bin

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		2

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
