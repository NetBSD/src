#	$NetBSD: bsd.inc.mk,v 1.18 2001/10/30 15:17:17 wiz Exp $

.PHONY:		incinstall
includes:	${INCS} incinstall

.if defined(INCS)
incinstall:: ${INCS:@I@${DESTDIR}${INCSDIR_${I}:U${INCSDIR}}/${INCSNAME_${I}:U${INCSNAME:U${I:T}}}@}
.PRECIOUS: ${INCS:@I@${DESTDIR}${INCSDIR_${I}:U${INCSDIR}}/${INCSNAME_${I}:U${INCSNAME:U${I:T}}}@}
.if !defined(UPDATE)
.PHONY: ${INCS:@I@${DESTDIR}${INCSDIR_${I}:U${INCSDIR}}/${INCSNAME_${I}:U${INCSNAME:U${I:T}}}@}
.endif

__incinstall: .USE
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${RENAME} ${PRESERVE} ${INSTPRIV} -c \
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} ${.ALLSRC} \
		${.TARGET}" && \
	     ${INSTALL} ${RENAME} ${PRESERVE} ${INSTPRIV} -c -o ${BINOWN} \
		 -g ${BINGRP} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET})

.for I in ${INCS:O:u}
${DESTDIR}${INCSDIR_${I}:U${INCSDIR}}/${INCSNAME_${I}:U${INCSNAME:U${I:T}}}: ${I} __incinstall
.endfor
.endif

.if !target(incinstall)
incinstall::
.endif
