#	$NetBSD: Makefile,v 1.11 1999/03/05 12:19:17 drochner Exp $

ARCHSUBDIR= ${MACHINE_ARCH:C/mipse[bl]/mips/}

.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif

.include <bsd.subdir.mk>
