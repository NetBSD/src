#	$NetBSD: Makefile,v 1.132 2001/10/01 17:19:17 mason Exp $

# This is the top-level makefile for building NetBSD. For an outline of
# how to build a snapshot or release, as well as other release engineering
# information, see http://www.netbsd.org/developers/releng/index.html
#
# Not everything you can set or do is documented in this makefile. In
# particular, you should review the files in /usr/share/mk (especially
# bsd.README) for general information on building programs and writing
# Makefiles within this structure, and see the comments in src/etc/Makefile
# for further information on installation and release set options.
#
# Variables listed below can be set on the make command line (highest
# priority), in /etc/mk.conf (middle priority), or in the environment
# (lowest priority).
#
# Variables:
#   DESTDIR is the target directory for installation of the compiled
#	software. It defaults to /. Note that programs are built against
#	libraries installed in DESTDIR.
#   MKMAN, if set to `no', will prevent building of manual pages.
#   MKOBJDIRS, if not set to `no', will build object directories at 
#	an appropriate point in a build.
#   MKSHARE, if set to `no', will prevent building and installing
#	anything in /usr/share.
#   NBUILDJOBS is the number of jobs to start in parallel during a
#	`make build'. It defaults to 1.
#   UPDATE, if defined, will avoid a `make cleandir' at the start of
#     `make build', as well as having the effects listed in
#     /usr/share/mk/bsd.README.
#   NOCLEANDIR, if defined, will avoid a `make cleandir' at the start
#     of the `make build'.
#   NOINCLUDES will avoid the `make includes' usually done by `make build'.

#
# Targets:
#   build: builds a full release of netbsd in DESTDIR. If BUILD_DONE is
#	set, this is an empty target.
#   release: does a `make build,' and then tars up the DESTDIR files
#	into RELEASEDIR, in release(7) format. (See etc/Makefile for
#	more information on this.)
#   snapshot: a synonym for release.
#
# Sub targets of `make build,' in order:
#   buildstartmsg: displays the start time of the build.
#   do-make-tools: builds host toolchain.
#   do-distrib-dirs: creates the distribution directories.
#   do-force-domestic: check's that FORCE_DOMESTIC isn't set (deprecated.)
#   do-cleandir: cleans the tree.
#   do-make-obj: creates object directories if required.
#   do-make-includes: install include files.
#   do-lib-csu: build & install startup object files.
#   do-lib: build & install system libraries.
#   do-gnu-lib: build & install gnu system libraries.
#   do-dependall: builds & install the entire system.
#   do-domestic: build & install the domestic tree (deprecated.)
#   do-whatisdb: build & install the `whatis.db' man database.
#   buildendmsg: displays the end time of the build.

.include <bsd.own.mk>

MKOBJDIRS ?= no

.if defined(NBUILDJOBS)
_J= -j${NBUILDJOBS}
.endif

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share sys
.if make(cleandir) || make(obj)
SUBDIR+= distrib
.ifdef MAKEOBJDIRPREFIX
SUBDIR+= etc
.endif
.endif

includes-lib: includes-include includes-sys

.if exists(games)
SUBDIR+= games
.endif

SUBDIR+= gnu
# This is needed for libstdc++ and gen-params.
includes-gnu: includes-include includes-sys

.if exists(regress)
.ifmake !(install)
SUBDIR+= regress
.endif

regression-tests:
	@echo Running regression tests...
	@(cd ${.CURDIR}/regress && ${MAKE} ${_M} regress)
.endif

buildstartmsg:
	@echo -n "Build started at: "
	@date

buildendmsg:
	@echo -n "Build finished at: "
	@date

afterinstall:
.if ${MKMAN} != "no" && !defined(_BUILD)
	${MAKE} ${_M} whatis.db
.endif

whatis.db:
	(cd ${.CURDIR}/share/man && ${MAKE} ${_M} makedb)

# wrt info/dir below:  It's safe to move this over top of /usr/share/info/dir,
# as the build will automatically remove/replace the non-pkg entries there.

.if defined(BUILD_DONE)
build:
	@echo "Build installed into ${DESTDIR}"
.else
build:
	@${MAKE} ${_M} buildstartmsg
	@${MAKE} ${_M} do-make-tools
	@${MAKE} ${_M} do-distrib-dirs
	@${MAKE} ${_M} do-force-domestic
	@${MAKE} ${_M} do-cleandir
	@${MAKE} ${_M} do-make-obj
	@${MAKE} ${_M} do-make-includes
	@${MAKE} ${_M} do-lib-csu
	@${MAKE} ${_M} do-lib
	@${MAKE} ${_M} do-gnu-lib
	@${MAKE} ${_M} do-dependall
	@${MAKE} ${_M} do-domestic
	@${MAKE} ${_M} do-whatisdb
	@${MAKE} ${_M} buildendmsg
.endif

do-make-tools:
.if ${USETOOLS} != "no"
.if ${MKOBJDIRS} != "no"
	cd ${.CURDIR}/tools && ${MAKE} ${_M} obj
.endif
	cd ${.CURDIR}/tools && ${MAKE} ${_M} build
.endif

do-distrib-dirs:
.ifndef NODISTRIBDIRS
.ifndef DESTDIR
	(cd ${.CURDIR}/etc && ${MAKE} ${_M} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} ${_M} DESTDIR=${DESTDIR} distrib-dirs)
.endif
.endif

do-force-domestic:
.if defined(FORCE_DOMESTIC)
	@echo '*** CAPUTE!'
	@echo '    The FORCE_DOMESTIC flag is not compatible with "make build".'
	@echo '    Please correct the problem and try again.'
	@false
.endif

do-cleandir:
.if !defined(UPDATE) && !defined(NOCLEANDIR)
	${MAKE} ${_J} ${_M} cleandir
.endif

do-make-obj:
.if ${MKOBJDIRS} != "no"
	${MAKE} ${_J} ${_M} obj
.endif

do-make-includes:
.if !defined(NOINCLUDES)
	${MAKE} ${_M} includes
.endif

do-lib-csu:
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)

do-lib:
	(cd ${.CURDIR}/lib && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)

do-gnu-lib:
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)

do-dependall:
	${MAKE} ${_M} ${_J} dependall && ${MAKE} ${_M} _BUILD= install

do-domestic:
.if defined(DOMESTIC) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/${DOMESTIC} && ${MAKE} ${_M} ${_J} _SLAVE_BUILD= build)
.endif

do-whatisdb:
	${MAKE} ${_M} whatis.db

release snapshot: build
	(cd ${.CURDIR}/etc && ${MAKE} ${_M} INSTALL_DONE=1 release)

# Speedup stubs for some subtrees that don't need to run these rules
includes-bin includes-games includes-libexec includes-regress \
includes-sbin includes-usr.sbin:
	@${TRUE}

.if !exists(${.CURDIR}/share/mk)
.BEGIN:
	@echo 'BUILD ABORTED: share/mk does not exist, cannot run make.'
	@false
.endif

.include <bsd.subdir.mk>

_M:=	-m ${.CURDIR}/share/mk
