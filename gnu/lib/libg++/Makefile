#	$NetBSD: Makefile,v 1.26 1998/10/14 15:05:30 tv Exp $

HAVE_GCC28!= ${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if empty(HAVE_GCC28)

SUBDIR= libstdc++

HEADERS= cassert cctype cerrno cfloat ciso646 climits clocale cmath complex \
         csetjmp csignal cstdarg cstddef cstdio cstdlib cstring ctime \
         cwchar cwctype new stddef string exception stdexcept typeinfo \
         algorithm deque list map queue set stack vector utility functional \
         iterator memory numeric

INCS=    _G_config.h
INCSDIR= /usr/include/${MACHINE}

incinstall:: other-includes
other-includes:
	@echo installing includes from libio
	@(cd libio ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from libstdc++/std
	@(cd libstdc++/std ; for j in *.h *.cc; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/std/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/std/$$j; \
	done)
	@echo installing includes from libstdc++/stl
	@(cd libstdc++/stl ; for j in *.h *.cc; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from libstdc++
	@(cd libstdc++ ; for j in *.h ${HEADERS}; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)

.include <bsd.inc.mk>
.include <bsd.subdir.mk>

.else
.include <bsd.own.mk>
${TARGETS}:
.endif
