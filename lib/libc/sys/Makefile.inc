#	$NetBSD: Makefile.inc,v 1.139 2003/03/14 22:26:13 nathanw Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

# sys sources
.PATH: ${ARCHDIR}/sys ${.CURDIR}/sys

# modules with non-default implementations on at least one architecture:
SRCS+=	Ovfork.S __clone.S __vfork14.S brk.S exect.S fork.S msgctl.S pipe.S \
	getcontext.S \
	ptrace.S sbrk.S __semctl.S shmat.S shmctl.S sigaction.S \
	sigpending.S sigprocmask.S sigreturn.S __sigreturn14.S sigsuspend.S \
	syscall.S __syscall.S __clone.S cerror.S

_LSRC=	Lint_Ovfork.c Lint___clone.c Lint___vfork14.c Lint_brk.c Lint_clone.c \
	Lint_getcontext.c \
	Lint_exect.c Lint_fork.c Lint_pipe.c Lint_ptrace.c Lint_sbrk.c \
	Lint_setlogin.c Lint___sigreturn14.c Lint_syscall.c Lint___syscall.c \
	LintSysNormal.c LintSysNoerr.c LintSysPseudoNoerr.c

.if ${MKLINT} != "no"
LSRCS+=	${_LSRC}
DPSRCS+=${_LSRC:MLintSys*.c}
.endif

# glue to offer userland wrappers for some syscalls
SRCS+=	sigtimedwait.c sigwait.c sigwaitinfo.c

# glue to provide compatibility between GCC 1.X and 2.X and for compat
# with old syscall interfaces.
SRCS+=	adjtime.c clock_settime.c ftruncate.c getdirentries.c lseek.c mmap.c \
	msync.c ntp_adjtime.c pread.c preadv.c pwrite.c pwritev.c semctl.c \
	settimeofday.c sigaltstack.c stat.c swapon.c truncate.c

# glue to syscalls that may pass structs or unions, which cannot be
# handled portably and easily by the syscall interface. The glue
# converts structs and unions to pointers to them.
SRCS+=	__semctl13.c

# namespace purity wrappers
SRCS+=	_brk.c _fork.c _getcontext.c _pipe.c _sbrk.c

# modules with default implementations on all architectures:
ASM=	access.S acct.S bind.S chdir.S chflags.S \
	chmod.S chown.S chroot.S clock_getres.S clock_gettime.S \
	dup.S dup2.S  \
	fchdir.S fchflags.S fchmod.S fchown.S fchroot.S \
	fdatasync.S fhopen.S fhstat.S fhstatfs.S \
	fktrace.S flock.S fpathconf.S __fstat13.S fstatfs.S \
	futimes.S __getcwd.S \
	getdents.S getfh.S getfsstat.S getgroups.S getitimer.S \
	__getlogin.S getpeername.S getpgid.S getpgrp.S \
	getpriority.S getrlimit.S getrusage.S getsid.S getsockname.S \
	getsockopt.S gettimeofday.S ioctl.S kevent.S kqueue.S \
	ktrace.S \
	lchflags.S lchmod.S lchown.S __lstat13.S lfs_bmapv.S lfs_markv.S \
	lfs_segclean.S lfs_segwait.S link.S listen.S lutimes.S \
	madvise.S mincore.S minherit.S mkdir.S mkfifo.S mknod.S \
	mlock.S mlockall.S mount.S mprotect.S __msgctl13.S msgget.S \
	munlock.S munlockall.S munmap.S \
	nfssvc.S ntp_gettime.S rasctl.S pathconf.S \
	pmc_get_info.S pmc_control.S profil.S quotactl.S \
	readlink.S reboot.S recvfrom.S recvmsg.S rename.S revoke.S \
	rmdir.S semconfig.S semget.S semop.S ____semctl13.S sendmsg.S \
	sendto.S _ksem_close.S _ksem_destroy.S _ksem_getvalue.S _ksem_init.S \
	_ksem_post.S _ksem_trywait.S _ksem_unlink.S _ksem_wait.S _ksem_open.S \
	setegid.S setcontext.S seteuid.S setgid.S setgroups.S setitimer.S \
	__setlogin.S setpgid.S setpriority.S setregid.S setreuid.S setrlimit.S \
	setsid.S setsockopt.S setuid.S \
	__shmctl13.S shmdt.S shmget.S shutdown.S \
	__sigaltstack14.S __sigpending14.S __sigprocmask14.S \
	__sigaction_sigtramp.S __sigtimedwait.S \
	socket.S socketpair.S __stat13.S statfs.S \
	swapctl.S symlink.S umask.S undelete.S unlink.S \
	unmount.S utimes.S utrace.S vadvise.S \
	__sysctl.S \
	__posix_chown.S __posix_fchown.S __posix_lchown.S __posix_rename.S \
	_lwp_create.S _lwp_exit.S _lwp_self.S _lwp_wait.S \
	_lwp_suspend.S _lwp_continue.S _lwp_wakeup.S \
	_lwp_getprivate.S _lwp_setprivate.S \
	sa_register.S sa_stacks.S sa_enable.S sa_setconcurrency.S sa_yield.S \
	sa_preempt.S \
	timer_create.S timer_delete.S timer_gettime.S timer_getoverrun.S \
	timer_settime.S

WEAKASM= accept.S close.S connect.S execve.S fcntl.S fsync.S kill.S \
	msgrcv.S msgsnd.S __msync13.S \
	nanosleep.S open.S poll.S read.S readv.S \
	select.S __sigsuspend14.S sysarch.S wait4.S write.S writev.S

NOERR=	getegid.S geteuid.S getgid.S getpid.S getppid.S getuid.S \
	issetugid.S sync.S

PSEUDONOERR=	_exit.S

SRCS+=		${ASM} ${WEAKASM} ${NOERR} ${PSEUDONOERR}
CLEANFILES+=	${ASM} ${WEAKASM} ${NOERR} ${PSEUDONOERR}

ASMDEPS=	${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
		${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}

${WEAKASM}: ${ASMDEPS}
	printf '#include "SYS.h"\nWSYSCALL(${.PREFIX},_sys_${.PREFIX})\nWEAK_ALIAS(_${.PREFIX},_sys_${.PREFIX})\n' >${.TARGET}

${NOERR}: ${ASMDEPS}
	printf '#include "SYS.h"\nRSYSCALL_NOERROR(${.PREFIX})\n' >${.TARGET}

${PSEUDONOERR}: ${ASMDEPS}
	printf '#include "SYS.h"\nPSEUDO_NOERROR(${.PREFIX},${.PREFIX:S/_//})\n' >${.TARGET}

LintSysNormal.c: ${.CURDIR}/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	CPP=${CPP:Q} sh ${.CURDIR}/sys/makelintstub -o ${.TARGET} \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${ASM} ${WEAKASM}

LintSysNoerr.c: ${.CURDIR}/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	CPP=${CPP:Q} sh ${.CURDIR}/sys/makelintstub -o ${.TARGET} -n \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${NOERR}

LintSysPseudoNoerr.c: ${.CURDIR}/sys/makelintstub \
    ${DESTDIR}/usr/include/sys/syscall.h
	CPP=${CPP:Q} sh ${.CURDIR}/sys/makelintstub -o ${.TARGET} -p \
	    -s ${DESTDIR}/usr/include/sys/syscall.h ${PSEUDONOERR}

MAN+=	accept.2 access.2 acct.2 adjtime.2 bind.2 brk.2 chdir.2 \
	chflags.2 chmod.2 chown.2 chroot.2 clock_settime.2 clone.2 close.2 \
	connect.2 dup.2 execve.2 _exit.2 fcntl.2 fdatasync.2 fhopen.2 \
	flock.2 fork.2 fsync.2 getcontext.2 getdents.2 getdirentries.3 \
	getfh.2 getfsstat.2 getgid.2 getgroups.2 \
	getitimer.2 getlogin.2 getpeername.2 getpgrp.2 getpid.2 \
	getpriority.2 getrlimit.2 getrusage.2 getsid.2 getsockname.2 \
	getsockopt.2 gettimeofday.2 getuid.2 intro.2 ioctl.2 issetugid.2 \
	kill.2 kqueue.2 ktrace.2 \
	lfs_bmapv.2 lfs_markv.2 lfs_segclean.2 lfs_segwait.2 \
	link.2 listen.2 lseek.2 _lwp_create.2 _lwp_exit.2 _lwp_self.2 \
	_lwp_suspend.2 _lwp_wakeup.2 _lwp_wait.2 _lwp_getprivate.2 \
	mkdir.2 mkfifo.2 mknod.2 \
	madvise.2 mincore.2 minherit.2 mlock.2 mlockall.2 mmap.2 mount.2 \
	mprotect.2 msgctl.2 msgget.2 msgrcv.2 msgsnd.2 msync.2 \
	munmap.2 nanosleep.2 nfssvc.2 ntp_adjtime.2 open.2 pathconf.2 pipe.2 \
	pmc_control.2 poll.2 profil.2 ptrace.2 quotactl.2 rasctl.2 \
	read.2 readlink.2 \
	reboot.2 recv.2 rename.2 revoke.2 rmdir.2 select.2 semctl.2 \
	semget.2 semop.2 send.2 setgroups.2 setpgid.2 setregid.2 \
	setreuid.2 setsid.2 setuid.2 shmat.2 shmctl.2 shmget.2 \
	shutdown.2 sigaction.2 sigaltstack.2 sigpending.2 \
	sigprocmask.2 sigreturn.2 sigstack.2 sigsuspend.2 sigtimedwait.2 \
	socket.2 \
	socketpair.2 stat.2 statfs.2 swapctl.2 swapon.3 symlink.2 \
	sync.2 sysarch.2 syscall.2 truncate.2 umask.2 undelete.2 \
	unlink.2 utimes.2 utrace.2 vfork.2 wait.2 write.2

MLINKS+=_exit.2 _Exit.2
MLINKS+=brk.2 sbrk.2
MLINKS+=clone.2 __clone.2
MLINKS+=dup.2 dup2.2
MLINKS+=chdir.2 fchdir.2
MLINKS+=chflags.2 fchflags.2 chflags.2 lchflags.2
MLINKS+=chmod.2 fchmod.2 chmod.2 lchmod.2
MLINKS+=chown.2 fchown.2 chown.2 lchown.2
MLINKS+=chroot.2 fchroot.2
MLINKS+=clock_settime.2 clock_gettime.2
MLINKS+=clock_settime.2 clock_getres.2
MLINKS+=fhopen.2 fhstat.2 fhopen.2 fhstatfs.2
MLINKS+=getcontext.2 setcontext.2
MLINKS+=getgid.2 getegid.2
MLINKS+=getitimer.2 setitimer.2
MLINKS+=getlogin.2 setlogin.2
MLINKS+=getpgrp.2 getpgid.2
MLINKS+=getpid.2 getppid.2
MLINKS+=getpriority.2 setpriority.2
MLINKS+=getrlimit.2 setrlimit.2
MLINKS+=getsockopt.2 setsockopt.2
MLINKS+=gettimeofday.2 settimeofday.2
MLINKS+=getuid.2 geteuid.2
MLINKS+=intro.2 errno.2
MLINKS+=kqueue.2 kevent.2
MLINKS+=ktrace.2 fktrace.2
MLINKS+=lseek.2 seek.2
MLINKS+=_lwp_suspend.2 _lwp_continue.2
MLINKS+=_lwp_getprivate.2 _lwp_setprivate.2
MLINKS+=mlock.2 munlock.2
MLINKS+=mlockall.2 munlockall.2
MLINKS+=mount.2 unmount.2
MLINKS+=ntp_adjtime.2 ntp_gettime.2
MLINKS+=pathconf.2 fpathconf.2
MLINKS+=pmc_control.2 pmc_get_info.2
MLINKS+=read.2 readv.2 read.2 pread.2 read.2 preadv.2
MLINKS+=recv.2 recvfrom.2 recv.2 recvmsg.2
MLINKS+=send.2 sendmsg.2 send.2 sendto.2
MLINKS+=setpgid.2 setpgrp.2
MLINKS+=setuid.2 setegid.2 setuid.2 seteuid.2 setuid.2 setgid.2
MLINKS+=shmat.2 shmdt.2
MLINKS+=sigtimedwait.2 sigwaitinfo.2
MLINKS+=sigtimedwait.2 sigwait.2
MLINKS+=stat.2 fstat.2 stat.2 lstat.2
MLINKS+=statfs.2 fstatfs.2
MLINKS+=syscall.2 __syscall.2
MLINKS+=truncate.2 ftruncate.2
MLINKS+=utimes.2 futimes.2 utimes.2 lutimes.2
MLINKS+=wait.2 wait3.2 wait.2 wait4.2 wait.2 waitpid.2
MLINKS+=write.2 writev.2 write.2 pwrite.2 write.2 pwritev.2
