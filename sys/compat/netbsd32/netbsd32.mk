#	$NetBSD: netbsd32.mk,v 1.1.4.2 2019/06/10 22:07:01 christos Exp $

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
