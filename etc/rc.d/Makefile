# $NetBSD: Makefile,v 1.8 2000/07/26 00:11:48 lukem Exp $

FILES=	DAEMON LOGIN NETWORK SERVERS accounting amd apmd bootparams \
	bootconf.sh ccd cleartmp cron dhclient dhcpd dhcrelay dmesg fsck.sh \
	gated inetd ipfilter ipmon ipnat ipsec kerberos ldconfig lkm1 lkm2 \
	lkm3 local lpd mopd \
	motd mountall mountcritlocal mountcritremote mountd mrouted named \
	network nfsd nfslocking ntpd ntpdate rpcbind postfix ppp pwcheck quota \
	raidframe rarpd rbootd root route6d routed rtadvd rtsold rwho savecore \
	screenblank sendmail swap1 swap2 sysdb sysctl syslogd timed \
	ttys virecover wscons xdm xfs ypbind yppasswdd ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
