# $NetBSD: Makefile.inc,v 1.2 2006/11/09 17:08:04 christos Exp $

WARNS=	4

CPPFLAGS+=-I${DESTDIR}/usr/include/ssp -D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector

LDADD+=-lssp
DPADD+=${LIBSSP}
