#	$NetBSD: Makefile.inc,v 1.4 2017/01/16 17:39:59 christos Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux32

.include <bsd.syscall.mk>
.endif
