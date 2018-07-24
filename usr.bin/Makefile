#	$NetBSD: Makefile,v 1.232 2018/07/24 15:26:16 christos Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply asa at audio audiocfg \
	banner base64 basename biff bthset btkey btpin \
	bzip2 bzip2recover c11 c89 c99 cal calendar cap_mkdb cdplay \
	checknr chflags chpass cksum cmp cleantags col colcrt colrm \
	column comm compress config crunch csplit ctags cut cvslatest \
	dc deroff db dirname du \
	eject elf2aout elf2ecoff env error expand extattr \
	false fdformat fgen fincore find finger flock fmt fold fpr from \
	fsplit fstat ftp gcore genassym gencat getaddrinfo getconf getent \
	getopt gettext gprof \
	head hexdump iconv id indent infocmp innetgr ipcrm ipcs join jot \
	kdump ktrace ktruss lam last lastcomm ldd leave \
	locale locate lock logger login logname look lorder m4 \
	machine mail make man menuc mesg midiplay midirecord mixerctl \
	mkcsmapper mkdep mkesdb mkfifo mklocale mkstr mktemp mkubootimage \
	moduli msgc msgs \
	nbperf nc netgroup netstat newgrp newsyslog nfsstat nice nl nohup \
	pagesize passwd paste patch pathchk pkill pmap pr \
	printenv printf progress pwait pwhash qsubst quota radioctl rdist \
	renice rev revoke rfcomm_sppd rlogin rpcgen rpcinfo rs rsh \
	rup ruptime rusers rwall rwho \
	script sdiff sdpquery sed seq shar shlock \
	showmount shuffle sockstat sort sortinfo \
	spell split stat su sys_info systat \
	tabs tail talk tcopy tee telnet tftp tic time timeout tip touch \
	tput \
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
SUBDIR+= rump_allserver rump_dhcpclient rump_halt rump_server shmif_dumpbus \
	rump_wmd
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

SUBDIR+= bdes
SUBDIR+= nbsvtool

.include <bsd.subdir.mk>
