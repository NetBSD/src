#	$NetBSD: bsd.files.mk,v 1.12 2000/06/06 09:22:01 mycroft Exp $

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

.for F in ${FILES}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}: .MADE
.endif
${DESTDIR}${FILESDIR_${F}:U${FILESDIR}}/${FILESNAME_${F}:U${FILESNAME:U${F:T}}}: ${F}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} \
	    -o ${FILESOWN_${F}:U${FILESOWN}} -g ${FILESGRP_${F}:U${FILESGRP}} \
	    -m ${FILESMODE_${F}:U${FILESMODE}} ${.ALLSRC} ${.TARGET}
.endfor
.endif

.if !target(filesinstall)
filesinstall::
.endif

FILES:=
