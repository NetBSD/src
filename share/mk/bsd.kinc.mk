#	$NetBSD: bsd.kinc.mk,v 1.4 1999/01/15 10:57:36 castor Exp $

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

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.MAIN:		all
.endif

# Change SYS_INCLUDE in bsd.own.mk or /etc/mk.conf to "symlinks" if you
# don't want copies
SYS_INCLUDE?=   copies

# If DESTDIR is set, we're probably building a release, so force "copies".
.if defined(DESTDIR) && (${DESTDIR} != "/" && !empty(DESTDIR))
SYS_INCLUDE=    copies
.endif


.PHONY:		incinstall
includes:	${INCS} incinstall


.if ${SYS_INCLUDE} == "symlinks"

# don't install includes, just make symlinks.

.if defined(KDIR)
SYMLINKS+=	${KDIR} ${INCSDIR}
.endif

.else # not symlinks

# make sure the directory is OK, and install includes.

.PRECIOUS: ${DESTDIR}${INCSDIR}
.PHONY: ${DESTDIR}${INCSDIR}
${DESTDIR}${INCSDIR}:
	@if [ ! -d ${.TARGET} ] || [ -L ${.TARGET} ] ; then \
		echo creating ${.TARGET}; \
		/bin/rm -rf ${.TARGET}; \
		${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 ${.TARGET}; \
	fi

incinstall:: ${DESTDIR}${INCSDIR}

.if defined(INCS)
.for I in ${INCS}
incinstall:: ${DESTDIR}${INCSDIR}/$I

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${INCSDIR}/$I
.endif
${DESTDIR}${INCSDIR}/$I: ${DESTDIR}${INCSDIR} $I 
	@cmp -s ${.CURDIR}/$I ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.CURDIR}/$I ${.TARGET}" && \
	     ${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.CURDIR}/$I ${.TARGET})
.endfor
.endif

.if defined(DEPINCS)
.for I in ${DEPINCS}
incinstall:: ${DESTDIR}${INCSDIR}/$I

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${INCSDIR}/$I
.endif
${DESTDIR}${INCSDIR}/$I: ${DESTDIR}${INCSDIR} $I 
	@cmp -s $I ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} $I ${.TARGET}" && \
	     ${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} $I ${.TARGET})
.endfor
.endif

.endif # not symlinks

.if defined(SYMLINKS) && !empty(SYMLINKS)
incinstall::
	@set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo ".include <bsd.own.mk>"; \
		echo "all:: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -rf $$t"; \
		echo "	@ln -s $$l $$t"; \
	done | ${MAKE} -f-
.endif

.if !target(incinstall)
incinstall::
.endif

.include <bsd.subdir.mk>
