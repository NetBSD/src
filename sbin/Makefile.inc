#	$NetBSD: Makefile.inc,v 1.15 2001/12/05 05:50:22 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for LDSTATIC def'n

WARNS?=		2
BINDIR?=	/sbin
LDSTATIC?=	-static
WFORMAT?=	2
