#	$NetBSD: Makefile,v 1.11.12.1 2024/07/03 18:38:53 martin Exp $

NOMAN=	# defined
PROG=	macppc_installboot
SRCS=	installboot.c cd9660.c cd9660_util.c

.PATH.c: ${.CURDIR}
.PATH.c: ${NETBSDSRCDIR}/usr.sbin/installboot ${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=	-I. -I${.CURDIR}
#CPPFLAGS+=	-DDEBUG

.if !defined(HOSTPROGNAME)
.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	-rm -rf fs
	${HOST_INSTALL_DIR} fs
	${HOST_INSTALL_DIR} fs/cd9660
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/unicode.h fs
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/cd9660/iso.h fs/cd9660
	${HOST_LN} -s ${NETBSDSRCDIR}/sys/fs/cd9660/cd9660_extern.h fs/cd9660
.endif

cleandir distclean: cleaninc

cleaninc:
	-rm -rf fs
.endif	# !defined(HOSTPROGNAME)

.include <bsd.prog.mk>
