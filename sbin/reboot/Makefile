#	$NetBSD: Makefile,v 1.11 1996/06/26 05:22:58 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	reboot.8 boot_hp300.8 boot_i386.8 boot_mac68k.8 boot_pmax.8 \
	boot_sparc.8 boot_sun3.8 boot_tahoe.8 boot_vax.8 boot_x68k.8
MLINKS=	reboot.8 halt.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt

.include <bsd.prog.mk>
