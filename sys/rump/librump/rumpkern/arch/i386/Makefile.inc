#	$NetBSD: Makefile.inc,v 1.6.18.1 2014/08/20 00:04:41 tls Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/i386/i386
SRCS+=	kobj_machdep.c
