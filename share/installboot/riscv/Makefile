#	$NetBSD: Makefile,v 1.1 2025/10/06 06:46:12 mrg Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		boards.plist
FILESDIR=	/usr/share/installboot/riscv
.endif

.include <bsd.prog.mk>
