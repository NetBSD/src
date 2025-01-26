/*	$NetBSD: delv.c,v 1.15 2025/01/26 16:24:32 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <arpa/inet.h>
#include <bind.keys.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/async.h>
#include <isc/attributes.h>
#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/hex.h>
#include <isc/log.h>
#include <isc/managers.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/parseint.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/tls.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/byaddr.h>
#include <dns/cache.h>
#include <dns/client.h>
#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/keytable.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/request.h>
#include <dns/result.h>
#include <dns/rootns.h>
#include <dns/secalg.h>
#include <dns/stats.h>
#include <dns/view.h>

#include <dst/dst.h>

#include <isccfg/log.h>
#include <isccfg/namedconf.h>

#include <ns/client.h>
#include <ns/interfacemgr.h>
#include <ns/server.h>

#include <irs/resconf.h>

#define CHECK(r)                             \
	do {                                 \
		result = (r);                \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

#define MAXNAME (DNS_NAME_MAXTEXT + 1)

#define MAX_QUERIES  32
#define MAX_RESTARTS 11

/* Variables used internally by delv. */
char *progname = NULL;
static isc_mem_t *mctx = NULL;
static isc_log_t *lctx = NULL;
static dns_view_t *view = NULL;
static ns_server_t *sctx = NULL;
static ns_interface_t *ifp = NULL;
static dns_dispatch_t *dispatch = NULL;
static dns_db_t *roothints = NULL;
static isc_stats_t *resstats = NULL;
static dns_stats_t *resquerystats = NULL;
static FILE *logfp = NULL;

/* Managers */
static isc_nm_t *netmgr = NULL;
static isc_loopmgr_t *loopmgr = NULL;
static dns_dispatchmgr_t *dispatchmgr = NULL;
static dns_requestmgr_t *requestmgr = NULL;
static ns_interfacemgr_t *interfacemgr = NULL;

/* TLS */
static isc_tlsctx_cache_t *tlsctx_client_cache = NULL;

/* Configurables */
static char *server = NULL;
static const char *port = "53";
static uint32_t destport = 53;
static isc_sockaddr_t *srcaddr4 = NULL, *srcaddr6 = NULL;
static isc_sockaddr_t a4, a6;
static char *curqname = NULL, *qname = NULL;
static bool classset = false;
static dns_rdatatype_t qtype = dns_rdatatype_none;
static bool typeset = false;
static const char *hintfile = NULL;

static unsigned int styleflags = 0;
static uint32_t splitwidth = 0xffffffff;
static bool showcomments = true, showdnssec = true, showtrust = true,
	    rrcomments = true, noclass = false, nocrypto = false, nottl = false,
	    multiline = false, short_form = false, print_unknown_format = false,
	    yaml = false, fulltrace = false;

static uint32_t maxqueries = MAX_QUERIES;
static uint32_t restarts = MAX_RESTARTS;

static bool resolve_trace = false, validator_trace = false,
	    message_trace = false, send_trace = false;

static bool use_ipv4 = true, use_ipv6 = true;

static bool cdflag = false, no_sigs = false, root_validation = true;
static bool qmin = false, qmin_strict = false;

static bool use_tcp = false;

static char *anchorfile = NULL;
static char *trust_anchor = NULL;
static int num_keys = 0;

static dns_fixedname_t afn;
static dns_name_t *anchor_name = NULL;

static dns_master_style_t *style = NULL;
static dns_fixedname_t qfn;

/* Default trust anchors */
static char anchortext[] = TRUST_ANCHORS;

/*
 * Static function prototypes
 */
static isc_result_t
get_reverse(char *reverse, size_t len, char *value, bool strict);

static isc_result_t
parse_uint(uint32_t *uip, const char *value, uint32_t max, const char *desc);

static void
usage(void) {
	fprintf(stderr,
		"Usage:  delv [@server] {q-opt} {d-opt} [domain] [q-type] "
		"[q-class]\n"
		"Where:  domain	  is in the Domain Name System\n"
		"        q-class  is one of (in,hs,ch,...) [default: in]\n"
		"        q-type   is one of "
		"(a,any,mx,ns,soa,hinfo,axfr,txt,...) "
		"[default:a]\n"
		"        q-opt    is one of:\n"
		"                 -4                  (use IPv4 query "
		"transport "
		"only)\n"
		"                 -6                  (use IPv6 query "
		"transport "
		"only)\n"
		"                 -a anchor-file      (specify root trust "
		"anchor)\n"
		"                 -b address[#port]   (bind to source "
		"address/port)\n"
		"                 -c class            (option included for "
		"compatibility;\n"
		"                 -d level            (set debugging level)\n"
		"                 -h                  (print help and exit)\n"
		"                 -i                  (disable DNSSEC "
		"validation)\n"
		"                 -m                  (enable memory usage "
		"debugging)\n"
		"                 -p port             (specify port number)\n"
		"                 -q name             (specify query name)\n"
		"                 -t type             (specify query type)\n"
		"                                      only IN is supported)\n"
		"                 -v                  (print version and "
		"exit)\n"
		"                 -x dot-notation     (shortcut for reverse "
		"lookups)\n"
		"        d-opt    is of the form +keyword[=value], where "
		"keyword "
		"is:\n"
		"                 +[no]all            (Set or clear all "
		"display "
		"flags)\n"
		"                 +[no]class          (Control display of "
		"class)\n"
		"                 +[no]comments       (Control display of "
		"comment lines)\n"
		"                 +[no]crypto         (Control display of "
		"cryptographic\n"
		"                                      fields in records)\n"
		"                 +[no]dlv            (Obsolete)\n"
		"                 +[no]dnssec         (Display DNSSEC "
		"records)\n"
		"                 +[no]mtrace         (Trace messages "
		"received)\n"
		"                 +[no]ns             (Run internal name "
		"server)\n"
		"                 +[no]multiline      (Print records in an "
		"expanded format)\n"
		"                 +[no]qmin[=mode]    (QNAME minimization: "
		"relaxed or strict)\n"
		"                 +[no]root           (DNSSEC validation trust "
		"anchor)\n"
		"                 +[no]rrcomments     (Control display of "
		"per-record "
		"comments)\n"
		"                 +[no]rtrace         (Trace resolver "
		"fetches)\n"
		"                 +[no]short          (Short form answer)\n"
		"                 +[no]split=##       (Split hex/base64 fields "
		"into chunks)\n"
		"                 +[no]strace         (Trace messages "
		"sent)\n"
		"                 +[no]tcp            (TCP mode)\n"
		"                 +[no]ttl            (Control display of ttls "
		"in records)\n"
		"                 +[no]trust          (Control display of "
		"trust "
		"level)\n"
		"                 +[no]unknownformat  (Print RDATA in RFC 3597 "
		"\"unknown\" format)\n"
		"                 +[no]vtrace         (Trace validation "
		"process)\n"
		"                 +[no]yaml           (Present the results as "
		"YAML)\n");
	exit(EXIT_FAILURE);
}

noreturn static void
fatal(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

static void
fatal(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "%s: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	_exit(EXIT_FAILURE);
}

static void
warn(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

static void
warn(const char *format, ...) {
	va_list args;

	fflush(stdout);
	fprintf(stderr, "%s: warning: ", progname);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

static isc_logcategory_t categories[] = { { "delv", 0 }, { NULL, 0 } };
#define LOGCATEGORY_DEFAULT (&categories[0])
#define LOGMODULE_DEFAULT   (&modules[0])

static isc_logmodule_t modules[] = { { "delv", 0 }, { NULL, 0 } };

static void
delv_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

static void
delv_log(int level, const char *fmt, ...) {
	va_list ap;
	char msgbuf[2048];

	if (!isc_log_wouldlog(lctx, level)) {
		return;
	}

	va_start(ap, fmt);

	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	isc_log_write(lctx, LOGCATEGORY_DEFAULT, LOGMODULE_DEFAULT, level, "%s",
		      msgbuf);
	va_end(ap);
}

static int loglevel = 0;

static void
setup_logging(FILE *errout) {
	isc_result_t result;
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;
	int packetlevel = 10;

	isc_log_create(mctx, &lctx, &logconfig);
	isc_log_registercategories(lctx, categories);
	isc_log_registermodules(lctx, modules);
	isc_log_setcontext(lctx);
	dns_log_init(lctx);
	dns_log_setcontext(lctx);
	cfg_log_init(lctx);

	destination.file.stream = errout;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
			      ISC_LOG_DYNAMIC, &destination,
			      ISC_LOG_PRINTPREFIX);

	isc_log_setdebuglevel(lctx, loglevel);
	isc_log_settag(logconfig, ";; ");

	result = isc_log_usechannel(logconfig, "stderr",
				    ISC_LOGCATEGORY_DEFAULT, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("Couldn't attach to log channel 'stderr'");
	}

	if (resolve_trace && loglevel < 1) {
		isc_log_createchannel(logconfig, "resolver", ISC_LOG_TOFILEDESC,
				      ISC_LOG_DEBUG(1), &destination,
				      ISC_LOG_PRINTPREFIX);

		result = isc_log_usechannel(logconfig, "resolver",
					    DNS_LOGCATEGORY_RESOLVER,
					    DNS_LOGMODULE_RESOLVER);
		if (result != ISC_R_SUCCESS) {
			fatal("Couldn't attach to log channel 'resolver'");
		}
	}

	if (validator_trace && loglevel < 3) {
		isc_log_createchannel(logconfig, "validator",
				      ISC_LOG_TOFILEDESC, ISC_LOG_DEBUG(3),
				      &destination, ISC_LOG_PRINTPREFIX);

		result = isc_log_usechannel(logconfig, "validator",
					    DNS_LOGCATEGORY_DNSSEC,
					    DNS_LOGMODULE_VALIDATOR);
		if (result != ISC_R_SUCCESS) {
			fatal("Couldn't attach to log channel 'validator'");
		}
	}

	if (send_trace) {
		packetlevel = 11;
	}
	if ((message_trace || send_trace) && loglevel < packetlevel) {
		isc_log_createchannel(logconfig, "messages", ISC_LOG_TOFILEDESC,
				      ISC_LOG_DEBUG(packetlevel), &destination,
				      ISC_LOG_PRINTPREFIX);

		result = isc_log_usechannel(logconfig, "messages",
					    DNS_LOGCATEGORY_RESOLVER,
					    DNS_LOGMODULE_PACKETS);
		if (result != ISC_R_SUCCESS) {
			fatal("Couldn't attach to log channel 'messagse'");
		}
	}
}

static void
print_status(dns_rdataset_t *rdataset) {
	char buf[1024] = { 0 };

	REQUIRE(rdataset != NULL);

	if (!showtrust || !dns_rdataset_isassociated(rdataset)) {
		return;
	}

	buf[0] = '\0';

	if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
		strlcat(buf, "negative response", sizeof(buf));
		strlcat(buf, (yaml ? "_" : ", "), sizeof(buf));
	}

	switch (rdataset->trust) {
	case dns_trust_none:
		strlcat(buf, "untrusted", sizeof(buf));
		break;
	case dns_trust_pending_additional:
		strlcat(buf, "signed additional data", sizeof(buf));
		if (!yaml) {
			strlcat(buf, ", ", sizeof(buf));
		}
		strlcat(buf, "pending validation", sizeof(buf));
		break;
	case dns_trust_pending_answer:
		strlcat(buf, "signed answer", sizeof(buf));
		if (!yaml) {
			strlcat(buf, ", ", sizeof(buf));
		}
		strlcat(buf, "pending validation", sizeof(buf));
		break;
	case dns_trust_additional:
		strlcat(buf, "unsigned additional data", sizeof(buf));
		break;
	case dns_trust_glue:
		strlcat(buf, "glue data", sizeof(buf));
		break;
	case dns_trust_answer:
		if (root_validation) {
			strlcat(buf, "unsigned answer", sizeof(buf));
		} else {
			strlcat(buf, "answer not validated", sizeof(buf));
		}
		break;
	case dns_trust_authauthority:
		strlcat(buf, "authority data", sizeof(buf));
		break;
	case dns_trust_authanswer:
		strlcat(buf, "authoritative", sizeof(buf));
		break;
	case dns_trust_secure:
		strlcat(buf, "fully validated", sizeof(buf));
		break;
	case dns_trust_ultimate:
		strlcat(buf, "ultimate trust", sizeof(buf));
		break;
	}

	if (yaml) {
		char *p;

		/* Convert spaces to underscores for YAML */
		for (p = buf; p != NULL && *p != '\0'; p++) {
			if (*p == ' ') {
				*p = '_';
			}
		}

		printf("  - %s:\n", buf);
	} else {
		printf("; %s\n", buf);
	}
}

static void
printdata(dns_rdataset_t *rdataset, dns_name_t *owner) {
	isc_result_t result = ISC_R_SUCCESS;
	static dns_trust_t trust;
	static bool first = true;
	isc_buffer_t target;
	isc_region_t r;
	char *t = NULL;
	int len = 2048;

	if (!dns_rdataset_isassociated(rdataset)) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(owner, namebuf, sizeof(namebuf));
		delv_log(ISC_LOG_DEBUG(4), "warning: empty rdataset %s",
			 namebuf);
		return;
	}

	if (!showdnssec && rdataset->type == dns_rdatatype_rrsig) {
		return;
	}

	if (first || rdataset->trust != trust) {
		if (!first && showtrust && !short_form && !yaml) {
			putchar('\n');
		}
		print_status(rdataset);
		trust = rdataset->trust;
		first = false;
	}

	do {
		t = isc_mem_get(mctx, len);

		isc_buffer_init(&target, t, len);
		if (short_form) {
			dns_rdata_t rdata = DNS_RDATA_INIT;
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset))
			{
				if ((rdataset->attributes &
				     DNS_RDATASETATTR_NEGATIVE) != 0)
				{
					continue;
				}

				dns_rdataset_current(rdataset, &rdata);
				result = dns_rdata_tofmttext(
					&rdata, dns_rootname, styleflags, 0,
					splitwidth, " ", &target);
				if (result != ISC_R_SUCCESS) {
					break;
				}

				if (isc_buffer_availablelength(&target) < 1) {
					result = ISC_R_NOSPACE;
					break;
				}

				isc_buffer_putstr(&target, "\n");

				dns_rdata_reset(&rdata);
			}
		} else {
			dns_indent_t indent = { "  ", 2 };
			if (!yaml && (rdataset->attributes &
				      DNS_RDATASETATTR_NEGATIVE) != 0)
			{
				isc_buffer_putstr(&target, "; ");
			}
			result = dns_master_rdatasettotext(
				owner, rdataset, style, yaml ? &indent : NULL,
				&target);
		}

		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, t, len);
			len += 1024;
		} else if (result == ISC_R_NOMORE) {
			result = ISC_R_SUCCESS;
		} else {
			CHECK(result);
		}
	} while (result == ISC_R_NOSPACE);

	isc_buffer_usedregion(&target, &r);
	printf("%.*s", (int)r.length, (char *)r.base);

cleanup:
	if (t != NULL) {
		isc_mem_put(mctx, t, len);
	}
}

static isc_result_t
setup_style(void) {
	isc_result_t result;

	styleflags |= DNS_STYLEFLAG_REL_OWNER;
	if (yaml) {
		styleflags |= DNS_STYLEFLAG_YAML;
	} else {
		if (showcomments) {
			styleflags |= DNS_STYLEFLAG_COMMENT;
		}
		if (print_unknown_format) {
			styleflags |= DNS_STYLEFLAG_UNKNOWNFORMAT;
		}
		if (rrcomments) {
			styleflags |= DNS_STYLEFLAG_RRCOMMENT;
		}
		if (nottl) {
			styleflags |= DNS_STYLEFLAG_NO_TTL;
		}
		if (noclass) {
			styleflags |= DNS_STYLEFLAG_NO_CLASS;
		}
		if (nocrypto) {
			styleflags |= DNS_STYLEFLAG_NOCRYPTO;
		}
		if (multiline) {
			styleflags |= DNS_STYLEFLAG_MULTILINE;
			styleflags |= DNS_STYLEFLAG_COMMENT;
		}
	}

	if (multiline || (nottl && noclass)) {
		result = dns_master_stylecreate(&style, styleflags, 24, 24, 24,
						32, 80, 8, splitwidth, mctx);
	} else if (nottl || noclass) {
		result = dns_master_stylecreate(&style, styleflags, 24, 24, 32,
						40, 80, 8, splitwidth, mctx);
	} else {
		result = dns_master_stylecreate(&style, styleflags, 24, 32, 40,
						48, 80, 8, splitwidth, mctx);
	}

	return result;
}

static isc_result_t
convert_name(dns_fixedname_t *fn, dns_name_t **name, const char *text) {
	isc_result_t result;
	isc_buffer_t b;
	dns_name_t *n = NULL;
	unsigned int len;

	REQUIRE(fn != NULL && name != NULL && text != NULL);
	len = strlen(text);

	isc_buffer_constinit(&b, text, len);
	isc_buffer_add(&b, len);
	n = dns_fixedname_initname(fn);

	result = dns_name_fromtext(n, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		delv_log(ISC_LOG_ERROR, "failed to convert name %s: %s", text,
			 isc_result_totext(result));
		return result;
	}

	*name = n;
	return ISC_R_SUCCESS;
}

static isc_result_t
key_fromconfig(const cfg_obj_t *key, dns_client_t *client, dns_view_t *toview) {
	dns_rdata_dnskey_t dnskey;
	dns_rdata_ds_t ds;
	uint32_t rdata1, rdata2, rdata3;
	const char *datastr = NULL, *keynamestr = NULL, *atstr = NULL;
	unsigned char data[4096];
	isc_buffer_t databuf;
	unsigned char rrdata[4096];
	isc_buffer_t rrdatabuf;
	isc_region_t r;
	dns_fixedname_t fkeyname;
	dns_name_t *keyname;
	isc_result_t result;
	bool match_root = false;
	enum {
		INITIAL_KEY,
		STATIC_KEY,
		INITIAL_DS,
		STATIC_DS,
		TRUSTED
	} anchortype;
	const cfg_obj_t *obj;

	REQUIRE(client != NULL || toview != NULL);

	keynamestr = cfg_obj_asstring(cfg_tuple_get(key, "name"));
	CHECK(convert_name(&fkeyname, &keyname, keynamestr));

	if (!root_validation) {
		return ISC_R_SUCCESS;
	}

	if (anchor_name) {
		match_root = dns_name_equal(keyname, anchor_name);
	}

	if (!match_root) {
		return ISC_R_SUCCESS;
	}

	if (!root_validation) {
		return ISC_R_SUCCESS;
	}

	delv_log(ISC_LOG_DEBUG(3), "adding trust anchor %s", trust_anchor);

	/* if DNSKEY, flags; if DS, key tag */
	rdata1 = cfg_obj_asuint32(cfg_tuple_get(key, "rdata1"));

	/* if DNSKEY, protocol; if DS, algorithm */
	rdata2 = cfg_obj_asuint32(cfg_tuple_get(key, "rdata2"));

	/* if DNSKEY, algorithm; if DS, digest type */
	rdata3 = cfg_obj_asuint32(cfg_tuple_get(key, "rdata3"));

	/* What type of trust anchor is this? */
	obj = cfg_tuple_get(key, "anchortype");
	if (cfg_obj_isvoid(obj)) {
		/*
		 * "anchortype" is not defined, this must be a static-key
		 * configured with trusted-keys.
		 */
		anchortype = STATIC_KEY;
	} else {
		atstr = cfg_obj_asstring(obj);
		if (strcasecmp(atstr, "static-key") == 0) {
			anchortype = STATIC_KEY;
		} else if (strcasecmp(atstr, "static-ds") == 0) {
			anchortype = STATIC_DS;
		} else if (strcasecmp(atstr, "initial-key") == 0) {
			anchortype = INITIAL_KEY;
		} else if (strcasecmp(atstr, "initial-ds") == 0) {
			anchortype = INITIAL_DS;
		} else {
			delv_log(ISC_LOG_ERROR,
				 "key '%s': invalid initialization method '%s'",
				 keynamestr, atstr);
			result = ISC_R_FAILURE;
			goto cleanup;
		}
	}

	isc_buffer_init(&databuf, data, sizeof(data));
	isc_buffer_init(&rrdatabuf, rrdata, sizeof(rrdata));

	if (rdata1 > 0xffff) {
		CHECK(ISC_R_RANGE);
	}
	if (rdata2 > 0xff) {
		CHECK(ISC_R_RANGE);
	}
	if (rdata3 > 0xff) {
		CHECK(ISC_R_RANGE);
	}

	switch (anchortype) {
	case STATIC_KEY:
	case INITIAL_KEY:
	case TRUSTED:
		dnskey.common.rdclass = dns_rdataclass_in;
		dnskey.common.rdtype = dns_rdatatype_dnskey;
		dnskey.mctx = NULL;

		ISC_LINK_INIT(&dnskey.common, link);

		dnskey.flags = (uint16_t)rdata1;
		dnskey.protocol = (uint8_t)rdata2;
		dnskey.algorithm = (uint8_t)rdata3;

		datastr = cfg_obj_asstring(cfg_tuple_get(key, "data"));
		CHECK(isc_base64_decodestring(datastr, &databuf));
		isc_buffer_usedregion(&databuf, &r);
		dnskey.datalen = r.length;
		dnskey.data = r.base;

		CHECK(dns_rdata_fromstruct(NULL, dnskey.common.rdclass,
					   dnskey.common.rdtype, &dnskey,
					   &rrdatabuf));
		if (client != NULL) {
			CHECK(dns_client_addtrustedkey(
				client, dns_rdataclass_in, dns_rdatatype_dnskey,
				keyname, &rrdatabuf));
		} else if (toview != NULL) {
			CHECK(dns_view_addtrustedkey(toview,
						     dns_rdatatype_dnskey,
						     keyname, &rrdatabuf));
		}
		break;
	case INITIAL_DS:
	case STATIC_DS:
		ds.common.rdclass = dns_rdataclass_in;
		ds.common.rdtype = dns_rdatatype_ds;
		ds.mctx = NULL;

		ISC_LINK_INIT(&ds.common, link);

		ds.key_tag = (uint16_t)rdata1;
		ds.algorithm = (uint8_t)rdata2;
		ds.digest_type = (uint8_t)rdata3;

		datastr = cfg_obj_asstring(cfg_tuple_get(key, "data"));
		CHECK(isc_hex_decodestring(datastr, &databuf));
		isc_buffer_usedregion(&databuf, &r);

		switch (ds.digest_type) {
		case DNS_DSDIGEST_SHA1:
			if (r.length != ISC_SHA1_DIGESTLENGTH) {
				CHECK(ISC_R_UNEXPECTEDEND);
			}
			break;
		case DNS_DSDIGEST_SHA256:
			if (r.length != ISC_SHA256_DIGESTLENGTH) {
				CHECK(ISC_R_UNEXPECTEDEND);
			}
			break;
		case DNS_DSDIGEST_SHA384:
			if (r.length != ISC_SHA384_DIGESTLENGTH) {
				CHECK(ISC_R_UNEXPECTEDEND);
			}
			break;
		}

		ds.length = r.length;
		ds.digest = r.base;

		CHECK(dns_rdata_fromstruct(NULL, ds.common.rdclass,
					   ds.common.rdtype, &ds, &rrdatabuf));
		if (client != NULL) {
			CHECK(dns_client_addtrustedkey(
				client, dns_rdataclass_in, dns_rdatatype_ds,
				keyname, &rrdatabuf));
		} else if (toview != NULL) {
			CHECK(dns_view_addtrustedkey(toview, dns_rdatatype_ds,
						     keyname, &rrdatabuf));
		}
	}

	num_keys++;

cleanup:
	if (result == DST_R_NOCRYPTO) {
		cfg_obj_log(key, lctx, ISC_LOG_ERROR, "no crypto support");
	} else if (result == DST_R_UNSUPPORTEDALG) {
		cfg_obj_log(key, lctx, ISC_LOG_WARNING,
			    "skipping trusted key '%s': %s", keynamestr,
			    isc_result_totext(result));
		result = ISC_R_SUCCESS;
	} else if (result != ISC_R_SUCCESS) {
		cfg_obj_log(key, lctx, ISC_LOG_ERROR,
			    "failed to add trusted key '%s': %s", keynamestr,
			    isc_result_totext(result));
		result = ISC_R_FAILURE;
	}

	return result;
}

static isc_result_t
load_keys(const cfg_obj_t *keys, dns_client_t *client, dns_view_t *toview) {
	const cfg_listelt_t *elt, *elt2;
	const cfg_obj_t *key, *keylist;
	isc_result_t result = ISC_R_SUCCESS;

	for (elt = cfg_list_first(keys); elt != NULL; elt = cfg_list_next(elt))
	{
		keylist = cfg_listelt_value(elt);

		for (elt2 = cfg_list_first(keylist); elt2 != NULL;
		     elt2 = cfg_list_next(elt2))
		{
			key = cfg_listelt_value(elt2);
			CHECK(key_fromconfig(key, client, toview));
		}
	}

cleanup:
	if (result == DST_R_NOCRYPTO) {
		result = ISC_R_SUCCESS;
	}
	return result;
}

static isc_result_t
setup_dnsseckeys(dns_client_t *client, dns_view_t *toview) {
	isc_result_t result;
	cfg_parser_t *parser = NULL;
	const cfg_obj_t *trusted_keys = NULL;
	const cfg_obj_t *managed_keys = NULL;
	const cfg_obj_t *trust_anchors = NULL;
	cfg_obj_t *bindkeys = NULL;

	if (!root_validation) {
		return ISC_R_SUCCESS;
	}

	if (trust_anchor == NULL) {
		trust_anchor = isc_mem_strdup(mctx, ".");
	}

	if (trust_anchor != NULL) {
		CHECK(convert_name(&afn, &anchor_name, trust_anchor));
	}

	CHECK(cfg_parser_create(mctx, dns_lctx, &parser));

	if (anchorfile != NULL) {
		if (access(anchorfile, R_OK) != 0) {
			fatal("Unable to read key file '%s'", anchorfile);
		}

		result = cfg_parse_file(parser, anchorfile, &cfg_type_bindkeys,
					&bindkeys);
		if (result != ISC_R_SUCCESS) {
			fatal("Unable to load keys from '%s'", anchorfile);
		}
	} else {
		isc_buffer_t b;

		isc_buffer_init(&b, anchortext, sizeof(anchortext) - 1);
		isc_buffer_add(&b, sizeof(anchortext) - 1);
		cfg_parser_reset(parser);
		result = cfg_parse_buffer(parser, &b, NULL, 0,
					  &cfg_type_bindkeys, 0, &bindkeys);
		if (result != ISC_R_SUCCESS) {
			fatal("Unable to parse built-in keys");
		}
	}

	INSIST(bindkeys != NULL);
	cfg_map_get(bindkeys, "trusted-keys", &trusted_keys);
	cfg_map_get(bindkeys, "managed-keys", &managed_keys);
	cfg_map_get(bindkeys, "trust-anchors", &trust_anchors);

	if (trusted_keys != NULL) {
		CHECK(load_keys(trusted_keys, client, toview));
	}
	if (managed_keys != NULL) {
		CHECK(load_keys(managed_keys, client, toview));
	}
	if (trust_anchors != NULL) {
		CHECK(load_keys(trust_anchors, client, toview));
	}
	result = ISC_R_SUCCESS;

	if (num_keys == 0) {
		fatal("No trusted keys were loaded");
	}

cleanup:
	if (bindkeys != NULL) {
		cfg_obj_destroy(parser, &bindkeys);
	}
	if (parser != NULL) {
		cfg_parser_destroy(&parser);
	}
	if (result != ISC_R_SUCCESS) {
		delv_log(ISC_LOG_ERROR, "setup_dnsseckeys: %s",
			 isc_result_totext(result));
	}
	return result;
}

static isc_result_t
addserver(dns_client_t *client) {
	struct addrinfo hints, *res = NULL, *cur = NULL;
	int gaierror;
	struct in_addr in4;
	struct in6_addr in6;
	isc_sockaddr_t *sa = NULL;
	isc_sockaddrlist_t servers;
	isc_result_t result;
	dns_name_t *name = NULL;

	ISC_LIST_INIT(servers);

	if (inet_pton(AF_INET, server, &in4) == 1) {
		if (!use_ipv4) {
			fatal("Use of IPv4 disabled by -6");
		}
		sa = isc_mem_get(mctx, sizeof(*sa));
		ISC_LINK_INIT(sa, link);
		isc_sockaddr_fromin(sa, &in4, destport);
		ISC_LIST_APPEND(servers, sa, link);
	} else if (inet_pton(AF_INET6, server, &in6) == 1) {
		if (!use_ipv6) {
			fatal("Use of IPv6 disabled by -4");
		}
		sa = isc_mem_get(mctx, sizeof(*sa));
		ISC_LINK_INIT(sa, link);
		isc_sockaddr_fromin6(sa, &in6, destport);
		ISC_LIST_APPEND(servers, sa, link);
	} else {
		memset(&hints, 0, sizeof(hints));
		if (!use_ipv6) {
			hints.ai_family = AF_INET;
		} else if (!use_ipv4) {
			hints.ai_family = AF_INET6;
		} else {
			hints.ai_family = AF_UNSPEC;
		}
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		gaierror = getaddrinfo(server, port, &hints, &res);
		if (gaierror != 0) {
			delv_log(ISC_LOG_ERROR, "getaddrinfo failed: %s",
				 gai_strerror(gaierror));
			return ISC_R_FAILURE;
		}

		result = ISC_R_SUCCESS;
		for (cur = res; cur != NULL; cur = cur->ai_next) {
			if (cur->ai_family != AF_INET &&
			    cur->ai_family != AF_INET6)
			{
				continue;
			}
			sa = isc_mem_get(mctx, sizeof(*sa));
			*sa = (isc_sockaddr_t){
				.length = (unsigned int)cur->ai_addrlen,
			};
			ISC_LINK_INIT(sa, link);
			memmove(&sa->type, cur->ai_addr, cur->ai_addrlen);
			ISC_LIST_APPEND(servers, sa, link);
		}
		freeaddrinfo(res);
		CHECK(result);
	}

	CHECK(dns_client_setservers(client, dns_rdataclass_in, name, &servers));

cleanup:
	while (!ISC_LIST_EMPTY(servers)) {
		sa = ISC_LIST_HEAD(servers);
		ISC_LIST_UNLINK(servers, sa, link);
		isc_mem_put(mctx, sa, sizeof(*sa));
	}

	if (result != ISC_R_SUCCESS) {
		delv_log(ISC_LOG_ERROR, "addserver: %s",
			 isc_result_totext(result));
	}

	return result;
}

static isc_result_t
findserver(dns_client_t *client) {
	isc_result_t result;
	irs_resconf_t *resconf = NULL;
	isc_sockaddrlist_t *nameservers;
	isc_sockaddr_t *sa = NULL, *next = NULL;

	result = irs_resconf_load(mctx, "/etc/resolv.conf", &resconf);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		delv_log(ISC_LOG_ERROR, "irs_resconf_load: %s",
			 isc_result_totext(result));
		goto cleanup;
	}

	/* Get nameservers from resolv.conf */
	nameservers = irs_resconf_getnameservers(resconf);
	for (sa = ISC_LIST_HEAD(*nameservers); sa != NULL; sa = next) {
		next = ISC_LIST_NEXT(sa, link);

		/* Set destination port */
		if (sa->type.sa.sa_family == AF_INET && use_ipv4) {
			sa->type.sin.sin_port = htons(destport);
			continue;
		}
		if (sa->type.sa.sa_family == AF_INET6 && use_ipv6) {
			sa->type.sin6.sin6_port = htons(destport);
			continue;
		}

		/* Incompatible protocol family */
		ISC_LIST_UNLINK(*nameservers, sa, link);
		isc_mem_put(mctx, sa, sizeof(*sa));
	}

	/* None found, use localhost */
	if (ISC_LIST_EMPTY(*nameservers)) {
		if (use_ipv4) {
			struct in_addr localhost;
			localhost.s_addr = htonl(INADDR_LOOPBACK);
			sa = isc_mem_get(mctx, sizeof(*sa));
			isc_sockaddr_fromin(sa, &localhost, destport);

			ISC_LINK_INIT(sa, link);
			ISC_LIST_APPEND(*nameservers, sa, link);
		}

		if (use_ipv6) {
			sa = isc_mem_get(mctx, sizeof(*sa));
			isc_sockaddr_fromin6(sa, &in6addr_loopback, destport);

			ISC_LINK_INIT(sa, link);
			ISC_LIST_APPEND(*nameservers, sa, link);
		}
	}

	result = dns_client_setservers(client, dns_rdataclass_in, NULL,
				       nameservers);
	if (result != ISC_R_SUCCESS) {
		delv_log(ISC_LOG_ERROR, "dns_client_setservers: %s",
			 isc_result_totext(result));
	}

cleanup:
	if (resconf != NULL) {
		irs_resconf_destroy(&resconf);
	}
	return result;
}

static isc_result_t
parse_uint(uint32_t *uip, const char *value, uint32_t max, const char *desc) {
	uint32_t n;
	isc_result_t result = isc_parse_uint32(&n, value, 10);
	if (result == ISC_R_SUCCESS && n > max) {
		result = ISC_R_RANGE;
	}
	if (result != ISC_R_SUCCESS) {
		printf("invalid %s '%s': %s\n", desc, value,
		       isc_result_totext(result));
		return result;
	}
	*uip = n;
	return ISC_R_SUCCESS;
}

static void
plus_option(char *option) {
	isc_result_t result;
	char *cmd, *value, *last = NULL;
	bool state = true;

	INSIST(option != NULL);

	cmd = strtok_r(option, "=", &last);
	if (cmd == NULL) {
		printf(";; Invalid option %s\n", option);
		return;
	}
	if (strncasecmp(cmd, "no", 2) == 0) {
		cmd += 2;
		state = false;
	}

	value = strtok_r(NULL, "\0", &last);

#define FULLCHECK(A)                                                 \
	do {                                                         \
		size_t _l = strlen(cmd);                             \
		if (_l >= sizeof(A) || strncasecmp(cmd, A, _l) != 0) \
			goto invalid_option;                         \
	} while (0)

	switch (cmd[0]) {
	case 'a': /* all */
		FULLCHECK("all");
		showcomments = state;
		rrcomments = state;
		showtrust = state;
		break;
	case 'c':
		switch (cmd[1]) {
		case 'd': /* cdflag */
			FULLCHECK("cdflag");
			cdflag = state;
			break;
		case 'l': /* class */
			FULLCHECK("class");
			noclass = !state;
			break;
		case 'o': /* comments */
			FULLCHECK("comments");
			showcomments = state;
			break;
		case 'r': /* crypto */
			FULLCHECK("crypto");
			nocrypto = !state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'd':
		switch (cmd[1]) {
		case 'l': /* dlv */
			FULLCHECK("dlv");
			if (state) {
				fprintf(stderr, "Invalid option: "
						"+dlv is obsolete\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'n': /* dnssec */
			FULLCHECK("dnssec");
			showdnssec = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'h':
		switch (cmd[1]) {
		case 'i': /* hint */
			if (state) {
				if (value == NULL) {
					fatal("+hint: must specify hint file");
				}
				hintfile = value;
			} else {
				hintfile = NULL;
			}
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'm':
		switch (cmd[1]) {
		case 'a': /* maxqueries */
			FULLCHECK("maxqueries");
			if (value == NULL) {
				goto need_value;
			}
			if (!state) {
				goto invalid_option;
			}
			result = parse_uint(&maxqueries, value, UINT_MAX,
					    "maxqueries");
			if (result != ISC_R_SUCCESS) {
				fatal("Couldn't parse maxqueries");
			}
			if (maxqueries == 0) {
				fatal("maxqueries must be nonzero");
			}
			break;
		case 't': /* mtrace */
			FULLCHECK("mtrace");
			message_trace = state;
			if (state) {
				resolve_trace = state;
			}
			break;
		case 'u': /* multiline */
			FULLCHECK("multiline");
			multiline = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'n':
		switch (cmd[1]) {
		case 's': /* ns */
			FULLCHECK("ns");
			fulltrace = state;
			if (state) {
				message_trace = state;
				send_trace = state;
				resolve_trace = state;
				logfp = stdout;
			}
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'q': /* qmin */
		FULLCHECK("qmin");
		if (state) {
			if (value == NULL || strcasecmp(value, "relaxed") == 0)
			{
				qmin = true;
			} else if (strcasecmp(value, "strict") == 0) {
				qmin = true;
				qmin_strict = true;
			} else {
				fatal("Invalid qmin option '%s': "
				      "use 'relaxed' or 'strict'\n",
				      value);
			}
		} else {
			qmin = false;
			qmin_strict = false;
		}
		break;
	case 'r':
		switch (cmd[1]) {
		case 'e': /* restarts */
			FULLCHECK("restarts");
			if (value == NULL) {
				goto need_value;
			}
			if (!state) {
				goto invalid_option;
			}
			result = parse_uint(&restarts, value, 255, "restarts");
			if (result != ISC_R_SUCCESS) {
				fatal("Couldn't parse restarts");
			}
			if (restarts == 0) {
				fatal("restarts must be between 1..255");
			}
			break;
		case 'o': /* root */
			FULLCHECK("root");
			if (state && no_sigs) {
				break;
			}
			root_validation = state;
			if (value != NULL) {
				trust_anchor = isc_mem_strdup(mctx, value);
			}
			break;
		case 'r': /* rrcomments */
			FULLCHECK("rrcomments");
			rrcomments = state;
			break;
		case 't': /* rtrace */
			FULLCHECK("rtrace");
			resolve_trace = state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 's':
		switch (cmd[1]) {
		case 'h': /* short */
			FULLCHECK("short");
			short_form = state;
			if (short_form) {
				multiline = false;
				showcomments = false;
				showtrust = false;
				showdnssec = false;
			}
			break;
		case 'p': /* split */
			FULLCHECK("split");
			if (value != NULL && !state) {
				goto invalid_option;
			}
			if (!state) {
				splitwidth = 0;
				break;
			} else if (value == NULL) {
				break;
			}

			result = parse_uint(&splitwidth, value, 1023, "split");
			if (splitwidth % 4 != 0) {
				splitwidth = ((splitwidth + 3) / 4) * 4;
				warn("split must be a multiple of 4; "
				     "adjusting to %d",
				     splitwidth);
			}
			/*
			 * There is an adjustment done in the
			 * totext_<rrtype>() functions which causes
			 * splitwidth to shrink.  This is okay when we're
			 * using the default width but incorrect in this
			 * case, so we correct for it
			 */
			if (splitwidth) {
				splitwidth += 3;
			}
			if (result != ISC_R_SUCCESS) {
				fatal("Couldn't parse split");
			}
			break;
		case 't': /* strace */
			FULLCHECK("strace");
			send_trace = state;
			if (state) {
				message_trace = state;
			}
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'u':
		FULLCHECK("unknownformat");
		print_unknown_format = state;
		break;
	case 't':
		switch (cmd[1]) {
		case 'c': /* tcp */
			FULLCHECK("tcp");
			use_tcp = state;
			break;
		case 'r':
			switch (cmd[2]) {
			case 'a': /* trace */
				FULLCHECK("trace");
				fatal("Invalid argument +trace. For "
				      "delegation path tracing, use +ns.");
				break;
			case 'u': /* trust */
				FULLCHECK("trust");
				showtrust = state;
				break;
			default:
				goto invalid_option;
			}
			break;
		case 't': /* ttl */
			FULLCHECK("ttl");
			nottl = !state;
			break;
		default:
			goto invalid_option;
		}
		break;
	case 'v': /* vtrace */
		FULLCHECK("vtrace");
		validator_trace = state;
		if (state) {
			resolve_trace = state;
		}
		break;
	case 'y': /* yaml */
		FULLCHECK("yaml");
		yaml = state;
		if (state) {
			rrcomments = false;
		}
		break;
	default:
	invalid_option:
	need_value:
		fprintf(stderr, "Invalid option: +%s\n", option);
		usage();
	}
	return;
}

/*
 * options: "46a:b:c:d:himp:q:t:vx:";
 */
static const char *single_dash_opts = "46himv";
static const char *dash_opts = "46abcdhimpqtvx";

static bool
dash_option(char *option, char *next, bool *open_type_class) {
	char opt, *value;
	isc_result_t result;
	bool value_from_next;
	isc_textregion_t tr;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	char textname[MAXNAME];
	struct in_addr in4;
	struct in6_addr in6;
	in_port_t srcport;
	uint32_t num;
	char *hash;

	while (strpbrk(option, single_dash_opts) == &option[0]) {
		/*
		 * Since the -[46himv] options do not take an argument,
		 * account for them (in any number and/or combination)
		 * if they appear as the first character(s) of a q-opt.
		 */
		opt = option[0];
		switch (opt) {
		case '4':
			if (isc_net_probeipv4() != ISC_R_SUCCESS) {
				fatal("IPv4 networking not available");
			}
			if (use_ipv6) {
				isc_net_disableipv6();
				use_ipv6 = false;
			}
			break;
		case '6':
			if (isc_net_probeipv6() != ISC_R_SUCCESS) {
				fatal("IPv6 networking not available");
			}
			if (use_ipv4) {
				isc_net_disableipv4();
				use_ipv4 = false;
			}
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'i':
			no_sigs = true;
			root_validation = false;
			break;
		case 'm':
			/* handled in preparse_args() */
			break;
		case 'v':
			printf("delv %s\n", PACKAGE_VERSION);
			exit(EXIT_SUCCESS);
		default:
			UNREACHABLE();
		}
		if (strlen(option) > 1U) {
			option = &option[1];
		} else {
			return false;
		}
	}
	opt = option[0];
	if (strlen(option) > 1U) {
		value_from_next = false;
		value = &option[1];
	} else {
		value_from_next = true;
		value = next;
	}
	if (value == NULL) {
		goto invalid_option;
	}
	switch (opt) {
	case 'a':
		anchorfile = isc_mem_strdup(mctx, value);
		return value_from_next;
	case 'b':
		hash = strchr(value, '#');
		if (hash != NULL) {
			result = parse_uint(&num, hash + 1, 0xffff, "port");
			if (result != ISC_R_SUCCESS) {
				fatal("Couldn't parse port number");
			}
			srcport = num;
			*hash = '\0';
		} else {
			srcport = 0;
		}

		if (inet_pton(AF_INET, value, &in4) == 1) {
			if (srcaddr4 != NULL) {
				fatal("Only one local address per family "
				      "can be specified\n");
			}
			isc_sockaddr_fromin(&a4, &in4, srcport);
			srcaddr4 = &a4;
		} else if (inet_pton(AF_INET6, value, &in6) == 1) {
			if (srcaddr6 != NULL) {
				fatal("Only one local address per family "
				      "can be specified\n");
			}
			isc_sockaddr_fromin6(&a6, &in6, srcport);
			srcaddr6 = &a6;
		} else {
			if (hash != NULL) {
				*hash = '#';
			}
			fatal("Invalid address %s", value);
		}
		if (hash != NULL) {
			*hash = '#';
		}
		return value_from_next;
	case 'c':
		if (classset) {
			warn("extra query class");
		}

		*open_type_class = false;
		tr.base = value;
		tr.length = strlen(value);
		result = dns_rdataclass_fromtext(&rdclass,
						 (isc_textregion_t *)&tr);
		if (result == ISC_R_SUCCESS) {
			classset = true;
		} else if (rdclass != dns_rdataclass_in) {
			warn("ignoring non-IN query class");
		} else {
			warn("ignoring invalid class");
		}
		return value_from_next;
	case 'd':
		result = parse_uint(&num, value, 99, "debug level");
		if (result != ISC_R_SUCCESS) {
			fatal("Couldn't parse debug level");
		}
		loglevel = num;
		return value_from_next;
	case 'p':
		port = value;
		result = parse_uint(&destport, port, 0xffff, "port");
		if (result != ISC_R_SUCCESS) {
			fatal("Couldn't parse port number");
		}
		return value_from_next;
	case 'q':
		if (curqname != NULL) {
			warn("extra query name");
			isc_mem_free(mctx, curqname);
		}
		curqname = isc_mem_strdup(mctx, value);
		return value_from_next;
	case 't':
		*open_type_class = false;
		tr.base = value;
		tr.length = strlen(value);
		result = dns_rdatatype_fromtext(&rdtype,
						(isc_textregion_t *)&tr);
		if (result == ISC_R_SUCCESS) {
			if (typeset) {
				warn("extra query type");
			}
			if (rdtype == dns_rdatatype_ixfr ||
			    rdtype == dns_rdatatype_axfr)
			{
				fatal("Transfer not supported");
			}
			qtype = rdtype;
			typeset = true;
		} else {
			warn("ignoring invalid type");
		}
		return value_from_next;
	case 'x':
		result = get_reverse(textname, sizeof(textname), value, false);
		if (result == ISC_R_SUCCESS) {
			if (curqname != NULL) {
				isc_mem_free(mctx, curqname);
				warn("extra query name");
			}
			curqname = isc_mem_strdup(mctx, textname);
			if (typeset) {
				warn("extra query type");
			}
			qtype = dns_rdatatype_ptr;
			typeset = true;
		} else {
			fprintf(stderr, "Invalid IP address %s\n", value);
			exit(EXIT_FAILURE);
		}
		return value_from_next;
	invalid_option:
	default:
		fprintf(stderr, "Invalid option: -%s\n", option);
		usage();
	}
	UNREACHABLE();
	return false;
}

/*
 * Check for -m first to determine whether to enable
 * memory debugging when setting up the memory context.
 */
static void
preparse_args(int argc, char **argv) {
	bool ipv4only = false, ipv6only = false;
	char *option;

	for (argc--, argv++; argc > 0; argc--, argv++) {
		if (argv[0][0] != '-') {
			continue;
		}

		option = &argv[0][1];
		while (strpbrk(option, single_dash_opts) == &option[0]) {
			switch (option[0]) {
			case 'm':
				isc_mem_debugging = ISC_MEM_DEBUGTRACE |
						    ISC_MEM_DEBUGRECORD;
				break;
			case '4':
				if (ipv6only) {
					fatal("only one of -4 and -6 allowed");
				}
				ipv4only = true;
				break;
			case '6':
				if (ipv4only) {
					fatal("only one of -4 and -6 allowed");
				}
				ipv6only = true;
				break;
			}
			option = &option[1];
		}

		if (strlen(option) == 0U) {
			continue;
		}

		/* Look for dash value option. */
		if (strpbrk(option, dash_opts) != &option[0] ||
		    strlen(option) > 1U)
		{
			/* Error or value in option. */
			continue;
		}

		/* Dash value is next argument so we need to skip it. */
		argc--;
		argv++;

		/* Handle missing argument */
		if (argc == 0) {
			break;
		}
	}
}

/*
 * Argument parsing is based on dig, but simplified: only one
 * QNAME/QCLASS/QTYPE tuple can be specified, and options have
 * been removed that aren't applicable to delv. The interface
 * should be familiar to dig users, however.
 */
static void
parse_args(int argc, char **argv) {
	isc_result_t result;
	isc_textregion_t tr;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	bool open_type_class = true;

	for (; argc > 0; argc--, argv++) {
		if (argv[0][0] == '@') {
			server = &argv[0][1];
		} else if (argv[0][0] == '+') {
			plus_option(&argv[0][1]);
		} else if (argv[0][0] == '-') {
			if (argc <= 1) {
				if (dash_option(&argv[0][1], NULL,
						&open_type_class))
				{
					argc--;
					argv++;
				}
			} else {
				if (dash_option(&argv[0][1], argv[1],
						&open_type_class))
				{
					argc--;
					argv++;
				}
			}
		} else {
			/*
			 * Anything which isn't an option
			 */
			if (open_type_class) {
				tr.base = argv[0];
				tr.length = strlen(argv[0]);
				result = dns_rdatatype_fromtext(
					&rdtype, (isc_textregion_t *)&tr);
				if (result == ISC_R_SUCCESS) {
					if (typeset) {
						warn("extra query type");
					}
					if (rdtype == dns_rdatatype_ixfr ||
					    rdtype == dns_rdatatype_axfr)
					{
						fatal("Transfer not supported");
					}
					qtype = rdtype;
					typeset = true;
					continue;
				}
				result = dns_rdataclass_fromtext(
					&rdclass, (isc_textregion_t *)&tr);
				if (result == ISC_R_SUCCESS) {
					if (classset) {
						warn("extra query class");
					} else if (rdclass != dns_rdataclass_in)
					{
						warn("ignoring non-IN "
						     "query class");
					}
					continue;
				}
			}

			if (curqname == NULL) {
				curqname = isc_mem_strdup(mctx, argv[0]);
			}
		}
	}

	/* check consistency */
	if (qmin && !fulltrace) {
		fatal("'+qmin' cannot be used without '+ns'");
	}

	/*
	 * If no qname or qtype specified, search for root/NS
	 * If no qtype specified, use A
	 */
	if (!typeset) {
		qtype = dns_rdatatype_a;
	}

	if (curqname == NULL) {
		qname = isc_mem_strdup(mctx, ".");

		if (!typeset) {
			qtype = dns_rdatatype_ns;
		}
	} else {
		qname = curqname;
	}
}

static isc_result_t
append_str(const char *text, int len, char **p, char *end) {
	if (len > end - *p) {
		return ISC_R_NOSPACE;
	}
	memmove(*p, text, len);
	*p += len;
	return ISC_R_SUCCESS;
}

static isc_result_t
reverse_octets(const char *in, char **p, char *end) {
	char *dot = strchr(in, '.');
	int len;
	if (dot != NULL) {
		isc_result_t result;
		result = reverse_octets(dot + 1, p, end);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		result = append_str(".", 1, p, end);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		len = (int)(dot - in);
	} else {
		len = strlen(in);
	}
	return append_str(in, len, p, end);
}

static isc_result_t
get_reverse(char *reverse, size_t len, char *value, bool strict) {
	int r;
	isc_result_t result;
	isc_netaddr_t addr;

	addr.family = AF_INET6;
	r = inet_pton(AF_INET6, value, &addr.type.in6);
	if (r > 0) {
		/* This is a valid IPv6 address. */
		dns_fixedname_t fname;
		dns_name_t *name;

		name = dns_fixedname_initname(&fname);
		result = dns_byaddr_createptrname(&addr, name);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		dns_name_format(name, reverse, (unsigned int)len);
		return ISC_R_SUCCESS;
	} else {
		/*
		 * Not a valid IPv6 address.  Assume IPv4.
		 * If 'strict' is not set, construct the
		 * in-addr.arpa name by blindly reversing
		 * octets whether or not they look like integers,
		 * so that this can be used for RFC2317 names
		 * and such.
		 */
		char *p = reverse;
		char *end = reverse + len;
		if (strict && inet_pton(AF_INET, value, &addr.type.in) != 1) {
			return DNS_R_BADDOTTEDQUAD;
		}
		result = reverse_octets(value, &p, end);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		result = append_str(".in-addr.arpa.", 15, &p, end);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
		return ISC_R_SUCCESS;
	}
}

static void
resolve_cb(dns_client_t *client, const dns_name_t *query_name,
	   dns_namelist_t *namelist, isc_result_t result) {
	char namestr[DNS_NAME_FORMATSIZE];
	dns_rdataset_t *rdataset;

	if (result != ISC_R_SUCCESS && !yaml) {
		delv_log(ISC_LOG_ERROR, "resolution failed: %s",
			 isc_result_totext(result));
	}

	if (yaml) {
		printf("type: DELV_RESULT\n");
		dns_name_format(query_name, namestr, sizeof(namestr));
		printf("query_name: %s\n", namestr);
		printf("status: %s\n", isc_result_totext(result));
		printf("records:\n");
	}

	for (dns_name_t *response_name = ISC_LIST_HEAD(*namelist);
	     response_name != NULL;
	     response_name = ISC_LIST_NEXT(response_name, link))
	{
		for (rdataset = ISC_LIST_HEAD(response_name->list);
		     rdataset != NULL; rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			printdata(rdataset, response_name);
		}
	}

	dns_client_freeresanswer(client, namelist);
	isc_mem_put(mctx, namelist, sizeof(*namelist));

	dns_client_detach(&client);

	isc_loopmgr_shutdown(loopmgr);
}

static void
run_resolve(void *arg) {
	dns_client_t *client = NULL;
	dns_namelist_t *namelist = NULL;
	unsigned int resopt;
	isc_result_t result;
	dns_name_t *query_name = NULL;

	UNUSED(arg);

	namelist = isc_mem_get(mctx, sizeof(*namelist));
	ISC_LIST_INIT(*namelist);

	/* Construct QNAME */
	CHECK(convert_name(&qfn, &query_name, qname));

	/* Set up resolution options */
	resopt = DNS_CLIENTRESOPT_NOCDFLAG;
	if (no_sigs) {
		resopt |= DNS_CLIENTRESOPT_NODNSSEC;
	}
	if (!root_validation) {
		resopt |= DNS_CLIENTRESOPT_NOVALIDATE;
	}
	if (cdflag) {
		resopt &= ~DNS_CLIENTRESOPT_NOCDFLAG;
	}
	if (use_tcp) {
		resopt |= DNS_CLIENTRESOPT_TCP;
	}

	/* Create client */
	CHECK(dns_client_create(mctx, loopmgr, netmgr, 0, tlsctx_client_cache,
				&client, srcaddr4, srcaddr6));
	dns_client_setmaxrestarts(client, restarts);

	/* Set the nameserver */
	if (server != NULL) {
		addserver(client);
	} else {
		findserver(client);
	}

	CHECK(setup_dnsseckeys(client, NULL));

	/* Perform resolution */
	CHECK(dns_client_resolve(client, query_name, dns_rdataclass_in, qtype,
				 resopt, namelist, resolve_cb));
	return;
cleanup:
	if (!yaml) {
		delv_log(ISC_LOG_ERROR, "resolution failed: %s",
			 isc_result_totext(result));
	}

	isc_mem_put(mctx, namelist, sizeof(*namelist));
	isc_loopmgr_shutdown(loopmgr);

	dns_client_detach(&client);
}

static void
shutdown_server(void) {
	if (requestmgr != NULL) {
		dns_requestmgr_shutdown(requestmgr);
		dns_requestmgr_detach(&requestmgr);
	}
	if (interfacemgr != NULL) {
		ns_interfacemgr_shutdown(interfacemgr);
		ns_interfacemgr_detach(&interfacemgr);
	}
	if (dispatch != NULL) {
		dns_dispatch_detach(&dispatch);
	}
	if (dispatchmgr != NULL) {
		dns_dispatchmgr_detach(&dispatchmgr);
	}
	if (sctx != NULL) {
		ns_server_detach(&sctx);
	}

	isc_loopmgr_shutdown(loopmgr);
}

static void
recvresponse(void *arg) {
	dns_request_t *request = (dns_request_t *)arg;
	dns_message_t *query = dns_request_getarg(request);
	isc_result_t result = dns_request_getresult(request);
	dns_message_t *response = NULL;
	dns_name_t *prev = NULL;

	if (result != ISC_R_SUCCESS) {
		fatal("request event result: %s", isc_result_totext(result));
	}

	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE,
			   &response);

	result = dns_request_getresponse(request, response,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	if (result != ISC_R_SUCCESS) {
		fatal("request response failed: %s", isc_result_totext(result));
	}
	if (response->rcode != dns_rcode_noerror) {
		result = dns_result_fromrcode(response->rcode);
		delv_log(ISC_LOG_INFO, "response code: %s",
			 isc_result_totext(result));
		goto cleanup;
	}

	for (result = dns_message_firstname(response, DNS_SECTION_ANSWER);
	     result == ISC_R_SUCCESS;
	     result = dns_message_nextname(response, DNS_SECTION_ANSWER))
	{
		dns_name_t *name = NULL;
		dns_rdataset_t *rdataset = NULL;
		dns_rdatatype_t prevtype = 0;

		dns_message_currentname(response, DNS_SECTION_ANSWER, &name);

		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link))
		{
			dns_rdataset_t rds, sigs;
			int options = 0;

			/*
			 * The response message contains the answer the
			 * resolver found, but it doesn't contain the
			 * trust status. if we're not displaying that,
			 * fine, we can just print that version.
			 */
			if (!showtrust) {
				printdata(rdataset, name);
				continue;
			}

			/*
			 * ... but if we are printing the trust status
			 * (which is the default behavior)), we'll need
			 * to retrieve a copy of the rdataset from the cache.
			 * if we do that for ever record, it will produce
			 * duplicate output, so we check here whether we've
			 * already printed this name and type.
			 */
			if (prev != NULL && dns_name_equal(prev, name)) {
				continue;
			}
			prev = name;

			if (prevtype == rdataset->type) {
				continue;
			}
			prevtype = rdataset->type;

			/* do the cache lookup */
			if (rdataset->type == dns_rdatatype_rrsig) {
				continue;
			}

			dns_rdataset_init(&rds);
			dns_rdataset_init(&sigs);

			if (cdflag) {
				options |= DNS_DBFIND_PENDINGOK;
			}
			result = dns_view_simplefind(view, name, rdataset->type,
						     0, options, false, &rds,
						     &sigs);
			if (result == ISC_R_SUCCESS) {
				printdata(&rds, name);
				dns_rdataset_disassociate(&rds);
				if (dns_rdataset_isassociated(&sigs)) {
					printdata(&sigs, name);
					dns_rdataset_disassociate(&sigs);
				}
			}
		}
	}

cleanup:
	dns_message_detach(&query);
	dns_message_detach(&response);
	dns_request_destroy(&request);

	dns_view_detach(&view);
	shutdown_server();
}

static isc_result_t
accept_cb(isc_nmhandle_t *handle, isc_result_t result, void *arg) {
	UNUSED(handle);
	UNUSED(arg);

	return result;
}

static void
sendquery(void *arg) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)arg;
	isc_sockaddr_t peer = isc_nmsocket_getaddr(sock);
	isc_result_t result;
	dns_message_t *message = NULL;
	dns_name_t *query_name = NULL, *mname = NULL;
	dns_rdataset_t *mrdataset = NULL;
	dns_rdataset_t *opt = NULL;
	dns_request_t *request = NULL;

	/* Construct query message */
	CHECK(convert_name(&qfn, &query_name, qname));

	dns_message_create(mctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER,
			   &message);
	message->opcode = dns_opcode_query;
	message->flags = DNS_MESSAGEFLAG_RD | DNS_MESSAGEFLAG_AD;
	if (cdflag) {
		message->flags |= DNS_MESSAGEFLAG_CD;
	}
	message->rdclass = dns_rdataclass_in;
	message->id = (dns_messageid_t)isc_random16();

	dns_message_gettempname(message, &mname);
	dns_message_gettemprdataset(message, &mrdataset);
	dns_name_clone(query_name, mname);
	dns_rdataset_makequestion(mrdataset, dns_rdataclass_in, qtype);
	ISC_LIST_APPEND(mname->list, mrdataset, link);
	dns_message_addname(message, mname, DNS_SECTION_QUESTION);
	mrdataset = NULL;
	mname = NULL;

	CHECK(dns_message_buildopt(message, &opt, 0, 0, DNS_MESSAGEEXTFLAG_DO,
				   NULL, 0));
	CHECK(dns_message_setopt(message, opt));

	CHECK(dns_requestmgr_create(mctx, loopmgr, dispatchmgr, NULL, NULL,
				    &requestmgr));

	dns_view_attach(view, &(dns_view_t *){ NULL });
	CHECK(dns_request_create(requestmgr, message, NULL, &peer, NULL, NULL,
				 DNS_REQUESTOPT_TCP, NULL, 1, 0, 0, isc_loop(),
				 recvresponse, message, &request));
	return;

cleanup:
	if (message != NULL) {
		dns_message_detach(&message);
	}

	shutdown_server();
}

static isc_result_t
matchview(isc_netaddr_t *srcaddr, isc_netaddr_t *destaddr,
	  dns_message_t *message, dns_aclenv_t *env, ns_server_t *lsctx,
	  isc_loop_t *loop, isc_job_cb cb, void *cbarg,
	  isc_result_t *sigresultp, isc_result_t *viewpatchresultp,
	  dns_view_t **viewp) {
	UNUSED(srcaddr);
	UNUSED(destaddr);
	UNUSED(message);
	UNUSED(env);
	UNUSED(lsctx);
	UNUSED(loop);
	UNUSED(cb);
	UNUSED(cbarg);
	UNUSED(sigresultp);

	*viewp = view;
	*viewpatchresultp = ISC_R_SUCCESS;
	return ISC_R_SUCCESS;
}

static void
run_server(void *arg) {
	isc_result_t result;
	dns_cache_t *cache = NULL;
	isc_sockaddr_t addr, any;
	struct in_addr in;

	UNUSED(arg);

	RUNTIME_CHECK(inet_pton(AF_INET, "127.0.0.1", &in));
	isc_sockaddr_fromin(&addr, &in, 0);

	ns_server_create(mctx, matchview, &sctx);

	CHECK(dns_dispatchmgr_create(mctx, loopmgr, netmgr, &dispatchmgr));
	isc_sockaddr_any(&any);
	CHECK(dns_dispatch_createudp(dispatchmgr, &any, &dispatch));
	CHECK(ns_interfacemgr_create(mctx, sctx, loopmgr, netmgr, dispatchmgr,
				     NULL, &interfacemgr));

	CHECK(dns_view_create(mctx, loopmgr, dispatchmgr, dns_rdataclass_in,
			      "_default", &view));
	CHECK(dns_cache_create(loopmgr, dns_rdataclass_in, "", mctx, &cache));
	dns_view_setcache(view, cache, false);
	dns_cache_detach(&cache);
	dns_view_setdstport(view, destport);
	dns_view_setmaxrestarts(view, restarts);

	CHECK(dns_rootns_create(mctx, dns_rdataclass_in, hintfile, &roothints));
	dns_view_sethints(view, roothints);
	dns_db_detach(&roothints);

	view->qminimization = qmin;
	view->qmin_strict = qmin_strict;

	dns_view_initsecroots(view);
	CHECK(setup_dnsseckeys(NULL, view));

	CHECK(dns_view_createresolver(view, netmgr, 0, tlsctx_client_cache,
				      dispatch, NULL));
	dns_resolver_setmaxqueries(view->resolver, maxqueries);

	isc_stats_create(mctx, &resstats, dns_resstatscounter_max);
	dns_resolver_setstats(view->resolver, resstats);
	isc_stats_detach(&resstats);

	dns_rdatatypestats_create(mctx, &resquerystats);
	dns_resolver_setquerystats(view->resolver, resquerystats);
	dns_stats_detach(&resquerystats);

	dns_view_freeze(view);

	ns_interface_create(interfacemgr, &addr, NULL, &ifp);

	CHECK(isc_nm_listenstreamdns(netmgr, ISC_NM_LISTEN_ONE, &addr,
				     ns_client_request, ifp, accept_cb, ifp, 10,
				     NULL, NULL, ISC_NM_PROXY_NONE,
				     &ifp->tcplistensocket));
	ifp->flags |= NS_INTERFACEFLAG_LISTENING;
	isc_async_current(sendquery, ifp->tcplistensocket);

	return;

cleanup:
	if (view != NULL) {
		dns_view_detach(&view);
	}
	shutdown_server();
}

int
main(int argc, char *argv[]) {
	isc_result_t result;
	isc_loop_t *loop = NULL;

	progname = argv[0];
	logfp = stderr;

	preparse_args(argc, argv);

	argc--;
	argv++;

	isc_managers_create(&mctx, 1, &loopmgr, &netmgr);
	loop = isc_loop_main(loopmgr);

	result = dst_lib_init(mctx, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("dst_lib_init failed: %d", result);
	}

	parse_args(argc, argv);

	CHECK(setup_style());

	setup_logging(logfp);

	if (!fulltrace && hintfile != NULL) {
		delv_log(ISC_LOG_WARNING,
			 "WARNING: not using internal name server mode, "
			 "hint file will be ignored");
	}

	if (fulltrace && server != NULL) {
		delv_log(ISC_LOG_WARNING,
			 "WARNING: using internal name server mode: "
			 "'@%s' will be ignored",
			 server);
	}

	isc_tlsctx_cache_create(mctx, &tlsctx_client_cache);

	isc_loop_setup(loop, fulltrace ? run_server : run_resolve, NULL);
	isc_loopmgr_run(loopmgr);

cleanup:
	if (tlsctx_client_cache != NULL) {
		isc_tlsctx_cache_detach(&tlsctx_client_cache);
	}
	if (trust_anchor != NULL) {
		isc_mem_free(mctx, trust_anchor);
	}
	if (anchorfile != NULL) {
		isc_mem_free(mctx, anchorfile);
	}
	if (qname != NULL) {
		isc_mem_free(mctx, qname);
	}
	if (style != NULL) {
		dns_master_styledestroy(&style, mctx);
	}

	isc_log_destroy(&lctx);
	dst_lib_destroy();

	isc_managers_destroy(&mctx, &loopmgr, &netmgr);

	return 0;
}
