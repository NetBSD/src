#	$NetBSD: Makefile,v 1.84 1999/02/11 14:49:49 tv Exp $

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

# Descend into the domestic tree if it exists AND
#  1) the target is clean, cleandir, or obj, OR
#  2) the the target is install or includes AND NOT
#    a) compiling only "exportable" code OR
#    b) doing it as part of build.

.if exists(domestic) && \
    (make(clean) || make(cleandir) || make(obj) || \
    ((make(includes) || make(install)) && \
    !(defined(EXPORTABLE_SYSTEM) || defined(_BUILD))))
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

# wrt info/dir below:  It's safe to move this over top of /usr/share/info/dir,
# as the build will automatically remove/replace the non-pkg entries there.

build: beforeinstall
.if !defined(NOSHARE)
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
	(cd ${.CURDIR}/share/tmac && ${MAKE} && ${MAKE} install)
	if [ -f ${DESTDIR}/usr/pkg/info/dir ]; then \
		${INSTALL} -c -m 644 -o ${BINOWN} -g ${BINGRP} \
			${DESTDIR}/usr/pkg/info/dir ${DESTDIR}/usr/share/info/dir; \
		rm -f ${DESTDIR}/usr/pkg/info/dir; \
		ln -s /usr/share/info/dir ${DESTDIR}/usr/pkg/info/dir; \
	fi
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
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/domestic/lib && \
	    ${MAKE} depend && ${MAKE} ${_J} NOMAN= && ${MAKE} NOMAN= install)
.endif
	${MAKE} depend && ${MAKE} ${_J} && ${MAKE} _BUILD= install
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
	(cd ${.CURDIR}/domestic && ${MAKE} ${_J} _SLAVE_BUILD= build)
.endif
	${MAKE} whatis.db
	@echo -n "Build finished at: "
	@date

.include <bsd.subdir.mk>
