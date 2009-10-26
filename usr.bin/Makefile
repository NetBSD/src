#	$NetBSD: Makefile,v 1.181 2009/10/26 04:09:45 christos Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply apropos asa at audio awk \
	banner basename biff bthset btkey btpin \
	bzip2 bzip2recover cal calendar cap_mkdb cdplay \
	checknr chflags chpass cksum cmp col colcrt colrm \
	column comm compress config crontab crunch csplit ctags cut \
	deroff db dirname du \
	eject elf2aout elf2ecoff env error expand extattr \
	false fdformat fgen find finger fmt fold fpr from \
	fsplit fstat ftp gcore genassym gencat getconf getent getopt gprof \
	gzip head hexdump iconv id indent innetgr ipcrm ipcs join jot \
	kdump ktrace ktruss lam last lastcomm ldd leave less \
	lex locale locate lock logger login logname look lorder m4 \
	machine mail make man menuc mesg midiplay mixerctl mkcsmapper \
	mkdep mkesdb mkfifo mklocale mkstr mktemp moduli msgc msgs \
	nbperf netgroup netstat newgrp newsyslog nfsstat nice nl nohup nvi \
	pagesize passwd paste patch pathchk pkill pmap pmc pr \
	printenv printf progress pwhash qsubst quota radioctl rdist \
	renice rev revoke rfcomm_sppd rlogin rpcgen rpcinfo rs rsh rup \
	ruptime rusers rwall rwho script sdiff sdpquery sed seq shar shlock \
	showmount shuffle sockstat soelim sort spell split stat su systat \
	tabs tail talk tcopy tee telnet tftp time tip tn3270 touch tput \
	tr true tset tsort tty ul uname unexpand unifdef \
	uniq units unvis unzip usbhidaction usbhidctl users utoppya \
	uudecode uuencode uuidgen vacation vgrind vis vmstat vndcompress w \
	wall wc what whatis whereis who whois window \
	write xargs xinstall xlint xstr yacc yes

.if (${MKHESIOD} != "no")
SUBDIR+= hesinfo
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+= compile_et kdestroy kgetcred kinit klist mk_cmds string2key
SUBDIR+= hxtool verify_krb5_conf gss krb5-config
.endif

.if (${MKSKEY} != "no")
SUBDIR+= skey skeyinfo skeyinit
.endif

.if (${MKYP} != "no")
SUBDIR+= ypcat ypmatch ypwhich
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+= bdes
SUBDIR+= nbsvtool
.endif

.include <bsd.subdir.mk>
