/*	$NetBSD: remote-ipkdb.c,v 1.3 2000/08/07 15:20:36 ws Exp $	*/

/*
 * Part of this code is derived from software copyrighted by the
 * Free Software Foundation.
 *
 * Modified 1993-2000 by Wolfgang Solfrank.
 */

/* Remote target communications for NetBSD targets via UDP (aka IPKDB).
   Copyright (C) 1988-1991 Free Software Foundation, Inc.

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

   Data is sent in binary.
   The first byte is the command code.

   Packets have a sequence number and the data length prepended
   and a signature appended.  The signature is computed according
   to the HMAC algorithm (see draft-ietf-ipsec-hmac-md5-00.txt).
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
		It is	s

	detach		X
	reply		ok
*/

#include "defs.h"
#include "command.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "target.h"
#include "valprint.h"
#include "wait.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netdb.h>

#include <md5.h>

static struct target_ops ipkdb_ops;

static int ipkdb_desc = -1;
static char host[512];

#define	DBGPORT	1138

#define	PBUFSIZ	400

static void remote_send();
static void putpkt();
static void getpkt();
static int startprot();
static void netread();
static void netwrite();
static void trykey();

/* Clean up connection to remote debugger. */
static void
ipkdb_close(quitting)
	int quitting;
{
	if (ipkdb_desc >= 0)
		close(ipkdb_desc);
	ipkdb_desc = -1;
}

/* Open a connection to a remote debugger.
   The argument NAME, being the hostname of the target,
   may optionally have the HMAC key for this appended. */
static void
ipkdb_open(name, from_tty)
	char *name;
	int from_tty;
{
	struct hostent *he;
	struct sockaddr_in sin;

	if (name == 0)
		error(
"To open a remote debug connection, you need to specify\n\
the name of the target machine.");

	trykey(name, from_tty);
	strcpy(host, name);

	ipkdb_close(0);

	if (!(he = gethostbyname(host))
	    || he->h_addrtype != AF_INET)
		error("host '%s' unknown\n", host);

	target_preopen(from_tty);
	push_target(&ipkdb_ops);

	if ((ipkdb_desc = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		perror_with_name(host);

	sin.sin_len = sizeof sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DBGPORT);
	sin.sin_addr = *(struct in_addr *)he->h_addr;

	if (connect(ipkdb_desc, (struct sockaddr *)&sin, sizeof sin) < 0)
		perror_with_name(host);

	if (!catch_errors(startprot, NULL,
			  "Couldn't establish connection to remote target\n",
			  RETURN_MASK_ALL))
		pop_target();
	else if (from_tty)
		printf_filtered("Remote debugging on %s\n", name);
}

/* Close the open connection to the remote debugger.
   Use this when you want to detach and do something else
   with your gdb. */
static void
ipkdb_detach(args, from_tty)
	char *args;
	int from_tty;
{
	char buf[PBUFSIZ];
	int l;

	*buf = 'X';
	l = 1;
	remote_send(buf, &l);
	pop_target();
	if (from_tty)
		puts_filtered("Ending remote debugging IPKDB.\n");
}

/* Tell the remote machine to resume. */
static void
ipkdb_resume(pid, step, ssignal)
	int pid, step;
	enum target_signal ssignal;
{
	if (ssignal != TARGET_SIGNAL_0)
		error("Can't send signals to a remote IPKDB system.");

	putpkt(step ? "S" : "C", 1);
}

/* Wait until the remote machine stops, then return. */
static int
ipkdb_wait(pid, status)
	int pid;
	struct target_waitstatus *status;
{
	unsigned char buf[PBUFSIZ];
	int l;

	getpkt(buf, &l);
	if (l > 0 && buf[0] == 'e') {
		buf[l] = 0;
		error("Remote failure reply: '%s'", buf + 1);
	}
	if (buf[0] != 's')
		error("Invalid remote reply: '%s'", buf);
	status->kind = TARGET_WAITKIND_STOPPED;
	status->value.sig = TARGET_SIGNAL_TRAP;

	return 0;
}

/* Read the remote registers. */
static void
ipkdb_fetch_register(regno)
	int regno;
{
	char buf[PBUFSIZ];
	int l;

	*buf = 'R';
	l = 1;
	remote_send(buf, &l);

	/* Reply describes registers byte by byte.
	   Suck them all up, and supply them to the
	   register cacheing/storage mechanism. */
	for (l = 0; l < NUM_REGS; l++)
		supply_register(l, &buf[1 + REGISTER_BYTE(l)]);
}

/* Prepare to store registers.  Since we send them all, we have to
   read out the ones we don't want to change first. */
static void
ipkdb_prepare_to_store()
{
	ipkdb_fetch_register(-1);
}

/* Store the remote registers. */
static void
ipkdb_store_register(regno)
	int regno;
{
	char buf[PBUFSIZ];
	int l;

	buf[0] = 'W';

	memcpy(buf + 1, registers, REGISTER_BYTES);
	l = 1 + REGISTER_BYTES;

	remote_send(buf, &l);
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

/* Write memory data directly to the remote machine. */
static void
ipkdb_write_bytes(memaddr, myaddr, len)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
{
	char buf[PBUFSIZ];

	*buf = 'N';
	addrput(memaddr, buf + 1);
	addrput((CORE_ADDR)len, buf + 1 + sizeof(CORE_ADDR));

	memcpy(buf + 1 + 2 * sizeof(CORE_ADDR), myaddr, len);

	len += 1 + 2 * sizeof(CORE_ADDR);
	remote_send(buf, &len);
}

/* Read memory data directly from the remote machine. */
static void
ipkdb_read_bytes(memaddr, myaddr, len)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
{
	char buf[PBUFSIZ];
	int l;
	char *p;

	*buf = 'M';
	addrput(memaddr, buf + 1);
	addrput((CORE_ADDR)len, buf + 1 + sizeof(CORE_ADDR));

	l = 1 + 2 * sizeof(CORE_ADDR);
	remote_send(buf, &l);

	if (l != len + 1)
		error("ipkdb_read_bytes got wrong size of answer");

	memcpy(myaddr, buf + 1, len);
}

/* Read or write LEN bytes from inferior memory at MEMADDR */
static int
ipkdb_xfer_memory(memaddr, myaddr, len, should_write, target)
	CORE_ADDR memaddr;
	char *myaddr;
	int len;
	int should_write;
	struct target_ops *target;
{
	int origlen = len;
	int xfersize;

	while (len > 0) {
		xfersize = min(len, PBUFSIZ - 3 * sizeof(CORE_ADDR));

		if (should_write)
			ipkdb_write_bytes(memaddr, myaddr, xfersize);
		else
			ipkdb_read_bytes(memaddr, myaddr, xfersize);
		memaddr += xfersize;
		myaddr += xfersize;
		len -= xfersize;
	}
	return origlen;
}

static void
ipkdb_files_info()
{
	printf_unfiltered("\tAttached to IPKDB at %s.\n", host);
}

#ifdef	BREAKPOINT
static char bpt[] = BREAKPOINT;
#endif

static int
ipkdb_insert_breakpoint(addr, save)
	CORE_ADDR addr;
	char *save;
{
#ifdef	BREAKPOINT
	ipkdb_read_bytes(addr, save, sizeof bpt);
	ipkdb_write_bytes(addr, bpt, sizeof bpt);
	return 0;
#else
	error("Don't know how to insert breakpoint");
#endif
}

static int
ipkdb_remove_breakpoint(addr, save)
	CORE_ADDR addr;
	char *save;
{
#ifdef	BREAKPOINT
	ipkdb_write_bytes(addr, save, sizeof bpt);
	return 0;
#else
	error("Don't know how to remove breakpoint");
#endif
}

static void
ipkdb_kill()
{
}

/* Send the command in BUF to the remote machine,
   and read the reply into BUF. */
static void
remote_send(buf, lp)
	char *buf;
	int *lp;
{
	putpkt(buf, *lp);
	getpkt(buf, lp);

	if (buf[0] == 'e') {
		buf[*lp] = 0;
		error("Remote failure reply: '%s'", buf + 1);
	}
}

/* HMAC Checksumming routines */

/*
 * The following code is more or less stolen from the hmac_md5
 * function in the Appendix of the HMAC IETF draft, but is
 * optimized as suggested in this same paper.
 */
static char *key;
static MD5_CTX icontext, ocontext;

static void
hmac_init()
{
	char pad[64];
	char tk[16];
	int key_len;
	int i;

	/* Require key to be at least 16 bytes long */
	if (!key || (key_len = strlen(key)) < 16) {
		if (key)
			/* This doesn't make too much sense, but... */
			memset(key, 0, key_len);

		error("ipkdbkey must be at least 16 bytes long.");
	}
	/* if key is longer than 64 bytes reset it to key=MD5(key) */
	if (key_len > 64) {
		MD5Init(&icontext);
		MD5Update(&icontext, key, key_len);
		MD5Final(tk, &icontext);

		/* This doesn't make too much sense, but... */
		memset(key, 0, key_len);

		key = tk;
		key_len = 16;
	}

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is and n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */
	/*
	 * We do the initial part of MD5(K XOR ipad)
	 * and MD5(K XOR opad) here, in order to
	 * speed up the computation later on.
	 */
	memset(pad, 0, sizeof pad);
	memcpy(pad, key, key_len);
	for (i = 0; i < 64; i++)
		pad[i] ^= 0x36;
	MD5Init(&icontext);
	MD5Update(&icontext, pad, 64);

	memset(pad, 0, sizeof pad);
	memcpy(pad, key, key_len);
	for (i = 0; i < 64; i++)
		pad[i] ^= 0x5c;
	MD5Init(&ocontext);
	MD5Update(&ocontext, pad, 64);

	/*
	 * Zero out the key.
	 * This doesn't make too much sense, but...
	 */
	memset(key, 0, key_len);
}

/*
 * This is more or less hmac_md5 from the HMAC IETF draft, Appendix.
 */
static void *
chksum(buf, len)
	void *buf;
	int len;
{
	static u_char digest[16];
	struct MD5Context context;

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */
	/*
	 * Since we've already done the precomputation,
	 * we can now stuff the data into the relevant
	 * preinitialized contexts to get the result.
	 */
	/*
	 * perform inner MD5
	 */
	memcpy(&context, &icontext, sizeof context);
	MD5Update(&context, buf, len);
	MD5Final(digest, &context);
	/*
	 * perform outer MD5
	 */
	memcpy(&context, &ocontext, sizeof context);
	MD5Update(&context, digest, 16);
	MD5Final(digest, &context);

	return digest;
}

static struct cmd_list_element *setkeycmd;

static void
trykey(name, from_tty)
	char *name;
	int from_tty;
{
	char *k;

	for (k = name; *k && !isspace(*k); k++);
	if (*k)
		*k++ = 0;
	for (; *k && isspace(*k); k++);
	if (*k)
		do_setshow_command(k, from_tty, setkeycmd);
	else if (!key)
		error("No valid ipkdbkey yet");
}

/* net I/O routines */
static long netseq;

static long
getnl(p)
	void *p;
{
	u_char *s = p;

	return (s[0] << 24)|(s[1] << 16)|(s[2] << 8)|s[3];
}

static int
getns(p)
	void *p;
{
	u_char *s = p;

	return (s[0] << 8)|s[1];
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
startprot(dummy)
	char *dummy;
{
	char buf[PBUFSIZ];
	char myname[128];

	netseq = 0;

	gethostname(myname, sizeof myname);
	buf[0] = 'O';
	setnl(buf + 1, time(NULL));
	sprintf(buf + 5, "%s@%s", getlogin(), myname);
	putpkt(buf, 5 + strlen(buf + 5));

	/* Don't print character strings by default.  This might break the target. */
	print_max = 0;

	start_remote();

	return 1;
}

static char ibuf[512], obuf[512];
static int olen;

static void
putpkt(buf, len)
	char *buf;
	int len;
{
	if (len > sizeof obuf - 10)
		error("putpkt: packet too large");

	setnl(obuf, netseq);
	setns(obuf + 4, len);
	memcpy(obuf + 6, buf, len + 1);
	memcpy(obuf + len + 6, chksum(obuf, len + 6), 16);
	olen = len + 6 + 16;

	if (write(ipkdb_desc, obuf, olen) < 0
	    && (errno != EINTR
#ifdef	ECONNREFUSED
		&& errno != ECONNREFUSED
#endif
#ifdef	EHOSTDOWN
		&& errno != EHOSTDOWN
#endif
	    ))
		fatal("putpkt: write");
}

static void
getpkt(buf, lp)
	char *buf;
	int *lp;
{
	int len;
	struct timeval tv, deadline;
	fd_set rd;

	gettimeofday(&deadline, NULL);
	deadline.tv_sec++;

	while (1) {
		notice_quit();
		if (quit_flag)
			quit();

		FD_ZERO(&rd);
		FD_SET(ipkdb_desc, &rd);
		gettimeofday(&tv, NULL);
		tv.tv_sec = deadline.tv_sec - tv.tv_sec;
		tv.tv_usec = deadline.tv_usec - tv.tv_usec;
		if (tv.tv_usec < 0) {
			tv.tv_usec += 1000000;
			tv.tv_sec--;
		}
		if (tv.tv_sec < 0)
			tv.tv_sec = tv.tv_usec = 0;
		switch (select(ipkdb_desc + 1, &rd, NULL, NULL, &tv)) {
		case -1:
			if (errno == EINTR
#ifdef	ECONNREFUSED
			    || errno == ECONNREFUSED /* Solaris lossage??? */
#endif
			    )
				continue;
			fatal("getpkt: select");
		default:
			if (read(ipkdb_desc, ibuf, sizeof ibuf) < 0) {
				if (errno == EINTR
#ifdef	ECONNREFUSED
				    || errno == ECONNREFUSED /* Solaris lossage??? */
#endif
				    )
					continue;
				fatal("getpkt: recvfrom");
			}
			if (getnl(ibuf) != netseq)
				break;
			len = getns(ibuf + 4);
			if (!memcmp(chksum(ibuf, len + 6), ibuf + 6 + len, 16)) {
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
			if (write(ipkdb_desc, obuf, olen) < 0) {
				if (errno == EINTR
#ifdef	ECONNREFUSED
				    || errno == ECONNREFUSED /* Solaris lossage??? */
#endif
				    )
					continue;
				fatal("getpkt: write");
			}
			gettimeofday(&deadline, NULL);
			deadline.tv_sec++;
			break;
		}
	}
}

/* Defint the target subroutine names */
static struct target_ops ipkdb_ops = {
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
	ipkdb_fetch_register,
	ipkdb_store_register,
	ipkdb_prepare_to_store,
	ipkdb_xfer_memory,
	ipkdb_files_info,
	ipkdb_insert_breakpoint,
	ipkdb_remove_breakpoint,
	0,					/* terminal_init */
	0,					/* terminal_inferior */
	0,					/* terminal_ours_for_output */
	0,					/* terminal_ours */
	0,					/* terminal_info */
	ipkdb_kill,
	0,					/* load */
	0,					/* lookup_symbol */
	0,					/* create_inferior */
	0,					/* mourn_inferior */
	0,					/* can_run */
	0,					/* notice_signals */
	0,					/* thread_alive */
	0,					/* stop */
	process_stratum,			/* stratum */
	0,					/* next (unused) */
	1,					/* has_all_memory */
	1,					/* has_memory */
	1,					/* has_stack */
	1,					/* has_registers */
	1,					/* has_execution */
	0,					/* sections */
	0,					/* sections_end */
	OPS_MAGIC				/* Always the last thing */
};

void
_initialize_ipkdb()
{
	add_target(&ipkdb_ops);

	setkeycmd = add_set_cmd("ipkdbkey", class_support, var_string,
				(char *)&key,
				"Set the key to be used for authentication of IPKDB debugging.\n\
The key given must be at least 16 bytes in length.",
				&setlist);
	setkeycmd->function.sfunc = hmac_init;
}
