#	$NetBSD: bsd.files.mk,v 1.5 1997/05/09 13:25:49 mycroft Exp $

.PHONY:		filesinstall
realinstall:	filesinstall

.if defined(FILES)
FILESDIR?=${BINDIR}
FILESOWN?=${BINOWN}
FILESGRP?=${BINGRP}
FILESMODE?=${NONBINMODE}
.for F in ${FILES}
FILESDIR_${F}?=${FILESDIR}
FILESOWN_${F}?=${FILESOWN}
FILESGRP_${F}?=${FILESGRP}
FILESMODE_${F}?=${FILESMODE}
.if defined(FILESNAME)
FILESNAME_${F} ?= ${FILESNAME}
.else
FILESNAME_${F} ?= ${F:T}
.endif
FILESDIR_${F} ?= ${FILESDIR}
filesinstall:: ${DESTDIR}${FILESDIR_${F}}/${FILESNAME_${F}}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${FILESDIR_${F}}/${FILESNAME_${F}}
.endif
.if !defined(BUILD)
${DESTDIR}${FILESDIR_${F}}/${FILESNAME_${F}}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${FILESDIR_${F}}/${FILESNAME_${F}}
${DESTDIR}${FILESDIR_${F}}/${FILESNAME_${F}}: ${F}
	${INSTALL} ${COPY} -o ${FILESOWN_${F}} -g ${FILESGRP_${F}} \
		-m ${FILESMODE_${F}} ${.ALLSRC} ${.TARGET}
.endfor
.endif

.if !target(filesinstall)
filesinstall::
.endif
