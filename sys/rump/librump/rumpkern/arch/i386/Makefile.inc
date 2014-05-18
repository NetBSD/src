#	$NetBSD: Makefile.inc,v 1.6.22.1 2014/05/18 17:46:18 rmind Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
