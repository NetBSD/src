#	from: @(#)Makefile	5.6 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.4 1993/08/01 05:26:50 mycroft Exp $

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil

MAN8=	reboot_${MACHINE}.0
MLINKS=	reboot_${MACHINE}.8 reboot.8
MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>
