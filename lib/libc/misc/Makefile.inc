#	$NetBSD: Makefile.inc,v 1.6 2026/10/04 03:28:44 mrg Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${ARCHDIR}/misc ${.PARSEDIR}

# other sources shared with the kernel
.if ${MKLIBCSANITIZER:Uno} == "yes"
SRCS+= ubsan.c
.endif

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
