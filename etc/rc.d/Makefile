# $NetBSD: Makefile,v 1.1.1.1 2000/03/10 11:53:23 lukem Exp $

FILES=	accounting amd apmd bootparams bootconf.sh ccd cleartmp cron daemon \
	dhclient dhcpd dhcrelay dmesg fsck.sh gated inetd ipfilter ipmon ipnat \
	kerberos ldconfig lkm1 lkm2 lkm3 local login lpd mopd motd \
	mountall mountcritlocal mountcritremote mountd mrouted named network \
	nfsd nfsiod nfslocking ntpdate portmap ppp pwcheck quota raidframe \
	rarpd rbootd root route6d routed rtadvd rtsold rwho savecore \
	screenblank sendmail servers sshd swap1 swap2 sysdb sysctl syslogd \
	systemfs timed ttys virecover wscons xdm xfs xntpd ypbind yppasswdd \
	ypserv
FILESDIR=	/etc/rc.d
FILESMODE=	${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
