#	$NetBSD: Makefile.inc,v 1.17 2000/09/28 12:46:43 abs Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

TOP= $(.CURDIR)/..

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= GENERIC_SCSI3
BUILD_KERNELS+= INSTALL INSTALL_SYSINST

# Add sparc installation directories
MD_INSTALLATION_DIRS= \
	installation/miniroot installation/netboot installation/bootfs
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
.for dir in notes miniroot sparc/ramdisk sparc/bootfs sparc/ramdisk.sysinst sparc/bootfs.sysinst
	(cd $(TOP)/distrib/$(dir); ${MAKE} release)
.endfor

	# install boot image and installation scripts in netboot directory
	cp -p $(DESTDIR)/usr/mdec/boot.net $(RELEASEDIR)/installation/netboot
.for f in install.sh upgrade.sh install.sub ../sparc/install.md
	cp -p $(TOP)/distrib/miniroot/$f $(RELEASEDIR)/installation/netboot
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	sh $(TOP)/distrib/sets/makesums -t ${RELEASEDIR}/${dir} '*'
.endfor
