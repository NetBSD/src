#	$NetBSD: Makefile,v 1.11 2004/04/13 12:50:34 lukem Exp $

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
BUILDTARGETS+=	do-lib-Xmu do-lib-dps do-lib-expat
BUILDTARGETS+=	do-lib
BUILDTARGETS+=	do-build

.ORDER: ${BUILDTARGETS}

START_TIME!=	date

build: check_X11SRCDIR
	@echo "X11 build started at: ${START_TIME}"
.for tgt in ${BUILDTARGETS}
	${MAKEDIRTARGET} . ${tgt}
.endfor
	@echo   "X11 build started at:  ${START_TIME}"
	@printf "X11 build finished at: " && date

do-build:
.for targ in dependall install
	${MAKEDIRTARGET} . ${targ}
.endfor

.for dir in	tools lib/X11 lib/ICE lib/Xau lib/SM lib/Xext lib/Xt lib/Xpm \
		lib/Xrender lib/GL lib/Xmu lib/dps lib/expat lib
do-${dir:S/\//-/g}:
.  for targ in dependall install
	${MAKEDIRTARGET} ${dir} ${targ}
.  endfor
.endfor

afterinstall: .PHONY
.if ${MKMAN} != "no"
	${MAKEDIRTARGET} ${NETBSDSRCDIR}/share/man makedb \
	    WHATISDBDIR=${X11MANDIR}
.endif


check_X11SRCDIR: .PHONY .NOTMAIN
.if !defined(X11SRCDIR)
	@echo
	@echo "ERROR: setenv X11SRCDIR before doing that!"
	@false
.elif !exists(${X11SRCDIR.xc})
	@echo
	@echo "ERROR: X11SRCDIR.xc \`${X11SRCDIR.xc}' does not exist"
	@false
.else
	@true
.endif

.include <bsd.subdir.mk>
