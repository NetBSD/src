#	from: @(#)Makefile	5.45.1.1 (Berkeley) 5/6/91
#	$Id: Makefile,v 1.31 1994/01/08 12:22:47 cgd Exp $

# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.

# Missing: mp.h

FILES=	a.out.h ar.h assert.h bitstring.h ctype.h db.h dirent.h disktab.h \
	err.h fnmatch.h fstab.h fts.h glob.h grp.h kvm.h limits.h locale.h \
	math.h memory.h mpool.h ndbm.h netdb.h nlist.h paths.h pwd.h ranlib.h \
	regex.h resolv.h search.h setjmp.h sgtty.h stab.h stdarg.h stddef.h \
	stdio.h stdlib.h string.h strings.h struct.h sysexits.h time.h \
	ttyent.h tzfile.h unistd.h utime.h utmp.h varargs.h vis.h malloc.h \
	link.h

MFILES=	float.h frame.h
LFILES=	errno.h fcntl.h signal.h syslog.h termios.h

DIRS=	arpa protocols rpc rpcsvc
LDIRS=	dev net netinet netccitt netiso netns nfs sys ufs vm 

NOOBJ=	noobj

# Change SYS_INCLUDE in bsd.own.mk to "symlinks" if you don't want copies
.include <bsd.own.mk>
SYS_INCLUDE?=	copies

realinstall:
	@echo installing ${FILES}
	@-for i in ${FILES}; do \
		cmp -s $$i ${DESTDIR}/usr/include/$$i || \
		    install -c -m 644 $$i ${DESTDIR}/usr/include/$$i; \
	done
	@echo installing ${DIRS}
	@-for i in ${DIRS}; do \
		if [ ! -d ${DESTDIR}/usr/include/$$i ]; \
		then \
			mkdir ${DESTDIR}/usr/include/$$i; \
		fi; \
		(cd $$i; for j in *.[ih]; do \
			cmp -s $$j ${DESTDIR}/usr/include/$$i/$$j || \
			install -c -m 644 $$j ${DESTDIR}/usr/include/$$i/$$j; \
		done); \
	done
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
	@chown -R ${BINOWN}:${BINGRP} ${DESTDIR}/usr/include
	@chmod -R a-w ${DESTDIR}/usr/include

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}/usr/include

copies:
	@echo copies: ${LDIRS}
	@-for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		cd ../sys; \
		tar cf - $$i/*.h | (cd ${DESTDIR}/usr/include; tar xpfB -); \
	done
	rm -rf ${DESTDIR}/usr/include/machine
	mkdir ${DESTDIR}/usr/include/machine
	cd ../sys/arch/${MACHINE}/include; \
	    tar cf - *.h | (cd ${DESTDIR}/usr/include/machine; tar xpfB -);
	rm -rf ${DESTDIR}/usr/include/${MACHINE_ARCH}
	if test ${MACHINE} != ${MACHINE_ARCH} ; then \
		mkdir ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
		cd ../sys/arch/${MACHINE_ARCH}/include ; \
		tar cf - *.h | \
		    (cd ${DESTDIR}/usr/include/${MACHINE_ARCH} ; tar xpfB -); \
	else \
		ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
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
	if test ${MACHINE} != ${MACHINE_ARCH} ; then \
		ln -s /sys/arch/${MACHINE_ARCH}/include \
		    ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
	else \
		ln -s machine ${DESTDIR}/usr/include/${MACHINE_ARCH} ; \
	fi

.include <bsd.prog.mk>

realinstall: ${SYS_INCLUDE}
