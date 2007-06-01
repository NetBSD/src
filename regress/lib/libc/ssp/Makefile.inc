# $NetBSD: Makefile.inc,v 1.2 2007/06/01 16:54:11 martin Exp $

WARNS=	4

CPPFLAGS+=-D_FORTIFY_SOURCE=2
CFLAGS+=-fstack-protector-all -Wstack-protector

# Bootstrap hack

.ifmake !clean && !obj && !cleandir
.BEGIN:
	${AR} cr libssp_nonshared.a
.endif
CLEANFILES+= libssp_nonshared.a
LDFLAGS+=-L.
