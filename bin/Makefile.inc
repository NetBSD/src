#	$NetBSD: Makefile.inc,v 1.12 2002/08/27 14:46:11 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		2
BINDIR?=	/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
