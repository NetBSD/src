/*	$NetBSD: isakmp_unity.c,v 1.9 2007/10/19 03:37:19 manu Exp $	*/

/* Id: isakmp_unity.c,v 1.10 2006/07/31 04:49:23 manubsd Exp */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus
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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <resolv.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "isakmp_var.h"
#include "isakmp.h"
#include "handler.h"
#include "isakmp_xauth.h"
#include "isakmp_unity.h"
#include "isakmp_cfg.h"
#include "strnames.h"

static vchar_t *isakmp_cfg_split(struct ph1handle *, 
    struct isakmp_data *, struct unity_netentry*,int);

vchar_t *
isakmp_unity_req(iph1, attr)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
{
	int type;
	vchar_t *reply_attr = NULL;

	if ((iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_UNITY) == 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "Unity mode config request but the peer "
		    "did not declare itself as  unity compliant\n");
		return NULL;
	}

	type = ntohs(attr->type);

	/* Handle short attributes */
	if ((type & ISAKMP_GEN_MASK) == ISAKMP_GEN_TV) {
		type &= ~ISAKMP_GEN_MASK;

		plog(LLV_DEBUG, LOCATION, NULL,
		     "Short attribute %s = %d\n", 
		     s_isakmp_cfg_type(type), ntohs(attr->lorv));

		switch (type) {
		default:
			plog(LLV_DEBUG, LOCATION, NULL,
			     "Ignored short attribute %s\n",
			     s_isakmp_cfg_type(type));
			break;
		}

		return reply_attr;
	}

	switch(type) {
	case UNITY_BANNER: {
#define MAXMOTD 65536
		char buf[MAXMOTD + 1];
		int fd;
		char *filename = &isakmp_cfg_config.motd[0];
		int len;

		if ((fd = open(filename, O_RDONLY, 0)) == -1) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot open \"%s\"\n", filename);
			return NULL;
		}

		if ((len = read(fd, buf, MAXMOTD)) == -1) {
			plog(LLV_ERROR, LOCATION, NULL, 
			    "Cannot read \"%s\"\n", filename);
			close(fd);
			return NULL;
		}
		close(fd);

		buf[len] = '\0';
		reply_attr = isakmp_cfg_string(iph1, attr, buf);

		break;
	}

	case UNITY_PFS:
		reply_attr = isakmp_cfg_short(iph1, attr, 
		    isakmp_cfg_config.pfs_group);
		break;

	case UNITY_SAVE_PASSWD:
		reply_attr = isakmp_cfg_short(iph1, attr, 
		    isakmp_cfg_config.save_passwd);
		break;

	case UNITY_DDNS_HOSTNAME:
		reply_attr = isakmp_cfg_copy(iph1, attr);
		break;

	case UNITY_DEF_DOMAIN:
		reply_attr = isakmp_cfg_string(iph1, 
		    attr, isakmp_cfg_config.default_domain);
		break;

	case UNITY_SPLIT_INCLUDE:
		if(isakmp_cfg_config.splitnet_type == UNITY_SPLIT_INCLUDE)
			reply_attr = isakmp_cfg_split(iph1, attr,
			isakmp_cfg_config.splitnet_list,
			isakmp_cfg_config.splitnet_count);
		else
			return NULL;
		break;
	case UNITY_LOCAL_LAN:
		if(isakmp_cfg_config.splitnet_type == UNITY_LOCAL_LAN)
			reply_attr = isakmp_cfg_split(iph1, attr,
			isakmp_cfg_config.splitnet_list,
			isakmp_cfg_config.splitnet_count);
		else
			return NULL;
		break;
	case UNITY_SPLITDNS_NAME:
		reply_attr = isakmp_cfg_varlen(iph1, attr,
				isakmp_cfg_config.splitdns_list,
				isakmp_cfg_config.splitdns_len);
		break;
	case UNITY_FW_TYPE:
	case UNITY_NATT_PORT:
	case UNITY_BACKUP_SERVERS:
	default:
		plog(LLV_DEBUG, LOCATION, NULL,
		     "Ignored attribute %s\n", s_isakmp_cfg_type(type));
		return NULL;
		break;
	}

	return reply_attr;
}

void
isakmp_unity_reply(iph1, attr)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
{
	int type = ntohs(attr->type);
	int alen = ntohs(attr->lorv);

	struct unity_network *network = (struct unity_network *)(attr + 1);
	int index = 0;
	int count = 0;

	switch(type) {
	case UNITY_SPLIT_INCLUDE:
	{
		if (alen)
			count = alen / sizeof(struct unity_network);

		for(;index < count; index++)
			splitnet_list_add(
				&iph1->mode_cfg->split_include,
				&network[index],
				&iph1->mode_cfg->include_count);

		iph1->mode_cfg->flags |= ISAKMP_CFG_GOT_SPLIT_INCLUDE;
		break;
	}
	case UNITY_LOCAL_LAN:
	{
		if (alen)
			count = alen / sizeof(struct unity_network);

		for(;index < count; index++)
			splitnet_list_add(
				&iph1->mode_cfg->split_local,
				&network[index],
				&iph1->mode_cfg->local_count);

		iph1->mode_cfg->flags |= ISAKMP_CFG_GOT_SPLIT_LOCAL;
		break;
	}
	case UNITY_SPLITDNS_NAME:
	case UNITY_BANNER:
	case UNITY_SAVE_PASSWD:
	case UNITY_NATT_PORT:
	case UNITY_PFS:
	case UNITY_FW_TYPE:
	case UNITY_BACKUP_SERVERS:
	case UNITY_DDNS_HOSTNAME:
	default:
		plog(LLV_WARNING, LOCATION, NULL,
		     "Ignored attribute %s\n",
		     s_isakmp_cfg_type(type));
		break;
	}
	return;
}

static vchar_t *
isakmp_cfg_split(iph1, attr, netentry, count)
	struct ph1handle *iph1;
	struct isakmp_data *attr;
	struct unity_netentry *netentry;
	int count;
{
	vchar_t *buffer;
	struct isakmp_data *new;
	struct unity_network * network;
	size_t len;
	int index = 0;

	char tmp1[40];
	char tmp2[40];

	len = sizeof(struct unity_network) * count;
	if ((buffer = vmalloc(sizeof(*attr) + len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "Cannot allocate memory\n");
		return NULL;
	}

	new = (struct isakmp_data *)buffer->v;
	new->type = attr->type;
	new->lorv = htons(len);

	network = (struct unity_network *)(new + 1);
	for (; index < count; index++) {

		memcpy(&network[index],
			&netentry->network,
			sizeof(struct unity_network));

		inet_ntop(AF_INET, &netentry->network.addr4, tmp1, 40);
		inet_ntop(AF_INET, &netentry->network.mask4, tmp2, 40);
		plog(LLV_DEBUG, LOCATION, NULL, "splitnet: %s/%s\n", tmp1, tmp2);

		netentry = netentry->next;
	}

	return buffer;
}

int  splitnet_list_add(list, network, count)
	struct unity_netentry ** list;
	struct unity_network * network;
	int *count;
{
	struct unity_netentry * nentry;

	/*
	 * search for network in current list
	 * to avoid adding duplicates
	 */
	for (nentry = *list; nentry != NULL; nentry = nentry->next)
		if (memcmp(&nentry->network, network,
			   sizeof(struct unity_network)) == 0)
			return 0;	/* it's a dupe */

	/*
	 * allocate new netentry and copy
	 * new splitnet network data
	 */
	nentry = (struct unity_netentry *)
		racoon_malloc(sizeof(struct unity_netentry));
	if (nentry == NULL)
		return -1;

	memcpy(&nentry->network,network,
		sizeof(struct unity_network));
	nentry->next = NULL;

	/*
	 * locate the last netentry in our
	 * splitnet list and add our entry
	 */
	if (*list == NULL)
		*list = nentry;
	else {
		struct unity_netentry * tmpentry = *list;
		while (tmpentry->next != NULL)
			tmpentry = tmpentry->next;
		tmpentry->next = nentry;
	}

	(*count)++;

	return 0;
}

void splitnet_list_free(list, count)
	struct unity_netentry * list;
	int *count;
{
	struct unity_netentry * netentry = list;
	struct unity_netentry * delentry;

	*count = 0;

	while (netentry != NULL) {
		delentry = netentry;
		netentry = netentry->next;
		racoon_free(delentry);
	}
}

char * splitnet_list_2str(list, splitnet_ipaddr)
	struct unity_netentry * list;
	enum splinet_ipaddr splitnet_ipaddr;
{
	struct unity_netentry * netentry;
	char tmp1[40];
	char tmp2[40];
	char * str;
	int len;

	/* determine string length */
	len = 0;
	netentry = list;
	while (netentry != NULL) {

		inet_ntop(AF_INET, &netentry->network.addr4, tmp1, 40);
		inet_ntop(AF_INET, &netentry->network.mask4, tmp2, 40);
		len += strlen(tmp1);
		len += strlen(tmp2);
		len += 2;

		netentry = netentry->next;
	}

	/* allocate network list string */
	str = racoon_malloc(len);
	if (str == NULL)
		return NULL;

	/* create network list string */
	len = 0;
	netentry = list;
	while (netentry != NULL) {

		inet_ntop(AF_INET, &netentry->network.addr4, tmp1, 40);
		inet_ntop(AF_INET, &netentry->network.mask4, tmp2, 40);
		if (splitnet_ipaddr == CIDR) {
			uint32_t tmp3;
			int cidrmask;

			tmp3 = ntohl(netentry->network.mask4.s_addr);
			for (cidrmask = 0; tmp3 != 0; cidrmask++)
				tmp3 <<= 1;
			len += sprintf(str+len, "%s/%d ", tmp1, cidrmask);
		} else {
			len += sprintf(str+len, "%s/%s ", tmp1, tmp2);
		}

		netentry = netentry->next;
	}

	str[len-1]=0;

	return str;
}
