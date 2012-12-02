#	$NetBSD: Makefile,v 1.9 2012/12/02 12:55:27 apb Exp $

HOSTPROG=	macppc_installboot
SRCS=		installboot.c cd9660.c cd9660_util.c
NOMAN=	# defined

HOST_CPPFLAGS+=	-I. -I${.CURDIR}
#HOST_CPPFLAGS+=	-DDEBUG

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	-rm -rf fs
	${HOST_INSTALL_DIR} fs
	${HOST_INSTALL_DIR} fs/cd9660
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/unicode.h fs
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/cd9660/iso.h fs/cd9660
.endif

cleandir distclean: cleaninc

cleaninc:
	-rm -rf fs

.include <bsd.own.mk>

.ifndef NOCOMPATLIB
.-include	"${TOOLDIR}/share/compat/defs.mk"
.endif

.include <bsd.hostprog.mk>
