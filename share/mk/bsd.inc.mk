#	$NetBSD: bsd.inc.mk,v 1.8.2.1 1997/08/01 21:08:00 cjs Exp $
#
# If an include file is already installed, this always compares the
# installed file to the one we are about to install. If they are the
# same, it doesn't do the install. This is to avoid triggering massive
# amounts of rebuilding.
#
# Targets:
#
# includes:
#	Installs the include files in BUILDDIR if we are using one, DESTDIR
#	otherwise. This is to be run before `make depend' or `make'.
#
# install:
#	Installs the include files in DESTDIR.
#
#	$NetBSD: bsd.inc.mk,v 1.8.2.1 1997/08/01 21:08:00 cjs Exp $

.PHONY:		incbuild incinstall

.if defined(INCS)

.for I in ${INCS}

.if defined(OBJDIR)
.PRECIOUS: ${BUILDDIR}${INCSDIR}/$I
${BUILDDIR}${INCSDIR}/$I: $I
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} -d `dirname ${.TARGET}`" && \
	    ${INSTALL} -d `dirname ${.TARGET}` && \
	    echo "${INSTALL} -c -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} -c -m ${NONBINMODE} ${.ALLSRC} ${.TARGET})

incbuild:: ${BUILDDIR}${INCSDIR}/$I
.else
incbuild:: ${DESTDIR}${INCSDIR}/$I
.endif	# defined(OBJDIR)

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${INCSDIR}/$I
.endif
${DESTDIR}${INCSDIR}/$I: $I
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET})

incinstall:: ${DESTDIR}${INCSDIR}/$I
.endfor

includes:	${INCS} incbuild

install:	${INCS} incinstall

.endif	# defined(INCS)

.if !target(incbuild)
incbuild::
.endif

.if !target(incinstall)
incinstall::
.endif
