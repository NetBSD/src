#	$NetBSD: Makefile,v 1.3 2012/08/02 00:22:32 matt Exp $
#

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	bpf
# XXX: bpf_filter.c maybe needs to be its own module (or made
# inseparable from bpfilter?)
SRCS=	bpf.c bpf_filter.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.PATH:	${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}

.if (exists(${S}/arch/${MACHINE}/${MACHINE}/bpf_jit_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/bpf_jit_machdep.c)) || \
    (exists(${S}/arch/${MACHINE_CPU}/${MACHINE_CPU}/bpf_jit_machdep.c))
SRCS+=		bpf_jit.c
SRCS+=		bpf_jit_machdep.c
CPPFLAGS+=	-DBPF_JIT
.endif

.include <bsd.kmodule.mk>
