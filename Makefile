#	$NetBSD: Makefile,v 1.117 2000/05/21 07:33:05 mrg Exp $

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
#   build: builds a full release of netbsd in DESTDIR. If BUILD_DONE is
#	set, this is an empty target.
#   release: does a `make build,' and then tars up the DESTDIR files
#	into RELEASEDIR, in release(7) format. (See etc/Makefile for
#	more information on this.)
#   snapshot: a synonym for release.

SRCTOP=.
.include <bsd.crypto.mk>		# for configuration variables.

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/Makefile.frag"
.endif

MKOBJDIRS ?= no
HAVE_EGCS!=	${CXX} --version | egrep "^(2\.[89]|egcs)" ; echo

.if defined(NBUILDJOBS)
_J= -j${NBUILDJOBS}
.endif

.if defined(DESTDIR)
_M=-m ${DESTDIR}/usr/share/mk
.endif

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share sys
.if make(obj)
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

buildmsg:
	@echo -n "Build started at: "
	@date

beforeinstall:
.ifndef NODISTRIBDIRS
.ifndef DESTDIR
	(cd ${.CURDIR}/etc && ${MAKE} DESTDIR=/ distrib-dirs)
.else
	(cd ${.CURDIR}/etc && ${MAKE} distrib-dirs)
.endif
.endif

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
build: buildmsg beforeinstall
.if defined(FORCE_DOMESTIC)
	@echo '*** CAPUTE!'
	@echo '    The FORCE_DOMESTIC flag is not compatible with "make build".'
	@echo '    Please correct the problem and try again.'
	@false
.endif
.if ${MKSHARE} != "no"
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
.endif
.if !defined(UPDATE) && !defined(NOCLEANDIR)
	${MAKE} ${_J} ${_M} cleandir
.endif
.if ${MKOBJDIRS} != "no"
	${MAKE} ${_M} obj
.endif
.if empty(HAVE_EGCS)
.if defined(DESTDIR)
	@echo "*** CAPUTE!"
	@echo "    You attempted to compile the world without egcs.  You must"
	@echo "    first install a native egcs compiler."
	@false
.else
	(cd ${.CURDIR}/gnu/usr.bin/egcs && \
	    ${MAKE} ${_M} ${_J} dependall MKMAN=no && \
	    ${MAKE} ${_M} MKMAN=no install && ${MAKE} ${_M} cleandir)
.endif
.endif
.if !defined(NOINCLUDES)
	${MAKE} ${_M} includes
.endif
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)
	(cd ${.CURDIR}/lib && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} ${_M} ${_J} MKSHARE=no dependall && \
	    ${MAKE} ${_M} MKSHARE=no install)
.if target(cryptobuild)
	${MAKE} ${_M} ${_J} cryptobuild
.endif
	${MAKE} ${_M} ${_J} dependall && ${MAKE} ${_M} _BUILD= install
.if defined(DOMESTIC) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/${DOMESTIC} && ${MAKE} ${_M} ${_J} _SLAVE_BUILD= build)
.endif
	${MAKE} ${_M} whatis.db
	@echo -n "Build finished at: "
	@date
.endif

release snapshot: build
	(cd ${.CURDIR}/etc && ${MAKE} ${_M} INSTALL_DONE=1 release)

.include <bsd.subdir.mk>
