#	$NetBSD: Makefile,v 1.7 2001/03/31 14:11:12 itojun Exp $
#
#	@(#)Makefile	8.29 (Berkeley) 10/19/96

CAT=		dutch english french german ru_RU.KOI8-R spanish swedish
VI_FILES=	${.CURDIR}/../cl/*.c ${.CURDIR}/../common/*.c \
		${.CURDIR}/../ex/*.c ${.CURDIR}/../vi/*.c
FILES=		${CAT}
FILESDIR=	/usr/share/vi/catalog

HOSTPROG=	dump
SRCS=		dump.c
MKMAN=		no

CLEANFILES+=	${CAT} english.base *.check __ck1 __ck2

realall: ${CAT}

${CAT}: english.base ${HOSTPROG}
	@echo "... $@";							\
	rm -f $@;							\
	if test -f $@.base; then					\
		f=$@.base;						\
	else								\
		f=${.CURDIR}/$@.base;					\
	fi;								\
	sort -u $$f |							\
	awk '{								\
		if ($$1 == 1) {						\
			print "\nMESSAGE NUMBER 1 IS NOT LEGAL";	\
			exit 1;						\
		}							\
		if (++nline > $$1) {					\
			print "DUPLICATE MESSAGE NUMBER " $$1;		\
			exit 1;						\
		}							\
		for (; nline < $$1; ++nline)				\
			print "";					\
		print $0;						\
	}' |								\
	sed -e '1s/^/VI_MESSAGE_CATALOG/'				\
	    -e '/"/s/^[^"]*"//'						\
	    -e '1!s/"$$/X/' > $@;					\
	chmod 444 $@;							\
	if grep DUPLICATE $@ > /dev/null; then				\
		grep DUPLICATE $@;					\
	fi;								\
	if grep 'NOT LEGAL' $@ > /dev/null; then			\
		grep 'NOT LEGAL' $@;					\
	fi

CHK=	dutch.check english.check french.check german.check \
	ru_RU.KOI8-R.check spanish.check swedish.check
check: ${CHK}
${CHK}: ${CAT}
	@echo "... $@";							\
	f=`basename $@ .check`;						\
	(echo "Unused message id's (this is okay):";			\
	awk '{								\
		while (++nline < $$1)					\
			printf "%03d\n", nline;				\
	}' < ${.CURDIR}/$$f.base;					\
	echo =========================;					\
	echo "MISSING ERROR MESSAGES (Please add!):";			\
	awk '{print $$1}' < ${.CURDIR}/$$f.base | sort -u > __ck1;	\
	awk '{print $$1}' < english.base | sort -u > __ck2;		\
	comm -13 __ck1 __ck2;						\
	echo =========================;					\
	echo "Extra error messages (just delete them):";		\
	comm -23 __ck1 __ck2;						\
	echo =========================;					\
	echo "MESSAGES WITH THE SAME MESSAGE ID's (FIX!):";		\
	for j in							\
	    `sed '/^$$/d' < $$f.base | sort -u |			\
	    awk '{print $$1}' | uniq -d`; do				\
		egrep $$j $$f.base;					\
	done;								\
	echo =========================;					\
	echo "Duplicate messages, both id and message (this is okay):";	\
	sed '/^$$/d' < $$f.base | sort | uniq -c |			\
	awk '$$1 != 1 { print $$0 }' | sort -n;				\
	echo =========================;					\
	echo "Duplicate messages, just message (this is okay):";	\
	sed '/^$$/d' < $$f | sort | uniq -c |				\
	awk '$$1 != 1 { print $$0 }' | sort -n;				\
	echo =========================) > $@

english.base: dump ${VI_FILES} #Makefile
	./dump ${VI_FILES} |\
	sed -e '/|/!d' \
	    -e 's/|/ "/' \
	    -e 's/^"//' \
	    -e 's/\\"/"/g' |\
	sort -n > $@

.include <bsd.hostprog.mk>
