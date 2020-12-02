# $NetBSD: Makefile,v 1.11 2020/12/02 13:54:25 wiz Exp $
# Original from FreeBSD, no rcs id.

LIBISPRIVATE=	yes

LIB=		install
SRCS=		automatic.c conflicts.c dewey.c \
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
CPPFLAGS+=	-DPREFIX=\"/usr/pkg\"

COPTS.plist.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-error=restrict :}

.include <bsd.lib.mk>
