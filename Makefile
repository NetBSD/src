#	$NetBSD: Makefile,v 1.81 1999/01/28 15:36:48 scottr Exp $

.include <bsd.own.mk>			# for configuration variables.

# Configurations variables (can be set either in /etc/mk.conf or
# as environement variable
# NBUILDJOBS: the number of jobs to start in parallel in a 'make build'.
#             defaults to 1
# NOMAN: if set to 1, don't build and install man pages
# NOSHARE: if set to 1, don't build or install /usr/share stuffs
# UPDATE: if set to 1, don't do a 'make cleandir' before compile
# DESTDIR: The target directory for installation (default to '/',
#          which mean the current system is updated).

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

# This little mess makes the includes and install targets
# do the expected thing.
.if exists(domestic) && \
    (make(clean) || make(cleandir) || make(obj) || \
    (!defined(_BUILD) && (make(includes) || make(install))))
SUBDIR+= domestic
.endif

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
.if !defined(NOMAN) && !defined(NOSHARE) && !defined(_BUILD)
	${MAKE} whatis.db
.endif

whatis.db:
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)

build: beforeinstall
.if !defined(NOSHARE)
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
	(cd ${.CURDIR}/share/tmac && ${MAKE} && ${MAKE} install)
.endif
.if !defined(UPDATE)
	${MAKE} cleandir
.endif
.if empty(HAVE_GCC28)
.if defined(DESTDIR)
	@echo "*** CAPUTE!"
	@echo "    You attempted to compile the world with egcs.  You must"
	@echo "    first install a native egcs compiler."
	false
.else
	(cd ${.CURDIR}/gnu/usr.bin/egcs && \
	    ${MAKE} depend && ${MAKE} ${_J} NOMAN= && \
	    ${MAKE} NOMAN= install && ${MAKE} cleandir)
.endif
.endif
	${MAKE} _BUILD= includes
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} depend && ${MAKE} ${_J} NOMAN= && ${MAKE} NOMAN= install)
	(cd ${.CURDIR}/lib && \
	    ${MAKE} depend && ${MAKE} ${_J} NOMAN= && ${MAKE} NOMAN= install)
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} depend && ${MAKE} ${_J} NOMAN= && ${MAKE} NOMAN= install)
	${MAKE} depend && ${MAKE} ${_J} && ${MAKE} _BUILD= install
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/domestic && ${MAKE} ${_J} _SLAVE_BUILD= build)
.endif
	${MAKE} whatis.db
	@echo -n "Build finished at: "
	@date

.include <bsd.subdir.mk>
