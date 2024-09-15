#	$NetBSD: Makefile,v 1.6 2024/09/15 04:05:32 tsutsui Exp $

# XXX
# This installboot(8) tool for macppc is prepared to handle quirks on
# creating bootable ISO using mkhybrid(8) for OpenFirmware 1.x and 2.x
# machines that require "bootable partition" info the Apple Partition map
# entry even on HFS/ISO9660 hybrid images.
#
# This should be merge into MI installboot(8) once after we have proper
# defintitions how Apple Partition Map should be handled on creating
# a new label on fresh disks in MD macppc/disksubr.c etc.

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
