#	$NetBSD: Makefile.inc,v 1.21 2000/10/31 21:18:36 pk Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

TOP= $(.CURDIR)/..

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= GENERIC_SCSI3
BUILD_KERNELS+= INSTALL

# Add sparc installation directories
MD_INSTALLATION_DIRS= \
	installation/miniroot installation/netboot installation/bootfs \
	installation/tape installation/floppy
INSTALLATION_DIRS+=$(MD_INSTALLATION_DIRS) 

snap_pre_md:
.ifndef UPDATE
	cd $(TOP)/distrib && ${MAKE} clean 
.endif

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
	# make boot/miniroot images
	(cd $(TOP)/distrib; ${MAKE})

	# transfer images to release hierarchy
.for dir in notes miniroot sparc/bootfs sparc/instfs
	(cd $(TOP)/distrib/$(dir); ${MAKE} release)
.endfor

	# install boot image in netboot directory
	cp -p $(DESTDIR)/usr/mdec/boot.net $(RELEASEDIR)/installation/netboot

	# install links to boot and installation utilities for various media
	rm -f ${RELEASEDIR}/installation/tape/tapefile1.gz
	rm -f ${RELEASEDIR}/installation/tape/tapefile2
	rm -f ${RELEASEDIR}/installation/floppy/disk1.gz
	rm -f ${RELEASEDIR}/installation/floppy/disk2
	rm -f ${RELEASEDIR}/installation/netboot/rootfs.tgz
	ln -s ../bootfs/netbsd.ram.aout.gz \
			${RELEASEDIR}/installation/tape/tapefile1.gz
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/tape/tapefile2
	ln -s ../bootfs/boot.fs.gz \
			${RELEASEDIR}/installation/floppy/disk1.gz
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/floppy/disk2
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/netboot/rootfs.tgz

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	sh $(TOP)/distrib/sets/makesums -t ${RELEASEDIR}/${dir} '*'
.endfor

CDBOOTIMAGE!=	cd ${TOP}/distrib/sparc/bootfs; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/boot.fs\n" | \
		${MAKE} -s -f-

iso-image_md_post:
	mksunbootcd ${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		$(CDBOOTIMAGE) $(CDBOOTIMAGE) $(CDBOOTIMAGE)
