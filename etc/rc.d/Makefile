# $NetBSD: Makefile,v 1.33 2002/08/27 14:12:14 takemura Exp $

.include <bsd.own.mk>

# if you're adding new scripts, don't forget to update
# src/etc/postinstall and src/etc/mtree/special.

FILES=	DAEMON LOGIN NETWORKING SERVERS \
	accounting altqd amd apmd \
	bootparams bootconf.sh ccd cleartmp cron \
	dhclient dhcpd dhcrelay dmesg downinterfaces fsck \
	ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec isdnd kdc \
	ldconfig lkm1 lkm2 lkm3 local lpd \
	mixerctl mopd motd mountall mountcritlocal mountcritremote mountd \
	moused mrouted \
	named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
	poffd postfix ppp pwcheck quota \
	racoon rpcbind raidframe rarpd rbootd root \
	route6d routed rtadvd rtsold rwho \
	savecore screenblank sendmail securelevel sshd \
	swap1 swap2 sysdb sysctl syslogd \
	timed tpctl ttys virecover wdogctl wscons wsmoused xdm xfs \
	ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
