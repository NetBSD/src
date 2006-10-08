#	$NetBSD: Makefile.inc,v 1.20 2006/10/08 17:52:28 peter Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		3
BINDIR?=	/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
