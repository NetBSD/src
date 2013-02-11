#	$NetBSD: Makefile,v 1.213 2013/02/11 17:24:21 christos Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply asa at audio audiocfg \
	banner basename biff bthset btkey btpin \
	bzip2 bzip2recover cal calendar cap_mkdb cdplay \
	checknr chflags chpass cksum cmp cleantags col colcrt colrm \
	column comm compress config crunch csplit ctags cut \
	deroff db dirname du \
	eject elf2aout elf2ecoff env error expand extattr \
	false fdformat fgen fincore find finger flock fmt fold fpr from \
	fsplit fstat ftp gcore genassym gencat getconf getent getopt gprof \
	head hexdump iconv id indent infocmp innetgr ipcrm ipcs join jot \
	kdump ktrace ktruss lam last lastcomm ldd leave \
	locale locate lock logger login logname look lorder m4 \
	machine mail make man menuc mesg midiplay mixerctl mkcsmapper \
	mkdep mkesdb mkfifo mklocale mkstr mktemp mkubootimage moduli \
	msgc msgs \
	nbperf netgroup netstat newgrp newsyslog nfsstat nice nl nohup nvi \
	pagesize passwd paste patch pathchk pkill pmap pmc pr \
	printenv printf progress pwhash qsubst quota radioctl rdist \
	renice rev revoke rfcomm_sppd rlogin rpcgen rpcinfo rs rsh \
	rup ruptime rusers rwall rwho \
	script sdiff sdpquery sed seq shar shlock shmif_dumpbus \
	showmount shuffle sockstat sort spell split stat su systat \
	tabs tail talk tcopy tee telnet tftp tic time tip touch tpfmt tput \
	tr true tset tsort tty ul uname unexpand unifdef \
	uniq units unvis unzip usbhidaction usbhidctl users utoppya \
	uudecode uuencode uuidgen vacation vgrind videoctl vis \
	vmstat vndcompress w \
	wall wc what whereis who whois \
	write xargs xinstall xlint xstr yes

SUBDIR+= ../external/zlib/pigz/bin/pigz

.if (${MKGROFF} == "no")
SUBDIR+= soelim
.endif

.if (${MKMAKEMANDB} == "no")
SUBDIR+= apropos whatis
.endif

.if (${MKRUMP} != "no")
SUBDIR+= rump_allserver rump_dhcpclient rump_halt rump_server
.endif

.if (${MKBSDGREP} != "no")
SUBDIR+= grep
.endif

.if (${MKPIGZGZIP} != "no")
# nothing
.else
SUBDIR+= gzip 
.endif

.if (${MKHESIOD} != "no")
SUBDIR+= hesinfo
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
