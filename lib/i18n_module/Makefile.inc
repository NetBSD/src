#	$NetBSD: Makefile.inc,v 1.9 2006/11/22 23:38:27 tnozaki Exp $

NOLINT=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

SHLIB_VERSION_FILE=	${NETBSDSRCDIR}/lib/i18n_module/shlib_version
LIBDIR=		/usr/lib/i18n
SHLIBDIR=	/usr/lib/i18n
SHLIBINSTALLDIR=/usr/lib/i18n
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libc/citrus
CPPFLAGS+=	-DLOCALEMOD_MAJOR=${SHLIB_MAJOR}
CPPFLAGS+=	-DLIBC_SCCS
BASENAME=	${.CURDIR:T}
LIB=		${BASENAME}
SRCS?=		${SRCPRE:tl}${BASENAME:tl}.c

.PATH:	${NETBSDSRCDIR}/lib/libc/citrus ${NETBSDSRCDIR}/lib/libc/citrus/modules
