#	$NetBSD: Makefile,v 1.6 1995/03/18 14:59:24 cgd Exp $
#	@(#)Makefile	5.6 (Berkeley) 6/29/90

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil

MAN=	reboot_${MACHINE}.8
MLINKS=	reboot_${MACHINE}.8 reboot.8
MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>
