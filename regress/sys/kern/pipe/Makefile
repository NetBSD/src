#	$NetBSD: Makefile,v 1.2 2001/10/31 15:57:56 tv Exp $

PROG=	pipe1
MKMAN=	no

all:	${PROG}

regress: ${PROG}
	@./${PROG} || ( echo 'pipe write restart test failed' && exit 1 )

.include <bsd.prog.mk>
