#	$NetBSD: bsd.kinc.mk,v 1.22 2002/02/11 21:14:59 mycroft Exp $

# System configuration variables:
#
# SYS_INCLUDE	"symlinks": symlinks to include directories are created.
#		This may not work 100% properly for all headers.
#
#		"copies": directories are made, if necessary, and headers
#		are installed into them.
#
# Variables:
#
# INCSDIR	Directory to install includes into (and/or make, and/or
#		symlink, depending on what's going on).
#
# KDIR		Kernel directory to symlink to, if SYS_INCLUDE is symlinks.
#		If unspecified, no action will be taken when making include
#		for the directory if SYS_INCLUDE is symlinks.
#
# INCS		Headers to install, if SYS_INCLUDE is copies.
#
# DEPINCS	Headers to install which are built dynamically.
#
# SUBDIR	Subdirectories to enter
#
# SYMLINKS	Symlinks to make (unconditionally), a la bsd.links.mk.
#		Note that the original bits will be 'rm -rf'd rather than
#		just 'rm -f'd, to make the right thing happen with include
#		directories.
#

.include <bsd.init.mk>

##### Basic targets
.PHONY:		incinstall
includes:	${INCS} incinstall

##### Default values
# Change SYS_INCLUDE in bsd.own.mk or /etc/mk.conf to "symlinks" if you
# don't want copies
SYS_INCLUDE?=	copies

# If DESTDIR is set, we're probably building a release, so force "copies".
.if defined(DESTDIR) && (${DESTDIR} != "/" && !empty(DESTDIR))
SYS_INCLUDE=	copies
.endif

##### Install rules
incinstall::	# ensure existence

.if ${SYS_INCLUDE} == "symlinks"

# don't install includes, just make symlinks.
.if defined(KDIR)
SYMLINKS+=	${KDIR} ${INCSDIR}
.endif

.else # ${SYS_INCLUDE} != "symlinks"

# make sure the directory is OK, and install includes.
incinstall::	${DESTDIR}${INCSDIR}
.PRECIOUS:	${DESTDIR}${INCSDIR}
.PHONY:		${DESTDIR}${INCSDIR}

${DESTDIR}${INCSDIR}:
	@if [ ! -d ${.TARGET} ] || [ -h ${.TARGET} ] ; then \
		echo creating ${.TARGET}; \
		/bin/rm -rf ${.TARGET}; \
		${INSTALL_DIR} -o ${BINOWN} -g ${BINGRP} -m 755 ${.TARGET}; \
	fi

# -c is forced on here, in order to preserve modtimes for "make depend"
__incinstall: .USE
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL_FILE:N-c} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL_FILE:N-c} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET})

.for F in ${INCS:O:u} ${DEPINCS:O:u}
_F:=		${DESTDIR}${INCSDIR}/${F}		# installed path

.if !defined(UPDATE)
${_F}!		${F} __incinstall			# install rule
.else
${_F}:		${F} __incinstall			# install rule
.endif

incinstall::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.endfor

.undef _F

.endif # ${SYS_INCLUDE} ?= "symlinks"

.if defined(SYMLINKS) && !empty(SYMLINKS)
incinstall::
	@(set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; shift; \
		t=${DESTDIR}$$1; shift; \
		if [ -h $$t ]; then \
			cur=`ls -ld $$t | awk '{print $$NF}'` ; \
			if [ "$$cur" = "$$l" ]; then \
				continue ; \
			fi; \
		fi; \
		echo "$$t -> $$l"; \
		rm -rf $$t; ${INSTALL_SYMLINK} $$l $$t; \
	 done; )
.endif

##### Pull in related .mk logic
.include <bsd.subdir.mk>
