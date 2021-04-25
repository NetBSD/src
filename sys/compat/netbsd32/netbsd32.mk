#	$NetBSD: netbsd32.mk,v 1.3 2021/04/25 22:27:08 christos Exp $

# makefile fragment that tells you if you should support netbsd32 or not.
# include this and check ${COMPAT_USE_NETBSD32} != "no".

.if ${MACHINE_ARCH} == "x86_64" \
    || ${MACHINE_CPU} == "arm" \
    || ${MACHINE_CPU} == "aarch64" \
    || ${MACHINE_ARCH} == "sparc64" \
    || (${MACHINE_MIPS64} && !defined(BSD_MK_COMPAT_FILE))
COMPAT_USE_NETBSD32?=yes
.else
COMPAT_USE_NETBSD32?=no
.endif
