#	$NetBSD: Makefile,v 1.13 1996/03/11 04:58:22 thorpej Exp $

SUBDIR= libg++ libstdc++ # libio libiostream

beforeinstall:
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++/gen
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++/std
	install -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/g++/stl
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
	@echo installing includes from libstdc++/std
	@(cd libstdc++/std ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++/std/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/std/$$j; \
	done)
	@echo installing includes from libstdc++/stl
	@(cd libstdc++/stl ; for j in *.h; do \
		cmp -s $$j ${DESTDIR}/usr/include/g++stl/$$j || \
		install -c -o ${BINOWN} -g ${BINGRP} -m 444 $$j \
			${DESTDIR}/usr/include/g++/stl/$$j; \
	done)

.include <bsd.subdir.mk>
