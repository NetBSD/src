# $NetBSD: Makefile,v 1.68.2.4 2009/05/10 21:32:57 snj Exp $

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
		dhclient dhcpcd dhcpd dhcrelay dmesg downinterfaces envsys \
		fsck ftp_proxy ftpd \
		hostapd httpd \
		identd ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec \
		irdaattach iscsi_target isdnd \
		kdc \
		ldconfig lkm1 lkm2 lkm3 local lpd \
		mixerctl mopd motd mountall mountcritlocal mountcritremote \
		mountd moused mrouted \
		named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
		perusertmp pf pf_boot pflogd poffd postfix powerd ppp pwcheck \
		quota \
		racoon rpcbind raidframe raidframeparity rarpd rbootd rndctl \
		root route6d routed rtadvd rtclocaltime rtsold rwho \
		savecore screenblank sdpd securelevel sshd \
		staticroute swap1 swap2 sysctl sysdb syslogd \
		timed tpctl ttys \
		veriexec virecover wdogctl wpa_supplicant wscons wsmoused \
		ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.if ${MKX11} != "no"
CONFIGFILES+=	xdm xfs
FILESBUILD_xdm=	yes
FILESBUILD_xfs=	yes

.SUFFIXES: .in
.in:
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,@X11ROOTDIR@,${X11ROOTDIR},g' < ${.IMPSRC} > ${.TARGET}
.endif

.include <bsd.prog.mk>
