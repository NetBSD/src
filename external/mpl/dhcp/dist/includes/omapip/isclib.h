/*	$NetBSD: isclib.h,v 1.1.1.3 2022/04/03 01:08:45 christos Exp $	*/

/* isclib.h

   connections to the isc and dns libraries */

/*
 * Copyright (C) 2009-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 */

#ifndef ISCLIB_H
#define ISCLIB_H

#include "config.h"

#include <syslog.h>

#define MAXWIRE 256

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <isc/boolean.h>
#include <isc/int.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/lib.h>
#include <isc/app.h>
#include <isc/mem.h>
#include <isc/parseint.h>
#include <isc/socket.h>
#include <isc/sockaddr.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/heap.h>
#include <isc/random.h>

#include <irs/resconf.h>

#include <dns/client.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/lib.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/secalg.h>
#include <dns/tsec.h>

#include <dst/dst.h>

#include "result.h"


/*
 * DHCP context structure
 * This holds the libisc information for a dhcp entity
 */

typedef struct dhcp_context {
	isc_mem_t	*mctx;
	isc_appctx_t	*actx;
	int              actx_started; // ISC_TRUE if ctxstart has been called
	int              actx_running; // ISC_TRUE if ctxrun has been called
	isc_taskmgr_t	*taskmgr;
	isc_task_t	*task;
	isc_socketmgr_t *socketmgr;
	isc_timermgr_t	*timermgr;
#if defined (NSUPDATE)
  	dns_client_t    *dnsclient;
	int use_local4;
	isc_sockaddr_t local4_sockaddr;
	int use_local6;
	isc_sockaddr_t local6_sockaddr;
#endif
} dhcp_context_t;

extern dhcp_context_t dhcp_gbl_ctx;

#define DHCP_MAXDNS_WIRE 256
#define DHCP_MAXNS         3
#define DHCP_HMAC_MD5_NAME "HMAC-MD5.SIG-ALG.REG.INT."
#define DHCP_HMAC_SHA1_NAME "HMAC-SHA1.SIG-ALG.REG.INT."
#define DHCP_HMAC_SHA224_NAME "HMAC-SHA224.SIG-ALG.REG.INT."
#define DHCP_HMAC_SHA256_NAME "HMAC-SHA256.SIG-ALG.REG.INT."
#define DHCP_HMAC_SHA384_NAME "HMAC-SHA384.SIG-ALG.REG.INT."
#define DHCP_HMAC_SHA512_NAME "HMAC-SHA512.SIG-ALG.REG.INT."

isc_result_t dhcp_isc_name(unsigned char    *namestr,
			   dns_fixedname_t  *namefix,
			   dns_name_t      **name);

isc_result_t
isclib_make_dst_key(char          *inname,
		    char          *algorithm,
		    unsigned char *secret,
		    int            length,
		    dst_key_t    **dstkey);

#define DHCP_CONTEXT_PRE_DB  1
#define DHCP_CONTEXT_POST_DB 2
#define DHCP_DNS_CLIENT_LAZY_INIT 4
isc_result_t dhcp_context_create(int              flags,
				 struct in_addr  *local4,
				 struct in6_addr *local6);
void isclib_cleanup(void);

void dhcp_signal_handler(int signal);
extern int shutdown_signal;

#if defined (NSUPDATE)
isc_result_t dns_client_init();
#endif /* defined NSUPDATE */


#endif /* ISCLIB_H */
