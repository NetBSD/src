#	$NetBSD: Makefile,v 1.7 1995/03/18 16:11:33 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	reboot.8 boot_hp300.8 boot_i386.8 boot_sparc.8 boot_tahoe.8 boot_vax.8
MLINKS=	reboot.8 halt.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt

.include <bsd.prog.mk>
