#	from: @(#)Makefile	5.11 (Berkeley) 5/21/91
#	$Id: Makefile,v 1.28 1994/01/07 23:55:00 jtc Exp $

TZDIR=		/usr/share/zoneinfo
LOCALTIME=	US/Pacific

NOOBJ=	oobj

# disktab may be wrong -- hcx9 is a tahoe, but gets its own.
# -rw-r--r--
BINOWN= root
BINGRP= wheel
BIN1=	aliases csh.cshrc csh.login csh.logout dm.conf \
	ftpusers gettytab group hosts hosts.equiv hosts.lpd inetd.conf \
	man.conf motd myname netstart newsyslog.conf phones \
	printcap protocols rc rc.local remote security services shells \
	syslog.conf ttys etc.${MACHINE}/disktab rpc

# -rw-rw-rw-
BIN2=	motd

# -rwxr-xr-x root.wheel, for the new f***ing cron root.wheel
BIN3=	daily weekly monthly

MTREE=	BSD.root.dist BSD.usr.dist BSD.var.dist
NAMEDB=	localhost.rev named.boot root.cache
PCS=	pcs750.bin
WCS1=	wcs fppwcs poc poc1 poc2 fppoc
WCS2=	fpevent fppwcs fppwcs_dual hdcwcs load_diags start_fpp wcs wcs_dual

all clean cleandir depend etc install lint:

distribution: distrib-dirs
	install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} ${DESTDIR}/etc
	install -c -o ${BINOWN} -g ${BINGRP} -m 666 ${BIN2} ${DESTDIR}/etc
	install -c -o root -g wheel -m 755 ${BIN3} ${DESTDIR}/etc
	install -c -o root -g wheel -m 600 crontab ${DESTDIR}/var/cron/tabs/root
	install -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	(cd ${DESTDIR}/etc; pwd_mkdb -p master.passwd)
	install -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	(cd root; \
		install -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		install -c -o root -g wheel -m 644 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		install -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		install -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	cd mtree; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${MTREE} \
	    ${DESTDIR}/etc/mtree
	cd namedb; install -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
	    ${DESTDIR}/etc/namedb
	/bin/rm -f ${DESTDIR}/etc/localtime
	ln -s ${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	install -c -o ${BINOWN} -g operator -m 664 /dev/null \
	    ${DESTDIR}/etc/dumpdates
	install -c -o nobody -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/db/locate.database
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/messages
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/maillog
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/log/lpd-errs
	install -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
	    ${DESTDIR}/var/run/utmp
	(cd etc.${MACHINE}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    fstab.* ${DESTDIR}/etc)
.if ${MACHINE} == "tahoe"
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS1} \
	    ${DESTDIR}/)
.endif
.if ${MACHINE} == "vax"
	(cd etc.vax; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${PCS} \
	    ${DESTDIR}/)
.endif

hcx9-distribution:
	(cd etc.tahoe; install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${WCS2} \
	    ${DESTDIR}/)

distrib-dirs:
	mtree -u -d -p ${DESTDIR} -f ${.CURDIR}/mtree/BSD.root.dist
#	mtree -u -d -p ${DESTDIR}/usr -f ${.CURDIR}/mtree/BSD.usr.dist
#	mtree -u -d -p ${DESTDIR}/var -f ${.CURDIR}/mtree/BSD.var.dist
	-mkdir ${DESTDIR}/var/at ${DESTDIR}/var/backups ${DESTDIR}/var/cron \
	       ${DESTDIR}/var/db ${DESTDIR}/var/log ${DESTDIR}/var/mail \
	       ${DESTDIR}/var/msgs ${DESTDIR}/var/preserve ${DESTDIR}/var/run \
	       ${DESTDIR}/var/rwho ${DESTDIR}/var/spool ${DESTDIR}/var/tmp
	chmod 755 ${DESTDIR}/var/at ${DESTDIR}/var/backups ${DESTDIR}/var/cron \
		  ${DESTDIR}/var/db ${DESTDIR}/var/log ${DESTDIR}/var/mail \
		  ${DESTDIR}/var/msgs ${DESTDIR}/var/preserve \
		  ${DESTDIR}/var/run ${DESTDIR}/var/rwho ${DESTDIR}/var/spool
	chmod 1777 ${DESTDIR}/var/tmp
	-mkdir ${DESTDIR}/var/cron/tabs \
	       ${DESTDIR}/var/at/jobs ${DESTDIR}/var/at/spool
	chmod 700 ${DESTDIR}/var/cron/tabs \
		  ${DESTDIR}/var/at/jobs ${DESTDIR}/var/at/spool
	-mkdir ${DESTDIR}/var/spool/lock ${DESTDIR}/var/spool/mqueue \
	       ${DESTDIR}/var/spool/output ${DESTDIR}/var/spool/uucp \
	       ${DESTDIR}/var/spool/uucppublic
	chown uucp.dialer ${DESTDIR}/var/spool/lock
	chmod 755 ${DESTDIR}/var/spool/lock ${DESTDIR}/var/spool/mqueue \
		  ${DESTDIR}/var/spool/output ${DESTDIR}/var/spool/uucp \
		  ${DESTDIR}/var/spool/uucppublic
	cd ${DESTDIR}; rm -f sys; ln -s usr/src/sys sys
	-mkdir ${DESTDIR}/usr/obj
	chmod 755 ${DESTDIR}/usr/obj

.include <bsd.prog.mk>
