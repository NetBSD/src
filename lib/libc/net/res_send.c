/*	$NetBSD: res_send.c,v 1.31 2000/07/27 00:35:02 itojun Exp $	*/

/*-
 * Copyright (c) 1985, 1989, 1993
 *    The Regents of the University of California.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)res_send.c	8.1 (Berkeley) 6/4/93";
static char rcsid[] = "Id: res_send.c,v 8.13 1997/06/01 20:34:37 vixie Exp ";
#else
__RCSID("$NetBSD: res_send.c,v 1.31 2000/07/27 00:35:02 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

	/* change this to "0"
	 * if you talk to a lot
	 * of multi-homed SunOS
	 * ("broken") name servers.
	 */
#define	CHECK_SRVR_ADDR	1	/* XXX - should be in options.h */

/*
 * Send query to name server and wait for reply.
 */

#if defined(_LIBC)
#include "namespace.h"
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>

#if defined(BSD) && (BSD >= 199306)
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
#else
# include "../conf/portability.h"
#endif

#if defined(USE_OPTIONS_H)
# include <../conf/options.h>
#endif

#ifdef __weak_alias
__weak_alias(res_close,__res_close)
__weak_alias(res_send,__res_send)
#endif

static int s = -1;	/* socket used for communications */
static int connected = 0;	/* is the socket connected */
static int vc = 0;	/* is the socket a virtual ciruit? */
static int af = 0;	/* address family of socket */

#define CAN_RECONNECT 1

#ifndef DEBUG
#   define Dprint(cond, args) /*empty*/
#   define DprintQ(cond, args, query, size) /*empty*/
#   define Aerror(file, string, error, address) /*empty*/
#   define Perror(file, string, error) /*empty*/
#else
#   define Dprint(cond, args) if (cond) {fprintf args;} else {}
#   define DprintQ(cond, args, query, size) if (cond) {\
			fprintf args;\
			__fp_nquery(query, size, stdout);\
		} else {}
    static char abuf[NI_MAXHOST];
    static char pbuf[32];
    static void Aerror __P((FILE *, char *, int, struct sockaddr *));
    static void Perror __P((FILE *, char *, int));

    static void
    Aerror(file, string, error, address)
	FILE *file;
	char *string;
	int error;
	struct sockaddr *address;
    {
	int save = errno;

	if (_res.options & RES_DEBUG) {
		if (getnameinfo(address, (size_t)address->sa_len, abuf,
		    sizeof(abuf), pbuf, sizeof(pbuf),
		    NI_NUMERICHOST|NI_NUMERICSERV|NI_WITHSCOPEID) != 0) {
			strcpy(abuf, "?");
			strcpy(pbuf, "?");
		}
		fprintf(file, "res_send: %s ([%s].%s): %s\n",
			string, abuf, pbuf, strerror(error));
	}
	errno = save;
    }
    static void
    Perror(file, string, error)
	FILE *file;
	char *string;
	int error;
    {
	int save = errno;

	if (_res.options & RES_DEBUG) {
		fprintf(file, "res_send: %s: %s\n",
			string, strerror(error));
	}
	errno = save;
    }
#endif

void res_send_setqhook __P((res_send_qhook));
void res_send_setrhook __P((res_send_rhook));

static res_send_qhook Qhook = NULL;
static res_send_rhook Rhook = NULL;

void
res_send_setqhook(hook)
	res_send_qhook hook;
{

	Qhook = hook;
}

void
res_send_setrhook(hook)
	res_send_rhook hook;
{

	Rhook = hook;
}

#ifdef INET6
static struct sockaddr * get_nsaddr __P((size_t));

/*
 * pick appropriate nsaddr_list for use.  see res_init() for initialization.
 */
static struct sockaddr *
get_nsaddr(n)
	size_t n;
{

	if (!_res.nsaddr_list[n].sin_family) {
		/*
		 * - _res_ext.nsaddr_list[n] holds an address that is larger
		 *   than struct sockaddr, and
		 * - user code did not update _res.nsaddr_list[n].
		 */
		return (struct sockaddr *)(void *)&_res_ext.nsaddr_list[n];
	} else {
		/*
		 * - user code updated _res.nsaddr_list[n], or
		 * - _res.nsaddr_list[n] has the same content as
		 *   _res_ext.nsaddr_list[n].
		 */
		return (struct sockaddr *)(void *)&_res.nsaddr_list[n];
	}
}
#else
#define get_nsaddr(n)	((struct sockaddr *)&_res.nsaddr_list[(n)])
#endif

/* int
 * res_isourserver(ina)
 *	looks up "ina" in _res.ns_addr_list[]
 * returns:
 *	0  : not found
 *	>0 : found
 * author:
 *	paul vixie, 29may94
 */
int
res_isourserver(inp)
	const struct sockaddr_in *inp;
{
#ifdef INET6
	const struct sockaddr_in6 *in6p = (const struct sockaddr_in6 *)
	    (const void *)inp;
	const struct sockaddr_in6 *srv6;
#endif
	const struct sockaddr_in *srv;
	int ns, ret;

	_DIAGASSERT(inp != NULL);

	ret = 0;
	switch (inp->sin_family) {
#ifdef INET6
	case AF_INET6:
		for (ns = 0; ns < _res.nscount; ns++) {
			srv6 = (struct sockaddr_in6 *)(void *)
			    get_nsaddr((size_t)ns);
			if (srv6->sin6_family == in6p->sin6_family &&
			    srv6->sin6_port == in6p->sin6_port &&
			    srv6->sin6_scope_id == in6p->sin6_scope_id &&
			    (memcmp(&srv6->sin6_addr, &in6addr_any,
				    sizeof(struct in6_addr)) == 0 ||
			     memcmp(&srv6->sin6_addr, &in6p->sin6_addr,
				    sizeof(struct in6_addr)) == 0)) {
				ret++;
				break;
			}
		}
		break;
#endif
	case AF_INET:
		for (ns = 0; ns < _res.nscount; ns++) {
			srv = (struct sockaddr_in *)
			    (void *)get_nsaddr((size_t)ns);
			if (srv->sin_family == inp->sin_family &&
			    srv->sin_port == inp->sin_port &&
			    (srv->sin_addr.s_addr == INADDR_ANY ||
			     srv->sin_addr.s_addr == inp->sin_addr.s_addr)) {
				ret++;
				break;
			}
		}
		break;
	}
	return (ret);
}

/* int
 * res_nameinquery(name, type, class, buf, eom)
 *	look for (name,type,class) in the query section of packet (buf,eom)
 * requires:
 *	buf + HFIXESDZ <= eom
 * returns:
 *	-1 : format error
 *	0  : not found
 *	>0 : found
 * author:
 *	paul vixie, 29may94
 */
int
res_nameinquery(name, type, class, buf, eom)
	const char *name;
	int type, class;
	const u_char *buf, *eom;
{
	const u_char *cp = buf + HFIXEDSZ;
	int qdcount = ntohs(((const HEADER*)(const void *)buf)->qdcount);

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(buf != NULL);
	_DIAGASSERT(eom != NULL);

	while (qdcount-- > 0) {
		char tname[MAXDNAME+1];
		int n, ttype, tclass;

		n = dn_expand(buf, eom, cp, tname, sizeof tname);
		if (n < 0)
			return (-1);
		cp += n;
		if (cp + 2 * INT16SZ > eom)
			return (-1);
		ttype = _getshort(cp); cp += INT16SZ;
		tclass = _getshort(cp); cp += INT16SZ;
		if (ttype == type &&
		    tclass == class &&
		    strcasecmp(tname, name) == 0)
			return (1);
	}
	return (0);
}

/* int
 * res_queriesmatch(buf1, eom1, buf2, eom2)
 *	is there a 1:1 mapping of (name,type,class)
 *	in (buf1,eom1) and (buf2,eom2)?
 * returns:
 *	-1 : format error
 *	0  : not a 1:1 mapping
 *	>0 : is a 1:1 mapping
 * author:
 *	paul vixie, 29may94
 */
int
res_queriesmatch(buf1, eom1, buf2, eom2)
	const u_char *buf1, *eom1;
	const u_char *buf2, *eom2;
{
	const u_char *cp = buf1 + HFIXEDSZ;
	int qdcount = ntohs(((const HEADER *)(const void *)buf1)->qdcount);

	_DIAGASSERT(buf1 != NULL);
	_DIAGASSERT(eom1 != NULL);
	_DIAGASSERT(buf2 != NULL);
	_DIAGASSERT(eom2 != NULL);

	if (buf1 + HFIXEDSZ > eom1 || buf2 + HFIXEDSZ > eom2)
		return (-1);

	if (qdcount != ntohs(((const HEADER *)(const void *)buf2)->qdcount))
		return (0);
	while (qdcount-- > 0) {
		char tname[MAXDNAME+1];
		int n, ttype, tclass;

		n = dn_expand(buf1, eom1, cp, tname, sizeof tname);
		if (n < 0)
			return (-1);
		cp += n;
		if (cp + 2 * INT16SZ > eom1)
			return (-1);
		ttype = _getshort(cp);	cp += INT16SZ;
		tclass = _getshort(cp); cp += INT16SZ;
		if (!res_nameinquery(tname, ttype, tclass, buf2, eom2))
			return (0);
	}
	return (1);
}

int
res_send(buf, buflen, ans, anssiz)
	const u_char *buf;
	int buflen;
	u_char *ans;
	int anssiz;
{
	const HEADER *hp = (const HEADER *)(const void *)buf;
	HEADER *anhp = (HEADER *)(void *)ans;
	int gotsomewhere, connreset, terrno, try, v_circuit, resplen, ns;
	int n;
	u_int badns;	/* XXX NSMAX can't exceed #/bits in this var */

	_DIAGASSERT(buf != NULL);
	_DIAGASSERT(ans != NULL);

	if ((_res.options & RES_INIT) == 0 && res_init() == -1) {
		/* errno should have been set by res_init() in this case. */
		return (-1);
	}
	if (anssiz < HFIXEDSZ) {
		errno = EINVAL;
		return (-1);
	}
	DprintQ((_res.options & RES_DEBUG) || (_res.pfcode & RES_PRF_QUERY),
		(stdout, ";; res_send()\n"), buf, buflen);
	v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
	gotsomewhere = 0;
	connreset = 0;
	terrno = ETIMEDOUT;
	badns = 0;

	/*
	 * Send request, RETRY times, or until successful
	 */
	for (try = 0; try < _res.retry; try++) {
	    for (ns = 0; ns < _res.nscount; ns++) {
		struct sockaddr *nsap = get_nsaddr((size_t)ns);
		socklen_t salen;

		if (nsap->sa_len)
			salen = nsap->sa_len;
#ifdef INET6
		else if (nsap->sa_family == AF_INET6)
			salen = sizeof(struct sockaddr_in6);
#endif
		else if (nsap->sa_family == AF_INET)
			salen = sizeof(struct sockaddr_in);
		else
			salen = 0;	/* unknown, die on connect */

    same_ns:
		if (badns & (1 << ns)) {
			res_close();
			goto next_ns;
		}

		if (Qhook) {
			int done = 0, loops = 0;

			do {
				res_sendhookact act;

				act = (*Qhook)((struct sockaddr_in **)&nsap,
					       &buf, &buflen,
					       ans, anssiz, &resplen);
				switch (act) {
				case res_goahead:
					done = 1;
					break;
				case res_nextns:
					res_close();
					goto next_ns;
				case res_done:
					return (resplen);
				case res_modified:
					/* give the hook another try */
					if (++loops < 42) /*doug adams*/
						break;
					/*FALLTHROUGH*/
				case res_error:
					/*FALLTHROUGH*/
				default:
					return (-1);
				}
			} while (!done);
		}

		Dprint((_res.options & RES_DEBUG) &&
		       getnameinfo(nsap, salen, abuf, sizeof(abuf),
			   NULL, 0, NI_NUMERICHOST | NI_WITHSCOPEID) == 0,
		       (stdout, ";; Querying server (# %d) address = %s\n",
			ns + 1, abuf));

		if (v_circuit) {
			int truncated;
			struct iovec iov[2];
			u_short len;
			u_char *cp;

			/*
			 * Use virtual circuit;
			 * at most one attempt per server.
			 */
			try = _res.retry;
			truncated = 0;
			if ((s < 0) || (!vc) || (af != nsap->sa_family)) {
				if (s >= 0)
					res_close();

				af = nsap->sa_family;
				s = socket(af, SOCK_STREAM, 0);
				if (s < 0) {
					terrno = errno;
					Perror(stderr, "socket(vc)", errno);
#if 0
					return (-1);
#else
					badns |= (1 << ns);
					res_close();
					goto next_ns;
#endif
				}
				errno = 0;
				if (connect(s, nsap, salen) < 0) {
					terrno = errno;
					Aerror(stderr, "connect/vc",
					       errno, nsap);
					badns |= (1 << ns);
					res_close();
					goto next_ns;
				}
				vc = 1;
			}
			/*
			 * Send length & message
			 */
			putshort((u_short)buflen, (u_char*)(void *)&len);
			iov[0].iov_base = &len;
			iov[0].iov_len = INT16SZ;
			/* LINTED write does not affect base */
			iov[1].iov_base = (void *)buf;
			iov[1].iov_len = buflen;
			if (writev(s, iov, 2) != (INT16SZ + buflen)) {
				terrno = errno;
				Perror(stderr, "write failed", errno);
				badns |= (1 << ns);
				res_close();
				goto next_ns;
			}
			/*
			 * Receive length & response
			 */
read_len:
			cp = ans;
			len = INT16SZ;
			while ((n = read(s, (char *)cp, (size_t)len)) > 0) {
				cp += n;
				/* len is unsigned, no need to compare < 0 */
				if ((len -= n) == 0)
					break;
			}
			if (n <= 0) {
				terrno = errno;
				Perror(stderr, "read failed", errno);
				res_close();
				/*
				 * A long running process might get its TCP
				 * connection reset if the remote server was
				 * restarted.  Requery the server instead of
				 * trying a new one.  When there is only one
				 * server, this means that a query might work
				 * instead of failing.  We only allow one reset
				 * per query to prevent looping.
				 */
				if (terrno == ECONNRESET && !connreset) {
					connreset = 1;
					res_close();
					goto same_ns;
				}
				res_close();
				goto next_ns;
			}
			resplen = _getshort(ans);
			if (resplen > anssiz) {
				Dprint(_res.options & RES_DEBUG,
				       (stdout, ";; response truncated\n")
				       );
				truncated = 1;
				len = anssiz;
			} else
				len = resplen;
			if (len < HFIXEDSZ) {
				/*
				 * Undersized message.
				 */
				Dprint(_res.options & RES_DEBUG,
				       (stdout, ";; undersized: %d\n", len));
				terrno = EMSGSIZE;
				badns |= (1 << ns);
				res_close();
				goto next_ns;
			}
			cp = ans;
			while (len != 0 &&
			       (n = read(s, cp, (size_t)len)) > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = errno;
				Perror(stderr, "read(vc)", errno);
				res_close();
				goto next_ns;
			}
			if (truncated) {
				/*
				 * Flush rest of answer
				 * so connection stays in synch.
				 */
				anhp->tc = 1;
				len = resplen - anssiz;
				while (len != 0) {
					char junk[PACKETSZ];

					n = (len > sizeof(junk)
					     ? sizeof(junk)
					     : len);
					if ((n = read(s, junk, (size_t)n)) > 0)
						len -= n;
					else
						break;
				}
			}
			/*
			 * The calling applicating has bailed out of
			 * a previous call and failed to arrange to have
			 * the circuit closed or the server has got
			 * itself confused. Anyway drop the packet and
			 * wait for the correct one.
			 */
			if (hp->id != anhp->id) {
				DprintQ((_res.options & RES_DEBUG) ||
					(_res.pfcode & RES_PRF_REPLY),
					(stdout, ";; old answer (unexpected):\n"),
					ans, (resplen>anssiz)?anssiz:resplen);
				goto read_len;
			}
		} else {
			/*
			 * Use datagrams.
			 */
			time_t seconds, now, timeout, finish;
			struct pollfd dsfd;
			struct sockaddr_storage from;
			socklen_t fromlen;

			if ((s < 0) || vc || (af != nsap->sa_family)) {
				if (vc)
					res_close();
				af = nsap->sa_family;
				s = socket(af, SOCK_DGRAM, 0);
				if (s < 0) {
#if !CAN_RECONNECT
 bad_dg_sock:
#endif
					terrno = errno;
					Perror(stderr, "socket(dg)", errno);
#if 0
					return (-1);
#else
					badns |= (1 << ns);
					res_close();
					goto next_ns;
#endif
				}
				connected = 0;
			}
			/*
			 * On a 4.3BSD+ machine (client and server,
			 * actually), sending to a nameserver datagram
			 * port with no nameserver will cause an
			 * ICMP port unreachable message to be returned.
			 * If our datagram socket is "connected" to the
			 * server, we get an ECONNREFUSED error on the next
			 * socket operation, and poll returns if the
			 * error message is received.  We can thus detect
			 * the absence of a nameserver without timing out.
			 * If we have sent queries to at least two servers,
			 * however, we don't want to remain connected,
			 * as we wish to receive answers from the first
			 * server to respond.
			 */
			if (_res.nscount == 1 || (try == 0 && ns == 0)) {
				/*
				 * Connect only if we are sure we won't
				 * receive a response from another server.
				 */
				if (!connected) {
					if (connect(s, nsap, salen) < 0) {
						Aerror(stderr,
						       "connect(dg)",
						       errno, nsap);
						badns |= (1 << ns);
						res_close();
						goto next_ns;
					}
					connected = 1;
				}
				if (send(s, buf, (size_t)buflen, 0) != buflen) {
					Perror(stderr, "send", errno);
					badns |= (1 << ns);
					res_close();
					goto next_ns;
				}
			} else {
				/*
				 * Disconnect if we want to listen
				 * for responses from more than one server.
				 */
				if (connected) {
#if CAN_RECONNECT
#ifdef INET6
					/* XXX: any errornous address */
#endif /* INET6 */
					struct sockaddr_in no_addr;

					no_addr.sin_family = AF_INET;
					no_addr.sin_addr.s_addr = INADDR_ANY;
					no_addr.sin_port = 0;
					(void) connect(s,
					    (struct sockaddr *)(void *)&no_addr,
					    sizeof(no_addr));
#else
					int s1 = socket(af, SOCK_DGRAM,0);
					if (s1 < 0)
						goto bad_dg_sock;
					(void) dup2(s1, s);
					(void) close(s1);
					Dprint(_res.options & RES_DEBUG,
					       (stdout, ";; new DG socket\n"))
#endif
					connected = 0;
					errno = 0;
				}
				if (sendto(s, buf, (size_t)buflen, 0,
				    nsap, salen) != buflen) {
					Aerror(stderr, "sendto", errno, nsap);
					badns |= (1 << ns);
					res_close();
					goto next_ns;
				}
			}

			/*
			 * Wait for reply
			 */
			seconds = (_res.retrans << try);
			if (try > 0)
				seconds /= _res.nscount;
			if ((long) seconds <= 0)
				seconds = 1;
			now = time(NULL);
			timeout = seconds;
			finish = now + timeout;
			dsfd.fd = s;
			dsfd.events = POLLIN;
wait:
			n = poll(&dsfd, 1, (int)(timeout * 1000));
			if (n < 0) {
				if (errno == EINTR) {
					now = time(NULL);
					if (finish > now) {
						timeout = finish - now;
						goto wait;
					}
				}
				Perror(stderr, "poll", errno);
				res_close();
				goto next_ns;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
				Dprint(_res.options & RES_DEBUG,
				       (stdout, ";; timeout\n"));
				gotsomewhere = 1;
				res_close();
				goto next_ns;
			}
			errno = 0;
			fromlen = sizeof from;
			resplen = recvfrom(s, ans, (size_t)anssiz, 0,
			    (struct sockaddr *)(void *)&from, &fromlen);
			if (resplen <= 0) {
				Perror(stderr, "recvfrom", errno);
				res_close();
				goto next_ns;
			}
			gotsomewhere = 1;
			if (resplen < HFIXEDSZ) {
				/*
				 * Undersized message.
				 */
				Dprint(_res.options & RES_DEBUG,
				       (stdout, ";; undersized: %d\n",
					resplen));
				terrno = EMSGSIZE;
				badns |= (1 << ns);
				res_close();
				goto next_ns;
			}
			if (hp->id != anhp->id) {
				/*
				 * response from old query, ignore it.
				 * XXX - potential security hazard could
				 *	 be detected here.
				 */
				DprintQ((_res.options & RES_DEBUG) ||
					(_res.pfcode & RES_PRF_REPLY),
					(stdout, ";; old answer:\n"),
					ans, (resplen>anssiz)?anssiz:resplen);
				goto wait;
			}
#if CHECK_SRVR_ADDR
			if (!(_res.options & RES_INSECURE1) &&
			    !res_isourserver(
			    (struct sockaddr_in *)(void *)&from)) {
				/*
				 * response from wrong server? ignore it.
				 * XXX - potential security hazard could
				 *	 be detected here.
				 */
				DprintQ((_res.options & RES_DEBUG) ||
					(_res.pfcode & RES_PRF_REPLY),
					(stdout, ";; not our server:\n"),
					ans, (resplen>anssiz)?anssiz:resplen);
				goto wait;
			}
#endif
			if (!(_res.options & RES_INSECURE2) &&
			    !res_queriesmatch(buf, buf + buflen,
					      ans, ans + anssiz)) {
				/*
				 * response contains wrong query? ignore it.
				 * XXX - potential security hazard could
				 *	 be detected here.
				 */
				DprintQ((_res.options & RES_DEBUG) ||
					(_res.pfcode & RES_PRF_REPLY),
					(stdout, ";; wrong query name:\n"),
					ans, (resplen>anssiz)?anssiz:resplen);
				goto wait;
			}
			if (anhp->rcode == SERVFAIL ||
			    anhp->rcode == NOTIMP ||
			    anhp->rcode == REFUSED) {
				DprintQ(_res.options & RES_DEBUG,
					(stdout, "server rejected query:\n"),
					ans, (resplen>anssiz)?anssiz:resplen);
				badns |= (1 << ns);
				res_close();
				/* don't retry if called from dig */
				if (!_res.pfcode)
					goto next_ns;
			}
			if (!(_res.options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer;
				 * use TCP with same server.
				 */
				Dprint(_res.options & RES_DEBUG,
				       (stdout, ";; truncated answer\n"));
				v_circuit = 1;
				res_close();
				goto same_ns;
			}
		} /*if vc/dg*/
		Dprint((_res.options & RES_DEBUG) ||
		       ((_res.pfcode & RES_PRF_REPLY) &&
			(_res.pfcode & RES_PRF_HEAD1)),
		       (stdout, ";; got answer:\n"));
		DprintQ((_res.options & RES_DEBUG) ||
			(_res.pfcode & RES_PRF_REPLY),
			(stdout, "%s", ""),
			ans, (resplen>anssiz)?anssiz:resplen);
		/*
		 * If using virtual circuits, we assume that the first server
		 * is preferred over the rest (i.e. it is on the local
		 * machine) and only keep that one open.
		 * If we have temporarily opened a virtual circuit,
		 * or if we haven't been asked to keep a socket open,
		 * close the socket.
		 */
		if ((v_circuit && (!(_res.options & RES_USEVC) || ns != 0)) ||
		    !(_res.options & RES_STAYOPEN)) {
			res_close();
		}
		if (Rhook) {
			int done = 0, loops = 0;

			do {
				res_sendhookact act;

				act = (*Rhook)(
				    (struct sockaddr_in *)(void *)nsap,
				    buf, buflen, ans, anssiz, &resplen);
				switch (act) {
				case res_goahead:
				case res_done:
					done = 1;
					break;
				case res_nextns:
					res_close();
					goto next_ns;
				case res_modified:
					/* give the hook another try */
					if (++loops < 42) /*doug adams*/
						break;
					/*FALLTHROUGH*/
				case res_error:
					/*FALLTHROUGH*/
				default:
					return (-1);
				}
			} while (!done);

		}
		return (resplen);
    next_ns: ;
	   } /*foreach ns*/
	} /*foreach retry*/
	res_close();
	if (!v_circuit)
		if (!gotsomewhere)
			errno = ECONNREFUSED;	/* no nameservers found */
		else
			errno = ETIMEDOUT;	/* no answer obtained */
	else
		errno = terrno;
	return (-1);
}

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
void
res_close()
{
	if (s >= 0) {
		(void) close(s);
		s = -1;
		connected = 0;
		vc = 0;
		af = 0;
	}
}

#ifdef ultrix
/* XXX: remove this cruft? - lukem */
/* ultrix 4.0 had some icky packaging in its libc.a.  alias for it here.
 * there is more gunk of this kind over in res_debug.c.
 */

void
_res_close()
{
	res_close();
}

#undef res_send
int
res_send(buf, buflen, ans, anssiz)
	const u_char *buf;
	int buflen;
	u_char *ans;
	int anssiz;
{
	return (__res_send(buf, buflen, ans, anssiz));
}
#endif /* Ultrix 4.0 hackery */
