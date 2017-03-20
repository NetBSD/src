#	$NetBSD: Makefile.inc,v 1.3.10.1 2017/03/20 06:57:25 pgoyette Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux32

.include <bsd.syscall.mk>
.endif
