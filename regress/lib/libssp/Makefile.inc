# $NetBSD: Makefile.inc,v 1.6 2006/11/12 15:18:29 he Exp $

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
.endif
CLEANFILES+= libssp_nonshared.a
LDFLAGS+=-L.
