/* Remote target communications for NetBSD targets via UDP.
   Copyright (C) 1988-1991 Free Software Foundation, Inc.
   Copyright (C) 1993, 1995, 1996 Wolfgang Solfrank.
   Copyright (C) 1993, 1995, 1996 TooLs GmbH.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foudation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that is will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Remote communication protocol.

   Data is send binary.
   The first byte is the command code.

   Packets have a sequence number and the data length prepended
   and a checksum byte appended.
   The answer has the same sequence number.  On a timeout, packets
   are rerequested by resending the request.

	Request		Packet

	read registers	R
	reply		pXX...X		Register data is sent in target format.
			emessage	for an error.

	write regs	WXX...X		Same as above.
	reply		ok		for success.
			emessage	for an error.
	
	read mem	MAA...ALL...L	AA...A is address, LL...L is length
					(both as CORE_ADDR).
	reply		pXX...X		XX...X is mem contents.
			emessage	for an error.
	
	write mem	NAA...ALL...LXX...X
					AA...A is address, LL...L is length,
					XX...X is data.
	reply		ok		for success.
			emessage	for an error.

	cont		C

	step		S

	open debug	OII...IWW...W	II...I is an arbitrary id of the session,
					WW...W is the user and machine running gdb.

	There is no immediate reply to step, cont or open.
	The reply comes when the machines stops (again).
	It is		s

	detach		X
	reply		ok
*/

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "target.h"
#include "wait.h"
#include "terminal.h"

extern void start_remote();

extern struct target_ops ipkdb_ops;	/* Forward decl */

static int timeout = 1;

/* Descriptor for I/O to remote machine.  Initialize it to -1 so that
   remote_open knows that we don't have a file open when the program
   starts.  */
static int ipkdb_desc = -1;

#define	PBUFSIZ	400

/* Maximum number of bytes to read/write at once.  */
#define	MAXBUFBYTES	(PBUFSIZ-10)

static void remote_send();
static void putpkt();
static void getpkt();
static int netopen();
static void netread();
static void netwrite();


/* Initialize remote connection */
void
ipkdb_start()
{
}

/* Clean up connection to remote debugger.  */
/* ARGSUSED */
void
ipkdb_close(quitting)
	int quitting;
{
	if (ipkdb_desc >= 0)
		close(ipkdb_desc);
	ipkdb_desc = -1;
}

/* Open a connection to a remote debugger.
   NAME is the name of the target machines.  */
void
ipkdb_open(name, from_tty)
	char *name;
	int from_tty;
{
	if (name == 0)
		error(
"To open a remote debug connection, you need to specify\n\
the name of the target machines.");

	target_preopen(from_tty);

	ipkdb_close(0);

	if (netopen(name,from_tty) < 0)
		perror_with_name(name);
}

/* ipkdb_detach()
   takes a program previously attached to and detaches it.
   We better not have left any breakpoints
   in the program or it'll die when it hits one.
   Close the open connection to the remote debugger.
   Use this when you want to detach and do something else
   with your gdb.  */
static void
ipkdb_detach(args, from_tty)
	char *args;
	int from_tty;
{
	char buf[PBUFSIZ];
	int l;

	if (args)
		error("Argument given to \"detach\" when remotely debugging IPKDB.");
    
	*buf = 'X';
	l = 1;
	remote_send(buf, &l);

	pop_target();
	if (from_tty)
		puts_filtered("Ending remote debugging IPKDB.\n");
}

/* Tell the remote machine to resume.  */
void
ipkdb_resume(pid, step, siggnal)
	int pid;
	int step;
	enum target_signal siggnal;
{
	if (siggnal != TARGET_SIGNAL_0)
		error("Can't send signals to a remote IPKDB system.");
    
	putpkt(step ? "S" : "C", 1);
}

/* Wait until the remote machines stops, then return,
   storing status in STATUS just as `wait' would.
   Returns "pid" (though it's not clear what, if anything, that
   means in the case of this target).  */
int
ipkdb_wait(pid, status)
	int pid;
	struct target_waitstatus *status;
{
	unsigned char buf[PBUFSIZ];
	int l;

	status->kind = TARGET_WAITKIND_EXITED;
	status->value.integer = 0;
	getpkt(buf, &l);
	if (buf[0] == 'e') {
		buf[l] = 0;
		error("Remote failuer reply: %s", buf + 1);
	}
	if (buf[0] != 's')
		error("Invalid remote reply: %s", buf);
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_TRAP;
	return 0;
}

/* Read the remote registers into the block REGS.  */
/* Currently we just read all the registers, so we don't use regno.  */
/* ARGSUSED */
void
ipkdb_fetch_registers(regno)
	int regno;
{
	char buf[PBUFSIZ];
	int i;

	*buf = 'R';
	i = 1;
	remote_send(buf, &i);

	/* Reply describes registers byte by byte.
	   Suck them all up, then supply them to the
	   register cacheing/storage mechanism.  */
	for (i = 0; i < NUM_REGS; i++)
		supply_register(i, &buf[1 + REGISTER_BYTE(i)]);
}

/* Prepare to store registers.  Since we send them all, we have to
   read out the ones we don't want to change first.  */
void
ipkdb_prepare_to_store()
{
	ipkdb_fetch_registers(-1);
}

/* Store the remote registers from the contents of the block REGISTERS.  */
/* ARGSUSED */
void
ipkdb_store_registers(regno)
	int regno;
{
	char buf[PBUFSIZ];
	int l;

	buf[0] = 'W';

	memcpy(buf + 1, registers, REGISTER_BYTES);
	l = 1 + REGISTER_BYTES;

	remote_send(buf,&l);
}

/* Put addr into buf */
static void
addrput(addr, buf)
	CORE_ADDR addr;
	char *buf;
{
	int i;
    
	buf += sizeof addr;
	for (i = sizeof addr; --i >= 0; addr >>= 8)
		*--buf = addr;
}

/* Write memory data directly to the remote machine.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.  */
void
ipkdb_write_bytes(memaddr, myaddr, len)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
{
	char buf[PBUFSIZ];

	if (len > PBUFSIZ - 1 - 2 * sizeof(CORE_ADDR))
		abort();
    
	*buf = 'N';
	addrput(memaddr, buf + 1);
	addrput((CORE_ADDR)len, buf + 1 + sizeof(CORE_ADDR));

	memcpy(buf + 1 + 2 * sizeof(CORE_ADDR), myaddr, len);

	len += 1 + 2 * sizeof(CORE_ADDR);
	remote_send(buf, &len);
}

/* Read memory data directly from the remote machine.
   MEMADDR is the address in the remote memory space.
   MYADDR is the address of the buffer in our space.
   LEN is the number of bytes.  */
void
ipkdb_read_bytes(memaddr, myaddr, len)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
{
	char buf[PBUFSIZ];
	int i;
	char *p;

	if (len > PBUFSIZ - 1)
		abort();
    
	*buf = 'M';
	addrput(memaddr, buf + 1);
	addrput((CORE_ADDR)len, buf + 1 + sizeof(CORE_ADDR));

	i = 1 + 2 * sizeof(CORE_ADDR);
	remote_send(buf, &i);

	if (i != len + 1)
		error("ipkdb_read_bytes got wrong size of answer");
    
	/* Reply describes bytes in target format */
	memcpy(myaddr, buf + 1, len);
}

/* Read or write LEN bytes from inferior memory at MEMADDR, transferring
   to or from debugger address MYADDR.  Write to inferior if SHOULD_WRITE is
   nonzero.  Returns length of data written or read; 0 for error.  */
/* ARGSUSED */
int
ipkdb_xfer_memory(memaddr, myaddr, len, should_write, target)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
	int should_write;
	struct target_ops *target;		/* ignored */
{
	int origlen = len;
	int xfersize;

	while (len > 0) {
		xfersize = min(len, MAXBUFBYTES);

		if (should_write)
			ipkdb_write_bytes(memaddr, myaddr, xfersize);
		else
			ipkdb_read_bytes(memaddr, myaddr, xfersize);
		memaddr	+= xfersize;
		myaddr	+= xfersize;
		len	-= xfersize;
	}
	return origlen;	/* no error possible */
}

void
ipkdb_files_info()
{
	printf("ipkdb files info missing here.  FIXME.\n");
}

static char bpt[] = BREAKPOINT;

int
ipkdb_insert_breakpoint(addr, save)
	CORE_ADDR addr;
	char *save;
{
	ipkdb_read_bytes(addr, save, sizeof bpt);
	ipkdb_write_bytes(addr, bpt, sizeof bpt);
	return 0;
}

int
ipkdb_remove_breakpoint(addr, save)
	CORE_ADDR addr;
	char *save;
{
	ipkdb_write_bytes(addr, save, sizeof bpt);
	return 0;
}

/* Send the command in BUF to the remote machine,
   and read the reply into BUF.
   Report an error if we get an error reply.  */
static void
remote_send(buf, lp)
	char *buf;
	int *lp;
{
	putpkt(buf, *lp);
	getpkt(buf, lp);

	if (buf[0] == 'e') {
		buf[*lp] = 0;
		error("Remote failure reply: %s", buf + 1);
	}
}

/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF.  */
static void
putpkt(buf, l)
	char *buf;
	int l;
{
	int i;
	unsigned char csum = 0;
	char buf2[500];
	char *p;

	/* Copy the packet info buffer BUF2, encapsulating it
	   and giving it a checksum.  */

	p = buf2;

	for (i = 0; i < l; i++) {
		csum += buf[i];
		*p++ = buf[i];
	}
	*p++ = csum;

	/* Send it over and over until we get a positive ack.  */
	netwrite(buf2, l);
}

/* Read a packet from the remote machine, with error checking,
   and store it in BUF.  */
static void
getpkt(buf, lp)
	char *buf;
	int *lp;
{
	netread(buf, lp);
}

/* net I/O routines */
/* open a network connection to the remote machine */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static struct sockaddr_in sin;
static long netseq;

static long
getnl(p)
	void *p;
{
	u_char *s = p;

	return (*s << 24)|(s[1] << 16)|(s[2] << 8)|s[3];
}

static int
getns(p)
	void *p;
{
	u_char *s = p;

	return (*s << 8)|s[1];
}

static void
setnl(p, l)
	void *p;
	long l;
{
	u_char *s = p;

	*s++ = l >> 24;
	*s++ = l >> 16;
	*s++ = l >> 8;
	*s = l;
}

static void
setns(p, l)
	void *p;
	int l;
{
	u_char *s = p;

	*s++ = l >> 8;
	*s = l;
}

static int
netopen(name, from_tty)
	char *name;
	int from_tty;
{
	struct hostent *he;
	char buf[512];
	char hostname[32];
	char *cp;
	int csum;
	
	if (!(he = gethostbyname(name)))
		return -1;

	if ((ipkdb_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		fatal("netopen: socket failed\n");

	sin.sin_family = AF_INET;
	sin.sin_port = 1138;
	sin.sin_addr.s_addr = 0;
	if (bind(ipkdb_desc, (struct sockaddr *)&sin, sizeof sin) < 0)
		fatal("netopen: bind failed\n");

	sin.sin_addr = *(struct in_addr *)he->h_addr;

	netseq = 0;

	if (from_tty)
		printf("Remote debugging on %s\n", name);
	push_target(&ipkdb_ops);	/* Switch to using remote target now.  */

	/* Send initial open message */
	gethostname(hostname, sizeof hostname);
	buf[0] = 'O';
	setnl(buf + 1, time(NULL));
	sprintf(buf + 5, "%s:%s", cuserid(0), hostname);
	csum = 0;
	for (cp = buf; *cp; csum += *cp++);
	*cp = csum;
	netwrite(buf, cp - buf);

	start_remote();
	return ipkdb_desc;
}

static char ibuf[512], obuf[512];
static int olen;

static void
netwrite(buf, len)
	char *buf;
	int len;
{
	struct sockaddr_in frm;
	int l;

	if (len > sizeof obuf + 6)
		error("netwrite: packet too large\n");

	setnl(obuf, netseq);
	setns(obuf + 4, len);
	memcpy(obuf + 6, buf, len + 1);
	olen = len + 7;
	if (sendto(ipkdb_desc, obuf, olen, 0,
		   (struct sockaddr *)&sin, sizeof sin) < 0)
		fatal("netwrite: sendto\n");
}

static void
netread(buf, lp)
	char *buf;
	int *lp;
{
	int len, l, i, csum;
	char *cp;
	struct sockaddr_in frm;
	struct timeval tv, deadline;
	fd_set rd;
	
	gettimeofday(&deadline, NULL);
	deadline.tv_sec++;
	while (1) {
		FD_ZERO(&rd);
		FD_SET(ipkdb_desc, &rd);
		gettimeofday(&tv, NULL);
		tv.tv_sec = deadline.tv_sec - tv.tv_sec;
		if ((tv.tv_usec = deadline.tv_usec - tv.tv_usec) < 0) {
			tv.tv_usec += 1000000;
			tv.tv_sec--;
		}
		if (tv.tv_sec < 0)
			tv.tv_sec = tv.tv_usec = 0;
		switch (select(ipkdb_desc + 1, &rd, NULL, NULL, &tv)) {
		case -1:
			fatal("netread: select");
		default:
			l = sizeof frm;
			if ((len = recvfrom(ipkdb_desc, ibuf, sizeof ibuf, 0,
					    (struct sockaddr *)&frm, &l)) < 0)
				fatal("netread: recvfrom\n");
			if (getnl(ibuf) != netseq)
				break; /* Ignore packet */
			csum = 0;
			cp = ibuf + 6;
			len = getns(ibuf + 4);
			for (i = len; --i >= 0; csum += *cp++);
			if ((char)csum == *cp) {
				netseq++;
				memcpy(buf, ibuf + 6, len);
				*lp = len;
				return;
			}
			/*
			 * Packet had correct sequence number, but
			 * wrong checksum, do an immediate retry, so...
			 */
			/* FALLTHROUGH */
		case 0:
			if (sendto(ipkdb_desc, obuf, olen, 0,
				   (struct sockaddr *)&sin, sizeof sin) < 0)
				fatal("netread: sendto\n");
			gettimeofday(&deadline, NULL);
			deadline.tv_sec++;
			break;
		}
	}
}

static void
ipkdb_kill()
{
	target_mourn_inferior();
}

static void
ipkdb_mourn()
{
	unpush_target(&ipkdb_ops);
	generic_mourn_inferior();
}

/* Define the target subroutine names */
struct target_ops ipkdb_ops = {
	"ipkdb",
	"Remote IPKDB target via UDP/IP",
	"Debug a remote computer via UDP/IP.\n\"
Specify the name of the target machine",
	ipkdb_open,
	ipkdb_close,
	0,					/* attach */
	ipkdb_detach,
	ipkdb_resume,
	ipkdb_wait,
	ipkdb_fetch_registers,
	ipkdb_store_registers,
	ipkdb_prepare_to_store,
	ipkdb_xfer_memory,
	ipkdb_files_info,
	ipkdb_insert_breakpoint,
	ipkdb_remove_breakpoint,
	0,					/* terminal init */
	0,					/* terminal inferior */
	0,					/* terminal ours for output */
	0,					/* terminal ours */
	0,					/* terminal info */
	ipkdb_kill,
	0,					/* load */
	0,					/* lookup_symbol */
	0,					/* create_inferior */
	ipkdb_mourn,
	0,					/* can run */
	0,					/* notice signals */
	0,					/* thread alive */
	0,					/* stop */
	process_stratum,
	0,					/* next */
	1,					/* has all mem */
	1,					/* has mem */
	1,					/* has stack */
	1,					/* has regs */
	1,					/* has exec */
	0,					/* sections */
	0,					/* sections end */
	OPS_MAGIC				/* Always the last thing */
};

void
_initialize_ipkdb()
{
	add_target(&ipkdb_ops);
}
