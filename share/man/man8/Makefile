#	$NetBSD: Makefile,v 1.58 2002/06/18 13:30:03 wiz Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN=	compat_freebsd.8 compat_hpux.8 compat_ibcs2.8 compat_linux.8 \
	compat_netbsd32.8 compat_osf1.8 compat_pecoff.8 compat_sunos.8 \
	compat_svr4.8 compat_ultrix.8 diskless.8 genassym.sh.8 intro.8 \
	rc.8 rc.subr.8 sticky.8 yp.8
MLINKS+=compat_svr4.8 compat_sco.8
MLINKS+=compat_svr4.8 compat_solaris.8
MLINKS+=compat_svr4.8 compat_svr4_32.8
MLINKS+=compat_svr4.8 compat_xenix.8
MLINKS+=compat_pecoff.8 compat_win32.8
MLINKS+=compat_pecoff.8 peace.8
MLINKS+=compat_netbsd32.8 netbsd32.8
MLINKS+=yp.8 nis.8
MLINKS+=rc.8 rc.d.8
MLINKS+=rc.8 rc.shutdown.8
SUBDIR=	man8.acorn26 man8.acorn32 man8.algor man8.alpha man8.amiga \
	man8.arc man8.atari man8.bebox man8.cats man8.cesfic \
	man8.cobalt man8.dreamcast man8.evbarm man8.evbmips man8.evbsh3 \
	man8.hp300 man8.hp700 man8.hpcarm man8.hpcmips man8.hpcsh \
	man8.i386 man8.luna68k \
	man8.mac68k man8.macppc man8.mipsco man8.mmeye man8.mvme68k \
	man8.mvmeppc man8.netwinder man8.news68k man8.newsmips \
	man8.next68k man8.pc532 man8.playstation2 man8.pmax \
	man8.pmppc man8.prep man8.sandpoint man8.sbmips man8.sgimips \
	man8.shark man8.sparc man8.sparc64 \
	man8.sun2 man8.sun3 man8.vax man8.walnut man8.x68k

# to create man8.${arch}/MAKEDEV.8 from ../../etc/etc.${arch}/MAKEDEV
# set ARCHS to desired architectures, or leave empty for all.
makedevs:
	cd ${.CURDIR} && sh MAKEDEV2manpage.sh ${ARCHS}

.include <bsd.man.mk>
.include <bsd.subdir.mk>
