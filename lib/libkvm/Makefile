#	$NetBSD: Makefile,v 1.17 1997/06/23 22:16:56 veego Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

.if	(${MACHINE} != "arm32")  && \
	(${MACHINE} != "i386")   && \
	(${MACHINE} != "pica")   && \
	(${MACHINE} != "pmax")   && \
	(${MACHINE} != "x68k")
LIB=	kvm
CFLAGS+=-DLIBC_SCCS

SRCS=	kvm.c kvm_file.c kvm_getloadavg.c kvm_proc.c

# This library should compile to the same thing on all variants of
# ${MACHINE} with the same ${MACHINE_ARCH} so shared /usr works.
# If it is necessary to run different code based on ${MACHINE} then
# the dispatch on ${MACHINE} should be done at run time (see m68k).

SRCS+=	kvm_${MACHINE_ARCH}.c

# Additional modules needed for m68k
.if (${MACHINE_ARCH} == "m68k")
SRCS+=	kvm_m68k_cmn.c kvm_sun3.c kvm_sun3x.c
.endif
.endif

MAN=	kvm.3 kvm_dump.3 kvm_geterr.3 kvm_getfiles.3 kvm_getloadavg.3 \
	kvm_getprocs.3 kvm_nlist.3 kvm_open.3 kvm_read.3

MLINKS+=kvm_getprocs.3 kvm_getargv.3 kvm_getprocs.3 kvm_getenvv.3
MLINKS+=kvm_open.3 kvm_openfiles.3 kvm_open.3 kvm_close.3
MLINKS+=kvm_read.3 kvm_write.3
MLINKS+=kvm_dump.3 kvm_dump_mkheader.3 kvm_dump.3 kvm_dump_wrtheader.3
MLINKS+=kvm_dump.3 kvm_dump_inval.3

.if	(${MACHINE} != "arm32")  && \
	(${MACHINE} != "i386")   && \
	(${MACHINE} != "pica")   && \
	(${MACHINE} != "pmax")   && \
	(${MACHINE} != "x68k")
.include <bsd.lib.mk>
.else
.include <bsd.prog.mk>
.endif
