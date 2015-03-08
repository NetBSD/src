#	$NetBSD: Makefile,v 1.6 2015/03/08 17:11:57 christos Exp $
# $FreeBSD: src/sys/modules/dtrace/Makefile,v 1.6.2.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

SUBDIR=		dtrace sdt fbt lockstat profile syscall
SUBDIR+=	linux_syscall

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=	netbsd32_syscall
SUBDIR+=	linux32_syscall
.endif

.include <bsd.subdir.mk>
