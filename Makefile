#	$NetBSD: Makefile,v 1.62 1998/07/28 18:55:41 thorpej Exp $

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
.ifndef NOMAN
	(cd ${.CURDIR}/share/man && ${MAKE} makedb)
.endif

build: beforeinstall
	(cd ${.CURDIR}/share/mk && ${MAKE} install)
	(cd ${.CURDIR}/share/tmac && ${MAKE} && ${MAKE} install)
.if !defined(UPDATE)
	${MAKE} cleandir
.endif
	${MAKE} includes
	(cd ${.CURDIR}/lib/csu && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
	(cd ${.CURDIR}/lib && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
	(cd ${.CURDIR}/gnu/lib && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.if defined(USE_EGCS)
	(cd ${.CURDIR}/gnu/usr.bin/egcs/libgcc && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.else
.if	(${MACHINE_ARCH} != "alpha") && \
	(${MACHINE_ARCH} != "powerpc")
	(cd ${.CURDIR}/gnu/usr.bin/gcc/libgcc && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.endif
.endif
.if exists(domestic) && !defined(EXPORTABLE_SYSTEM)
# libtelnet depends on libdes and libkrb.  libkrb depends on
# libcom_err.
.if exists(domestic/lib/libdes)
	(cd ${.CURDIR}/domestic/lib/libdes && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.endif
.if exists(domestic/lib/libcom_err)
	(cd ${.CURDIR}/domestic/lib/libcom_err && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.endif
.if exists(domestic/lib/libkrb)
	(cd ${.CURDIR}/domestic/lib/libkrb && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.endif
	(cd ${.CURDIR}/domestic/lib && \
	    ${MAKE} depend && NOMAN= ${MAKE} && NOMAN= ${MAKE} install)
.endif
	${MAKE} depend && ${MAKE} && ${MAKE} install
	@echo -n "Build finished at: "
	@date

.include <bsd.subdir.mk>
