#	$NetBSD: Makefile,v 1.15 1997/07/19 23:04:24 perry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	reboot.8
MLINKS=	reboot.8 halt.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt
WARNS=	1

.include <bsd.prog.mk>
