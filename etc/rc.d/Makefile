# $NetBSD: Makefile,v 1.22 2001/09/08 06:06:52 martin Exp $

.include <bsd.own.mk>

FILES=	DAEMON LOGIN NETWORK SERVERS accounting altqd amd apmd bootparams \
	bootconf.sh ccd cleartmp cron dhclient dhcpd dhcrelay dmesg \
	downinterfaces fsck inetd ipfilter ipmon ipnat ipsec isdnd kdc \
	ldconfig lkm1 lkm2 lkm3 local lpd mopd motd mountall mountcritlocal \
	mountcritremote mountd mrouted named ndbootd network newsyslog nfsd \
	nfslocking ntpd ntpdate rpcbind poffd postfix ppp pwcheck racoon \
	quota raidframe rarpd rbootd root route6d routed rtadvd rtsold rwho \
	savecore screenblank sendmail securelevel sshd \
	swap1 swap2 sysdb sysctl syslogd \
	timed ttys virecover wscons xdm xfs ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
