#	$NetBSD: Makefile,v 1.7 1997/10/09 14:36:21 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/../libkvm

LIB=	kvm
CFLAGS+=-DLIBC_SCCS
CFLAGS+=-I- -I${.CURDIR}

# Try most specific name first.
.if	exists(kvm_${MACHINE}.c)
SRCS=	kvm_${MACHINE}.c
.else
# Less specific name (i.e. m68k)
CFLAGS+=-D${MACHINE}
SRCS=	kvm_${MACHINE_ARCH}.c
.endif

SRCS+=	kvm.c kvm_file.c kvm_getloadavg.c kvm_proc.c
NOMAN=	yes

.include <bsd.lib.mk>
