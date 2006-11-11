# $NetBSD: Makefile.inc,v 1.3 2006/11/11 06:42:49 christos Exp $

WARNS=	4

CPPFLAGS+=-I${DESTDIR}/usr/include/ssp -D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector

LDADD+=-lssp
DPADD+=${LIBSSP}

# Bootstrap hack

.ifmake all || dependall
.BEGIN:
	${AR} cr libssp-nonshared.a
CLEANFILES+= libssp-nonshared.a
LDFLAGS+=-L.
.endif
