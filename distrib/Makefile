#	$NetBSD: Makefile,v 1.11.4.1 2005/07/24 02:10:49 snj Exp $

SUBDIR=	notes .WAIT

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.if make(obj)
SUBDIR+= utils
.endif

TARGETS+=release

.include <bsd.subdir.mk>
