#	$NetBSD: Makefile,v 1.2 2003/09/13 20:11:38 lukem Exp $

.include <bsd.own.mk>


SUBDIR=	tools include lib bin share
XTODO=	Xserver


.if ${MKUPDATE} == "no" && !defined(NOCLEANDIR)
BUILDTARGETS+=  cleandir
.endif
.if ${MKOBJDIRS} != "no"
BUILDTARGETS+=  obj
.endif
BUILDTARGETS+=	includes do-tools
BUILDTARGETS+=	do-lib-X11 do-lib-ICE do-lib-Xau
BUILDTARGETS+=	do-lib-SM do-lib-Xext
BUILDTARGETS+=	do-lib-Xt do-lib-Xpm do-lib-Xrender do-lib-GL
BUILDTARGETS+=	do-lib-Xmu do-lib-dps
BUILDTARGETS+=	do-lib
BUILDTARGETS+=	do-build

.ORDER: ${BUILDTARGETS}


START_TIME!=	date

build:
	@echo "Build started at: ${START_TIME}"
.for tgt in ${BUILDTARGETS}
	@(cd ${.CURDIR} && ${MAKE} ${tgt})
.endfor
	@echo   "Build started at:  ${START_TIME}"
	@printf "Build finished at: " && date


do-build:
.for targ in dependall install
	(cd ${.CURDIR} && ${MAKE} ${targ})
.endfor

.for dir in	tools lib/X11 lib/ICE lib/Xau lib/SM lib/Xext lib/Xt lib/Xpm \
		lib/Xrender lib/GL lib/Xmu lib/dps lib
do-${dir:S/\//-/g}:
.for targ in dependall install
	(cd ${.CURDIR}/${dir} && ${MAKE} ${targ})
.endfor
.endfor


.include <bsd.subdir.mk>
