#	$NetBSD: Makefile.inc,v 1.4 2020/04/22 23:32:25 joerg Exp $
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

# Ownership of globals from crt0.o is shared with libc for historic reasons
COPTS.initfini.c+=		-fcommon
