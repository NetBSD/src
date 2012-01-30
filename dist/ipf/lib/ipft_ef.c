/*	$NetBSD: ipft_ef.c,v 1.4 2012/01/30 16:12:04 darrenr Exp $	*/

/*
 * Copyright (C) 2011 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ipft_ef.c,v 1.21.2.2 2012/01/26 05:29:15 darrenr Exp
 */

/*
                                            icmp type
 lnth proto         source     destination   src port   dst port

etherfind -n

   60  tcp   128.250.20.20  128.250.133.13       2419     telnet

etherfind -n -t

 0.32    91   04    131.170.1.10  128.250.133.13
 0.33   566  udp  128.250.37.155   128.250.133.3        901        901
*/

#include "ipf.h"
#include "ipt.h"
#include <ctype.h>


#if !defined(lint)
static const char sccsid[] = "@(#)ipft_ef.c	1.6 2/4/96 (C)1995 Darren Reed";
static const char rcsid[] = "@(#)Id: ipft_ef.c,v 1.21.2.2 2012/01/26 05:29:15 darrenr Exp";
#endif

static	int	etherf_open __P((char *));
static	int	etherf_close __P((void));
static	int	etherf_readip __P((mb_t *, char **, int *));

struct	ipread	etherf = { etherf_open, etherf_close, etherf_readip, 0 };

static	FILE	*efp = NULL;
static	int	efd = -1;


static	int	etherf_open(fname)
	char	*fname;
{
	if (efd != -1)
		return efd;

	if (!strcmp(fname, "-")) {
		efd = 0;
		efp = stdin;
	} else {
		efd = open(fname, O_RDONLY);
		efp = fdopen(efd, "r");
	}
	return efd;
}


static	int	etherf_close()
{
	return close(efd);
}


static	int	etherf_readip(mb, ifn, dir)
	mb_t	*mb;
	char	**ifn;
	int	*dir;
{
	u_char	pkt[40];
	tcphdr_t *tcp;
	ip_t	*ip;
	char	src[16], dst[16], sprt[16], dprt[16];
	char	lbuf[128], len[8], prot[8], time[8], *s;
	int	slen, extra = 0, i;
	char	*buf;
	int	cnt;

	ip = (ip_t *)pkt;
	tcp = (tcphdr_t *)(ip + 1);
	buf = (char *)mb->mb_buf;
	cnt = sizeof(mb->mb_buf);

	if (!fgets(lbuf, sizeof(lbuf) - 1, efp))
		return 0;

	if ((s = strchr(lbuf, '\n')))
		*s = '\0';
	lbuf[sizeof(lbuf)-1] = '\0';

	bzero(&pkt, sizeof(pkt));

	if (sscanf(lbuf, "%7s %7s %15s %15s %15s %15s", len, prot, src, dst,
		   sprt, dprt) != 6)
		if (sscanf(lbuf, "%7s %7s %7s %15s %15s %15s %15s", time,
			   len, prot, src, dst, sprt, dprt) != 7)
			return -1;

	ip->ip_p = getproto(prot);

	switch (ip->ip_p) {
	case IPPROTO_TCP :
		if (ISDIGIT(*sprt))
			tcp->th_sport = htons(atoi(sprt) & 65535);
		if (ISDIGIT(*dprt))
			tcp->th_dport = htons(atoi(dprt) & 65535);
		extra = sizeof(struct tcphdr);
		break;
	case IPPROTO_UDP :
		if (ISDIGIT(*sprt))
			tcp->th_sport = htons(atoi(sprt) & 65535);
		if (ISDIGIT(*dprt))
			tcp->th_dport = htons(atoi(dprt) & 65535);
		extra = sizeof(struct udphdr);
		break;
#ifdef	IGMP
	case IPPROTO_IGMP :
		extra = sizeof(struct igmp);
		break;
#endif
	case IPPROTO_ICMP :
		extra = sizeof(struct icmp);
		break;
	default :
		break;
	}

	(void) inet_aton(src, &ip->ip_src);
	(void) inet_aton(dst, &ip->ip_dst);
	ip->ip_len = atoi(len);
	IP_HL_A(ip, sizeof(ip_t));

	slen = IP_HL(ip) + extra;
	i = MIN(cnt, slen);
	bcopy((char *)&pkt, buf, i);
	mb->mb_len = i;
	return i;
}
