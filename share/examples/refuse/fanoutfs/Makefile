# $NetBSD: Makefile,v 1.1 2007/03/31 21:05:56 agc Exp $

PROG=fanoutfs
LDADD+= -lrefuse
CPPFLAGS+= -g 
NOMAN=yes
WARNS=4

.include <bsd.prog.mk>

test:
	echo "# New fanout configuration at `date`" > fan.conf; 	\
	for i in 1 2 3 4 5 6 7 8; do					\
		mkdir dir$$i;						\
		cp defs.h dir$$i;					\
		echo dir$$i >> fan.conf;				\
	done;								\
	cp Makefile dir4;						\
	cp fanoutfs.c dir7;						\
	priv ${.CURDIR}/${PROG} -f fan.conf /mnt;			\
	sleep 1;							\
	ls -la /mnt;							\
	sleep 1;							\
	priv pkill fanoutfs;						\
	priv rm -rf dir? fan.conf
