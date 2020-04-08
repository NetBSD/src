#	$NetBSD: Makefile,v 1.6.18.1 2020/04/08 14:08:55 martin Exp $
# $FreeBSD: src/sys/modules/dtrace/Makefile,v 1.6.2.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

SUBDIR=		dtrace sdt fbt lockstat profile syscall

.if ${MACHINE_ARCH} != "aarch64"
SUBDIR+=	linux_syscall
.endif

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	netbsd32_syscall
SUBDIR+=	linux32_syscall
.endif

.include <bsd.subdir.mk>
