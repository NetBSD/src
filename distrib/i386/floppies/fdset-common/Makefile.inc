#	$NetBSD: Makefile.inc,v 1.2 1999/02/01 14:25:09 he Exp $

# TOP is assumed to be defined by Makefile including this one.

COMMONDIR=	${TOP}/fdset-common

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}d
VND_CRDEV=	/dev/r${VND}d
BASENAME=	boot
IMAGE1=		${BASENAME}1.fs
FSTMP=		ustar.tmp
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec
BOOTCODE=	${MDEC}/biosboot.sym
STRIP?=		strip

RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

DISKTYPE?=	floppy3
DISKSIZE?=	2880
BLOCK8K?=	179

CLEANFILES+=	netbsd.ram.gz netbsd boot ustar.tmp

all:	netbsd boot
	@echo Making disk number one
	tar cvf ${FSTMP} boot netbsd
	@ echo ''
	@ls -l ${FSTMP} | (read mode links uid gid size junk;   \
		disks=$$((($$size / (${DISKSIZE} * 512 - 8 * 1024)) + 1)); \
                bytes=$$((${DISKSIZE} * 512 * $$disks - $$size));    \
                echo There are $$bytes \($$(($$bytes / 1024))K\) bytes free\
                        on disk $$disks.)
	@ echo ''
	dd if=/dev/zero of=${IMAGE1} count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE1}
	dd bs=8k seek=1 count=${BLOCK8K} if=${FSTMP} of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot -b 17 -v ${BOOTCODE} ${VND_RDEV}
	vnconfig -u ${VND_CDEV}
	:
	@ls -l ${FSTMP} | (read mode links uid gid size junk; \
		dksize=$$((${DISKSIZE} * 512 - 8 * 1024)); \
		if test $$size -gt $$dksize; then \
			disks=$$((($$size / (${DISKSIZE} * 512 - 8 * 1024)) \
				+ 1)); \
			d=2; \
			while test $$d -le $$disks; do \
				echo Making disk number $$d.; \
				IMAGE=${BASENAME}$${d}.fs; \
				PIMAGE=${BASENAME}$$(($$d - 1)).fs; \
				echo USTARFS $$d > $${IMAGE}; \
				(echo 'Prototype image created on: ';   \
					echo -n '     '; date;          \
					echo -n '     '; date -u)	\
					>> $${IMAGE}; \
				(echo Hash value of corresponding volume:; \
					md5 $${PIMAGE}) >> $${IMAGE}; \
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
		fi; )

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	: ${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

# Let the kernel on the diskette be called "netbsd" although
# it is compressed.  This is because the boot code will search
# for "netbsd" first, and calling it "netbsd" instead of "netbsd.gz" 
# we avoid scanning all the diskettes before reading the file.

netbsd: netbsd.ram.gz
	rm -f netbsd
	ln netbsd.ram.gz netbsd

# This is "cheating", just so that we have the available
# space at the start of the tar file.  Installboot will overwrite
# the start of the bootcode with the "real", aligned second-stage
# boot loader when it is run (which is quite a bit shorter than the
# entire ${BOOTCODE} file.

boot: ${BOOTCODE}
	rm -f boot
	cp ${BOOTCODE} boot

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} disk?.fs

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
