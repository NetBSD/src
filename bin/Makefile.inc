#	$NetBSD: Makefile.inc,v 1.17.34.2 2020/04/21 19:37:29 martin Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
