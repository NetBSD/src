#	$NetBSD: Makefile.inc,v 1.10 2001/08/23 16:01:56 mrg Exp $
#
#	etc.sparc64/Makefile.inc -- sparc64-specific etc Makefile targets
#

TOP= $(.CURDIR)/..

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
BUILD_KERNELS+= INSTALL

# Add sparc64 installation directories
MD_INSTALLATION_DIRS= \
	installation/miniroot installation/netboot installation/misc
INSTALLATION_DIRS+=$(MD_INSTALLATION_DIRS) 

# Used by 32 bit lib/csu, lib, gnu/lib & libexec/ld.elf_so
LIBDIR=/usr/lib/sparcv7
BINDIR=/usr/libexec/sparcv7
SPARC32=COPTS=-m32 LD="ld -m elf32_sparc" AS="as -32" \
	MACHINE=sparc MACHINE_ARCH=sparc LIBDIR=${LIBDIR} BINDIR=${BINDIR} \
	SHLIB_LDSTARTFILE=${DESTDIR}${LIBDIR}/crtbeginS.o \
	SHLIB_LDENDFILE=${DESTDIR}${LIBDIR}/crtendS.o

snap_md_pre:
.ifndef UPDATE
	# clean up the distrib area
	cd $(TOP)/distrib && ${MAKE} clean 
.endif

.if (${MACHINE_ARCH} == sparc64 && ${MACHINE} == sparc64 && ${MK32BITLIBS} == yes)

	# build 32 bit csu
.if ${MKOBJDIRS} != "no"
	(cd ${.CURDIR}/../lib/csu && \
	    ${MAKE} ${SPARC32} ${_M} obj)
.endif
	(cd ${.CURDIR}/../lib/csu && \
	    ${MAKE} ${SPARC32} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${SPARC32} ${_M} MKSHARE=no install)

	# build 32 bit libraries
.if ${MKOBJDIRS} != "no"
	(cd ${.CURDIR}/../lib && \
	    ${MAKE} ${SPARC32} ${_M} obj)
.endif
	(cd ${.CURDIR}/../lib && \
	    ${MAKE} ${SPARC32} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${SPARC32} ${_M} MKSHARE=no install)

	# build 32 bit gnu libraries
.if ${MKOBJDIRS} != "no"
	(cd ${.CURDIR}/../gnu/lib && \
	    ${MAKE} ${SPARC32} ${_M} obj)
.endif
	(cd ${.CURDIR}/../gnu/lib && \
	    ${MAKE} ${SPARC32} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${SPARC32} ${_M} MKSHARE=no install)

	# build 32 bit ld.elf_so
.if ${MKOBJDIRS} != "no"
	(cd ${.CURDIR}/../libexec/ld.elf_so && \
	    ${MAKE} ${SPARC32} ${_M} obj)
.endif
	(cd ${.CURDIR}/../libexec/ld.elf_so && \
	    ${MAKE} ${SPARC32} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${SPARC32} ${_M} MKSHARE=no install)
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
.for dir in notes sparc64/xminiroot sparc64/instfs
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
