#	from: @(#)Makefile	5.6 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.5 1994/12/22 10:48:06 cgd Exp $

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil

MAN=	reboot_${MACHINE}.8
MLINKS=	reboot_${MACHINE}.8 reboot.8
MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>
