#	$NetBSD: Makefile.inc,v 1.13 1998/03/02 22:03:11 cgd Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for LDSTATIC def'n

WARNS?=		1
BINDIR?=	/sbin
LDSTATIC?=	-static
