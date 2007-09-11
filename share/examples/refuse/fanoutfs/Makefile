# $NetBSD: Makefile,v 1.2 2007/09/11 23:23:51 agc Exp $

PROG=fanoutfs
LDADD+= -lrefuse
CPPFLAGS+= -g 
MAN=fanoutfs.8
WARNS=4

.include <bsd.prog.mk>

.include <bsd.own.mk>

FANOUTOBJDIR!=cd ${.CURDIR} && ${PRINTOBJDIR}

test:
	echo "# New fanout configuration at `date`" > fan.conf; 	\
	for i in 1 2 3 4 5 6 7 8; do					\
		mkdir dir$$i;						\
		cp ${.CURDIR}/defs.h dir$$i/defs.h;			\
		cp ${.CURDIR}/defs.h dir$$i/defs.h.$$i;			\
		echo dir$$i >> fan.conf;				\
	done;								\
	cp ${.CURDIR}/Makefile dir4;					\
	cp ${.CURDIR}/fanoutfs.c dir7;					\
	priv ${FANOUTOBJDIR}/${PROG} -f fan.conf /mnt;			\
	sleep 1;							\
	ls -la /mnt;							\
	sleep 1;							\
	priv pkill fanoutfs;						\
	priv rm -rf dir? fan.conf
