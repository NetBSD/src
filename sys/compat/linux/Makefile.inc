#	$NetBSD: Makefile.inc,v 1.6.6.1 2017/02/05 13:40:24 skrll Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux

.include <bsd.syscall.mk>
.endif
