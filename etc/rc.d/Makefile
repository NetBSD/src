# $NetBSD: Makefile,v 1.27 2002/02/11 14:00:35 lukem Exp $

NOPROG= noprog

.include <bsd.own.mk>

FILES=	DAEMON LOGIN NETWORKING SERVERS \
	accounting altqd amd apmd \
	bootparams bootconf.sh ccd cleartmp cron \
	dhclient dhcpd dhcrelay dmesg downinterfaces fsck \
	ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec isdnd kdc \
	ldconfig lkm1 lkm2 lkm3 local lpd \
	mopd motd mountall mountcritlocal mountcritremote mountd \
	moused mrouted \
	named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
	poffd postfix ppp pwcheck quota \
	racoon rpcbind raidframe rarpd rbootd root \
	route6d routed rtadvd rtsold rwho \
	savecore screenblank sendmail securelevel sshd \
	swap1 swap2 sysdb sysctl syslogd \
	timed ttys virecover wscons xdm xfs ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
