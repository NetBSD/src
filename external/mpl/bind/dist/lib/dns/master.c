/*	$NetBSD: master.c,v 1.10 2023/01/25 21:43:30 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>

#include <isc/atomic.h>
#include <isc/event.h>
#include <isc/lex.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/serial.h>
#include <isc/stdio.h>
#include <isc/stdtime.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/events.h>
#include <dns/fixedname.h>
#include <dns/master.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/soa.h>
#include <dns/time.h>
#include <dns/ttl.h>

/*!
 * Grow the number of dns_rdatalist_t (#RDLSZ) and dns_rdata_t (#RDSZ)
 * structures by these sizes when we need to.
 *
 */
/*% RDLSZ reflects the number of different types with the same name expected. */
#define RDLSZ 32
/*%
 * RDSZ reflects the number of rdata expected at a give name that can fit into
 * 64k.
 */
#define RDSZ 512

#define NBUFS	  4
#define MAXWIRESZ 255

/*%
 * Target buffer size and minimum target size.
 * MINTSIZ must be big enough to hold the largest rdata record.
 * \brief
 * TSIZ >= MINTSIZ
 */
#define TSIZ (128 * 1024)
/*%
 * max message size - header - root - type - class - ttl - rdlen
 */
#define MINTSIZ DNS_RDATA_MAXLENGTH
/*%
 * Size for tokens in the presentation format,
 * The largest tokens are the base64 blocks in KEY and CERT records,
 * Largest key allowed is about 1372 bytes but
 * there is no fixed upper bound on CERT records.
 * 2K is too small for some X.509s, 8K is overkill.
 */
#define TOKENSIZ (8 * 1024)

/*%
 * Buffers sizes for $GENERATE.
 */
#define DNS_MASTER_LHS 2048
#define DNS_MASTER_RHS MINTSIZ

#define CHECKNAMESFAIL(x) (((x)&DNS_MASTER_CHECKNAMESFAIL) != 0)

typedef ISC_LIST(dns_rdatalist_t) rdatalist_head_t;

typedef struct dns_incctx dns_incctx_t;

/*%
 * Master file load state.
 */

struct dns_loadctx {
	unsigned int magic;
	isc_mem_t *mctx;
	dns_masterformat_t format;

	dns_rdatacallbacks_t *callbacks;
	isc_task_t *task;
	dns_loaddonefunc_t done;
	void *done_arg;

	/* Common methods */
	isc_result_t (*openfile)(dns_loadctx_t *lctx, const char *filename);
	isc_result_t (*load)(dns_loadctx_t *lctx);

	/* Members used by all formats */
	uint32_t maxttl;

	/* Members specific to the text format: */
	isc_lex_t *lex;
	bool keep_lex;
	unsigned int options;
	bool ttl_known;
	bool default_ttl_known;
	bool warn_1035;
	bool warn_tcr;
	bool warn_sigexpired;
	bool seen_include;
	uint32_t ttl;
	uint32_t default_ttl;
	dns_rdataclass_t zclass;
	dns_fixedname_t fixed_top;
	dns_name_t *top; /*%< top of zone */

	/* Members specific to the raw format: */
	FILE *f;
	bool first;
	dns_masterrawheader_t header;

	/* Which fixed buffers we are using? */
	unsigned int loop_cnt; /*% records per quantum,
				* 0 => all. */
	isc_result_t result;

	/* Atomic */
	isc_refcount_t references;
	atomic_bool canceled;

	/* locked by lock */
	dns_incctx_t *inc;
	uint32_t resign;
	isc_stdtime_t now;

	dns_masterincludecb_t include_cb;
	void *include_arg;
};

struct dns_incctx {
	dns_incctx_t *parent;
	dns_name_t *origin;
	dns_name_t *current;
	dns_name_t *glue;
	dns_fixedname_t fixed[NBUFS]; /* working buffers */
	unsigned int in_use[NBUFS];   /* covert to bitmap? */
	int glue_in_use;
	int current_in_use;
	int origin_in_use;
	bool origin_changed;
	bool drop;
	unsigned int glue_line;
	unsigned int current_line;
};

#define DNS_LCTX_MAGIC	     ISC_MAGIC('L', 'c', 't', 'x')
#define DNS_LCTX_VALID(lctx) ISC_MAGIC_VALID(lctx, DNS_LCTX_MAGIC)

#define DNS_AS_STR(t) ((t).value.as_textregion.base)

static isc_result_t
openfile_text(dns_loadctx_t *lctx, const char *master_file);

static isc_result_t
load_text(dns_loadctx_t *lctx);

static isc_result_t
openfile_raw(dns_loadctx_t *lctx, const char *master_file);

static isc_result_t
load_raw(dns_loadctx_t *lctx);

static isc_result_t
openfile_map(dns_loadctx_t *lctx, const char *master_file);

static isc_result_t
load_map(dns_loadctx_t *lctx);

static isc_result_t
pushfile(const char *master_file, dns_name_t *origin, dns_loadctx_t *lctx);

static isc_result_t
commit(dns_rdatacallbacks_t *, dns_loadctx_t *, rdatalist_head_t *,
       dns_name_t *, const char *, unsigned int);

static bool
is_glue(rdatalist_head_t *, dns_name_t *);

static dns_rdatalist_t *
grow_rdatalist(int, dns_rdatalist_t *, int, rdatalist_head_t *,
	       rdatalist_head_t *, isc_mem_t *mctx);

static dns_rdata_t *
grow_rdata(int, dns_rdata_t *, int, rdatalist_head_t *, rdatalist_head_t *,
	   isc_mem_t *);

static void
load_quantum(isc_task_t *task, isc_event_t *event);

static isc_result_t
task_send(dns_loadctx_t *lctx);

static void
loadctx_destroy(dns_loadctx_t *lctx);

#define GETTOKENERR(lexer, options, token, eol, err)                      \
	do {                                                              \
		result = gettoken(lexer, options, token, eol, callbacks); \
		switch (result) {                                         \
		case ISC_R_SUCCESS:                                       \
			break;                                            \
		case ISC_R_UNEXPECTED:                                    \
			goto insist_and_cleanup;                          \
		default:                                                  \
			if (MANYERRS(lctx, result)) {                     \
				SETRESULT(lctx, result);                  \
				LOGIT(result);                            \
				read_till_eol = true;                     \
				err goto next_line;                       \
			} else                                            \
				goto log_and_cleanup;                     \
		}                                                         \
		if ((token)->type == isc_tokentype_special) {             \
			result = DNS_R_SYNTAX;                            \
			if (MANYERRS(lctx, result)) {                     \
				SETRESULT(lctx, result);                  \
				LOGIT(result);                            \
				read_till_eol = true;                     \
				goto next_line;                           \
			} else                                            \
				goto log_and_cleanup;                     \
		}                                                         \
	} while (0)
#define GETTOKEN(lexer, options, token, eol) \
	GETTOKENERR(lexer, options, token, eol, {})

#define COMMITALL                                                              \
	do {                                                                   \
		result = commit(callbacks, lctx, &current_list, ictx->current, \
				source, ictx->current_line);                   \
		if (MANYERRS(lctx, result)) {                                  \
			SETRESULT(lctx, result);                               \
		} else if (result != ISC_R_SUCCESS)                            \
			goto insist_and_cleanup;                               \
		result = commit(callbacks, lctx, &glue_list, ictx->glue,       \
				source, ictx->glue_line);                      \
		if (MANYERRS(lctx, result)) {                                  \
			SETRESULT(lctx, result);                               \
		} else if (result != ISC_R_SUCCESS)                            \
			goto insist_and_cleanup;                               \
		rdcount = 0;                                                   \
		rdlcount = 0;                                                  \
		isc_buffer_init(&target, target_mem, target_size);             \
		rdcount_save = rdcount;                                        \
		rdlcount_save = rdlcount;                                      \
	} while (0)

#define WARNUNEXPECTEDEOF(lexer)                                         \
	do {                                                             \
		if (isc_lex_isfile(lexer))                               \
			(*callbacks->warn)(callbacks,                    \
					   "%s: file does not end with " \
					   "newline",                    \
					   source);                      \
	} while (0)

#define EXPECTEOL                                              \
	do {                                                   \
		GETTOKEN(lctx->lex, 0, &token, true);          \
		if (token.type != isc_tokentype_eol) {         \
			isc_lex_ungettoken(lctx->lex, &token); \
			result = DNS_R_EXTRATOKEN;             \
			if (MANYERRS(lctx, result)) {          \
				SETRESULT(lctx, result);       \
				LOGIT(result);                 \
				read_till_eol = true;          \
				break;                         \
			} else if (result != ISC_R_SUCCESS)    \
				goto log_and_cleanup;          \
		}                                              \
	} while (0)

#define MANYERRS(lctx, result)                                     \
	((result != ISC_R_SUCCESS) && (result != ISC_R_IOERROR) && \
	 ((lctx)->options & DNS_MASTER_MANYERRORS) != 0)

#define SETRESULT(lctx, r)                           \
	do {                                         \
		if ((lctx)->result == ISC_R_SUCCESS) \
			(lctx)->result = r;          \
	} while (0)

#define LOGITFILE(result, filename)                                            \
	if (result == ISC_R_INVALIDFILE || result == ISC_R_FILENOTFOUND ||     \
	    result == ISC_R_IOERROR || result == ISC_R_TOOMANYOPENFILES ||     \
	    result == ISC_R_NOPERM)                                            \
		(*callbacks->error)(callbacks, "%s: %s:%lu: %s: %s",           \
				    "dns_master_load", source, line, filename, \
				    dns_result_totext(result));                \
	else                                                                   \
		LOGIT(result)

#define LOGIT(result)                                                 \
	if (result == ISC_R_NOMEMORY)                                 \
		(*callbacks->error)(callbacks, "dns_master_load: %s", \
				    dns_result_totext(result));       \
	else                                                          \
		(*callbacks->error)(callbacks, "%s: %s:%lu: %s",      \
				    "dns_master_load", source, line,  \
				    dns_result_totext(result))

static unsigned char in_addr_arpa_data[] = "\007IN-ADDR\004ARPA";
static unsigned char in_addr_arpa_offsets[] = { 0, 8, 13 };
static dns_name_t const in_addr_arpa =
	DNS_NAME_INITABSOLUTE(in_addr_arpa_data, in_addr_arpa_offsets);

static unsigned char ip6_int_data[] = "\003IP6\003INT";
static unsigned char ip6_int_offsets[] = { 0, 4, 8 };
static dns_name_t const ip6_int = DNS_NAME_INITABSOLUTE(ip6_int_data,
							ip6_int_offsets);

static unsigned char ip6_arpa_data[] = "\003IP6\004ARPA";
static unsigned char ip6_arpa_offsets[] = { 0, 4, 9 };
static dns_name_t const ip6_arpa = DNS_NAME_INITABSOLUTE(ip6_arpa_data,
							 ip6_arpa_offsets);

static bool
dns_master_isprimary(dns_loadctx_t *lctx) {
	return ((lctx->options & DNS_MASTER_ZONE) != 0 &&
		(lctx->options & DNS_MASTER_SLAVE) == 0 &&
		(lctx->options & DNS_MASTER_KEY) == 0);
}

static isc_result_t
gettoken(isc_lex_t *lex, unsigned int options, isc_token_t *token, bool eol,
	 dns_rdatacallbacks_t *callbacks) {
	isc_result_t result;

	options |= ISC_LEXOPT_EOL | ISC_LEXOPT_EOF | ISC_LEXOPT_DNSMULTILINE |
		   ISC_LEXOPT_ESCAPE;
	result = isc_lex_gettoken(lex, options, token);
	if (result != ISC_R_SUCCESS) {
		switch (result) {
		case ISC_R_NOMEMORY:
			return (ISC_R_NOMEMORY);
		default:
			(*callbacks->error)(callbacks,
					    "dns_master_load: %s:%lu:"
					    " isc_lex_gettoken() failed: %s",
					    isc_lex_getsourcename(lex),
					    isc_lex_getsourceline(lex),
					    isc_result_totext(result));
			return (result);
		}
		/*NOTREACHED*/
	}
	if (eol != true) {
		if (token->type == isc_tokentype_eol ||
		    token->type == isc_tokentype_eof)
		{
			{
				unsigned long int line;
				const char *what;
				const char *file;
				file = isc_lex_getsourcename(lex);
				line = isc_lex_getsourceline(lex);
				if (token->type == isc_tokentype_eol) {
					line--;
					what = "line";
				} else {
					what = "file";
				}
				(*callbacks->error)(callbacks,
						    "dns_master_load: %s:%lu: "
						    "unexpected end of %s",
						    file, line, what);
				return (ISC_R_UNEXPECTEDEND);
			}
		}
	}
	return (ISC_R_SUCCESS);
}

void
dns_loadctx_attach(dns_loadctx_t *source, dns_loadctx_t **target) {
	REQUIRE(target != NULL && *target == NULL);
	REQUIRE(DNS_LCTX_VALID(source));

	isc_refcount_increment(&source->references);

	*target = source;
}

void
dns_loadctx_detach(dns_loadctx_t **lctxp) {
	dns_loadctx_t *lctx;

	REQUIRE(lctxp != NULL);
	lctx = *lctxp;
	*lctxp = NULL;
	REQUIRE(DNS_LCTX_VALID(lctx));

	if (isc_refcount_decrement(&lctx->references) == 1) {
		loadctx_destroy(lctx);
	}
}

static void
incctx_destroy(isc_mem_t *mctx, dns_incctx_t *ictx) {
	dns_incctx_t *parent;

again:
	parent = ictx->parent;
	ictx->parent = NULL;

	isc_mem_put(mctx, ictx, sizeof(*ictx));

	if (parent != NULL) {
		ictx = parent;
		goto again;
	}
}

static void
loadctx_destroy(dns_loadctx_t *lctx) {
	REQUIRE(DNS_LCTX_VALID(lctx));

	isc_refcount_destroy(&lctx->references);

	lctx->magic = 0;
	if (lctx->inc != NULL) {
		incctx_destroy(lctx->mctx, lctx->inc);
	}

	if (lctx->f != NULL) {
		isc_result_t result = isc_stdio_close(lctx->f);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_stdio_close() failed: %s",
					 isc_result_totext(result));
		}
	}

	/* isc_lex_destroy() will close all open streams */
	if (lctx->lex != NULL && !lctx->keep_lex) {
		isc_lex_destroy(&lctx->lex);
	}

	if (lctx->task != NULL) {
		isc_task_detach(&lctx->task);
	}

	isc_mem_putanddetach(&lctx->mctx, lctx, sizeof(*lctx));
}

static isc_result_t
incctx_create(isc_mem_t *mctx, dns_name_t *origin, dns_incctx_t **ictxp) {
	dns_incctx_t *ictx;
	isc_region_t r;
	int i;

	ictx = isc_mem_get(mctx, sizeof(*ictx));

	for (i = 0; i < NBUFS; i++) {
		dns_fixedname_init(&ictx->fixed[i]);
		ictx->in_use[i] = false;
	}

	ictx->origin_in_use = 0;
	ictx->origin = dns_fixedname_name(&ictx->fixed[ictx->origin_in_use]);
	ictx->in_use[ictx->origin_in_use] = true;
	dns_name_toregion(origin, &r);
	dns_name_fromregion(ictx->origin, &r);

	ictx->glue = NULL;
	ictx->current = NULL;
	ictx->glue_in_use = -1;
	ictx->current_in_use = -1;
	ictx->parent = NULL;
	ictx->drop = false;
	ictx->glue_line = 0;
	ictx->current_line = 0;
	ictx->origin_changed = true;

	*ictxp = ictx;
	return (ISC_R_SUCCESS);
}

static isc_result_t
loadctx_create(dns_masterformat_t format, isc_mem_t *mctx, unsigned int options,
	       uint32_t resign, dns_name_t *top, dns_rdataclass_t zclass,
	       dns_name_t *origin, dns_rdatacallbacks_t *callbacks,
	       isc_task_t *task, dns_loaddonefunc_t done, void *done_arg,
	       dns_masterincludecb_t include_cb, void *include_arg,
	       isc_lex_t *lex, dns_loadctx_t **lctxp) {
	dns_loadctx_t *lctx;
	isc_result_t result;
	isc_region_t r;
	isc_lexspecials_t specials;

	REQUIRE(lctxp != NULL && *lctxp == NULL);
	REQUIRE(callbacks != NULL);
	REQUIRE(callbacks->add != NULL);
	REQUIRE(callbacks->error != NULL);
	REQUIRE(callbacks->warn != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(dns_name_isabsolute(top));
	REQUIRE(dns_name_isabsolute(origin));
	REQUIRE((task == NULL && done == NULL) ||
		(task != NULL && done != NULL));

	lctx = isc_mem_get(mctx, sizeof(*lctx));

	lctx->inc = NULL;
	result = incctx_create(mctx, origin, &lctx->inc);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_ctx;
	}

	lctx->maxttl = 0;

	lctx->format = format;
	switch (format) {
	case dns_masterformat_text:
		lctx->openfile = openfile_text;
		lctx->load = load_text;
		break;
	case dns_masterformat_raw:
		lctx->openfile = openfile_raw;
		lctx->load = load_raw;
		break;
	case dns_masterformat_map:
		lctx->openfile = openfile_map;
		lctx->load = load_map;
		break;
	default:
		UNREACHABLE();
	}

	if (lex != NULL) {
		lctx->lex = lex;
		lctx->keep_lex = true;
	} else {
		lctx->lex = NULL;
		result = isc_lex_create(mctx, TOKENSIZ, &lctx->lex);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_inc;
		}
		lctx->keep_lex = false;
		/*
		 * If specials change update dns_test_rdatafromstring()
		 * in lib/dns/tests/dnstest.c.
		 */
		memset(specials, 0, sizeof(specials));
		specials[0] = 1;
		specials['('] = 1;
		specials[')'] = 1;
		specials['"'] = 1;
		isc_lex_setspecials(lctx->lex, specials);
		isc_lex_setcomments(lctx->lex, ISC_LEXCOMMENT_DNSMASTERFILE);
	}

	lctx->ttl_known = ((options & DNS_MASTER_NOTTL) != 0);
	lctx->ttl = 0;
	lctx->default_ttl_known = lctx->ttl_known;
	lctx->default_ttl = 0;
	lctx->warn_1035 = true;	      /* XXX Argument? */
	lctx->warn_tcr = true;	      /* XXX Argument? */
	lctx->warn_sigexpired = true; /* XXX Argument? */
	lctx->options = options;
	lctx->seen_include = false;
	lctx->zclass = zclass;
	lctx->resign = resign;
	lctx->result = ISC_R_SUCCESS;
	lctx->include_cb = include_cb;
	lctx->include_arg = include_arg;
	isc_stdtime_get(&lctx->now);

	lctx->top = dns_fixedname_initname(&lctx->fixed_top);
	dns_name_toregion(top, &r);
	dns_name_fromregion(lctx->top, &r);

	lctx->f = NULL;
	lctx->first = true;
	dns_master_initrawheader(&lctx->header);

	lctx->loop_cnt = (done != NULL) ? 100 : 0;
	lctx->callbacks = callbacks;
	lctx->task = NULL;
	if (task != NULL) {
		isc_task_attach(task, &lctx->task);
	}
	lctx->done = done;
	lctx->done_arg = done_arg;
	atomic_init(&lctx->canceled, false);
	lctx->mctx = NULL;
	isc_mem_attach(mctx, &lctx->mctx);

	isc_refcount_init(&lctx->references, 1); /* Implicit attach. */

	lctx->magic = DNS_LCTX_MAGIC;
	*lctxp = lctx;
	return (ISC_R_SUCCESS);

cleanup_inc:
	incctx_destroy(mctx, lctx->inc);
cleanup_ctx:
	isc_mem_put(mctx, lctx, sizeof(*lctx));
	return (result);
}

static const char *hex = "0123456789abcdef0123456789ABCDEF";

/*%
 * Convert value into a nibble sequence from least significant to most
 * significant nibble.  Zero fill upper most significant nibbles if
 * required to make the width.
 *
 * Returns the number of characters that should have been written without
 * counting the terminating NUL.
 */
static unsigned int
nibbles(char *numbuf, size_t length, unsigned int width, char mode, int value) {
	unsigned int count = 0;

	/*
	 * This reserve space for the NUL string terminator.
	 */
	if (length > 0U) {
		*numbuf = '\0';
		length--;
	}
	do {
		char val = hex[(value & 0x0f) + ((mode == 'n') ? 0 : 16)];
		value >>= 4;
		if (length > 0U) {
			*numbuf++ = val;
			*numbuf = '\0';
			length--;
		}
		if (width > 0) {
			width--;
		}
		count++;
		/*
		 * If width is non zero then we need to add a label separator.
		 * If value is non zero then we need to add another label and
		 * that requires a label separator.
		 */
		if (width > 0 || value != 0) {
			if (length > 0U) {
				*numbuf++ = '.';
				*numbuf = '\0';
				length--;
			}
			if (width > 0) {
				width--;
			}
			count++;
		}
	} while (value != 0 || width > 0);
	return (count);
}

static isc_result_t
genname(char *name, int it, char *buffer, size_t length) {
	char fmt[sizeof("%04000000000d")];
	char numbuf[128];
	char *cp;
	char mode[2] = { 0 };
	char brace[2] = { 0 };
	char comma1[2] = { 0 };
	char comma2[2] = { 0 };
	int delta = 0;
	isc_textregion_t r;
	unsigned int n;
	unsigned int width;
	bool nibblemode;

	r.base = buffer;
	r.length = (unsigned int)length;

	while (*name != '\0') {
		if (*name == '$') {
			name++;
			if (*name == '$') {
				if (r.length == 0) {
					return (ISC_R_NOSPACE);
				}
				r.base[0] = *name++;
				isc_textregion_consume(&r, 1);
				continue;
			}
			nibblemode = false;
			strlcpy(fmt, "%d", sizeof(fmt));
			/* Get format specifier. */
			if (*name == '{') {
				n = sscanf(name,
					   "{%d%1[,}]%u%1[,}]%1[doxXnN]%1[}]",
					   &delta, comma1, &width, comma2, mode,
					   brace);
				if (n < 2 || n > 6) {
					return (DNS_R_SYNTAX);
				}
				if (comma1[0] == '}') {
					/* %{delta} */
				} else if (comma1[0] == ',' && comma2[0] == '}')
				{
					/* %{delta,width} */
					n = snprintf(fmt, sizeof(fmt), "%%0%ud",
						     width);
				} else if (comma1[0] == ',' &&
					   comma2[0] == ',' && mode[0] != 0 &&
					   brace[0] == '}')
				{
					/* %{delta,width,format} */
					if (mode[0] == 'n' || mode[0] == 'N') {
						nibblemode = true;
					}
					n = snprintf(fmt, sizeof(fmt),
						     "%%0%u%c", width, mode[0]);
				} else {
					return (DNS_R_SYNTAX);
				}
				if (n >= sizeof(fmt)) {
					return (ISC_R_NOSPACE);
				}
				/* Skip past closing brace. */
				while (*name != '\0' && *name++ != '}') {
					continue;
				}
			}
			/*
			 * 'it' is >= 0 so we don't need to check for
			 * underflow.
			 */
			if ((it > 0 && delta > INT_MAX - it)) {
				return (ISC_R_RANGE);
			}
			if (nibblemode) {
				n = nibbles(numbuf, sizeof(numbuf), width,
					    mode[0], it + delta);
			} else {
				n = snprintf(numbuf, sizeof(numbuf), fmt,
					     it + delta);
			}
			if (n >= sizeof(numbuf)) {
				return (ISC_R_NOSPACE);
			}
			cp = numbuf;
			while (*cp != '\0') {
				if (r.length == 0) {
					return (ISC_R_NOSPACE);
				}
				r.base[0] = *cp++;
				isc_textregion_consume(&r, 1);
			}
		} else if (*name == '\\') {
			if (r.length == 0) {
				return (ISC_R_NOSPACE);
			}
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
			if (*name == '\0') {
				continue;
			}
			if (r.length == 0) {
				return (ISC_R_NOSPACE);
			}
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
		} else {
			if (r.length == 0) {
				return (ISC_R_NOSPACE);
			}
			r.base[0] = *name++;
			isc_textregion_consume(&r, 1);
		}
	}
	if (r.length == 0) {
		return (ISC_R_NOSPACE);
	}
	r.base[0] = '\0';
	return (ISC_R_SUCCESS);
}

static isc_result_t
generate(dns_loadctx_t *lctx, char *range, char *lhs, char *gtype, char *rhs,
	 const char *source, unsigned int line) {
	char *target_mem = NULL;
	char *lhsbuf = NULL;
	char *rhsbuf = NULL;
	dns_fixedname_t ownerfixed;
	dns_name_t *owner;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatacallbacks_t *callbacks;
	dns_rdatalist_t rdatalist;
	dns_rdatatype_t type;
	rdatalist_head_t head;
	int target_size = MINTSIZ; /* only one rdata at a time */
	isc_buffer_t buffer;
	isc_buffer_t target;
	isc_result_t result;
	isc_textregion_t r;
	int n, start, stop, step = 0;
	unsigned int i;
	dns_incctx_t *ictx;
	char dummy[2];

	ictx = lctx->inc;
	callbacks = lctx->callbacks;
	owner = dns_fixedname_initname(&ownerfixed);
	ISC_LIST_INIT(head);

	target_mem = isc_mem_get(lctx->mctx, target_size);
	rhsbuf = isc_mem_get(lctx->mctx, DNS_MASTER_RHS);
	lhsbuf = isc_mem_get(lctx->mctx, DNS_MASTER_LHS);
	if (target_mem == NULL || rhsbuf == NULL || lhsbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto error_cleanup;
	}
	isc_buffer_init(&target, target_mem, target_size);

	n = sscanf(range, "%d-%d%1[/]%d", &start, &stop, dummy, &step);
	if ((n != 2 && n != 4) || (start < 0) || (stop < 0) ||
	    (n == 4 && step < 1) || (stop < start))
	{
		(*callbacks->error)(callbacks, "%s: %s:%lu: invalid range '%s'",
				    "$GENERATE", source, line, range);
		result = DNS_R_SYNTAX;
		goto insist_cleanup;
	}
	if (n == 2) {
		step = 1;
	}

	/*
	 * Get type.
	 */
	r.base = gtype;
	r.length = strlen(gtype);
	result = dns_rdatatype_fromtext(&type, &r);
	if (result != ISC_R_SUCCESS) {
		(*callbacks->error)(callbacks,
				    "%s: %s:%lu: unknown RR type '%s'",
				    "$GENERATE", source, line, gtype);
		goto insist_cleanup;
	}

	/*
	 * RFC2930: TKEY and TSIG are not allowed to be loaded
	 * from master files.
	 */
	if (dns_master_isprimary(lctx) && dns_rdatatype_ismeta(type)) {
		(*callbacks->error)(callbacks, "%s: %s:%lu: meta RR type '%s'",
				    "$GENERATE", source, line, gtype);
		result = DNS_R_METATYPE;
		goto insist_cleanup;
	}

	for (i = start; i <= (unsigned int)stop; i += step) {
		result = genname(lhs, i, lhsbuf, DNS_MASTER_LHS);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}
		result = genname(rhs, i, rhsbuf, DNS_MASTER_RHS);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}

		isc_buffer_init(&buffer, lhsbuf, strlen(lhsbuf));
		isc_buffer_add(&buffer, strlen(lhsbuf));
		isc_buffer_setactive(&buffer, strlen(lhsbuf));
		result = dns_name_fromtext(owner, &buffer, ictx->origin, 0,
					   NULL);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}

		if (dns_master_isprimary(lctx) &&
		    !dns_name_issubdomain(owner, lctx->top))
		{
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(owner, namebuf, sizeof(namebuf));
			/*
			 * Ignore out-of-zone data.
			 */
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "ignoring out-of-zone data (%s)",
					   source, line, namebuf);
			continue;
		}

		isc_buffer_init(&buffer, rhsbuf, strlen(rhsbuf));
		isc_buffer_add(&buffer, strlen(rhsbuf));
		isc_buffer_setactive(&buffer, strlen(rhsbuf));

		result = isc_lex_openbuffer(lctx->lex, &buffer);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}

		isc_buffer_init(&target, target_mem, target_size);
		result = dns_rdata_fromtext(&rdata, lctx->zclass, type,
					    lctx->lex, ictx->origin, 0,
					    lctx->mctx, &target, callbacks);
		RUNTIME_CHECK(isc_lex_close(lctx->lex) == ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}

		dns_rdatalist_init(&rdatalist);
		rdatalist.type = type;
		rdatalist.rdclass = lctx->zclass;
		rdatalist.ttl = lctx->ttl;
		ISC_LIST_PREPEND(head, &rdatalist, link);
		ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
		result = commit(callbacks, lctx, &head, owner, source, line);
		ISC_LIST_UNLINK(rdatalist.rdata, &rdata, link);
		if (result != ISC_R_SUCCESS) {
			goto error_cleanup;
		}
		dns_rdata_reset(&rdata);
	}
	result = ISC_R_SUCCESS;
	goto cleanup;

error_cleanup:
	if (result == ISC_R_NOMEMORY) {
		(*callbacks->error)(callbacks, "$GENERATE: %s",
				    dns_result_totext(result));
	} else {
		(*callbacks->error)(callbacks, "$GENERATE: %s:%lu: %s", source,
				    line, dns_result_totext(result));
	}

insist_cleanup:
	INSIST(result != ISC_R_SUCCESS);

cleanup:
	if (target_mem != NULL) {
		isc_mem_put(lctx->mctx, target_mem, target_size);
	}
	if (lhsbuf != NULL) {
		isc_mem_put(lctx->mctx, lhsbuf, DNS_MASTER_LHS);
	}
	if (rhsbuf != NULL) {
		isc_mem_put(lctx->mctx, rhsbuf, DNS_MASTER_RHS);
	}
	return (result);
}

static void
limit_ttl(dns_rdatacallbacks_t *callbacks, const char *source,
	  unsigned int line, uint32_t *ttlp) {
	if (*ttlp > 0x7fffffffUL) {
		(callbacks->warn)(callbacks,
				  "%s: %s:%lu: "
				  "$TTL %lu > MAXTTL, "
				  "setting $TTL to 0",
				  "dns_master_load", source, line, *ttlp);
		*ttlp = 0;
	}
}

static isc_result_t
check_ns(dns_loadctx_t *lctx, isc_token_t *token, const char *source,
	 unsigned long line) {
	char *tmp = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	void (*callback)(struct dns_rdatacallbacks *, const char *, ...);

	if ((lctx->options & DNS_MASTER_FATALNS) != 0) {
		callback = lctx->callbacks->error;
	} else {
		callback = lctx->callbacks->warn;
	}

	if (token->type == isc_tokentype_string) {
		struct in_addr addr;
		struct in6_addr addr6;

		tmp = isc_mem_strdup(lctx->mctx, DNS_AS_STR(*token));
		/*
		 * Catch both "1.2.3.4" and "1.2.3.4."
		 */
		if (tmp[strlen(tmp) - 1] == '.') {
			tmp[strlen(tmp) - 1] = '\0';
		}
		if (inet_pton(AF_INET, tmp, &addr) == 1 ||
		    inet_pton(AF_INET6, tmp, &addr6) == 1)
		{
			result = DNS_R_NSISADDRESS;
		}
	}
	if (result != ISC_R_SUCCESS) {
		(*callback)(lctx->callbacks,
			    "%s:%lu: NS record '%s' "
			    "appears to be an address",
			    source, line, DNS_AS_STR(*token));
	}
	if (tmp != NULL) {
		isc_mem_free(lctx->mctx, tmp);
	}
	return (result);
}

static void
check_wildcard(dns_incctx_t *ictx, const char *source, unsigned long line,
	       dns_rdatacallbacks_t *callbacks) {
	dns_name_t *name;

	name = (ictx->glue != NULL) ? ictx->glue : ictx->current;
	if (dns_name_internalwildcard(name)) {
		char namebuf[DNS_NAME_FORMATSIZE];

		dns_name_format(name, namebuf, sizeof(namebuf));
		(*callbacks->warn)(callbacks,
				   "%s:%lu: warning: ownername "
				   "'%s' contains an non-terminal wildcard",
				   source, line, namebuf);
	}
}

static isc_result_t
openfile_text(dns_loadctx_t *lctx, const char *master_file) {
	return (isc_lex_openfile(lctx->lex, master_file));
}

static int
find_free_name(dns_incctx_t *incctx) {
	int i;

	for (i = 0; i < (NBUFS - 1); i++) {
		if (!incctx->in_use[i]) {
			break;
		}
	}
	INSIST(!incctx->in_use[i]);
	return (i);
}

static isc_result_t
load_text(dns_loadctx_t *lctx) {
	dns_rdataclass_t rdclass;
	dns_rdatatype_t type, covers;
	uint32_t ttl_offset = 0;
	dns_name_t *new_name;
	bool current_has_delegation = false;
	bool done = false;
	bool finish_origin = false;
	bool finish_include = false;
	bool read_till_eol = false;
	bool initialws;
	char *include_file = NULL;
	isc_token_t token;
	isc_result_t result = ISC_R_UNEXPECTED;
	rdatalist_head_t glue_list;
	rdatalist_head_t current_list;
	dns_rdatalist_t *this;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdatalist_t *new_rdatalist;
	int rdlcount = 0;
	int rdlcount_save = 0;
	int rdatalist_size = 0;
	isc_buffer_t buffer;
	isc_buffer_t target;
	isc_buffer_t target_ft;
	isc_buffer_t target_save;
	dns_rdata_t *rdata = NULL;
	dns_rdata_t *new_rdata;
	int rdcount = 0;
	int rdcount_save = 0;
	int rdata_size = 0;
	unsigned char *target_mem = NULL;
	int target_size = TSIZ;
	int new_in_use;
	unsigned int loop_cnt = 0;
	isc_mem_t *mctx;
	dns_rdatacallbacks_t *callbacks;
	dns_incctx_t *ictx;
	char *range = NULL;
	char *lhs = NULL;
	char *gtype = NULL;
	char *rhs = NULL;
	const char *source = "";
	unsigned long line = 0;
	bool explicit_ttl;
	char classname1[DNS_RDATACLASS_FORMATSIZE];
	char classname2[DNS_RDATACLASS_FORMATSIZE];
	unsigned int options = 0;

	REQUIRE(DNS_LCTX_VALID(lctx));
	callbacks = lctx->callbacks;
	mctx = lctx->mctx;
	ictx = lctx->inc;

	ISC_LIST_INIT(glue_list);
	ISC_LIST_INIT(current_list);

	/*
	 * Allocate target_size of buffer space.  This is greater than twice
	 * the maximum individual RR data size.
	 */
	target_mem = isc_mem_get(mctx, target_size);
	isc_buffer_init(&target, target_mem, target_size);
	target_save = target;

	if ((lctx->options & DNS_MASTER_CHECKNAMES) != 0) {
		options |= DNS_RDATA_CHECKNAMES;
	}
	if ((lctx->options & DNS_MASTER_CHECKNAMESFAIL) != 0) {
		options |= DNS_RDATA_CHECKNAMESFAIL;
	}
	if ((lctx->options & DNS_MASTER_CHECKMX) != 0) {
		options |= DNS_RDATA_CHECKMX;
	}
	if ((lctx->options & DNS_MASTER_CHECKMXFAIL) != 0) {
		options |= DNS_RDATA_CHECKMXFAIL;
	}
	source = isc_lex_getsourcename(lctx->lex);
	do {
		initialws = false;
		line = isc_lex_getsourceline(lctx->lex);
		GETTOKEN(lctx->lex, ISC_LEXOPT_INITIALWS | ISC_LEXOPT_QSTRING,
			 &token, true);
		line = isc_lex_getsourceline(lctx->lex);

		if (token.type == isc_tokentype_eof) {
			if (read_till_eol) {
				WARNUNEXPECTEDEOF(lctx->lex);
			}
			/* Pop the include stack? */
			if (ictx->parent != NULL) {
				COMMITALL;
				lctx->inc = ictx->parent;
				ictx->parent = NULL;
				incctx_destroy(lctx->mctx, ictx);
				RUNTIME_CHECK(isc_lex_close(lctx->lex) ==
					      ISC_R_SUCCESS);
				line = isc_lex_getsourceline(lctx->lex);
				POST(line);
				source = isc_lex_getsourcename(lctx->lex);
				ictx = lctx->inc;
				continue;
			}
			done = true;
			continue;
		}

		if (token.type == isc_tokentype_eol) {
			read_till_eol = false;
			continue; /* blank line */
		}

		if (read_till_eol) {
			continue;
		}

		if (token.type == isc_tokentype_initialws) {
			/*
			 * Still working on the same name.
			 */
			initialws = true;
		} else if (token.type == isc_tokentype_string ||
			   token.type == isc_tokentype_qstring)
		{
			/*
			 * "$" Support.
			 *
			 * "$ORIGIN" and "$INCLUDE" can both take domain names.
			 * The processing of "$ORIGIN" and "$INCLUDE" extends
			 * across the normal domain name processing.
			 */

			if (strcasecmp(DNS_AS_STR(token), "$ORIGIN") == 0) {
				GETTOKEN(lctx->lex, 0, &token, false);
				finish_origin = true;
			} else if (strcasecmp(DNS_AS_STR(token), "$TTL") == 0) {
				GETTOKENERR(lctx->lex, 0, &token, false,
					    lctx->ttl = 0;
					    lctx->default_ttl_known = true;);
				result = dns_ttl_fromtext(
					&token.value.as_textregion, &lctx->ttl);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					lctx->ttl = 0;
				} else if (result != ISC_R_SUCCESS) {
					goto insist_and_cleanup;
				}
				limit_ttl(callbacks, source, line, &lctx->ttl);
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = true;
				EXPECTEOL;
				continue;
			} else if (strcasecmp(DNS_AS_STR(token), "$INCLUDE") ==
				   0)
			{
				COMMITALL;
				if ((lctx->options & DNS_MASTER_NOINCLUDE) != 0)
				{
					(callbacks->error)(callbacks,
							   "%s: %s:%lu: "
							   "$INCLUDE not "
							   "allowed",
							   "dns_master_load",
							   source, line);
					result = DNS_R_REFUSED;
					goto insist_and_cleanup;
				}
				if (ttl_offset != 0) {
					(callbacks->error)(callbacks,
							   "%s: %s:%lu: "
							   "$INCLUDE "
							   "may not be used "
							   "with $DATE",
							   "dns_master_load",
							   source, line);
					result = DNS_R_SYNTAX;
					goto insist_and_cleanup;
				}
				GETTOKEN(lctx->lex, ISC_LEXOPT_QSTRING, &token,
					 false);
				if (include_file != NULL) {
					isc_mem_free(mctx, include_file);
				}
				include_file =
					isc_mem_strdup(mctx, DNS_AS_STR(token));
				GETTOKEN(lctx->lex, 0, &token, true);

				if (token.type == isc_tokentype_eol ||
				    token.type == isc_tokentype_eof)
				{
					if (token.type == isc_tokentype_eof) {
						WARNUNEXPECTEDEOF(lctx->lex);
					}
					/*
					 * No origin field.
					 */
					result = pushfile(include_file,
							  ictx->origin, lctx);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
						LOGITFILE(result, include_file);
						continue;
					} else if (result != ISC_R_SUCCESS) {
						LOGITFILE(result, include_file);
						goto insist_and_cleanup;
					}
					ictx = lctx->inc;
					source = isc_lex_getsourcename(
						lctx->lex);
					line = isc_lex_getsourceline(lctx->lex);
					POST(line);
					continue;
				}
				/*
				 * There is an origin field.  Fall through
				 * to domain name processing code and do
				 * the actual inclusion later.
				 */
				finish_include = true;
			} else if (strcasecmp(DNS_AS_STR(token), "$DATE") == 0)
			{
				int64_t dump_time64;
				isc_stdtime_t dump_time, current_time;
				GETTOKEN(lctx->lex, 0, &token, false);
				isc_stdtime_get(&current_time);
				result = dns_time64_fromtext(DNS_AS_STR(token),
							     &dump_time64);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					LOGIT(result);
					dump_time64 = 0;
				} else if (result != ISC_R_SUCCESS) {
					goto log_and_cleanup;
				}
				dump_time = (isc_stdtime_t)dump_time64;
				if (dump_time != dump_time64) {
					UNEXPECTED_ERROR(__FILE__, __LINE__,
							 "%s: %s:%lu: $DATE "
							 "outside epoch",
							 "dns_master_load",
							 source, line);
					result = ISC_R_UNEXPECTED;
					goto insist_and_cleanup;
				}
				if (dump_time > current_time) {
					UNEXPECTED_ERROR(__FILE__, __LINE__,
							 "%s: %s:%lu: "
							 "$DATE in future, "
							 "using current date",
							 "dns_master_load",
							 source, line);
					dump_time = current_time;
				}
				ttl_offset = current_time - dump_time;
				EXPECTEOL;
				continue;
			} else if (strcasecmp(DNS_AS_STR(token), "$GENERATE") ==
				   0)
			{
				/*
				 * Lazy cleanup.
				 */
				if (range != NULL) {
					isc_mem_free(mctx, range);
				}
				if (lhs != NULL) {
					isc_mem_free(mctx, lhs);
				}
				if (gtype != NULL) {
					isc_mem_free(mctx, gtype);
				}
				if (rhs != NULL) {
					isc_mem_free(mctx, rhs);
				}
				range = lhs = gtype = rhs = NULL;
				/* RANGE */
				GETTOKEN(lctx->lex, 0, &token, false);
				range = isc_mem_strdup(mctx, DNS_AS_STR(token));
				/* LHS */
				GETTOKEN(lctx->lex, 0, &token, false);
				lhs = isc_mem_strdup(mctx, DNS_AS_STR(token));
				rdclass = 0;
				explicit_ttl = false;
				/* CLASS? */
				GETTOKEN(lctx->lex, 0, &token, false);
				if (dns_rdataclass_fromtext(
					    &rdclass,
					    &token.value.as_textregion) ==
				    ISC_R_SUCCESS)
				{
					GETTOKEN(lctx->lex, 0, &token, false);
				}
				/* TTL? */
				if (dns_ttl_fromtext(&token.value.as_textregion,
						     &lctx->ttl) ==
				    ISC_R_SUCCESS)
				{
					limit_ttl(callbacks, source, line,
						  &lctx->ttl);
					lctx->ttl_known = true;
					explicit_ttl = true;
					GETTOKEN(lctx->lex, 0, &token, false);
				}
				/* CLASS? */
				if (rdclass == 0 &&
				    dns_rdataclass_fromtext(
					    &rdclass,
					    &token.value.as_textregion) ==
					    ISC_R_SUCCESS)
				{
					GETTOKEN(lctx->lex, 0, &token, false);
				}
				/* TYPE */
				gtype = isc_mem_strdup(mctx, DNS_AS_STR(token));
				/* RHS */
				GETTOKEN(lctx->lex, ISC_LEXOPT_QSTRING, &token,
					 false);
				rhs = isc_mem_strdup(mctx, DNS_AS_STR(token));
				if (!lctx->ttl_known &&
				    !lctx->default_ttl_known)
				{
					(*callbacks->error)(callbacks,
							    "%s: %s:%lu: no "
							    "TTL specified",
							    "dns_master_load",
							    source, line);
					result = DNS_R_NOTTL;
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
						lctx->ttl = 0;
					} else {
						goto insist_and_cleanup;
					}
				} else if (!explicit_ttl &&
					   lctx->default_ttl_known)
				{
					lctx->ttl = lctx->default_ttl;
				}
				/*
				 * If the class specified does not match the
				 * zone's class print out a error message and
				 * exit.
				 */
				if (rdclass != 0 && rdclass != lctx->zclass) {
					goto bad_class;
				}
				result = generate(lctx, range, lhs, gtype, rhs,
						  source, line);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS) {
					goto insist_and_cleanup;
				}
				EXPECTEOL;
				continue;
			} else if (strncasecmp(DNS_AS_STR(token), "$", 1) == 0)
			{
				(callbacks->error)(callbacks,
						   "%s: %s:%lu: "
						   "unknown $ directive '%s'",
						   "dns_master_load", source,
						   line, DNS_AS_STR(token));
				result = DNS_R_SYNTAX;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else {
					goto insist_and_cleanup;
				}
			}

			/*
			 * Normal processing resumes.
			 */
			new_in_use = find_free_name(ictx);
			new_name = dns_fixedname_initname(
				&ictx->fixed[new_in_use]);
			isc_buffer_init(&buffer, token.value.as_region.base,
					token.value.as_region.length);
			isc_buffer_add(&buffer, token.value.as_region.length);
			isc_buffer_setactive(&buffer,
					     token.value.as_region.length);
			result = dns_name_fromtext(new_name, &buffer,
						   ictx->origin, 0, NULL);
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				LOGIT(result);
				read_till_eol = true;
				continue;
			} else if (result != ISC_R_SUCCESS) {
				goto log_and_cleanup;
			}

			/*
			 * Finish $ORIGIN / $INCLUDE processing if required.
			 */
			if (finish_origin) {
				if (ictx->origin_in_use != -1) {
					ictx->in_use[ictx->origin_in_use] =
						false;
				}
				ictx->origin_in_use = new_in_use;
				ictx->in_use[ictx->origin_in_use] = true;
				ictx->origin = new_name;
				ictx->origin_changed = true;
				finish_origin = false;
				EXPECTEOL;
				continue;
			}
			if (finish_include) {
				finish_include = false;
				EXPECTEOL;
				result = pushfile(include_file, new_name, lctx);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					LOGITFILE(result, include_file);
					continue;
				} else if (result != ISC_R_SUCCESS) {
					LOGITFILE(result, include_file);
					goto insist_and_cleanup;
				}
				ictx = lctx->inc;
				ictx->origin_changed = true;
				source = isc_lex_getsourcename(lctx->lex);
				line = isc_lex_getsourceline(lctx->lex);
				POST(line);
				continue;
			}

			/*
			 * "$" Processing Finished
			 */

			/*
			 * If we are processing glue and the new name does
			 * not match the current glue name, commit the glue
			 * and pop stacks leaving us in 'normal' processing
			 * state.  Linked lists are undone by commit().
			 */
			if (ictx->glue != NULL &&
			    !dns_name_caseequal(ictx->glue, new_name))
			{
				result = commit(callbacks, lctx, &glue_list,
						ictx->glue, source,
						ictx->glue_line);
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS) {
					goto insist_and_cleanup;
				}
				if (ictx->glue_in_use != -1) {
					ictx->in_use[ictx->glue_in_use] = false;
				}
				ictx->glue_in_use = -1;
				ictx->glue = NULL;
				rdcount = rdcount_save;
				rdlcount = rdlcount_save;
				target = target_save;
			}

			/*
			 * If we are in 'normal' processing state and the new
			 * name does not match the current name, see if the
			 * new name is for glue and treat it as such,
			 * otherwise we have a new name so commit what we
			 * have.
			 */
			if ((ictx->glue == NULL) &&
			    (ictx->current == NULL ||
			     !dns_name_caseequal(ictx->current, new_name)))
			{
				if (current_has_delegation &&
				    is_glue(&current_list, new_name))
				{
					rdcount_save = rdcount;
					rdlcount_save = rdlcount;
					target_save = target;
					ictx->glue = new_name;
					ictx->glue_in_use = new_in_use;
					ictx->in_use[ictx->glue_in_use] = true;
				} else {
					result = commit(callbacks, lctx,
							&current_list,
							ictx->current, source,
							ictx->current_line);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
					} else if (result != ISC_R_SUCCESS) {
						goto insist_and_cleanup;
					}
					rdcount = 0;
					rdlcount = 0;
					if (ictx->current_in_use != -1) {
						ictx->in_use
							[ictx->current_in_use] =
							false;
					}
					ictx->current_in_use = new_in_use;
					ictx->in_use[ictx->current_in_use] =
						true;
					ictx->current = new_name;
					current_has_delegation = false;
					isc_buffer_init(&target, target_mem,
							target_size);
				}
				/*
				 * Check for internal wildcards.
				 */
				if ((lctx->options &
				     DNS_MASTER_CHECKWILDCARD) != 0)
				{
					check_wildcard(ictx, source, line,
						       callbacks);
				}
			}
			if (dns_master_isprimary(lctx) &&
			    !dns_name_issubdomain(new_name, lctx->top))
			{
				char namebuf[DNS_NAME_FORMATSIZE];
				dns_name_format(new_name, namebuf,
						sizeof(namebuf));
				/*
				 * Ignore out-of-zone data.
				 */
				(*callbacks->warn)(callbacks,
						   "%s:%lu: "
						   "ignoring out-of-zone data "
						   "(%s)",
						   source, line, namebuf);
				ictx->drop = true;
			} else {
				ictx->drop = false;
			}
		} else {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "%s:%lu: isc_lex_gettoken() returned "
					 "unexpected token type (%d)",
					 source, line, token.type);
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				LOGIT(result);
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		/*
		 * Find TTL, class and type.  Both TTL and class are optional
		 * and may occur in any order if they exist. TTL and class
		 * come before type which must exist.
		 *
		 * [<TTL>] [<class>] <type> <RDATA>
		 * [<class>] [<TTL>] <type> <RDATA>
		 */

		type = 0;
		rdclass = 0;

		GETTOKEN(lctx->lex, 0, &token, initialws);

		if (initialws) {
			if (token.type == isc_tokentype_eol) {
				read_till_eol = false;
				continue; /* blank line */
			}

			if (token.type == isc_tokentype_eof) {
				WARNUNEXPECTEDEOF(lctx->lex);
				read_till_eol = false;
				isc_lex_ungettoken(lctx->lex, &token);
				continue;
			}

			if (ictx->current == NULL) {
				(*callbacks->error)(callbacks,
						    "%s:%lu: no current owner "
						    "name",
						    source, line);
				result = DNS_R_NOOWNER;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					read_till_eol = true;
					continue;
				} else {
					goto insist_and_cleanup;
				}
			}

			if (ictx->origin_changed) {
				char cbuf[DNS_NAME_FORMATSIZE];
				char obuf[DNS_NAME_FORMATSIZE];
				dns_name_format(ictx->current, cbuf,
						sizeof(cbuf));
				dns_name_format(ictx->origin, obuf,
						sizeof(obuf));
				(*callbacks->warn)(callbacks,
						   "%s:%lu: record with "
						   "inherited "
						   "owner (%s) immediately "
						   "after "
						   "$ORIGIN (%s)",
						   source, line, cbuf, obuf);
			}
		}

		ictx->origin_changed = false;

		if (dns_rdataclass_fromtext(&rdclass,
					    &token.value.as_textregion) ==
		    ISC_R_SUCCESS)
		{
			GETTOKEN(lctx->lex, 0, &token, false);
		}

		explicit_ttl = false;
		result = dns_ttl_fromtext(&token.value.as_textregion,
					  &lctx->ttl);
		if (result == ISC_R_SUCCESS) {
			limit_ttl(callbacks, source, line, &lctx->ttl);
			explicit_ttl = true;
			lctx->ttl_known = true;
			GETTOKEN(lctx->lex, 0, &token, false);
		}

		if (token.type != isc_tokentype_string) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_lex_gettoken() returned "
					 "unexpected token type");
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = true;
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		if (rdclass == 0 &&
		    dns_rdataclass_fromtext(&rdclass,
					    &token.value.as_textregion) ==
			    ISC_R_SUCCESS)
		{
			GETTOKEN(lctx->lex, 0, &token, false);
		}

		if (token.type != isc_tokentype_string) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "isc_lex_gettoken() returned "
					 "unexpected token type");
			result = ISC_R_UNEXPECTED;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = true;
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		result = dns_rdatatype_fromtext(&type,
						&token.value.as_textregion);
		if (result != ISC_R_SUCCESS) {
			(*callbacks->warn)(
				callbacks, "%s:%lu: unknown RR type '%.*s'",
				source, line, token.value.as_textregion.length,
				token.value.as_textregion.base);
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = true;
				continue;
			} else if (result != ISC_R_SUCCESS) {
				goto insist_and_cleanup;
			}
		}

		/*
		 * If the class specified does not match the zone's class
		 * print out a error message and exit.
		 */
		if (rdclass != 0 && rdclass != lctx->zclass) {
		bad_class:

			dns_rdataclass_format(rdclass, classname1,
					      sizeof(classname1));
			dns_rdataclass_format(lctx->zclass, classname2,
					      sizeof(classname2));
			(*callbacks->error)(callbacks,
					    "%s:%lu: class '%s' != "
					    "zone class '%s'",
					    source, line, classname1,
					    classname2);
			result = DNS_R_BADCLASS;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = true;
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		if (type == dns_rdatatype_ns && ictx->glue == NULL) {
			current_has_delegation = true;
		}

		/*
		 * RFC1123: MD and MF are not allowed to be loaded from
		 * master files.
		 */
		if (dns_master_isprimary(lctx) &&
		    (type == dns_rdatatype_md || type == dns_rdatatype_mf))
		{
			char typebuf[DNS_RDATATYPE_FORMATSIZE];

			result = DNS_R_OBSOLETE;

			dns_rdatatype_format(type, typebuf, sizeof(typebuf));
			(*callbacks->error)(callbacks, "%s:%lu: %s '%s': %s",
					    source, line, "type", typebuf,
					    dns_result_totext(result));
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
			} else {
				goto insist_and_cleanup;
			}
		}

		/*
		 * RFC2930: TKEY and TSIG are not allowed to be loaded
		 * from master files.
		 */
		if (dns_master_isprimary(lctx) && dns_rdatatype_ismeta(type)) {
			char typebuf[DNS_RDATATYPE_FORMATSIZE];

			result = DNS_R_METATYPE;

			dns_rdatatype_format(type, typebuf, sizeof(typebuf));
			(*callbacks->error)(callbacks, "%s:%lu: %s '%s': %s",
					    source, line, "type", typebuf,
					    dns_result_totext(result));
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
			} else {
				goto insist_and_cleanup;
			}
		}

		/*
		 * Find a rdata structure.
		 */
		if (rdcount == rdata_size) {
			new_rdata = grow_rdata(rdata_size + RDSZ, rdata,
					       rdata_size, &current_list,
					       &glue_list, mctx);
			if (new_rdata == NULL) {
				result = ISC_R_NOMEMORY;
				goto log_and_cleanup;
			}
			rdata_size += RDSZ;
			rdata = new_rdata;
		}

		/*
		 * Peek at the NS record.
		 */
		if (type == dns_rdatatype_ns &&
		    lctx->zclass == dns_rdataclass_in &&
		    (lctx->options & DNS_MASTER_CHECKNS) != 0)
		{
			GETTOKEN(lctx->lex, 0, &token, false);
			result = check_ns(lctx, &token, source, line);
			isc_lex_ungettoken(lctx->lex, &token);
			if ((lctx->options & DNS_MASTER_FATALNS) != 0) {
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
				} else if (result != ISC_R_SUCCESS) {
					goto insist_and_cleanup;
				}
			}
		}

		/*
		 * Check owner name.
		 */
		options &= ~DNS_RDATA_CHECKREVERSE;
		if ((lctx->options & DNS_MASTER_CHECKNAMES) != 0) {
			bool ok;
			dns_name_t *name;

			name = (ictx->glue != NULL) ? ictx->glue
						    : ictx->current;
			ok = dns_rdata_checkowner(name, lctx->zclass, type,
						  true);
			if (!ok) {
				char namebuf[DNS_NAME_FORMATSIZE];
				const char *desc;
				dns_name_format(name, namebuf, sizeof(namebuf));
				result = DNS_R_BADOWNERNAME;
				desc = dns_result_totext(result);
				if (CHECKNAMESFAIL(lctx->options) ||
				    type == dns_rdatatype_nsec3)
				{
					(*callbacks->error)(
						callbacks, "%s:%lu: %s: %s",
						source, line, namebuf, desc);
					if (MANYERRS(lctx, result)) {
						SETRESULT(lctx, result);
					} else {
						goto cleanup;
					}
				} else {
					(*callbacks->warn)(
						callbacks, "%s:%lu: %s: %s",
						source, line, namebuf, desc);
				}
			}
			if (type == dns_rdatatype_ptr &&
			    !dns_name_isdnssd(name) &&
			    (dns_name_issubdomain(name, &in_addr_arpa) ||
			     dns_name_issubdomain(name, &ip6_arpa) ||
			     dns_name_issubdomain(name, &ip6_int)))
			{
				options |= DNS_RDATA_CHECKREVERSE;
			}
		}

		/*
		 * Read rdata contents.
		 */
		dns_rdata_init(&rdata[rdcount]);
		target_ft = target;
		result = dns_rdata_fromtext(&rdata[rdcount], lctx->zclass, type,
					    lctx->lex, ictx->origin, options,
					    lctx->mctx, &target, callbacks);
		if (MANYERRS(lctx, result)) {
			SETRESULT(lctx, result);
			continue;
		} else if (result != ISC_R_SUCCESS) {
			goto insist_and_cleanup;
		}

		if (ictx->drop) {
			target = target_ft;
			continue;
		}

		if (type == dns_rdatatype_soa &&
		    (lctx->options & DNS_MASTER_ZONE) != 0 &&
		    !dns_name_equal(ictx->current, lctx->top))
		{
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(ictx->current, namebuf,
					sizeof(namebuf));
			(*callbacks->error)(callbacks,
					    "%s:%lu: SOA "
					    "record not at top of zone (%s)",
					    source, line, namebuf);
			result = DNS_R_NOTZONETOP;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				read_till_eol = true;
				target = target_ft;
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		if (dns_rdatatype_atparent(type) &&
		    dns_master_isprimary(lctx) &&
		    dns_name_equal(ictx->current, lctx->top))
		{
			char namebuf[DNS_NAME_FORMATSIZE];
			char typebuf[DNS_RDATATYPE_FORMATSIZE];

			dns_name_format(ictx->current, namebuf,
					sizeof(namebuf));
			dns_rdatatype_format(type, typebuf, sizeof(typebuf));
			(*callbacks->error)(
				callbacks,
				"%s:%lu: %s record at top of zone (%s)", source,
				line, typebuf, namebuf);
			result = DNS_R_ATZONETOP;
			if (MANYERRS(lctx, result)) {
				SETRESULT(lctx, result);
				target = target_ft;
				continue;
			} else {
				goto insist_and_cleanup;
			}
		}

		if (type == dns_rdatatype_rrsig || type == dns_rdatatype_sig) {
			covers = dns_rdata_covers(&rdata[rdcount]);
		} else {
			covers = 0;
		}

		if (!lctx->ttl_known && !lctx->default_ttl_known) {
			if (type == dns_rdatatype_soa) {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: no TTL specified; "
						   "using SOA MINTTL instead",
						   source, line);
				lctx->ttl = dns_soa_getminimum(&rdata[rdcount]);
				limit_ttl(callbacks, source, line, &lctx->ttl);
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = true;
			} else if ((lctx->options & DNS_MASTER_HINT) != 0) {
				/*
				 * Zero TTL's are fine for hints.
				 */
				lctx->ttl = 0;
				lctx->default_ttl = lctx->ttl;
				lctx->default_ttl_known = true;
			} else {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: no TTL specified; "
						   "zone rejected",
						   source, line);
				result = DNS_R_NOTTL;
				if (MANYERRS(lctx, result)) {
					SETRESULT(lctx, result);
					lctx->ttl = 0;
				} else {
					goto insist_and_cleanup;
				}
			}
		} else if (!explicit_ttl && lctx->default_ttl_known) {
			lctx->ttl = lctx->default_ttl;
		} else if (!explicit_ttl && lctx->warn_1035) {
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "using RFC1035 TTL semantics",
					   source, line);
			lctx->warn_1035 = false;
		}

		if (type == dns_rdatatype_rrsig && lctx->warn_sigexpired) {
			dns_rdata_rrsig_t sig;
			result = dns_rdata_tostruct(&rdata[rdcount], &sig,
						    NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			if (isc_serial_lt(sig.timeexpire, lctx->now)) {
				(*callbacks->warn)(callbacks,
						   "%s:%lu: "
						   "signature has expired",
						   source, line);
				lctx->warn_sigexpired = false;
			}
		}

		if ((type == dns_rdatatype_sig || type == dns_rdatatype_nxt) &&
		    lctx->warn_tcr && dns_master_isprimary(lctx))
		{
			(*callbacks->warn)(callbacks,
					   "%s:%lu: old style DNSSEC "
					   " zone detected",
					   source, line);
			lctx->warn_tcr = false;
		}

		if ((lctx->options & DNS_MASTER_AGETTL) != 0) {
			/*
			 * Adjust the TTL for $DATE. If the RR has
			 * already expired, set its TTL to 0. This
			 * should be okay even if the TTL stretching
			 * feature is not in effect, because it will
			 * just be quickly expired by the cache, and the
			 * way this was written before the patch it
			 * could potentially add 0 TTLs anyway.
			 */
			if (lctx->ttl < ttl_offset) {
				lctx->ttl = 0;
			} else {
				lctx->ttl -= ttl_offset;
			}
		}

		/*
		 * Find type in rdatalist.
		 * If it does not exist create new one and prepend to list
		 * as this will minimise list traversal.
		 */
		if (ictx->glue != NULL) {
			this = ISC_LIST_HEAD(glue_list);
		} else {
			this = ISC_LIST_HEAD(current_list);
		}

		while (this != NULL) {
			if (this->type == type && this->covers == covers) {
				break;
			}
			this = ISC_LIST_NEXT(this, link);
		}

		if (this == NULL) {
			if (rdlcount == rdatalist_size) {
				new_rdatalist = grow_rdatalist(
					rdatalist_size + RDLSZ, rdatalist,
					rdatalist_size, &current_list,
					&glue_list, mctx);
				if (new_rdatalist == NULL) {
					result = ISC_R_NOMEMORY;
					goto log_and_cleanup;
				}
				rdatalist = new_rdatalist;
				rdatalist_size += RDLSZ;
			}
			this = &rdatalist[rdlcount++];
			dns_rdatalist_init(this);
			this->type = type;
			this->covers = covers;
			this->rdclass = lctx->zclass;
			this->ttl = lctx->ttl;
			if (ictx->glue != NULL) {
				ISC_LIST_INITANDPREPEND(glue_list, this, link);
			} else {
				ISC_LIST_INITANDPREPEND(current_list, this,
							link);
			}
		} else if (this->ttl != lctx->ttl) {
			(*callbacks->warn)(callbacks,
					   "%s:%lu: "
					   "TTL set to prior TTL (%lu)",
					   source, line, this->ttl);
			lctx->ttl = this->ttl;
		}

		if ((lctx->options & DNS_MASTER_CHECKTTL) != 0 &&
		    lctx->ttl > lctx->maxttl)
		{
			(callbacks->error)(callbacks,
					   "dns_master_load: %s:%lu: "
					   "TTL %d exceeds configured "
					   "max-zone-ttl %d",
					   source, line, lctx->ttl,
					   lctx->maxttl);
			result = ISC_R_RANGE;
			goto log_and_cleanup;
		}

		ISC_LIST_APPEND(this->rdata, &rdata[rdcount], link);
		if (ictx->glue != NULL) {
			ictx->glue_line = line;
		} else {
			ictx->current_line = line;
		}
		rdcount++;

		/*
		 * We must have at least 64k as rdlen is 16 bits.
		 * If we don't commit everything we have so far.
		 */
		if ((target.length - target.used) < MINTSIZ) {
			COMMITALL;
		}
	next_line:;
	} while (!done && (lctx->loop_cnt == 0 || loop_cnt++ < lctx->loop_cnt));

	/*
	 * Commit what has not yet been committed.
	 */
	result = commit(callbacks, lctx, &current_list, ictx->current, source,
			ictx->current_line);
	if (MANYERRS(lctx, result)) {
		SETRESULT(lctx, result);
	} else if (result != ISC_R_SUCCESS) {
		goto insist_and_cleanup;
	}
	result = commit(callbacks, lctx, &glue_list, ictx->glue, source,
			ictx->glue_line);
	if (MANYERRS(lctx, result)) {
		SETRESULT(lctx, result);
	} else if (result != ISC_R_SUCCESS) {
		goto insist_and_cleanup;
	}

	if (!done) {
		INSIST(lctx->done != NULL && lctx->task != NULL);
		result = DNS_R_CONTINUE;
	} else if (result == ISC_R_SUCCESS && lctx->result != ISC_R_SUCCESS) {
		result = lctx->result;
	} else if (result == ISC_R_SUCCESS && lctx->seen_include) {
		result = DNS_R_SEENINCLUDE;
	}
	goto cleanup;

log_and_cleanup:
	LOGIT(result);

insist_and_cleanup:
	INSIST(result != ISC_R_SUCCESS);

cleanup:
	while ((this = ISC_LIST_HEAD(current_list)) != NULL) {
		ISC_LIST_UNLINK(current_list, this, link);
	}
	while ((this = ISC_LIST_HEAD(glue_list)) != NULL) {
		ISC_LIST_UNLINK(glue_list, this, link);
	}
	if (rdatalist != NULL) {
		isc_mem_put(mctx, rdatalist,
			    rdatalist_size * sizeof(*rdatalist));
	}
	if (rdata != NULL) {
		isc_mem_put(mctx, rdata, rdata_size * sizeof(*rdata));
	}
	if (target_mem != NULL) {
		isc_mem_put(mctx, target_mem, target_size);
	}
	if (include_file != NULL) {
		isc_mem_free(mctx, include_file);
	}
	if (range != NULL) {
		isc_mem_free(mctx, range);
	}
	if (lhs != NULL) {
		isc_mem_free(mctx, lhs);
	}
	if (gtype != NULL) {
		isc_mem_free(mctx, gtype);
	}
	if (rhs != NULL) {
		isc_mem_free(mctx, rhs);
	}
	return (result);
}

static isc_result_t
pushfile(const char *master_file, dns_name_t *origin, dns_loadctx_t *lctx) {
	isc_result_t result;
	dns_incctx_t *ictx;
	dns_incctx_t *newctx = NULL;
	isc_region_t r;

	REQUIRE(master_file != NULL);
	REQUIRE(DNS_LCTX_VALID(lctx));

	ictx = lctx->inc;
	lctx->seen_include = true;

	result = incctx_create(lctx->mctx, origin, &newctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Push origin_changed.
	 */
	newctx->origin_changed = ictx->origin_changed;

	/* Set current domain. */
	if (ictx->glue != NULL || ictx->current != NULL) {
		newctx->current_in_use = find_free_name(newctx);
		newctx->current = dns_fixedname_name(
			&newctx->fixed[newctx->current_in_use]);
		newctx->in_use[newctx->current_in_use] = true;
		dns_name_toregion(
			(ictx->glue != NULL) ? ictx->glue : ictx->current, &r);
		dns_name_fromregion(newctx->current, &r);
		newctx->drop = ictx->drop;
	}

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	newctx->parent = ictx;
	lctx->inc = newctx;

	if (lctx->include_cb != NULL) {
		lctx->include_cb(master_file, lctx->include_arg);
	}
	return (ISC_R_SUCCESS);

cleanup:
	incctx_destroy(lctx->mctx, newctx);
	return (result);
}

/*
 * Fill/check exists buffer with 'len' bytes.  Track remaining bytes to be
 * read when incrementally filling the buffer.
 */
static isc_result_t
read_and_check(bool do_read, isc_buffer_t *buffer, size_t len, FILE *f,
	       uint32_t *totallen) {
	isc_result_t result;

	REQUIRE(totallen != NULL);

	if (do_read) {
		INSIST(isc_buffer_availablelength(buffer) >= len);
		result = isc_stdio_read(isc_buffer_used(buffer), 1, len, f,
					NULL);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		isc_buffer_add(buffer, (unsigned int)len);
		if (*totallen < len) {
			return (ISC_R_RANGE);
		}
		*totallen -= (uint32_t)len;
	} else if (isc_buffer_remaininglength(buffer) < len) {
		return (ISC_R_RANGE);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
load_header(dns_loadctx_t *lctx) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_masterrawheader_t header;
	dns_rdatacallbacks_t *callbacks;
	size_t commonlen = sizeof(header.format) + sizeof(header.version);
	size_t remainder;
	unsigned char data[sizeof(header)];
	isc_buffer_t target;

	REQUIRE(DNS_LCTX_VALID(lctx));

	if (lctx->format != dns_masterformat_raw &&
	    lctx->format != dns_masterformat_map)
	{
		return (ISC_R_NOTIMPLEMENTED);
	}

	callbacks = lctx->callbacks;
	dns_master_initrawheader(&header);

	INSIST(commonlen <= sizeof(header));
	isc_buffer_init(&target, data, sizeof(data));

	result = isc_stdio_read(data, 1, commonlen, lctx->f, NULL);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_stdio_read failed: %s",
				 isc_result_totext(result));
		return (result);
	}

	isc_buffer_add(&target, (unsigned int)commonlen);
	header.format = isc_buffer_getuint32(&target);
	if (header.format != lctx->format) {
		(*callbacks->error)(callbacks,
				    "dns_master_load: "
				    "file format mismatch (not %s)",
				    lctx->format == dns_masterformat_map ? "map"
									 : "ra"
									   "w");
		return (ISC_R_NOTIMPLEMENTED);
	}

	header.version = isc_buffer_getuint32(&target);

	switch (header.version) {
	case 0:
		remainder = sizeof(header.dumptime);
		break;
	case DNS_RAWFORMAT_VERSION:
		remainder = sizeof(header) - commonlen;
		break;
	default:
		(*callbacks->error)(callbacks, "dns_master_load: "
					       "unsupported file format "
					       "version");
		return (ISC_R_NOTIMPLEMENTED);
	}

	result = isc_stdio_read(data + commonlen, 1, remainder, lctx->f, NULL);
	if (result != ISC_R_SUCCESS) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_stdio_read failed: %s",
				 isc_result_totext(result));
		return (result);
	}

	isc_buffer_add(&target, (unsigned int)remainder);
	header.dumptime = isc_buffer_getuint32(&target);
	if (header.version == DNS_RAWFORMAT_VERSION) {
		header.flags = isc_buffer_getuint32(&target);
		header.sourceserial = isc_buffer_getuint32(&target);
		header.lastxfrin = isc_buffer_getuint32(&target);
	}

	lctx->first = false;
	lctx->header = header;

	return (ISC_R_SUCCESS);
}

static isc_result_t
openfile_map(dns_loadctx_t *lctx, const char *master_file) {
	isc_result_t result;

	result = isc_stdio_open(master_file, "rb", &lctx->f);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_stdio_open() failed: %s",
				 isc_result_totext(result));
	}

	return (result);
}

/*
 * Load a map format file, using mmap() to access RBT trees directly
 */
static isc_result_t
load_map(dns_loadctx_t *lctx) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdatacallbacks_t *callbacks;

	REQUIRE(DNS_LCTX_VALID(lctx));

	callbacks = lctx->callbacks;

	if (lctx->first) {
		result = load_header(lctx);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}

		result = (*callbacks->deserialize)(
			callbacks->deserialize_private, lctx->f,
			sizeof(dns_masterrawheader_t));
	}

	return (result);
}

static isc_result_t
openfile_raw(dns_loadctx_t *lctx, const char *master_file) {
	isc_result_t result;

	result = isc_stdio_open(master_file, "rb", &lctx->f);
	if (result != ISC_R_SUCCESS && result != ISC_R_FILENOTFOUND) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_stdio_open() failed: %s",
				 isc_result_totext(result));
	}

	return (result);
}

static isc_result_t
load_raw(dns_loadctx_t *lctx) {
	isc_result_t result = ISC_R_SUCCESS;
	bool done = false;
	unsigned int loop_cnt = 0;
	dns_rdatacallbacks_t *callbacks;
	unsigned char namebuf[DNS_NAME_MAXWIRE];
	dns_fixedname_t fixed;
	dns_name_t *name;
	rdatalist_head_t head, dummy;
	dns_rdatalist_t rdatalist;
	isc_mem_t *mctx = lctx->mctx;
	dns_rdata_t *rdata = NULL;
	unsigned int rdata_size = 0;
	int target_size = TSIZ;
	isc_buffer_t target, buf;
	unsigned char *target_mem = NULL;
	dns_decompress_t dctx;

	callbacks = lctx->callbacks;
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_NONE);

	if (lctx->first) {
		result = load_header(lctx);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	ISC_LIST_INIT(head);
	ISC_LIST_INIT(dummy);

	/*
	 * Allocate target_size of buffer space.  This is greater than twice
	 * the maximum individual RR data size.
	 */
	target_mem = isc_mem_get(mctx, target_size);
	isc_buffer_init(&target, target_mem, target_size);

	name = dns_fixedname_initname(&fixed);

	/*
	 * In the following loop, we regard any error fatal regardless of
	 * whether "MANYERRORS" is set in the context option.  This is because
	 * normal errors should already have been checked at creation time.
	 * Besides, it is very unlikely that we can recover from an error
	 * in this format, and so trying to continue parsing erroneous data
	 * does not really make sense.
	 */
	for (loop_cnt = 0; (lctx->loop_cnt == 0 || loop_cnt < lctx->loop_cnt);
	     loop_cnt++)
	{
		unsigned int i, rdcount;
		uint16_t namelen;
		uint32_t totallen;
		size_t minlen, readlen;
		bool sequential_read = false;

		/* Read the data length */
		isc_buffer_clear(&target);
		INSIST(isc_buffer_availablelength(&target) >= sizeof(totallen));
		result = isc_stdio_read(target.base, 1, sizeof(totallen),
					lctx->f, NULL);
		if (result == ISC_R_EOF) {
			result = ISC_R_SUCCESS;
			done = true;
			break;
		}
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		isc_buffer_add(&target, sizeof(totallen));
		totallen = isc_buffer_getuint32(&target);

		/*
		 * Validation: the input data must at least contain the common
		 * header.
		 */
		minlen = sizeof(totallen) + sizeof(uint16_t) +
			 sizeof(uint16_t) + sizeof(uint16_t) +
			 sizeof(uint32_t) + sizeof(uint32_t);
		if (totallen < minlen) {
			result = ISC_R_RANGE;
			goto cleanup;
		}
		totallen -= sizeof(totallen);

		isc_buffer_clear(&target);
		if (totallen > isc_buffer_availablelength(&target)) {
			/*
			 * The default buffer size should typically be large
			 * enough to store the entire RRset.  We could try to
			 * allocate enough space if this is not the case, but
			 * it might cause a hazardous result when "totallen"
			 * is forged.  Thus, we'd rather take an inefficient
			 * but robust approach in this atypical case: read
			 * data step by step, and commit partial data when
			 * necessary.  Note that the buffer must be large
			 * enough to store the "header part", owner name, and
			 * at least one rdata (however large it is).
			 */
			sequential_read = true;
			readlen = minlen - sizeof(totallen);
		} else {
			/*
			 * Typical case.  We can read the whole RRset at once
			 * with the default buffer.
			 */
			readlen = totallen;
		}
		result = isc_stdio_read(target.base, 1, readlen, lctx->f, NULL);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		isc_buffer_add(&target, (unsigned int)readlen);
		totallen -= (uint32_t)readlen;

		/* Construct RRset headers */
		dns_rdatalist_init(&rdatalist);
		rdatalist.rdclass = isc_buffer_getuint16(&target);
		if (lctx->zclass != rdatalist.rdclass) {
			result = DNS_R_BADCLASS;
			goto cleanup;
		}
		rdatalist.type = isc_buffer_getuint16(&target);
		rdatalist.covers = isc_buffer_getuint16(&target);
		rdatalist.ttl = isc_buffer_getuint32(&target);
		rdcount = isc_buffer_getuint32(&target);
		if (rdcount == 0 || rdcount > 0xffff) {
			result = ISC_R_RANGE;
			goto cleanup;
		}
		INSIST(isc_buffer_consumedlength(&target) <= readlen);

		/* Owner name: length followed by name */
		result = read_and_check(sequential_read, &target,
					sizeof(namelen), lctx->f, &totallen);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		namelen = isc_buffer_getuint16(&target);
		if (namelen > sizeof(namebuf)) {
			result = ISC_R_RANGE;
			goto cleanup;
		}

		result = read_and_check(sequential_read, &target, namelen,
					lctx->f, &totallen);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}

		isc_buffer_setactive(&target, (unsigned int)namelen);
		result = dns_name_fromwire(name, &target, &dctx, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}

		if ((lctx->options & DNS_MASTER_CHECKTTL) != 0 &&
		    rdatalist.ttl > lctx->maxttl)
		{
			(callbacks->error)(callbacks,
					   "dns_master_load: "
					   "TTL %d exceeds configured "
					   "max-zone-ttl %d",
					   rdatalist.ttl, lctx->maxttl);
			result = ISC_R_RANGE;
			goto cleanup;
		}

		/* Rdata contents. */
		if (rdcount > rdata_size) {
			dns_rdata_t *new_rdata = NULL;

			new_rdata = grow_rdata(rdcount + RDSZ, rdata,
					       rdata_size, &head, &dummy, mctx);
			if (new_rdata == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			rdata_size = rdcount + RDSZ;
			rdata = new_rdata;
		}

	continue_read:
		for (i = 0; i < rdcount; i++) {
			uint16_t rdlen;

			dns_rdata_init(&rdata[i]);

			if (sequential_read &&
			    isc_buffer_availablelength(&target) < MINTSIZ)
			{
				unsigned int j;

				INSIST(i > 0); /* detect an infinite loop */

				/* Partial Commit. */
				ISC_LIST_APPEND(head, &rdatalist, link);
				result = commit(callbacks, lctx, &head, name,
						NULL, 0);
				for (j = 0; j < i; j++) {
					ISC_LIST_UNLINK(rdatalist.rdata,
							&rdata[j], link);
					dns_rdata_reset(&rdata[j]);
				}
				if (result != ISC_R_SUCCESS) {
					goto cleanup;
				}

				/* Rewind the buffer and continue */
				isc_buffer_clear(&target);

				rdcount -= i;

				goto continue_read;
			}

			/* rdata length */
			result = read_and_check(sequential_read, &target,
						sizeof(rdlen), lctx->f,
						&totallen);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
			rdlen = isc_buffer_getuint16(&target);

			/* rdata */
			result = read_and_check(sequential_read, &target, rdlen,
						lctx->f, &totallen);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
			isc_buffer_setactive(&target, (unsigned int)rdlen);
			/*
			 * It is safe to have the source active region and
			 * the target available region be the same if
			 * decompression is disabled (see dctx above) and we
			 * are not downcasing names (options == 0).
			 */
			isc_buffer_init(&buf, isc_buffer_current(&target),
					(unsigned int)rdlen);
			result = dns_rdata_fromwire(
				&rdata[i], rdatalist.rdclass, rdatalist.type,
				&target, &dctx, 0, &buf);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
			ISC_LIST_APPEND(rdatalist.rdata, &rdata[i], link);
		}

		/*
		 * Sanity check.  Still having remaining space is not
		 * necessarily critical, but it very likely indicates broken
		 * or malformed data.
		 */
		if (isc_buffer_remaininglength(&target) != 0 || totallen != 0) {
			result = ISC_R_RANGE;
			goto cleanup;
		}

		ISC_LIST_APPEND(head, &rdatalist, link);

		/* Commit this RRset.  rdatalist will be unlinked. */
		result = commit(callbacks, lctx, &head, name, NULL, 0);

		for (i = 0; i < rdcount; i++) {
			ISC_LIST_UNLINK(rdatalist.rdata, &rdata[i], link);
			dns_rdata_reset(&rdata[i]);
		}

		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
	}

	if (!done) {
		INSIST(lctx->done != NULL && lctx->task != NULL);
		result = DNS_R_CONTINUE;
	} else if (result == ISC_R_SUCCESS && lctx->result != ISC_R_SUCCESS) {
		result = lctx->result;
	}

	if (result == ISC_R_SUCCESS && callbacks->rawdata != NULL) {
		(*callbacks->rawdata)(callbacks->zone, &lctx->header);
	}

cleanup:
	if (rdata != NULL) {
		isc_mem_put(mctx, rdata, rdata_size * sizeof(*rdata));
	}
	if (target_mem != NULL) {
		isc_mem_put(mctx, target_mem, target_size);
	}
	if (result != ISC_R_SUCCESS && result != DNS_R_CONTINUE) {
		(*callbacks->error)(callbacks, "dns_master_load: %s",
				    dns_result_totext(result));
	}

	return (result);
}

isc_result_t
dns_master_loadfile(const char *master_file, dns_name_t *top,
		    dns_name_t *origin, dns_rdataclass_t zclass,
		    unsigned int options, uint32_t resign,
		    dns_rdatacallbacks_t *callbacks,
		    dns_masterincludecb_t include_cb, void *include_arg,
		    isc_mem_t *mctx, dns_masterformat_t format,
		    dns_ttl_t maxttl) {
	dns_loadctx_t *lctx = NULL;
	isc_result_t result;

	result = loadctx_create(format, mctx, options, resign, top, zclass,
				origin, callbacks, NULL, NULL, NULL, include_cb,
				include_arg, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	lctx->maxttl = maxttl;

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadfileinc(const char *master_file, dns_name_t *top,
		       dns_name_t *origin, dns_rdataclass_t zclass,
		       unsigned int options, uint32_t resign,
		       dns_rdatacallbacks_t *callbacks, isc_task_t *task,
		       dns_loaddonefunc_t done, void *done_arg,
		       dns_loadctx_t **lctxp, dns_masterincludecb_t include_cb,
		       void *include_arg, isc_mem_t *mctx,
		       dns_masterformat_t format, uint32_t maxttl) {
	dns_loadctx_t *lctx = NULL;
	isc_result_t result;

	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(format, mctx, options, resign, top, zclass,
				origin, callbacks, task, done, done_arg,
				include_cb, include_arg, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	lctx->maxttl = maxttl;

	result = (lctx->openfile)(lctx, master_file);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadstream(FILE *stream, dns_name_t *top, dns_name_t *origin,
		      dns_rdataclass_t zclass, unsigned int options,
		      dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(stream != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				NULL, NULL, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = isc_lex_openstream(lctx->lex, stream);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

cleanup:
	if (lctx != NULL) {
		dns_loadctx_detach(&lctx);
	}
	return (result);
}

isc_result_t
dns_master_loadstreaminc(FILE *stream, dns_name_t *top, dns_name_t *origin,
			 dns_rdataclass_t zclass, unsigned int options,
			 dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			 dns_loaddonefunc_t done, void *done_arg,
			 dns_loadctx_t **lctxp, isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(stream != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done, done_arg,
				NULL, NULL, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = isc_lex_openstream(lctx->lex, stream);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

cleanup:
	if (lctx != NULL) {
		dns_loadctx_detach(&lctx);
	}
	return (result);
}

isc_result_t
dns_master_loadbuffer(isc_buffer_t *buffer, dns_name_t *top, dns_name_t *origin,
		      dns_rdataclass_t zclass, unsigned int options,
		      dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(buffer != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				NULL, NULL, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = isc_lex_openbuffer(lctx->lex, buffer);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadbufferinc(isc_buffer_t *buffer, dns_name_t *top,
			 dns_name_t *origin, dns_rdataclass_t zclass,
			 unsigned int options, dns_rdatacallbacks_t *callbacks,
			 isc_task_t *task, dns_loaddonefunc_t done,
			 void *done_arg, dns_loadctx_t **lctxp,
			 isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(buffer != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done, done_arg,
				NULL, NULL, NULL, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = isc_lex_openbuffer(lctx->lex, buffer);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

cleanup:
	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadlexer(isc_lex_t *lex, dns_name_t *top, dns_name_t *origin,
		     dns_rdataclass_t zclass, unsigned int options,
		     dns_rdatacallbacks_t *callbacks, isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(lex != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, NULL, NULL, NULL,
				NULL, NULL, lex, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = (lctx->load)(lctx);
	INSIST(result != DNS_R_CONTINUE);

	dns_loadctx_detach(&lctx);
	return (result);
}

isc_result_t
dns_master_loadlexerinc(isc_lex_t *lex, dns_name_t *top, dns_name_t *origin,
			dns_rdataclass_t zclass, unsigned int options,
			dns_rdatacallbacks_t *callbacks, isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **lctxp, isc_mem_t *mctx) {
	isc_result_t result;
	dns_loadctx_t *lctx = NULL;

	REQUIRE(lex != NULL);
	REQUIRE(task != NULL);
	REQUIRE(done != NULL);

	result = loadctx_create(dns_masterformat_text, mctx, options, 0, top,
				zclass, origin, callbacks, task, done, done_arg,
				NULL, NULL, lex, &lctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = task_send(lctx);
	if (result == ISC_R_SUCCESS) {
		dns_loadctx_attach(lctx, lctxp);
		return (DNS_R_CONTINUE);
	}

	dns_loadctx_detach(&lctx);
	return (result);
}

/*
 * Grow the slab of dns_rdatalist_t structures.
 * Re-link glue and current list.
 */
static dns_rdatalist_t *
grow_rdatalist(int new_len, dns_rdatalist_t *oldlist, int old_len,
	       rdatalist_head_t *current, rdatalist_head_t *glue,
	       isc_mem_t *mctx) {
	dns_rdatalist_t *newlist;
	int rdlcount = 0;
	ISC_LIST(dns_rdatalist_t) save;
	dns_rdatalist_t *this;

	newlist = isc_mem_get(mctx, new_len * sizeof(*newlist));
	if (newlist == NULL) {
		return (NULL);
	}

	ISC_LIST_INIT(save);
	while ((this = ISC_LIST_HEAD(*current)) != NULL) {
		ISC_LIST_UNLINK(*current, this, link);
		ISC_LIST_APPEND(save, this, link);
	}
	while ((this = ISC_LIST_HEAD(save)) != NULL) {
		ISC_LIST_UNLINK(save, this, link);
		INSIST(rdlcount < new_len);
		newlist[rdlcount] = *this;
		ISC_LIST_APPEND(*current, &newlist[rdlcount], link);
		rdlcount++;
	}

	ISC_LIST_INIT(save);
	while ((this = ISC_LIST_HEAD(*glue)) != NULL) {
		ISC_LIST_UNLINK(*glue, this, link);
		ISC_LIST_APPEND(save, this, link);
	}
	while ((this = ISC_LIST_HEAD(save)) != NULL) {
		ISC_LIST_UNLINK(save, this, link);
		INSIST(rdlcount < new_len);
		newlist[rdlcount] = *this;
		ISC_LIST_APPEND(*glue, &newlist[rdlcount], link);
		rdlcount++;
	}

	INSIST(rdlcount == old_len);
	if (oldlist != NULL) {
		isc_mem_put(mctx, oldlist, old_len * sizeof(*oldlist));
	}
	return (newlist);
}

/*
 * Grow the slab of rdata structs.
 * Re-link the current and glue chains.
 */
static dns_rdata_t *
grow_rdata(int new_len, dns_rdata_t *oldlist, int old_len,
	   rdatalist_head_t *current, rdatalist_head_t *glue, isc_mem_t *mctx) {
	dns_rdata_t *newlist;
	int rdcount = 0;
	ISC_LIST(dns_rdata_t) save;
	dns_rdatalist_t *this;
	dns_rdata_t *rdata;

	newlist = isc_mem_get(mctx, new_len * sizeof(*newlist));
	if (newlist == NULL) {
		return (NULL);
	}
	memset(newlist, 0, new_len * sizeof(*newlist));

	/*
	 * Copy current relinking.
	 */
	this = ISC_LIST_HEAD(*current);
	while (this != NULL) {
		ISC_LIST_INIT(save);
		while ((rdata = ISC_LIST_HEAD(this->rdata)) != NULL) {
			ISC_LIST_UNLINK(this->rdata, rdata, link);
			ISC_LIST_APPEND(save, rdata, link);
		}
		while ((rdata = ISC_LIST_HEAD(save)) != NULL) {
			ISC_LIST_UNLINK(save, rdata, link);
			INSIST(rdcount < new_len);
			newlist[rdcount] = *rdata;
			ISC_LIST_APPEND(this->rdata, &newlist[rdcount], link);
			rdcount++;
		}
		this = ISC_LIST_NEXT(this, link);
	}

	/*
	 * Copy glue relinking.
	 */
	this = ISC_LIST_HEAD(*glue);
	while (this != NULL) {
		ISC_LIST_INIT(save);
		while ((rdata = ISC_LIST_HEAD(this->rdata)) != NULL) {
			ISC_LIST_UNLINK(this->rdata, rdata, link);
			ISC_LIST_APPEND(save, rdata, link);
		}
		while ((rdata = ISC_LIST_HEAD(save)) != NULL) {
			ISC_LIST_UNLINK(save, rdata, link);
			INSIST(rdcount < new_len);
			newlist[rdcount] = *rdata;
			ISC_LIST_APPEND(this->rdata, &newlist[rdcount], link);
			rdcount++;
		}
		this = ISC_LIST_NEXT(this, link);
	}
	INSIST(rdcount == old_len || rdcount == 0);
	if (oldlist != NULL) {
		isc_mem_put(mctx, oldlist, old_len * sizeof(*oldlist));
	}
	return (newlist);
}

static uint32_t
resign_fromlist(dns_rdatalist_t *this, dns_loadctx_t *lctx) {
	dns_rdata_t *rdata;
	dns_rdata_rrsig_t sig;
	uint32_t when;

	rdata = ISC_LIST_HEAD(this->rdata);
	INSIST(rdata != NULL);
	(void)dns_rdata_tostruct(rdata, &sig, NULL);
	if (isc_serial_gt(sig.timesigned, lctx->now)) {
		when = lctx->now;
	} else {
		when = sig.timeexpire - lctx->resign;
	}

	rdata = ISC_LIST_NEXT(rdata, link);
	while (rdata != NULL) {
		(void)dns_rdata_tostruct(rdata, &sig, NULL);
		if (isc_serial_gt(sig.timesigned, lctx->now)) {
			when = lctx->now;
		} else if (sig.timeexpire - lctx->resign < when) {
			when = sig.timeexpire - lctx->resign;
		}
		rdata = ISC_LIST_NEXT(rdata, link);
	}
	return (when);
}

/*
 * Convert each element from a rdatalist_t to rdataset then call commit.
 * Unlink each element as we go.
 */

static isc_result_t
commit(dns_rdatacallbacks_t *callbacks, dns_loadctx_t *lctx,
       rdatalist_head_t *head, dns_name_t *owner, const char *source,
       unsigned int line) {
	dns_rdatalist_t *this;
	dns_rdataset_t dataset;
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];
	void (*error)(struct dns_rdatacallbacks *, const char *, ...);

	this = ISC_LIST_HEAD(*head);
	error = callbacks->error;

	if (this == NULL) {
		return (ISC_R_SUCCESS);
	}
	do {
		dns_rdataset_init(&dataset);
		RUNTIME_CHECK(dns_rdatalist_tordataset(this, &dataset) ==
			      ISC_R_SUCCESS);
		dataset.trust = dns_trust_ultimate;
		/*
		 * If this is a secure dynamic zone set the re-signing time.
		 */
		if (dataset.type == dns_rdatatype_rrsig &&
		    (lctx->options & DNS_MASTER_RESIGN) != 0)
		{
			dataset.attributes |= DNS_RDATASETATTR_RESIGN;
			dataset.resign = resign_fromlist(this, lctx);
		}
		result = ((*callbacks->add)(callbacks->add_private, owner,
					    &dataset));
		if (result == ISC_R_NOMEMORY) {
			(*error)(callbacks, "dns_master_load: %s",
				 dns_result_totext(result));
		} else if (result != ISC_R_SUCCESS) {
			dns_name_format(owner, namebuf, sizeof(namebuf));
			if (source != NULL) {
				(*error)(callbacks, "%s: %s:%lu: %s: %s",
					 "dns_master_load", source, line,
					 namebuf, dns_result_totext(result));
			} else {
				(*error)(callbacks, "%s: %s: %s",
					 "dns_master_load", namebuf,
					 dns_result_totext(result));
			}
		}
		if (MANYERRS(lctx, result)) {
			SETRESULT(lctx, result);
		} else if (result != ISC_R_SUCCESS) {
			return (result);
		}
		ISC_LIST_UNLINK(*head, this, link);
		this = ISC_LIST_HEAD(*head);
	} while (this != NULL);
	return (ISC_R_SUCCESS);
}

/*
 * Returns true if one of the NS rdata's contains 'owner'.
 */

static bool
is_glue(rdatalist_head_t *head, dns_name_t *owner) {
	dns_rdatalist_t *this;
	dns_rdata_t *rdata;
	isc_region_t region;
	dns_name_t name;

	/*
	 * Find NS rrset.
	 */
	this = ISC_LIST_HEAD(*head);
	while (this != NULL) {
		if (this->type == dns_rdatatype_ns) {
			break;
		}
		this = ISC_LIST_NEXT(this, link);
	}
	if (this == NULL) {
		return (false);
	}

	rdata = ISC_LIST_HEAD(this->rdata);
	while (rdata != NULL) {
		dns_name_init(&name, NULL);
		dns_rdata_toregion(rdata, &region);
		dns_name_fromregion(&name, &region);
		if (dns_name_equal(&name, owner)) {
			return (true);
		}
		rdata = ISC_LIST_NEXT(rdata, link);
	}
	return (false);
}

static void
load_quantum(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	dns_loadctx_t *lctx;

	REQUIRE(event != NULL);
	lctx = event->ev_arg;
	REQUIRE(DNS_LCTX_VALID(lctx));

	if (atomic_load_acquire(&lctx->canceled)) {
		result = ISC_R_CANCELED;
	} else {
		result = (lctx->load)(lctx);
	}
	if (result == DNS_R_CONTINUE) {
		event->ev_arg = lctx;
		isc_task_send(task, &event);
	} else {
		(lctx->done)(lctx->done_arg, result);
		isc_event_free(&event);
		dns_loadctx_detach(&lctx);
	}
}

static isc_result_t
task_send(dns_loadctx_t *lctx) {
	isc_event_t *event;

	event = isc_event_allocate(lctx->mctx, NULL, DNS_EVENT_MASTERQUANTUM,
				   load_quantum, lctx, sizeof(*event));
	isc_task_send(lctx->task, &event);
	return (ISC_R_SUCCESS);
}

void
dns_loadctx_cancel(dns_loadctx_t *lctx) {
	REQUIRE(DNS_LCTX_VALID(lctx));

	atomic_store_release(&lctx->canceled, true);
}

void
dns_master_initrawheader(dns_masterrawheader_t *header) {
	memset(header, 0, sizeof(dns_masterrawheader_t));
}
