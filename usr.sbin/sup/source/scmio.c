/*	$NetBSD: scmio.c,v 1.15 2003/08/27 08:15:16 itojun Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * SUP Communication Module for 4.3 BSD
 *
 * SUP COMMUNICATION MODULE SPECIFICATIONS:
 *
 * IN THIS MODULE:
 *
 * OUTPUT TO NETWORK
 *
 *   MESSAGE START/END
 *	writemsg (msg)		start message
 *	  int msg;			message type
 *	writemend ()		end message and flush data to network
 *
 *   MESSAGE DATA
 *	writeint (i)		write int
 *	  int i;			integer to write
 *	writestring (p)		write string
 *	  char *p;			string pointer
 *	writefile (f)		write open file
 *	  int f;			open file descriptor
 *
 *   COMPLETE MESSAGE (start, one data block, end)
 *	writemnull (msg)	write message with no data
 *	  int msg;			message type
 *	writemint (msg,i)	write int message
 *	  int msg;			message type
 *	  int i;			integer to write
 *	writemstr (msg,p)	write string message
 *	  int msg;			message type
 *	  char *p;			string pointer
 *
 * INPUT FROM NETWORK
 *   MESSAGE START/END
 *	readflush ()		flush any unread data (close)
 *	readmsg (msg)		read specified message type
 *	  int msg;			message type
 *	readmend ()		read message end
 *
 *   MESSAGE DATA
 *	readskip ()		skip over one input data block
 *	readint (i)		read int
 *	  int *i;			pointer to integer
 *	readstring (p)		read string
 *	  char **p;			pointer to string pointer
 *	readfile (f)		read into open file
 *	  int f;			open file descriptor
 *
 *   COMPLETE MESSAGE (start, one data block, end)
 *	readmnull (msg)		read message with no data
 *	  int msg;			message type
 *	readmint (msg,i)	read int message
 *	  int msg;			message type
 *	  int *i;			pointer to integer
 *	readmstr (msg,p)	read string message
 *	  int msg;			message type
 *	  char **p;			pointer to string pointer
 *
 * RETURN CODES
 *	All routines normally return SCMOK.  SCMERR may be returned
 *	by any routine on abnormal (usually fatal) errors.  An
 *	unexpected MSGGOAWAY will result in SCMEOF being returned.
 *
 * COMMUNICATION PROTOCOL
 *	Messages always alternate, with the first message after
 *	connecting being sent by the client process.
 *
 *	At the end of the conversation, the client process will
 *	send a message to the server telling it to go away.  Then,
 *	both processes will close the network connection.
 *
 *	Any time a process receives a message it does not expect,
 *	the "readmsg" routine will send a MSGGOAWAY message and
 *	return SCMEOF.
 *
 *	Each message has this format:
 *	    ----------    ------------    ------------         ----------
 *	    |msg type|    |count|data|    |count|data|   ...   |ENDCOUNT|
 *	    ----------    ------------    ------------         ----------
 *	size:  int	    int  var.	    int  var.	   	  int
 *
 *	All ints are assumed to be 32-bit quantities.  A message
 *	with no data simply has a message type followed by ENDCOUNT.
 *
 **********************************************************************
 * HISTORY
 * Revision 1.7  92/09/09  22:04:41  mrt
 * 	Removed the data encryption routines from here to netcrypt.c
 * 	[92/09/09            mrt]
 *
 * Revision 1.6  92/08/11  12:05:57  mrt
 * 	Brad's changes: Delinted,Added forward declarations of
 * 	static functions. Added copyright.
 * 	[92/07/24            mrt]
 *
 * 27-Dec-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added crosspatch support.
 *
 * 28-Jun-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Found error in debugging code for readint().
 *
 * 01-Apr-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to readdata to "push" data back into the data buffer.
 *	Added prereadcount() to return the message count size after
 *	reading it and then pushing it back into the buffer.  Clear
 *	any encryption when a GOAWAY message is detected before reading
 *	the reason string. [V5.19]
 *
 * 02-Oct-86  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Put a timeout on reading from the network.
 *
 * 25-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Renamed "howmany" parameter to routines "encode" and "decode" from
 *	to "count" to avoid conflict with 4.3BSD macro.
 *
 * 15-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added readflush() to flush any unread data from the input
 *	buffer.  Called by requestend() in scm.c module.
 *
 * 19-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added register variables to decode() for speedup.  Added I/O
 *	buffering to reduce the number or read/write system calls.
 *	Removed readmfil/writemfil routines which were not used and were
 *	not compatible with the other similarly defined routines anyway.
 *
 * 19-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created from scm.c I/O and crypt routines.
 *
 **********************************************************************
 */

#include "libc.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/poll.h>
#include "supcdefs.h"
#include "supextern.h"
#include "supmsg.h"
#ifndef INFTIM
#define INFTIM -1
#endif

/*************************
 ***    M A C R O S    ***
 *************************/

/* end of message */
#define ENDCOUNT (-1)		/* end of message marker */
#define NULLCOUNT (-2)		/* used for sending NULL pointer */

#define RETRIES 15		/* # of times to retry io */
#define FILEXFER 2048		/* block transfer size */
#define XFERSIZE(count) ((count > FILEXFER) ? FILEXFER : count)

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

extern int netfile;		/* network file descriptor */

int scmdebug;			/* scm debug flag */

int cryptflag;			/* whether to encrypt/decrypt data */
char *cryptbuf;			/* buffer for data encryption/decryption */

extern char *goawayreason;	/* reason for goaway message */

struct buf {
	char b_data[FILEXFER];	/* buffered data */
	char *b_ptr;		/* pointer to end of buffer */
	int b_cnt;		/* number of bytes in buffer */
}   buffers[2];
struct buf *bufptr;		/* buffer pointer */

static int writedata(int, char *);
static int writeblock(int, char *);
static int readdata(int, char *);
static int readcount(int *);


/***********************************************
 ***    O U T P U T   T O   N E T W O R K    ***
 ***********************************************/

static int
writedata(int count, char *data)
{				/* write raw data to network */
	int x, tries;
	struct buf *bp;

	if (bufptr) {
		if (bufptr->b_cnt + count <= FILEXFER) {
			bcopy(data, bufptr->b_ptr, count);
			bufptr->b_cnt += count;
			bufptr->b_ptr += count;
			return (SCMOK);
		}
		bp = (bufptr == buffers) ? &buffers[1] : buffers;
		bcopy(data, bp->b_data, count);
		bp->b_cnt = count;
		bp->b_ptr = bp->b_data + count;
		data = bufptr->b_data;
		count = bufptr->b_cnt;
		bufptr->b_cnt = 0;
		bufptr->b_ptr = bufptr->b_data;
		bufptr = bp;
	}
	tries = 0;
	for (;;) {
		errno = 0;
		x = write(netfile, data, count);
		if (x > 0)
			break;
		if (errno)
			break;
		if (++tries > RETRIES)
			break;
		if (scmdebug > 0)
			logerr("SCM Retrying failed network write");
	}
	if (x <= 0) {
		if (errno == EPIPE)
			return (scmerr(-1, "Network write timed out"));
		if (errno)
			return (scmerr(errno, "Write error on network"));
		return (scmerr(-1, "Write retries failed"));
	}
	if (x != count)
		return (scmerr(-1, "Write error on network returned %d on write of %d", x, count));
	return (SCMOK);
}

static int
writeblock(int count, char *data)
{				/* write data block */
	int x;
	int y = byteswap(count);

	x = writedata(sizeof(int), (char *) &y);
	if (x == SCMOK)
		x = writedata(count, data);
	return (x);
}

int
writemsg(int msg)
{				/* write start of message */
	int x;

	if (scmdebug > 1)
		loginfo("SCM Writing message %d", msg);
	if (bufptr)
		return (scmerr(-1, "Buffering already enabled"));
	bufptr = buffers;
	bufptr->b_ptr = bufptr->b_data;
	bufptr->b_cnt = 0;
	x = byteswap(msg);
	return (writedata(sizeof(int), (char *) &x));
}

int
writemend(void)
{				/* write end of message */
	int count;
	char *data;
	int x;

	x = byteswap(ENDCOUNT);
	x = writedata(sizeof(int), (char *) &x);
	if (x != SCMOK)
		return (x);
	if (bufptr == NULL)
		return (scmerr(-1, "Buffering already disabled"));
	if (bufptr->b_cnt == 0) {
		bufptr = NULL;
		return (SCMOK);
	}
	data = bufptr->b_data;
	count = bufptr->b_cnt;
	bufptr = NULL;
	return (writedata(count, data));
}

int
writeint(int i)
{				/* write int as data block */
	int x;
	if (scmdebug > 2)
		loginfo("SCM Writing integer %d", i);
	x = byteswap(i);
	return (writeblock(sizeof(int), (char *) &x));
}

int
writestring(char *p)
{				/* write string as data block */
	int len, x;
	if (p == NULL) {
		int y = byteswap(NULLCOUNT);
		if (scmdebug > 2)
			loginfo("SCM Writing string NULL");
		return (writedata(sizeof(int), (char *) &y));
	}
	if (scmdebug > 2)
		loginfo("SCM Writing string %s", p);
	len = strlen(p);
	if (cryptflag) {
		x = getcryptbuf(len + 1);
		if (x != SCMOK)
			return (x);
		encode(p, cryptbuf, len);
		p = cryptbuf;
	}
	return (writeblock(len, p));
}

int
writefile(int f)
{				/* write open file as a data block */
	char buf[FILEXFER];
	int number = 0, sum = 0, filesize, x;
	int y;
	struct stat statbuf;

	if (fstat(f, &statbuf) < 0)
		return (scmerr(errno, "Can't access open file for message"));
	filesize = statbuf.st_size;
	y = byteswap(filesize);
	x = writedata(sizeof(int), (char *) &y);

	if (cryptflag)
		x = getcryptbuf(FILEXFER);

	if (x == SCMOK) {
		sum = 0;
		do {
			number = read(f, buf, FILEXFER);
			if (number > 0) {
				if (cryptflag) {
					encode(buf, cryptbuf, number);
					x = writedata(number, cryptbuf);
				} else {
					x = writedata(number, buf);
				}
				sum += number;
			}
		} while (x == SCMOK && number > 0);
	}
	if (sum != filesize)
		return (scmerr(-1, "File size error on output message"));
	if (number < 0)
		return (scmerr(errno, "Read error on file output message"));
	return (x);
}

int
writemnull(int msg)
{				/* write message with no data */
	int x;
	x = writemsg(msg);
	if (x == SCMOK)
		x = writemend();
	return (x);
}

int
writemint(int msg, int i)
{				/* write message of one int */
	int x;
	x = writemsg(msg);
	if (x == SCMOK)
		x = writeint(i);
	if (x == SCMOK)
		x = writemend();
	return (x);
}

int
writemstr(int msg, char *p)
{				/* write message of one string */
	int x;
	x = writemsg(msg);
	if (x == SCMOK)
		x = writestring(p);
	if (x == SCMOK)
		x = writemend();
	return (x);
}
/*************************************************
 ***    I N P U T   F R O M   N E T W O R K    ***
 *************************************************/

static int
readdata(int count, char *data)
{				/* read raw data from network */
	char *p;
	int c, n, m, x;
	static int bufcnt = 0;
	static char *bufptr;
	static char buffer[FILEXFER];
	struct pollfd set[1];

	if (count < 0) {
		if (bufptr + count < buffer)
			return (scmerr(-1, "No space in buffer %d", count));
		bufptr += count;
		bufcnt -= count;
		bcopy(data, bufptr, -count);
		return (SCMOK);
	}
	if (count == 0 && data == NULL) {
		bufcnt = 0;
		return (SCMOK);
	}
	if (count <= bufcnt) {
		bcopy(bufptr, data, count);
		bufptr += count;
		bufcnt -= count;
		return (SCMOK);
	}
	if (bufcnt > 0) {
		bcopy(bufptr, data, bufcnt);
		data += bufcnt;
		count -= bufcnt;
	}
	bufptr = buffer;
	bufcnt = 0;
	set[0].fd = netfile;
	set[0].events = POLLIN;
	p = buffer;
	n = FILEXFER;
	m = count;
	while (m > 0) {
		while ((c = poll(set, 1, 2 * 60 * 60 * 1000)) < 1) {
			if (c == 0)
				return (scmerr(-1, "Timeout on network input"));
			if (errno != EINTR)
				sleep(5);
		}
		x = read(netfile, p, n);
		if (x == 0)
			return (scmerr(-1, "Premature EOF on network input"));
		if (x < 0)
			return (scmerr(errno, "Read error on network"));
		p += x;
		n -= x;
		m -= x;
		bufcnt += x;
	}
	bcopy(bufptr, data, count);
	bufptr += count;
	bufcnt -= count;
	return (SCMOK);
}

static int 
readcount(int *count)
{				/* read count of data block */
	int x;
	int y;
	x = readdata(sizeof(int), (char *) &y);
	if (x != SCMOK)
		return (x);
	*count = byteswap(y);
	return (SCMOK);
}

int 
prereadcount(int *count)
{				/* preread count of data block */
	int x;
	int y;
	x = readdata(sizeof(int), (char *) &y);
	if (x != SCMOK)
		return (x);
	x = readdata(-((int) (sizeof(int))), (char *) &y);
	if (x != SCMOK)
		return (x);
	*count = byteswap(y);
	return (SCMOK);
}

int
readflush(void)
{
	return (readdata(0, (char *) NULL));
}

int
readmsg(int msg)
{				/* read header for expected message */
	/* if message is unexpected, send back SCMHUH */
	int x;
	int m;
	if (scmdebug > 1)
		loginfo("SCM Reading message %d", msg);
	x = readdata(sizeof(int), (char *) &m);	/* msg type */
	if (x != SCMOK)
		return (x);
	m = byteswap(m);
	if (m == msg)
		return (x);

	/* check for MSGGOAWAY in case he noticed problems first */
	if (m != MSGGOAWAY)
		return (scmerr(-1, "Received unexpected message %d", m));
	(void) netcrypt((char *) NULL);
	(void) readstring(&goawayreason);
	(void) readmend();
	if (goawayreason == NULL)
		return (SCMEOF);
	logerr("SCM GOAWAY %s", goawayreason);
	return (SCMEOF);
}

int
readmend(void)
{
	int x;
	int y;
	x = readdata(sizeof(int), (char *) &y);
	y = byteswap(y);
	if (x == SCMOK && y != ENDCOUNT)
		return (scmerr(-1, "Error reading end of message"));
	return (x);
}

int
readskip(void)
{				/* skip over one input block */
	int x;
	int n;
	char buf[FILEXFER];
	x = readcount(&n);
	if (x != SCMOK)
		return (x);
	if (n < 0)
		return (scmerr(-1, "Invalid message count %d", n));
	while (x == SCMOK && n > 0) {
		x = readdata(XFERSIZE(n), buf);
		n -= XFERSIZE(n);
	}
	return (x);
}

int 
readint(int *buf)
{				/* read int data block */
	int x;
	int y;
	x = readcount(&y);
	if (x != SCMOK)
		return (x);
	if (y < 0)
		return (scmerr(-1, "Invalid message count %d", y));
	if (y != sizeof(int))
		return (scmerr(-1, "Size error for int message is %d", y));
	x = readdata(sizeof(int), (char *) &y);
	(*buf) = byteswap(y);
	if (scmdebug > 2)
		loginfo("SCM Reading integer %d", *buf);
	return (x);
}

int 
readstring(char **buf)
{				/* read string data block */
	int x;
	int count;
	char *p;

	x = readcount(&count);
	if (x != SCMOK)
		return (x);
	if (count == NULLCOUNT) {
		if (scmdebug > 2)
			loginfo("SCM Reading string NULL");
		*buf = NULL;
		return (SCMOK);
	}
	if (count < 0)
		return (scmerr(-1, "Invalid message count %d", count));
	if (scmdebug > 3)
		loginfo("SCM Reading string count %d", count);
	if ((p = (char *) malloc((unsigned) count + 1)) == NULL)
		return (scmerr(-1, "Can't malloc %d bytes for string", count));
	if (cryptflag) {
		x = getcryptbuf(count + 1);
		if (x == SCMOK)
			x = readdata(count, cryptbuf);
		if (x != SCMOK) {
			free(p);
			return (x);
		}
		if (scmdebug > 3)
			printf("SCM Reading encrypted string %s\n", cryptbuf);
		decode(cryptbuf, p, count);
	} else {
		x = readdata(count, p);
		if (x != SCMOK) {
			free(p);
			return (x);
		}
	}
	p[count] = 0;		/* NULL at end of string */
	*buf = p;
	if (scmdebug > 2)
		loginfo("SCM Reading string %s", *buf);
	return (SCMOK);
}

int
readfile(int f)
{				/* read data block into open file */
	int x;
	int count;
	char buf[FILEXFER];

	if (cryptflag) {
		x = getcryptbuf(FILEXFER);
		if (x != SCMOK)
			return (x);
	}
	x = readcount(&count);
	if (x != SCMOK)
		return (x);
	if (count < 0)
		return (scmerr(-1, "Invalid message count %d", count));
	while (x == SCMOK && count > 0) {
		if (cryptflag) {
			x = readdata(XFERSIZE(count), cryptbuf);
			if (x == SCMOK)
				decode(cryptbuf, buf, XFERSIZE(count));
		} else
			x = readdata(XFERSIZE(count), buf);
		if (x == SCMOK) {
			(void) write(f, buf, XFERSIZE(count));
			count -= XFERSIZE(count);
		}
	}
	return (x);
}

int
readmnull(int msg)
{				/* read null message */
	int x;
	x = readmsg(msg);
	if (x == SCMOK)
		x = readmend();
	return (x);
}

int
readmint(int msg, int *buf)
{				/* read int message */
	int x;
	x = readmsg(msg);
	if (x == SCMOK)
		x = readint(buf);
	if (x == SCMOK)
		x = readmend();
	return (x);
}

int 
readmstr(int msg, char **buf)
{				/* read string message */
	int x;
	x = readmsg(msg);
	if (x == SCMOK)
		x = readstring(buf);
	if (x == SCMOK)
		x = readmend();
	return (x);
}
/**********************************
 ***    C R O S S P A T C H     ***
 **********************************/

void
crosspatch(void)
{
	struct pollfd set[2];
	int c;
	char buf[STRINGLENGTH];

	set[0].fd = STDIN_FILENO;
	set[0].events = POLLIN;
	set[1].fd = netfile;
	set[1].events = POLLIN;
	for (;;) {
		if ((c = poll(set, 2, INFTIM)) < 1) {
			if (c == -1) {
				if (errno == EINTR) {
					continue;
				}
			}
			sleep(5);
			continue;
		}
		if (set[1].revents & POLLIN) {
			c = read(netfile, buf, sizeof(buf));
			if (c < 0 && errno == EWOULDBLOCK)
				c = 0;
			else {
				if (c <= 0) {
					break;
				}
				(void) write(1, buf, c);
			}
		}
		if (set[0].revents & POLLIN) {
			c = read(0, buf, sizeof(buf));
			if (c < 0 && errno == EWOULDBLOCK)
				c = 0;
			else {
				if (c <= 0)
					break;
				(void) write(netfile, buf, c);
			}
		}
	}
}
