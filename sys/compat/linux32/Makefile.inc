#	$NetBSD: Makefile.inc,v 1.3.14.1 2017/04/21 16:53:42 bouyer Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux32

.include <bsd.syscall.mk>
.endif
