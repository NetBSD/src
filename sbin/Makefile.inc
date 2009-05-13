#	$NetBSD: Makefile.inc,v 1.20.28.1 2009/05/13 19:18:59 jym Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		4
BINDIR?=	/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
