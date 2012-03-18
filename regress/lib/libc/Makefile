#	$NetBSD: Makefile,v 1.81 2012/03/18 07:54:00 jruoho Exp $

SUBDIR+= citrus divrem

.include <bsd.own.mk>
.include <bsd.sys.mk>

.if exists(arch/${MACHINE_ARCH})
SUBDIR+= arch/${MACHINE_ARCH}
.endif

.include <bsd.subdir.mk>
