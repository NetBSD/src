#	$NetBSD: Makefile.inc,v 1.14 2000/10/10 20:24:49 is Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for LDSTATIC def'n

WARNS?=		1
BINDIR?=	/sbin
LDSTATIC?=	-static
WFORMAT?=	2
