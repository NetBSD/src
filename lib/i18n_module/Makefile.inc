#	$NetBSD: Makefile.inc,v 1.15 2026/06/30 23:17:48 riastradh Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

NOCLANGERROR=	# defined

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
CPPFLAGS+=	-D_DIAGNOSTIC
BASENAME=	${.CURDIR:T}
LIB=		${BASENAME}
SRCS?=		${SRCPRE:tl}${BASENAME:tl}.c

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus ${NETBSDSRCDIR}/lib/libc/citrus/modules
