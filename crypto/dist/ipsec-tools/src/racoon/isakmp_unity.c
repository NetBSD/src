/*	$NetBSD: isakmp_unity.c,v 1.1.1.3 2005/08/07 08:47:23 manu Exp $	*/

/* Id: isakmp_unity.c,v 1.5.4.1 2005/05/10 09:45:46 manubsd Exp */

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
		     "Short attribute %d = %d\n", 
		     type, ntohs(attr->lorv));

		switch (type) {
		default:
			plog(LLV_DEBUG, LOCATION, NULL,
			     "Ignored short attribute %d\n", type);
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
		size_t len;

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
	case UNITY_FW_TYPE:
	case UNITY_SPLITDNS_NAME:
	case UNITY_SPLIT_INCLUDE:
	case UNITY_NATT_PORT:
	case UNITY_BACKUP_SERVERS:
	default:
		plog(LLV_DEBUG, LOCATION, NULL,
		     "Ignored attribute %d\n", type);
		return NULL;
		break;
	}

	return reply_attr;
}


