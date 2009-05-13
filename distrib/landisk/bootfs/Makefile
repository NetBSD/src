#	$NetBSD: Makefile,v 1.1.10.1 2009/05/13 19:17:43 jym Exp $
#
# boot.fs is the image for wd booting.
#
# It is constructed by packaging an INSTALL kernel into an ustarfs.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	boot
FLOPPYSUFFIX=	.fs

FLOPPYMAX=1
FLOPPYSIZE=	8192
FLOPPYMETAFILE!=	printf "USTAR.volsize.%o" ${FLOPPYSIZE}

FLOPPYKERNEL=	netbsd-INSTALL.gz
FLOPPYKERNDIR=	instkernel
FLOPPYKERNOBJ!=	cd ${.CURDIR}/../${FLOPPYKERNDIR} && ${PRINTOBJDIR}

MDEC=		${DESTDIR}/usr/mdec
FLOPPYINSTBOOT=	"${TOOL_INSTALLBOOT} -m ${MACHINE} @IMAGE@ ${MDEC}/bootxx_ustarfs"

FLOPPY_BOOT=	${MDEC}/boot
FLOPPY_NETBSD=	${FLOPPYKERNOBJ}/${FLOPPYKERNEL}
FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd

FLOPPY_RELEASEDIR=	installation/misc

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
