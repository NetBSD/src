#	$NetBSD: Makefile,v 1.24 1999/02/13 02:54:36 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.files.mk bsd.inc.mk bsd.info.mk \
	bsd.kinc.mk bsd.kmod.mk bsd.lib.mk bsd.links.mk bsd.man.mk bsd.nls.mk \
	bsd.obj.mk bsd.own.mk bsd.prog.mk bsd.subdir.mk bsd.sys.mk sys.mk
FILESDIR=/usr/share/mk
.endif
MKOBJ=	no

FILESMODE=444

.include <bsd.prog.mk>
