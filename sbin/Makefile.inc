#	$NetBSD: Makefile.inc,v 1.21.6.1 2012/04/17 00:05:38 yamt Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
