#	@(#)Makefile	5.6.1.2 (Berkeley) 5/8/91
#
#	$Header: /cvsroot/src/usr.sbin/Makefile,v 1.4 1993/04/04 19:00:49 cgd Exp $
#

SUBDIR=	arp chown chroot config dbsym dev_mkdb diskpart \
	inetd kgmon kvm_mkdb lpr \
	mtree named portmap pwd_mkdb rmt rwalld rwhod\
	sendmail sliplogin syslogd timed traceroute update vipw

.if   make(clean) || make(cleandir)
SUBDIR+=bad144
.elif ${MACHINE} == "i386"
SUBDIR+=bad144 
.endif

.include <bsd.subdir.mk>
