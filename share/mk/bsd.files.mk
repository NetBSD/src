#	$NetBSD: bsd.files.mk,v 1.15 2001/05/08 03:19:52 sommerfeld Exp $

# This file can be included multiple times.  It clears the definition of
# FILES at the end so that this is possible.

.PHONY:		filesinstall
realinstall:	filesinstall

.if defined(FILES) && !empty(FILES)
FILESDIR?=${BINDIR}
FILESOWN?=${BINOWN}
FILESGRP?=${BINGRP}
FILESMODE?=${NONBINMODE}

filesinstall:: ${FILES:@F@${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}@}
.PRECIOUS: ${FILES:@F@${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}@}
.if !defined(UPDATE)
.PHONY: ${FILES:@F@${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}@}
.endif

__fileinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} \
	    -o ${FILESOWN_${.ALLSRC:T}:U${FILESOWN}} \
	    -g ${FILESGRP_${.ALLSRC:T}:U${FILESGRP}} \
	    -m ${FILESMODE_${.ALLSRC:T}:U${FILESMODE}} \
	    ${.ALLSRC} ${.TARGET}

.for F in ${FILES:O:u}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}: .MADE
.endif
${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}: ${F} __fileinstall
.endfor
.endif

.if !target(filesinstall)
filesinstall::
.endif

FILES:=
