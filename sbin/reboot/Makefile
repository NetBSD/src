#	$NetBSD: Makefile,v 1.17 2000/04/25 14:03:26 hubertf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	reboot.8
MLINKS=	reboot.8 halt.8 \
	reboot.8 poweroff.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt \
	${BINDIR}/reboot ${BINDIR}/poweroff

.include <bsd.prog.mk>
