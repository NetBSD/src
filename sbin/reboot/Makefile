#	@(#)Makefile	5.6 (Berkeley) 6/29/90
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil

MAN8=	reboot_${MACHINE}.0
MLINKS=	reboot_${MACHINE}.8 reboot.8
MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>
