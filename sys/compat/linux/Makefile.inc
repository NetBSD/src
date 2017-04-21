#	$NetBSD: Makefile.inc,v 1.6.14.1 2017/04/21 16:53:41 bouyer Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux

.include <bsd.syscall.mk>
.endif
