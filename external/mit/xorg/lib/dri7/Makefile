# $NetBSD: Makefile,v 1.1.2.2 2015/03/03 08:28:12 snj Exp $

# Build MesaLib 7.x r300 driver, which requires LLVM in Mesa 10.x

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR=		libmesa .WAIT r300
.endif

.include <bsd.subdir.mk>
