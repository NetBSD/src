#	@(#)Makefile	5.6 (Berkeley) 6/29/90
#
#	$Header: /cvsroot/src/sbin/reboot/Makefile,v 1.3 1993/03/23 00:29:51 cgd Exp $

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil

MAN8=	reboot_${MACHINE}.0
MLINKS=	reboot_${MACHINE}.8 reboot.8
MANSUBDIR=/${MACHINE}

.include <bsd.prog.mk>
