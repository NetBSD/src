#	$NetBSD: Makefile,v 1.18 1996/05/16 21:03:24 phil Exp $

SUBDIR= libg++ libstdc++ # libio libiostream

HEADERS= cassert cctype cerrno cfloat ciso646 climits clocale cmath complex \
         csetjmp csignal cstdarg cstddef cstdio cstdlib cstring ctime \
         cwchar cwctype new stddef string exception stdexcept typeinfo \
         algorithm deque list map queue set stack vector utility functional \
         iterator memory numeric


includes:
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++/gen
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++/std
	@echo installing includes from libg++/src
	@(cd libg++/src ; for j in *.[ih]; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from include/gen
	@(cd include/gen ; for j in *.*P; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/gen/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/gen/$$j; \
	done)
	@echo installing includes from libio
	@(cd libio ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from libstdc++/std
	@(cd libstdc++/std ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/std/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/std/$$j; \
	done)
	@echo installing includes from libstdc++/stl
	@(cd libstdc++/stl ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)
	@echo installing includes from libstdc++
	@(cd libstdc++ ; for j in *.h ${HEADERS}; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/$$j; \
	done)

.include <bsd.subdir.mk>
