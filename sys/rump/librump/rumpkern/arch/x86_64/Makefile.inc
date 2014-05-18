#	$NetBSD: Makefile.inc,v 1.7.22.1 2014/05/18 17:46:19 rmind Exp $
#

.include "${RUMPTOP}/librump/rumpkern/arch/x86/Makefile.inc"

.PATH:	${RUMPTOP}/../arch/amd64/amd64
SRCS+=	kobj_machdep.c
