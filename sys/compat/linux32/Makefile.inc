#	$NetBSD: Makefile.inc,v 1.3.6.1 2017/02/05 13:40:25 skrll Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux32

.include <bsd.syscall.mk>
.endif
