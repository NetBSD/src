# $NetBSD: Makefile,v 1.42 2003/12/11 09:46:27 dyoung Exp $

.include <bsd.own.mk>

# if you're adding new scripts, don't forget to update followings.
#	src/etc/postinstall
#	src/etc/mtree/special
#	src/distrib/sets/lists/etc/mi

FILES=	DAEMON LOGIN NETWORKING SERVERS \
	accounting altqd amd apmd \
	bootparams bootconf.sh ccd cgd cleartmp cron \
	dhclient dhcpd dhcrelay dmesg downinterfaces fsck \
	ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec isdnd \
	ldconfig lkm1 lkm2 lkm3 local lpd \
	mixerctl mopd motd mountall mountcritlocal mountcritremote mountd \
	moused mrouted \
	named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
	poffd postfix powerd ppp pwcheck quota \
	racoon rpcbind raidframe raidframeparity rarpd rbootd root \
	route6d routed rtadvd rtsold rwho \
	savecore screenblank sendmail securelevel smmsp sshd \
	staticroute swap1 swap2 sysdb sysctl syslogd \
	timed tpctl ttys virecover wdogctl wscons wsmoused xdm xfs \
	ypbind yppasswdd ypserv
.if (${MKKERBEROS} != "no")
FILES+=	kdc
.endif
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
