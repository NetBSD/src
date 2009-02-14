#	$NetBSD: Makefile.inc,v 1.16 2009/02/14 08:31:13 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		4
BINDIR?=	/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
