#	$NetBSD: Makefile.inc,v 1.16.4.1 2000/07/19 17:25:42 thorpej Exp $

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
BOOTCODE=	${MDEC}/biosboot.sym
#BOOTCODE=	${MDEC}/biosboot_com0.sym	# Serial output
STRIP?=		strip

DISKTYPE?=	floppy3
DISKSIZE?=	2880
BLOCK8K?=	179

CLEANFILES+=	netbsd boot ustar.tmp

realall:	netbsd boot
	tar cvf ${FSTMP} boot netbsd
	@echo
	@echo Making disk number one
	dd if=/dev/zero of=${IMAGE1} count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE1}
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
		fi; \
		echo; \
                bytes=$$(($$dksize * $$disks - $$size));    \
                echo "There are $$bytes ($$(($$bytes / 1024))K) bytes free\
                        on disk $$disks."; \
	)

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
	-mkdir -p ${RELEASEDIR}/installation/floppies
	cp -p ${BASENAME}?.fs ${RELEASEDIR}/installation/floppies
	for f in ${RELEASEDIR}/installation/floppies/${BASENAME}?.fs; do \
		gzip <$$f >$$f.gz; \
	done
	
clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} ${BASENAME}?.fs

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
