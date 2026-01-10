#	$NetBSD: Makefile,v 1.2 2026/01/10 22:05:08 jmcneill Exp $

.if ${MACHINE} == "i386" || \
    ${MACHINE} == "amd64" || \
    ${MACHINE} == "evbarm" || \
    ${MACHINE} == "evbppc"
SUBDIR=	modesetting
.endif

.include <bsd.subdir.mk>
