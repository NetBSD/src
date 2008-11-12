# $NetBSD: Makefile,v 1.69 2008/11/12 12:35:52 ad Exp $

.include <bsd.own.mk>

# if you're adding new scripts, don't forget to update followings.
#	src/distrib/sets/lists/etc/mi
#	src/etc/mtree/special
#	src/usr.sbin/postinstall/postinstall

CONFIGFILES=\
		DAEMON LOGIN NETWORKING SERVERS \
		accounting altqd amd apmd \
		bootconf.sh bootparams btattach btconfig btdevctl bthcid \
		ccd cgd cleartmp cron \
		dhclient dhcpd dhcrelay dmesg downinterfaces envsys \
		fsck ftp_proxy ftpd \
		hostapd \
		identd ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec \
		irdaattach iscsi_target isdnd \
		kdc \
		ldconfig local lpd \
		mixerctl mopd motd mountall mountcritlocal mountcritremote \
		mountd moused mrouted \
		named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
		perusertmp pf pf_boot pflogd poffd postfix powerd ppp pwcheck \
		quota \
		racoon rpcbind raidframe raidframeparity rarpd rbootd root \
		route6d routed rtadvd rtclocaltime rtsold rwho \
		savecore screenblank sdpd securelevel sshd \
		staticroute swap1 swap2 sysctl sysdb syslogd \
		timed tpctl ttys \
		veriexec virecover wdogctl wpa_supplicant wscons wsmoused \
		xdm xfs \
		ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
