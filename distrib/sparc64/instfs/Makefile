#	$NetBSD: Makefile,v 1.7 2002/03/25 01:58:57 lukem Exp $
#
# instfs.tgz is the filesystem image for disk 2 of the floppy based
# installation method.
# It is constructed by tarring up the contents of the miniroot with
# the exeption of the kernel and boot program.
#

TOP=		${.CURDIR}/..
MINIROOT=	${.CURDIR}/../xminiroot

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>

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

release: check_RELEASEDIR .WAIT ${INSTFS}
	cp ${INSTFS} $(RELEASEDIR)/installation/misc
	gzip -9 -c ${MINIROOT_KERNEL} > $(RELEASEDIR)/binary/kernel/netbsd.INSTALL.gz

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
