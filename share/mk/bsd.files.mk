#	$NetBSD: bsd.files.mk,v 1.16 2001/11/02 05:21:50 tv Exp $

.if !target(__fileinstall)
# This file can be included multiple times.  It clears the definition of
# FILES at the end so that this is possible.

##### Basic targets
.PHONY:		filesinstall
realinstall:	filesinstall

##### Default values
FILESDIR?=	${BINDIR}
FILESOWN?=	${BINOWN}
FILESGRP?=	${BINGRP}
FILESMODE?=	${NONBINMODE}

##### Install rules
filesinstall::	# ensure existence

__fileinstall: .USE
	${INSTALL_FILE} \
	    -o ${FILESOWN_${.ALLSRC:T}:U${FILESOWN}} \
	    -g ${FILESGRP_${.ALLSRC:T}:U${FILESGRP}} \
	    -m ${FILESMODE_${.ALLSRC:T}:U${FILESMODE}} \
	    ${.ALLSRC} ${.TARGET}

.endif # !target(__fileinstall)

.for F in ${FILES:O:u}
_FDIR:=		${FILESDIR_${F}:U${FILESDIR}}		# dir override
_FNAME:=	${FILESNAME_${F}:U${FILESNAME:U${F:T}}}	# name override
_F:=		${DESTDIR}${_FDIR}/${_FNAME}		# installed path

${_F}:		${F} __fileinstall			# install rule
filesinstall::	${_F}
.PRECIOUS: 	${_F}					# keep if install fails
.PHONY:		${UPDATE:U${_F}}			# clobber unless UPDATE
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endfor

.undef _FDIR
.undef _FNAME
.undef _F

FILES:=		# reset to empty
