#	$NetBSD: bsd.files.mk,v 1.19 2002/04/24 08:18:52 lukem Exp $

.if !target(__fileinstall)
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

.if !defined(UPDATE)
${_F}!		${F} __fileinstall			# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}!		.MADE					# no build at install
.endif
.else
${_F}:		${F} __fileinstall			# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endif

filesinstall::	${_F}
.PRECIOUS: 	${_F}					# keep if install fails
.endfor

.undef _FDIR
.undef _FNAME
.undef _F
