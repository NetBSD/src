#	$NetBSD: Makefile.inc,v 1.5 2001/01/07 09:31:17 mrg Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

TOP= $(.CURDIR)/..

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
#EXTRA_KERNELS+= POWEROFSEVEN
# XXX; no ramdisk install yet.
#BUILD_KERNELS+= INSTALL


# Add sparc64 installation directories
MD_INSTALLATION_DIRS= \
	installation/xminiroot installation/netboot installation/misc
INSTALLATION_DIRS+=$(MD_INSTALLATION_DIRS) 

snap_pre_md:
.ifndef UPDATE
	cd $(TOP)/distrib && ${MAKE} clean 
.endif

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
.if exists($(XSRCDIR)/xc/Imakefile)
	# build X11
.ifndef XSRC_DONE
	(cd $(XSRCDIR); $(MAKE) release)
.endif
.endif

	# make boot/miniroot images
	(cd $(TOP)/distrib; ${MAKE})

	# transfer images to release hierarchy
.for dir in notes sparc64/xminiroot
	(cd $(TOP)/distrib/$(dir); ${MAKE} release)
.endfor

	# install boot image and installation scripts in netboot directory
.for f in ofwboot.net
	cp -p $(DESTDIR)/usr/mdec/$f $(RELEASEDIR)/installation/netboot
.endfor
.for f in ofwboot bootblk installboot
	cp -p $(DESTDIR)/usr/mdec/$f $(RELEASEDIR)/installation/misc
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	sh $(TOP)/distrib/sets/makesums -t ${RELEASEDIR}/${dir} '*'
.endfor
