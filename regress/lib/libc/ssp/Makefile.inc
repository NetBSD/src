# $NetBSD: Makefile.inc,v 1.1 2007/05/31 21:51:49 christos Exp $

WARNS=	4

CPPFLAGS+=-D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector
LDFLAGS+=-fstack-protector-all -Wstack-protector

# Bootstrap hack

.ifmake !clean && !obj && !cleandir
.BEGIN:
	${AR} cr libssp_nonshared.a
.endif
CLEANFILES+= libssp_nonshared.a
LDFLAGS+=-L.
