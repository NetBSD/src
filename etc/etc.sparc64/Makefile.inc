#	$NetBSD: Makefile.inc,v 1.27 2007/07/24 10:49:28 pavel Exp $
#
#	etc.sparc64/Makefile.inc -- sparc64-specific etc Makefile targets
#
# Specify whether or not to build 32-bit libs.
MK32BITLIBS?=no

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS=	installation/miniroot	\
			installation/netboot	\
			installation/misc
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

# Used by 32 bit lib/csu, lib, gnu/lib & libexec/ld.elf_so
LIBDIR=/usr/lib/32
BINDIR=/usr/libexec/32
SPARC32_ENV=MACHINE=sparc MACHINE_ARCH=sparc
SPARC32=COPTS=-m32 LD="ld -m elf32_sparc" AS="as -32" \
	LIBDIR=${LIBDIR} BINDIR=${BINDIR} \
	SHLIB_LDSTARTFILE=${DESTDIR}${LIBDIR}/crtbeginS.o \
	SHLIB_LDENDFILE=${DESTDIR}${LIBDIR}/crtendS.o

.if ${MACHINE_ARCH} == "sparc64" && ${MACHINE} == "sparc64" && \
	${MK32BITLIBS} != "no"

	# build 32 bit programs
.for _s64dir in lib/csu lib gnu/lib libexec/ld_elf.so
.if ${MKOBJDIRS} != "no"
	(cd ${NETBSDSRCDIR}/${_s64dir} && \
	    ${SPARC32_ENV} ${MAKE} ${SPARC32} ${_M} obj)
.endif
.ifndef UPDATE
	(cd ${NETBSDSRCDIR}/${_s64dir} && ${MAKE} cleandir
.endif
	(cd ${NETBSDSRCDIR}/${_s64dir} && \
	    ${SPARC32_ENV} ${MAKE} ${SPARC32} ${_M} ${_J} MKSHARE=no dependall \
	    && ${SPARC32_ENV} ${MAKE} ${SPARC32} ${_M} MKSHARE=no install)
.endfor
.endif	# MACHINE_ARCH==sparc64 && MACHINE==sparc && MK32BITLIBS!=no

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

	# install boot image and installation scripts in netboot directory
.for file in ofwboot.net
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${MACHINE}/installation/netboot
.endfor
.for file in ofwboot bootblk
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
.endfor
.for file in installboot
	${HOST_INSTALL_FILE} -m ${BINMODE} ${DESTDIR}/usr/sbin/${file} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/${dir} '*'
.endfor

ISOBOOTFILEDIR!=cd ${.CURDIR}/../distrib/${MACHINE}/bootfs && ${PRINTOBJDIR}
ISOBOOTFILE?=	${ISOBOOTFILEDIR}/boot.fs

iso-image-md-post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
	    ${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
	    ${CDROM.image} - - - - ${ISOBOOTFILE}
