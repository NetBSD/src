#	$NetBSD: Makefile.inc,v 1.5.120.2 2017/12/03 11:36:53 jdolecek Exp $

.if !defined(SUBDIR)
SYSCALL_PREFIX=linux

.include <bsd.syscall.mk>
.endif
