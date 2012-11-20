/*	$NetBSD: ipf.c,v 1.5.6.1 2012/11/20 02:57:40 tls Exp $	*/

/*
 * Copyright (c) 2004, 2008 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip_compat.h>
#include <netinet/ipl.h>
#include <netinet/ip_fil.h>
#include <netinet/ip_nat.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filter.h"

void ipf_init_filter(char *, char *, int);
int ipf_add_filter(u_int32_t, u_int8_t, struct sockaddr *, struct sockaddr *,
    u_int16_t);
int ipf_add_nat(u_int32_t, struct sockaddr *, struct sockaddr *, u_int16_t,
    struct sockaddr *, u_int16_t, u_int16_t);
int ipf_add_rdr(u_int32_t, struct sockaddr *, struct sockaddr *, u_int16_t,
    struct sockaddr *, u_int16_t);
int ipf_server_lookup(struct sockaddr *, struct sockaddr *, struct sockaddr *);
int ipf_prepare_commit(u_int32_t);
int ipf_do_commit(void);
int ipf_do_rollback(void);

const ftp_proxy_ops_t ipf_fprx_ops = {
	.init_filter	= ipf_init_filter,
	.add_filter	= ipf_add_filter,
	.add_nat	= ipf_add_nat,
	.add_rdr	= ipf_add_rdr,
	.server_lookup	= ipf_server_lookup,
	.prepare_commit	= ipf_prepare_commit,
	.do_commit	= ipf_do_commit,
	.do_rollback	= ipf_do_rollback
};

/* From netinet/in.h, but only _KERNEL_ gets them. */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
#define	satosin6(sa)	((struct sockaddr_in6 *)(sa))

static int natfd;
const char *netif;

struct ftp_proxy_nat {
	struct ipnat	ipn;
	LIST_ENTRY(ftp_proxy_nat) link;
};

struct ftp_proxy_entry {
	u_int32_t	id;
	char		proxy_tag[IPFTAG_LEN];
	int		status;
	LIST_HEAD(, ftp_proxy_nat) nat_entries;
	LIST_ENTRY(ftp_proxy_entry) link;
};

LIST_HEAD(, ftp_proxy_entry) ftp_proxy_entries =
    LIST_HEAD_INITIALIZER(ftp_proxy_entries);

static struct ftp_proxy_entry *
ftp_proxy_entry_create(u_int32_t id)
{
	struct ftp_proxy_entry *fpe;
	int rv;

	fpe = malloc(sizeof(*fpe));
	if (fpe == NULL)
		return (NULL);

	fpe->id = id;
	fpe->status = 0;

	rv = snprintf(fpe->proxy_tag, sizeof(fpe->proxy_tag), "ftp_%d", id);
	if (rv == -1 || rv >= sizeof(fpe->proxy_tag)) {
		free(fpe);
		errno = EINVAL;
		return (NULL);
	}
	LIST_INIT(&fpe->nat_entries);
	LIST_INSERT_HEAD(&ftp_proxy_entries, fpe, link);

	return (fpe);
}

static void 
ftp_proxy_entry_remove(struct ftp_proxy_entry *fpe)
{
	struct ftp_proxy_nat *fpn;

	while ((fpn = LIST_FIRST(&fpe->nat_entries)) != NULL) {
		LIST_REMOVE(fpn, link);
		free(fpn);
	}

	LIST_REMOVE(fpe, link);
	free(fpe);
}

static struct ftp_proxy_entry *
ftp_proxy_entry_find(u_int32_t id)
{
	struct ftp_proxy_entry *fpe;

	LIST_FOREACH(fpe, &ftp_proxy_entries, link) {
		if (fpe->id == id) {
			return fpe;
		}
	}
	return NULL;
}

static int
ftp_proxy_entry_add_nat(struct ftp_proxy_entry *fpe, ipnat_t *ipn)
{
	struct ftp_proxy_nat *fpn;

	fpn = malloc(sizeof(*fpn));
	if (fpn == NULL)
		return (-1);

	memcpy(&fpn->ipn, ipn, sizeof(fpn->ipn));
	LIST_INSERT_HEAD(&fpe->nat_entries, fpn, link);

	return (0);
}

static int
ipfilter_add_nat(ipnat_t *ipn)
{
	ipfobj_t obj;

	memset(&obj, 0, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = ipn->in_size;
	obj.ipfo_type = IPFOBJ_IPNAT;
	obj.ipfo_ptr = ipn;

	return ioctl(natfd, SIOCADNAT, &obj);
}

static int
ipfilter_remove_nat(ipnat_t ipn)
{
	ipfobj_t obj;

	memset(&obj, 0, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(ipn);
	obj.ipfo_type = IPFOBJ_IPNAT;
	obj.ipfo_ptr = &ipn;

	return ioctl(natfd, SIOCRMNAT, &obj);
}

int
ipf_add_filter(u_int32_t id, u_int8_t dir, struct sockaddr *src,
    struct sockaddr *dst, u_int16_t d_port)
{

	if (!src || !dst || !d_port) {
		errno = EINVAL;
		return (-1);
	}

	/* TODO */

	return (0);
}

int
ipf_add_nat(u_int32_t id, struct sockaddr *src, struct sockaddr *dst,
    u_int16_t d_port, struct sockaddr *snat, u_int16_t nat_range_low,
    u_int16_t nat_range_high)
{

	/* TODO */

	return (0);
}

int
ipf_add_rdr(u_int32_t id, struct sockaddr *src, struct sockaddr *dst,
    u_int16_t d_port, struct sockaddr *rdr, u_int16_t rdr_port)
{
	struct ftp_proxy_entry *fpe = ftp_proxy_entry_find(id);
	ipnat_t *ipn;

	if (fpe == NULL) {
		errno = ENOENT;
		return (-1);
	}

	if (!src || !dst || !d_port || !rdr || !rdr_port ||
	    (src->sa_family != rdr->sa_family)) {
		errno = EINVAL;
		return (-1);
	}

	ipn = calloc(1, sizeof(*ipn) + 2 * IF_NAMESIZE + 2);
	if (ipn == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	ipn->in_redir = NAT_REDIRECT;
	ipn->in_v[0] = 4;
	ipn->in_v[1] = 4;
	ipn->in_odstaddr = satosin(dst)->sin_addr.s_addr;
	ipn->in_odstmsk = 0xffffffff;
	ipn->in_odport = htons(d_port);
	ipn->in_dtop = htons(d_port);
	ipn->in_ndstaddr = satosin(rdr)->sin_addr.s_addr;
	ipn->in_ndstmsk  = 0xffffffff;
	ipn->in_dpnext = htons(rdr_port);
	ipn->in_flags = IPN_FIXEDDPORT | IPN_TCP;
	strlcpy(ipn->in_tag.ipt_tag, fpe->proxy_tag,
	    sizeof(ipn->in_tag.ipt_tag));

	ipn->in_ifnames[0] = 0;
	(void) strlcpy(ipn->in_names, netif, IF_NAMESIZE);
	ipn->in_namelen = strlen(ipn->in_names) + 1;
	ipn->in_ifnames[1] = ipn->in_namelen;
	(void) strlcpy(ipn->in_names + ipn->in_namelen, netif, IF_NAMESIZE);
	ipn->in_namelen += strlen(ipn->in_names + ipn->in_ifnames[1]) + 1;
	ipn->in_size = sizeof(*ipn) + ipn->in_namelen;

	if (ipfilter_add_nat(ipn) == -1) {
		free(ipn);
		return (-1);
	}

	if (ftp_proxy_entry_add_nat(fpe, ipn) == -1) {
		free(ipn);
		return (-1);
	}

	fpe->status = 1;
	free(ipn);

	return (0);
}

int
ipf_do_commit(void)
{
	struct ftp_proxy_entry *fpe, *n;
	struct ftp_proxy_nat *fpn;

	for (fpe = LIST_FIRST(&ftp_proxy_entries); fpe != NULL; fpe = n) {
		n = LIST_NEXT(fpe, link);

		/*
		 * If status is nul, then the session is going to be ended.
		 * Remove all nat mappings that were added.
		 */
		if (fpe->status == 0) {
			while ((fpn = LIST_FIRST(&fpe->nat_entries)) != NULL) {
				if (ipfilter_remove_nat(fpn->ipn) == -1)
					return (-1);

				LIST_REMOVE(fpn, link);
				free(fpn);
			}

			ftp_proxy_entry_remove(fpe);
		}
	}

	return (0);
}

int
ipf_do_rollback(void)
{

	/* TODO ??? */

	return (0);
}

void
ipf_init_filter(char *opt_qname, char *opt_tagname, int opt_verbose)
{
	natfd = open(IPNAT_NAME, O_RDWR);
	if (natfd == -1)
		err(EXIT_FAILURE, "cannot open " IPNAT_NAME);
}

int
ipf_prepare_commit(u_int32_t id)
{
	struct ftp_proxy_entry *fpe;

	fpe = ftp_proxy_entry_find(id);
	if (fpe == NULL) {
		fpe = ftp_proxy_entry_create(id);
		if (fpe == NULL)
			return (-1);
	}
	fpe->status = 0;

	return (0);
}

int
ipf_server_lookup(struct sockaddr *client, struct sockaddr *proxy,
    struct sockaddr *server)
{
	natlookup_t natlook;
	ipfobj_t obj;

	/* IPv4-only for now. */
	if (client->sa_family != AF_INET) {
		errno = EPROTONOSUPPORT;
		return (-1);
	}

	/*
	 * Build up the ipf object description structure.
	 */
	memset((void *)&obj, 0, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(natlook);
	obj.ipfo_ptr = &natlook;
	obj.ipfo_type = IPFOBJ_NATLOOKUP;
	/*
	 * Build up the ipf natlook structure.
	 */
	memset((void *)&natlook, 0, sizeof(natlook));
	natlook.nl_flags = IPN_TCPUDP;
	natlook.nl_outip = satosin(client)->sin_addr;
	natlook.nl_inip = satosin(proxy)->sin_addr;
	natlook.nl_outport = satosin(client)->sin_port;
	natlook.nl_inport = satosin(proxy)->sin_port;

	if (ioctl(natfd, SIOCGNATL, &obj) == -1)
		return (-1);

	/*
	 * Return the real destination address and port number in the sockaddr
	 * passed in.
	 */
	memset((void *)server, 0, sizeof(struct sockaddr_in));
	satosin(server)->sin_len = sizeof(struct sockaddr_in);
	satosin(server)->sin_family = AF_INET;
	satosin(server)->sin_addr = natlook.nl_realip;
	satosin(server)->sin_port = natlook.nl_realport;

	return (0);
}
