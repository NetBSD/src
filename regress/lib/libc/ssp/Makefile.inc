# $NetBSD: Makefile.inc,v 1.3.8.2 2007/09/03 07:02:28 wrstuden Exp $

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
