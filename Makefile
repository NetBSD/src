#	$NetBSD: Makefile,v 1.75 1998/12/12 23:44:22 tv Exp $

.include <bsd.own.mk>			# for configuration variables.

HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

# NOTE THAT etc *DOES NOT* BELONG IN THE LIST BELOW

SUBDIR+= lib include bin libexec sbin usr.bin usr.sbin share sys

.if exists(games)
SUBDIR+= games
.endif

SUBDIR+= gnu
# This is needed for libstdc++ and gen-params.
includes-gnu: includes-include includes-sys

.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
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
.if !defined(NOMAN) && !defined(NOSHARE)
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)
.endif

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
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install && \
	    ${MAKE} cleandir)
.endif
.endif
	${MAKE} includes
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
	(cd ${.CURDIR}/lib && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
# libtelnet depends on libdes and libkrb.  libkrb depends on
# libcom_err.
.if exists(domestic/lib/libdes)
	(cd ${.CURDIR}/domestic/lib/libdes && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
.endif
.if exists(domestic/lib/libcom_err)
	(cd ${.CURDIR}/domestic/lib/libcom_err && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
.endif
.if exists(domestic/lib/libkrb)
	(cd ${.CURDIR}/domestic/lib/libkrb && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
.endif
	(cd ${.CURDIR}/domestic/lib && \
	    ${MAKE} depend && ${MAKE} NOMAN= && ${MAKE} NOMAN= install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install
	@echo -n "Build finished at: "
	@date

.include <bsd.subdir.mk>
