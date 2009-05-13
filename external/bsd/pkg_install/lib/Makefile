# $NetBSD: Makefile,v 1.2.2.1 2009/05/13 18:52:38 jym Exp $
# Original from FreeBSD, no rcs id.

LIBISPRIVATE=	yes

LIB=		install
SRCS=		automatic.c conflicts.c decompress.c dewey.c \
		fexec.c file.c global.c gpgsig.c iterate.c license.c lpkg.c \
		opattern.c parse-config.c pkcs7.c pkg_signature.c \
		pkgdb.c pkg_io.c plist.c remove.c \
		str.c version.c var.c vulnerabilities-file.c xwrapper.c
MAN=		pkg_install.conf.5 pkg_summary.5

version.o: version.h version.c

.include <bsd.init.mk>

pkg_install.conf.5: ${DIST}/lib/pkg_install.conf.5.in
	${TOOL_SED} -e 's,@SYSCONFDIR@,/etc,' \
	    ${DIST}/lib/pkg_install.conf.5.in > ${.TARGET}

.PATH:	${DIST}/lib

CLEANFILES+=	pkg_install.conf.5

CPPFLAGS+=	-DNETBSD
COPTS.plist.c+=	-Wno-pointer-sign

.include <bsd.lib.mk>
