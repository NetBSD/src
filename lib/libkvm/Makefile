#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$Id: Makefile,v 1.6.2.1 1994/07/29 05:10:36 cgd Exp $

LIB=	kvm
CFLAGS+=-DLIBC_SCCS

# Try most specific name first.
.if	exists(kvm_${MACHINE}.c)
SRCS=	kvm_${MACHINE}.c
.else
# Less specific name (i.e. m68k)
CFLAGS+=-D${MACHINE}
# XXX - Should merge m68k versions...
.if	(${MACHINE_ARCH} == "m68k")
SRCS=	kvm_o${MACHINE_ARCH}.c
.else
SRCS=	kvm_${MACHINE_ARCH}.c
.endif
.endif

SRCS+=	kvm.c kvm_file.c kvm_getloadavg.c kvm_proc.c

MAN3=	kvm.0 kvm_geterr.0 kvm_getfiles.0 kvm_getloadavg.0 kvm_getprocs.0 \
	kvm_nlist.0 kvm_open.0 kvm_read.0

MLINKS+=kvm_getprocs.3 kvm_getargv.3 kvm_getprocs.3 kvm_getenvv.3
MLINKS+=kvm_open.3 kvm_openfiles.3 kvm_open.3 kvm_close.3
MLINKS+=kvm_read.3 kvm_write.3

.include <bsd.lib.mk>
