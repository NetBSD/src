/*	$NetBSD: statschannel.c,v 1.1.1.1 2008/06/21 18:35:14 christos Exp $	*/

/*
 * Copyright (C) 2008  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: statschannel.c,v 1.2.2.9 2008/04/09 22:53:06 tbox Exp */

/*! \file */

#include <config.h>

#include <isc/buffer.h>
#include <isc/httpd.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/task.h>

#include <dns/db.h>
#include <dns/opcode.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/stats.h>
#include <dns/view.h>
#include <dns/zt.h>

#include <named/log.h>
#include <named/server.h>
#include <named/statschannel.h>

#include "bind9.xsl.h"

struct ns_statschannel {
	/* Unlocked */
	isc_httpdmgr_t				*httpdmgr;
	isc_sockaddr_t				address;
	isc_mem_t				*mctx;

	/*
	 * Locked by channel lock: can be refererenced and modified by both
	 * the server task and the channel task.
	 */
	isc_mutex_t				lock;
	dns_acl_t				*acl;

	/* Locked by server task */
	ISC_LINK(struct ns_statschannel)	link;
};

typedef enum { statsformat_file, statsformat_xml } statsformat_t;

typedef struct
stats_dumparg {
	statsformat_t	type;
	void		*arg;		/* type dependent argument */
	const char	**desc;		/* used for general statistics */
	int		ncounters;	/* used for general statistics */
} stats_dumparg_t;

static isc_once_t once = ISC_ONCE_INIT;

static void
generalstat_dump(dns_statscounter_t counter, isc_uint64_t val, void *arg) {
	stats_dumparg_t *dumparg = arg;
	FILE *fp;
#ifdef HAVE_LIBXML2
	xmlTextWriterPtr writer;
#endif

	REQUIRE(counter < dumparg->ncounters);

	switch (dumparg->type) {
	case statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" ISC_PRINT_QUADFORMAT "u %s\n", val,
			dumparg->desc[counter]);
		break;
	case statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		xmlTextWriterStartElement(writer, ISC_XMLCHAR
					  dumparg->desc[counter]);
		xmlTextWriterWriteFormatString(writer,
					       "%" ISC_PRINT_QUADFORMAT "u",
					       val);
		xmlTextWriterEndElement(writer);
#endif
		break;
	}
}

static void
rdtypestat_dump(dns_rdatastatstype_t type, isc_uint64_t val, void *arg) {
	char typebuf[64];
	const char *typestr;
	stats_dumparg_t *dumparg = arg;
	FILE *fp;
#ifdef HAVE_LIBXML2
	xmlTextWriterPtr writer;
#endif

	if ((DNS_RDATASTATSTYPE_ATTR(type) & DNS_RDATASTATSTYPE_ATTR_OTHERTYPE)
	    == 0) {
		dns_rdatatype_format(DNS_RDATASTATSTYPE_BASE(type), typebuf,
				     sizeof(typebuf));
		typestr = typebuf;
	} else
		typestr = "Others";

	switch (dumparg->type) {
	case statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" ISC_PRINT_QUADFORMAT "u %s\n", val, typestr);
		break;
	case statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "rdtype");

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "name");
		xmlTextWriterWriteString(writer, ISC_XMLCHAR typestr);
		xmlTextWriterEndElement(writer); /* name */

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter");
		xmlTextWriterWriteFormatString(writer,
					       "%" ISC_PRINT_QUADFORMAT "u",
					       val);
		xmlTextWriterEndElement(writer); /* counter */

		xmlTextWriterEndElement(writer); /* rdtype */
#endif
		break;
	}
}

static void
rdatasetstats_dump(dns_rdatastatstype_t type, isc_uint64_t val, void *arg) {
	stats_dumparg_t *dumparg = arg;
	FILE *fp;
	char typebuf[64];
	const char *typestr;
	isc_boolean_t nxrrset = ISC_FALSE;
#ifdef HAVE_LIBXML2
	xmlTextWriterPtr writer;
#endif

	if ((DNS_RDATASTATSTYPE_ATTR(type) & DNS_RDATASTATSTYPE_ATTR_NXDOMAIN)
	    != 0) {
		typestr = "NXDOMAIN";
	} else if ((DNS_RDATASTATSTYPE_ATTR(type) &
		    DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) != 0) {
		typestr = "Others";
	} else {
		dns_rdatatype_format(DNS_RDATASTATSTYPE_BASE(type), typebuf,
				     sizeof(typebuf));
		typestr = typebuf;
	}

	if ((DNS_RDATASTATSTYPE_ATTR(type) & DNS_RDATASTATSTYPE_ATTR_NXRRSET)
	    != 0)
		nxrrset = ISC_TRUE;

	switch (dumparg->type) {
	case statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" ISC_PRINT_QUADFORMAT "u %s%s\n", val,
			nxrrset ? "!" : "", typestr);
		break;
	case statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "rrset");
		xmlTextWriterStartElement(writer, ISC_XMLCHAR "name");
		xmlTextWriterWriteFormatString(writer, "%s%s",
					       nxrrset ? "!" : "", typestr);
		xmlTextWriterEndElement(writer); /* name */

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter");
		xmlTextWriterWriteFormatString(writer,
					       "%" ISC_PRINT_QUADFORMAT "u",
					       val);
		xmlTextWriterEndElement(writer); /* counter */

		xmlTextWriterEndElement(writer); /* rrset */
#endif
		break;
	}
}

static void
opcodestat_dump(dns_opcode_t code, isc_uint64_t val, void *arg) {
	FILE *fp = arg;
	isc_buffer_t b;
	char codebuf[64];
	stats_dumparg_t *dumparg = arg;
#ifdef HAVE_LIBXML2
	xmlTextWriterPtr writer;
#endif

	isc_buffer_init(&b, codebuf, sizeof(codebuf) - 1);
	dns_opcode_totext(code, &b);
	codebuf[isc_buffer_usedlength(&b)] = '\0';

	switch (dumparg->type) {
	case statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" ISC_PRINT_QUADFORMAT "u %s\n", val, codebuf);
		break;
	case statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "opcode");

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "name");
		xmlTextWriterWriteString(writer, ISC_XMLCHAR codebuf);
		xmlTextWriterEndElement(writer); /* name */

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter");
		xmlTextWriterWriteFormatString(writer,
					       "%" ISC_PRINT_QUADFORMAT "u",
					       val);
		xmlTextWriterEndElement(writer); /* counter */

		xmlTextWriterEndElement(writer); /* opcode */
#endif
		break;
	}
}

#ifdef HAVE_LIBXML2

/* XXXMLG below here sucks. */

#define TRY(a) do { result = (a); INSIST(result == ISC_R_SUCCESS); } while(0);
#define TRY0(a) do { xmlrc = (a); INSIST(xmlrc >= 0); } while(0);

static isc_result_t
zone_xmlrender(dns_zone_t *zone, void *arg) {
	char buf[1024 + 32];	/* sufficiently large for zone name and class */
	dns_rdataclass_t rdclass;
	isc_uint32_t serial;
	xmlTextWriterPtr writer = arg;
	stats_dumparg_t dumparg;
	dns_stats_t *zonestats;

	xmlTextWriterStartElement(writer, ISC_XMLCHAR "zone");

	dns_zone_name(zone, buf, sizeof(buf));
	xmlTextWriterStartElement(writer, ISC_XMLCHAR "name");
	xmlTextWriterWriteString(writer, ISC_XMLCHAR buf);
	xmlTextWriterEndElement(writer);

	rdclass = dns_zone_getclass(zone);
	dns_rdataclass_format(rdclass, buf, sizeof(buf));
	xmlTextWriterStartElement(writer, ISC_XMLCHAR "rdataclass");
	xmlTextWriterWriteString(writer, ISC_XMLCHAR buf);
	xmlTextWriterEndElement(writer);

	serial = dns_zone_getserial(zone);
	xmlTextWriterStartElement(writer, ISC_XMLCHAR "serial");
	xmlTextWriterWriteFormatString(writer, "%u", serial);
	xmlTextWriterEndElement(writer);

	dumparg.type = statsformat_xml;
	dumparg.arg = writer;
	dumparg.desc = nsstats_xmldesc;
	dumparg.ncounters = dns_nsstatscounter_max;

	zonestats = dns_zone_getrequeststats(zone);
	if (zonestats != NULL) {
		xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters");
		dns_generalstats_dump(zonestats, generalstat_dump,
				      &dumparg, DNS_STATSDUMP_VERBOSE);
		xmlTextWriterEndElement(writer); /* counters */
	}

	xmlTextWriterEndElement(writer); /* zone */

	return (ISC_R_SUCCESS);
}

static void
generatexml(ns_server_t *server, int *buflen, xmlChar **buf) {
	char boottime[sizeof "yyyy-mm-ddThh:mm:ssZ"];
	char nowstr[sizeof "yyyy-mm-ddThh:mm:ssZ"];
	isc_time_t now;
	xmlTextWriterPtr writer;
	xmlDocPtr doc;
	int xmlrc;
	dns_view_t *view;
	stats_dumparg_t dumparg;
	dns_stats_t *cachestats;

	isc_time_now(&now);
	isc_time_formatISO8601(&ns_g_boottime, boottime, sizeof boottime);
	isc_time_formatISO8601(&now, nowstr, sizeof nowstr);

	writer = xmlNewTextWriterDoc(&doc, 0);
	TRY0(xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL));
	TRY0(xmlTextWriterWritePI(writer, ISC_XMLCHAR "xml-stylesheet",
			ISC_XMLCHAR "type=\"text/xsl\" href=\"/bind9.xsl\""));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "isc"));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "version",
					 ISC_XMLCHAR "1.0"));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "bind"));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "statistics"));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "version",
					 ISC_XMLCHAR "1.0"));

	/* Set common fields for statistics dump */
	dumparg.type = statsformat_xml;
	dumparg.arg = writer;

	/*
	 * Start by rendering the views we know of here.  For each view we
	 * know of, call its rendering function.
	 */
	view = ISC_LIST_HEAD(server->viewlist);
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "views"));
	while (view != NULL) {
		xmlTextWriterStartElement(writer, ISC_XMLCHAR "view");

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "name");
		xmlTextWriterWriteString(writer, ISC_XMLCHAR view->name);
		xmlTextWriterEndElement(writer);

		xmlTextWriterStartElement(writer, ISC_XMLCHAR "zones");
		dns_zt_apply(view->zonetable, ISC_FALSE, zone_xmlrender,
			     writer);
		xmlTextWriterEndElement(writer);

		if (view->resquerystats != NULL) {
			dns_rdatatypestats_dump(view->resquerystats,
						rdtypestat_dump, &dumparg, 0);
		}

		if (view->resstats != NULL) {
			xmlTextWriterStartElement(writer,
						  ISC_XMLCHAR "resstats");
			dumparg.ncounters = dns_resstatscounter_max;
			dumparg.desc = resstats_xmldesc; /* auto-generated */
			dns_generalstats_dump(view->resstats, generalstat_dump,
					      &dumparg, DNS_STATSDUMP_VERBOSE);
			xmlTextWriterEndElement(writer); /* resstats */
		}

		cachestats = dns_db_getrrsetstats(view->cachedb);
		if (cachestats != NULL) {
			xmlTextWriterStartElement(writer,
						  ISC_XMLCHAR "cache");
			dns_rdatasetstats_dump(cachestats, rdatasetstats_dump,
					       &dumparg, 0);
			xmlTextWriterEndElement(writer); /* cache */
		}

		xmlTextWriterEndElement(writer); /* view */

		view = ISC_LIST_NEXT(view, link);
	}
	TRY0(xmlTextWriterEndElement(writer)); /* views */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "socketmgr"));
	isc_socketmgr_renderxml(ns_g_socketmgr, writer);
	TRY0(xmlTextWriterEndElement(writer)); /* socketmgr */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "taskmgr"));
	isc_taskmgr_renderxml(ns_g_taskmgr, writer);
	TRY0(xmlTextWriterEndElement(writer)); /* taskmgr */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "server"));
	xmlTextWriterStartElement(writer, ISC_XMLCHAR "boot-time");
	xmlTextWriterWriteString(writer, ISC_XMLCHAR boottime);
	xmlTextWriterEndElement(writer);
	xmlTextWriterStartElement(writer, ISC_XMLCHAR "current-time");
	xmlTextWriterWriteString(writer, ISC_XMLCHAR nowstr);
	xmlTextWriterEndElement(writer);

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "requests"));
	dns_opcodestats_dump(server->opcodestats, opcodestat_dump, &dumparg,
			     0);
	xmlTextWriterEndElement(writer); /* requests */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "queries-in"));
	dns_rdatatypestats_dump(server->rcvquerystats, rdtypestat_dump,
				&dumparg, 0);
	xmlTextWriterEndElement(writer); /* queries-in */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "nsstats"));
	dumparg.desc = nsstats_xmldesc; /* auto-generated in bind9.xsl.h */
	dumparg.ncounters = dns_nsstatscounter_max;
	dns_generalstats_dump(server->nsstats, generalstat_dump, &dumparg,
			      DNS_STATSDUMP_VERBOSE);
	xmlTextWriterEndElement(writer); /* nsstats */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "zonestats"));
	dumparg.desc = zonestats_xmldesc; /* auto-generated in bind9.xsl.h */
	dumparg.ncounters = dns_zonestatscounter_max;
	dns_generalstats_dump(server->zonestats, generalstat_dump, &dumparg,
			      DNS_STATSDUMP_VERBOSE);
	xmlTextWriterEndElement(writer); /* zonestats */

	xmlTextWriterStartElement(writer, ISC_XMLCHAR "resstats");
	dumparg.ncounters = dns_resstatscounter_max;
	dumparg.desc = resstats_xmldesc;
	dns_generalstats_dump(server->resolverstats, generalstat_dump,
			      &dumparg, DNS_STATSDUMP_VERBOSE);
	xmlTextWriterEndElement(writer); /* resstats */

	xmlTextWriterEndElement(writer); /* server */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "memory"));
	isc_mem_renderxml(writer);
	TRY0(xmlTextWriterEndElement(writer)); /* memory */

	TRY0(xmlTextWriterEndElement(writer)); /* statistics */
	TRY0(xmlTextWriterEndElement(writer)); /* bind */
	TRY0(xmlTextWriterEndElement(writer)); /* isc */

	TRY0(xmlTextWriterEndDocument(writer));

	xmlFreeTextWriter(writer);

	xmlDocDumpFormatMemoryEnc(doc, buf, buflen, "UTF-8", 1);
	xmlFreeDoc(doc);
}

static void
wrap_xmlfree(isc_buffer_t *buffer, void *arg) {
	UNUSED(arg);

	xmlFree(isc_buffer_base(buffer));
}

static isc_result_t
render_index(const char *url, const char *querystring, void *arg,
	     unsigned int *retcode, const char **retmsg, const char **mimetype,
	     isc_buffer_t *b, isc_httpdfree_t **freecb,
	     void **freecb_args)
{
	unsigned char *msg;
	int msglen;
	ns_server_t *server = arg;

	UNUSED(url);
	UNUSED(querystring);

	generatexml(server, &msglen, &msg);

	*retcode = 200;
	*retmsg = "OK";
	*mimetype = "text/xml";
	isc_buffer_reinit(b, msg, msglen);
	isc_buffer_add(b, msglen);
	*freecb = wrap_xmlfree;
	*freecb_args = NULL;

	return (ISC_R_SUCCESS);
}

#endif	/* HAVE_LIBXML2 */

static isc_result_t
render_xsl(const char *url, const char *querystring, void *args,
	   unsigned int *retcode, const char **retmsg, const char **mimetype,
	   isc_buffer_t *b, isc_httpdfree_t **freecb,
	   void **freecb_args)
{
	UNUSED(url);
	UNUSED(querystring);
	UNUSED(args);

	*retcode = 200;
	*retmsg = "OK";
	*mimetype = "text/xslt+xml";
	isc_buffer_reinit(b, xslmsg, strlen(xslmsg));
	isc_buffer_add(b, strlen(xslmsg));
	*freecb = NULL;
	*freecb_args = NULL;

	return (ISC_R_SUCCESS);
}

static void
shutdown_listener(ns_statschannel_t *listener) {
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_format(&listener->address, socktext, sizeof(socktext));
	isc_log_write(ns_g_lctx, NS_LOGCATEGORY_GENERAL,NS_LOGMODULE_SERVER,
		      ISC_LOG_NOTICE, "stopping statistics channel on %s",
		      socktext);

	isc_httpdmgr_shutdown(&listener->httpdmgr);
}

static isc_boolean_t
client_ok(const isc_sockaddr_t *fromaddr, void *arg) {
	ns_statschannel_t *listener = arg;
	isc_netaddr_t netaddr;
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	int match;

	REQUIRE(listener != NULL);

	isc_netaddr_fromsockaddr(&netaddr, fromaddr);

	LOCK(&listener->lock);
	if (dns_acl_match(&netaddr, NULL, listener->acl, &ns_g_server->aclenv,
			  &match, NULL) == ISC_R_SUCCESS && match > 0) {
		UNLOCK(&listener->lock);
		return (ISC_TRUE);
	}
	UNLOCK(&listener->lock);

	isc_sockaddr_format(fromaddr, socktext, sizeof(socktext));
	isc_log_write(ns_g_lctx, NS_LOGCATEGORY_GENERAL,
		      NS_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "rejected statistics connection from %s", socktext);

	return (ISC_FALSE);
}

static void
destroy_listener(void *arg) {
	ns_statschannel_t *listener = arg;

	REQUIRE(listener != NULL);
	REQUIRE(!ISC_LINK_LINKED(listener, link));

	/* We don't have to acquire the lock here since it's already unlinked */
	dns_acl_detach(&listener->acl);

	DESTROYLOCK(&listener->lock);
	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));
}

static isc_result_t
add_listener(ns_server_t *server, ns_statschannel_t **listenerp,
	     const cfg_obj_t *listen_params, const cfg_obj_t *config,
	     isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
	     const char *socktext)
{
	isc_result_t result;
	ns_statschannel_t *listener;
	isc_task_t *task = NULL;
	isc_socket_t *sock = NULL;
	const cfg_obj_t *allow;
	dns_acl_t *new_acl = NULL;

	listener = isc_mem_get(server->mctx, sizeof(*listener));
	if (listener == NULL)
		return (ISC_R_NOMEMORY);

	listener->httpdmgr = NULL;
	listener->address = *addr;
	listener->acl = NULL;
	listener->mctx = NULL;
	ISC_LINK_INIT(listener, link);

	result = isc_mutex_init(&listener->lock);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(server->mctx, listener, sizeof(*listener));
		return (ISC_R_FAILURE);
	}

	isc_mem_attach(server->mctx, &listener->mctx);

	allow = cfg_tuple_get(listen_params, "allow");
	if (allow != NULL && cfg_obj_islist(allow)) {
		result = cfg_acl_fromconfig(allow, config, ns_g_lctx,
					    aclconfctx, listener->mctx, 0,
					    &new_acl);
	} else
		result = dns_acl_any(listener->mctx, &new_acl);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	dns_acl_attach(new_acl, &listener->acl);
	dns_acl_detach(&new_acl);

	result = isc_task_create(ns_g_taskmgr, 0, &task);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	isc_task_setname(task, "statchannel", NULL);

	result = isc_socket_create(ns_g_socketmgr, isc_sockaddr_pf(addr),
				   isc_sockettype_tcp, &sock);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	isc_socket_setname(sock, "statchannel", NULL);

#ifndef ISC_ALLOW_MAPPED
	isc_socket_ipv6only(sock, ISC_TRUE);
#endif

	result = isc_socket_bind(sock, addr);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = isc_httpdmgr_create(server->mctx, sock, task, client_ok,
				     destroy_listener, listener, ns_g_timermgr,
				     &listener->httpdmgr);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

#ifdef HAVE_LIBXML2
	isc_httpdmgr_addurl(listener->httpdmgr, "/", render_index, server);
#endif
	isc_httpdmgr_addurl(listener->httpdmgr, "/bind9.xsl", render_xsl,
			    server);

	*listenerp = listener;
	isc_log_write(ns_g_lctx, NS_LOGCATEGORY_GENERAL,
		      NS_LOGMODULE_SERVER, ISC_LOG_NOTICE,
		      "statistics channel listening on %s", socktext);

cleanup:
	if (result != ISC_R_SUCCESS) {
		if (listener->acl != NULL)
			dns_acl_detach(&listener->acl);
		DESTROYLOCK(&listener->lock);
		isc_mem_putanddetach(&listener->mctx, listener,
				     sizeof(*listener));
	}
	if (task != NULL)
		isc_task_detach(&task);
	if (sock != NULL)
		isc_socket_detach(&sock);

	return (result);
}

static void
update_listener(ns_server_t *server, ns_statschannel_t **listenerp,
		const cfg_obj_t *listen_params, const cfg_obj_t *config,
		isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
		const char *socktext)
{
	ns_statschannel_t *listener;
	const cfg_obj_t *allow = NULL;
	dns_acl_t *new_acl = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	for (listener = ISC_LIST_HEAD(server->statschannels);
	     listener != NULL;
	     listener = ISC_LIST_NEXT(listener, link))
		if (isc_sockaddr_equal(addr, &listener->address))
			break;

	if (listener == NULL) {
		*listenerp = NULL;
		return;
	}

	/*
	 * Now, keep the old access list unless a new one can be made.
	 */
	allow = cfg_tuple_get(listen_params, "allow");
	if (allow != NULL && cfg_obj_islist(allow)) {
		result = cfg_acl_fromconfig(allow, config, ns_g_lctx,
					    aclconfctx, listener->mctx, 0,
					    &new_acl);
	} else
		result = dns_acl_any(listener->mctx, &new_acl);

	if (result == ISC_R_SUCCESS) {
		LOCK(&listener->lock);

		dns_acl_detach(&listener->acl);
		dns_acl_attach(new_acl, &listener->acl);
		dns_acl_detach(&new_acl);

		UNLOCK(&listener->lock);
	} else {
		cfg_obj_log(listen_params, ns_g_lctx, ISC_LOG_WARNING,
			    "couldn't install new acl for "
			    "statistics channel %s: %s",
			    socktext, isc_result_totext(result));
	}

	*listenerp = listener;
}

isc_result_t
ns_statschannels_configure(ns_server_t *server, const cfg_obj_t *config,
			 cfg_aclconfctx_t *aclconfctx)
{
	ns_statschannel_t *listener, *listener_next;
	ns_statschannellist_t new_listeners;
	const cfg_obj_t *statschannellist = NULL;
	const cfg_listelt_t *element, *element2;
	char socktext[ISC_SOCKADDR_FORMATSIZE];

	ISC_LIST_INIT(new_listeners);

	/*
	 * Get the list of named.conf 'statistics-channels' statements.
	 */
	(void)cfg_map_get(config, "statistics-channels", &statschannellist);

	/*
	 * Run through the new address/port list, noting sockets that are
	 * already being listened on and moving them to the new list.
	 *
	 * Identifying duplicate addr/port combinations is left to either
	 * the underlying config code, or to the bind attempt getting an
	 * address-in-use error.
	 */
	if (statschannellist != NULL) {
#ifndef HAVE_LIBXML2
		isc_log_write(ns_g_lctx, NS_LOGCATEGORY_GENERAL,
			      NS_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "statistics-channels specified but not effective "
			      "due to missing XML library");
#endif

		for (element = cfg_list_first(statschannellist);
		     element != NULL;
		     element = cfg_list_next(element)) {
			const cfg_obj_t *statschannel;
			const cfg_obj_t *listenercfg = NULL;

			statschannel = cfg_listelt_value(element);
			(void)cfg_map_get(statschannel, "inet",
					  &listenercfg);
			if (listenercfg == NULL)
				continue;

			for (element2 = cfg_list_first(listenercfg);
			     element2 != NULL;
			     element2 = cfg_list_next(element2)) {
				const cfg_obj_t *listen_params;
				const cfg_obj_t *obj;
				isc_sockaddr_t addr;

				listen_params = cfg_listelt_value(element2);

				obj = cfg_tuple_get(listen_params, "address");
				addr = *cfg_obj_assockaddr(obj);
				if (isc_sockaddr_getport(&addr) == 0)
					isc_sockaddr_setport(&addr, NS_STATSCHANNEL_HTTPPORT);

				isc_sockaddr_format(&addr, socktext,
						    sizeof(socktext));

				isc_log_write(ns_g_lctx,
					      NS_LOGCATEGORY_GENERAL,
					      NS_LOGMODULE_SERVER,
					      ISC_LOG_DEBUG(9),
					      "processing statistics "
					      "channel %s",
					      socktext);

				update_listener(server, &listener,
						listen_params, config, &addr,
						aclconfctx, socktext);

				if (listener != NULL) {
					/*
					 * Remove the listener from the old
					 * list, so it won't be shut down.
					 */
					ISC_LIST_UNLINK(server->statschannels,
							listener, link);
				} else {
					/*
					 * This is a new listener.
					 */
					isc_result_t r;

					r = add_listener(server, &listener,
							 listen_params, config,
							 &addr, aclconfctx,
							 socktext);
					if (r != ISC_R_SUCCESS) {
						cfg_obj_log(listen_params,
							    ns_g_lctx,
							    ISC_LOG_WARNING,
							    "couldn't allocate "
							    "statistics channel"
							    " %s: %s",
							    socktext,
							    isc_result_totext(r));
					}
				}

				if (listener != NULL)
					ISC_LIST_APPEND(new_listeners, listener,
							link);
			}
		}
	}

	for (listener = ISC_LIST_HEAD(server->statschannels);
	     listener != NULL;
	     listener = listener_next) {
		listener_next = ISC_LIST_NEXT(listener, link);
		ISC_LIST_UNLINK(server->statschannels, listener, link);
		shutdown_listener(listener);
	}

	ISC_LIST_APPENDLIST(server->statschannels, new_listeners, link);
	return (ISC_R_SUCCESS);
}

void
ns_statschannels_shutdown(ns_server_t *server) {
	ns_statschannel_t *listener;

	while ((listener = ISC_LIST_HEAD(server->statschannels)) != NULL) {
		ISC_LIST_UNLINK(server->statschannels, listener, link);
		shutdown_listener(listener);
	}
}

/*%
 * Statistics descriptions.  These could be statistically initialized at
 * compile time, but we configure them run time in the init_desc() function
 * below so that they'll be less susceptible to counter name changes.
 * Note that bind9.xsl must still be updated consistently with the counter
 * numbering.
 */
static const char *nsstats_desc[dns_nsstatscounter_max];
static const char *resstats_desc[dns_resstatscounter_max];
static const char *zonestats_desc[dns_zonestatscounter_max];

static inline void
set_desc(int counter, int maxcounter, const char *desc, const char **descs) {
	REQUIRE(counter < maxcounter);
	REQUIRE(descs[counter] == NULL);

	descs[counter] = desc;
}

static void
init_desc() {
	int i;

	/* Initialize name server statistics */
	memset(nsstats_desc, 0,
	       dns_nsstatscounter_max * sizeof(nsstats_desc[0]));
	set_desc(dns_nsstatscounter_requestv4, dns_nsstatscounter_max,
		 "IPv4 requests received", nsstats_desc);
	set_desc(dns_nsstatscounter_requestv6, dns_nsstatscounter_max,
		 "IPv6 requests received", nsstats_desc);
	set_desc(dns_nsstatscounter_edns0in, dns_nsstatscounter_max,
		 "requests with EDNS(0) received", nsstats_desc);
	set_desc(dns_nsstatscounter_badednsver, dns_nsstatscounter_max,
		 "requests with unsupported EDNS version received",
		 nsstats_desc);
	set_desc(dns_nsstatscounter_tsigin, dns_nsstatscounter_max,
		 "requests with TSIG received", nsstats_desc);
	set_desc(dns_nsstatscounter_sig0in, dns_nsstatscounter_max,
		 "requests with SIG(0) received", nsstats_desc);
	set_desc(dns_nsstatscounter_invalidsig, dns_nsstatscounter_max,
		 "requests with invalid signature", nsstats_desc);
	set_desc(dns_nsstatscounter_tcp, dns_nsstatscounter_max,
		 "TCP requests received", nsstats_desc);
	set_desc(dns_nsstatscounter_authrej, dns_nsstatscounter_max,
		 "auth queries rejected", nsstats_desc);
	set_desc(dns_nsstatscounter_recurserej, dns_nsstatscounter_max,
		 "recursive queries rejected", nsstats_desc);
	set_desc(dns_nsstatscounter_xfrrej, dns_nsstatscounter_max,
		 "transfer requests rejected", nsstats_desc);
	set_desc(dns_nsstatscounter_updaterej, dns_nsstatscounter_max,
		 "update requests rejected", nsstats_desc);
	set_desc(dns_nsstatscounter_response, dns_nsstatscounter_max,
		 "responses sent", nsstats_desc);
	set_desc(dns_nsstatscounter_truncatedresp, dns_nsstatscounter_max,
		 "truncated responses sent", nsstats_desc);
	set_desc(dns_nsstatscounter_edns0out, dns_nsstatscounter_max,
		 "responses with EDNS(0) sent", nsstats_desc);
	set_desc(dns_nsstatscounter_tsigout, dns_nsstatscounter_max,
		 "responses with TSIG sent", nsstats_desc);
	set_desc(dns_nsstatscounter_sig0out, dns_nsstatscounter_max,
		 "responses with SIG(0) sent", nsstats_desc);
	set_desc(dns_nsstatscounter_success, dns_nsstatscounter_max,
		 "queries resulted in successful answer", nsstats_desc);
	set_desc(dns_nsstatscounter_authans, dns_nsstatscounter_max,
		 "queries resulted in authoritative answer", nsstats_desc);
	set_desc(dns_nsstatscounter_nonauthans, dns_nsstatscounter_max,
		 "queries resulted in non authoritative answer", nsstats_desc);
	set_desc(dns_nsstatscounter_referral, dns_nsstatscounter_max,
		 "queries resulted in referral answer", nsstats_desc);
	set_desc(dns_nsstatscounter_nxrrset, dns_nsstatscounter_max,
		 "queries resulted in nxrrset", nsstats_desc);
	set_desc(dns_nsstatscounter_servfail, dns_nsstatscounter_max,
		 "queries resulted in SERVFAIL", nsstats_desc);
	set_desc(dns_nsstatscounter_formerr, dns_nsstatscounter_max,
		 "queries resulted in FORMERR", nsstats_desc);
	set_desc(dns_nsstatscounter_nxdomain, dns_nsstatscounter_max,
		 "queries resulted in NXDOMAIN", nsstats_desc);
	set_desc(dns_nsstatscounter_recursion, dns_nsstatscounter_max,
		 "queries caused recursion", nsstats_desc);
	set_desc(dns_nsstatscounter_duplicate, dns_nsstatscounter_max,
		 "duplicate queries received", nsstats_desc);
	set_desc(dns_nsstatscounter_dropped, dns_nsstatscounter_max,
		 "queries dropped", nsstats_desc);
	set_desc(dns_nsstatscounter_failure, dns_nsstatscounter_max,
		 "other query failures", nsstats_desc);
	set_desc(dns_nsstatscounter_xfrdone, dns_nsstatscounter_max,
		 "requested transfers completed", nsstats_desc);
	set_desc(dns_nsstatscounter_updatereqfwd, dns_nsstatscounter_max,
		 "update requests forwarded", nsstats_desc);
	set_desc(dns_nsstatscounter_updaterespfwd, dns_nsstatscounter_max,
		 "update responses forwarded", nsstats_desc);
	set_desc(dns_nsstatscounter_updatefwdfail, dns_nsstatscounter_max,
		 "update forward failed", nsstats_desc);
	set_desc(dns_nsstatscounter_updatedone, dns_nsstatscounter_max,
		 "updates completed", nsstats_desc);
	set_desc(dns_nsstatscounter_updatefail, dns_nsstatscounter_max,
		 "updates failed", nsstats_desc);
	set_desc(dns_nsstatscounter_updatebadprereq, dns_nsstatscounter_max,
		 "updates rejected due to prerequisite failure", nsstats_desc);

	/* Initialize resolver statistics */
	memset(resstats_desc, 0,
	       dns_resstatscounter_max * sizeof(resstats_desc[0]));
	set_desc(dns_resstatscounter_queryv4, dns_resstatscounter_max,
		 "IPv4 queries sent", resstats_desc);
	set_desc(dns_resstatscounter_queryv6, dns_resstatscounter_max,
		 "IPv6 queries sent", resstats_desc);
	set_desc(dns_resstatscounter_responsev4, dns_resstatscounter_max,
		 "IPv4 responses received", resstats_desc);
	set_desc(dns_resstatscounter_responsev6, dns_resstatscounter_max,
		 "IPv6 responses received", resstats_desc);
	set_desc(dns_resstatscounter_nxdomain, dns_resstatscounter_max,
		 "NXDOMAIN received", resstats_desc);
	set_desc(dns_resstatscounter_servfail, dns_resstatscounter_max,
		 "SERVFAIL received", resstats_desc);
	set_desc(dns_resstatscounter_formerr, dns_resstatscounter_max,
		 "FORMERR received", resstats_desc);
	set_desc(dns_resstatscounter_othererror, dns_resstatscounter_max,
		 "other errors received", resstats_desc);
	set_desc(dns_resstatscounter_edns0fail, dns_resstatscounter_max,
		 "EDNS(0) query failures", resstats_desc);
	set_desc(dns_resstatscounter_mismatch, dns_resstatscounter_max,
		 "mismatch responses received", resstats_desc);
	set_desc(dns_resstatscounter_truncated, dns_resstatscounter_max,
		 "truncated responses received", resstats_desc);
	set_desc(dns_resstatscounter_lame, dns_resstatscounter_max,
		 "lame delegations received", resstats_desc);
	set_desc(dns_resstatscounter_retry, dns_resstatscounter_max,
		 "query retries", resstats_desc);
	set_desc(dns_resstatscounter_gluefetchv4, dns_resstatscounter_max,
		 "IPv4 NS address fetches", resstats_desc);
	set_desc(dns_resstatscounter_gluefetchv6, dns_resstatscounter_max,
		 "IPv6 NS address fetches", resstats_desc);
	set_desc(dns_resstatscounter_gluefetchv4fail, dns_resstatscounter_max,
		 "IPv4 NS address fetch failed", resstats_desc);
	set_desc(dns_resstatscounter_gluefetchv6fail, dns_resstatscounter_max,
		 "IPv6 NS address fetch failed", resstats_desc);
	set_desc(dns_resstatscounter_val, dns_resstatscounter_max,
		 "DNSSEC validation attempted", resstats_desc);
	set_desc(dns_resstatscounter_valsuccess, dns_resstatscounter_max,
		 "DNSSEC validation succeeded", resstats_desc);
	set_desc(dns_resstatscounter_valnegsuccess, dns_resstatscounter_max,
		 "DNSSEC NX validation succeeded", resstats_desc);
	set_desc(dns_resstatscounter_valfail, dns_resstatscounter_max,
		 "DNSSEC validation failed", resstats_desc);

	/* Initialize zone statistics */
	memset(zonestats_desc, 0,
	       dns_zonestatscounter_max * sizeof(zonestats_desc[0]));
	set_desc(dns_zonestatscounter_notifyoutv4, dns_zonestatscounter_max,
		 "IPv4 notifies sent", zonestats_desc);
	set_desc(dns_zonestatscounter_notifyoutv6, dns_zonestatscounter_max,
		 "IPv6 notifies sent", zonestats_desc);
	set_desc(dns_zonestatscounter_notifyinv4, dns_zonestatscounter_max,
		 "IPv4 notifies received", zonestats_desc);
	set_desc(dns_zonestatscounter_notifyinv6, dns_zonestatscounter_max,
		 "IPv6 notifies received", zonestats_desc);
	set_desc(dns_zonestatscounter_notifyrej, dns_zonestatscounter_max,
		 "notifies rejected", zonestats_desc);
	set_desc(dns_zonestatscounter_soaoutv4, dns_zonestatscounter_max,
		 "IPv4 SOA queries sent", zonestats_desc);
	set_desc(dns_zonestatscounter_soaoutv6, dns_zonestatscounter_max,
		 "IPv6 SOA queries sent", zonestats_desc);
	set_desc(dns_zonestatscounter_axfrreqv4, dns_zonestatscounter_max,
		 "IPv4 AXFR requested", zonestats_desc);
	set_desc(dns_zonestatscounter_axfrreqv6, dns_zonestatscounter_max,
		 "IPv6 AXFR requested", zonestats_desc);
	set_desc(dns_zonestatscounter_ixfrreqv4, dns_zonestatscounter_max,
		 "IPv4 IXFR requested", zonestats_desc);
	set_desc(dns_zonestatscounter_ixfrreqv6, dns_zonestatscounter_max,
		 "IPv6 IXFR requested", zonestats_desc);
	set_desc(dns_zonestatscounter_xfrsuccess, dns_zonestatscounter_max,
		 "transfer requests succeeded", zonestats_desc);
	set_desc(dns_zonestatscounter_xfrfail, dns_zonestatscounter_max,
		 "transfer requests failed", zonestats_desc);

	/* Sanity check */
	for (i = 0; i < dns_nsstatscounter_max; i++)
		INSIST(nsstats_desc[i] != NULL);
	for (i = 0; i < dns_resstatscounter_max; i++)
		INSIST(resstats_desc[i] != NULL);
	for (i = 0; i < dns_zonestatscounter_max; i++)
		INSIST(zonestats_desc[i] != NULL);
}

isc_result_t
ns_stats_dump(ns_server_t *server, FILE *fp) {
	isc_stdtime_t now;
	isc_result_t result;
	dns_view_t *view;
	dns_zone_t *zone, *next;
	stats_dumparg_t dumparg;

	RUNTIME_CHECK(isc_once_do(&once, init_desc) == ISC_R_SUCCESS);

	/* Set common fields */
	dumparg.type = statsformat_file;
	dumparg.arg = fp;

	isc_stdtime_get(&now);
	fprintf(fp, "+++ Statistics Dump +++ (%lu)\n", (unsigned long)now);

	fprintf(fp, "++ Incoming Requests ++\n");
	dns_opcodestats_dump(server->opcodestats, opcodestat_dump, &dumparg, 0);

	fprintf(fp, "++ Incoming Queries ++\n");
	dns_rdatatypestats_dump(server->rcvquerystats, rdtypestat_dump,
				&dumparg, 0);

	fprintf(fp, "++ Outgoing Queries ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		if (view->resquerystats == NULL)
			continue;
		if (strcmp(view->name, "_default") == 0)
			fprintf(fp, "[View: default]\n");
		else
			fprintf(fp, "[View: %s]\n", view->name);
		dns_rdatatypestats_dump(view->resquerystats, rdtypestat_dump,
					&dumparg, 0);
	}

	fprintf(fp, "++ Name Server Statistics ++\n");
	dumparg.desc = nsstats_desc;
	dumparg.ncounters = dns_nsstatscounter_max;
	dns_generalstats_dump(server->nsstats, generalstat_dump, &dumparg, 0);
	fprintf(fp, "++ Zone Maintenance Statistics ++\n");
	dumparg.desc = zonestats_desc;
	dumparg.ncounters = dns_zonestatscounter_max;
	dns_generalstats_dump(server->zonestats, generalstat_dump, &dumparg, 0);

	fprintf(fp, "++ Resolver Statistics ++\n");
	fprintf(fp, "[Common]\n");
	dumparg.desc = resstats_desc;
	dumparg.ncounters = dns_resstatscounter_max;
	dns_generalstats_dump(server->resolverstats, generalstat_dump, &dumparg,
			      0);
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		if (view->resstats == NULL)
			continue;
		if (strcmp(view->name, "_default") == 0)
			fprintf(fp, "[View: default]\n");
		else
			fprintf(fp, "[View: %s]\n", view->name);
		dns_generalstats_dump(view->resstats, generalstat_dump,
				      &dumparg, 0);
	}

	fprintf(fp, "++ Cache DB RRsets ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist);
	     view != NULL;
	     view = ISC_LIST_NEXT(view, link)) {
		dns_stats_t *cachestats;

		cachestats = dns_db_getrrsetstats(view->cachedb);
		if (cachestats == NULL)
			continue;
		if (strcmp(view->name, "_default") == 0)
			fprintf(fp, "[View: default]\n");
		else
			fprintf(fp, "[View: %s]\n", view->name);
		dns_rdatasetstats_dump(cachestats, rdatasetstats_dump, &dumparg,
				       0);
	}

	fprintf(fp, "++ Per Zone Query Statistics ++\n");
	zone = NULL;
	for (result = dns_zone_first(server->zonemgr, &zone);
	     result == ISC_R_SUCCESS;
	     next = NULL, result = dns_zone_next(zone, &next), zone = next)
	{
		dns_stats_t *zonestats = dns_zone_getrequeststats(zone);
		if (zonestats != NULL) {
			char zonename[DNS_NAME_FORMATSIZE];

			dns_name_format(dns_zone_getorigin(zone),
					zonename, sizeof(zonename));
			view = dns_zone_getview(zone);

			fprintf(fp, "[%s", zonename);
			if (strcmp(view->name, "_default") != 0)
				fprintf(fp, " (view: %s)", view->name);
			fprintf(fp, "]\n");

			dumparg.desc = nsstats_desc;
			dumparg.ncounters = dns_nsstatscounter_max;
			dns_generalstats_dump(zonestats, generalstat_dump,
					      &dumparg, 0);
		}
	}

	fprintf(fp, "--- Statistics Dump --- (%lu)\n", (unsigned long)now);

	return (ISC_R_SUCCESS);	/* this function currently always succeeds */
}
