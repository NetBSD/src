#	$NetBSD: Makefile,v 1.28.2.1 2000/06/22 16:17:52 minoura Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.depall.mk bsd.files.mk \
	bsd.hostprog.mk bsd.inc.mk bsd.info.mk bsd.kernobj.mk bsd.kinc.mk \
	bsd.kmod.mk bsd.lib.mk bsd.links.mk bsd.man.mk bsd.nls.mk \
	bsd.obj.mk bsd.own.mk bsd.prog.mk bsd.subdir.mk bsd.sys.mk sys.mk

FILESDIR=/usr/share/mk
.endif
MKOBJ=	no

FILESMODE=444

.include <bsd.prog.mk>
