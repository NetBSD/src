#	$NetBSD: Makefile.inc,v 1.6.10.1 2017/03/20 06:57:24 pgoyette Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux

.include <bsd.syscall.mk>
.endif
