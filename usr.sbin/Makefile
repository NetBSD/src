#	from: @(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#	$Id: Makefile,v 1.19 1993/12/05 22:11:09 mycroft Exp $

SUBDIR=	accton amd arp chown chroot config config.new dbsym dev_mkdb \
	diskpart edquota flcopy gettable htable inetd iostat kgmon \
	kvm_mkdb lpr mtree named portmap pppd pwd_mkdb quotaon repquota \
	rmt rwhod sendmail sliplogin slstats spray swapinfo syslogd tcpdump \
	timed traceroute trpt trsp update vipw yp

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
