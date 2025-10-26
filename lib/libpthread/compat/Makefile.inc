# $NetBSD: Makefile.inc,v 1.1 2025/10/26 20:48:28 christos Exp $

COMPAT:= ${.PARSEDIR}
.PATH.c: ${COMPAT}

SRCS +=	compat_pthread_setname_np.c
CPPFLAGS.compat_pthread_setname_np.c += -I${COMPAT}
COPTS.compat_pthread_setname_np.c += -Wno-format-nonliteral
