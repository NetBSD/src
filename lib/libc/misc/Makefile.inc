#	$NetBSD: Makefile.inc,v 1.3 2018/08/03 04:29:35 kamil Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${ARCHDIR}/misc ${.CURDIR}/misc

# other sources shared with the kernel
.if ${MKLIBCSANITIZER:Uno} == "yes"
SRCS+= ubsan.c
.endif

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
