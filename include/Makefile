#	from: @(#)Makefile	5.45.1.1 (Berkeley) 5/6/91
#	$Id: Makefile,v 1.24 1993/10/01 00:22:00 jtc Exp $

# Doing a make install builds /usr/include
#
# The ``rm -rf''s used below are safe because rm doesn't follow symbolic
# links.

SUBDIR= rpcsvc

# Missing: mp.h

FILES=	a.out.h ar.h assert.h bitstring.h ctype.h db.h dirent.h disktab.h \
	err.h fnmatch.h fstab.h fts.h glob.h grp.h kvm.h limits.h locale.h \
	math.h memory.h mpool.h ndbm.h netdb.h nlist.h paths.h pwd.h ranlib.h \
	regexp.h resolv.h setjmp.h sgtty.h stab.h stdarg.h stddef.h stdio.h \
	stdlib.h string.h strings.h struct.h sysexits.h time.h ttyent.h \
	tzfile.h unistd.h utime.h utmp.h varargs.h vis.h malloc.h

MFILES=	float.h frame.h
LFILES=	errno.h fcntl.h signal.h syslog.h termios.h

DIRS=	arpa protocols rpc
LDIRS=	net netinet netccitt netiso netns nfs sys ufs vm 

NOOBJ=	noobj

# Change SYS_INCLUDE in bsd.own.mk to "symlinks" if you don't want copies
.if !defined(SYS_INCLUDE)
SYS_INCLUDE=	copies
.endif

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
	tar cf - *.h | (cd ${DESTDIR}/usr/include/machine; tar xpfB -); \

symlinks:
	@echo symlinks: ${LDIRS}
	@for i in ${LDIRS}; do \
		rm -rf ${DESTDIR}/usr/include/$$i; \
		ln -s /sys/$$i ${DESTDIR}/usr/include/$$i; \
	done
	rm -rf ${DESTDIR}/usr/include/machine
	ln -s /sys/arch/${MACHINE}/include ${DESTDIR}/usr/include/machine

.include <bsd.prog.mk>

realinstall: ${SYS_INCLUDE}
