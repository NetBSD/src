/*	$KAME: getcertsbyname.c,v 1.7 2001/11/16 04:12:59 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#ifdef HAVE_LWRES_GETRRSETBYNAME
#include <lwres/netdb.h>
#include <lwres/lwres.h>
#else
#include <netdb.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef DNSSEC_DEBUG
#include <stdio.h>
#include <strings.h>
#endif

#include "netdb_dnssec.h"

/* XXX should it use ci_errno to hold errno instead of h_errno ? */
extern int h_errno;

static struct certinfo *getnewci __P((int, int, int, int, int, char *));

static struct certinfo *
getnewci(qtype, keytag, algorithm, flags, certlen, cert)
	int qtype, keytag, algorithm, flags, certlen;
	char *cert;
{
	struct certinfo *res;

	res = malloc(sizeof(*res));
	if (!res)
		return NULL;

	memset(res, 0, sizeof(*res));
	res->ci_type = qtype;
	res->ci_keytag = keytag;
	res->ci_algorithm = algorithm;
	res->ci_flags = flags;
	res->ci_certlen = certlen;
	res->ci_cert = malloc(certlen);
	if (!res->ci_cert) {
		free(res);
		return NULL;
	}
	memcpy(res->ci_cert, cert, certlen);

	return res;
}

void
freecertinfo(ci)
	struct certinfo *ci;
{
	struct certinfo *next;

	do {
		next = ci->ci_next;
		if (ci->ci_cert)
			free(ci->ci_cert);
		free(ci);
		ci = next;
	} while (ci);
}

/*
 * get CERT RR by FQDN and create certinfo structure chain.
 */
#ifdef HAVE_LWRES_GETRRSETBYNAME
#define getrrsetbyname lwres_getrrsetbyname
#define freerrset lwres_freerrset
#define hstrerror lwres_hstrerror
#endif
#if defined(HAVE_LWRES_GETRRSETBYNAME) || defined(AHVE_GETRRSETBYNAME)
int
getcertsbyname(name, res)
	char *name;
	struct certinfo **res;
{
	int rdlength;
	char *cp;
	int type, keytag, algorithm;
	struct certinfo head, *cur;
	struct rrsetinfo *rr = NULL;
	int i;
	int error = -1;

	/* initialize res */
	*res = NULL;

	memset(&head, 0, sizeof(head));
	cur = &head;

	error = getrrsetbyname(name, C_IN, T_CERT, 0, &rr);
	if (error) {
#ifdef DNSSEC_DEBUG
		printf("getrrsetbyname: %s\n", hstrerror(error));
#endif
		h_errno = NO_RECOVERY;
		goto end;
	}

	if (rr->rri_rdclass != C_IN
	 || rr->rri_rdtype != T_CERT
	 || rr->rri_nrdatas == 0) {
#ifdef DNSSEC_DEBUG
		printf("getrrsetbyname: %s", hstrerror(error));
#endif
		h_errno = NO_RECOVERY;
		goto end;
	}
#ifdef DNSSEC_DEBUG
	if (!(rr->rri_flags & LWRDATA_VALIDATED))
		printf("rr is not valid");
#endif

	for (i = 0; i < rr->rri_nrdatas; i++) {
		rdlength = rr->rri_rdatas[i].rdi_length;
		cp = rr->rri_rdatas[i].rdi_data;

		GETSHORT(type, cp);	/* type */
		rdlength -= INT16SZ;
		GETSHORT(keytag, cp);	/* key tag */
		rdlength -= INT16SZ;
		algorithm = *cp++;	/* algorithm */
		rdlength -= 1;

#ifdef DNSSEC_DEBUG
		printf("type=%d keytag=%d alg=%d len=%d\n",
			type, keytag, algorithm, rdlength);
#endif

		/* create new certinfo */
		cur->ci_next = getnewci(type, keytag, algorithm,
					rr->rri_flags, rdlength, cp);
		if (!cur->ci_next) {
#ifdef DNSSEC_DEBUG
			printf("getnewci: %s", strerror(errno));
#endif
			h_errno = NO_RECOVERY;
			goto end;
		}
		cur = cur->ci_next;
	}

	*res = head.ci_next;
	error = 0;

end:
	if (rr)
		freerrset(rr);
	if (error && head.ci_next)
		freecertinfo(head.ci_next);

	return error;
}
#else	/*!HAVE_LWRES_GETRRSETBYNAME*/
int
getcertsbyname(name, res)
	char *name;
	struct certinfo **res;
{
	caddr_t answer = NULL, p;
	int buflen, anslen, len;
	HEADER *hp;
	int qdcount, ancount, rdlength;
	char *cp, *eom;
	char hostbuf[1024];	/* XXX */
	int qtype, qclass, keytag, algorithm;
	struct certinfo head, *cur;
	int error = -1;

	/* initialize res */
	*res = NULL;

	memset(&head, 0, sizeof(head));
	cur = &head;

	/* get CERT RR */
	buflen = 512;
	do {

		buflen *= 2;
		p = realloc(answer, buflen);
		if (!p) {
#ifdef DNSSEC_DEBUG
			printf("realloc: %s", strerror(errno));
#endif
			h_errno = NO_RECOVERY;
			goto end;
		}
		answer = p;

		anslen = res_query(name,  C_IN, T_CERT, answer, buflen);
		if (anslen == -1)
			goto end;

	} while (buflen < anslen);

#ifdef DNSSEC_DEBUG
	printf("get a DNS packet len=%d\n", anslen);
#endif

	/* parse CERT RR */
	eom = answer + anslen;

	hp = (HEADER *)answer;
	qdcount = ntohs(hp->qdcount);
	ancount = ntohs(hp->ancount);

	/* question section */
	if (qdcount != 1) {
#ifdef DNSSEC_DEBUG
		printf("query count is not 1.\n");
#endif
		h_errno = NO_RECOVERY;
		goto end;
	}
	cp = (char *)(hp + 1);
	len = dn_expand(answer, eom, cp, hostbuf, sizeof(hostbuf));
	if (len < 0) {
#ifdef DNSSEC_DEBUG
		printf("dn_expand failed.\n");
#endif
		goto end;
	}
	cp += len;
	GETSHORT(qtype, cp);		/* QTYPE */
	GETSHORT(qclass, cp);		/* QCLASS */

	/* answer section */
	while (ancount-- && cp < eom) {
		len = dn_expand(answer, eom, cp, hostbuf, sizeof(hostbuf));
		if (len < 0) {
#ifdef DNSSEC_DEBUG
			printf("dn_expand failed.\n");
#endif
			goto end;
		}
		cp += len;
		GETSHORT(qtype, cp);	/* TYPE */
		GETSHORT(qclass, cp);	/* CLASS */
		cp += INT32SZ;		/* TTL */
		GETSHORT(rdlength, cp);	/* RDLENGTH */

		/* CERT RR */
		if (qtype != T_CERT) {
#ifdef DNSSEC_DEBUG
			printf("not T_CERT\n");
#endif
			h_errno = NO_RECOVERY;
			goto end;
		}
		GETSHORT(qtype, cp);	/* type */
		rdlength -= INT16SZ;
		GETSHORT(keytag, cp);	/* key tag */
		rdlength -= INT16SZ;
		algorithm = *cp++;	/* algorithm */
		rdlength -= 1;
		if (cp + rdlength > eom) {
#ifdef DNSSEC_DEBUG
			printf("rdlength is too long.\n");
#endif
			h_errno = NO_RECOVERY;
			goto end;
		}
#ifdef DNSSEC_DEBUG
		printf("type=%d keytag=%d alg=%d len=%d\n",
			qtype, keytag, algorithm, rdlength);
#endif

		/* create new certinfo */
		cur->ci_next = getnewci(qtype, keytag, algorithm,
					0, rdlength, cp);
		if (!cur->ci_next) {
#ifdef DNSSEC_DEBUG
			printf("getnewci: %s", strerror(errno));
#endif
			h_errno = NO_RECOVERY;
			goto end;
		}
		cur = cur->ci_next;

		cp += rdlength;
	}

	*res = head.ci_next;
	error = 0;

end:
	if (answer)
		free(answer);
	if (error && head.ci_next)
		freecertinfo(head.ci_next);

	return error;
}
#endif

#ifdef DNSSEC_DEBUG
int
b64encode(p, len)
	char *p;
	int len;
{
	static const char b64t[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/=";

	while (len > 2) {
                printf("%c", b64t[(p[0] >> 2) & 0x3f]);
                printf("%c", b64t[((p[0] << 4) & 0x30) | ((p[1] >> 4) & 0x0f)]);
                printf("%c", b64t[((p[1] << 2) & 0x3c) | ((p[2] >> 6) & 0x03)]);
                printf("%c", b64t[p[2] & 0x3f]);
		len -= 3;
		p += 3;
	}

	if (len == 2) {
                printf("%c", b64t[(p[0] >> 2) & 0x3f]);
                printf("%c", b64t[((p[0] << 4) & 0x30)| ((p[1] >> 4) & 0x0f)]);
                printf("%c", b64t[((p[1] << 2) & 0x3c)]);
                printf("%c", '=');
        } else if (len == 1) {
                printf("%c", b64t[(p[0] >> 2) & 0x3f]);
                printf("%c", b64t[((p[0] << 4) & 0x30)]);
                printf("%c", '=');
                printf("%c", '=');
	}

	return 0;
}

int
main(ac, av)
	int ac;
	char **av;
{
	struct certinfo *res, *p;
	int i;

	if (ac < 2) {
		printf("Usage: a.out (FQDN)\n");
		exit(1);
	}

	i = getcertsbyname(*(av + 1), &res);
	if (i != 0) {
		herror("getcertsbyname");
		exit(1);
	}
	printf("getcertsbyname succeeded.\n");

	i = 0;
	for (p = res; p; p = p->ci_next) {
		printf("certinfo[%d]:\n", i);
		printf("\tci_type=%d\n", p->ci_type);
		printf("\tci_keytag=%d\n", p->ci_keytag);
		printf("\tci_algorithm=%d\n", p->ci_algorithm);
		printf("\tci_flags=%d\n", p->ci_flags);
		printf("\tci_certlen=%d\n", p->ci_certlen);
		printf("\tci_cert: ");
		b64encode(p->ci_cert, p->ci_certlen);
		printf("\n");
		i++;
	}

	freecertinfo(res);

	exit(0);
}
#endif
