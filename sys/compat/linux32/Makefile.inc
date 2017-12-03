#	$NetBSD: Makefile.inc,v 1.2.80.2 2017/12/03 11:36:55 jdolecek Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux32

.include <bsd.syscall.mk>
.endif
