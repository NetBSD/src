#	$NetBSD: Makefile,v 1.1 2001/09/29 13:54:20 jdolecek Exp $

PROG=	pipe1
MKMAN=	no

all:	${PROG}

regress: ${PROG}
	@${.OBJDIR}/${PROG} || ( echo 'pipe write restart test failed' && exit 1 )

.include <bsd.prog.mk>
