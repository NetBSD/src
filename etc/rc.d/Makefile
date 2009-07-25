# $NetBSD: Makefile,v 1.79 2009/07/25 16:20:10 mbalmer Exp $

.include <bsd.own.mk>

#
# NOTE:
#
# If you're adding new scripts, don't forget to update following:
#	src/distrib/sets/lists/etc/mi
#	src/etc/mtree/special
#	src/usr.sbin/postinstall/postinstall
#
# You can find a script to visualize the dependency graph in
# src/sbin/rcorder.

CONFIGFILES=\
		DAEMON DISKS LOGIN NETWORKING SERVERS \
		accounting altqd amd apmd \
		bootconf.sh bootparams btattach btconfig btdevctl bthcid \
		ccd cgd cleartmp cron \
		dhclient dhcpcd dhcpd dhcrelay dmesg downinterfaces envsys \
		fsck fsck_root ftp_proxy ftpd \
		gpio \
		hostapd httpd \
		identd ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec \
		irdaattach iscsi_target isdnd \
		kdc \
		ldconfig local lpd lvm\
		mixerctl mopd motd mountall mountcritlocal mountcritremote \
		mountd moused mrouted \
		named ndbootd network newsyslog nfsd nfslocking ntpd ntpdate \
		perusertmp pf pf_boot pflogd postfix powerd ppp pwcheck \
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
