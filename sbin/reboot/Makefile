#	$NetBSD: Makefile,v 1.13 1996/10/17 01:05:57 perry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	reboot
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	reboot.8 boot_atari.8 boot_hp300.8 boot_i386.8 boot_mac68k.8 \
	boot_pmax.8 boot_sparc.8 boot_sun3.8 boot_vax.8 boot_x68k.8
MLINKS=	reboot.8 halt.8
LINKS=	${BINDIR}/reboot ${BINDIR}/halt

.include <bsd.prog.mk>
