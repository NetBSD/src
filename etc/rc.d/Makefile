# $NetBSD: Makefile,v 1.115 2022/05/29 08:10:00 nia Exp $

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
		CRITLOCALMOUNTED DAEMON DISKS LOGIN NETWORKING SERVERS \
		accounting altqd amd apmd automount automountd autounmountd \
		bluetooth bootconf.sh bootparams \
		ccd cgd clearcritlocal cleartmp cron \
		devpubd dhcpcd dhcpd dhcpd6 dhcrelay dmesg \
		downinterfaces \
		entropy envsys \
		fsck fsck_root ftp_proxy ftpd \
		gpio \
		hostapd httpd \
		identd ifwatchd inetd ip6addrctl ipfilter ipfs ipmon ipnat \
		ipsec irdaattach iscsi_target iscsid isibootd \
		kdc \
		ldconfig ldpd lvmlockdir local lpd lvm \
		makemandb mdnsd mixerctl modules mopd motd mountall \
		mountcritlocal mountcritremote mountd moused \
		named ndbootd network newsyslog nfsd nfslocking npf npf_boot \
		npfd \
		perusertmp pf pf_boot pflogd postfix powerd pwcheck \
		quota \
		racoon rpcbind raidframe raidframeparity random_seed rarpd \
		rbootd resize_root rndctl root route6d routed rtadvd \
		rtclocaltime rwho \
		savecore screenblank securelevel smtoff sshd \
		staticroute swap1 swap2 sysctl sysdb syslogd \
		timed tpctl ttys \
		veriexec virecover wdogctl wpa_supplicant wscons wsmoused
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

.if ${MKMROUTING} != "no"
CONFIGFILES+=	mrouted
.endif

.if ${MKNTP} != "no"
CONFIGFILES+=	ntpd ntpdate
.endif

.if ${MKPPP} != "no"
CONFIGFILES+=	ppp
.endif

.if ${MKYP} != "no"
CONFIGFILES+=	ypbind yppasswdd ypserv
.endif

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

.if ${MKZFS} != "no"
CONFIGFILES+=	zfs
.endif

.include <bsd.prog.mk>
