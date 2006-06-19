# $NetBSD: Makefile,v 1.57 2006/06/19 15:44:36 gdamore Exp $

.include <bsd.own.mk>

# if you're adding new scripts, don't forget to update followings.
#	src/distrib/sets/lists/etc/mi
#	src/etc/mtree/special
#	src/usr.sbin/postinstall/postinstall

CONFIGFILES=\
		DAEMON LOGIN NETWORKING SERVERS \
		accounting altqd amd apmd \
		bootconf.sh bootparams btconfig bthcid \
		ccd cgd cleartmp cron \
		dhclient dhcpd dhcrelay dmesg downinterfaces \
		fixsb fsck ftpd \
		hostapd \
		identd ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec \
		irdaattach iscsi_target isdnd \
		kdc \
		ldconfig lkm1 lkm2 lkm3 local lpd \
		mixerctl mopd motd mountall mountcritlocal mountcritremote \
		mountd moused mrouted \
		named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
		pf pf_boot pflogd poffd postfix powerd ppp pwcheck \
		quota \
		racoon rpcbind raidframe raidframeparity rarpd rbootd root \
		route6d routed rtadvd rtclocaltime rtsold rwho \
		savecore screenblank sdpd securelevel sshd \
		staticroute swap1 swap2 sysctl sysdb syslogd \
		timed tpctl ttys \
		veriexec virecover wdogctl wscons wsmoused \
		xdm xfs \
		ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
