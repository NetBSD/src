#	$NetBSD: Makefile,v 1.8.10.1 2021/04/17 17:26:20 thorpej Exp $
# $FreeBSD: src/sys/modules/dtrace/Makefile,v 1.6.2.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

SUBDIR=		dtrace sdt fbt lockstat profile syscall

.if ${MACHINE_ARCH} == "x86_64" || !empty(MACHINE_ARCH:Mmips64*)
SUBDIR+=	netbsd32_syscall
.endif

.if ${MACHINE_ARCH} != "aarch64" && ${MACHINE_CPU} != "mips"
SUBDIR+=	linux_syscall
.endif

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	linux32_syscall
.endif

.include <bsd.subdir.mk>
