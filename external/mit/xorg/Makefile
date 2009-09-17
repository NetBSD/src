#	$NetBSD: Makefile,v 1.4.2.2 2009/09/17 04:24:28 snj Exp $

# Top level Makefile to cross build xorg inside the NetBSD tree.

.include <bsd.own.mk>

SUBDIR=		tools .WAIT include .WAIT lib .WAIT bin share server

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

.for dir in	tools lib
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


DISTRIBTARGETS=	\
	bin/lbxproxy \
	bin/proxymngr \
	bin/twm \
	bin/xdm/config \
	bin/xfs \
	bin/xinit \
	bin/xsm \
	lib/fontconfig/etc \
	lib/fontconfig/etc/conf.avail \
	lib/fontconfig/etc/conf.d

distribution: check_X11SRCDIR
.for tgt in ${DISTRIBTARGETS}
	${MAKEDIRTARGET} ${tgt} configinstall
.endfor


check_X11SRCDIR: .PHONY .NOTMAIN
.if !defined(X11SRCDIR)
	@echo
	@echo "ERROR: setenv X11SRCDIR before doing that!"
	@false
.elif !exists(${X11SRCDIR.X11})
	@echo
	@echo "ERROR: X11SRCDIR.X11 \`${X11SRCDIR.X11}' does not exist"
	@false
.else
	@true
.endif

.include <bsd.subdir.mk>
