/*	$NetBSD: nsupdate.c,v 1.16 2025/01/26 16:24:34 christos Exp $	*/

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

/*! \file */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/async.h>
#include <isc/attributes.h>
#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/getaddresses.h>
#include <isc/hash.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/loop.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/nonce.h>
#include <isc/parseint.h>
#include <isc/portset.h>
#include <isc/random.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/tls.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/dispatch.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/nsec3.h>
#include <dns/rcode.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/request.h>
#include <dns/tkey.h>
#include <dns/transport.h>
#include <dns/tsig.h>

#include <dst/dst.h>

#include <isccfg/namedconf.h>

#include <irs/resconf.h>

#if HAVE_GSSAPI
#include <dst/gssapi.h>

#if HAVE_KRB5_KRB5_H
#include <krb5/krb5.h>
#elif HAVE_KRB5_H
#include <krb5.h>
#endif

#if HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#elif HAVE_GSSAPI_H
#include <gssapi.h>
#endif

#endif /* HAVE_GSSAPI */

#include "../dig/readline.h"

#define MAXCMD	 (128 * 1024)
#define MAXWIRE	 (64 * 1024)
#define INITTEXT (2 * 1024)
#define MAXTEXT	 (128 * 1024)
#define TTL_MAX	 2147483647U /* Maximum signed 32 bit integer. */

#define DNSDEFAULTPORT 53

#define DEFAULT_EDNS_BUFSIZE 1232

/* Number of addresses to request from isc_getaddresses() */
#define MAX_SERVERADDRS 4

static uint16_t dnsport = DNSDEFAULTPORT;

#ifndef RESOLV_CONF
#define RESOLV_CONF "/etc/resolv.conf"
#endif /* ifndef RESOLV_CONF */

static bool debugging = false, ddebugging = false;
static bool memdebugging = false;
static bool have_ipv4 = false;
static bool have_ipv6 = false;
static bool is_dst_up = false;
static bool use_tls = false;
static bool usevc = false;
static bool usegsstsig = false;
static bool local_only = false;
static isc_nm_t *netmgr = NULL;
static isc_loopmgr_t *loopmgr = NULL;
static isc_log_t *glctx = NULL;
static isc_mem_t *gmctx = NULL;
static dns_dispatchmgr_t *dispatchmgr = NULL;
static dns_requestmgr_t *requestmgr = NULL;
static dns_dispatch_t *dispatchv4 = NULL;
static dns_dispatch_t *dispatchv6 = NULL;
static dns_message_t *updatemsg = NULL;
static dns_fixedname_t fuserzone;
static dns_fixedname_t fzname;
static dns_name_t *userzone = NULL;
static dns_name_t *zname = NULL;
static dns_name_t tmpzonename = DNS_NAME_INITEMPTY;
static dns_name_t restart_primary = DNS_NAME_INITEMPTY;
static dns_tsigkeyring_t *gssring = NULL;
static dns_tsigkey_t *tsigkey = NULL;
static dst_key_t *sig0key = NULL;
static isc_sockaddr_t *servers = NULL;
static isc_sockaddr_t *primary_servers = NULL;
static dns_transport_list_t *transport_list = NULL;
static dns_transport_t *transport = NULL;
static isc_tlsctx_cache_t *tls_ctx_cache = NULL;
static char *tls_hostname = NULL;
static char *tls_client_key_file = NULL;
static char *tls_client_cert_file = NULL;
static char *tls_ca_file = NULL;
static bool tls_always_verify_remote = true;
static bool default_servers = true;
static int ns_inuse = 0;
static int primary_inuse = 0;
static int ns_total = 0;
static int ns_alloc = 0;
static int primary_total = 0;
static int primary_alloc = 0;
static isc_sockaddr_t *localaddr4 = NULL;
static isc_sockaddr_t *localaddr6 = NULL;
static const char *keyfile = NULL;
static char *keystr = NULL;
static bool shuttingdown = false;
static FILE *input;
static bool interactive = true;
static bool seenerror = false;
static const dns_master_style_t *style;
static int requests = 0;
static unsigned int logdebuglevel = 0;
static unsigned int timeout = 300;
static unsigned int udp_timeout = 3;
static unsigned int udp_retries = 3;
static dns_rdataclass_t defaultclass = dns_rdataclass_in;
static dns_rdataclass_t zoneclass = dns_rdataclass_none;
static isc_mutex_t answer_lock;
static dns_message_t *answer = NULL;
static uint32_t default_ttl = 0;
static bool default_ttl_set = false;
static uint32_t lease = 0, keylease = 0;
static bool lease_set = false, keylease_set = false;
static bool checknames = true;
static bool checksvcb = true;
static const char *resolvconf = RESOLV_CONF;

bool done = false;

typedef struct nsu_requestinfo {
	dns_message_t *msg;
	isc_sockaddr_t *addr;
} nsu_requestinfo_t;

static void
sendrequest(isc_sockaddr_t *destaddr, dns_message_t *msg,
	    dns_request_t **request);
static void
send_update(dns_name_t *zonename, isc_sockaddr_t *primary);

static void
getinput(void *arg);

noreturn static void
fatal(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

static void
debug(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

static void
ddebug(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

#if HAVE_GSSAPI
static dns_fixedname_t fkname;
static isc_sockaddr_t *kserver = NULL;
static char *realm = NULL;
static char servicename[DNS_NAME_FORMATSIZE];
static dns_name_t *keyname;
typedef struct nsu_gssinfo {
	dns_message_t *msg;
	isc_sockaddr_t *addr;
	gss_ctx_id_t context;
} nsu_gssinfo_t;

static void
failed_gssrequest(void);
static void
start_gssrequest(dns_name_t *primary);
static void
send_gssrequest(isc_sockaddr_t *destaddr, dns_message_t *msg,
		dns_request_t **request, gss_ctx_id_t context);
static void
recvgss(void *arg);
#endif /* HAVE_GSSAPI */

static void
error(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

#define STATUS_MORE   (uint16_t)0
#define STATUS_SEND   (uint16_t)1
#define STATUS_QUIT   (uint16_t)2
#define STATUS_SYNTAX (uint16_t)3

static void
primary_from_servers(void) {
	if (primary_servers != NULL && primary_servers != servers) {
		isc_mem_cput(gmctx, primary_servers, primary_alloc,
			     sizeof(isc_sockaddr_t));
	}
	primary_servers = servers;
	primary_total = ns_total;
	primary_alloc = ns_alloc;
	primary_inuse = ns_inuse;
}

static dns_rdataclass_t
getzoneclass(void) {
	if (zoneclass == dns_rdataclass_none) {
		zoneclass = defaultclass;
	}
	return zoneclass;
}

static bool
setzoneclass(dns_rdataclass_t rdclass) {
	if (zoneclass == dns_rdataclass_none || rdclass == dns_rdataclass_none)
	{
		zoneclass = rdclass;
	}
	if (zoneclass != rdclass) {
		return false;
	}
	return true;
}

static void
fatal(const char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	_exit(EXIT_FAILURE);
}

static void
error(const char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

static void
debug(const char *format, ...) {
	va_list args;

	if (debugging) {
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}

static void
ddebug(const char *format, ...) {
	va_list args;

	if (ddebugging) {
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}

ISC_NO_SANITIZE_ADDRESS static void
check_result(isc_result_t result, const char *msg) {
	if (result != ISC_R_SUCCESS) {
		fatal("%s: %s", msg, isc_result_totext(result));
	}
}

static char *
nsu_strsep(char **stringp, const char *delim) {
	char *string = *stringp;
	*stringp = NULL;
	char *s;
	const char *d;
	char sc, dc;

	if (string == NULL) {
		return NULL;
	}

	for (; *string != '\0'; string++) {
		sc = *string;
		for (d = delim; (dc = *d) != '\0'; d++) {
			if (sc == dc) {
				break;
			}
		}
		if (dc == 0) {
			break;
		}
	}

	for (s = string; *s != '\0'; s++) {
		sc = *s;
		for (d = delim; (dc = *d) != '\0'; d++) {
			if (sc == dc) {
				*s++ = '\0';
				*stringp = s;
				return string;
			}
		}
	}
	return string;
}

static void
reset_system(void) {
	ddebug("reset_system()");
	/* If the update message is still around, destroy it */
	if (updatemsg != NULL) {
		dns_message_reset(updatemsg, DNS_MESSAGE_INTENTRENDER);
	} else {
		dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER,
				   &updatemsg);
	}
	updatemsg->opcode = dns_opcode_update;
	if (usegsstsig) {
		if (tsigkey != NULL) {
			dns_tsigkey_detach(&tsigkey);
		}
		if (gssring != NULL) {
			dns_tsigkeyring_detach(&gssring);
		}
	}
}

static bool
parse_hmac(const char *hmacstr, size_t len, dst_algorithm_t *hmac_alg,
	   uint16_t *digestbitsp) {
	uint16_t digestbits = 0;
	isc_result_t result;
	char buf[20];

	REQUIRE(hmac_alg != NULL);
	REQUIRE(hmacstr != NULL);

	if (len >= sizeof(buf)) {
		error("unknown key type '%.*s'", (int)(len), hmacstr);
		return false;
	}

	/* Copy len bytes and NUL terminate. */
	strlcpy(buf, hmacstr, ISC_MIN(len + 1, sizeof(buf)));

	if (strcasecmp(buf, "hmac-md5") == 0) {
		*hmac_alg = DST_ALG_HMACMD5;
	} else if (strncasecmp(buf, "hmac-md5-", 9) == 0) {
		*hmac_alg = DST_ALG_HMACMD5;
		result = isc_parse_uint16(&digestbits, &buf[9], 10);
		if (result != ISC_R_SUCCESS || digestbits > 128) {
			error("digest-bits out of range [0..128]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else if (strcasecmp(buf, "hmac-sha1") == 0) {
		*hmac_alg = DST_ALG_HMACSHA1;
	} else if (strncasecmp(buf, "hmac-sha1-", 10) == 0) {
		*hmac_alg = DST_ALG_HMACSHA1;
		result = isc_parse_uint16(&digestbits, &buf[10], 10);
		if (result != ISC_R_SUCCESS || digestbits > 160) {
			error("digest-bits out of range [0..160]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else if (strcasecmp(buf, "hmac-sha224") == 0) {
		*hmac_alg = DST_ALG_HMACSHA224;
	} else if (strncasecmp(buf, "hmac-sha224-", 12) == 0) {
		*hmac_alg = DST_ALG_HMACSHA224;
		result = isc_parse_uint16(&digestbits, &buf[12], 10);
		if (result != ISC_R_SUCCESS || digestbits > 224) {
			error("digest-bits out of range [0..224]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else if (strcasecmp(buf, "hmac-sha256") == 0) {
		*hmac_alg = DST_ALG_HMACSHA256;
	} else if (strncasecmp(buf, "hmac-sha256-", 12) == 0) {
		*hmac_alg = DST_ALG_HMACSHA256;
		result = isc_parse_uint16(&digestbits, &buf[12], 10);
		if (result != ISC_R_SUCCESS || digestbits > 256) {
			error("digest-bits out of range [0..256]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else if (strcasecmp(buf, "hmac-sha384") == 0) {
		*hmac_alg = DST_ALG_HMACSHA384;
	} else if (strncasecmp(buf, "hmac-sha384-", 12) == 0) {
		*hmac_alg = DST_ALG_HMACSHA384;
		result = isc_parse_uint16(&digestbits, &buf[12], 10);
		if (result != ISC_R_SUCCESS || digestbits > 384) {
			error("digest-bits out of range [0..384]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else if (strcasecmp(buf, "hmac-sha512") == 0) {
		*hmac_alg = DST_ALG_HMACSHA512;
	} else if (strncasecmp(buf, "hmac-sha512-", 12) == 0) {
		*hmac_alg = DST_ALG_HMACSHA512;
		result = isc_parse_uint16(&digestbits, &buf[12], 10);
		if (result != ISC_R_SUCCESS || digestbits > 512) {
			error("digest-bits out of range [0..512]");
			return false;
		}
		*digestbitsp = (digestbits + 7) & ~0x7U;
	} else {
		error("unknown key type '%s'", buf);
		return false;
	}
	return true;
}

static int
basenamelen(const char *file) {
	int len = strlen(file);

	if (len > 1 && file[len - 1] == '.') {
		len -= 1;
	} else if (len > 8 && strcmp(file + len - 8, ".private") == 0) {
		len -= 8;
	} else if (len > 4 && strcmp(file + len - 4, ".key") == 0) {
		len -= 4;
	}
	return len;
}

static void
setup_keystr(void) {
	unsigned char *secret = NULL;
	int secretlen;
	isc_buffer_t secretbuf;
	isc_result_t result;
	isc_buffer_t keynamesrc;
	char *secretstr = NULL;
	char *s = NULL, *n = NULL;
	dns_fixedname_t fkeyname;
	dns_name_t *mykeyname = NULL;
	char *name = NULL;
	dst_algorithm_t hmac_alg;
	uint16_t digestbits = 0;

	mykeyname = dns_fixedname_initname(&fkeyname);

	debug("Creating key...");

	s = strchr(keystr, ':');
	if (s == NULL || s == keystr || s[1] == 0) {
		fatal("key option must specify [hmac:]keyname:secret");
	}
	secretstr = s + 1;
	n = strchr(secretstr, ':');
	if (n != NULL) {
		if (n == secretstr || n[1] == 0) {
			fatal("key option must specify [hmac:]keyname:secret");
		}
		name = secretstr;
		secretstr = n + 1;
		if (!parse_hmac(keystr, s - keystr, &hmac_alg, &digestbits)) {
			exit(EXIT_FAILURE);
		}
	} else {
		hmac_alg = DST_ALG_HMACMD5;
		name = keystr;
		n = s;
	}

	isc_buffer_init(&keynamesrc, name, (unsigned int)(n - name));
	isc_buffer_add(&keynamesrc, (unsigned int)(n - name));

	debug("namefromtext");
	result = dns_name_fromtext(mykeyname, &keynamesrc, dns_rootname, 0,
				   NULL);
	check_result(result, "dns_name_fromtext");

	secretlen = strlen(secretstr) * 3 / 4;
	secret = isc_mem_allocate(gmctx, secretlen);

	isc_buffer_init(&secretbuf, secret, secretlen);
	result = isc_base64_decodestring(secretstr, &secretbuf);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not create key from %s: %s\n", keystr,
			isc_result_totext(result));
		goto failure;
	}

	secretlen = isc_buffer_usedlength(&secretbuf);

	debug("keycreate");
	result = dns_tsigkey_create(mykeyname, hmac_alg, secret, secretlen,
				    gmctx, &tsigkey);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not create key from %s: %s\n", keystr,
			isc_result_totext(result));
	} else {
		dst_key_setbits(tsigkey->key, digestbits);
	}
failure:
	if (secret != NULL) {
		isc_mem_free(gmctx, secret);
	}
}

/*
 * Get a key from a named.conf format keyfile
 */
static isc_result_t
read_sessionkey(isc_mem_t *mctx, isc_log_t *lctx) {
	cfg_parser_t *pctx = NULL;
	cfg_obj_t *sessionkey = NULL;
	const cfg_obj_t *key = NULL;
	const cfg_obj_t *secretobj = NULL;
	const cfg_obj_t *algorithmobj = NULL;
	const char *mykeyname;
	const char *secretstr;
	const char *algorithm;
	isc_result_t result;
	int len;

	if (!isc_file_exists(keyfile)) {
		return ISC_R_FILENOTFOUND;
	}

	result = cfg_parser_create(mctx, lctx, &pctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = cfg_parse_file(pctx, keyfile, &cfg_type_sessionkey,
				&sessionkey);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = cfg_map_get(sessionkey, "key", &key);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	(void)cfg_map_get(key, "secret", &secretobj);
	(void)cfg_map_get(key, "algorithm", &algorithmobj);
	if (secretobj == NULL || algorithmobj == NULL) {
		fatal("key must have algorithm and secret");
	}

	mykeyname = cfg_obj_asstring(cfg_map_getname(key));
	secretstr = cfg_obj_asstring(secretobj);
	algorithm = cfg_obj_asstring(algorithmobj);

	len = strlen(algorithm) + strlen(mykeyname) + strlen(secretstr) + 3;
	keystr = isc_mem_allocate(mctx, len);
	snprintf(keystr, len, "%s:%s:%s", algorithm, mykeyname, secretstr);
	setup_keystr();

cleanup:
	if (pctx != NULL) {
		if (sessionkey != NULL) {
			cfg_obj_destroy(pctx, &sessionkey);
		}
		cfg_parser_destroy(&pctx);
	}

	if (keystr != NULL) {
		isc_mem_free(mctx, keystr);
	}

	return result;
}

static void
setup_keyfile(isc_mem_t *mctx, isc_log_t *lctx) {
	dst_key_t *dstkey = NULL;
	isc_result_t result;
	dst_algorithm_t hmac_alg = DST_ALG_UNKNOWN;

	debug("Creating key...");

	if (sig0key != NULL) {
		dst_key_free(&sig0key);
	}

	/* Try reading the key from a K* pair */
	result = dst_key_fromnamedfile(
		keyfile, NULL, DST_TYPE_PRIVATE | DST_TYPE_KEY, mctx, &dstkey);

	/* If that didn't work, try reading it as a session.key keyfile */
	if (result != ISC_R_SUCCESS) {
		result = read_sessionkey(mctx, lctx);
		if (result == ISC_R_SUCCESS) {
			return;
		}
	}

	if (result != ISC_R_SUCCESS) {
		fprintf(stderr,
			"could not read key from %.*s.{private,key}: "
			"%s\n",
			basenamelen(keyfile), keyfile,
			isc_result_totext(result));
		return;
	}

	switch (dst_key_alg(dstkey)) {
	case DST_ALG_HMACMD5:
	case DST_ALG_HMACSHA1:
	case DST_ALG_HMACSHA224:
	case DST_ALG_HMACSHA256:
	case DST_ALG_HMACSHA384:
	case DST_ALG_HMACSHA512:
		hmac_alg = dst_key_alg(dstkey);
		break;
	default:
		dst_key_attach(dstkey, &sig0key);
		dst_key_free(&dstkey);
		return;
	}

	result = dns_tsigkey_createfromkey(dst_key_name(dstkey), hmac_alg,
					   dstkey, false, false, NULL, 0, 0,
					   mctx, &tsigkey);
	dst_key_free(&dstkey);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not create key from %s: %s\n", keyfile,
			isc_result_totext(result));
	}
}

static void
doshutdown(void) {
	/*
	 * The isc_mem_put of primary_servers must be before the
	 * isc_mem_put of servers as it sets the servers pointer
	 * to NULL.
	 */
	if (primary_servers != NULL && primary_servers != servers) {
		isc_mem_cput(gmctx, primary_servers, primary_alloc,
			     sizeof(isc_sockaddr_t));
	}

	if (servers != NULL) {
		isc_mem_cput(gmctx, servers, ns_alloc, sizeof(isc_sockaddr_t));
	}

	if (localaddr4 != NULL) {
		isc_mem_put(gmctx, localaddr4, sizeof(isc_sockaddr_t));
	}

	if (localaddr6 != NULL) {
		isc_mem_put(gmctx, localaddr6, sizeof(isc_sockaddr_t));
	}

	if (tsigkey != NULL) {
		ddebug("Freeing TSIG key");
		dns_tsigkey_detach(&tsigkey);
	}

	if (sig0key != NULL) {
		ddebug("Freeing SIG(0) key");
		dst_key_free(&sig0key);
	}

	if (updatemsg != NULL) {
		dns_message_detach(&updatemsg);
	}

	ddebug("Destroying request manager");
	dns_requestmgr_detach(&requestmgr);

	ddebug("Freeing the dispatchers");
	if (have_ipv4) {
		dns_dispatch_detach(&dispatchv4);
	}
	if (have_ipv6) {
		dns_dispatch_detach(&dispatchv6);
	}

	ddebug("Shutting down dispatch manager");
	dns_dispatchmgr_detach(&dispatchmgr);
}

static void
maybeshutdown(void) {
	/* when called from getinput, doshutdown might be already finished */
	if (requestmgr == NULL) {
		return;
	}

	ddebug("Shutting down request manager");
	dns_requestmgr_shutdown(requestmgr);

	if (requests != 0) {
		return;
	}

	doshutdown();
}

static void
shutdown_program(void *arg) {
	UNUSED(arg);

	ddebug("shutdown_program()");

	shuttingdown = true;
	maybeshutdown();
}

/*
 * Try honoring the operating system's preferred ephemeral port range.
 */
static void
set_source_ports(dns_dispatchmgr_t *manager) {
	isc_portset_t *v4portset = NULL, *v6portset = NULL;
	in_port_t udpport_low, udpport_high;
	isc_result_t result;

	result = isc_portset_create(gmctx, &v4portset);
	check_result(result, "isc_portset_create (v4)");
	result = isc_net_getudpportrange(AF_INET, &udpport_low, &udpport_high);
	check_result(result, "isc_net_getudpportrange (v4)");
	isc_portset_addrange(v4portset, udpport_low, udpport_high);

	result = isc_portset_create(gmctx, &v6portset);
	check_result(result, "isc_portset_create (v6)");
	result = isc_net_getudpportrange(AF_INET6, &udpport_low, &udpport_high);
	check_result(result, "isc_net_getudpportrange (v6)");
	isc_portset_addrange(v6portset, udpport_low, udpport_high);

	result = dns_dispatchmgr_setavailports(manager, v4portset, v6portset);
	check_result(result, "dns_dispatchmgr_setavailports");

	isc_portset_destroy(gmctx, &v4portset);
	isc_portset_destroy(gmctx, &v6portset);
}

static isc_result_t
create_name(const char *str, char *namedata, size_t len, dns_name_t *name) {
	isc_buffer_t namesrc, namebuf;

	dns_name_init(name, NULL);
	isc_buffer_constinit(&namesrc, str, strlen(str));
	isc_buffer_add(&namesrc, strlen(str));
	isc_buffer_init(&namebuf, namedata, len);

	return dns_name_fromtext(name, &namesrc, dns_rootname,
				 DNS_NAME_DOWNCASE, &namebuf);
}

static void
setup_system(void *arg ISC_ATTR_UNUSED) {
	isc_result_t result;
	isc_sockaddr_t bind_any, bind_any6;
	isc_sockaddrlist_t *nslist;
	isc_logconfig_t *logconfig = NULL;
	irs_resconf_t *resconf = NULL;
	dns_name_t tlsname;
	char namedata[DNS_NAME_FORMATSIZE + 1];

	ddebug("setup_system()");

	isc_log_create(gmctx, &glctx, &logconfig);
	isc_log_setcontext(glctx);
	dns_log_init(glctx);
	dns_log_setcontext(glctx);

	result = isc_log_usechannel(logconfig, "default_debug", NULL, NULL);
	check_result(result, "isc_log_usechannel");

	isc_log_setdebuglevel(glctx, logdebuglevel);

	result = irs_resconf_load(gmctx, resolvconf, &resconf);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		fatal("parse of %s failed", resolvconf);
	}

	nslist = irs_resconf_getnameservers(resconf);

	if (servers != NULL) {
		if (primary_servers == servers) {
			primary_servers = NULL;
		}
		isc_mem_cput(gmctx, servers, ns_alloc, sizeof(isc_sockaddr_t));
	}

	ns_inuse = 0;
	if (local_only || ISC_LIST_EMPTY(*nslist)) {
		struct in_addr in;
		struct in6_addr in6;

		if (local_only && keyfile == NULL) {
			keyfile = SESSION_KEYFILE;
		}

		default_servers = !local_only;

		ns_total = ns_alloc = (have_ipv4 ? 1 : 0) + (have_ipv6 ? 1 : 0);
		servers = isc_mem_cget(gmctx, ns_alloc, sizeof(isc_sockaddr_t));

		if (have_ipv6) {
			memset(&in6, 0, sizeof(in6));
			in6.s6_addr[15] = 1;
			isc_sockaddr_fromin6(&servers[0], &in6, dnsport);
		}
		if (have_ipv4) {
			in.s_addr = htonl(INADDR_LOOPBACK);
			isc_sockaddr_fromin(&servers[(have_ipv6 ? 1 : 0)], &in,
					    dnsport);
		}
	} else {
		isc_sockaddr_t *sa;
		int i;

		/*
		 * Count the nameservers (skipping any that we can't use
		 * because of address family restrictions) and allocate
		 * the servers array.
		 */
		ns_total = 0;
		for (sa = ISC_LIST_HEAD(*nslist); sa != NULL;
		     sa = ISC_LIST_NEXT(sa, link))
		{
			switch (sa->type.sa.sa_family) {
			case AF_INET:
				if (have_ipv4) {
					ns_total++;
				}
				break;
			case AF_INET6:
				if (have_ipv6) {
					ns_total++;
				}
				break;
			default:
				fatal("bad family");
			}
		}

		ns_alloc = ns_total;
		servers = isc_mem_cget(gmctx, ns_alloc, sizeof(isc_sockaddr_t));

		i = 0;
		for (sa = ISC_LIST_HEAD(*nslist); sa != NULL;
		     sa = ISC_LIST_NEXT(sa, link))
		{
			switch (sa->type.sa.sa_family) {
			case AF_INET:
				if (have_ipv4) {
					sa->type.sin.sin_port = htons(dnsport);
				} else {
					continue;
				}
				break;
			case AF_INET6:
				if (have_ipv6) {
					sa->type.sin6.sin6_port =
						htons(dnsport);
				} else {
					continue;
				}
				break;
			default:
				fatal("bad family");
			}
			INSIST(i < ns_alloc);
			servers[i++] = *sa;
		}
	}

	irs_resconf_destroy(&resconf);

	result = dns_dispatchmgr_create(gmctx, loopmgr, netmgr, &dispatchmgr);
	check_result(result, "dns_dispatchmgr_create");

	result = dst_lib_init(gmctx, NULL);
	check_result(result, "dst_lib_init");
	is_dst_up = true;

	set_source_ports(dispatchmgr);

	if (have_ipv6) {
		isc_sockaddr_any6(&bind_any6);
		result = dns_dispatch_createudp(dispatchmgr, &bind_any6,
						&dispatchv6);
		check_result(result, "dns_dispatch_createudp (v6)");
	}

	if (have_ipv4) {
		isc_sockaddr_any(&bind_any);
		result = dns_dispatch_createudp(dispatchmgr, &bind_any,
						&dispatchv4);
		check_result(result, "dns_dispatch_createudp (v4)");
	}
	transport_list = dns_transport_list_new(gmctx);

	isc_tlsctx_cache_create(gmctx, &tls_ctx_cache);

	if (tls_client_key_file == NULL) {
		result = create_name("tls-non-auth-client", namedata,
				     sizeof(namedata), &tlsname);
		check_result(result, "create_name (tls-non-auth-client)");
		transport = dns_transport_new(&tlsname, DNS_TRANSPORT_TLS,
					      transport_list);
		dns_transport_set_tlsname(transport, "tls-non-auth-client");
	} else {
		result = create_name("tls-auth-client", namedata,
				     sizeof(namedata), &tlsname);
		check_result(result, "create_name (tls-auth-client)");
		transport = dns_transport_new(&tlsname, DNS_TRANSPORT_TLS,
					      transport_list);
		dns_transport_set_tlsname(transport, "tls-auth-client");
		dns_transport_set_keyfile(transport, tls_client_key_file);
		dns_transport_set_certfile(transport, tls_client_cert_file);
	}
	dns_transport_set_cafile(transport, tls_ca_file);
	dns_transport_set_remote_hostname(transport, tls_hostname);
	dns_transport_set_always_verify_remote(transport,
					       tls_always_verify_remote);

	result = dns_requestmgr_create(gmctx, loopmgr, dispatchmgr, dispatchv4,
				       dispatchv6, &requestmgr);
	check_result(result, "dns_requestmgr_create");

	if (keystr != NULL) {
		setup_keystr();
	} else if (local_only) {
		result = read_sessionkey(gmctx, glctx);
		if (result != ISC_R_SUCCESS) {
			fatal("can't read key from %s: %s\n", keyfile,
			      isc_result_totext(result));
		}
	} else if (keyfile != NULL) {
		setup_keyfile(gmctx, glctx);
	}

	isc_mutex_init(&answer_lock);
}

static int
get_addresses(char *host, in_port_t port, isc_sockaddr_t *sockaddr,
	      int naddrs) {
	int count = 0;
	isc_result_t result;

	isc_loopmgr_blocking(loopmgr);
	result = isc_getaddresses(host, port, sockaddr, naddrs, &count);
	isc_loopmgr_nonblocking(loopmgr);
	if (result != ISC_R_SUCCESS) {
		error("couldn't get address for '%s': %s", host,
		      isc_result_totext(result));
	}
	return count;
}

#define PARSE_ARGS_FMT "46A:C:dDE:ghH:iK:lL:MoOk:p:Pr:R:St:Tu:vVy:"

static void
pre_parse_args(int argc, char **argv) {
	dns_rdatatype_t t;
	int ch;
	char buf[100];
	bool doexit = false;
	bool ipv4only = false, ipv6only = false;

	while ((ch = isc_commandline_parse(argc, argv, PARSE_ARGS_FMT)) != -1) {
		switch (ch) {
		case 'M': /* was -dm */
			debugging = true;
			ddebugging = true;
			memdebugging = true;
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

		case '?':
		case 'h':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					argv[0], isc_commandline_option);
			}
			fprintf(stderr, "usage: nsupdate [-CdDi] [-L level] "
					"[-l] [-g | -o | -y keyname:secret "
					"| -k keyfile] [-p port] "
					"[ -S [-K tlskeyfile] [-E tlscertfile] "
					"[-A tlscafile] [-H tlshostname] "
					"[-O] ] [-v] [-V] [-P] [-T] [-4 | -6] "
					"[filename]\n");
			exit(EXIT_FAILURE);

		case 'P':
			for (t = 0xff00; t <= 0xfffe; t++) {
				if (dns_rdatatype_ismeta(t)) {
					continue;
				}
				dns_rdatatype_format(t, buf, sizeof(buf));
				if (strncmp(buf, "TYPE", 4) != 0) {
					fprintf(stdout, "%s\n", buf);
				}
			}
			doexit = true;
			break;

		case 'T':
			for (t = 1; t <= 0xfeff; t++) {
				if (dns_rdatatype_ismeta(t)) {
					continue;
				}
				dns_rdatatype_format(t, buf, sizeof(buf));
				if (strncmp(buf, "TYPE", 4) != 0) {
					fprintf(stdout, "%s\n", buf);
				}
			}
			doexit = true;
			break;

		case 'V':
			printf("nsupdate %s\n", PACKAGE_VERSION);
			doexit = true;
			break;

		default:
			break;
		}
	}
	if (doexit) {
		exit(EXIT_SUCCESS);
	}
	isc_commandline_reset = true;
	isc_commandline_index = 1;
}

static void
parse_args(int argc, char **argv) {
	int ch;
	uint32_t i;
	isc_result_t result;
	bool force_interactive = false;

	debug("parse_args");
	while ((ch = isc_commandline_parse(argc, argv, PARSE_ARGS_FMT)) != -1) {
		switch (ch) {
		case '4':
			if (have_ipv4) {
				isc_net_disableipv6();
				have_ipv6 = false;
			} else {
				fatal("can't find IPv4 networking");
			}
			break;
		case '6':
			if (have_ipv6) {
				isc_net_disableipv4();
				have_ipv4 = false;
			} else {
				fatal("can't find IPv6 networking");
			}
			break;
		case 'A':
			use_tls = true;
			tls_ca_file = isc_commandline_argument;
			break;
		case 'C':
			resolvconf = isc_commandline_argument;
			break;
		case 'd':
			debugging = true;
			break;
		case 'D': /* was -dd */
			debugging = true;
			ddebugging = true;
			break;
		case 'E':
			use_tls = true;
			tls_client_cert_file = isc_commandline_argument;
			break;
		case 'H':
			use_tls = true;
			tls_hostname = isc_commandline_argument;
			break;
		case 'M':
			break;
		case 'i':
			force_interactive = true;
			interactive = true;
			break;
		case 'K':
			use_tls = true;
			tls_client_key_file = isc_commandline_argument;
			break;
		case 'l':
			local_only = true;
			break;
		case 'L':
			result = isc_parse_uint32(&i, isc_commandline_argument,
						  10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr,
					"bad library debug value "
					"'%s'\n",
					isc_commandline_argument);
				exit(EXIT_FAILURE);
			}
			logdebuglevel = i;
			break;
		case 'y':
			keystr = isc_commandline_argument;
			break;
		case 'v':
			usevc = true;
			break;
		case 'k':
			keyfile = isc_commandline_argument;
			break;
		case 'g':
			usegsstsig = true;
			break;
		case 'o':
			usegsstsig = true;
			break;
		case 'O':
			use_tls = true;
			tls_always_verify_remote = false;
			break;
		case 'p':
			result = isc_parse_uint16(&dnsport,
						  isc_commandline_argument, 10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr,
					"bad port number "
					"'%s'\n",
					isc_commandline_argument);
				exit(EXIT_FAILURE);
			}
			break;
		case 'S':
			use_tls = true;
			break;
		case 't':
			result = isc_parse_uint32(&timeout,
						  isc_commandline_argument, 10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "bad timeout '%s'\n",
					isc_commandline_argument);
				exit(EXIT_FAILURE);
			}
			if (timeout == 0) {
				timeout = UINT_MAX;
			}
			break;
		case 'u':
			result = isc_parse_uint32(&udp_timeout,
						  isc_commandline_argument, 10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "bad udp timeout '%s'\n",
					isc_commandline_argument);
				exit(EXIT_FAILURE);
			}
			break;
		case 'r':
			result = isc_parse_uint32(&udp_retries,
						  isc_commandline_argument, 10);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "bad udp retries '%s'\n",
					isc_commandline_argument);
				exit(EXIT_FAILURE);
			}
			break;

		case 'R':
			fatal("The -R option has been deprecated.");
			break;

		default:
			fprintf(stderr, "%s: unhandled option: %c\n", argv[0],
				isc_commandline_option);
			exit(EXIT_FAILURE);
		}
	}
	if (keyfile != NULL && keystr != NULL) {
		fprintf(stderr, "%s: cannot specify both -k and -y\n", argv[0]);
		exit(EXIT_FAILURE);
	}

#if HAVE_GSSAPI
	if (usegsstsig && (keyfile != NULL || keystr != NULL)) {
		fprintf(stderr, "%s: cannot specify -g with -k or -y\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}
#else  /* HAVE_GSSAPI */
	if (usegsstsig) {
		fprintf(stderr,
			"%s: cannot specify -g	or -o, "
			"program not linked with GSS API Library\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}
#endif /* HAVE_GSSAPI */

	if (use_tls) {
		usevc = true;
		if ((tls_client_key_file == NULL) !=
		    (tls_client_cert_file == NULL))
		{
			fprintf(stderr,
				"%s: cannot specify the -K option without"
				"the -E option, and vice versa.\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
		if (tls_ca_file != NULL && tls_always_verify_remote == false) {
			fprintf(stderr,
				"%s: cannot specify the -A option in "
				"conjuction with the -O option.\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (argv[isc_commandline_index] != NULL) {
		if (strcmp(argv[isc_commandline_index], "-") == 0) {
			input = stdin;
		} else {
			result = isc_stdio_open(argv[isc_commandline_index],
						"r", &input);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "could not open '%s': %s\n",
					argv[isc_commandline_index],
					isc_result_totext(result));
				exit(EXIT_FAILURE);
			}
		}
		if (!force_interactive) {
			interactive = false;
		}
	}
}

static uint16_t
parse_name(char **cmdlinep, dns_message_t *msg, dns_name_t **namep) {
	isc_result_t result;
	char *word;
	isc_buffer_t source;

	word = nsu_strsep(cmdlinep, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read owner name\n");
		return STATUS_SYNTAX;
	}

	dns_message_gettempname(msg, namep);
	isc_buffer_init(&source, word, strlen(word));
	isc_buffer_add(&source, strlen(word));
	result = dns_name_fromtext(*namep, &source, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		error("invalid owner name: %s", isc_result_totext(result));
		isc_buffer_invalidate(&source);
		dns_message_puttempname(msg, namep);
		return STATUS_SYNTAX;
	}
	isc_buffer_invalidate(&source);
	return STATUS_MORE;
}

static uint16_t
parse_rdata(char **cmdlinep, dns_rdataclass_t rdataclass,
	    dns_rdatatype_t rdatatype, dns_message_t *msg, dns_rdata_t *rdata) {
	char *cmdline = *cmdlinep;
	isc_buffer_t source, *buf = NULL, *newbuf = NULL;
	isc_region_t r;
	isc_lex_t *lex = NULL;
	dns_rdatacallbacks_t callbacks;
	isc_result_t result;

	if (cmdline == NULL) {
		rdata->flags = DNS_RDATA_UPDATE;
		return STATUS_MORE;
	}

	while (*cmdline != 0 && isspace((unsigned char)*cmdline)) {
		cmdline++;
	}

	if (*cmdline != 0) {
		dns_rdatacallbacks_init(&callbacks);
		isc_lex_create(gmctx, strlen(cmdline), &lex);
		isc_buffer_init(&source, cmdline, strlen(cmdline));
		isc_buffer_add(&source, strlen(cmdline));
		result = isc_lex_openbuffer(lex, &source);
		check_result(result, "isc_lex_openbuffer");
		isc_buffer_allocate(gmctx, &buf, MAXWIRE);
		result = dns_rdata_fromtext(NULL, rdataclass, rdatatype, lex,
					    dns_rootname, 0, gmctx, buf,
					    &callbacks);
		isc_lex_destroy(&lex);
		if (result == ISC_R_SUCCESS) {
			isc_buffer_usedregion(buf, &r);
			isc_buffer_allocate(gmctx, &newbuf, r.length);
			isc_buffer_putmem(newbuf, r.base, r.length);
			isc_buffer_usedregion(newbuf, &r);
			dns_rdata_fromregion(rdata, rdataclass, rdatatype, &r);
			isc_buffer_free(&buf);
			dns_message_takebuffer(msg, &newbuf);
		} else {
			fprintf(stderr, "invalid rdata format: %s\n",
				isc_result_totext(result));
			isc_buffer_free(&buf);
			return STATUS_SYNTAX;
		}
	} else {
		rdata->flags = DNS_RDATA_UPDATE;
	}
	*cmdlinep = cmdline;
	return STATUS_MORE;
}

static uint16_t
make_prereq(char *cmdline, bool ispositive, bool isrrset) {
	isc_result_t result;
	char *word;
	dns_name_t *name = NULL;
	isc_textregion_t region;
	dns_rdataset_t *rdataset = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataclass_t rdataclass;
	dns_rdatatype_t rdatatype;
	dns_rdata_t *rdata = NULL;
	uint16_t retval;

	ddebug("make_prereq()");

	/*
	 * Read the owner name
	 */
	retval = parse_name(&cmdline, updatemsg, &name);
	if (retval != STATUS_MORE) {
		return retval;
	}

	/*
	 * If this is an rrset prereq, read the class or type.
	 */
	if (isrrset) {
		word = nsu_strsep(&cmdline, " \t\r\n");
		if (word == NULL || *word == 0) {
			fprintf(stderr, "could not read class or type\n");
			goto failure;
		}
		region.base = word;
		region.length = strlen(word);
		result = dns_rdataclass_fromtext(&rdataclass, &region);
		if (result == ISC_R_SUCCESS) {
			if (!setzoneclass(rdataclass)) {
				fprintf(stderr, "class mismatch: %s\n", word);
				goto failure;
			}
			/*
			 * Now read the type.
			 */
			word = nsu_strsep(&cmdline, " \t\r\n");
			if (word == NULL || *word == 0) {
				fprintf(stderr, "could not read type\n");
				goto failure;
			}
			region.base = word;
			region.length = strlen(word);
			result = dns_rdatatype_fromtext(&rdatatype, &region);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "invalid type: %s\n", word);
				goto failure;
			}
		} else {
			rdataclass = getzoneclass();
			result = dns_rdatatype_fromtext(&rdatatype, &region);
			if (result != ISC_R_SUCCESS) {
				fprintf(stderr, "invalid type: %s\n", word);
				goto failure;
			}
		}
	} else {
		rdatatype = dns_rdatatype_any;
	}

	dns_message_gettemprdata(updatemsg, &rdata);

	dns_rdata_init(rdata);

	if (isrrset && ispositive) {
		retval = parse_rdata(&cmdline, rdataclass, rdatatype, updatemsg,
				     rdata);
		if (retval != STATUS_MORE) {
			goto failure;
		}
	} else {
		rdata->flags = DNS_RDATA_UPDATE;
	}

	dns_message_gettemprdatalist(updatemsg, &rdatalist);
	dns_message_gettemprdataset(updatemsg, &rdataset);
	rdatalist->type = rdatatype;
	if (ispositive) {
		if (isrrset && rdata->data != NULL) {
			rdatalist->rdclass = rdataclass;
		} else {
			rdatalist->rdclass = dns_rdataclass_any;
		}
	} else {
		rdatalist->rdclass = dns_rdataclass_none;
	}
	rdata->rdclass = rdatalist->rdclass;
	rdata->type = rdatatype;
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	ISC_LIST_INIT(name->list);
	ISC_LIST_APPEND(name->list, rdataset, link);
	dns_message_addname(updatemsg, name, DNS_SECTION_PREREQUISITE);
	return STATUS_MORE;

failure:
	if (name != NULL) {
		dns_message_puttempname(updatemsg, &name);
	}
	return STATUS_SYNTAX;
}

static uint16_t
evaluate_prereq(char *cmdline) {
	char *word;
	bool ispositive, isrrset;

	ddebug("evaluate_prereq()");
	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read operation code\n");
		return STATUS_SYNTAX;
	}
	if (strcasecmp(word, "nxdomain") == 0) {
		ispositive = false;
		isrrset = false;
	} else if (strcasecmp(word, "yxdomain") == 0) {
		ispositive = true;
		isrrset = false;
	} else if (strcasecmp(word, "nxrrset") == 0) {
		ispositive = false;
		isrrset = true;
	} else if (strcasecmp(word, "yxrrset") == 0) {
		ispositive = true;
		isrrset = true;
	} else {
		fprintf(stderr, "incorrect operation code: %s\n", word);
		return STATUS_SYNTAX;
	}
	return make_prereq(cmdline, ispositive, isrrset);
}

static void
updateopt(void) {
	isc_result_t result;
	dns_ednsopt_t ednsopts[1];
	unsigned char ul[8];
	unsigned int count = 0;

	if (lease_set) {
		isc_buffer_t b;
		INSIST(count < ARRAY_SIZE(ednsopts));
		ednsopts[count++] = (dns_ednsopt_t){ .code = DNS_OPT_UL,
						     .length = keylease_set ? 8
									    : 4,
						     .value = ul };

		isc_buffer_init(&b, ul, sizeof(ul));
		isc_buffer_putuint32(&b, lease);
		isc_buffer_putuint32(&b, keylease);
	}

	if (count != 0) {
		dns_rdataset_t *opt = NULL;
		result = dns_message_buildopt(updatemsg, &opt, 0,
					      DEFAULT_EDNS_BUFSIZE, 0, ednsopts,
					      count);
		check_result(result, "dns_message_buildopt");
		result = dns_message_setopt(updatemsg, opt);
		check_result(result, "dns_message_setopt");
	} else {
		result = dns_message_setopt(updatemsg, NULL);
		check_result(result, "dns_message_setopt");
	}
}

static uint16_t
evaluate_lease(char *cmdline) {
	char *word;
	isc_result_t result;
	uint32_t value1, value2;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read ttl\n");
		return STATUS_SYNTAX;
	}

	if (!strcasecmp(word, "none")) {
		lease = 0;
		lease_set = false;
		keylease = 0;
		keylease_set = false;
		updateopt();
		return STATUS_MORE;
	}

	result = isc_parse_uint32(&value1, word, 10);
	if (result != ISC_R_SUCCESS) {
		return STATUS_SYNTAX;
	}

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		lease = value1;
		lease_set = true;
		keylease = 0;
		keylease_set = false;
		updateopt();
		return STATUS_MORE;
	}

	result = isc_parse_uint32(&value2, word, 10);
	if (result != ISC_R_SUCCESS) {
		return STATUS_SYNTAX;
	}

	lease = value1;
	lease_set = true;
	keylease = value2;
	keylease_set = true;
	updateopt();

	return STATUS_MORE;
}

static uint16_t
evaluate_server(char *cmdline) {
	char *word, *server;
	long port;

	if (local_only) {
		fprintf(stderr, "cannot reset server in localhost-only mode\n");
		return STATUS_SYNTAX;
	}

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read server name\n");
		return STATUS_SYNTAX;
	}
	server = word;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		port = dnsport;
	} else {
		char *endp;
		port = strtol(word, &endp, 10);
		if (*endp != 0) {
			fprintf(stderr, "port '%s' is not numeric\n", word);
			return STATUS_SYNTAX;
		} else if (port < 1 || port > 65535) {
			fprintf(stderr,
				"port '%s' is out of range "
				"(1 to 65535)\n",
				word);
			return STATUS_SYNTAX;
		}
	}

	if (servers != NULL) {
		if (primary_servers == servers) {
			primary_servers = NULL;
		}
		isc_mem_cput(gmctx, servers, ns_alloc, sizeof(isc_sockaddr_t));
	}

	default_servers = false;

	ns_alloc = MAX_SERVERADDRS;
	ns_inuse = 0;
	servers = isc_mem_cget(gmctx, ns_alloc, sizeof(isc_sockaddr_t));
	ns_total = get_addresses(server, (in_port_t)port, servers, ns_alloc);
	if (ns_total == 0) {
		return STATUS_SYNTAX;
	}

	return STATUS_MORE;
}

static uint16_t
evaluate_local(char *cmdline) {
	char *word, *local;
	long port;
	struct in_addr in4;
	struct in6_addr in6;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read server name\n");
		return STATUS_SYNTAX;
	}
	local = word;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		port = 0;
	} else {
		char *endp;
		port = strtol(word, &endp, 10);
		if (*endp != 0) {
			fprintf(stderr, "port '%s' is not numeric\n", word);
			return STATUS_SYNTAX;
		} else if (port < 1 || port > 65535) {
			fprintf(stderr,
				"port '%s' is out of range "
				"(1 to 65535)\n",
				word);
			return STATUS_SYNTAX;
		}
	}

	if (have_ipv6 && inet_pton(AF_INET6, local, &in6) == 1) {
		if (localaddr6 == NULL) {
			localaddr6 = isc_mem_get(gmctx, sizeof(isc_sockaddr_t));
		}
		isc_sockaddr_fromin6(localaddr6, &in6, (in_port_t)port);
	} else if (have_ipv4 && inet_pton(AF_INET, local, &in4) == 1) {
		if (localaddr4 == NULL) {
			localaddr4 = isc_mem_get(gmctx, sizeof(isc_sockaddr_t));
		}
		isc_sockaddr_fromin(localaddr4, &in4, (in_port_t)port);
	} else {
		fprintf(stderr, "invalid address %s", local);
		return STATUS_SYNTAX;
	}

	return STATUS_MORE;
}

static uint16_t
evaluate_key(char *cmdline) {
	char *namestr;
	char *secretstr;
	isc_buffer_t b;
	isc_result_t result;
	dns_fixedname_t fkeyname;
	dns_name_t *mykeyname;
	int secretlen;
	unsigned char *secret = NULL;
	isc_buffer_t secretbuf;
	dst_algorithm_t hmac_alg = DST_ALG_UNKNOWN;
	uint16_t digestbits = 0;
	char *n;

	namestr = nsu_strsep(&cmdline, " \t\r\n");
	if (namestr == NULL || *namestr == 0) {
		fprintf(stderr, "could not read key name\n");
		return STATUS_SYNTAX;
	}

	mykeyname = dns_fixedname_initname(&fkeyname);

	n = strchr(namestr, ':');
	if (n != NULL) {
		if (!parse_hmac(namestr, n - namestr, &hmac_alg, &digestbits)) {
			return STATUS_SYNTAX;
		}
		namestr = n + 1;
	} else {
		hmac_alg = DST_ALG_HMACMD5;
	}

	isc_buffer_init(&b, namestr, strlen(namestr));
	isc_buffer_add(&b, strlen(namestr));
	result = dns_name_fromtext(mykeyname, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not parse key name\n");
		return STATUS_SYNTAX;
	}

	secretstr = nsu_strsep(&cmdline, "\r\n");
	if (secretstr == NULL || *secretstr == 0) {
		fprintf(stderr, "could not read key secret\n");
		return STATUS_SYNTAX;
	}
	secretlen = strlen(secretstr) * 3 / 4;
	secret = isc_mem_allocate(gmctx, secretlen);

	isc_buffer_init(&secretbuf, secret, secretlen);
	result = isc_base64_decodestring(secretstr, &secretbuf);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not create key from %s: %s\n", secretstr,
			isc_result_totext(result));
		isc_mem_free(gmctx, secret);
		return STATUS_SYNTAX;
	}
	secretlen = isc_buffer_usedlength(&secretbuf);

	if (tsigkey != NULL) {
		dns_tsigkey_detach(&tsigkey);
	}
	result = dns_tsigkey_create(mykeyname, hmac_alg, secret, secretlen,
				    gmctx, &tsigkey);
	isc_mem_free(gmctx, secret);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not create key from %s %s: %s\n",
			namestr, secretstr, isc_result_totext(result));
		return STATUS_SYNTAX;
	}
	dst_key_setbits(tsigkey->key, digestbits);
	return STATUS_MORE;
}

static uint16_t
evaluate_zone(char *cmdline) {
	char *word;
	isc_buffer_t b;
	isc_result_t result;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read zone name\n");
		return STATUS_SYNTAX;
	}

	userzone = dns_fixedname_initname(&fuserzone);
	isc_buffer_init(&b, word, strlen(word));
	isc_buffer_add(&b, strlen(word));
	result = dns_name_fromtext(userzone, &b, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		userzone = NULL; /* Lest it point to an invalid name */
		fprintf(stderr, "could not parse zone name\n");
		return STATUS_SYNTAX;
	}

	return STATUS_MORE;
}

static uint16_t
evaluate_realm(char *cmdline) {
#if HAVE_GSSAPI
	char *word;
	char buf[1024];
	int n;

	if (realm != NULL) {
		isc_mem_free(gmctx, realm);
		realm = NULL;
	}

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		return STATUS_MORE;
	}

	n = snprintf(buf, sizeof(buf), "@%s", word);
	if (n < 0 || (size_t)n >= sizeof(buf)) {
		error("realm is too long");
		return STATUS_SYNTAX;
	}
	realm = isc_mem_strdup(gmctx, buf);
	return STATUS_MORE;
#else  /* HAVE_GSSAPI */
	UNUSED(cmdline);
	return STATUS_SYNTAX;
#endif /* HAVE_GSSAPI */
}

static uint16_t
evaluate_ttl(char *cmdline) {
	char *word;
	isc_result_t result;
	uint32_t ttl;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read ttl\n");
		return STATUS_SYNTAX;
	}

	if (!strcasecmp(word, "none")) {
		default_ttl = 0;
		default_ttl_set = false;
		return STATUS_MORE;
	}

	result = isc_parse_uint32(&ttl, word, 10);
	if (result != ISC_R_SUCCESS) {
		return STATUS_SYNTAX;
	}

	if (ttl > TTL_MAX) {
		fprintf(stderr, "ttl '%s' is out of range (0 to %u)\n", word,
			TTL_MAX);
		return STATUS_SYNTAX;
	}
	default_ttl = ttl;
	default_ttl_set = true;

	return STATUS_MORE;
}

static uint16_t
evaluate_class(char *cmdline) {
	char *word;
	isc_textregion_t r;
	isc_result_t result;
	dns_rdataclass_t rdclass;

	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read class name\n");
		return STATUS_SYNTAX;
	}

	r.base = word;
	r.length = strlen(word);
	result = dns_rdataclass_fromtext(&rdclass, &r);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not parse class name: %s\n", word);
		return STATUS_SYNTAX;
	}
	switch (rdclass) {
	case dns_rdataclass_none:
	case dns_rdataclass_any:
	case dns_rdataclass_reserved0:
		fprintf(stderr, "bad default class: %s\n", word);
		return STATUS_SYNTAX;
	default:
		defaultclass = rdclass;
	}

	return STATUS_MORE;
}

static uint16_t
update_addordelete(char *cmdline, bool isdelete) {
	isc_result_t result;
	dns_name_t *name = NULL;
	uint32_t ttl;
	char *word;
	dns_rdataclass_t rdataclass;
	dns_rdatatype_t rdatatype;
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdataset_t *rdataset = NULL;
	isc_textregion_t region;
	uint16_t retval;

	ddebug("update_addordelete()");

	/*
	 * Read the owner name.
	 */
	retval = parse_name(&cmdline, updatemsg, &name);
	if (retval != STATUS_MORE) {
		return retval;
	}

	dns_message_gettemprdata(updatemsg, &rdata);

	dns_rdata_init(rdata);

	/*
	 * If this is an add, read the TTL and verify that it's in range.
	 * If it's a delete, ignore a TTL if present (for compatibility).
	 */
	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		if (!isdelete) {
			fprintf(stderr, "could not read owner ttl\n");
			goto failure;
		} else {
			ttl = 0;
			rdataclass = dns_rdataclass_any;
			rdatatype = dns_rdatatype_any;
			rdata->flags = DNS_RDATA_UPDATE;
			goto doneparsing;
		}
	}
	result = isc_parse_uint32(&ttl, word, 10);
	if (result != ISC_R_SUCCESS) {
		if (isdelete) {
			ttl = 0;
			goto parseclass;
		} else if (default_ttl_set) {
			ttl = default_ttl;
			goto parseclass;
		} else {
			fprintf(stderr, "ttl '%s': %s\n", word,
				isc_result_totext(result));
			goto failure;
		}
	}

	if (isdelete) {
		ttl = 0;
	} else if (ttl > TTL_MAX) {
		fprintf(stderr, "ttl '%s' is out of range (0 to %u)\n", word,
			TTL_MAX);
		goto failure;
	}

	/*
	 * Read the class or type.
	 */
	word = nsu_strsep(&cmdline, " \t\r\n");
parseclass:
	if (word == NULL || *word == 0) {
		if (isdelete) {
			rdataclass = dns_rdataclass_any;
			rdatatype = dns_rdatatype_any;
			rdata->flags = DNS_RDATA_UPDATE;
			goto doneparsing;
		} else {
			fprintf(stderr, "could not read class or type\n");
			goto failure;
		}
	}
	region.base = word;
	region.length = strlen(word);
	rdataclass = dns_rdataclass_any;
	result = dns_rdataclass_fromtext(&rdataclass, &region);
	if (result == ISC_R_SUCCESS && rdataclass != dns_rdataclass_any) {
		if (!setzoneclass(rdataclass)) {
			fprintf(stderr, "class mismatch: %s\n", word);
			goto failure;
		}
		/*
		 * Now read the type.
		 */
		word = nsu_strsep(&cmdline, " \t\r\n");
		if (word == NULL || *word == 0) {
			if (isdelete) {
				rdataclass = dns_rdataclass_any;
				rdatatype = dns_rdatatype_any;
				rdata->flags = DNS_RDATA_UPDATE;
				goto doneparsing;
			} else {
				fprintf(stderr, "could not read type\n");
				goto failure;
			}
		}
		region.base = word;
		region.length = strlen(word);
		result = dns_rdatatype_fromtext(&rdatatype, &region);
		if (result != ISC_R_SUCCESS) {
			fprintf(stderr, "'%s' is not a valid type: %s\n", word,
				isc_result_totext(result));
			goto failure;
		}
	} else {
		rdataclass = getzoneclass();
		result = dns_rdatatype_fromtext(&rdatatype, &region);
		if (result != ISC_R_SUCCESS) {
			fprintf(stderr,
				"'%s' is not a valid class or type: "
				"%s\n",
				word, isc_result_totext(result));
			goto failure;
		}
	}

	retval = parse_rdata(&cmdline, rdataclass, rdatatype, updatemsg, rdata);
	if (retval != STATUS_MORE) {
		goto failure;
	}

	if (isdelete) {
		if ((rdata->flags & DNS_RDATA_UPDATE) != 0) {
			rdataclass = dns_rdataclass_any;
		} else {
			rdataclass = dns_rdataclass_none;
		}
	} else {
		if ((rdata->flags & DNS_RDATA_UPDATE) != 0) {
			fprintf(stderr, "could not read rdata\n");
			goto failure;
		}
	}

	if (!isdelete && checknames) {
		dns_fixedname_t fixed;
		dns_name_t *bad;

		if (!dns_rdata_checkowner(name, rdata->rdclass, rdata->type,
					  true))
		{
			char namebuf[DNS_NAME_FORMATSIZE];

			dns_name_format(name, namebuf, sizeof(namebuf));
			fprintf(stderr, "check-names failed: bad owner '%s'\n",
				namebuf);
			goto failure;
		}

		bad = dns_fixedname_initname(&fixed);
		if (!dns_rdata_checknames(rdata, name, bad)) {
			char namebuf[DNS_NAME_FORMATSIZE];

			dns_name_format(bad, namebuf, sizeof(namebuf));
			fprintf(stderr, "check-names failed: bad name '%s'\n",
				namebuf);
			goto failure;
		}
	}

	if (!isdelete && checksvcb && rdata->type == dns_rdatatype_svcb) {
		result = dns_rdata_checksvcb(name, rdata);
		if (result != ISC_R_SUCCESS) {
			fprintf(stderr, "check-svcb failed: %s\n",
				isc_result_totext(result));
			goto failure;
		}
	}

	if (!isdelete && rdata->type == dns_rdatatype_nsec3param) {
		dns_rdata_nsec3param_t nsec3param;

		result = dns_rdata_tostruct(rdata, &nsec3param, NULL);
		check_result(result, "dns_rdata_tostruct");
		if (nsec3param.iterations > dns_nsec3_maxiterations()) {
			fprintf(stderr,
				"NSEC3PARAM has excessive iterations (> %u)\n",
				dns_nsec3_maxiterations());
			goto failure;
		}
	}

doneparsing:

	dns_message_gettemprdatalist(updatemsg, &rdatalist);
	dns_message_gettemprdataset(updatemsg, &rdataset);
	rdatalist->type = rdatatype;
	rdatalist->rdclass = rdataclass;
	rdatalist->covers = rdatatype;
	rdatalist->ttl = (dns_ttl_t)ttl;
	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	dns_rdatalist_tordataset(rdatalist, rdataset);
	ISC_LIST_INIT(name->list);
	ISC_LIST_APPEND(name->list, rdataset, link);
	dns_message_addname(updatemsg, name, DNS_SECTION_UPDATE);
	return STATUS_MORE;

failure:
	if (name != NULL) {
		dns_message_puttempname(updatemsg, &name);
	}
	dns_message_puttemprdata(updatemsg, &rdata);
	return STATUS_SYNTAX;
}

static uint16_t
evaluate_update(char *cmdline) {
	char *word;
	bool isdelete;

	ddebug("evaluate_update()");
	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read operation code\n");
		return STATUS_SYNTAX;
	}
	if (strcasecmp(word, "delete") == 0) {
		isdelete = true;
	} else if (strcasecmp(word, "del") == 0) {
		isdelete = true;
	} else if (strcasecmp(word, "add") == 0) {
		isdelete = false;
	} else {
		fprintf(stderr, "incorrect operation code: %s\n", word);
		return STATUS_SYNTAX;
	}
	return update_addordelete(cmdline, isdelete);
}

static uint16_t
evaluate_checknames(char *cmdline) {
	char *word;

	ddebug("evaluate_checknames()");
	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read check-names directive\n");
		return STATUS_SYNTAX;
	}
	if (strcasecmp(word, "yes") == 0 || strcasecmp(word, "true") == 0 ||
	    strcasecmp(word, "on") == 0)
	{
		checknames = true;
	} else if (strcasecmp(word, "no") == 0 ||
		   strcasecmp(word, "false") == 0 ||
		   strcasecmp(word, "off") == 0)
	{
		checknames = false;
	} else {
		fprintf(stderr, "incorrect check-names directive: %s\n", word);
		return STATUS_SYNTAX;
	}
	return STATUS_MORE;
}

static uint16_t
evaluate_checksvcb(char *cmdline) {
	char *word;

	ddebug("evaluate_checksvcb()");
	word = nsu_strsep(&cmdline, " \t\r\n");
	if (word == NULL || *word == 0) {
		fprintf(stderr, "could not read check-svcb directive\n");
		return STATUS_SYNTAX;
	}
	if (strcasecmp(word, "yes") == 0 || strcasecmp(word, "true") == 0 ||
	    strcasecmp(word, "on") == 0)
	{
		checksvcb = true;
	} else if (strcasecmp(word, "no") == 0 ||
		   strcasecmp(word, "false") == 0 ||
		   strcasecmp(word, "off") == 0)
	{
		checksvcb = false;
	} else {
		fprintf(stderr, "incorrect check-svcb directive: %s\n", word);
		return STATUS_SYNTAX;
	}
	return STATUS_MORE;
}

static void
setzone(dns_name_t *zonename) {
	isc_result_t result;
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;

	result = dns_message_firstname(updatemsg, DNS_SECTION_ZONE);
	if (result == ISC_R_SUCCESS) {
		dns_message_currentname(updatemsg, DNS_SECTION_ZONE, &name);
		dns_message_removename(updatemsg, name, DNS_SECTION_ZONE);
		for (rdataset = ISC_LIST_HEAD(name->list); rdataset != NULL;
		     rdataset = ISC_LIST_HEAD(name->list))
		{
			ISC_LIST_UNLINK(name->list, rdataset, link);
			dns_rdataset_disassociate(rdataset);
			dns_message_puttemprdataset(updatemsg, &rdataset);
		}
		dns_message_puttempname(updatemsg, &name);
	}

	if (zonename != NULL) {
		dns_message_gettempname(updatemsg, &name);
		dns_name_clone(zonename, name);
		dns_message_gettemprdataset(updatemsg, &rdataset);
		dns_rdataset_makequestion(rdataset, getzoneclass(),
					  dns_rdatatype_soa);
		ISC_LIST_INIT(name->list);
		ISC_LIST_APPEND(name->list, rdataset, link);
		dns_message_addname(updatemsg, name, DNS_SECTION_ZONE);
	}
}

static void
show_message(FILE *stream, dns_message_t *msg, const char *description) {
	isc_result_t result;
	isc_buffer_t *buf = NULL;
	int bufsz;

	ddebug("show_message()");

	setzone(userzone);

	bufsz = INITTEXT;
	do {
		if (bufsz > MAXTEXT) {
			fprintf(stderr, "could not allocate large enough "
					"buffer to display message\n");
			exit(EXIT_FAILURE);
		}
		if (buf != NULL) {
			isc_buffer_free(&buf);
		}
		isc_buffer_allocate(gmctx, &buf, bufsz);
		result = dns_message_totext(msg, style, 0, buf);
		bufsz *= 2;
	} while (result == ISC_R_NOSPACE);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "could not convert message to text format.\n");
		isc_buffer_free(&buf);
		return;
	}
	fprintf(stream, "%s\n%.*s", description,
		(int)isc_buffer_usedlength(buf), (char *)isc_buffer_base(buf));
	fflush(stream);
	isc_buffer_free(&buf);
}

static uint16_t
do_next_command(char *cmdline) {
	char *word;

	ddebug("do_next_command()");
	word = nsu_strsep(&cmdline, " \t\r\n");

	if (word == NULL || *word == 0) {
		return STATUS_SEND;
	}
	if (word[0] == ';') {
		return STATUS_MORE;
	}
	if (strcasecmp(word, "quit") == 0) {
		return STATUS_QUIT;
	}
	if (strcasecmp(word, "prereq") == 0) {
		return evaluate_prereq(cmdline);
	}
	if (strcasecmp(word, "nxdomain") == 0) {
		return make_prereq(cmdline, false, false);
	}
	if (strcasecmp(word, "yxdomain") == 0) {
		return make_prereq(cmdline, true, false);
	}
	if (strcasecmp(word, "nxrrset") == 0) {
		return make_prereq(cmdline, false, true);
	}
	if (strcasecmp(word, "yxrrset") == 0) {
		return make_prereq(cmdline, true, true);
	}
	if (strcasecmp(word, "update") == 0) {
		return evaluate_update(cmdline);
	}
	if (strcasecmp(word, "delete") == 0) {
		return update_addordelete(cmdline, true);
	}
	if (strcasecmp(word, "del") == 0) {
		return update_addordelete(cmdline, true);
	}
	if (strcasecmp(word, "add") == 0) {
		return update_addordelete(cmdline, false);
	}
	if (strcasecmp(word, "lease") == 0) {
		return evaluate_lease(cmdline);
	}
	if (strcasecmp(word, "server") == 0) {
		return evaluate_server(cmdline);
	}
	if (strcasecmp(word, "local") == 0) {
		return evaluate_local(cmdline);
	}
	if (strcasecmp(word, "zone") == 0) {
		return evaluate_zone(cmdline);
	}
	if (strcasecmp(word, "class") == 0) {
		return evaluate_class(cmdline);
	}
	if (strcasecmp(word, "send") == 0) {
		return STATUS_SEND;
	}
	if (strcasecmp(word, "debug") == 0) {
		if (debugging) {
			ddebugging = true;
		} else {
			debugging = true;
		}
		return STATUS_MORE;
	}
	if (strcasecmp(word, "ttl") == 0) {
		return evaluate_ttl(cmdline);
	}
	if (strcasecmp(word, "show") == 0) {
		show_message(stdout, updatemsg, "Outgoing update query:");
		return STATUS_MORE;
	}
	if (strcasecmp(word, "answer") == 0) {
		LOCK(&answer_lock);
		if (answer != NULL) {
			show_message(stdout, answer, "Answer:");
		}
		UNLOCK(&answer_lock);
		return STATUS_MORE;
	}
	if (strcasecmp(word, "key") == 0) {
		usegsstsig = false;
		return evaluate_key(cmdline);
	}
	if (strcasecmp(word, "realm") == 0) {
		return evaluate_realm(cmdline);
	}
	if (strcasecmp(word, "check-names") == 0 ||
	    strcasecmp(word, "checknames") == 0)
	{
		return evaluate_checknames(cmdline);
	}
	if (strcasecmp(word, "check-svcb") == 0 ||
	    strcasecmp(word, "checksvcb") == 0)
	{
		return evaluate_checksvcb(cmdline);
	}
	if (strcasecmp(word, "gsstsig") == 0) {
#if HAVE_GSSAPI
		usegsstsig = true;
#else  /* HAVE_GSSAPI */
		fprintf(stderr, "gsstsig not supported\n");
#endif /* HAVE_GSSAPI */
		return STATUS_MORE;
	}
	if (strcasecmp(word, "oldgsstsig") == 0) {
#if HAVE_GSSAPI
		usegsstsig = true;
#else  /* HAVE_GSSAPI */
		fprintf(stderr, "gsstsig not supported\n");
#endif /* HAVE_GSSAPI */
		return STATUS_MORE;
	}
	if (strcasecmp(word, "help") == 0) {
		fprintf(stdout, "nsupdate " PACKAGE_VERSION ":\n"
				"local address [port]      (set local "
				"resolver)\n"
				"server address [port]     (set primary server "
				"for zone)\n"
				"send                      (send the update "
				"request)\n"
				"show                      (show the update "
				"request)\n"
				"answer                    (show the answer to "
				"the last request)\n"
				"quit                      (quit, any pending "
				"update is not sent)\n"
				"help                      (display this "
				"message)\n"
				"key [hmac:]keyname secret (use TSIG to sign "
				"the request)\n"
				"gsstsig                   (use GSS_TSIG to "
				"sign the request)\n"
				"zone name                 (set the zone to be "
				"updated)\n"
				"class CLASS               (set the zone's DNS "
				"class, e.g. IN (default), CH)\n"
				"check-names { on | off }  (enable / disable "
				"check-names)\n"
				"[prereq] nxdomain name    (require that this "
				"name does not exist)\n"
				"[prereq] yxdomain name    (require that this "
				"name exists)\n"
				"[prereq] nxrrset ....     (require that this "
				"RRset does not exist)\n"
				"[prereq] yxrrset ....     (require that this "
				"RRset exists)\n"
				"[update] add ....         (add the given "
				"record to the zone)\n"
				"[update] del[ete] ....    (remove the given "
				"record(s) from the zone)\n");
		return STATUS_MORE;
	}
	if (strcasecmp(word, "version") == 0) {
		fprintf(stdout, "nsupdate " PACKAGE_VERSION "\n");
		return STATUS_MORE;
	}
	fprintf(stderr, "incorrect section name: %s\n", word);
	return STATUS_SYNTAX;
}

static uint16_t
get_next_command(void) {
	uint16_t result = STATUS_QUIT;
	char cmdlinebuf[MAXCMD];
	char *cmdline = NULL, *ptr = NULL;

	if (interactive) {
		cmdline = ptr = readline("> ");
		if (ptr != NULL && *ptr != 0) {
			add_history(ptr);
		}
	} else {
		cmdline = fgets(cmdlinebuf, MAXCMD, input);
	}

	if (cmdline != NULL) {
		char *tmp = cmdline;

		/*
		 * Normalize input by removing any eol as readline()
		 * removes eol but fgets doesn't.
		 */
		(void)nsu_strsep(&tmp, "\r\n");
		result = do_next_command(cmdline);
	}
	if (ptr != NULL) {
		free(ptr);
	}

	return result;
}

static bool
user_interaction(void) {
	uint16_t result = STATUS_MORE;

	ddebug("user_interaction()");
	while ((result == STATUS_MORE) || (result == STATUS_SYNTAX)) {
		result = get_next_command();
		if (!interactive && result == STATUS_SYNTAX) {
			fatal("syntax error");
		}
	}
	if (result == STATUS_SEND) {
		return true;
	}
	return false;
}

static void
done_update(void) {
	ddebug("done_update()");

	isc_async_current(getinput, NULL);
}

static void
check_tsig_error(dns_rdataset_t *rdataset, isc_buffer_t *b) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_any_tsig_t tsig;

	result = dns_rdataset_first(rdataset);
	check_result(result, "dns_rdataset_first");
	dns_rdataset_current(rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &tsig, NULL);
	check_result(result, "dns_rdata_tostruct");
	if (tsig.error != 0) {
		if (isc_buffer_remaininglength(b) < 1) {
			check_result(ISC_R_NOSPACE, "isc_buffer_"
						    "remaininglength");
		}
		isc_buffer_putstr(b, "(" /*)*/);
		result = dns_tsigrcode_totext(tsig.error, b);
		check_result(result, "dns_tsigrcode_totext");
		if (isc_buffer_remaininglength(b) < 1) {
			check_result(ISC_R_NOSPACE, "isc_buffer_"
						    "remaininglength");
		}
		isc_buffer_putstr(b, /*(*/ ")");
	}
}

static bool
next_primary(const char *caller, isc_sockaddr_t *addr, isc_result_t eresult) {
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];

	isc_sockaddr_format(addr, addrbuf, sizeof(addrbuf));
	fprintf(stderr, "; Communication with %s failed: %s\n", addrbuf,
		isc_result_totext(eresult));
	if (++primary_inuse >= primary_total) {
		return false;
	}
	ddebug("%s: trying next server", caller);
	return true;
}

static void
update_completed(void *arg) {
	dns_request_t *request = (dns_request_t *)arg;
	isc_result_t result;

	ddebug("update_completed()");

	requests--;

	if (shuttingdown) {
		dns_request_destroy(&request);
		maybeshutdown();
		return;
	}

	result = dns_request_getresult(request);
	if (result != ISC_R_SUCCESS) {
		if (!next_primary("update_completed",
				  &primary_servers[primary_inuse], result))
		{
			seenerror = true;
			goto done;
		}

		ddebug("Destroying request [%p]", request);
		dns_request_destroy(&request);
		dns_message_renderreset(updatemsg);
		dns_message_settsigkey(updatemsg, NULL);
		send_update(zname, &primary_servers[primary_inuse]);
		return;
	}

	LOCK(&answer_lock);
	dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, &answer);
	result = dns_request_getresponse(request, answer,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	switch (result) {
	case ISC_R_SUCCESS:
		if (answer->verify_attempted) {
			ddebug("tsig verification successful");
		}
		break;
	case DNS_R_CLOCKSKEW:
	case DNS_R_EXPECTEDTSIG:
	case DNS_R_TSIGERRORSET:
	case DNS_R_TSIGVERIFYFAILURE:
	case DNS_R_UNEXPECTEDTSIG:
	case ISC_R_FAILURE:
#if 0
		if (usegsstsig && answer->rcode == dns_rcode_noerror) {
			/*
			 * For MS DNS that violates RFC 2845, section 4.2
			 */
			break;
		}
#endif /* if 0 */
		fprintf(stderr, "; TSIG error with server: %s\n",
			isc_result_totext(result));
		seenerror = true;
		break;
	default:
		check_result(result, "dns_request_getresponse");
	}

	if (answer->opcode != dns_opcode_update) {
		fatal("invalid OPCODE in response to UPDATE request");
	}

	if (answer->rcode != dns_rcode_noerror) {
		seenerror = true;
		if (!debugging) {
			char buf[64];
			isc_buffer_t b;
			dns_rdataset_t *rds;

			isc_buffer_init(&b, buf, sizeof(buf) - 1);
			result = dns_rcode_totext(answer->rcode, &b);
			check_result(result, "dns_rcode_totext");
			rds = dns_message_gettsig(answer, NULL);
			if (rds != NULL) {
				check_tsig_error(rds, &b);
			}
			fprintf(stderr, "update failed: %.*s\n",
				(int)isc_buffer_usedlength(&b), buf);
		}
	}
	if (debugging) {
		show_message(stderr, answer, "\nReply from update query:");
	}
	UNLOCK(&answer_lock);

done:
	dns_request_destroy(&request);
	if (usegsstsig) {
		dns_name_free(&tmpzonename, gmctx);
		dns_name_free(&restart_primary, gmctx);
		dns_name_init(&tmpzonename, 0);
		dns_name_init(&restart_primary, 0);
	}
	done_update();
}

static void
send_update(dns_name_t *zone, isc_sockaddr_t *primary) {
	isc_result_t result;
	dns_request_t *request = NULL;
	isc_sockaddr_t *srcaddr;
	unsigned int options = DNS_REQUESTOPT_CASE | DNS_REQUESTOPT_LARGE;
	dns_transport_t *req_transport = NULL;
	isc_tlsctx_cache_t *req_tls_ctx_cache = NULL;

	ddebug("send_update()");

	setzone(zone);

	if (usevc) {
		options |= DNS_REQUESTOPT_TCP;
		if (use_tls) {
			req_transport = transport;
			req_tls_ctx_cache = tls_ctx_cache;
		}
	}

	if (tsigkey == NULL && sig0key != NULL) {
		result = dns_message_setsig0key(updatemsg, sig0key);
		check_result(result, "dns_message_setsig0key");
	}
	if (debugging) {
		char addrbuf[ISC_SOCKADDR_FORMATSIZE];

		isc_sockaddr_format(primary, addrbuf, sizeof(addrbuf));
		fprintf(stderr, "Sending update to %s\n", addrbuf);
	}

	if (isc_sockaddr_pf(primary) == AF_INET6) {
		srcaddr = localaddr6;
	} else {
		srcaddr = localaddr4;
	}

	/* Windows doesn't like the tsig name to be compressed. */
	if (updatemsg->tsigname) {
		updatemsg->tsigname->attributes.nocompress = true;
	}

	result = dns_request_create(requestmgr, updatemsg, srcaddr, primary,
				    req_transport, req_tls_ctx_cache, options,
				    tsigkey, timeout, udp_timeout, udp_retries,
				    isc_loop_main(loopmgr), update_completed,
				    NULL, &request);
	check_result(result, "dns_request_create");

	if (debugging) {
		show_message(stdout, updatemsg, "Outgoing update query:");
	}

	requests++;
}

static void
next_server(const char *caller, isc_sockaddr_t *addr, isc_result_t eresult) {
	char addrbuf[ISC_SOCKADDR_FORMATSIZE];

	isc_sockaddr_format(addr, addrbuf, sizeof(addrbuf));
	fprintf(stderr, "; Communication with %s failed: %s\n", addrbuf,
		isc_result_totext(eresult));
	if (++ns_inuse >= ns_total) {
		fatal("could not reach any name server");
	} else {
		ddebug("%s: trying next server", caller);
	}
}

static void
recvsoa(void *arg) {
	dns_request_t *request = (dns_request_t *)arg;
	isc_result_t result, eresult = dns_request_getresult(request);
	nsu_requestinfo_t *reqinfo = dns_request_getarg(request);
	dns_message_t *soaquery = reqinfo->msg;
	dns_message_t *rcvmsg = NULL;
	dns_section_t section;
	dns_name_t *name = NULL;
	dns_rdataset_t *soaset = NULL;
	dns_rdata_soa_t soa;
	dns_rdata_t soarr = DNS_RDATA_INIT;
	int pass = 0;
	dns_name_t primary;
	isc_sockaddr_t *addr = reqinfo->addr;
	isc_sockaddr_t *srcaddr = NULL;
	bool seencname = false;
	dns_name_t tname;
	unsigned int nlabels;

	ddebug("recvsoa()");

	requests--;

	if (shuttingdown) {
		dns_request_destroy(&request);
		dns_message_detach(&soaquery);
		isc_mem_put(gmctx, reqinfo, sizeof(nsu_requestinfo_t));
		maybeshutdown();
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		next_server("recvsoa", addr, eresult);
		ddebug("Destroying request [%p]", request);
		dns_request_destroy(&request);
		dns_message_renderreset(soaquery);
		dns_message_settsigkey(soaquery, NULL);
		sendrequest(&servers[ns_inuse], soaquery, &request);
		isc_mem_put(gmctx, reqinfo, sizeof(nsu_requestinfo_t));
		setzoneclass(dns_rdataclass_none);
		return;
	}

	isc_mem_put(gmctx, reqinfo, sizeof(nsu_requestinfo_t));
	reqinfo = NULL;

	ddebug("About to create rcvmsg");
	dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, &rcvmsg);
	result = dns_request_getresponse(request, rcvmsg,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	if (result == DNS_R_TSIGERRORSET && servers != NULL) {
		unsigned int options = DNS_REQUESTOPT_CASE;
		dns_transport_t *req_transport = NULL;
		isc_tlsctx_cache_t *req_tls_ctx_cache = NULL;

		dns_message_detach(&rcvmsg);
		ddebug("Destroying request [%p]", request);
		dns_request_destroy(&request);
		reqinfo = isc_mem_get(gmctx, sizeof(nsu_requestinfo_t));
		reqinfo->msg = soaquery;
		reqinfo->addr = addr;
		dns_message_renderreset(soaquery);
		ddebug("retrying soa request without TSIG");

		if (!default_servers && usevc) {
			options |= DNS_REQUESTOPT_TCP;
			if (use_tls) {
				req_transport = transport;
				req_tls_ctx_cache = tls_ctx_cache;
			}
		}

		if (isc_sockaddr_pf(addr) == AF_INET6) {
			srcaddr = localaddr6;
		} else {
			srcaddr = localaddr4;
		}

		result = dns_request_create(requestmgr, soaquery, srcaddr, addr,
					    req_transport, req_tls_ctx_cache,
					    options, NULL, timeout, udp_timeout,
					    udp_retries, isc_loop_main(loopmgr),
					    recvsoa, reqinfo, &request);
		check_result(result, "dns_request_create");
		requests++;
		return;
	}
	check_result(result, "dns_request_getresponse");

	if (rcvmsg->rcode == dns_rcode_refused) {
		next_server("recvsoa", addr, DNS_R_REFUSED);
		dns_message_detach(&rcvmsg);
		dns_request_destroy(&request);
		dns_message_renderreset(soaquery);
		dns_message_settsigkey(soaquery, NULL);
		sendrequest(&servers[ns_inuse], soaquery, &request);
		return;
	}

	section = DNS_SECTION_ANSWER;
	POST(section);
	if (debugging) {
		show_message(stderr, rcvmsg, "Reply from SOA query:");
	}

	if (rcvmsg->opcode != dns_opcode_query) {
		fatal("invalid OPCODE in response to SOA query");
	}

	if (rcvmsg->rcode != dns_rcode_noerror &&
	    rcvmsg->rcode != dns_rcode_nxdomain)
	{
		fatal("response to SOA query was unsuccessful");
	}

	if (userzone != NULL && rcvmsg->rcode == dns_rcode_nxdomain) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(userzone, namebuf, sizeof(namebuf));
		error("specified zone '%s' does not exist (NXDOMAIN)", namebuf);
		dns_message_detach(&rcvmsg);
		dns_request_destroy(&request);
		dns_message_detach(&soaquery);
		ddebug("Out of recvsoa");
		seenerror = true;
		done_update();
		return;
	}

lookforsoa:
	if (pass == 0) {
		section = DNS_SECTION_ANSWER;
	} else if (pass == 1) {
		section = DNS_SECTION_AUTHORITY;
	} else {
		goto droplabel;
	}

	result = dns_message_firstname(rcvmsg, section);
	if (result != ISC_R_SUCCESS) {
		pass++;
		goto lookforsoa;
	}
	while (result == ISC_R_SUCCESS) {
		name = NULL;
		dns_message_currentname(rcvmsg, section, &name);
		soaset = NULL;
		result = dns_message_findtype(name, dns_rdatatype_soa, 0,
					      &soaset);
		if (result == ISC_R_SUCCESS) {
			break;
		}
		if (section == DNS_SECTION_ANSWER) {
			dns_rdataset_t *tset = NULL;
			if (dns_message_findtype(name, dns_rdatatype_cname, 0,
						 &tset) == ISC_R_SUCCESS ||
			    dns_message_findtype(name, dns_rdatatype_dname, 0,
						 &tset) == ISC_R_SUCCESS)
			{
				seencname = true;
				break;
			}
		}

		result = dns_message_nextname(rcvmsg, section);
	}

	if (soaset == NULL && !seencname) {
		pass++;
		goto lookforsoa;
	}

	if (seencname) {
		goto droplabel;
	}

	if (debugging) {
		char namestr[DNS_NAME_FORMATSIZE];
		dns_name_format(name, namestr, sizeof(namestr));
		fprintf(stderr, "Found zone name: %s\n", namestr);
	}

	result = dns_rdataset_first(soaset);
	check_result(result, "dns_rdataset_first");

	dns_rdata_init(&soarr);
	dns_rdataset_current(soaset, &soarr);
	result = dns_rdata_tostruct(&soarr, &soa, NULL);
	check_result(result, "dns_rdata_tostruct");

	dns_name_init(&primary, NULL);
	dns_name_clone(&soa.origin, &primary);

	if (userzone != NULL) {
		zname = userzone;
	} else {
		/*
		 * Save the zone name in case we need to try a second
		 * address.
		 */
		zname = dns_fixedname_initname(&fzname);
		dns_name_copy(name, zname);
	}

	if (debugging) {
		char namestr[DNS_NAME_FORMATSIZE];
		dns_name_format(&primary, namestr, sizeof(namestr));
		fprintf(stderr, "The primary is: %s\n", namestr);
	}

	if (default_servers) {
		char serverstr[DNS_NAME_MAXTEXT + 1];
		isc_buffer_t buf;

		isc_buffer_init(&buf, serverstr, sizeof(serverstr));
		result = dns_name_totext(&primary, DNS_NAME_OMITFINALDOT, &buf);
		check_result(result, "dns_name_totext");
		serverstr[isc_buffer_usedlength(&buf)] = 0;

		if (primary_servers != NULL && primary_servers != servers) {
			isc_mem_cput(gmctx, primary_servers, primary_alloc,
				     sizeof(isc_sockaddr_t));
		}
		primary_alloc = MAX_SERVERADDRS;
		primary_servers = isc_mem_cget(gmctx, primary_alloc,
					       sizeof(isc_sockaddr_t));
		primary_total = get_addresses(serverstr, dnsport,
					      primary_servers, primary_alloc);
		if (primary_total == 0) {
			seenerror = true;
			dns_rdata_freestruct(&soa);
			dns_message_detach(&soaquery);
			dns_request_destroy(&request);
			dns_message_detach(&rcvmsg);
			ddebug("Out of recvsoa");
			done_update();
			return;
		}
		primary_inuse = 0;
	} else {
		primary_from_servers();
	}
	dns_rdata_freestruct(&soa);

#if HAVE_GSSAPI
	if (usegsstsig) {
		dns_name_init(&tmpzonename, NULL);
		dns_name_dup(zname, gmctx, &tmpzonename);
		dns_name_init(&restart_primary, NULL);
		dns_name_dup(&primary, gmctx, &restart_primary);
		start_gssrequest(&primary);
	} else {
		send_update(zname, &primary_servers[primary_inuse]);
		setzoneclass(dns_rdataclass_none);
	}
#else  /* HAVE_GSSAPI */
	send_update(zname, &primary_servers[primary_inuse]);
	setzoneclass(dns_rdataclass_none);
#endif /* HAVE_GSSAPI */

	dns_message_detach(&soaquery);
	dns_request_destroy(&request);

out:
	dns_message_detach(&rcvmsg);
	ddebug("Out of recvsoa");
	return;

droplabel:
	result = dns_message_firstname(soaquery, DNS_SECTION_QUESTION);
	INSIST(result == ISC_R_SUCCESS);
	name = NULL;
	dns_message_currentname(soaquery, DNS_SECTION_QUESTION, &name);
	nlabels = dns_name_countlabels(name);
	if (nlabels == 1) {
		fatal("could not find enclosing zone");
	}
	dns_name_init(&tname, NULL);
	dns_name_getlabelsequence(name, 1, nlabels - 1, &tname);
	dns_name_clone(&tname, name);
	dns_request_destroy(&request);
	dns_message_renderreset(soaquery);
	dns_message_settsigkey(soaquery, NULL);
	sendrequest(&servers[ns_inuse], soaquery, &request);
	goto out;
}

static void
sendrequest(isc_sockaddr_t *destaddr, dns_message_t *msg,
	    dns_request_t **request) {
	isc_result_t result;
	nsu_requestinfo_t *reqinfo;
	isc_sockaddr_t *srcaddr;
	unsigned int options = DNS_REQUESTOPT_CASE;
	dns_transport_t *req_transport = NULL;
	isc_tlsctx_cache_t *req_tls_ctx_cache = NULL;

	if (!default_servers && usevc) {
		options |= DNS_REQUESTOPT_TCP;
		if (use_tls) {
			req_transport = transport;
			req_tls_ctx_cache = tls_ctx_cache;
		}
	}

	reqinfo = isc_mem_get(gmctx, sizeof(nsu_requestinfo_t));
	reqinfo->msg = msg;
	reqinfo->addr = destaddr;

	if (isc_sockaddr_pf(destaddr) == AF_INET6) {
		srcaddr = localaddr6;
	} else {
		srcaddr = localaddr4;
	}

	result = dns_request_create(
		requestmgr, msg, srcaddr, destaddr, req_transport,
		req_tls_ctx_cache, options, default_servers ? NULL : tsigkey,
		timeout, udp_timeout, udp_retries, isc_loop_main(loopmgr),
		recvsoa, reqinfo, request);
	check_result(result, "dns_request_create");
	requests++;
}

#if HAVE_GSSAPI

/*
 * Get the realm from the users kerberos ticket if possible
 */
static void
get_ticket_realm(isc_mem_t *mctx) {
	krb5_context ctx;
	krb5_error_code rc;
	krb5_ccache ccache;
	krb5_principal princ;
	char *name;
	const char *ticket_realm;

	rc = krb5_init_context(&ctx);
	if (rc != 0) {
		return;
	}

	rc = krb5_cc_default(ctx, &ccache);
	if (rc != 0) {
		krb5_free_context(ctx);
		return;
	}

	rc = krb5_cc_get_principal(ctx, ccache, &princ);
	if (rc != 0) {
		krb5_cc_close(ctx, ccache);
		krb5_free_context(ctx);
		return;
	}

	rc = krb5_unparse_name(ctx, princ, &name);
	if (rc != 0) {
		krb5_free_principal(ctx, princ);
		krb5_cc_close(ctx, ccache);
		krb5_free_context(ctx);
		return;
	}

	ticket_realm = strrchr(name, '@');
	if (ticket_realm != NULL) {
		realm = isc_mem_strdup(mctx, ticket_realm);
	}

	free(name);
	krb5_free_principal(ctx, princ);
	krb5_cc_close(ctx, ccache);
	krb5_free_context(ctx);
	if (realm != NULL && debugging) {
		fprintf(stderr, "Found realm from ticket: %s\n", realm + 1);
	}
}

static void
failed_gssrequest(void) {
	seenerror = true;

	dns_name_free(&tmpzonename, gmctx);
	dns_name_free(&restart_primary, gmctx);
	dns_name_init(&tmpzonename, NULL);
	dns_name_init(&restart_primary, NULL);

	done_update();
}

static void
start_gssrequest(dns_name_t *primary) {
	dns_gss_ctx_id_t context;
	isc_buffer_t buf;
	isc_result_t result;
	uint32_t val = 0;
	dns_message_t *rmsg = NULL;
	dns_request_t *request = NULL;
	dns_name_t *servname;
	dns_fixedname_t fname;
	char namestr[DNS_NAME_FORMATSIZE];
	char mykeystr[DNS_NAME_FORMATSIZE];
	char *err_message = NULL;

	debug("start_gssrequest");
	usevc = true;

	if (gssring != NULL) {
		dns_tsigkeyring_detach(&gssring);
	}

	dns_tsigkeyring_create(gmctx, &gssring);

	dns_name_format(primary, namestr, sizeof(namestr));
	if (kserver == NULL) {
		kserver = isc_mem_get(gmctx, sizeof(isc_sockaddr_t));
	}

	memmove(kserver, &primary_servers[primary_inuse],
		sizeof(isc_sockaddr_t));

	servname = dns_fixedname_initname(&fname);

	if (realm == NULL) {
		get_ticket_realm(gmctx);
	}

	result = snprintf(servicename, sizeof(servicename), "DNS/%s%s", namestr,
			  realm ? realm : "");
	RUNTIME_CHECK(result < sizeof(servicename));
	isc_buffer_init(&buf, servicename, strlen(servicename));
	isc_buffer_add(&buf, strlen(servicename));
	result = dns_name_fromtext(servname, &buf, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("dns_name_fromtext(servname) failed: %s",
		      isc_result_totext(result));
	}

	keyname = dns_fixedname_initname(&fkname);

	isc_nonce_buf(&val, sizeof(val));

	result = snprintf(mykeystr, sizeof(mykeystr), "%u.sig-%s", val,
			  namestr);
	RUNTIME_CHECK(result <= sizeof(mykeystr));

	isc_buffer_init(&buf, mykeystr, strlen(mykeystr));
	isc_buffer_add(&buf, strlen(mykeystr));

	result = dns_name_fromtext(keyname, &buf, dns_rootname, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		fatal("dns_name_fromtext(keyname) failed: %s",
		      isc_result_totext(result));
	}

	/* Windows doesn't recognize name compression in the key name. */
	keyname->attributes.nocompress = true;

	rmsg = NULL;
	dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER, &rmsg);

	/* Build first request. */
	context = GSS_C_NO_CONTEXT;
	result = dns_tkey_buildgssquery(rmsg, keyname, servname, 0, &context,
					gmctx, &err_message);
	if (result == ISC_R_FAILURE) {
		fprintf(stderr, "tkey query failed: %s\n",
			err_message != NULL ? err_message : "unknown error");
		goto failure;
	}
	if (result != ISC_R_SUCCESS) {
		fatal("dns_tkey_buildgssquery failed: %s",
		      isc_result_totext(result));
	}

	send_gssrequest(kserver, rmsg, &request, context);
	return;

failure:
	if (rmsg != NULL) {
		dns_message_detach(&rmsg);
	}
	if (err_message != NULL) {
		isc_mem_free(gmctx, err_message);
	}
	failed_gssrequest();
}

static void
send_gssrequest(isc_sockaddr_t *destaddr, dns_message_t *msg,
		dns_request_t **request, gss_ctx_id_t context) {
	isc_result_t result;
	nsu_gssinfo_t *reqinfo = NULL;
	isc_sockaddr_t *srcaddr = NULL;
	unsigned int options = DNS_REQUESTOPT_CASE | DNS_REQUESTOPT_TCP;
	dns_transport_t *req_transport = NULL;
	isc_tlsctx_cache_t *req_tls_ctx_cache = NULL;

	if (!default_servers && use_tls) {
		req_transport = transport;
		req_tls_ctx_cache = tls_ctx_cache;
	}

	debug("send_gssrequest");
	REQUIRE(destaddr != NULL);

	reqinfo = isc_mem_get(gmctx, sizeof(nsu_gssinfo_t));
	*reqinfo = (nsu_gssinfo_t){
		.msg = msg,
		.addr = destaddr,
		.context = context,
	};

	if (isc_sockaddr_pf(destaddr) == AF_INET6) {
		srcaddr = localaddr6;
	} else {
		srcaddr = localaddr4;
	}

	result = dns_request_create(
		requestmgr, msg, srcaddr, destaddr, req_transport,
		req_tls_ctx_cache, options, tsigkey, timeout, udp_timeout,
		udp_retries, isc_loop_main(loopmgr), recvgss, reqinfo, request);
	check_result(result, "dns_request_create");
	if (debugging) {
		show_message(stdout, msg, "Outgoing update query:");
	}
	requests++;
}

static void
recvgss(void *arg) {
	dns_request_t *request = (dns_request_t *)arg;
	nsu_gssinfo_t *reqinfo = dns_request_getarg(request);
	isc_result_t result, eresult = dns_request_getresult(request);
	dns_message_t *rcvmsg = NULL;
	dns_message_t *tsigquery = reqinfo->msg;
	dns_gss_ctx_id_t context = reqinfo->context;
	isc_sockaddr_t *addr = reqinfo->addr;
	isc_buffer_t buf;
	dns_name_t *servname = NULL;
	dns_fixedname_t fname;
	char *err_message = NULL;

	ddebug("recvgss()");

	requests--;

	if (shuttingdown) {
		dns_request_destroy(&request);
		dns_message_detach(&tsigquery);
		isc_mem_put(gmctx, reqinfo, sizeof(nsu_gssinfo_t));
		maybeshutdown();
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		ddebug("Destroying request [%p]", request);
		dns_request_destroy(&request);
		if (!next_primary("recvgss", addr, eresult)) {
			dns_message_detach(&tsigquery);
			failed_gssrequest();
		} else {
			dns_message_renderreset(tsigquery);
			memmove(kserver, &primary_servers[primary_inuse],
				sizeof(isc_sockaddr_t));
			send_gssrequest(kserver, tsigquery, &request, context);
		}
		isc_mem_put(gmctx, reqinfo, sizeof(nsu_gssinfo_t));
		return;
	}
	isc_mem_put(gmctx, reqinfo, sizeof(nsu_gssinfo_t));

	ddebug("recvgss creating rcvmsg");
	dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTPARSE, &rcvmsg);

	result = dns_request_getresponse(request, rcvmsg,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	check_result(result, "dns_request_getresponse");

	if (debugging) {
		show_message(stderr, rcvmsg,
			     "recvmsg reply from GSS-TSIG query");
	}

	if (rcvmsg->opcode != dns_opcode_query) {
		fatal("invalid OPCODE in response to GSS-TSIG query");
	}

	if (rcvmsg->rcode != dns_rcode_noerror &&
	    rcvmsg->rcode != dns_rcode_nxdomain)
	{
		char rcode[64];
		isc_buffer_t b;

		isc_buffer_init(&b, rcode, sizeof(rcode) - 1);
		result = dns_rcode_totext(rcvmsg->rcode, &b);
		check_result(result, "dns_rcode_totext");
		rcode[isc_buffer_usedlength(&b)] = 0;

		fatal("response to GSS-TSIG query was unsuccessful (%s)",
		      rcode);
	}

	servname = dns_fixedname_initname(&fname);
	isc_buffer_init(&buf, servicename, strlen(servicename));
	isc_buffer_add(&buf, strlen(servicename));
	result = dns_name_fromtext(servname, &buf, dns_rootname, 0, NULL);
	check_result(result, "dns_name_fromtext");

	result = dns_tkey_gssnegotiate(tsigquery, rcvmsg, servname, &context,
				       &tsigkey, gssring, &err_message);
	switch (result) {
	case DNS_R_CONTINUE:
		dns_message_detach(&rcvmsg);
		dns_request_destroy(&request);
		send_gssrequest(kserver, tsigquery, &request, context);
		ddebug("Out of recvgss");
		return;

	case ISC_R_SUCCESS:
		/*
		 * XXXSRA Waaay too much fun here.  There's no good
		 * reason why we need a TSIG here (the people who put
		 * it into the spec admitted at the time that it was
		 * not a security issue), and Windows clients don't
		 * seem to work if named complies with the spec and
		 * includes the gratuitous TSIG.  So we're in the
		 * bizarre situation of having to choose between
		 * complying with a useless requirement in the spec
		 * and interoperating.  This is nuts.  If we can
		 * confirm this behavior, we should ask the WG to
		 * consider removing the requirement for the
		 * gratuitous TSIG here.  For the moment, we ignore
		 * the TSIG -- this too is a spec violation, but it's
		 * the least insane thing to do.
		 */

		send_update(&tmpzonename, &primary_servers[primary_inuse]);
		setzoneclass(dns_rdataclass_none);
		break;

	default:
		fatal("dns_tkey_gssnegotiate: %s %s", isc_result_totext(result),
		      err_message != NULL ? err_message : "");
	}

	dns_request_destroy(&request);
	dns_message_detach(&tsigquery);

	dns_message_detach(&rcvmsg);
	ddebug("Out of recvgss");
}
#endif /* HAVE_GSSAPI */

static void
start_update(void) {
	isc_result_t result;
	dns_rdataset_t *rdataset = NULL;
	dns_name_t *name = NULL;
	dns_request_t *request = NULL;
	dns_message_t *soaquery = NULL;
	dns_name_t *firstname;
	dns_section_t section = DNS_SECTION_UPDATE;

	ddebug("start_update()");

	LOCK(&answer_lock);
	if (answer != NULL) {
		dns_message_detach(&answer);
	}
	UNLOCK(&answer_lock);

	/*
	 * If we have both the zone and the servers we have enough information
	 * to send the update straight away otherwise we need to discover
	 * the zone and / or the primary server.
	 */
	if (userzone != NULL && !default_servers && !usegsstsig) {
		primary_from_servers();
		send_update(userzone, &primary_servers[primary_inuse]);
		setzoneclass(dns_rdataclass_none);
		return;
	}

	dns_message_create(gmctx, NULL, NULL, DNS_MESSAGE_INTENTRENDER,
			   &soaquery);

	if (default_servers) {
		soaquery->flags |= DNS_MESSAGEFLAG_RD;
	}

	dns_message_gettempname(soaquery, &name);

	dns_message_gettemprdataset(soaquery, &rdataset);

	dns_rdataset_makequestion(rdataset, getzoneclass(), dns_rdatatype_soa);

	if (userzone != NULL) {
		dns_name_clone(userzone, name);
	} else {
		dns_rdataset_t *tmprdataset;
		result = dns_message_firstname(updatemsg, section);
		if (result == ISC_R_NOMORE) {
			section = DNS_SECTION_PREREQUISITE;
			result = dns_message_firstname(updatemsg, section);
		}
		if (result != ISC_R_SUCCESS) {
			dns_message_puttempname(soaquery, &name);
			dns_rdataset_disassociate(rdataset);
			dns_message_puttemprdataset(soaquery, &rdataset);
			dns_message_detach(&soaquery);
			done_update();
			return;
		}
		firstname = NULL;
		dns_message_currentname(updatemsg, section, &firstname);
		dns_name_clone(firstname, name);
		/*
		 * Looks to see if the first name references a DS record
		 * and if that name is not the root remove a label as DS
		 * records live in the parent zone so we need to start our
		 * search one label up.
		 */
		tmprdataset = ISC_LIST_HEAD(firstname->list);
		if (section == DNS_SECTION_UPDATE &&
		    !dns_name_equal(firstname, dns_rootname) &&
		    tmprdataset->type == dns_rdatatype_ds)
		{
			unsigned int labels = dns_name_countlabels(name);
			dns_name_getlabelsequence(name, 1, labels - 1, name);
		}
	}

	ISC_LIST_INIT(name->list);
	ISC_LIST_APPEND(name->list, rdataset, link);
	dns_message_addname(soaquery, name, DNS_SECTION_QUESTION);

	ns_inuse = 0;
	sendrequest(&servers[ns_inuse], soaquery, &request);
}

static void
cleanup(void) {
	ddebug("cleanup()");

	if (tls_ctx_cache != NULL) {
		isc_tlsctx_cache_detach(&tls_ctx_cache);
	}

	if (transport_list != NULL) {
		dns_transport_list_detach(&transport_list);
	}

	LOCK(&answer_lock);
	if (answer != NULL) {
		dns_message_detach(&answer);
	}
	UNLOCK(&answer_lock);

#if HAVE_GSSAPI
	if (tsigkey != NULL) {
		ddebug("detach tsigkey x%p", tsigkey);
		dns_tsigkey_detach(&tsigkey);
	}
	if (gssring != NULL) {
		ddebug("Detaching GSS-TSIG keyring");
		dns_tsigkeyring_detach(&gssring);
	}
#endif /* ifdef HAVE_GSSAPI */

	if (sig0key != NULL) {
		dst_key_free(&sig0key);
	}

#ifdef HAVE_GSSAPI
	if (kserver != NULL) {
		isc_mem_put(gmctx, kserver, sizeof(isc_sockaddr_t));
		kserver = NULL;
	}
	if (realm != NULL) {
		isc_mem_free(gmctx, realm);
		realm = NULL;
	}
	if (dns_name_dynamic(&tmpzonename)) {
		dns_name_free(&tmpzonename, gmctx);
	}
	if (dns_name_dynamic(&restart_primary)) {
		dns_name_free(&restart_primary, gmctx);
	}
#endif /* ifdef HAVE_GSSAPI */

	ddebug("Removing log context");
	isc_log_destroy(&glctx);

	ddebug("Destroying memory context");
	if (memdebugging) {
		isc_mem_stats(gmctx, stderr);
	}

	isc_mutex_destroy(&answer_lock);

	if (is_dst_up) {
		ddebug("Destroy DST lib");
		dst_lib_destroy();
		is_dst_up = false;
	}

	ddebug("Shutting down managers");
	isc_managers_destroy(&gmctx, &loopmgr, &netmgr);
}

static void
getinput(void *arg) {
	bool more;

	UNUSED(arg);

	if (shuttingdown) {
		maybeshutdown();
		return;
	}

	reset_system();
	isc_loopmgr_blocking(loopmgr);
	more = user_interaction();
	isc_loopmgr_nonblocking(loopmgr);
	if (!more) {
		isc_loopmgr_shutdown(loopmgr);
		return;
	}

	done = false;
	start_update();
}

int
main(int argc, char **argv) {
	uint32_t timeoutms;

	style = &dns_master_style_debug;

	input = stdin;

	interactive = isatty(0);

	if (isc_net_probeipv4() == ISC_R_SUCCESS) {
		have_ipv4 = true;
	}
	if (isc_net_probeipv6() == ISC_R_SUCCESS) {
		have_ipv6 = true;
	}
	if (!have_ipv4 && !have_ipv6) {
		fatal("could not find either IPv4 or IPv6");
	}

	pre_parse_args(argc, argv);

	isc_managers_create(&gmctx, 1, &loopmgr, &netmgr);

	parse_args(argc, argv);

	/* Set the network manager timeouts in milliseconds. */
	timeoutms = timeout * 1000;
	isc_nm_settimeouts(netmgr, timeoutms, timeoutms, timeoutms, timeoutms);

	isc_loopmgr_setup(loopmgr, setup_system, NULL);
	isc_loopmgr_setup(loopmgr, getinput, NULL);
	isc_loopmgr_teardown(loopmgr, shutdown_program, NULL);
	isc_loopmgr_run(loopmgr);

	cleanup();

	if (seenerror) {
		return 2;
	}

	return 0;
}
