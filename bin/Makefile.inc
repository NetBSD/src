#	$NetBSD: Makefile.inc,v 1.11 2001/11/03 13:40:30 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for LDSTATIC def'n

WARNS?=		2
BINDIR?=	/bin
LDSTATIC?=	-static
