#	$NetBSD: Makefile.inc,v 1.6.8.1 2014/05/22 11:41:15 yamt Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
