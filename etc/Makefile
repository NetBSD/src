#	$NetBSD: Makefile,v 1.73 1997/07/30 22:17:55 perry Exp $
#	from: @(#)Makefile	8.7 (Berkeley) 5/25/95

TZDIR=		/usr/share/zoneinfo
LOCALTIME=	US/Pacific

# setting NOOBJ prevents "make obj" from doing anything;
# an objdir would break the installation stuff below
NOOBJ=	oobj

.if exists(etc.${MACHINE}/Makefile.inc)
.include "etc.${MACHINE}/Makefile.inc"
.endif

# -rw-r--r--
BINOWN= root
BINGRP= wheel
BIN1=	aliases bootptab changelist csh.cshrc csh.login csh.logout daily \
	daily.conf dm.conf floppytab ftpchroot ftpusers ftpwelcome \
	gettytab group hosts hosts.equiv hosts.lpd inetd.conf lkm.conf \
	man.conf monthly monthly.conf mrouted.conf myname netstart networks \
	newsyslog.conf phones printcap protocols profile rbootd.conf rc \
	rc.conf rc.lkm rc.local remote rpc security security.conf services \
	shells syslog.conf weekly weekly.conf etc.${MACHINE}/ttys \
	etc.${MACHINE}/disktab

# -rw-rw-r--
BIN2=	motd

NAMEDB=	localhost.rev named.boot root.cache
PCS=	pcs750.bin

all clean cleandir depend etc includes install lint:

.ifndef DESTDIR
distribution distrib-dirs snapshot:
	@echo setenv DESTDIR before doing that!
	@false
.else
distribution: distrib-dirs
	(cd ..; ${MAKE} includes)
	(cd ..; ${MAKE} install)
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${BIN1} ${DESTDIR}/etc
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 ${BIN2} ${DESTDIR}/etc
	${INSTALL} -c -o root -g wheel -m 600 crontab \
	    ${DESTDIR}/var/cron/tabs/root
	${INSTALL} -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	pwd_mkdb -p -d ${DESTDIR}/ ${DESTDIR}/etc/master.passwd
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 555 \
	     MAKEDEV.local etc.${MACHINE}/MAKEDEV ${DESTDIR}/dev
	${INSTALL} -c -o root -g wheel -m 600 minfree \
	    ${DESTDIR}/var/crash
	(cd root; \
		${INSTALL} -c -o root -g wheel -m 644 dot.cshrc \
		    ${DESTDIR}/root/.cshrc; \
		${INSTALL} -c -o root -g wheel -m 600 dot.klogin \
		    ${DESTDIR}/root/.klogin; \
		${INSTALL} -c -o root -g wheel -m 644 dot.login \
		    ${DESTDIR}/root/.login; \
		${INSTALL} -c -o root -g wheel -m 644 dot.profile \
		    ${DESTDIR}/root/.profile; \
		rm -f ${DESTDIR}/.cshrc ${DESTDIR}/.profile; \
		ln ${DESTDIR}/root/.cshrc ${DESTDIR}/.cshrc; \
		ln ${DESTDIR}/root/.profile ${DESTDIR}/.profile)
	(cd mtree; \
		${INSTALL} -c -o root -g wheel -m 600 special \
		    ${DESTDIR}/etc/mtree; \
		${INSTALL} -c -o root -g wheel -m 444 NetBSD.dist \
		    ${DESTDIR}/etc/mtree)
	(cd namedb; \
		${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 ${NAMEDB} \
		    ${DESTDIR}/etc/namedb)
	/bin/rm -f ${DESTDIR}/etc/localtime
	ln -s ${TZDIR}/${LOCALTIME} ${DESTDIR}/etc/localtime
	/bin/rm -f ${DESTDIR}/etc/rmt
	ln -s /usr/sbin/rmt ${DESTDIR}/etc/rmt
	${INSTALL} -c -o ${BINOWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	${INSTALL} -c -o ${BINOWN} -g operator -m 644 /dev/null \
		${DESTDIR}/etc/skeykeys
	${INSTALL} -c -o root -g wheel -m 600 /dev/null \
		${DESTDIR}/var/cron/log
	${INSTALL} -c -o nobody -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/db/locate.database
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/authlog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/log/lastlog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 640 /dev/null \
		${DESTDIR}/var/log/lpd-errs
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/maillog
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/messages
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/secure
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
		${DESTDIR}/var/log/wtmp
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 /dev/null \
		${DESTDIR}/var/log/xferlog
	${INSTALL} -c -o daemon -g staff -m 664 /dev/null \
		${DESTDIR}/var/msgs/bounds
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 664 /dev/null \
		${DESTDIR}/var/run/utmp
	(cd etc.${MACHINE}; ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    fstab.* ${DESTDIR}/etc)
	(cd ${DESTDIR}/dev; ./MAKEDEV all)
	(cd ../usr.bin/mail; ${MAKE} distribution)
	(cd ../usr.sbin/sendmail/cf/cf; ${MAKE} distribution)


distrib-dirs:
	${INSTALL} -d -o root -g wheel -m 755 ${DESTDIR}
	-mtree -def mtree/NetBSD.dist -p ${DESTDIR}/ -u
	cd ${DESTDIR}; rm -f sys; ln -s usr/src/sys sys

snapshot: distribution snap_pre snap_tar snap_md
	cd ${DESTDIR}/snapshot && cksum * > CKSUMS

snap_pre:
	/bin/rm -rf ${DESTDIR}/snapshot
	${INSTALL} -d -o root -g wheel -m 755 ${DESTDIR}/snapshot

snap_tar:
	cd ${DESTDIR} && tar cf - bin | gzip -9 > snapshot/bin.tar.gz
	cd ${DESTDIR} && tar cf - dev | gzip -9 > snapshot/dev.tar.gz
	cd ${DESTDIR} && tar cf - .profile .cshrc etc mnt root sys tmp | \
	    gzip -9 > snapshot/etc.tar.gz
	cd ${DESTDIR} && tar cf - sbin | gzip -9 > snapshot/sbin.tar.gz
	cd ${DESTDIR} && tar cf - usr/bin | gzip -9 > snapshot/usr.bin.tar.gz
	cd ${DESTDIR} && tar cf - usr/games | gzip -9 > \
	    snapshot/usr.games.tar.gz
	cd ${DESTDIR} && tar cf - usr/include | gzip -9 > \
	    snapshot/usr.include.tar.gz
	cd ${DESTDIR} && tar cf - usr/lib | gzip -9 > snapshot/usr.lib.tar.gz
	cd ${DESTDIR} && tar cf - usr/libexec | gzip -9 > \
	    snapshot/usr.libexec.tar.gz
	cd ${DESTDIR} && tar cf - usr/mdec usr/libdata usr/local usr/src \
	    usr/obj | gzip -9 > snapshot/usr.misc.tar.gz
	cd ${DESTDIR} && tar cf - usr/sbin | gzip -9 > snapshot/usr.sbin.tar.gz
	cd ${DESTDIR} && tar cf - usr/share | gzip -9 > \
	    snapshot/usr.share.tar.gz
	cd ${DESTDIR} && tar cf - var | gzip -9 > snapshot/var.tar.gz

snap_md:
# nothing here -- look in the machine-dependent Makefile.inc

.endif	# DESTDIR check

.include <bsd.prog.mk>
