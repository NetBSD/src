#	from: @(#)Makefile.inc	5.21 (Berkeley) 5/24/91
#	$Id: Makefile.inc,v 1.51 1994/08/29 04:47:31 mycroft Exp $

# gen sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/gen ${.CURDIR}/gen

SRCS+=	alarm.c assert.c clock.c closedir.c confstr.c ctermid.c ctime.c \
	ctype_.c daemon.c devname.c difftime.c \
	disklabel.c err.c errlst.c errno.c exec.c fnmatch.c \
	fstab.c ftok.c fts.c \
	getbsize.c getcap.c getcwd.c getdomainname.c getgrent.c getgrouplist.c \
	gethostname.c getloadavg.c getlogin.c getmntinfo.c getnetgrent.c \
	getpagesize.c getpass.c getpwent.c getsubopt.c getttyent.c \
	getusershell.c glob.c infinity.c initgroups.c isatty.c isctype.c \
	isinf.c \
	msgctl.c msgget.c msgsnd.c msgrcv.c \
	mktemp.c nice.c nlist.c opendir.c pause.c popen.c psignal.c pwcache.c \
	raise.c readdir.c rewinddir.c scandir.c seekdir.c semconfig.c semctl.c \
	semget.c semop.c \
	setdomainname.c sethostname.c setjmperr.c setmode.c \
	setproctitle.c shmat.c shmctl.c shmdt.c shmget.c siginterrupt.c \
	siglist.c signal.c signame.c sigsetops.c sleep.c sysconf.c sysctl.c \
	syslog.c telldir.c \
	termios.c time.c times.c timezone.c tolower_.c ttyname.c ttyslot.c \
	toupper_.c ualarm.c uname.c unvis.c usleep.c \
	utime.c valloc.c vis.c wait.c wait3.c \
	waitpid.c

.if   (${MACHINE_ARCH} == "m68k")
SRCS+=	_setjmp.S alloca.S fabs.S frexp.c ldexp.S modf.S setjmp.S
SRCS+=	adddf3.S addsf3.S ashlsi3.S ashrsi3.S cmpdf2.S cmpsf2.S divdf3.S \
	divsf3.S divsi3.S extendsfdf2.S fixdfsi.S fixunsdfsi.S \
	floatsidf.S lshlsi3.S lshrsi3.S modsi3.S muldf3.S mulsf3.S mulsi3.S \
	negdf2.S negsf2.S saveregs.c subdf3.S subsf3.S truncdfsf2.S udivsi3.S \
	umodsi3.S umulsi3.S
.elif (${MACHINE_ARCH} == "i386")
SRCS+=	_setjmp.S alloca.S fabs.S frexp.c ldexp.c modf.S setjmp.S sigsetjmp.S
SRCS+=	divsi3.S fixdfsi.S fixunsdfsi.S udivsi3.S
.elif (${MACHINE_ARCH} == "ns32k")
SRCS+=	__main.c _setjmp.S alloca.S fabs.S frexp.S infinity.c isinf.c
SRCS+=  ldexp.S modf.S setjmp.S
.elif (${MACHINE_ARCH} == "tahoe")
CFLAGS+=-I/sys
SRCS+=	_setjmp.S alloca.S fabs.S frexp.c ldexp.S modf.S setjmp.S
SRCS+=	udiv.S urem.S
.elif (${MACHINE_ARCH} == "vax")
SRCS+=	_setjmp.S alloca.S fabs.S frexp.c ldexp.S modf.S setjmp.S
SRCS+=	udiv.S urem.S
.elif (${MACHINE_ARCH} == "sparc")
SRCS+=	_setjmp.S fabs.S frexp.c infinity.c isnan.c ldexp.c modf.S
SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S setjmp.S
.endif

MAN3+=	alarm.0 clock.0 confstr.0 ctermid.0 ctime.0 ctype.0 \
	daemon.0 devname.0 directory.0 err.0 exec.0 fnmatch.0 frexp.0 \
	ftok.0 fts.0 \
	getbsize.0 getcap.0 getcwd.0 getdomainname.0 getdiskbyname.0 \
	getfsent.0 getgrent.0 \
	getgrouplist.0 gethostname.0 getloadavg.0 \
	getmntinfo.0 getnetgrent.0 getpagesize.0 getpass.0 getpwent.0 \
	getsubopt.0 getttyent.0 getusershell.0 glob.0 \
	initgroups.0 isalnum.0 \
	isalpha.0 isascii.0 isblank.0 iscntrl.0 isdigit.0 isgraph.0 isinf.0 \
	islower.0 isprint.0 ispunct.0 isspace.0 isupper.0 isxdigit.0 \
	ldexp.0 modf.0 nice.0 nlist.0 pause.0 popen.0 psignal.0 \
	pwcache.0 raise.0 scandir.0 setjmp.0 setmode.0 setproctitle.0 \
	siginterrupt.0 signal.0 sigsetops.0 sleep.0 sysconf.0 sysctl.0 \
	syslog.0 tcgetpgrp.0 \
	tcsendbreak.0 tcsetattr.0 tcsetpgrp.0 time.0 times.0 timezone.0 \
	tolower.0 toupper.0 ttyname.0 tzset.0 \
	ualarm.0 uname.0 unvis.0 usleep.0 utime.0 valloc.0 vis.0

MLINKS+=ctime.3 asctime.3 ctime.3 difftime.3 ctime.3 gmtime.3 \
	ctime.3 localtime.3 ctime.3 mktime.3
MLINKS+=directory.3 closedir.3 directory.3 dirfd.3 directory.3 opendir.3 \
	directory.3 readdir.3 directory.3 rewinddir.3 directory.3 seekdir.3 \
	directory.3 telldir.3
MLINKS+=err.3 verr.3 err.3 errx.3 err.3 verrx.3 err.3 warn.3 err.3 vwarn.3 \
        err.3 warnx.3 err.3 vwarnx.3
MLINKS+=exec.3 execl.3 exec.3 execle.3 exec.3 execlp.3 exec.3 execv.3 \
	exec.3 execvp.3
MLINKS+=fts.3 fts_open.3 fts.3 fts_read.3 fts.3 fts_children.3 \
	fts.3 fts_set.3 fts.3 fts_close.3 
MLINKS+=isinf.3 isnan.3
MLINKS+=getcap.3 cgetcap.3 getcap.3 cgetclose.3 getcap.3 cgetent.3 \
	getcap.3 cgetfirst.3 getcap.3 cgetmatch.3 getcap.3 cgetnext.3 \
	getcap.3 cgetnum.3 getcap.3 cgetset.3 getcap.3 cgetstr.3 \
	getcap.3 cgetustr.3
MLINKS+=getcwd.3 getwd.3
MLINKS+=getdomainname.3 setdomainname.3
MLINKS+=getfsent.3 endfsent.3 getfsent.3 getfsfile.3 getfsent.3 getfsspec.3 \
	getfsent.3 getfstype.3 getfsent.3 setfsent.3
MLINKS+=getgrent.3 endgrent.3 getgrent.3 setgroupent.3 getgrent.3 getgrgid.3 \
	getgrent.3 getgrnam.3 getgrent.3 setgrent.3 getgrent.3 setgrfile.3
MLINKS+=gethostname.3 sethostname.3
MLINKS+=getnetgrent.3 innetgr.3 getnetgrent.3 setnetgrent.3 \
	getnetgrent.3 endnetgrent.3
MLINKS+=getpwent.3 endpwent.3 getpwent.3 setpassent.3 getpwent.3 getpwnam.3 \
	getpwent.3 getpwuid.3 getpwent.3 setpwent.3 getpwent.3 setpwfile.3
MLINKS+=getttyent.3 endttyent.3 getttyent.3 getttynam.3 getttyent.3 setttyent.3
MLINKS+=getusershell.3 endusershell.3 getusershell.3 setusershell.3
MLINKS+=glob.3 globfree.3
MLINKS+=popen.3 pclose.3
MLINKS+=psignal.3 sys_siglist.3 psignal.3 sys_signame.3
MLINKS+=pwcache.3 user_from_uid.3 pwcache.3 group_from_gid.3
MLINKS+=scandir.3 alphasort.3
MLINKS+=setjmp.3 _longjmp.3 setjmp.3 _setjmp.3 setjmp.3 longjmp.3 \
	setjmp.3 sigsetjmp.3 setjmp.3 siglongjmp.3 setjmp.3 longjmperror.3
MLINKS+=setmode.3 getmode.3
MLINKS+=sigsetops.3 sigemptyset.3 sigsetops.3 sigfillset.3 \
	sigsetops.3 sigaddset.3 sigsetops.3 sigdelset.3 \
	sigsetops.3 sigismember.3
MLINKS+=syslog.3 closelog.3 syslog.3 openlog.3 syslog.3 setlogmask.3 \
	syslog.3 vsyslog.3
MLINKS+=tcsendbreak.3 tcdrain.3 tcsendbreak.3 tcflow.3 tcsendbreak.3 \
	tcflush.3
MLINKS+=tcsetattr.3 cfgetispeed.3 tcsetattr.3 cfsetispeed.3 \
	tcsetattr.3 cfgetospeed.3 tcsetattr.3 cfsetospeed.3 \
	tcsetattr.3 cfsetspeed.3 tcsetattr.3 cfmakeraw.3 \
	tcsetattr.3 tcgetattr.3
MLINKS+=ttyname.3 isatty.3 ttyname.3 ttyslot.3
MLINKS+=tzset.3 tzsetwall.3
MLINKS+=vis.3 strvis.3 vis.3 strvisx.3 unvis.3 strunvis.3
