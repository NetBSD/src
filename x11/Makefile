#	$NetBSD: Makefile,v 1.7 2004/01/23 23:03:23 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	tools include lib bin share Xserver

.if ${MKUPDATE} == "no" && !defined(NOCLEANDIR)
BUILDTARGETS+=  cleandir
.endif
.if ${MKOBJDIRS} != "no"
BUILDTARGETS+=  obj
.endif
.if !defined(NOINCLUDES)
BUILDTARGETS+=	includes
.endif
BUILDTARGETS+=	do-tools
BUILDTARGETS+=	do-lib-X11 do-lib-ICE do-lib-Xau
BUILDTARGETS+=	do-lib-SM do-lib-Xext
BUILDTARGETS+=	do-lib-Xt do-lib-Xpm do-lib-Xrender do-lib-GL
BUILDTARGETS+=	do-lib-Xmu do-lib-dps
BUILDTARGETS+=	do-lib
BUILDTARGETS+=	do-build

.ORDER: ${BUILDTARGETS}

START_TIME!=	date

build: check_X11SRCDIR
	@echo "X11 build started at: ${START_TIME}"
.for tgt in ${BUILDTARGETS}
	@(cd ${.CURDIR} && ${MAKE} ${tgt})
.endfor
	@echo   "X11 build started at:  ${START_TIME}"
	@printf "X11 build finished at: " && date

do-build:
.for targ in dependall install
	(cd ${.CURDIR} && ${MAKE} ${targ})
.endfor

.for dir in	tools lib/X11 lib/ICE lib/Xau lib/SM lib/Xext lib/Xt lib/Xpm \
		lib/Xrender lib/GL lib/Xmu lib/dps lib
do-${dir:S/\//-/g}:
.  for targ in dependall install
	(cd ${.CURDIR}/${dir} && ${MAKE} ${targ})
.  endfor
.endfor


check_X11SRCDIR: .PHONY .NOTMAIN
.if !defined(X11SRCDIR)
	@echo
	@echo "ERROR: setenv X11SRCDIR before doing that!"
	@false
.elif !exists(${X11SRCDIR})
	@echo
	@echo "ERROR: X11SRCDIR \`${X11SRCDIR}' does not exist"
	@false
.else
	@true
.endif

.include <bsd.subdir.mk>
