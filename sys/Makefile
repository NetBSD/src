#	$NetBSD: Makefile,v 1.13 1997/03/17 08:27:54 veego Exp $

# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.

SUBDIR+= arch/${MACHINE}

.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips")
SUBDIR+= lkm
.endif

LFILES=	errno.h fcntl.h poll.h syslog.h termios.h
MFILES=	float.h frame.h stdarg.h varargs.h
LDIRS=	dev net netinet netccitt netiso netns netnatm nfs sys ufs vm 

# Change SYS_INCLUDE in bsd.own.mk or /etc/mk.conf to "symlinks" if you
# don't want copies
.include <bsd.own.mk>
SYS_INCLUDE?=	copies

includes: ${SYS_INCLUDE}
	@echo installing ${LFILES}
	@-for i in ${LFILES}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	@echo installing ${MFILES}
	@-for i in ${MFILES}; do \
		rm -f ${DESTDIR}/usr/include/$$i; \
		ln -s machine/$$i ${DESTDIR}/usr/include/$$i; \
	done

copies:
	@echo copies: ${LDIRS}
	@-for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
			${DESTDIR}/usr/include/$$i ; \
	done
	pax -rw -pa -L \
	    `find ${LDIRS} -follow -type f -name '*.h' '!' -path \
	    'netiso/xebec/*' -print` ${DESTDIR}/usr/include
	rm -rf ${DESTDIR}/usr/include/machine
	${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/machine
	pax -rw -pa -s "|arch/${MACHINE}/include||" \
	    arch/${MACHINE}/include/*.h \
	    ${DESTDIR}/usr/include/machine   
	rm -rf ${DESTDIR}/usr/include/${MACHINE_ARCH}
	if test ${MACHINE} != ${MACHINE_ARCH} -a \
	    -d arch/${MACHINE_ARCH}/include; then \
		${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
		${DESTDIR}/usr/include/${MACHINE_ARCH}; \
		pax -rw -pa -s "|arch/${MACHINE_ARCH}/include||" \
		    arch/${MACHINE_ARCH}/include/*.h \
		    ${DESTDIR}/usr/include/${MACHINE_ARCH}; \
	else \
		ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH}; \
	fi

symlinks:
	@echo symlinks: ${LDIRS}
	@for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		ln -s /sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	rm -rf ${DESTDIR}/usr/include/machine
	ln -s /sys/arch/${MACHINE}/include ${DESTDIR}/usr/include/machine
	rm -rf ${DESTDIR}/usr/include/${MACHINE_ARCH}
	if test ${MACHINE} != ${MACHINE_ARCH} -a \
	    -d arch/${MACHINE_ARCH}/include ; then \
		ln -s /sys/arch/${MACHINE_ARCH}/include \
		    ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
	else \
		ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
	fi

.include <bsd.subdir.mk>
