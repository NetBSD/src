#	$NetBSD: Makefile,v 1.10 1998/02/18 03:55:05 perry Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.PATH: ${.CURDIR}/../libkvm

LIB=	kvm
CPPFLAGS+=-DLIBC_SCCS
CPPFLAGS+=-I- -I${.CURDIR}

# Try most specific name first.
.if	exists(kvm_${MACHINE}.c)
SRCS=	kvm_${MACHINE}.c
.else
# Less specific name (i.e. m68k)
CPPFLAGS+=-D${MACHINE}
SRCS=	kvm_${MACHINE_ARCH}.c
.endif

SRCS+=	kvm.c kvm_file.c kvm_getloadavg.c kvm_proc.c
NOMAN=	yes

.include <bsd.lib.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=	-DUVM
.endif
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
