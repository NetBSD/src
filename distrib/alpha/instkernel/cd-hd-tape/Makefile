#	$NetBSD: Makefile,v 1.11 2002/05/02 18:02:19 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	cdhdtape
FLOPPYSIZE=	5760
FLOPPYMETAFILE!=printf "tapelabel USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd
FLOPPYMAX=	1

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=\
	"${INSTALLBOOT} -m ${MACHINE} -t raw -v -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

release: check_RELEASEDIR .WAIT ${FLOPPYBASE}1.fs
	${RELEASE_INSTALL} ${FLOPPYBASE}1.fs \
	    ${RELEASEDIR}/installation/diskimage/cdhdtape

.include <bsd.prog.mk>
