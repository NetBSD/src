#	$NetBSD: Makefile,v 1.10 2001/12/12 01:49:02 tv Exp $
.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

TEXINFO=	${G_TEXINFOS}
INFOFLAGS=	-I${DIST}/binutils

.PATH: ${DIST}/binutils

.include <bsd.info.mk>

.else
#
# XXX - BEGIN - Old toolchain build goop - delete when everything migrated - XXX
#
LIB=		common

SRCS=		arparse.y arlex.l arsup.c bucomm.c version.c filemode.c \
		rddbg.c debug.c stabs.c ieee.c rdcoff.c wrstabs.c
CPPFLAGS+=	-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"'
YHEADER=1

NOPROFILE=	# defined
NOPIC=		# defined
NOLINT=		# defined

# only needed during build
libinstall::

# mention explicit dependency, so arparse.h will appear timely
arlex.c: arparse.h

.include <bsd.lib.mk>
#
# XXX - END - Old toolchain build goop - delete when everything migrated - XXX
#
.endif
