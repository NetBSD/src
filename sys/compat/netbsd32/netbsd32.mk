#	$NetBSD: netbsd32.mk,v 1.1 2019/02/04 21:57:47 mrg Exp $

# makefile fragment that tells you if you should support netbsd32 or not.
# include this and check ${COMPAT_USE_NETBSD32} != "no".

.if ${MACHINE_ARCH} == "x86_64" \
    || ${MACHINE_CPU} == "arm" \
    || ${MACHINE_ARCH} == "sparc64" \
    || (!empty(MACHINE_ARCH:Mmips64*) && !defined(BSD_MK_COMPAT_FILE))
COMPAT_USE_NETBSD32?=yes
.else
COMPAT_USE_NETBSD32?=no
.endif
