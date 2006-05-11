# $NetBSD: Makefile,v 1.28 2006/05/11 23:16:30 mrg Exp $
# Original from FreeBSD, no rcs id.

MKPRIVATELIB=	yes

LIB=		install
SRCS=		automatic.c file.c ftpio.c global.c lpkg.c pen.c \
		pkgdb.c plist.c str.c version.c path.c fexec.c var.c \
		dewey.c pexec.c
MAN=		pkg_summary.5

version.o: version.h version.c

.include <bsd.lib.mk>

.if ${HAVE_GCC} == 4
.for f in file ftpio plist
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor
.endif
