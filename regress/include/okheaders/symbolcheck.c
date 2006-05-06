/*
 * The list of all system headers
 */
#include <sys/types.h>	/* Moved */
#include <sys/param.h>	/* Moved */
#include <sys/uio.h>	/* Moved */

#include <sys/acct.h>
#include <sys/agpio.h>
#include <sys/ansi.h>
#include <sys/ataio.h>
#include <sys/audioio.h>
#include <sys/bootblock.h>
#include <sys/bswap.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#ifdef __ELF__
#include <sys/cdefs_elf.h>
#else
#include <sys/cdefs_aout.h>
#endif
#include <sys/cdio.h>
#include <sys/chio.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/device.h>
#include <sys/dir.h>
#include <sys/dirent.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/disklabel_acorn.h>
#include <sys/dkbad.h>
#include <sys/dkio.h>
#include <sys/dkstat.h>
#include <sys/domain.h>
#include <sys/dvdio.h>
#include <sys/endian.h>
#include <sys/envsys.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/exec.h>

#ifdef notdef
/*
 * Possible MD issues.
 */
#include <sys/exec_aout.h>
#include <sys/exec_coff.h>
#include <sys/exec_ecoff.h>
#include <sys/exec_elf.h>
#endif

#include <sys/exec_script.h>
#include <sys/extent.h>
#include <sys/fcntl.h>
#include <sys/fdio.h>
#include <sys/featuretest.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/filio.h>
#include <sys/gmon.h>
#include <sys/hash.h>
#include <sys/inttypes.h>
#include <sys/ioccom.h>
#include <sys/ioctl.h>
#include <sys/ioctl_compat.h>
#include <sys/ipc.h>
#include <sys/kcore.h>
#ifdef notdef
#include <sys/kgdb.h>	/* machine/db_machdep.h does not exist everywhere */
#endif
#include <sys/ksem.h>
#include <sys/ksyms.h>
#include <sys/ktrace.h>
#include <sys/lkm.h>
#include <sys/localedef.h>
#include <sys/lock.h>
#include <sys/lockf.h>
#include <sys/lwp.h>
#include <sys/malloc.h>
#include <sys/mallocvar.h>
#include <sys/mbuf.h>
#include <sys/md4.h>
#include <sys/md5.h>
#include <sys/midiio.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/msgbuf.h>
#include <sys/mtio.h>
#include <sys/namei.h>
#include <sys/null.h>
#include <sys/param.h>
#ifdef notdef
#include <sys/pipe.h>	/* voff_t is undefined on pmppc */
#endif
#include <sys/poll.h>
#include <sys/pool.h>
#include <sys/power.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/ptrace.h>
#include <sys/queue.h>
#include <sys/radioio.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/rnd.h>
#include <sys/sa.h>
#include <sys/scanio.h>
#include <sys/sched.h>
#include <sys/scsiio.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <sys/sha1.h>
#include <sys/shm.h>
#include <sys/siginfo.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/sigtypes.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/stat.h>
#include <sys/stdint.h>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <sys/syslog.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/timepps.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <sys/trace.h>
#include <sys/tree.h>
#include <sys/tty.h>
#include <sys/ttychars.h>
#include <sys/ttycom.h>
#include <sys/ttydefaults.h>
#include <sys/ttydev.h>
#include <sys/ucontext.h>
#include <sys/ucred.h>
#include <sys/un.h>
#include <sys/unistd.h>
#include <sys/unpcb.h>
#include <sys/user.h>
#include <sys/utsname.h>
#include <sys/vadvise.h>
#include <sys/verified_exec.h>
#include <sys/vmmeter.h>
#include <sys/vnode.h>
#include <sys/wait.h>
#include <sys/wdog.h>
#include <sys/clockctl.h>	/* Moved */
#include <sys/syscallargs.h>	/* Moved */

/*
 * XXX: We are not doing <netinet> yet, but we need
 * the following in <net>
 */
#include <netinet/in_systm.h>	/* Special */
#include <netinet/in.h>		/* Special */
#include <netinet/ip.h>		/* Special */

#include <net/if.h>		/* Moved */
#include <net/if_ether.h>	/* Moved */
#include <net/route.h>		/* Moved */
#include <net/bpf.h>
#include <net/bpfdesc.h>
#include <net/dlt.h>
#include <net/ethertypes.h>
#include <net/if_arc.h>
#include <net/if_arp.h>
#include <net/if_atm.h>
#include <net/if_bridgevar.h>
#include <net/if_dl.h>
#include <net/if_fddi.h>
#include <net/if_gif.h>
#include <net/if_gre.h>		/* Needs <netinet> stuff */
#include <net/if_hippi.h>
#include <net/if_ieee1394.h>
#include <net/if_llc.h>
#include <net/if_media.h>
#include <net/ppp_defs.h>	/* Moved */
#include <net/if_ppp.h>
#include <net/if_pppoe.h>
#include <net/if_pppvar.h>
#include <net/if_slvar.h>
#include <net/if_sppp.h>
#include <net/if_stf.h>
#include <net/if_stripvar.h>
#include <net/if_token.h>
#include <net/if_tun.h>
#include <net/if_types.h>
#include <net/if_vlanvar.h>
#include <net/netisr.h>
#include <net/pfil.h>
#include <net/pfkeyv2.h>
#include <net/ppp-comp.h>
#include <net/radix.h>
#ifdef notdef
#include <net/raw_cb.h>		/* Cannot include, missing struct decl */
#endif
#include <net/slcompress.h>	/* Needs <netinet> stuff */
#include <net/slip.h>
#include <net/zlib.h>

#include <a.out.h>
#include <ar.h>
#include <assert.h>
#include <bitstring.h>
#include <bm.h>
#include <bzlib.h>
#include <cpio.h>
#include <ctype.h>
#include <curses.h>
#include <db.h>
#if HAVE_DES_H
#include <des.h>
#endif
#include <dirent.h>
#include <disktab.h>
#include <dlfcn.h>
#include <elf.h>
#include <err.h>
#include <errno.h>
#include <eti.h>
#include <event.h>
#include <fcntl.h>
#include <float.h>
#include <fmtmsg.h>
#include <fnmatch.h>
#include <form.h>
#include <fstab.h>
#include <fts.h>
#include <getopt.h>
#include <glob.h>
#include <grp.h>
#include <hesiod.h>
#include <histedit.h>
#include <iconv.h>
#ifndef __vax__
#include <ieeefp.h>
#endif
#include <ifaddrs.h>
#include <inttypes.h>
#include <iso646.h>
#include <kvm.h>
#include <langinfo.h>
#include <libgen.h>
#include <libintl.h>
#include <limits.h>
#include <link.h>
#include <link_aout.h>
#include <link_elf.h>
#include <locale.h>
#include <login_cap.h>
#include <lwp.h>
#include <magic.h>
#include <malloc.h>
#include <math.h>
#include <md2.h>
#include <md4.h>
#include <md5.h>
#include <memory.h>
#include <menu.h>
#include <mntopts.h>
#include <mpool.h>
#include <ndbm.h>
#include <netconfig.h>
#include <netdb.h>
#include <netgroup.h>
#include <nl_types.h>
#include <nlist.h>
#include <nsswitch.h>
#include <paths.h>
#include <pcap-namedb.h>
#include <pcap.h>
#include <pci.h>
#include <poll.h>
#include <pthread.h>
#include <pthread_dbg.h>
#include <pthread_queue.h>
#include <pthread_types.h>
#include <pwd.h>
#include <randomid.h>
#include <ranlib.h>
#include <re_comp.h>
#ifdef notdef
#include <regex.h>	/* Conflicts with regexp */
#endif
#include <regexp.h>
#include <resolv.h>
#include <crypto/rmd160.h>
/* without this rmt.h re-defines ioctl which is also defined in soundcard.h */
#define __RMTLIB_PRIVATE
#include <rmt.h>
#include <sa.h>
#include <sched.h>
#include <search.h>
#include <semaphore.h>
#include <setjmp.h>
#include <sgtty.h>
#include <sha1.h>
#include <signal.h>
#if HAVE_SKEY_H
#include <skey.h>
#endif
#include <soundcard.h>
#include <stab.h>
#include <stdarg.h>
#if __GNUC__ > 2
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringlist.h>
#include <strings.h>
#include <struct.h>
#include <sysexits.h>
#include <syslog.h>
#include <tar.h>
#include <tcpd.h>
#include <termcap.h>
#include <termios.h>
#include <time.h>
#include <ttyent.h>
#include <tzfile.h>
#include <ucontext.h>
#include <ulimit.h>
#include <unctrl.h>
#include <unistd.h>
#include <usbhid.h>
#include <util.h>
#include <utime.h>
#include <utmp.h>
#include <utmpx.h>
#include <vis.h>
#include <wchar.h>
#include <wctype.h>
#include <zconf.h>
#include <zlib.h>

#ifdef notyet
/*
 * Add symbols here, that should not be visible in userland,
 * but we make visible. We choose an empty struct to produce a conflict.
 */
struct {
} vaddr_t;
#endif

int main(int, char *[]);

int
main(int argc, char *argv[])
{
	return 0;
}
