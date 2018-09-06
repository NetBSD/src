#	$NetBSD: Makefile.inc,v 1.2.28.1 2018/09/06 06:55:19 pgoyette Exp $
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
