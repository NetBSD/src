# $NetBSD: Makefile,v 1.1 1999/08/16 06:42:48 bgrayson Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/mount_portal

FILES= overview advanced.1 advanced.1.conf fing.c rfilter.1 \
	rfilter.2 rfilter.2.conf tcp.1 tcp.1.conf fs.1 fs.1.conf
.endif

.include <bsd.prog.mk>
.include <bsd.files.mk>
