#	$NetBSD: Makefile.inc,v 1.13.10.1 2000/10/18 00:39:41 tv Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for LDSTATIC def'n

WARNS?=		1
BINDIR?=	/sbin
LDSTATIC?=	-static
WFORMAT?=	2
