# $NetBSD: Makefile.inc,v 1.11 2021/06/16 05:21:08 rin Exp $

COMPILER_RT_DIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt
COMPILER_RT_SRCDIR=	${COMPILER_RT_DIR}/dist

.if ${MACHINE_CPU} == "powerpc"
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
	muldc3.c \
	mulsc3.c \
	mulxc3.c \
	divdc3.c \
	divsc3.c \
	divxc3.c

.if ${MACHINE_CPU} == "powerpc" || ${LIBC_MACHINE_CPU:U} == "sparc64" || ${LIBC_MACHINE_CPU:U} == "aarch64"
GENERIC_SRCS+= \
	divtc3.c \
	multc3.c
.endif

.for src in ${GENERIC_SRCS}
.  if exists(${COMPILER_RT_CPU_DIR}/${src:R}.S) || \
      exists(${COMPILER_RT_ARCH_DIR}/${src:R}.S)
SRCS+=	${src:R}.S
.  else
SRCS+=	${src}
COPTS.${src}+=	-Wno-error=missing-prototypes
.  endif
.endfor

.include "${COMPILER_RT_DIR}/abi.mk"
