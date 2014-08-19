# $NetBSD: Makefile.inc,v 1.6.4.2 2014/08/20 00:02:18 tls Exp $

COMPILER_RT_SRCDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

.if ${MACHINE_ARCH} == "powerpc"
COMPILER_RT_CPU_DIR=	${COMPILER_RT_SRCDIR}/lib/builtins/ppc
COMPILER_RT_ARCH_DIR=	${COMPILER_RT_SRCDIR}/lib/builtins/ppc
.else
COMPILER_RT_CPU_DIR=	${COMPILER_RT_SRCDIR}/lib/builtins/${MACHINE_CPU}
COMPILER_RT_ARCH_DIR=	${COMPILER_RT_SRCDIR}/lib/builtins/${MACHINE_ARCH}
.endif

.PATH: ${COMPILER_RT_CPU_DIR}
.PATH: ${COMPILER_RT_ARCH_DIR}
.PATH: ${COMPILER_RT_SRCDIR}/lib/builtins

# Needs scaling support
GENERIC_SRCS+= \
	mulxc3.c \
	mulsc3.c \
	divxc3.c \
	divdc3.c \
	divsc3.c

.for src in ${GENERIC_SRCS}
.  if exists(${COMPILER_RT_CPU_DIR}/${src:R}.S) || \
      exists(${COMPILER_RT_ARCH_DIR}/${src:R}.S)
SRCS+=	${src:R}.S
.  else
SRCS+=	${src}
COPTS.${src}+=	-Wno-error=missing-prototypes
.  endif
.endfor
