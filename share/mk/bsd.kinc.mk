#	$NetBSD: bsd.kinc.mk,v 1.15 2000/06/06 09:22:01 mycroft Exp $

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
SYS_INCLUDE?=	copies

# If DESTDIR is set, we're probably building a release, so force "copies".
.if defined(DESTDIR) && (${DESTDIR} != "/" && !empty(DESTDIR))
SYS_INCLUDE=	copies
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

incinstall:: ${DESTDIR}${INCSDIR}
.PRECIOUS: ${DESTDIR}${INCSDIR}
.PHONY: ${DESTDIR}${INCSDIR}

${DESTDIR}${INCSDIR}:
	@if [ ! -d ${.TARGET} ] || [ -h ${.TARGET} ] ; then \
		echo creating ${.TARGET}; \
		/bin/rm -rf ${.TARGET}; \
		${INSTALL} ${INSTPRIV} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		    ${.TARGET}; \
	fi

.if defined(INCS)
incinstall:: ${INCS:@I@${DESTDIR}${INCSDIR}/$I@}
.PRECIOUS: ${INCS:@I@${DESTDIR}${INCSDIR}/$I@}
.if !defined(UPDATE)
.PHONY: ${INCS:@I@${DESTDIR}${INCSDIR}/$I@}
.endif

.for I in ${INCS}
${DESTDIR}${INCSDIR}/$I: ${DESTDIR}${INCSDIR} $I 
	@cmp -s ${.CURDIR}/$I ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${RENAME} ${PRESERVE} ${INSTPRIV} -c \
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} ${.CURDIR}/$I \
		${.TARGET}" && \
	     ${INSTALL} ${RENAME} ${PRESERVE} ${INSTPRIV} -c -o ${BINOWN} \
		-g ${BINGRP} -m ${NONBINMODE} ${.CURDIR}/$I ${.TARGET})
.endfor
.endif

.if defined(DEPINCS)
incinstall:: ${DEPINCS:@I@${DESTDIR}${INCSDIR}/$I@}
.PRECIOUS: ${DEPINCS:@I@${DESTDIR}${INCSDIR}/$I@}
.if !defined(UPDATE)
.PHONY: ${DEPINCS:@I@${DESTDIR}${INCSDIR}/$I@}
.endif

.for I in ${DEPINCS}
${DESTDIR}${INCSDIR}/$I: ${DESTDIR}${INCSDIR} $I 
	@cmp -s $I ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${RENAME} ${PRESERVE} -c -o ${BINOWN} \
		-g ${BINGRP} -m ${NONBINMODE} $I ${.TARGET}" && \
	     ${INSTALL} ${RENAME} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} $I ${.TARGET})
.endfor
.endif

.endif # not symlinks

.if defined(SYMLINKS) && !empty(SYMLINKS)
incinstall::
	@(set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		if [ -L $$t ]; then \
			cur=`ls -ld $$t | awk '{print $$NF}'` ; \
			if [ "$$cur" = "$$l" ]; then \
				continue ; \
			fi; \
		fi; \
		echo "$$t -> $$l"; \
		rm -rf $$t; ln -s $$l $$t; \
	 done; )
.endif

.if !target(incinstall)
incinstall::
.endif

.include <bsd.subdir.mk>
