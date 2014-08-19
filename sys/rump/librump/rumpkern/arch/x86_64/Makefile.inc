#	$NetBSD: Makefile.inc,v 1.7.18.1 2014/08/20 00:04:42 tls Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
