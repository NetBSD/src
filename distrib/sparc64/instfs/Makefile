#	$NetBSD: Makefile,v 1.8 2002/05/02 18:02:55 lukem Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../xminiroot

.include "${TOP}/Makefile.inc"

MINIROOT_DIR!=	cd ${MINIROOT} && ${PRINTOBJDIR}
MINIROOT_IMAGE=	${MINIROOT_DIR}/miniroot.fs
MINIROOT_KERNEL=${MINIROOT_DIR}/netbsd.INSTALL

MOUNT_POINT?=	${MINIROOT_DIR}/mountpoint
INSTFS?=	instfs.tgz

all: ${INSTFS}

${INSTFS}: ${MINIROOT_IMAGE}
	(cd ${MOUNT_POINT}; \
		tar -z -X ${.CURDIR}/exclude -c -f ${.OBJDIR}/${.TARGET} .)

clean cleandir distclean:
	/bin/rm -f *.core ${INSTFS}

release: check_RELEASEDIR .WAIT ${INSTFS} ${MINIROOT_KERNEL}
	${RELEASE_INSTALL} ${INSTFS} $(RELEASEDIR)/installation/misc
	gzip -9 -c ${MINIROOT_KERNEL} > $(RELEASEDIR)/binary/kernel/netbsd.INSTALL.gz

.include <bsd.prog.mk>
