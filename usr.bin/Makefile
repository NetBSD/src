#	$NetBSD: Makefile,v 1.73 1997/06/22 18:50:33 christos Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>			# for EXPORTABLE_SYSTEM definition

SUBDIR=	apply apropos asa at audioctl banner basename bdes biff cal calendar \
	cap_mkdb checknr chflags chpass cksum cmp col colcrt colrm column \
	comm compress crontab ctags cut dirname du \
	eject env error expand false file find finger fmt fold fpr from \
	fsplit fstat ftp gencat getconf getopt head hexdump id indent \
	ipcrm ipcs join jot kdump ktrace lam last lastcomm leave less lex \
	locate lock logger login logname look lorder m4 machine mail make man \
	mesg mixerctl mkdep mkfifo mkstr modstat msgs netstat newsyslog \
	nfsstat nice nohup pagesize passwd paste patch pr printenv printf \
	quota rdist renice rev rlogin rpcgen rpcinfo rs \
	rsh rup ruptime rusers rwall rwho \
	script sed shar showmount skey skeyinfo skeyinit soelim split strings \
	su systat tail talk tcopy tee tftp time \
	tip tn3270 touch tput tr true tset tsort tty ul uname unexpand \
	unifdef uniq units unvis users uudecode uuencode \
	vacation vgrind vi vis vmstat w wall wc what whatis whereis \
	which who whois window write \
	xargs xinstall xlint xstr yacc yes ypcat ypmatch ypwhich

.if	make(clean) || make(cleandir)
SUBDIR+=ar elf2aout elf2ecoff elfstrip fdformat \
	gprof ldd nm ranlib size strip telnet
.else

# XXX Keep telnet(1) and libtelnet from getting out of sync during the build
# XXX process.
.if !exists(${.CURDIR}/../domestic/usr.bin/telnet) || \
    defined(EXPORTABLE_SYSTEM)
SUBDIR+=telnet
.endif

# Don't use the standard a.out tool chain on non-aout (ELF) platforms.
# On ELF platforms, build the  ELF ldd.
.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips" && \
     ${MACHINE_ARCH} != "powerpc")
SUBDIR+= ar gprof nm ranlib size strip
.else
SUBDIR+= ldd
.endif

.if (${MACHINE_ARCH} == "i386" || ${MACHINE} == "atari")
SUBDIR+= fdformat
.endif

# Build ELF to {ecoff, aout} tools on mips, for old bootblocks/PROMs.
.if (${MACHINE_ARCH} == "mips")
SUBDIR+= elf2aout elf2ecoff elfstrip
.endif

.endif	# make(clean) || make(cleandir)

.include <bsd.subdir.mk>
