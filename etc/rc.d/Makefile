# $NetBSD: Makefile,v 1.90 2014/09/11 18:01:22 roy Exp $

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
		bluetooth bootconf.sh bootparams \
		ccd cgd cleartmp cron devpubd \
		dhclient dhcpcd dhcpd dhcrelay dmesg downinterfaces envsys \
		fsck fsck_root ftp_proxy ftpd \
		gpio \
		hostapd httpd \
		identd ifwatchd inetd ipfilter ipfs ipmon ipnat ipsec \
		irdaattach iscsi_target isdnd isibootd \
		kdc \
		ldconfig ldpd local lpd lvm \
		makemandb mdnsd mixerctl mopd motd mountall mountcritlocal \
		mountcritremote mountd moused mrouted \
		named ndbootd network newsyslog nfsd nfslocking npf ntpd \
		ntpdate \
		perusertmp pf pf_boot pflogd postfix powerd ppp pwcheck \
		quota \
		racoon rpcbind raidframe raidframeparity random_seed rarpd \
		rbootd rndctl root route6d routed rtadvd rtclocaltime \
		rwho \
		savecore screenblank securelevel sshd \
		staticroute swap1 swap2 sysctl sysdb syslogd \
		timed tpctl ttys \
		veriexec virecover wdogctl wpa_supplicant wscons wsmoused \
		ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.if ${MKX11} != "no"
CONFIGFILES+=	xdm xfs fccache
FILESBUILD_xdm=	yes
FILESBUILD_xfs=	yes
FILESBUILD_fccache= yes

.SUFFIXES: .in
.in:
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,@X11ROOTDIR@,${X11ROOTDIR},g' < ${.IMPSRC} > ${.TARGET}
.endif

.include <bsd.prog.mk>
