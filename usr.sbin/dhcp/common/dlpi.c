/* dlpi.c
 
   Data Link Provider Interface (DLPI) network interface code. */

/*
 * Copyright (c) 1998, 1999 The Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Eric James Negaard, <lmdejn@lmd.ericsson.se>.  To learn more about
 * the Internet Software Consortium, see ``http://www.vix.com/isc''.
 */

/*
 * Based largely in part to the existing NIT code in nit.c.
 *
 * This code has been developed and tested on sparc-based machines running
 * SunOS 5.5.1, with le and hme network interfaces.  It should be pretty
 * generic, though.
 */

/*
 * Implementation notes:
 *
 * I first tried to write this code to the "vanilla" DLPI 2.0 API.
 * It worked on a Sun Ultra-1 with a hme interface, but didn't work
 * on Sun SparcStation 5's with "le" interfaces (the packets sent out
 * via dlpiunitdatareq contained an Ethernet type of 0x0000 instead
 * of the expected 0x0800).
 *
 * Therefore I added the "DLPI_RAW" code which is a Sun extension to
 * the DLPI standard.  This code works on both of the above machines.
 * This is configurable in the OS-dependent include file by defining
 * USE_DLPI_RAW.
 *
 * It quickly became apparant that I should also use the "pfmod"
 * STREAMS module to cut down on the amount of user level packet
 * processing.  I don't know how widely available "pfmod" is, so it's
 * use is conditionally included. This is configurable in the
 * OS-dependent include file by defining USE_DLPI_PFMOD.
 *
 * A major quirk on the Sun's at least, is that no packets seem to get
 * sent out the interface until six seconds after the interface is
 * first "attached" to [per system reboot] (it's actually from when
 * the interface is attached, not when it is plumbed, so putting a
 * sleep into the dhclient-script at PREINIT time doesn't help).  I
 * HAVE tried, without success to poll the fd to see when it is ready
 * for writing.  This doesn't help at all. If the sleeps are not done,
 * the initial DHCPREQUEST or DHCPDISCOVER never gets sent out, so
 * I've put them here, when register_send and register_receive are
 * called (split up into two three-second sleeps between the notices,
 * so that it doesn't seem like so long when you're watching :-).  The
 * amount of time to sleep is configurable in the OS-dependent include
 * file by defining DLPI_FIRST_SEND_WAIT to be the number of seconds
 * to sleep.
 */

#include "dhcpd.h"

#if defined (USE_DLPI_SEND) || defined (USE_DLPI_RECEIVE)

# include <sys/ioctl.h>
# include <sys/time.h>
# include <sys/dlpi.h>
# include <stropts.h>
# ifdef USE_DLPI_PFMOD
#  include <sys/pfmod.h>
# endif
# ifdef USE_POLL
#  include <poll.h>
# endif

# include <netinet/in_systm.h>
# include "includes/netinet/ip.h"
# include "includes/netinet/udp.h"
# include "includes/netinet/if_ether.h"

# ifdef USE_DLPI_PFMOD
#  ifdef USE_DLPI_RAW
#   define DLPI_MODNAME "DLPI+RAW+PFMOD"
#  else
#   define DLPI_MODNAME "DLPI+PFMOD"
#  endif
# else
#  ifdef USE_DLPI_RAW
#   define DLPI_MODNAME "DLPI+RAW"
#  else
#   define DLPI_MODNAME "DLPI"
#  endif
# endif

# ifndef ABS
#  define ABS(x) ((x) >= 0 ? (x) : 0-(x))
# endif

static int strioctl PROTO ((int fd, int cmd, int timeout, int len, char *dp));

#define DLPI_MAXDLBUF		8192	/* Buffer size */
#define DLPI_MAXDLADDR		1024	/* Max address size */
#define DLPI_DEVDIR		"/dev/"	/* Device directory */
#define DLPI_DEFAULTSAP		0x0800	/* IP protocol */

static void dlpi_makeaddr PROTO ((unsigned char *physaddr, int physaddrlen,
				  unsigned char *sap, int saplen,
				  unsigned char *buf));
static void dlpi_parseaddr PROTO ((unsigned char *buf,
				   unsigned char *physaddr,
				   int physaddrlen, unsigned char *sap,
				   int saplen));
static int dlpiopen PROTO ((char *ifname));
static int dlpiunit PROTO ((char *ifname));
static int dlpiinforeq PROTO ((int fd));
static int dlpiphysaddrreq PROTO ((int fd, unsigned long addrtype));
static int dlpiattachreq PROTO ((int fd, unsigned long ppa));
static int dlpibindreq PROTO ((int fd, unsigned long sap, unsigned long max_conind,
			       unsigned long service_mode, unsigned long conn_mgmt,
			       unsigned long xidtest));
static int dlpidetachreq PROTO ((int fd));
static int dlpiunbindreq PROTO ((int fd));
static int dlpiokack PROTO ((int fd, char *bufp));
static int dlpiinfoack PROTO ((int fd, char *bufp));
static int dlpiphysaddrack PROTO ((int fd, char *bufp));
static int dlpibindack PROTO ((int fd, char *bufp));
static int dlpiunitdatareq PROTO ((int fd, unsigned char *addr,
				   int addrlen, unsigned long minpri,
				   unsigned long maxpri, unsigned char *data,
				   int datalen));
static int dlpiunitdataind PROTO ((int fd,
				   unsigned char *dstaddr,
				   unsigned long *dstaddrlen,
				   unsigned char *srcaddr,
				   unsigned long *srcaddrlen,
				   unsigned long *grpaddr,
				   unsigned char *data,
				   int datalen));

# ifndef USE_POLL
static void	sigalrm PROTO ((int sig));
# endif
static int	expected PROTO ((unsigned long prim, union DL_primitives *dlp,
				  int msgflags));
static int	strgetmsg PROTO ((int fd, struct strbuf *ctlp,
				  struct strbuf *datap, int *flagsp,
				  char *caller));

/* Reinitializes the specified interface after an address change.   This
   is not required for packet-filter APIs. */

#ifdef USE_DLPI_SEND
void if_reinitialize_send (info)
	struct interface_info *info;
{
}
#endif

#ifdef USE_DLPI_RECEIVE
void if_reinitialize_receive (info)
	struct interface_info *info;
{
}
#endif

/* Called by get_interface_list for each interface that's discovered.
   Opens a packet filter for each interface and adds it to the select
   mask. */

int if_register_dlpi (info)
	struct interface_info *info;
{
	int sock;
	int unit;
	long buf [DLPI_MAXDLBUF];
	union DL_primitives *dlp;

	dlp = (union DL_primitives *)buf;

	/* Open a DLPI device */
	if ((sock = dlpiopen (info -> name)) < 0) {
	    error ("Can't open DLPI device for %s: %m", info -> name);
	}

	/*
	 * Get information about the provider.
	 */

	/*
	 * Submit a DL_INFO_REQ request, to find
	 * the dl_mac_type and dl_provider_style
	 */
	if (dlpiinforeq(sock) < 0 || dlpiinfoack(sock, (char *)buf) < 0) {
	    error ("Can't get DLPI MAC type for %s: %m", info -> name);
	} else {
	    switch (dlp -> info_ack.dl_mac_type) {
	      case DL_CSMACD: /* IEEE 802.3 */
	      case DL_ETHER:
		info -> hw_address.htype = HTYPE_ETHER;
		break;
	      /* adding token ring 5/1999 - mayer@ping.at  */ 
	      case DL_TPR:
		info -> hw_address.htype = HTYPE_IEEE802;
		break;
	      case DL_FDDI:
		info -> hw_address.htype = HTYPE_FDDI;
		break;
	      default:
		error ("%s: unknown DLPI MAC type %d",
		       info -> name,
		       dlp -> info_ack.dl_mac_type);
		break;
	    }
	}

	if (dlp -> info_ack.dl_provider_style == DL_STYLE2) {
	    /*
	     * Attach to the device.  If this fails, the device
	     * does not exist.
	     */
	    unit = dlpiunit (info -> name);
	
	    if (dlpiattachreq (sock, unit) < 0
		|| dlpiokack (sock, (char *)buf) < 0) {
		error ("Can't attach DLPI device for %s: %m", info -> name);
	    }
	}

	/*
	 * Bind to the IP service access point (SAP), connectionless (CLDLS).
	 */
	if (dlpibindreq (sock, DLPI_DEFAULTSAP, 0, DL_CLDLS, 0, 0) < 0
	    || dlpibindack (sock, (char *)buf) < 0) {
	    error ("Can't bind DLPI device for %s: %m", info -> name);
	}

	/*
	 * Submit a DL_PHYS_ADDR_REQ request, to find
	 * the hardware address
	 */
	if (dlpiphysaddrreq (sock, DL_CURR_PHYS_ADDR) < 0
	    || dlpiphysaddrack (sock, (char *)buf) < 0) {
	    error ("Can't get DLPI hardware address for %s: %m",
		   info -> name);
	}

	info -> hw_address.hlen = dlp -> physaddr_ack.dl_addr_length;
	memcpy (info -> hw_address.haddr,
		(char *)buf + dlp -> physaddr_ack.dl_addr_offset,
		dlp -> physaddr_ack.dl_addr_length);

#ifdef USE_DLPI_RAW
	if (strioctl (sock, DLIOCRAW, INFTIM, 0, 0) < 0) {
	    error ("Can't set DLPI RAW mode for %s: %m",
		   info -> name);
	}
#endif

#ifdef USE_DLPI_PFMOD
	if (ioctl (sock, I_PUSH, "pfmod") < 0) {
	    error ("Can't push packet filter onto DLPI for %s: %m",
		   info -> name);
	}
#endif

	return sock;
}

static int
strioctl (fd, cmd, timeout, len, dp)
int fd;
int cmd;
int timeout;
int len;
char *dp;
{
    struct strioctl sio;
    int rslt;

    sio.ic_cmd = cmd;
    sio.ic_timout = timeout;
    sio.ic_len = len;
    sio.ic_dp = dp;

    if ((rslt = ioctl (fd, I_STR, &sio)) < 0) {
	return rslt;
    } else {
	return sio.ic_len;
    }
}

#ifdef USE_DLPI_SEND
void if_register_send (info)
	struct interface_info *info;
{
	/* If we're using the DLPI API for sending and receiving,
	   we don't need to register this interface twice. */
#ifndef USE_DLPI_RECEIVE
# ifdef USE_DLPI_PFMOD
	struct packetfilt pf;
# endif

	info -> wfdesc = if_register_dlpi (info);

# ifdef USE_DLPI_PFMOD
	/* Set up an PFMOD filter that rejects everything... */
	pf.Pf_Priority = 0;
	pf.Pf_FilterLen = 1;
	pf.Pf_Filter [0] = ENF_PUSHZERO;

	/* Install the filter */
	if (strioctl (info -> wfdesc, PFIOCSETF, INFTIM,
		      sizeof (pf), (char *)&pf) < 0) {
	    error ("Can't set PFMOD send filter on %s: %m", info -> name);
	}

# endif /* USE_DLPI_PFMOD */
#else /* !defined (USE_DLPI_RECEIVE) */
	/*
	 * If using DLPI for both send and receive, simply re-use
	 * the read file descriptor that was set up earlier.
	 */
	info -> wfdesc = info -> rfdesc;
#endif

        if (!quiet_interface_discovery)
		note ("Sending on   DLPI/%s/%s%s%s",
		      info -> name,
		      print_hw_addr (info -> hw_address.htype,
				     info -> hw_address.hlen,
				     info -> hw_address.haddr),
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));

#ifdef DLPI_FIRST_SEND_WAIT
/* See the implementation notes at the beginning of this file */
# ifdef USE_DLPI_RECEIVE
	sleep (DLPI_FIRST_SEND_WAIT - (DLPI_FIRST_SEND_WAIT / 2));
# else
	sleep (DLPI_FIRST_SEND_WAIT);
# endif
#endif
}
#endif /* USE_DLPI_SEND */

#ifdef USE_DLPI_RECEIVE
/* Packet filter program...
   XXX Changes to the filter program may require changes to the constant
   offsets used in if_register_send to patch the NIT program! XXX */

void if_register_receive (info)
	struct interface_info *info;
{
#ifdef USE_DLPI_PFMOD
	struct packetfilt pf;
#endif

	/* Open a DLPI device and hang it on this interface... */
	info -> rfdesc = if_register_dlpi (info);

#ifdef USE_DLPI_PFMOD
	/* Set up the PFMOD filter program. */
	/* XXX Unlike the BPF filter program, this one won't work if the
	   XXX IP packet is fragmented or if there are options on the IP
	   XXX header. */
	pf.Pf_Priority = 0;
	pf.Pf_FilterLen = 0;

#ifdef USE_DLPI_RAW
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 6;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (ETHERTYPE_IP);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 11;
#if defined(i386) || defined(__i386)
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHFF00 + ENF_AND;
#else
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSH00FF + ENF_AND;
#endif
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (IPPROTO_UDP);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 18;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = local_port;
#else
	/*
	 * The packets that will be received on this file descriptor
	 * will be IP packets (due to the SAP that was specified in
	 * the dlbind call).  There will be no ethernet header.
	 * Therefore, setup the packet filter to check the protocol
	 * field for UDP, and the destination port number equal
	 * to the local port.  All offsets are relative to the start
	 * of an IP packet.
	 */
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 4;
#if defined(i386) || defined(__i386)
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHFF00 + ENF_AND;
#else
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSH00FF + ENF_AND;
#endif
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = htons (IPPROTO_UDP);
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHWORD + 11;
	pf.Pf_Filter [pf.Pf_FilterLen++] = ENF_PUSHLIT + ENF_CAND;
	pf.Pf_Filter [pf.Pf_FilterLen++] = local_port;
#endif

	/* Install the filter... */
	if (strioctl (info -> rfdesc, PFIOCSETF, INFTIM,
		      sizeof (pf), (char *)&pf) < 0) {
	    error ("Can't set PFMOD receive filter on %s: %m", info -> name);
	}
#endif

        if (!quiet_interface_discovery)
		note ("Listening on DLPI/%s/%s%s%s",
		      info -> name,
		      print_hw_addr (info -> hw_address.htype,
				     info -> hw_address.hlen,
				     info -> hw_address.haddr),
		      (info -> shared_network ? "/" : ""),
		      (info -> shared_network ?
		       info -> shared_network -> name : ""));

#ifdef DLPI_FIRST_SEND_WAIT
/* See the implementation notes at the beginning of this file */
# ifdef USE_DLPI_SEND
	sleep (DLPI_FIRST_SEND_WAIT / 2);
# else
	sleep (DLPI_FIRST_SEND_WAIT);
# endif
#endif
}
#endif /* USE_DLPI_RECEIVE */

#ifdef USE_DLPI_SEND
ssize_t send_packet (interface, packet, raw, len, from, to, hto)
	struct interface_info *interface;
	struct packet *packet;
	struct dhcp_packet *raw;
	size_t len;
	struct in_addr from;
	struct sockaddr_in *to;
	struct hardware *hto;
{
	int dbuflen;
	unsigned char dbuf [1536];
	unsigned char sap [2];
	unsigned char dstaddr [DLPI_MAXDLADDR];
	unsigned addrlen;
	int saplen;
	int result;

	if (!strcmp (interface -> name, "fallback"))
		return send_fallback (interface, packet, raw,
				      len, from, to, hto);

	dbuflen = 0;

	/* Assemble the headers... */
#ifdef USE_DLPI_RAW
	assemble_hw_header (interface, dbuf, &dbuflen, hto);
#endif
	assemble_udp_ip_header (interface, dbuf, &dbuflen, from.s_addr,
				to -> sin_addr.s_addr, to -> sin_port,
				(unsigned char *)raw, len);

	/* Copy the data into the buffer (yuk). */
	memcpy (dbuf + dbuflen, raw, len);
	dbuflen += len;

#ifdef USE_DLPI_RAW
	result = write (interface -> wfdesc, dbuf, dbuflen);
#else
	/* XXX: Assumes ethernet, with two byte SAP */
	sap [0] = 0x08;		/* ETHERTYPE_IP, high byte */
	sap [1] = 0x0;		/* ETHERTYPE_IP, low byte */
	saplen = -2;		/* -2 indicates a two byte SAP at the end
				   of the address */

	/* Setup the destination address */
	if (hto && hto -> hlen == interface -> hw_address.hlen) {
	    dlpi_makeaddr (hto -> haddr, hto -> hlen, sap, saplen, dstaddr);
	} else {
	    /* XXX: Assumes broadcast addr is all ones */
	    /* Really should get the broadcast address as part of the
	     * dlpiinforeq, and store it somewhere in the interface structure.
	     */
	    unsigned char bcast_ether [DLPI_MAXDLADDR];

	    memset ((char *)bcast_ether, 0xFF, interface -> hw_address.hlen);
	    dlpi_makeaddr (bcast_ether, interface -> hw_address.hlen,
			 sap, saplen, dstaddr);
	}
	addrlen = interface -> hw_address.hlen + ABS (saplen);

	/* Send the packet down the wire... */
	result = dlpiunitdatareq (interface -> wfdesc, dstaddr, addrlen,
				  0, 0, dbuf, dbuflen);
#endif
	if (result < 0)
		warn ("send_packet: %m");
	return result;
}
#endif /* USE_DLPI_SEND */

#ifdef USE_DLPI_RECEIVE
ssize_t receive_packet (interface, buf, len, from, hfrom)
	struct interface_info *interface;
	unsigned char *buf;
	size_t len;
	struct sockaddr_in *from;
	struct hardware *hfrom;
{
	unsigned char dbuf [1536];
	unsigned char sap [2];
	unsigned char srcaddr [DLPI_MAXDLADDR];
	unsigned long srcaddrlen;
	int saplen;
	int flags = 0;
	int length = 0;
	int offset = 0;
	int bufix = 0;
	
#ifdef USE_DLPI_RAW
	length = read (interface -> rfdesc, dbuf, sizeof (dbuf));
#else	
	length = dlpiunitdataind (interface -> rfdesc, (unsigned char *)NULL,
				  (unsigned long *)NULL, srcaddr, &srcaddrlen,
				  (unsigned long *)NULL, dbuf, sizeof (dbuf));
#endif

	if (length <= 0) {
	    return length;
	}

#ifndef USE_DLPI_RAW
	/* Copy sender info */
	/* XXX: Assumes ethernet, where SAP comes at end of haddr */
	saplen = -2;
	if (hfrom && srcaddrlen == ABS(saplen) + interface -> hw_address.hlen) {
	    hfrom -> htype = interface -> hw_address.htype;
	    hfrom -> hlen = interface -> hw_address.hlen;
	    dlpi_parseaddr (srcaddr, hfrom -> haddr,
			    interface -> hw_address.hlen, sap, saplen);
	} else if (hfrom) {
	    memset ((char *)hfrom, '\0', sizeof (*hfrom));
	}
#endif

	/* Decode the IP and UDP headers... */
	bufix = 0;
#ifdef USE_DLPI_RAW
	/* Decode the physical header... */
	offset = decode_hw_header (interface, dbuf, bufix, hfrom);

	/* If a physical layer checksum failed (dunno of any
	   physical layer that supports this, but WTH), skip this
	   packet. */
	if (offset < 0) {
		return 0;
	}
	bufix += offset;
	length -= offset;
#endif
	offset = decode_udp_ip_header (interface, dbuf, bufix,
				       from, (unsigned char *)0, length);

	/* If the IP or UDP checksum was bad, skip the packet... */
	if (offset < 0) {
	    return 0;
	}

	bufix += offset;
	length -= offset;

	/* Copy out the data in the packet... */
	memcpy (buf, &dbuf [bufix], length);
	return length;
}
#endif

/* Common DLPI routines ...
 *
 * Written by Eric James Negaard, <lmdejn@lmd.ericsson.se>
 *
 * Based largely in part to the example code contained in the document
 * "How to Use the STREAMS Data Link Provider Interface (DLPI)", written
 * by Neal Nuckolls of SunSoft Internet Engineering.
 * 
 * This code has been developed and tested on sparc-based machines running
 * SunOS 5.5.1, with le and hme network interfaces.  It should be pretty
 * generic, though.
 * 
 * The usual disclaimers apply.  This code works for me.  Don't blame me
 * if it makes your machine or network go down in flames.  That taken
 * into consideration, use this code as you wish.  If you make usefull
 * modifications I'd appreciate hearing about it.
 */

#define DLPI_MAXWAIT		15	/* Max timeout */

static void dlpi_makeaddr (physaddr, physaddrlen, sap, saplen, buf)
	unsigned char *physaddr;
	int physaddrlen;
	unsigned char *sap;
	int saplen;
	unsigned char *buf;
{
	/*
	 * If the saplen is negative, the SAP goes at the end of the address,
	 * otherwise it goes at the beginning.
	 */
	if (saplen >= 0) {
		memcpy ((char *)buf, (char *)sap, saplen);
		memcpy ((char *)&buf [saplen], (char *)physaddr, physaddrlen);
	} else {
		memcpy ((char *)buf, (char *)physaddr, physaddrlen);
		memcpy ((char *)&buf [physaddrlen], (char *)sap, 0 - saplen);
	}
}

static void dlpi_parseaddr (buf, physaddr, physaddrlen, sap, saplen)
	unsigned char *buf;
	unsigned char *physaddr;
	int physaddrlen;
	unsigned char *sap;
	int saplen;
{
	/*
	 * If the saplen is negative, the SAP is at the end of the address,
	 * otherwise it is at the beginning.
	 */
	if (saplen >= 0) {
		memcpy ((char *)sap, (char *)buf, saplen);
		memcpy ((char *)physaddr, (char *)&buf [saplen], physaddrlen);
	} else {
		memcpy ((char *)physaddr, (char *)buf, physaddrlen);
		memcpy ((char *)sap, (char *)&buf [physaddrlen], 0 - saplen);
	}
}

/*
 * Parse an interface name and extract the unit number
 */

static int dlpiunit (ifname)
	char *ifname;
{
	int fd;
	char *cp, *dp, *ep;
	int unit;
	
	if (!ifname) {
		return 0;
	}
	
	/* Advance to the end of the name */
	cp = ifname;
	while (*cp) cp++;
	/* Back up to the start of the first digit */
	while ((*(cp-1) >= '0' && *(cp-1) <= '9') || *(cp - 1) == ':') cp--;
	
	/* Convert the unit number */
	unit = 0;
	while (*cp >= '0' && *cp <= '9') {
		unit *= 10;
		unit += (*cp++ - '0');
	}
	
	return unit;
}

/*
 * dlpiopen - open the DLPI device for a given interface name
 */
static int dlpiopen (ifname)
	char *ifname;
{
	char devname [50];
	char *cp, *dp, *ep;
	
	if (!ifname) {
		return -1;
	}
	
	/* Open a DLPI device */
	if (*ifname == '/') {
		dp = devname;
	} else {
		/* Prepend the device directory */
		memcpy (devname, DLPI_DEVDIR, strlen (DLPI_DEVDIR));
		dp = &devname [strlen (DLPI_DEVDIR)];
	}

	/* Find the end of the interface name */
	ep = cp = ifname;
	while (*ep)
		ep++;
	/* And back up to the first digit (unit number) */
	while ((*(ep - 1) >= '0' && *(ep - 1) <= '9') || *(ep - 1) == ':')
		ep--;
	
	/* Copy everything up to the unit number */
	while (cp < ep) {
		*dp++ = *cp++;
	}
	*dp = '\0';
	
	return open (devname, O_RDWR, 0);
}

/*
 * dlpiinforeq - request information about the data link provider.
 */

static int dlpiinforeq (fd)
	int fd;
{
	dl_info_req_t info_req;
	struct strbuf ctl;
	int flags;
	
	info_req.dl_primitive = DL_INFO_REQ;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (info_req);
	ctl.buf = (char *)&info_req;
	
	flags = RS_HIPRI;
	
	return putmsg (fd, &ctl, (struct strbuf *)NULL, flags);
}

/*
 * dlpiphysaddrreq - request the current physical address.
 */
static int dlpiphysaddrreq (fd, addrtype)
	int fd;
	unsigned long addrtype;
{
	dl_phys_addr_req_t physaddr_req;
	struct strbuf ctl;
	int flags;
	
	physaddr_req.dl_primitive = DL_PHYS_ADDR_REQ;
	physaddr_req.dl_addr_type = addrtype;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (physaddr_req);
	ctl.buf = (char *)&physaddr_req;
	
	flags = RS_HIPRI;
	
	return putmsg (fd, &ctl, (struct strbuf *)NULL, flags);
}

/*
 * dlpiattachreq - send a request to attach to a specific unit.
 */
static int dlpiattachreq (fd, ppa)
	unsigned long ppa;
	int fd;
{
	dl_attach_req_t	attach_req;
	struct strbuf ctl;
	int flags;
	
	attach_req.dl_primitive = DL_ATTACH_REQ;
	attach_req.dl_ppa = ppa;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (attach_req);
	ctl.buf = (char *)&attach_req;
	
	flags = 0;
	
	return putmsg (fd, &ctl, (struct strbuf*)NULL, flags);
}

/*
 * dlpibindreq - send a request to bind to a specific SAP address.
 */
static int dlpibindreq (fd, sap, max_conind, service_mode, conn_mgmt, xidtest)
	unsigned long sap;
	unsigned long max_conind;
	unsigned long service_mode;
	unsigned long conn_mgmt;
	unsigned long xidtest;
	int fd;
{
	dl_bind_req_t bind_req;
	struct strbuf ctl;
	int flags;
	
	bind_req.dl_primitive = DL_BIND_REQ;
	bind_req.dl_sap = sap;
	bind_req.dl_max_conind = max_conind;
	bind_req.dl_service_mode = service_mode;
	bind_req.dl_conn_mgmt = conn_mgmt;
	bind_req.dl_xidtest_flg = xidtest;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (bind_req);
	ctl.buf = (char *)&bind_req;
	
	flags = 0;
	
	return putmsg (fd, &ctl, (struct strbuf*)NULL, flags);
}

/*
 * dlpiunbindreq - send a request to unbind.
 */
static int dlpiunbindreq (fd)
	int fd;
{
	dl_unbind_req_t	unbind_req;
	struct strbuf ctl;
	int flags;
	
	unbind_req.dl_primitive = DL_UNBIND_REQ;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (unbind_req);
	ctl.buf = (char *)&unbind_req;
	
	flags = 0;
	
	return putmsg (fd, &ctl, (struct strbuf*)NULL, flags);
}


/*
 * dlpidetachreq - send a request to detach.
 */
static int dlpidetachreq (fd)
	int fd;
{
	dl_detach_req_t	detach_req;
	struct strbuf ctl;
	int flags;
	
	detach_req.dl_primitive = DL_DETACH_REQ;
	
	ctl.maxlen = 0;
	ctl.len = sizeof (detach_req);
	ctl.buf = (char *)&detach_req;
	
	flags = 0;
	
	return putmsg (fd, &ctl, (struct strbuf*)NULL, flags);
}


/*
 * dlpibindack - receive an ack to a dlbindreq.
 */
static int dlpibindack (fd, bufp)
	char *bufp;
	int fd;
{
	union DL_primitives *dlp;
	struct strbuf ctl;
	int flags;
	
	ctl.maxlen = DLPI_MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;

	if (strgetmsg (fd, &ctl,
		       (struct strbuf*)NULL, &flags, "dlpibindack") < 0) {
		return -1;
	}
	
	dlp = (union DL_primitives *)ctl.buf;
	
	if (!expected (DL_BIND_ACK, dlp, flags) < 0) {
		return -1;
	}
	
	if (ctl.len < sizeof (dl_bind_ack_t)) {
		/* Returned structure is too short */
		return -1;
	}

	return 0;
}

/*
 * dlpiokack - general acknowledgement reception.
 */
static int dlpiokack (fd, bufp)
	char *bufp;
	int fd;
{
	union DL_primitives *dlp;
	struct strbuf ctl;
	int flags;
	
	ctl.maxlen = DLPI_MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;
	
	if (strgetmsg (fd, &ctl,
		       (struct strbuf*)NULL, &flags, "dlpiokack") < 0) {
		return -1;
	}
	
	dlp = (union DL_primitives *)ctl.buf;
	
	if (!expected (DL_OK_ACK, dlp, flags) < 0) {
		return -1;
	}
	
	if (ctl.len < sizeof (dl_ok_ack_t)) {
		/* Returned structure is too short */
		return -1;
	}
	
	return 0;
}

/*
 * dlpiinfoack - receive an ack to a dlinforeq.
 */
static int dlpiinfoack (fd, bufp)
	char *bufp;
	int fd;
{
	union DL_primitives *dlp;
	struct strbuf ctl;
	int flags;
	
	ctl.maxlen = DLPI_MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;
	
	if (strgetmsg (fd, &ctl, (struct strbuf *)NULL, &flags,
		       "dlpiinfoack") < 0) {
		return -1;
	}
	
	dlp = (union DL_primitives *) ctl.buf;
	
	if (!expected (DL_INFO_ACK, dlp, flags) < 0) {
		return -1;
	}
	
	if (ctl.len < sizeof (dl_info_ack_t)) {
		/* Returned structure is too short */
		return -1;
	}
	
	return 0;
}

/*
 * dlpiphysaddrack - receive an ack to a dlpiphysaddrreq.
 */
int dlpiphysaddrack (fd, bufp)
	char *bufp;
	int fd;
{
	union DL_primitives *dlp;
	struct strbuf ctl;
	int flags;
	
	ctl.maxlen = DLPI_MAXDLBUF;
	ctl.len = 0;
	ctl.buf = bufp;
	
	if (strgetmsg (fd, &ctl, (struct strbuf *)NULL, &flags,
		       "dlpiphysaddrack") < 0) {
		return -1;
	}

	dlp = (union DL_primitives *)ctl.buf;
	
	if (!expected (DL_PHYS_ADDR_ACK, dlp, flags) < 0) {
		return -1;
	}

	if (ctl.len < sizeof (dl_phys_addr_ack_t)) {
		/* Returned structure is too short */
		return -1;
	}
	
	return 0;
}

int dlpiunitdatareq (fd, addr, addrlen, minpri, maxpri, dbuf, dbuflen)
	int fd;
	unsigned char *addr;
	int addrlen;
	unsigned long minpri;
	unsigned long maxpri;
	unsigned char *dbuf;
	int dbuflen;
{
	long buf [DLPI_MAXDLBUF];
	union DL_primitives *dlp;
	struct strbuf ctl, data;
	
	/* Set up the control information... */
	dlp = (union DL_primitives *)buf;
	dlp -> unitdata_req.dl_primitive = DL_UNITDATA_REQ;
	dlp -> unitdata_req.dl_dest_addr_length = addrlen;
	dlp -> unitdata_req.dl_dest_addr_offset = sizeof (dl_unitdata_req_t);
	dlp -> unitdata_req.dl_priority.dl_min = minpri;
	dlp -> unitdata_req.dl_priority.dl_max = maxpri;

	/* Append the destination address */
	memcpy ((char *)buf + dlp -> unitdata_req.dl_dest_addr_offset,
		addr, addrlen);
	
	ctl.maxlen = 0;
	ctl.len = dlp -> unitdata_req.dl_dest_addr_offset + addrlen;
	ctl.buf = (char *)buf;

	data.maxlen = 0;
	data.buf = (char *)dbuf;
	data.len = dbuflen;

	/* Send the packet down the wire... */
	return putmsg (fd, &ctl, &data, 0);
}

static int dlpiunitdataind (fd, daddr, daddrlen,
			    saddr, saddrlen, grpaddr, dbuf, dlen)
	int fd;
	unsigned char *daddr;
	unsigned long *daddrlen;
	unsigned char *saddr;
	unsigned long *saddrlen;
	unsigned long *grpaddr;
	unsigned char *dbuf;
	int dlen;
{
	long buf [DLPI_MAXDLBUF];
	union DL_primitives *dlp;
	struct strbuf ctl, data;
	int flags = 0;
	int result;

	/* Set up the msg_buf structure... */
	dlp = (union DL_primitives *)buf;
	dlp -> unitdata_ind.dl_primitive = DL_UNITDATA_IND;

	ctl.maxlen = DLPI_MAXDLBUF;
	ctl.len = 0;
	ctl.buf = (char *)buf;
	
	data.maxlen = dlen;
	data.len = 0;
	data.buf = (char *)dbuf;
	
	result = getmsg (fd, &ctl, &data, &flags);
	
	if (result != 0) {
		return -1;
	}
	
	if (ctl.len < sizeof (dl_unitdata_ind_t) ||
	    dlp -> unitdata_ind.dl_primitive != DL_UNITDATA_IND) {
		return -1;
	}
	
	if (data.len <= 0) {
		return data.len;
	}

	/* Copy sender info */
	if (saddr) {
		memcpy (saddr, &buf [dlp -> unitdata_ind.dl_src_addr_offset],
			dlp -> unitdata_ind.dl_src_addr_length);
	}
	if (saddrlen) {
		*saddrlen = dlp -> unitdata_ind.dl_src_addr_length;
	}

	/* Copy destination info */
	if (daddr) {
		memcpy (daddr, &buf [dlp -> unitdata_ind.dl_dest_addr_offset],
			dlp -> unitdata_ind.dl_dest_addr_length);
	}
	if (daddrlen) {
		*daddrlen = dlp -> unitdata_ind.dl_dest_addr_length;
	}
	
	if (grpaddr) {
		*grpaddr = dlp -> unitdata_ind.dl_group_address;
	}
	
	return data.len;
}

/*
 * expected - see if we got what we wanted.
 */
static int expected (prim, dlp, msgflags)
	unsigned long prim;
	union DL_primitives *dlp;
	int msgflags;
{
	if (msgflags != RS_HIPRI) {
		/* Message was not M_PCPROTO */
		return 0;
	}

	if (dlp -> dl_primitive != prim) {
		/* Incorrect/unexpected return message */
		return 0;
	}
	
	return 1;
}

/*
 * strgetmsg - get a message from a stream, with timeout.
 */
static int strgetmsg (fd, ctlp, datap, flagsp, caller)
	struct strbuf *ctlp, *datap;
	char *caller;
	int *flagsp;
	int fd;
{
	int result;
#ifdef USE_POLL
	struct pollfd pfd;
	int count;
	time_t now;
	time_t starttime;
	int to_msec;
#endif
	
#ifdef USE_POLL
	pfd.fd = fd;
	pfd.events = POLLPRI;	/* We're only interested in knowing
				 * when we can receive the next high
				 * priority message.
				 */
	pfd.revents = 0;

	now = time (&starttime);
	while (now <= starttime + DLPI_MAXWAIT) {
		to_msec = ((starttime + DLPI_MAXWAIT) - now) * 1000;
		count = poll (&pfd, 1, to_msec);
		
		if (count == 0) {
			/* error ("strgetmsg: timeout"); */
			return -1;
		} else if (count < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				time (&now);
				continue;
			} else {
				/* error ("poll: %m"); */
				return -1;
			}
		} else {
			break;
		}
	}
#else  /* defined (USE_POLL) */
	/*
	 * Start timer.  Can't use select, since it might return true if there
	 * were non High-Priority data available on the stream.
	 */
	(void) sigset (SIGALRM, sigalrm);
	
	if (alarm (DLPI_MAXWAIT) < 0) {
		/* error ("alarm: %m"); */
		return -1;
	}
#endif /* !defined (USE_POLL) */

	/*
	 * Set flags argument and issue getmsg ().
	 */
	*flagsp = 0;
	if ((result = getmsg (fd, ctlp, datap, flagsp)) < 0) {
		return result;
	}

#ifndef USE_POLL
	/*
	 * Stop timer.
	 */	
	if (alarm (0) < 0) {
		/* error ("alarm: %m"); */
		return -1;
	}
#endif

	/*
	 * Check for MOREDATA and/or MORECTL.
	 */
	if (result & (MORECTL|MOREDATA)) {
		return -1;
	}

	/*
	 * Check for at least sizeof (long) control data portion.
	 */
	if (ctlp -> len < sizeof (long)) {
		return -1;
	}

	return 0;
}

#ifndef USE_POLL
/*
 * sigalrm - handle alarms.
 */
static void sigalrm (sig)
	int sig;
{
	fprintf (stderr, "strgetmsg: timeout");
	exit (1);
}
#endif /* !defined (USE_POLL) */

int can_unicast_without_arp ()
{
	return 1;
}

int can_receive_unicast_unconfigured (ip)
	struct interface_info *ip;
{
	return 1;
}

void maybe_setup_fallback ()
{
	struct interface_info *fbi;
	fbi = setup_fallback ();
	if (fbi) {
		if_register_fallback (fbi);
		add_protocol ("fallback", fallback_interface -> wfdesc,
			      fallback_discard, fallback_interface);
	}
}
#endif /* USE_DLPI */
