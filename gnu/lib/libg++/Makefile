#	$NetBSD: Makefile,v 1.24 1997/05/31 21:21:19 cjs Exp $

SUBDIR= libg++ libstdc++ # libio libiostream

HEADERS= cassert cctype cerrno cfloat ciso646 climits clocale cmath complex \
         csetjmp csignal cstdarg cstddef cstdio cstdlib cstring ctime \
         cwchar cwctype new stddef string exception stdexcept typeinfo \
         algorithm deque list map queue set stack vector utility functional \
         iterator memory numeric


includes:
	@echo installing includes from libg++/src
	@(cd libg++/src ; for j in *.[ih]; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from include/gen
	@(cd include/gen ; for j in *.*P; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/gen/$$j || \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/gen/$$j; \
	done)
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

.include <bsd.subdir.mk>
