#	$NetBSD: Makefile,v 1.102 1999/12/03 15:09:01 simonb Exp $

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
#	'make build'. It defaults to 1.
#   UPDATE will avoid a `make cleandir' at the start of `make build',
#	as well as having the effects listed in /usr/share/mk/bsd.README.
#
# Targets:
#   build: builds a full release of netbsd in DESTDIR.
#   release: does a `make build,' and then tars up the DESTDIR files
#	into RELEASEDIR, in release(7) format. (See etc/Makefile for
#	more information on this.)
#   snapshot: a synonym for release.

.include <bsd.own.mk>			# for configuration variables.

MKOBJDIRS ?= no
HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if defined(NBUILDJOBS)
_J= -j${NBUILDJOBS}
.endif

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share sys

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
	@(cd ${.CURDIR}/regress && ${MAKE} regress)
.endif

beforeinstall:
.ifmake build
	@echo -n "Build started at: "
	@date
.endif
.ifndef DESTDIR
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} distrib-dirs)
.endif

afterinstall:
.if ${MKMAN} != "no" && !defined(_BUILD)
	${MAKE} whatis.db
.endif

whatis.db:
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)

# wrt info/dir below:  It's safe to move this over top of /usr/share/info/dir,
# as the build will automatically remove/replace the non-pkg entries there.

build: beforeinstall
.if defined(FORCE_DOMESTIC)
	@echo '*** CAPUTE!'
	@echo '    The FORCE_DOMESTIC flag is not compatible with "make build".'
	@echo '    Please correct the problem and try again.'
	@false
.endif
.if ${MKSHARE} != "no"
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
.if ${MKOBJDIRS} != "no"
	${MAKE} obj
.endif
	(cd ${.CURDIR}/share/tmac && ${MAKE} && ${MAKE} install)
.endif
.if !defined(UPDATE)
	${MAKE} cleandir
.endif
.if empty(HAVE_GCC28)
.if defined(DESTDIR)
	@echo "*** CAPUTE!"
	@echo "    You attempted to compile the world without egcs.  You must"
	@echo "    first install a native egcs compiler."
	@false
.else
	(cd ${.CURDIR}/gnu/usr.bin/egcs && \
	    ${MAKE} ${_J} dependall MKMAN=no && \
	    ${MAKE} MKMAN=no install && ${MAKE} cleandir)
.endif
.endif
	${MAKE} includes
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} ${_J} dependall MKMAN=no && \
	    ${MAKE} MKMAN=no install)
	(cd ${.CURDIR}/lib && \
	    ${MAKE} ${_J} dependall MKMAN=no && \
	    ${MAKE} MKMAN=no install)
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} ${_J} dependall MKMAN=no MKINFO=no && \
	    ${MAKE} MKMAN=no MKINFO=no install)
	${MAKE} ${_J} dependall && ${MAKE} _BUILD= install
.if defined(DOMESTIC) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/${DOMESTIC} && ${MAKE} ${_J} _SLAVE_BUILD= build)
.endif
	${MAKE} whatis.db
	@echo -n "Build finished at: "
	@date

release snapshot: build
	(cd ${.CURDIR}/etc && ${MAKE} INSTALL_DONE=1 release)

.include <bsd.subdir.mk>
