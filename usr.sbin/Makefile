#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.13 1993/08/30 07:19:32 glass Exp $

SUBDIR=	accountant accton arp chroot config config.new dbsym dev_mkdb \
	diskpart edquota flcopy gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mtree named portmap pppd pwd_mkdb quotaon repquota \
	rmt rwhod sendmail sliplogin swapinfo syslogd timed \
	traceroute trpt trsp update vipw yp

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
