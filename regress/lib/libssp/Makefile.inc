# $NetBSD: Makefile.inc,v 1.4 2006/11/11 11:23:39 he Exp $

WARNS=	4

CPPFLAGS+=-I${DESTDIR}/usr/include/ssp -D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector

LDADD+=-lssp
DPADD+=${LIBSSP}

# Bootstrap hack

# XXX This conditional does not appear to do the right thing(!)
# .ifmake all || dependall
.BEGIN:
	${AR} cr libssp_nonshared.a
CLEANFILES+= libssp_nonshared.a
LDFLAGS+=-L.
# .endif
