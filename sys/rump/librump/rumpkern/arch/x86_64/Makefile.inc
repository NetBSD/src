#	$NetBSD: Makefile.inc,v 1.7.8.1 2014/05/22 11:41:16 yamt Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
