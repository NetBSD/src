#	$NetBSD: Makefile.inc,v 1.7 2017/01/16 17:37:29 christos Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux

.include <bsd.syscall.mk>
.endif
