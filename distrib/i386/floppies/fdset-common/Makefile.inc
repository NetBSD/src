#	$NetBSD: Makefile.inc,v 1.26 2001/10/04 04:11:26 jmc Exp $

# TOP is assumed to be defined by Makefile including this one.

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
BASENAME?=	boot
IMAGE1?=	${BASENAME}1.fs
FSTMP=		ustar.tmp
MDEC=		${DESTDIR}/usr/mdec
BOOTCODE?=	${MDEC}/biosboot.sym
#BOOTCODE=	${MDEC}/biosboot_com0.sym	# Serial output
STRIP?=		strip
MOUNT_POINT?=	/mnt

DISKTYPE?=	floppy3
DISKSIZE?=	2880
BLOCK8K?=	179

CLEANFILES+=	netbsd boot ustar.tmp

realall:	unconfig netbsd boot
	tar cvf ${FSTMP} boot netbsd
	@echo
	@echo Making disk number one
	-mv -f ${IMAGE1} ${IMAGE1}.tmp
	dd if=/dev/zero of=${IMAGE1}.tmp count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE1}.tmp
	dd bs=8k seek=1 count=${BLOCK8K} if=${FSTMP} of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v -f ${BOOTCODE} ${VND_RDEV}
	vnconfig -u ${VND_CDEV}
	@ls -l ${FSTMP} | (read mode links uid gid size junk; \
		dksize=$$((${DISKSIZE} * 512 - 8 * 1024)); \
		disks=$$(($$size / $$dksize + 1)); \
		if test $$size -gt $$dksize; then \
			d=2; \
			while test $$d -le $$disks; do \
				echo; \
				echo Making disk number $$d.; \
				IMAGE=${BASENAME}$${d}.fs; \
				echo USTARFS $$d > $${IMAGE}; \
				skip=$$((($$d - 1) * ${BLOCK8K})); \
				if test $$d -eq $$disks; then \
					dd bs=8k seek=1 skip=$${skip} \
						conv=sync \
						if=${FSTMP} of=$${IMAGE}; \
				else \
					dd bs=8k seek=1 skip=$${skip} \
						conv=sync count=${BLOCK8K} \
						if=${FSTMP} of=$${IMAGE}; \
				fi; \
				d=$$(($$d + 1)); \
			done; \
		else \
			dd seek=$$(($${size} / 512 + 15)) count=1 \
				if=/dev/zero of=${IMAGE1}; \
		fi; \
		echo; \
                bytes=$$(($$dksize * $$disks - $$size));    \
                echo "There are $$bytes ($$(($$bytes / 1024))K) bytes free\
                        on disk $$disks."; \
	)
	mv -f ${IMAGE1}.tmp ${IMAGE1}

# Let the kernel on the diskette be called "netbsd" although
# it is compressed.  This is because the boot code will search
# for "netbsd" first, and calling it "netbsd" instead of "netbsd.gz" 
# we avoid scanning all the diskettes before reading the file.

netbsd: ${KERN}
	rm -f netbsd
	cp ${KERN} netbsd

# This is "cheating", just so that we have the available
# space at the start of the tar file.  Installboot will overwrite
# the start of the bootcode with the tail end of the "real", aligned 
# second-stage boot loader when it is run (which is quite a bit shorter
# than the entire ${BOOTCODE} file, even when symbols are removed.
# (the first 15*512 bytes are put into the first 8K filesystem block,
# around the disklabel)

boot: ${BOOTCODE}
	rm -f boot
	cp ${BOOTCODE} boot
	strip boot

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${BASENAME}?.fs ${RELEASEDIR}/installation/floppy
	for f in ${RELEASEDIR}/installation/floppy/${BASENAME}?.fs; do \
		gzip <$$f >$$f.gz; \
	done
	
unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} ${BASENAME}?.fs ${IMAGE1}.tmp

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>

