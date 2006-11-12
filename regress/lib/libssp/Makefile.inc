# $NetBSD: Makefile.inc,v 1.5 2006/11/12 01:41:46 christos Exp $

WARNS=	4

CPPFLAGS+=-I${DESTDIR}/usr/include/ssp -D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector

LDADD+=-lssp
DPADD+=${LIBSSP}

# Bootstrap hack

.ifmake !clean && !obj && !cleandir
.BEGIN:
	${AR} cr libssp_nonshared.a
CLEANFILES+= libssp_nonshared.a
LDFLAGS+=-L.
.endif
