# $NetBSD: Makefile,v 1.2 2003/10/26 07:25:35 lukem Exp $
#
#  This test creates and mounts a temporary filesystem.  Then it
#  forcibly unmounts it while in the mount point and checks that
#  things work as expected.
#

.include <bsd.own.mk>

TMPMP=${.OBJDIR}/umount-f_mount
TMPIM=${.OBJDIR}/umount-f.im

VND?= vnd0
BVND= /dev/${VND}
CVND= /dev/r${VND}
MPART?= a

regress: mountfs run_script clean

clean:
	@echo "***  Cleaning up ${TMPMP}, ${TMPIM}."
	-umount -f "${TMPMP}"
	-vnconfig -u "${VND}"
	-rm -rf "${TMPMP}" "${TMPIM}"

mountfs:
	@mkdir ${TMPMP}
	@touch ${TMPMP}/under_the_mount
	@echo "***  Creating a dummy directory tree at ${TMPMP} monted on" \
	    "${TMPIM}"
	@dd if=/dev/zero of=${TMPIM} count=5860
	@vnconfig -v ${VND} ${TMPIM}
	@disklabel -f ${.CURDIR}/disktab -rw ${VND} floppy288
	@newfs -i 500 -b 8192 -f 1024 ${CVND}${MPART}
	@mount -o async ${BVND}${MPART} ${TMPMP}
	@touch ${TMPMP}/in_mounted_directory


run_script:
	@echo "***  Testing forced unmount"
	@if [ ! -e "${TMPMP}/in_mounted_directory" ] ; then \
		echo "Test file not present in mounted directory!" ;\
		false ;\
	fi
	@cd "${TMPMP}" && umount -f "${BVND}${MPART}" && \
	 ${HOST_SH} ${.CURDIR}/check_umount.sh
	@if [ ! -e "${TMPMP}/under_the_mount" ]; then \
		echo "Original mount point dissapeared!" ;\
		false ;\
	fi
	@echo "***  SUCCESS"

.include <bsd.obj.mk>
