#	$NetBSD: Makefile.inc,v 1.10.8.1 2009/12/14 06:29:20 mrg Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

SHLIB_VERSION_FILE=	${NETBSDSRCDIR}/lib/i18n_module/shlib_version
.if defined(MLIBDIR)
LIBDIR=		/usr/lib/${MLIBDIR}/i18n
SHLIBDIR=	/usr/lib/${MLIBDIR}/i18n
SHLIBINSTALLDIR=/usr/lib/${MLIBDIR}/i18n
.else
LIBDIR=		/usr/lib/i18n
SHLIBDIR=	/usr/lib/i18n
SHLIBINSTALLDIR=/usr/lib/i18n
.endif
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus
CPPFLAGS+=	-DLOCALEMOD_MAJOR=${SHLIB_MAJOR}
CPPFLAGS+=	-DLIBC_SCCS
BASENAME=	${.CURDIR:T}
LIB=		${BASENAME}
SRCS?=		${SRCPRE:tl}${BASENAME:tl}.c

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus ${NETBSDSRCDIR}/lib/libc/citrus/modules
