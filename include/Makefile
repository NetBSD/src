#	$NetBSD: Makefile,v 1.63.2.1 1996/11/06 00:14:08 lukem Exp $
#	@(#)Makefile	5.45.1.1 (Berkeley) 5/6/91

# Missing: mp.h

FILES=	a.out.h ar.h assert.h bitstring.h bm.h cpio.h ctype.h db.h dirent.h \
	disktab.h err.h fnmatch.h fstab.h fts.h glob.h grp.h hesiod.h ieeefp.h \
	iso646.h kvm.h langinfo.h limits.h locale.h malloc.h math.h \
	memory.h mpool.h ndbm.h netdb.h netgroup.h nlist.h nl_types.h \
	nsswitch.h paths.h pwd.h ranlib.h re_comp.h regex.h regexp.h resolv.h \
	rmt.h search.h setjmp.h sgtty.h signal.h stab.h stddef.h stdio.h \
	stdlib.h string.h stringlist.h strings.h struct.h sysexits.h tar.h \
	time.h ttyent.h tzfile.h unistd.h util.h utime.h utmp.h vis.h

.if (${MACHINE_ARCH} != "alpha")
FILES+=	dlfcn.h link.h
.endif

DIRS=	arpa protocols rpc rpcsvc

NOOBJ=	noobj

includes: ${SYS_INCLUDE}

includes:
	@echo installing ${FILES}
	@-for i in ${FILES}; do \
		cmp -s $$i ${DESTDIR}/usr/include/$$i || \
		    ${INSTALL} -c -m 444 $$i ${DESTDIR}/usr/include/$$i; \
	done
	@echo installing ${DIRS}
	@-for i in ${DIRS}; do \
		${INSTALL} -d -o ${BINOWN} -g ${BINGRP} -m 755 \
			${DESTDIR}/usr/include/$$i; \
		(cd $$i; for j in *.[ih]; do \
			cmp -s $$j ${DESTDIR}/usr/include/$$i/$$j || \
			${INSTALL} -c -m 444 $$j \
			${DESTDIR}/usr/include/$$i/$$j; \
		done); \
	done
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}/usr/include
	find ${DESTDIR}/usr/include -type f | \
		xargs chmod a=r
	find ${DESTDIR}/usr/include -type d | \
		xargs chmod u=rwx,go=rx

.include <bsd.prog.mk>
