/*	NetBSD: sys-bsd.c,v 1.68 2013/06/24 20:43:48 christos Exp 	*/

/*
 * sys-bsd.c - System-dependent procedures for setting up
 * PPP interfaces on bsd-4.4-ish systems (including 386BSD, NetBSD, etc.)
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright (c) 1989-2002 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
#define RCSID	"Id: sys-bsd.c,v 1.47 2000/04/13 12:04:23 paulus Exp "
#else
__RCSID("NetBSD: sys-bsd.c,v 1.68 2013/06/24 20:43:48 christos Exp ");
#endif
#endif

/*
 * TODO:
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <vis.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#if defined(NetBSD1_2) || defined(__NetBSD_Version__)
#include <util.h>
#endif
#ifdef PPP_FILTER
#include <net/bpf.h>
#endif

#include <net/if.h>
#include <net/ppp_defs.h>
#include <net/if_ppp.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#ifdef __KAME__
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif
#include <ifaddrs.h>

#ifndef IN6_LLADDR_FROM_EUI64
#ifdef __KAME__
#define IN6_LLADDR_FROM_EUI64(sin6, eui64) do {			\
	sin6.sin6_family = AF_INET6;				\
	sin6.sin6_len = sizeof(struct sockaddr_in6);		\
	sin6.sin6_addr.s6_addr[0] = 0xfe;			\
	sin6.sin6_addr.s6_addr[1] = 0x80;			\
	eui64_copy(eui64, sin6.sin6_addr.s6_addr[8]);		\
} while (/*CONSTCOND*/0)
#define IN6_IFINDEX(sin6, ifindex)	 			\
    /* KAME ifindex hack */					\
    *(u_int16_t *)&sin6.sin6_addr.s6_addr[2] = htons(ifindex)
#else
#define IN6_LLADDR_FROM_EUI64(sin6, eui64) do {			\
	memset(&sin6.s6_addr, 0, sizeof(struct in6_addr));	\
	sin6.s6_addr16[0] = htons(0xfe80);			\
	eui64_copy(eui64, sin6.s6_addr32[2]);			\
} while (/*CONSTCOND*/0)
#endif
#endif

#if RTM_VERSION >= 3
#include <sys/param.h>
#if defined(NetBSD) && (NetBSD >= 199703)
#include <netinet/if_inarp.h>
#else	/* NetBSD 1.2D or later */
#ifdef __FreeBSD__
#include <netinet/if_ether.h>
#else
#include <net/if_ether.h>
#endif
#endif
#endif

#include "pppd.h"
#include "fsm.h"
#include "ipcp.h"

#ifdef RCSID
static const char rcsid[] = RCSID;
#endif

static int initdisc = -1;	/* Initial TTY discipline for ppp_fd */
static int initfdflags = -1;	/* Initial file descriptor flags for ppp_fd */
static int ppp_fd = -1;		/* fd which is set to PPP discipline */
static int rtm_seq;

static int restore_term;	/* 1 => we've munged the terminal */
static struct termios inittermios; /* Initial TTY termios */
static struct winsize wsinfo;	/* Initial window size info */

static int loop_slave = -1;
static int loop_master = -1;
static int doing_cleanup = 0;
static char loop_name[20];

static unsigned char inbuf[512]; /* buffer for chars read from loopback */

static int sock_fd;		/* socket for doing interface ioctls */
#ifdef INET6
static int sock6_fd = -1;	/* socket for doing ipv6 interface ioctls */
#endif /* INET6 */
static int ttyfd = -1;		/* the file descriptor of the tty */

static fd_set in_fds;		/* set of fds that wait_input waits for */
static int max_in_fd;		/* highest fd set in in_fds */

static int if_is_up;		/* the interface is currently up */
#ifdef INET6
static int if6_is_up;		/* the interface is currently up */
#endif /* INET6 */
static u_int32_t ifaddrs[2];	/* local and remote addresses we set */
static u_int32_t default_route_gateway;	/* gateway addr for default route */
static u_int32_t proxy_arp_addr;	/* remote addr for proxy arp */

/* Prototypes for procedures local to this file. */
static int get_flags(int);
static void set_flags(int, int);
static int dodefaultroute(u_int32_t, int);
static int get_ether_addr(u_int32_t, struct sockaddr_dl *);
static void restore_loop(void);	/* Transfer ppp unit back to loopback */
static int setifstate(int, int);


static void
set_queue_size(const char *fmt, int fd) {
#ifdef TIOCSQSIZE
    int oqsize, qsize = 32768;

    /* Only for ptys */
    if (ioctl(fd, TIOCGQSIZE, &oqsize) == -1)
	return;

    if (oqsize >= qsize)
	return;

    if (ioctl(fd, TIOCSQSIZE, &qsize) == -1)
	warn("%s: Cannot set tty queue size for %d from %d to %d", fmt, fd,
	    oqsize, qsize);
    else
	notice("%s: Changed queue size of %d from %d to %d", fmt, fd, oqsize,
	    qsize);
#endif
}

/********************************************************************
 *
 * Functions to read and set the flags value in the device driver
 */

static int
get_flags(int fd)
{    
    int flags;

    if (ioctl(fd, PPPIOCGFLAGS, (caddr_t) &flags) == -1)
	fatal("%s: ioctl(PPPIOCGFLAGS): %m", __func__);

    SYSDEBUG((LOG_DEBUG, "get flags = %x\n", flags));
    return flags;
}

/********************************************************************/

static void
set_flags(int fd, int flags)
{    
    SYSDEBUG((LOG_DEBUG, "set flags = %x\n", flags));

    if (ioctl(fd, PPPIOCSFLAGS, (caddr_t) &flags) == -1)
	fatal("%s: ioctl(PPPIOCSFLAGS, %x): %m", __func__, flags, errno);
}

/*
 * sys_init - System-dependent initialization.
 */
void
sys_init(void)
{
    /* Get an internet socket for doing socket ioctl's on. */
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	fatal("%s: Couldn't create IP socket: %m", __func__);

#ifdef INET6
    if ((sock6_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
	/* check it at runtime */
	sock6_fd = -1;
    }
#endif

    FD_ZERO(&in_fds);
    max_in_fd = 0;
}

/*
 * sys_cleanup - restore any system state we modified before exiting:
 * mark the interface down, delete default route and/or proxy arp entry.
 * This should call die() because it's called from die().
 */
void
sys_cleanup(void)
{
    struct ifreq ifr;

    doing_cleanup = 1;
    if (if_is_up) {
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(sock_fd, SIOCGIFFLAGS, &ifr) >= 0
	    && ((ifr.ifr_flags & IFF_UP) != 0)) {
	    ifr.ifr_flags &= ~IFF_UP;
	    ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
	}
    }
    if (ifaddrs[0] != 0)
	cifaddr(0, ifaddrs[0], ifaddrs[1]);
    if (default_route_gateway)
	cifdefaultroute(0, 0, default_route_gateway);
    if (proxy_arp_addr)
	cifproxyarp(0, proxy_arp_addr);
    doing_cleanup = 0;
}

/*
 * sys_close - Clean up in a child process before execing.
 */
void
sys_close()
{
    if (sock_fd >= 0)
	close(sock_fd);
#ifdef INET6
    if (sock6_fd >= 0)
	close(sock6_fd);
#endif
    if (loop_slave >= 0)
	close(loop_slave);
    if (loop_master >= 0)
	close(loop_master);
}

/*
 * sys_check_options - check the options that the user specified
 */
int
sys_check_options(void)
{
#ifndef CDTRCTS
    if (crtscts == 2) {
	warn("%s: DTR/CTS flow control is not supported on this system",
	    __func__);
	return 0;
    }
#endif
    return 1;
}

/*
 * ppp_available - check whether the system has any ppp interfaces
 * (in fact we check whether we can create one)
 */
int
ppp_available(void)
{
    int s;
    extern char *no_ppp_msg;
    struct ifreq ifr;


    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	fatal("%s: socket: %m", __func__);

    (void)memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, "ppp0", sizeof(ifr.ifr_name));
    if (ioctl(s, SIOCIFCREATE, &ifr) == -1) {
	int notmine = errno == EEXIST;
	(void)close(s);
	if (notmine)
	    return 1;
	goto out;
    }
    (void)ioctl(s, SIOCIFDESTROY, &ifr);
    (void)close(s);
    return 1;

out:
    no_ppp_msg = "\
This system lacks kernel support for PPP.  To include PPP support\n\
in the kernel, please read the ppp(4) manual page.\n";
    return 0;
}

/*
 * tty_establish_ppp - Turn the serial port into a ppp interface.
 */
int
tty_establish_ppp(int fd)
{
    int pppdisc = PPPDISC;
    int x;
    ttyfd = fd;

    if (demand) {
	/*
	 * Demand mode - prime the old ppp device to relinquish the unit.
	 */
	if (ioctl(ppp_fd, PPPIOCXFERUNIT, 0) < 0)
	    fatal("%s: ioctl(transfer ppp unit): %m", __func__);
    }

    set_queue_size(__func__, fd);
    /*
     * Save the old line discipline of fd, and set it to PPP.
     */
    if (ioctl(fd, TIOCGETD, &initdisc) < 0)
	fatal("%s: ioctl(TIOCGETD): %m", __func__);
    if (ioctl(fd, TIOCSETD, &pppdisc) < 0)
	fatal("%s: ioctl(TIOCSETD): %m", __func__);

    if (ioctl(fd, PPPIOCGUNIT, &x) < 0)
	fatal("%s: ioctl(PPPIOCGUNIT): %m", __func__);
    if (!demand) {
	/*
	 * Find out which interface we were given.
	 */
	ifunit = x;
    } else {
	/*
	 * Check that we got the same unit again.
	 */
	if (x != ifunit)
	    fatal("%s: transfer_ppp failed: wanted unit %d, got %d",
		__func__, ifunit, x);
	x = TTYDISC;
	if (ioctl(loop_slave, TIOCSETD, &x) == -1)
	    fatal("%s: ioctl(TIOCGETD): %m", __func__);
    }

    ppp_fd = fd;

    /*
     * Enable debug in the driver if requested.
     */
    if (kdebugflag) {
	x = get_flags(fd);
	x |= (kdebugflag & 0xFF) * SC_DEBUG;
	set_flags(fd, x);
    }

    /*
     * Set device for non-blocking reads.
     */
    if ((initfdflags = fcntl(fd, F_GETFL)) == -1
	|| fcntl(fd, F_SETFL, initfdflags | O_NONBLOCK) == -1) {
	warn("%s: Couldn't set device to non-blocking mode: %m", __func__);
    }

    return fd;
}

/*
 * restore_loop - reattach the ppp unit to the loopback.
 */
static void
restore_loop(void)
{
    int x;

    set_queue_size(__func__, loop_slave);
    /*
     * Transfer the ppp interface back to the loopback.
     */
    if (ioctl(ppp_fd, PPPIOCXFERUNIT, 0) < 0)
	fatal("%s: ioctl(transfer ppp unit): %m", __func__);
    x = PPPDISC;
    if (ioctl(loop_slave, TIOCSETD, &x) < 0)
	fatal("%s: ioctl(TIOCSETD): %m", __func__);

    /*
     * Check that we got the same unit again.
     */
    if (ioctl(loop_slave, PPPIOCGUNIT, &x) < 0)
	fatal("%s: ioctl(PPPIOCGUNIT): %m", __func__);
    if (x != ifunit)
	fatal("%s: transfer_ppp failed: wanted unit %d, got %d", __func__,
	    ifunit, x);
    ppp_fd = loop_slave;
}


/*
 * Determine if the PPP connection should still be present.
 */
extern int hungup;

/*
 * tty_disestablish_ppp - Restore the serial port to normal operation.
 * and reconnect the ppp unit to the loopback if in demand mode.
 * This shouldn't call die() because it's called from die().
 */
void
tty_disestablish_ppp(fd)
    int fd;
{
    if (!doing_cleanup && demand)
	restore_loop();

    if (!hungup || demand) {

	/* Flush the tty output buffer so that the TIOCSETD doesn't hang.  */
	if (tcflush(fd, TCIOFLUSH) < 0)
	    if (!doing_cleanup)
		warn("%s: tcflush failed: %m", __func__);

	/* Restore old line discipline. */
	if (initdisc >= 0 && ioctl(fd, TIOCSETD, &initdisc) < 0)
	    if (!doing_cleanup)
		error("%s: ioctl(TIOCSETD): %m", __func__);
	initdisc = -1;

	/* Reset non-blocking mode on fd. */
	if (initfdflags != -1 && fcntl(fd, F_SETFL, initfdflags) < 0)
	    if (!doing_cleanup)
		warn("%s: Couldn't restore device fd flags: %m", __func__);
    }
    initfdflags = -1;

    if (fd == ppp_fd)
	ppp_fd = -1;
}

/*
 * cfg_bundle - configure the existing bundle.
 * Used in demand mode.
 */
void
cfg_bundle(int mrru, int mtru, int rssn, int tssn)
{
    abort();
#ifdef notyet
    int flags;
    struct ifreq ifr;

    if (!new_style_driver)
	return;

    /* set the mrru, mtu and flags */
    if (ioctl(ppp_dev_fd, PPPIOCSMRRU, &mrru) < 0)
	error("%s: Couldn't set MRRU: %m", __func__);
    flags = get_flags(ppp_dev_fd);
    flags &= ~(SC_MP_SHORTSEQ | SC_MP_XSHORTSEQ);
    flags |= (rssn? SC_MP_SHORTSEQ: 0) | (tssn? SC_MP_XSHORTSEQ: 0)
	    | (mrru? SC_MULTILINK: 0);

    set_flags(ppp_dev_fd, flags);

    /* connect up the channel */
    if (ioctl(ppp_fd, PPPIOCCONNECT, &ifunit) < 0)
	fatal("%s: Couldn't attach to PPP unit %d: %m", __func__, ifunit);
    add_fd(ppp_dev_fd);
#endif
}

/*
 * make_new_bundle - create a new PPP unit (i.e. a bundle)
 * and connect our channel to it.  This should only get called
 * if `multilink' was set at the time establish_ppp was called.
 * In demand mode this uses our existing bundle instead of making
 * a new one.
 */
void
make_new_bundle(int mrru, int mtru, int rssn, int tssn)
{
    abort();
#ifdef notyet
    if (!new_style_driver)
	return;

    /* make us a ppp unit */
    if (make_ppp_unit() < 0)
	die(1);

    /* set the mrru, mtu and flags */
    cfg_bundle(mrru, mtru, rssn, tssn);
#endif
}

/*
 * bundle_attach - attach our link to a given PPP unit.
 * We assume the unit is controlled by another pppd.
 */
int
bundle_attach(int ifnum)
{
    abort();
#ifdef notyet
    if (!new_style_driver)
	return -1;

    if (ioctl(ppp_dev_fd, PPPIOCATTACH, &ifnum) < 0) {
	if (errno == ENXIO)
	    return 0;	/* doesn't still exist */
	fatal("%s: Couldn't attach to interface unit %d: %m", __func__, ifnum);
    }
    if (ioctl(ppp_fd, PPPIOCCONNECT, &ifnum) < 0)
	fatal("%s: Couldn't connect to interface unit %d: %m", __func__, ifnum);
    set_flags(ppp_dev_fd, get_flags(ppp_dev_fd) | SC_MULTILINK);

    ifunit = ifnum;
#endif
    return 1;
}

/*
 * destroy_bundle - tell the driver to destroy our bundle.
 */
void destroy_bundle(void)
{
#if notyet
	if (ppp_dev_fd >= 0) {
		close(ppp_dev_fd);
		remove_fd(ppp_dev_fd);
		ppp_dev_fd = -1;
	}
#endif
}

/*
 * Check whether the link seems not to be 8-bit clean.
 */
void
clean_check(void)
{
    int x;
    char *s;

    if (ioctl(ppp_fd, PPPIOCGFLAGS, (caddr_t) &x) == 0) {
	s = NULL;
	switch (~x & (SC_RCV_B7_0|SC_RCV_B7_1|SC_RCV_EVNP|SC_RCV_ODDP)) {
	case SC_RCV_B7_0:
	    s = "bit 7 set to 1";
	    break;
	case SC_RCV_B7_1:
	    s = "bit 7 set to 0";
	    break;
	case SC_RCV_EVNP:
	    s = "odd parity";
	    break;
	case SC_RCV_ODDP:
	    s = "even parity";
	    break;
	}
	if (s != NULL) {
	    struct ppp_rawin win;
	    char buf[4 * sizeof(win.buf) + 1];
	    int i;
	    warn("%s: Serial link is not 8-bit clean:", __func__);
	    warn("%s: All received characters had %s", __func__, s);
	    if (ioctl(ppp_fd, PPPIOCGRAWIN, &win) == -1) {
		warn("%s: ioctl(PPPIOCGRAWIN): %s", __func__, strerror(errno));
		return;
	    }
	    for (i = 0; i < sizeof(win.buf); i++)
		win.buf[i] = win.buf[i] & 0x7f;
	    strvisx(buf, (char *)win.buf, win.count, VIS_CSTYLE);
	    warn("%s: Last %d characters were: %s", __func__, (int)win.count,
		buf);
	}
    }
}


/*
 * set_up_tty: Set up the serial port on `fd' for 8 bits, no parity,
 * at the requested speed, etc.  If `local' is true, set CLOCAL
 * regardless of whether the modem option was specified.
 *
 * For *BSD, we assume that speed_t values numerically equal bits/second.
 */
void
set_up_tty(int fd, int local)
{
    struct termios tios;

    if (tcgetattr(fd, &tios) < 0)
	fatal("%s: tcgetattr: %m", __func__);

    if (!restore_term) {
	inittermios = tios;
	ioctl(fd, TIOCGWINSZ, &wsinfo);
    }

    set_queue_size(__func__, fd);

    tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CLOCAL);
    if (crtscts > 0 && !local) {
        if (crtscts == 2) {
#ifdef CDTRCTS
            tios.c_cflag |= CDTRCTS;
#endif
	} else
	    tios.c_cflag |= CRTSCTS;
    } else if (crtscts < 0) {
	tios.c_cflag &= ~CRTSCTS;
#ifdef CDTRCTS
	tios.c_cflag &= ~CDTRCTS;
#endif
    }

    tios.c_cflag |= CS8 | CREAD | HUPCL;
    if (local || !modem)
	tios.c_cflag |= CLOCAL;
    tios.c_iflag = IGNBRK | IGNPAR;
    tios.c_oflag = 0;
    tios.c_lflag = 0;
    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 0;

    if (crtscts == -2) {
	tios.c_iflag |= IXON | IXOFF;
	tios.c_cc[VSTOP] = 0x13;	/* DC3 = XOFF = ^S */
	tios.c_cc[VSTART] = 0x11;	/* DC1 = XON  = ^Q */
    }

    if (inspeed) {
	cfsetospeed(&tios, inspeed);
	cfsetispeed(&tios, inspeed);
    } else {
	inspeed = cfgetospeed(&tios);
	/*
	 * We can't proceed if the serial port speed is 0,
	 * since that implies that the serial port is disabled.
	 */
	if (inspeed == 0)
	    fatal("%s: Baud rate for %s is 0; need explicit baud rate",
		__func__, devnam);
    }
    baud_rate = inspeed;

    if (tcsetattr(fd, TCSAFLUSH, &tios) < 0)
	fatal("%s: tcsetattr: %m", __func__);

    restore_term = 1;
}

/*
 * restore_tty - restore the terminal to the saved settings.
 */
void
restore_tty(int fd)
{
    if (restore_term) {
	if (!default_device) {
	    /*
	     * Turn off echoing, because otherwise we can get into
	     * a loop with the tty and the modem echoing to each other.
	     * We presume we are the sole user of this tty device, so
	     * when we close it, it will revert to its defaults anyway.
	     */
	    inittermios.c_lflag &= ~(ECHO | ECHONL);
	}
	if (tcsetattr(fd, TCSAFLUSH, &inittermios) < 0)
	    if (errno != ENXIO)
		warn("%s: tcsetattr: %m", __func__);
	ioctl(fd, TIOCSWINSZ, &wsinfo);
	restore_term = 0;
    }
}

/*
 * setdtr - control the DTR line on the serial port.
 * This is called from die(), so it shouldn't call die().
 */
void
setdtr(int fd, int on)
{
    int modembits = TIOCM_DTR;

    ioctl(fd, (on? TIOCMBIS: TIOCMBIC), &modembits);
}

#ifdef INET6
/*
 * sif6addr - Config the interface with an IPv6 link-local address
 */
int
sif6addr(int unit, eui64_t our_eui64, eui64_t his_eui64)
{
#ifdef __KAME__
    int ifindex;
    struct in6_aliasreq addreq6;

    if (sock6_fd < 0) {
	fatal("%s: No IPv6 socket available", __func__);
	/*NOTREACHED*/
    }

    /* actually, this part is not kame local - RFC2553 conformant */
    ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
	error("%s: sifaddr6: no interface %s", __func__, ifname);
	return 0;
    }

    memset(&addreq6, 0, sizeof(addreq6));
    strlcpy(addreq6.ifra_name, ifname, sizeof(addreq6.ifra_name));

    /* my addr */
    IN6_LLADDR_FROM_EUI64(addreq6.ifra_addr, our_eui64);
    IN6_IFINDEX(addreq6.ifra_addr, ifindex);

#ifdef notdef
    /* his addr */
    IN6_LLADDR_FROM_EUI64(addreq6.ifra_dstaddr, his_eui64);
    IN6_IFINDEX(addreq6.ifra_dstaddr, ifindex);
#endif

    /* prefix mask: 72bit */
    addreq6.ifra_prefixmask.sin6_family = AF_INET6;
    addreq6.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
    memset(&addreq6.ifra_prefixmask.sin6_addr, 0xff,
	sizeof(addreq6.ifra_prefixmask.sin6_addr) - sizeof(our_eui64));
    memset((char *)&addreq6.ifra_prefixmask.sin6_addr +
	sizeof(addreq6.ifra_prefixmask.sin6_addr) - sizeof(our_eui64), 0x00,
	sizeof(our_eui64));

    /* address lifetime (infty) */
    addreq6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
    addreq6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;

    if (ioctl(sock6_fd, SIOCAIFADDR_IN6, &addreq6) < 0) {
	error("%s: sif6addr: ioctl(SIOCAIFADDR_IN6): %m", __func__);
	return 0;
    }

    return 1;
#else
    struct in6_ifreq ifr6;
    struct ifreq ifr;
    struct in6_rtmsg rt6;

    if (sock6_fd < 0) {
	fatal("%s: No IPv6 socket available", __func__);
	/*NOTREACHED*/
    }

    memset(&ifr, 0, sizeof (ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(sock6_fd, SIOCGIFINDEX, (caddr_t) &ifr) < 0) {
	error("%s: sif6addr: ioctl(SIOCGIFINDEX): %m", __func__);
	return 0;
    }
    
    /* Local interface */
    memset(&ifr6, 0, sizeof(ifr6));
    IN6_LLADDR_FROM_EUI64(ifr6.ifr6_addr, our_eui64);
    ifr6.ifr6_ifindex = ifindex;
    ifr6.ifr6_prefixlen = 10;

    if (ioctl(sock6_fd, SIOCSIFADDR, &ifr6) < 0) {
	error("%s: sif6addr: ioctl(SIOCSIFADDR): %m", __func__);
	return 0;
    }
    
    /* Route to remote host */
    memset(&rt6, 0, sizeof(rt6));
    IN6_LLADDR_FROM_EUI64(rt6.rtmsg_dst, his_eui64);
    rt6.rtmsg_flags = RTF_UP;
    rt6.rtmsg_dst_len = 10;
    rt6.rtmsg_ifindex = ifr.ifr_ifindex;
    rt6.rtmsg_metric = 1;
    
    if (ioctl(sock6_fd, SIOCADDRT, &rt6) < 0) {
	error("%s: sif6addr: ioctl(SIOCADDRT): %m", __func__);
	return 0;
    }

    return 1;
#endif
}


/*
 * cif6addr - Remove IPv6 address from interface
 */
int
cif6addr(int unit, eui64_t our_eui64, eui64_t his_eui64)
{
#ifdef __KAME__
    int ifindex;
    struct in6_ifreq delreq6;

    if (sock6_fd < 0) {
	fatal("%s: No IPv6 socket available", __func__);
	/*NOTREACHED*/
    }

    /* actually, this part is not kame local - RFC2553 conformant */
    ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
	error("%s: cifaddr6: no interface %s", __func__, ifname);
	return 0;
    }

    memset(&delreq6, 0, sizeof(delreq6));
    strlcpy(delreq6.ifr_name, ifname, sizeof(delreq6.ifr_name));

    /* my addr */
    IN6_LLADDR_FROM_EUI64(delreq6.ifr_ifru.ifru_addr, our_eui64);
    IN6_IFINDEX(delreq6.ifr_ifru.ifru_addr, ifindex);

    if (ioctl(sock6_fd, SIOCDIFADDR_IN6, &delreq6) < 0) {
	error("%s: cif6addr: ioctl(SIOCDIFADDR_IN6): %m", __func__);
	return 0;
    }

    return 1;
#else
    struct ifreq ifr;
    struct in6_ifreq ifr6;

    if (sock6_fd < 0) {
	fatal("%s: No IPv6 socket available", __func__);
	/*NOTREACHED*/
    }

    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(sock6_fd, SIOCGIFINDEX, (caddr_t) &ifr) < 0) {
	error("%s: cif6addr: ioctl(SIOCGIFINDEX): %m", __func__);
	return 0;
    }
    
    memset(&ifr6, 0, sizeof(ifr6));
    IN6_LLADDR_FROM_EUI64(ifr6.ifr6_addr, our_eui64);
    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    ifr6.ifr6_prefixlen = 10;

    if (ioctl(sock6_fd, SIOCDIFADDR, &ifr6) < 0) {
	if (errno != EADDRNOTAVAIL) {
	    if (! ok_error (errno))
		error("%s: cif6addr: ioctl(SIOCDIFADDR): %m", __func__);
	}
        else {
	    warn("%s: cif6addr: ioctl(SIOCDIFADDR): No such address", __func__);
	}
        return (0);
    }
    return 1;
#endif
}
#endif /* INET6 */

/*
 * get_pty - get a pty master/slave pair and chown the slave side
 * to the uid given.  Assumes slave_name points to >= 12 bytes of space.
 */
int
get_pty(int *master_fdp, int *slave_fdp, char *slave_name, int uid)
{
    struct termios tios;

    if (openpty(master_fdp, slave_fdp, slave_name, NULL, NULL) < 0)
	return 0;

    set_queue_size(__func__, *master_fdp);
    set_queue_size(__func__, *slave_fdp);
    fchown(*slave_fdp, uid, -1);
    fchmod(*slave_fdp, S_IRUSR | S_IWUSR);
    if (tcgetattr(*slave_fdp, &tios) == 0) {
	tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
	tios.c_cflag |= CS8 | CREAD | CLOCAL;
	tios.c_iflag  = IGNPAR;
	tios.c_oflag  = 0;
	tios.c_lflag  = 0;
	if (tcsetattr(*slave_fdp, TCSAFLUSH, &tios) < 0)
	    warn("%s: couldn't set attributes on pty: %m", __func__);
    } else
	warn("%s: couldn't get attributes on pty: %m", __func__);

    return 1;
}


/*
 * open_ppp_loopback - open the device we use for getting
 * packets in demand mode, and connect it to a ppp interface.
 * Here we use a pty.
 */
int
open_ppp_loopback(void)
{
    int flags;
    struct termios tios;
    int pppdisc = PPPDISC;

    if (openpty(&loop_master, &loop_slave, loop_name, NULL, NULL) < 0)
	fatal("%s: No free pty for loopback", __func__);
    SYSDEBUG(("using %s for loopback", loop_name));

    if (tcgetattr(loop_slave, &tios) == 0) {
	tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
	tios.c_cflag |= CS8 | CREAD | CLOCAL;
	tios.c_iflag = IGNPAR;
	tios.c_oflag = 0;
	tios.c_lflag = 0;
	if (tcsetattr(loop_slave, TCSAFLUSH, &tios) < 0)
	    warn("%s: couldn't set attributes on loopback: %m", __func__);
    }

    flags = fcntl(loop_master, F_GETFL);
    if (flags == -1 || fcntl(loop_master, F_SETFL, flags | O_NONBLOCK) == -1)
	    warn("%s: couldn't set master loopback to nonblock: %m", __func__);

    flags = fcntl(loop_slave, F_GETFL);
    if (flags == -1 || fcntl(loop_slave, F_SETFL, flags | O_NONBLOCK) == -1)
	    warn("%s: couldn't set slave loopback to nonblock: %m", __func__);

    ppp_fd = loop_slave;
    if (ioctl(ppp_fd, TIOCSETD, &pppdisc) < 0)
	fatal("%s: ioctl(TIOCSETD): %m", __func__);

    /*
     * Find out which interface we were given.
     */
    if (ioctl(ppp_fd, PPPIOCGUNIT, &ifunit) < 0)
	fatal("%s: ioctl(PPPIOCGUNIT): %m", __func__);

    /*
     * Enable debug in the driver if requested.
     */
    if (kdebugflag) {
	flags = get_flags(ppp_fd);
	flags |= (kdebugflag & 0xFF) * SC_DEBUG;
	set_flags(ppp_fd, flags);
    }

    return loop_master;
}


/*
 * output - Output PPP packet.
 */
void
output(int unit, u_char *p, int len)
{
    if (debug)
	dbglog("sent %P", p, len);

    if (write(ttyfd, p, len) < 0) {
	if (errno != EIO)
	    error("%s: write: %m", __func__);
    }
}


/*
 * wait_input - wait until there is data available,
 * for the length of time specified by *timo (indefinite
 * if timo is NULL).
 */
void
wait_input(struct timeval *timo)
{
    fd_set ready;
    int n;

    ready = in_fds;
    n = select(max_in_fd + 1, &ready, NULL, &ready, timo);
    if (n < 0 && errno != EINTR)
	fatal("%s: select: %m", __func__);
}


/*
 * add_fd - add an fd to the set that wait_input waits for.
 */
void add_fd(int fd)
{
    if (fd >= FD_SETSIZE)
	fatal("%s: descriptor too big", __func__);
    FD_SET(fd, &in_fds);
    if (fd > max_in_fd)
	max_in_fd = fd;
}

/*
 * remove_fd - remove an fd from the set that wait_input waits for.
 */
void remove_fd(int fd)
{
    FD_CLR(fd, &in_fds);
}

#if 0
/*
 * wait_loop_output - wait until there is data available on the
 * loopback, for the length of time specified by *timo (indefinite
 * if timo is NULL).
 */
void
wait_loop_output(struct timeval *timo)
{
    fd_set ready;
    int n;

    FD_ZERO(&ready);
    if (loop_master >= FD_SETSIZE)
	fatal("%s: descriptor too big", __func__);
    FD_SET(loop_master, &ready);
    n = select(loop_master + 1, &ready, NULL, &ready, timo);
    if (n < 0 && errno != EINTR)
	fatal("%s: select: %m", __func__);
}


/*
 * wait_time - wait for a given length of time or until a
 * signal is received.
 */
void
wait_time(struct timeval *timo)
{
    int n;

    n = select(0, NULL, NULL, NULL, timo);
    if (n < 0 && errno != EINTR)
	fatal("%s: select: %m", __func__);
}
#endif


/*
 * read_packet - get a PPP packet from the serial device.
 */
int
read_packet(u_char *buf)
{
    int len;

    if ((len = read(ttyfd, buf, PPP_MTU + PPP_HDRLEN)) < 0) {
	if (errno == EWOULDBLOCK || errno == EINTR)
	    return -1;
	fatal("%s: read: %m", __func__);
    }
    return len;
}


/*
 * get_loop_output - read characters from the loopback, form them
 * into frames, and detect when we want to bring the real link up.
 * Return value is 1 if we need to bring up the link, 0 otherwise.
 */
int
get_loop_output(void)
{
    int rv = 0;
    int n;

    while ((n = read(loop_master, inbuf, sizeof(inbuf))) >= 0) {
	if (loop_chars(inbuf, n))
	    rv = 1;
    }

    if (n == 0)
	fatal("%s: eof on loopback", __func__);
    if (n == -1 && errno != EWOULDBLOCK)
	fatal("%s: read from loopback: %m", __func__);

    return rv;
}


/*
 * netif_set_mtu - set the MTU on the PPP network interface.
 */
void
netif_set_mtu(int unit, int mtu)
{
    struct ifreq ifr;

    SYSDEBUG((LOG_DEBUG, "netif_set_mtu: mtu = %d\n", mtu));

    memset(&ifr, '\0', sizeof (ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    ifr.ifr_mtu = mtu;
	
    if (ifunit >= 0 && ioctl(sock_fd, SIOCSIFMTU, (caddr_t) &ifr) < 0)
	fatal("%s: ioctl(SIOCSIFMTU): %m", __func__);
}

/*
 * netif_get_mtu - get the MTU on the PPP network interface.
 */
int
netif_get_mtu(int unit)
{
    struct ifreq ifr;

    memset (&ifr, '\0', sizeof (ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));

    if (ifunit >= 0 && ioctl(sock_fd, SIOCGIFMTU, (caddr_t) &ifr) < 0) {
	error("%s: ioctl(SIOCGIFMTU): %m", __func__);
	return 0;
    }
    return ifr.ifr_mtu;
}

/*
 * tty_send_config - configure the transmit characteristics of
 * the ppp interface.
 */
void
tty_send_config(int mtu, u_int32_t asyncmap, int pcomp, int accomp)
{
    u_int x;
#if 0
    /* Linux code does not do anything with the mtu here */
    ifnet_set_mtu(-1, mtu);
#endif

    if (ioctl(ppp_fd, PPPIOCSASYNCMAP, (caddr_t) &asyncmap) < 0)
	fatal("%s: ioctl(PPPIOCSASYNCMAP): %m", __func__);

    x = get_flags(ppp_fd);
    x = pcomp? x | SC_COMP_PROT: x &~ SC_COMP_PROT;
    x = accomp? x | SC_COMP_AC: x &~ SC_COMP_AC;
    x = sync_serial ? x | SC_SYNC : x & ~SC_SYNC;
    set_flags(ppp_fd, x);
}


/*
 * ppp_set_xaccm - set the extended transmit ACCM for the interface.
 */
void
tty_set_xaccm(ext_accm accm)
{
    if (ioctl(ppp_fd, PPPIOCSXASYNCMAP, accm) < 0 && errno != ENOTTY)
	warn("%s: ioctl(set extended ACCM): %m", __func__);
}


/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.
 */
void
tty_recv_config(int mru, u_int32_t asyncmap, int pcomp, int accomp)
{
    int x;

    if (ioctl(ppp_fd, PPPIOCSMRU, (caddr_t) &mru) < 0)
	fatal("%s: ioctl(PPPIOCSMRU): %m", __func__);
    if (ioctl(ppp_fd, PPPIOCSRASYNCMAP, (caddr_t) &asyncmap) < 0)
	fatal("%s: ioctl(PPPIOCSRASYNCMAP): %m", __func__);
    x = get_flags(ppp_fd);
    x = !accomp? x | SC_REJ_COMP_AC: x &~ SC_REJ_COMP_AC;
    set_flags(ppp_fd, x);
}

/*
 * ccp_test - ask kernel whether a given compression method
 * is acceptable for use.  Returns 1 if the method and parameters
 * are OK, 0 if the method is known but the parameters are not OK
 * (e.g. code size should be reduced), or -1 if the method is unknown.
 */
int
ccp_test(int unit, u_char *opt_ptr, int opt_len, int for_transmit)
{
    struct ppp_option_data data;

    data.ptr = opt_ptr;
    data.length = opt_len;
    data.transmit = for_transmit;
    if (ioctl(ttyfd, PPPIOCSCOMPRESS, (caddr_t) &data) >= 0)
	return 1;
    return (errno == ENOBUFS)? 0: -1;
}

/*
 * ccp_flags_set - inform kernel about the current state of CCP.
 */
void
ccp_flags_set(int unit, int isopen, int isup)
{
    int x;

    x = get_flags(ppp_fd);
    x = isopen? x | SC_CCP_OPEN: x &~ SC_CCP_OPEN;
    x = isup? x | SC_CCP_UP: x &~ SC_CCP_UP;
    set_flags(ppp_fd, x);
}

/*
 * ccp_fatal_error - returns 1 if decompression was disabled as a
 * result of an error detected after decompression of a packet,
 * 0 otherwise.  This is necessary because of patent nonsense.
 */
int
ccp_fatal_error(int unit)
{
    int x;

    x = get_flags(ppp_fd);
    return x & SC_DC_FERROR;
}

/*
 * get_idle_time - return how long the link has been idle.
 */
int
get_idle_time(int u, struct ppp_idle *ip)
{
    return ioctl(ppp_fd, PPPIOCGIDLE, ip) >= 0;
}

/*
 * get_ppp_stats - return statistics for the link.
 */
int
get_ppp_stats(int u, struct pppd_stats *stats)
{
    struct ifpppstatsreq req;

    memset (&req, 0, sizeof (req));
    strlcpy(req.ifr_name, ifname, sizeof(req.ifr_name));
    if (ioctl(sock_fd, SIOCGPPPSTATS, &req) < 0) {
	error("%s: Couldn't get PPP statistics: %m", __func__);
	return 0;
    }
    stats->bytes_in = req.stats.p.ppp_ibytes;
    stats->bytes_out = req.stats.p.ppp_obytes;
    stats->pkts_in = req.stats.p.ppp_ipackets;
    stats->pkts_out = req.stats.p.ppp_opackets;
    return 1;
}


#ifdef PPP_FILTER
/*
 * set_filters - transfer the pass and active filters to the kernel.
 */
int
set_filters(struct bpf_program *pass_in, struct bpf_program *pass_out,
    struct bpf_program *active_in, struct bpf_program *active_out)
{
    int ret = 1;

    if (pass_in->bf_len > 0) {
	if (ioctl(ppp_fd, PPPIOCSIPASS, pass_in) < 0) {
	    error("%s: Couldn't set pass-filter-in in kernel: %m", __func__);
	    ret = 0;
	}
    }

    if (pass_out->bf_len > 0) {
	if (ioctl(ppp_fd, PPPIOCSOPASS, pass_out) < 0) {
	    error("%s: Couldn't set pass-filter-out in kernel: %m", __func__);
	    ret = 0;
	}
    }

    if (active_in->bf_len > 0) {
	if (ioctl(ppp_fd, PPPIOCSIACTIVE, active_in) < 0) {
	    error("%s: Couldn't set active-filter-in in kernel: %m", __func__);
	    ret = 0;
	}
    }

    if (active_out->bf_len > 0) {
	if (ioctl(ppp_fd, PPPIOCSOACTIVE, active_out) < 0) {
	    error("%s: Couldn't set active-filter-out in kernel: %m", __func__);
	    ret = 0;
	}
    }

    return ret;
}
#endif

/*
 * sifvjcomp - config tcp header compression
 */
int
sifvjcomp(int u, int vjcomp, int cidcomp, int maxcid)
{
    u_int x;

    x = get_flags(ppp_fd);
    x = vjcomp ? x | SC_COMP_TCP: x &~ SC_COMP_TCP;
    x = cidcomp? x & ~SC_NO_TCP_CCID: x | SC_NO_TCP_CCID;
    set_flags(ppp_fd, x);
    if (vjcomp && ioctl(ppp_fd, PPPIOCSMAXCID, (caddr_t) &maxcid) < 0) {
	error("%s: ioctl(PPPIOCSMAXCID): %m", __func__);
	return 0;
    }
    return 1;
}

/********************************************************************
 *
 * sifup - Config the interface up and enable IP packets to pass.
 */

int sifup(int u)
{
    int ret;

    if ((ret = setifstate(u, 1)))
	if_is_up++;

    return ret;
}

/********************************************************************
 *
 * sifdown - Disable the indicated protocol and config the interface
 *	     down if there are no remaining protocols.
 */

int sifdown (int u)
{
    if (if_is_up && --if_is_up > 0)
	return 1;

#ifdef INET6
    if (if6_is_up)
	return 1;
#endif /* INET6 */

    return setifstate(u, 0);
}

#ifdef INET6
/********************************************************************
 *
 * sif6up - Config the interface up for IPv6
 */

int sif6up(int u)
{
    int ret;

    if ((ret = setifstate(u, 1)))
	if6_is_up = 1;

    return ret;
}

/********************************************************************
 *
 * sif6down - Disable the IPv6CP protocol and config the interface
 *	      down if there are no remaining protocols.
 */

int sif6down (int u)
{
    if6_is_up = 0;

    if (if_is_up)
	return 1;

    return setifstate(u, 0);
}
#endif /* INET6 */

/********************************************************************
 *
 * setifstate - Config the interface up or down
 */

static int setifstate (int u, int state)
{
    struct ifreq ifr;

    strlcpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));
    if (ioctl(sock_fd, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
	error("%s: ioctl (SIOCGIFFLAGS): %m", __func__);
	return 0;
    }
    if (state)
	ifr.ifr_flags |= IFF_UP;
    else
	ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(sock_fd, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
	error("%s: ioctl(SIOCSIFFLAGS): %m", __func__);
	return 0;
    }
    if_is_up = 1;
    return 1;
}

/*
 * sifnpmode - Set the mode for handling packets for a given NP.
 */
int
sifnpmode(int u, int proto, enum NPmode mode)
{
    struct npioctl npi;

    npi.protocol = proto;
    npi.mode = mode;
    if (ioctl(ppp_fd, PPPIOCSNPMODE, &npi) < 0) {
	error("%s: ioctl(set NP %d mode to %d): %m", __func__, proto, mode);
	return 0;
    }
    return 1;
}

/*
 * SET_SA_FAMILY - set the sa_family field of a struct sockaddr,
 * if it exists.
 */
#define SET_SA_FAMILY(addr, family)		\
    BZERO((char *) &(addr), sizeof(addr));	\
    addr.sa_family = (family); 			\
    addr.sa_len = sizeof(addr);

/*
 * sifaddr - Config the interface IP addresses and netmask.
 */
int
sifaddr(int u, u_int32_t o, u_int32_t h, u_int32_t m)
{
    struct ifaliasreq ifra;
    struct ifreq ifr;

    strlcpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name));
    SET_SA_FAMILY(ifra.ifra_addr, AF_INET);
    ((struct sockaddr_in *) &ifra.ifra_addr)->sin_addr.s_addr = o;
    SET_SA_FAMILY(ifra.ifra_broadaddr, AF_INET);
    ((struct sockaddr_in *) &ifra.ifra_broadaddr)->sin_addr.s_addr = h;
    if (m != 0) {
	SET_SA_FAMILY(ifra.ifra_mask, AF_INET);
	((struct sockaddr_in *) &ifra.ifra_mask)->sin_addr.s_addr = m;
    } else
	BZERO(&ifra.ifra_mask, sizeof(ifra.ifra_mask));
    BZERO(&ifr, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(sock_fd, SIOCDIFADDR, (caddr_t) &ifr) < 0) {
	if (errno != EADDRNOTAVAIL)
	    warn("%s: Couldn't remove interface address: %m", __func__);
    }
    if (ioctl(sock_fd, SIOCAIFADDR, (caddr_t) &ifra) < 0) {
	if (errno != EEXIST) {
	    error("%s: Couldn't set interface address: %m", __func__);
	    return 0;
	}
	warn("%s: Couldn't set interface address: Address %I already exists",
	    __func__, o);
    }
    ifaddrs[0] = o;
    ifaddrs[1] = h;
    return 1;
}

/*
 * cifaddr - Clear the interface IP addresses, and delete routes
 * through the interface if possible.
 */
int
cifaddr(int u, u_int32_t o, u_int32_t h)
{
    struct ifaliasreq ifra;

    ifaddrs[0] = 0;
    strlcpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name));
    SET_SA_FAMILY(ifra.ifra_addr, AF_INET);
    ((struct sockaddr_in *) &ifra.ifra_addr)->sin_addr.s_addr = o;
    SET_SA_FAMILY(ifra.ifra_broadaddr, AF_INET);
    ((struct sockaddr_in *) &ifra.ifra_broadaddr)->sin_addr.s_addr = h;
    BZERO(&ifra.ifra_mask, sizeof(ifra.ifra_mask));
    if (ioctl(sock_fd, SIOCDIFADDR, (caddr_t) &ifra) < 0) {
	if (!doing_cleanup && errno != EADDRNOTAVAIL)
	    warn("%s: Couldn't delete interface address: %m", __func__);
	return 0;
    }
    return 1;
}

/*
 * sifdefaultroute - assign a default route through the address given.
 */
int
sifdefaultroute(int u, u_int32_t l, u_int32_t g)
{
    return dodefaultroute(g, 's');
}

/*
 * cifdefaultroute - delete a default route through the address given.
 */
int
cifdefaultroute(int u, u_int32_t l, u_int32_t g)
{
    return dodefaultroute(g, 'c');
}

/*
 * dodefaultroute - talk to a routing socket to add/delete a default route.
 */
static int
dodefaultroute(u_int32_t g, int cmd)
{
    int routes;
    struct {
	struct rt_msghdr	hdr;
	struct sockaddr_in	dst;
	struct sockaddr_in	gway;
	struct sockaddr_in	netmask;
	struct sockaddr_dl	ifp;
    } rtmsg;

    if ((routes = socket(PF_ROUTE, SOCK_RAW, AF_INET)) < 0) {
	if (!doing_cleanup)
	    error("%s: Couldn't %s default route: socket: %m", __func__,
		cmd == 's' ? "add" : "delete");
	return 0;
    }

    memset(&rtmsg, 0, sizeof(rtmsg));

    rtmsg.hdr.rtm_type = cmd == 's' ? RTM_ADD : RTM_DELETE;
    rtmsg.hdr.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;
    rtmsg.hdr.rtm_version = RTM_VERSION;
    rtmsg.hdr.rtm_seq = ++rtm_seq;
    rtmsg.hdr.rtm_addrs =
	RTA_DST | RTA_GATEWAY | RTA_NETMASK | RTA_IFP;

    rtmsg.dst.sin_len = sizeof(rtmsg.dst);
    rtmsg.dst.sin_family = AF_INET;
    rtmsg.dst.sin_addr.s_addr = 0;

    rtmsg.gway.sin_len = sizeof(rtmsg.gway);
    rtmsg.gway.sin_family = AF_INET;
    rtmsg.gway.sin_addr.s_addr = g;

    rtmsg.netmask.sin_len = sizeof(rtmsg.netmask);
    rtmsg.netmask.sin_family = AF_INET;
    rtmsg.netmask.sin_addr.s_addr = 0;

    rtmsg.ifp.sdl_family = AF_LINK;
    rtmsg.ifp.sdl_len = sizeof(rtmsg.ifp);
    link_addr(ifname, &rtmsg.ifp);

    rtmsg.hdr.rtm_msglen = sizeof(rtmsg);

    if (write(routes, &rtmsg, sizeof(rtmsg)) < 0) {
	if (!doing_cleanup)
	    error("%s: Couldn't %s default route: %m", __func__,
		cmd == 's' ? "add" : "delete");
	close(routes);
	return 0;
    }

    close(routes);
    default_route_gateway = (cmd == 's') ? g : 0;
    return 1;
}

#if RTM_VERSION >= 3

/*
 * sifproxyarp - Make a proxy ARP entry for the peer.
 */
static struct {
    struct rt_msghdr		hdr;
    struct sockaddr_inarp	dst;
    struct sockaddr_dl		hwa;
    char			extra[128];
} arpmsg;

static int arpmsg_valid;

int
sifproxyarp(int unit, u_int32_t hisaddr)
{
    int routes;

    /*
     * Get the hardware address of an interface on the same subnet
     * as our local address.
     */
    memset(&arpmsg, 0, sizeof(arpmsg));
    if (!get_ether_addr(hisaddr, &arpmsg.hwa)) {
	error("%s: Cannot determine ethernet address for proxy ARP", __func__);
	return 0;
    }

    if ((routes = socket(PF_ROUTE, SOCK_RAW, AF_INET)) < 0) {
	error("%s: Couldn't add proxy arp entry: socket: %m", __func__);
	return 0;
    }

    arpmsg.hdr.rtm_type = RTM_ADD;
    arpmsg.hdr.rtm_flags = RTF_ANNOUNCE | RTF_HOST | RTF_STATIC | RTF_LLDATA;
    arpmsg.hdr.rtm_version = RTM_VERSION;
    arpmsg.hdr.rtm_seq = ++rtm_seq;
    arpmsg.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY;
    arpmsg.hdr.rtm_inits = RTV_EXPIRE;
    arpmsg.dst.sin_len = sizeof(struct sockaddr_inarp);
    arpmsg.dst.sin_family = AF_INET;
    arpmsg.dst.sin_addr.s_addr = hisaddr;
    arpmsg.dst.sin_other = SIN_PROXY;

    arpmsg.hdr.rtm_msglen = (char *) &arpmsg.hwa - (char *) &arpmsg
	+ RT_ROUNDUP(arpmsg.hwa.sdl_len);
    if (write(routes, &arpmsg, arpmsg.hdr.rtm_msglen) < 0) {
	error("%s: Couldn't add proxy arp entry: %m", __func__);
	close(routes);
	return 0;
    }

    close(routes);
    arpmsg_valid = 1;
    proxy_arp_addr = hisaddr;
    return 1;
}

/*
 * cifproxyarp - Delete the proxy ARP entry for the peer.
 */
int
cifproxyarp(int unit, u_int32_t hisaddr)
{
    int routes;

    if (!arpmsg_valid)
	return 0;
    arpmsg_valid = 0;

    arpmsg.hdr.rtm_type = RTM_DELETE;
    arpmsg.hdr.rtm_seq = ++rtm_seq;

    if ((routes = socket(PF_ROUTE, SOCK_RAW, AF_INET)) < 0) {
	if (!doing_cleanup)
	    error("%s: Couldn't delete proxy arp entry: socket: %m", __func__);
	return 0;
    }

    if (write(routes, &arpmsg, arpmsg.hdr.rtm_msglen) < 0) {
	if (!doing_cleanup)
	    error("%s: Couldn't delete proxy arp entry: %m", __func__);
	close(routes);
	return 0;
    }

    close(routes);
    proxy_arp_addr = 0;
    return 1;
}

#else	/* RTM_VERSION */

/*
 * sifproxyarp - Make a proxy ARP entry for the peer.
 */
int
sifproxyarp(int unit, u_int32_t hisaddr)
{
    struct arpreq arpreq;
    struct {
	struct sockaddr_dl	sdl;
	char			space[128];
    } dls;

    BZERO(&arpreq, sizeof(arpreq));

    /*
     * Get the hardware address of an interface on the same subnet
     * as our local address.
     */
    if (!get_ether_addr(hisaddr, &dls.sdl)) {
	error("%s: Cannot determine ethernet address for proxy ARP", __func__);
	return 0;
    }

    arpreq.arp_ha.sa_len = sizeof(struct sockaddr);
    arpreq.arp_ha.sa_family = AF_UNSPEC;
    BCOPY(LLADDR(&dls.sdl), arpreq.arp_ha.sa_data, dls.sdl.sdl_alen);
    SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
    ((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = hisaddr;
    arpreq.arp_flags = ATF_PERM | ATF_PUBL;
    if (ioctl(sock_fd, SIOCSARP, (caddr_t)&arpreq) < 0) {
	error("%s: Couldn't add proxy arp entry: %m", __func__);
	return 0;
    }

    proxy_arp_addr = hisaddr;
    return 1;
}

/*
 * cifproxyarp - Delete the proxy ARP entry for the peer.
 */
int
cifproxyarp(int unit, u_int32_t hisaddr)
{
    struct arpreq arpreq;

    BZERO(&arpreq, sizeof(arpreq));
    SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
    ((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = hisaddr;
    if (ioctl(sock_fd, SIOCDARP, (caddr_t)&arpreq) < 0) {
	warn("%s: Couldn't delete proxy arp entry: %m", __func__);
	return 0;
    }
    proxy_arp_addr = 0;
    return 1;
}
#endif	/* RTM_VERSION */


/*
 * get_ether_addr - get the hardware address of an interface on the
 * the same subnet as ipaddr.
 */
static int
get_ether_addr(u_int32_t ipaddr, struct sockaddr_dl *hwaddr)
{
    u_int32_t ina, mask;
    struct sockaddr_dl *dla;
    struct ifaddrs *ifap, *ifa, *ifp;

    /*
     * Scan through looking for an interface with an Internet
     * address on the same subnet as `ipaddr'.
     */
    if (getifaddrs(&ifap) != 0) {
	error("%s: getifaddrs: %m", __func__);
	return 0;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	if (ifa->ifa_addr->sa_family != AF_INET)
	    continue;
	ina = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
	/*
	 * Check that the interface is up, and not point-to-point
	 * or loopback.
	 */
	if ((ifa->ifa_flags &
	     (IFF_UP|IFF_BROADCAST|IFF_POINTOPOINT|IFF_LOOPBACK|IFF_NOARP))
	     != (IFF_UP|IFF_BROADCAST))
	    continue;
	/*
	 * Get its netmask and check that it's on the right subnet.
	 */
	mask = ((struct sockaddr_in *) ifa->ifa_netmask)->sin_addr.s_addr;
	if ((ipaddr & mask) != (ina & mask))
	    continue;
	break;
    }

    if (!ifa) {
	freeifaddrs(ifap);
	return 0;
    }
    info("found interface %s for proxy arp", ifa->ifa_name);

    ifp = ifa;

    /*
     * Now scan through again looking for a link-level address
     * for this interface.
     */
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	if (strcmp(ifp->ifa_name, ifa->ifa_name) != 0)
	    continue;
	if (ifa->ifa_addr->sa_family != AF_LINK)
	    continue;
	/*
	 * Found the link-level address - copy it out
	 */
	dla = (struct sockaddr_dl *) ifa->ifa_addr;
	BCOPY(dla, hwaddr, dla->sdl_len);
	freeifaddrs(ifap);
	return 1;
    }

    freeifaddrs(ifap);
    return 0;
}

/*
 * get_if_hwaddr - get the hardware address for the specified
 * network interface device.
 */
int
get_if_hwaddr(u_char *addr, char *name)
{

#define IFREQ_SAFE (sizeof(struct ifreq) + sizeof(struct sockaddr_dl))
    /* XXX sockaddr_dl is larger than the sockaddr in struct ifreq! */
    union {			/* XXX */
    	struct ifreq _ifreq;	/* XXX */
	char _X[IFREQ_SAFE]; 	/* XXX */
    } _ifreq_dontsmashstack;	/* XXX */
#define ifreq_xxx _ifreq_dontsmashstack._ifreq			/* XXX */

    struct sockaddr_dl *sdl = (struct sockaddr_dl *) &ifreq_xxx.ifr_addr;
    int fd;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	return 0;
    (void)memset(sdl, 0, sizeof(*sdl));
    sdl->sdl_family = AF_LINK;
    (void)strlcpy(ifreq_xxx.ifr_name, name, sizeof(ifreq_xxx.ifr_name));
    if (ioctl(fd, SIOCGIFADDR, &ifreq_xxx) == -1) {
	(void)close(fd);
	return 0;
    }
    (void)close(fd);
    (void)memcpy(addr, LLADDR(sdl), sdl->sdl_alen);
    return sdl->sdl_nlen;
}

/*
 * get_first_ethernet - return the name of the first ethernet-style
 * interface on this system.
 */
char *
get_first_ethernet(void)
{
    static char ifname[IFNAMSIZ];
    struct ifaddrs *ifap, *ifa;

    /*
     * Scan through the system's network interfaces.
     */
    if (getifaddrs(&ifap) != 0) {
	warn("%s: getifaddrs: %m", __func__);
	return NULL;
    }
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	/*
	 * Check the interface's internet address.
	 */
	if (ifa->ifa_addr->sa_family != AF_INET)
	    continue;
	/*
	 * Check that the interface is up, and not point-to-point or loopback.
	 */
	if ((ifa->ifa_flags & (IFF_UP|IFF_POINTOPOINT|IFF_LOOPBACK))
	    != IFF_UP) {
	    strlcpy(ifname, ifa->ifa_name, sizeof(ifname));
	    freeifaddrs(ifap);
	    return ifname;
	}
    }
    freeifaddrs(ifap);
    return NULL;
}

/*
 * Return user specified netmask, modified by any mask we might determine
 * for address `addr' (in network byte order).
 * Here we scan through the system's list of interfaces, looking for
 * any non-point-to-point interfaces which might appear to be on the same
 * network as `addr'.  If we find any, we OR in their netmask to the
 * user-specified netmask.
 */
u_int32_t
GetMask(u_int32_t addr)
{
    u_int32_t mask, nmask, ina;
    struct ifaddrs *ifap, *ifa;

    addr = ntohl(addr);
    if (IN_CLASSA(addr))	/* determine network mask for address class */
	nmask = IN_CLASSA_NET;
    else if (IN_CLASSB(addr))
	nmask = IN_CLASSB_NET;
    else
	nmask = IN_CLASSC_NET;
    /* class D nets are disallowed by bad_ip_adrs */
    mask = netmask | htonl(nmask);

    /*
     * Scan through the system's network interfaces.
     */
    if (getifaddrs(&ifap) != 0) {
	warn("%s: getifaddrs: %m", __func__);
	return 0;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	/*
	 * Check the interface's internet address.
	 */
	if (ifa->ifa_addr->sa_family != AF_INET)
	    continue;
	ina = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
	if ((ntohl(ina) & nmask) != (addr & nmask))
	    continue;
	/*
	 * Check that the interface is up, and not point-to-point or loopback.
	 */
	if ((ifa->ifa_flags & (IFF_UP|IFF_POINTOPOINT|IFF_LOOPBACK)) != IFF_UP)
	    continue;
	/*
	 * Get its netmask and OR it into our mask.
	 */
	mask |= ((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr.s_addr;
    }

    freeifaddrs(ifap);
    return mask;
}

/*
 * have_route_to - determine if the system has any route to
 * a given IP address.
 * For demand mode to work properly, we have to ignore routes
 * through our own interface.
 */
int have_route_to(u_int32_t addr)
{
    return -1;
}

/*
 * Use the hostid as part of the random number seed.
 */
int
get_host_seed(void)
{
    return gethostid();
}

#if 0
/*
 * lock - create a lock file for the named lock device
 */
#define	LOCK_PREFIX	"/var/spool/lock/LCK.."

static char *lock_file;		/* name of lock file created */

int
lock(char *dev)
{
    char hdb_lock_buffer[12];
    int fd, pid, n;
    char *p;
    size_t l;

    if ((p = strrchr(dev, '/')) != NULL)
	dev = p + 1;
    l = strlen(LOCK_PREFIX) + strlen(dev) + 1;
    lock_file = malloc(l);
    if (lock_file == NULL)
	novm("lock file name");
    slprintf(lock_file, l, "%s%s", LOCK_PREFIX, dev);

    while ((fd = open(lock_file, O_EXCL | O_CREAT | O_RDWR, 0644)) < 0) {
	if (errno == EEXIST
	    && (fd = open(lock_file, O_RDONLY, 0)) >= 0) {
	    /* Read the lock file to find out who has the device locked */
	    n = read(fd, hdb_lock_buffer, 11);
	    if (n <= 0) {
		error("%s: Can't read pid from lock file %s", __func__,
		    lock_file);
		close(fd);
	    } else {
		hdb_lock_buffer[n] = 0;
		pid = atoi(hdb_lock_buffer);
		if (kill(pid, 0) == -1 && errno == ESRCH) {
		    /* pid no longer exists - remove the lock file */
		    if (unlink(lock_file) == 0) {
			close(fd);
			notice("%s: Removed stale lock on %s (pid %d)",
			    __func__, dev, pid);
			continue;
		    } else
			warn("%s: Couldn't remove stale lock on %s", __func__,
			    dev);
		} else
		    notice("%s: Device %s is locked by pid %d", __func__,
			   dev, pid);
	    }
	    close(fd);
	} else
	    error("%s: Can't create lock file %s: %m", __func__, lock_file);
	free(lock_file);
	lock_file = NULL;
	return -1;
    }

    slprintf(hdb_lock_buffer, sizeof(hdb_lock_buffer), "%10d\n", getpid());
    write(fd, hdb_lock_buffer, 11);

    close(fd);
    return 0;
}

/*
 * unlock - remove our lockfile
 */
void
unlock(void)
{
    if (lock_file) {
	unlink(lock_file);
	free(lock_file);
	lock_file = NULL;
    }
}
#endif

#ifdef INET6
/*
 * ether_to_eui64 - Convert 48-bit Ethernet address into 64-bit EUI
 *
 * convert the 48-bit MAC address of eth0 into EUI 64. caller also assumes
 * that the system has a properly configured Ethernet interface for this
 * function to return non-zero.
 */
int
ether_to_eui64(eui64_t *p_eui64)
{
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) != 0) {
	warn("%s: getifaddrs: %m", __func__);
	return 0;
    }
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	/*
	 * Check the interface's internet address.
	 */
	if (ifa->ifa_addr->sa_family != AF_LINK)
	    continue;
	/*
	 * Check that the interface is up, and not point-to-point or loopback.
	 */
	if ((ifa->ifa_flags & (IFF_UP|IFF_POINTOPOINT|IFF_LOOPBACK)) == IFF_UP)
	{
	    /*
	     * And convert the EUI-48 into EUI-64, per RFC 2472 [sec 4.1]
	     */
	    unsigned char *ptr = (void *)ifa->ifa_addr;
	    p_eui64->e8[0] = ptr[0] | 0x02;
	    p_eui64->e8[1] = ptr[1];
	    p_eui64->e8[2] = ptr[2];
	    p_eui64->e8[3] = 0xFF;
	    p_eui64->e8[4] = 0xFE;
	    p_eui64->e8[5] = ptr[3];
	    p_eui64->e8[6] = ptr[4];
	    p_eui64->e8[7] = ptr[5];
	    freeifaddrs(ifap);
	    return 1;
	}
    }
    warn("%s: can't find a link address", __func__);
    freeifaddrs(ifap);

    return 0;
}
#endif
