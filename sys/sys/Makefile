#	$NetBSD: Makefile,v 1.82.6.1 2006/05/24 15:50:47 tron Exp $

INCSDIR= /usr/include/sys

INCS=	acct.h agpio.h ansi.h ataio.h audioio.h \
	bootblock.h bswap.h buf.h \
	callout.h cdefs.h cdefs_aout.h \
	cdefs_elf.h cdio.h chio.h clockctl.h conf.h core.h \
	device.h dir.h dirent.h disk.h disklabel.h disklabel_acorn.h \
	dkbad.h dkio.h dkstat.h domain.h drvctlio.h dvdio.h \
	endian.h envsys.h errno.h evcnt.h event.h exec.h exec_aout.h \
	exec_coff.h exec_ecoff.h exec_elf.h exec_script.h extattr.h extent.h \
	fcntl.h fd_set.h fdio.h featuretest.h file.h filedesc.h filio.h \
	float_ieee754.h fstypes.h gmon.h gpio.h hash.h \
	ieee754.h inttypes.h ioccom.h ioctl.h ioctl_compat.h iostat.h ipc.h \
	joystick.h \
	kcore.h kgdb.h ksem.h ksyms.h ktrace.h \
	lkm.h localedef.h lock.h lockf.h lwp.h \
	malloc.h mallocvar.h mbuf.h md4.h \
	md5.h midiio.h mman.h mount.h msg.h msgbuf.h mtio.h \
	namei.h null.h \
	param.h pipe.h pmc.h poll.h pool.h power.h proc.h \
	protosw.h ptrace.h queue.h \
	ras.h reboot.h radioio.h resource.h resourcevar.h rnd.h \
	sa.h scanio.h sched.h scsiio.h select.h selinfo.h sem.h sha1.h shm.h \
	siginfo.h signal.h signalvar.h sigtypes.h socket.h \
	socketvar.h sockio.h stat.h statvfs.h syscall.h syscallargs.h \
	sysctl.h stdint.h swap.h syslimits.h syslog.h \
	tape.h termios.h time.h timeb.h timepps.h times.h \
	timex.h trace.h tree.h tty.h ttychars.h ttycom.h \
	ttydefaults.h ttydev.h types.h \
	ucontext.h ucred.h uio.h un.h unistd.h unpcb.h user.h utsname.h uuid.h \
	vadvise.h verified_exec.h vmmeter.h vnode.h vnode_if.h \
	wait.h wdog.h

INCSYMLINKS=\
	sys/exec_elf.h /usr/include/elf.h \
	sys/fcntl.h /usr/include/fcntl.h \
	sys/poll.h /usr/include/poll.h \
	sys/stdint.h /usr/include/stdint.h \
	sys/syslog.h /usr/include/syslog.h \
	sys/termios.h /usr/include/termios.h \
	sys/sha1.h /usr/include/sha1.h \
	sys/md4.h /usr/include/md4.h \
	sys/md5.h /usr/include/md5.h

INCSYMLINKS+=	../soundcard.h ${INCSDIR}/soundcard.h

.include <bsd.kinc.mk>
