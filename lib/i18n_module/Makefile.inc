#	$NetBSD: Makefile.inc,v 1.10 2008/10/26 07:06:07 mrg Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

SHLIB_VERSION_FILE=	${NETBSDSRCDIR}/lib/i18n_module/shlib_version
.if defined(LD32DIR)
LIBDIR=		/usr/lib/${LD32DIR}/i18n
SHLIBDIR=	/usr/lib/${LD32DIR}/i18n
SHLIBINSTALLDIR=/usr/lib/${LD32DIR}/i18n
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
