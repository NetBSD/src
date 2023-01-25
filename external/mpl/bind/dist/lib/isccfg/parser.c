/*	$NetBSD: parser.c,v 1.13 2023/01/25 21:43:32 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND BSD-2-Clause
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * duration_fromtext initially taken from OpenDNSSEC code base.
 * Modified to fit the BIND 9 code.
 *
 * Copyright (c) 2009-2018 NLNet Labs.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*! \file */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/formatcheck.h>
#include <isc/lex.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/netscope.h>
#include <isc/print.h>
#include <isc/sockaddr.h>
#include <isc/string.h>
#include <isc/symtab.h>
#include <isc/util.h>

#include <dns/ttl.h>

#include <isccfg/cfg.h>
#include <isccfg/grammar.h>
#include <isccfg/log.h>

/* Shorthand */
#define CAT CFG_LOGCATEGORY_CONFIG
#define MOD CFG_LOGMODULE_PARSER

#define MAP_SYM 1 /* Unique type for isc_symtab */

#define TOKEN_STRING(pctx) (pctx->token.value.as_textregion.base)

/* Check a return value. */
#define CHECK(op)                            \
	do {                                 \
		result = (op);               \
		if (result != ISC_R_SUCCESS) \
			goto cleanup;        \
	} while (0)

/* Clean up a configuration object if non-NULL. */
#define CLEANUP_OBJ(obj)                               \
	do {                                           \
		if ((obj) != NULL)                     \
			cfg_obj_destroy(pctx, &(obj)); \
	} while (0)

/*
 * Forward declarations of static functions.
 */

static void
free_tuple(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
parse_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static void
print_list(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
free_list(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
create_listelt(cfg_parser_t *pctx, cfg_listelt_t **eltp);

static isc_result_t
create_string(cfg_parser_t *pctx, const char *contents, const cfg_type_t *type,
	      cfg_obj_t **ret);

static void
free_string(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
create_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **objp);

static void
free_map(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
parse_symtab_elt(cfg_parser_t *pctx, const char *name, cfg_type_t *elttype,
		 isc_symtab_t *symtab, bool callback);

static void
free_noop(cfg_parser_t *pctx, cfg_obj_t *obj);

static isc_result_t
cfg_getstringtoken(cfg_parser_t *pctx);

static void
parser_complain(cfg_parser_t *pctx, bool is_warning, unsigned int flags,
		const char *format, va_list args);

#if defined(HAVE_GEOIP2)
static isc_result_t
parse_geoip(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret);

static void
print_geoip(cfg_printer_t *pctx, const cfg_obj_t *obj);

static void
doc_geoip(cfg_printer_t *pctx, const cfg_type_t *type);
#endif /* HAVE_GEOIP2 */

/*
 * Data representations.  These correspond to members of the
 * "value" union in struct cfg_obj (except "void", which does
 * not need a union member).
 */

LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_uint32 = { "uint32", free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_uint64 = { "uint64", free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_string = { "string", free_string };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_boolean = { "boolean", free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_map = { "map", free_map };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_list = { "list", free_list };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_tuple = { "tuple", free_tuple };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_sockaddr = { "sockaddr", free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_netprefix = { "netprefix",
							free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_void = { "void", free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_fixedpoint = { "fixedpoint",
							 free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_percentage = { "percentage",
							 free_noop };
LIBISCCFG_EXTERNAL_DATA cfg_rep_t cfg_rep_duration = { "duration", free_noop };

/*
 * Configuration type definitions.
 */

/*%
 * An implicit list.  These are formed by clauses that occur multiple times.
 */
static cfg_type_t cfg_type_implicitlist = { "implicitlist", NULL,
					    print_list,	    NULL,
					    &cfg_rep_list,  NULL };

/* Functions. */

void
cfg_print_obj(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	obj->type->print(pctx, obj);
}

void
cfg_print_chars(cfg_printer_t *pctx, const char *text, int len) {
	REQUIRE(pctx != NULL);
	REQUIRE(text != NULL);

	pctx->f(pctx->closure, text, len);
}

static void
print_open(cfg_printer_t *pctx) {
	if ((pctx->flags & CFG_PRINTER_ONELINE) != 0) {
		cfg_print_cstr(pctx, "{ ");
	} else {
		cfg_print_cstr(pctx, "{\n");
		pctx->indent++;
	}
}

void
cfg_print_indent(cfg_printer_t *pctx) {
	int indent = pctx->indent;
	if ((pctx->flags & CFG_PRINTER_ONELINE) != 0) {
		cfg_print_cstr(pctx, " ");
		return;
	}
	while (indent > 0) {
		cfg_print_cstr(pctx, "\t");
		indent--;
	}
}

static void
print_close(cfg_printer_t *pctx) {
	if ((pctx->flags & CFG_PRINTER_ONELINE) == 0) {
		pctx->indent--;
		cfg_print_indent(pctx);
	}
	cfg_print_cstr(pctx, "}");
}

isc_result_t
cfg_parse_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	result = type->parse(pctx, type, ret);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	ENSURE(*ret != NULL);
	return (ISC_R_SUCCESS);
}

void
cfg_print(const cfg_obj_t *obj,
	  void (*f)(void *closure, const char *text, int textlen),
	  void *closure) {
	REQUIRE(obj != NULL);
	REQUIRE(f != NULL);

	cfg_printx(obj, 0, f, closure);
}

void
cfg_printx(const cfg_obj_t *obj, unsigned int flags,
	   void (*f)(void *closure, const char *text, int textlen),
	   void *closure) {
	cfg_printer_t pctx;

	REQUIRE(obj != NULL);
	REQUIRE(f != NULL);

	pctx.f = f;
	pctx.closure = closure;
	pctx.indent = 0;
	pctx.flags = flags;
	obj->type->print(&pctx, obj);
}

/* Tuples. */

isc_result_t
cfg_create_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;
	cfg_obj_t *obj = NULL;
	unsigned int nfields = 0;
	int i;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	fields = type->of;

	for (f = fields; f->name != NULL; f++) {
		nfields++;
	}

	CHECK(cfg_create_obj(pctx, type, &obj));
	obj->value.tuple = isc_mem_get(pctx->mctx,
				       nfields * sizeof(cfg_obj_t *));
	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		obj->value.tuple[i] = NULL;
	}
	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	if (obj != NULL) {
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	}
	return (result);
}

isc_result_t
cfg_parse_tuple(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;
	cfg_obj_t *obj = NULL;
	unsigned int i;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	fields = type->of;

	CHECK(cfg_create_tuple(pctx, type, &obj));
	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		CHECK(cfg_parse_obj(pctx, f->type, &obj->value.tuple[i]));
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

void
cfg_print_tuple(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;
	bool need_space = false;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	fields = obj->type->of;

	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		const cfg_obj_t *fieldobj = obj->value.tuple[i];
		if (need_space && fieldobj->type->rep != &cfg_rep_void) {
			cfg_print_cstr(pctx, " ");
		}
		cfg_print_obj(pctx, fieldobj);
		need_space = (need_space ||
			      fieldobj->type->print != cfg_print_void);
	}
}

void
cfg_doc_tuple(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;
	bool need_space = false;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	fields = type->of;

	for (f = fields; f->name != NULL; f++) {
		if (need_space) {
			cfg_print_cstr(pctx, " ");
		}
		cfg_doc_obj(pctx, f->type);
		need_space = (f->type->print != cfg_print_void);
	}
}

static void
free_tuple(cfg_parser_t *pctx, cfg_obj_t *obj) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields = obj->type->of;
	const cfg_tuplefielddef_t *f;
	unsigned int nfields = 0;

	if (obj->value.tuple == NULL) {
		return;
	}

	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		CLEANUP_OBJ(obj->value.tuple[i]);
		nfields++;
	}
	isc_mem_put(pctx->mctx, obj->value.tuple,
		    nfields * sizeof(cfg_obj_t *));
}

bool
cfg_obj_istuple(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_tuple);
}

const cfg_obj_t *
cfg_tuple_get(const cfg_obj_t *tupleobj, const char *name) {
	unsigned int i;
	const cfg_tuplefielddef_t *fields;
	const cfg_tuplefielddef_t *f;

	REQUIRE(tupleobj != NULL && tupleobj->type->rep == &cfg_rep_tuple);
	REQUIRE(name != NULL);

	fields = tupleobj->type->of;
	for (f = fields, i = 0; f->name != NULL; f++, i++) {
		if (strcmp(f->name, name) == 0) {
			return (tupleobj->value.tuple[i]);
		}
	}
	UNREACHABLE();
}

isc_result_t
cfg_parse_special(cfg_parser_t *pctx, int special) {
	isc_result_t result;

	REQUIRE(pctx != NULL);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == special)
	{
		return (ISC_R_SUCCESS);
	}

	cfg_parser_error(pctx, CFG_LOG_NEAR, "'%c' expected", special);
	return (ISC_R_UNEXPECTEDTOKEN);
cleanup:
	return (result);
}

/*
 * Parse a required semicolon.  If it is not there, log
 * an error and increment the error count but continue
 * parsing.  Since the next token is pushed back,
 * care must be taken to make sure it is eventually
 * consumed or an infinite loop may result.
 */
static isc_result_t
parse_semicolon(cfg_parser_t *pctx) {
	isc_result_t result;

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == ';')
	{
		return (ISC_R_SUCCESS);
	}

	cfg_parser_error(pctx, CFG_LOG_BEFORE, "missing ';'");
	cfg_ungettoken(pctx);
cleanup:
	return (result);
}

/*
 * Parse EOF, logging and returning an error if not there.
 */
static isc_result_t
parse_eof(cfg_parser_t *pctx) {
	isc_result_t result;

	CHECK(cfg_gettoken(pctx, 0));

	if (pctx->token.type == isc_tokentype_eof) {
		return (ISC_R_SUCCESS);
	}

	cfg_parser_error(pctx, CFG_LOG_NEAR, "syntax error");
	return (ISC_R_UNEXPECTEDTOKEN);
cleanup:
	return (result);
}

/* A list of files, used internally for pctx->files. */

static cfg_type_t cfg_type_filelist = { "filelist",    NULL,
					print_list,    NULL,
					&cfg_rep_list, &cfg_type_qstring };

isc_result_t
cfg_parser_create(isc_mem_t *mctx, isc_log_t *lctx, cfg_parser_t **ret) {
	isc_result_t result;
	cfg_parser_t *pctx;
	isc_lexspecials_t specials;

	REQUIRE(mctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	pctx = isc_mem_get(mctx, sizeof(*pctx));

	pctx->mctx = NULL;
	isc_mem_attach(mctx, &pctx->mctx);

	isc_refcount_init(&pctx->references, 1);

	pctx->lctx = lctx;
	pctx->lexer = NULL;
	pctx->seen_eof = false;
	pctx->ungotten = false;
	pctx->errors = 0;
	pctx->warnings = 0;
	pctx->open_files = NULL;
	pctx->closed_files = NULL;
	pctx->line = 0;
	pctx->callback = NULL;
	pctx->callbackarg = NULL;
	pctx->token.type = isc_tokentype_unknown;
	pctx->flags = 0;
	pctx->buf_name = NULL;

	memset(specials, 0, sizeof(specials));
	specials['{'] = 1;
	specials['}'] = 1;
	specials[';'] = 1;
	specials['/'] = 1;
	specials['"'] = 1;
	specials['!'] = 1;

	CHECK(isc_lex_create(pctx->mctx, 1024, &pctx->lexer));

	isc_lex_setspecials(pctx->lexer, specials);
	isc_lex_setcomments(pctx->lexer,
			    (ISC_LEXCOMMENT_C | ISC_LEXCOMMENT_CPLUSPLUS |
			     ISC_LEXCOMMENT_SHELL));

	CHECK(cfg_create_list(pctx, &cfg_type_filelist, &pctx->open_files));
	CHECK(cfg_create_list(pctx, &cfg_type_filelist, &pctx->closed_files));

	*ret = pctx;
	return (ISC_R_SUCCESS);

cleanup:
	if (pctx->lexer != NULL) {
		isc_lex_destroy(&pctx->lexer);
	}
	CLEANUP_OBJ(pctx->open_files);
	CLEANUP_OBJ(pctx->closed_files);
	isc_mem_putanddetach(&pctx->mctx, pctx, sizeof(*pctx));
	return (result);
}

void
cfg_parser_setflags(cfg_parser_t *pctx, unsigned int flags, bool turn_on) {
	REQUIRE(pctx != NULL);

	if (turn_on) {
		pctx->flags |= flags;
	} else {
		pctx->flags &= ~flags;
	}
}

static isc_result_t
parser_openfile(cfg_parser_t *pctx, const char *filename) {
	isc_result_t result;
	cfg_listelt_t *elt = NULL;
	cfg_obj_t *stringobj = NULL;

	result = isc_lex_openfile(pctx->lexer, filename);
	if (result != ISC_R_SUCCESS) {
		cfg_parser_error(pctx, 0, "open: %s: %s", filename,
				 isc_result_totext(result));
		goto cleanup;
	}

	CHECK(create_string(pctx, filename, &cfg_type_qstring, &stringobj));
	CHECK(create_listelt(pctx, &elt));
	elt->obj = stringobj;
	ISC_LIST_APPEND(pctx->open_files->value.list, elt, link);

	return (ISC_R_SUCCESS);
cleanup:
	CLEANUP_OBJ(stringobj);
	return (result);
}

void
cfg_parser_setcallback(cfg_parser_t *pctx, cfg_parsecallback_t callback,
		       void *arg) {
	REQUIRE(pctx != NULL);

	pctx->callback = callback;
	pctx->callbackarg = arg;
}

void
cfg_parser_reset(cfg_parser_t *pctx) {
	REQUIRE(pctx != NULL);

	if (pctx->lexer != NULL) {
		isc_lex_close(pctx->lexer);
	}

	pctx->seen_eof = false;
	pctx->ungotten = false;
	pctx->errors = 0;
	pctx->warnings = 0;
	pctx->line = 0;
}

/*
 * Parse a configuration using a pctx where a lexer has already
 * been set up with a source.
 */
static isc_result_t
parse2(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	result = cfg_parse_obj(pctx, type, &obj);

	if (pctx->errors != 0) {
		/* Errors have been logged. */
		if (result == ISC_R_SUCCESS) {
			result = ISC_R_FAILURE;
		}
		goto cleanup;
	}

	if (result != ISC_R_SUCCESS) {
		/* Parsing failed but no errors have been logged. */
		cfg_parser_error(pctx, 0, "parsing failed: %s",
				 isc_result_totext(result));
		goto cleanup;
	}

	CHECK(parse_eof(pctx));

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

isc_result_t
cfg_parse_file(cfg_parser_t *pctx, const char *filename, const cfg_type_t *type,
	       cfg_obj_t **ret) {
	isc_result_t result;
	cfg_listelt_t *elt;

	REQUIRE(pctx != NULL);
	REQUIRE(filename != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(parser_openfile(pctx, filename));

	result = parse2(pctx, type, ret);

	/* Clean up the opened file */
	elt = ISC_LIST_TAIL(pctx->open_files->value.list);
	INSIST(elt != NULL);
	ISC_LIST_UNLINK(pctx->open_files->value.list, elt, link);
	ISC_LIST_APPEND(pctx->closed_files->value.list, elt, link);

cleanup:
	return (result);
}

isc_result_t
cfg_parse_buffer(cfg_parser_t *pctx, isc_buffer_t *buffer, const char *file,
		 unsigned int line, const cfg_type_t *type, unsigned int flags,
		 cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(buffer != NULL);
	REQUIRE(ret != NULL && *ret == NULL);
	REQUIRE((flags & ~(CFG_PCTX_NODEPRECATED)) == 0);

	CHECK(isc_lex_openbuffer(pctx->lexer, buffer));

	pctx->buf_name = file;
	pctx->flags = flags;

	if (line != 0U) {
		CHECK(isc_lex_setsourceline(pctx->lexer, line));
	}

	CHECK(parse2(pctx, type, ret));
	pctx->buf_name = NULL;

cleanup:
	return (result);
}

void
cfg_parser_attach(cfg_parser_t *src, cfg_parser_t **dest) {
	REQUIRE(src != NULL);
	REQUIRE(dest != NULL && *dest == NULL);

	isc_refcount_increment(&src->references);
	*dest = src;
}

void
cfg_parser_destroy(cfg_parser_t **pctxp) {
	cfg_parser_t *pctx;

	REQUIRE(pctxp != NULL && *pctxp != NULL);
	pctx = *pctxp;
	*pctxp = NULL;

	if (isc_refcount_decrement(&pctx->references) == 1) {
		isc_lex_destroy(&pctx->lexer);
		/*
		 * Cleaning up open_files does not
		 * close the files; that was already done
		 * by closing the lexer.
		 */
		CLEANUP_OBJ(pctx->open_files);
		CLEANUP_OBJ(pctx->closed_files);
		isc_mem_putanddetach(&pctx->mctx, pctx, sizeof(*pctx));
	}
}

/*
 * void
 */
isc_result_t
cfg_parse_void(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	return (cfg_create_obj(pctx, &cfg_type_void, ret));
}

void
cfg_print_void(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	UNUSED(pctx);
	UNUSED(obj);
}

void
cfg_doc_void(cfg_printer_t *pctx, const cfg_type_t *type) {
	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	UNUSED(pctx);
	UNUSED(type);
}

bool
cfg_obj_isvoid(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_void);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_void = {
	"void",	      cfg_parse_void, cfg_print_void,
	cfg_doc_void, &cfg_rep_void,  NULL
};

/*
 * percentage
 */
isc_result_t
cfg_parse_percentage(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	char *endp;
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	uint64_t percent;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected percentage");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	percent = strtoull(TOKEN_STRING(pctx), &endp, 10);
	if (*endp != '%' || *(endp + 1) != 0) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected percentage");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_percentage, &obj));
	obj->value.uint32 = (uint32_t)percent;
	*ret = obj;

cleanup:
	return (result);
}

void
cfg_print_percentage(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	char buf[64];
	int n;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	n = snprintf(buf, sizeof(buf), "%u%%", obj->value.uint32);
	INSIST(n > 0 && (size_t)n < sizeof(buf));
	cfg_print_chars(pctx, buf, strlen(buf));
}

uint32_t
cfg_obj_aspercentage(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_percentage);
	return (obj->value.uint32);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_percentage = {
	"percentage",	  cfg_parse_percentage, cfg_print_percentage,
	cfg_doc_terminal, &cfg_rep_percentage,	NULL
};

bool
cfg_obj_ispercentage(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_percentage);
}

/*
 * Fixed point
 */
isc_result_t
cfg_parse_fixedpoint(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	size_t n1, n2, n3, l;
	const char *p;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected fixed point number");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	p = TOKEN_STRING(pctx);
	l = strlen(p);
	n1 = strspn(p, "0123456789");
	n2 = strspn(p + n1, ".");
	n3 = strspn(p + n1 + n2, "0123456789");

	if ((n1 + n2 + n3 != l) || (n1 + n3 == 0) || n1 > 5 || n2 > 1 || n3 > 2)
	{
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected fixed point number");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_fixedpoint, &obj));

	obj->value.uint32 = strtoul(p, NULL, 10) * 100;
	switch (n3) {
	case 2:
		obj->value.uint32 += strtoul(p + n1 + n2, NULL, 10);
		break;
	case 1:
		obj->value.uint32 += strtoul(p + n1 + n2, NULL, 10) * 10;
		break;
	}
	*ret = obj;

cleanup:
	return (result);
}

void
cfg_print_fixedpoint(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	char buf[64];
	int n;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	n = snprintf(buf, sizeof(buf), "%u.%02u", obj->value.uint32 / 100,
		     obj->value.uint32 % 100);
	INSIST(n > 0 && (size_t)n < sizeof(buf));
	cfg_print_chars(pctx, buf, strlen(buf));
}

uint32_t
cfg_obj_asfixedpoint(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_fixedpoint);
	return (obj->value.uint32);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_fixedpoint = {
	"fixedpoint",	  cfg_parse_fixedpoint, cfg_print_fixedpoint,
	cfg_doc_terminal, &cfg_rep_fixedpoint,	NULL
};

bool
cfg_obj_isfixedpoint(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_fixedpoint);
}

/*
 * uint32
 */
isc_result_t
cfg_parse_uint32(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));
	if (pctx->token.type != isc_tokentype_number) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected number");
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_uint32, &obj));

	obj->value.uint32 = pctx->token.value.as_ulong;
	*ret = obj;
cleanup:
	return (result);
}

void
cfg_print_cstr(cfg_printer_t *pctx, const char *s) {
	cfg_print_chars(pctx, s, strlen(s));
}

void
cfg_print_rawuint(cfg_printer_t *pctx, unsigned int u) {
	char buf[32];

	snprintf(buf, sizeof(buf), "%u", u);
	cfg_print_cstr(pctx, buf);
}

void
cfg_print_uint32(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_rawuint(pctx, obj->value.uint32);
}

bool
cfg_obj_isuint32(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_uint32);
}

uint32_t
cfg_obj_asuint32(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_uint32);
	return (obj->value.uint32);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_uint32 = {
	"integer",	  cfg_parse_uint32, cfg_print_uint32,
	cfg_doc_terminal, &cfg_rep_uint32,  NULL
};

/*
 * uint64
 */
bool
cfg_obj_isuint64(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_uint64);
}

uint64_t
cfg_obj_asuint64(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_uint64);
	return (obj->value.uint64);
}

void
cfg_print_uint64(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	char buf[32];

	snprintf(buf, sizeof(buf), "%" PRIu64, obj->value.uint64);
	cfg_print_cstr(pctx, buf);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_uint64 = {
	"64_bit_integer", NULL, cfg_print_uint64, cfg_doc_terminal,
	&cfg_rep_uint64,  NULL
};

/*
 * Get the number of digits in a number.
 */
static size_t
numlen(uint32_t num) {
	uint32_t period = num;
	size_t count = 0;

	if (period == 0) {
		return (1);
	}
	while (period > 0) {
		count++;
		period /= 10;
	}
	return (count);
}

/*
 * duration
 */
void
cfg_print_duration(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	char buf[CFG_DURATION_MAXLEN];
	char *str;
	const char *indicators = "YMWDHMS";
	int count, i;
	int durationlen[7] = { 0 };
	cfg_duration_t duration;
	/*
	 * D ? The duration has a date part.
	 * T ? The duration has a time part.
	 */
	bool D = false, T = false;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	duration = obj->value.duration;

	/* If this is not an ISO 8601 duration, just print it as a number. */
	if (!duration.iso8601) {
		cfg_print_rawuint(pctx, duration.parts[6]);
		return;
	}

	/* Calculate length of string. */
	buf[0] = 'P';
	buf[1] = '\0';
	str = &buf[1];
	count = 2;
	for (i = 0; i < 6; i++) {
		if (duration.parts[i] > 0) {
			durationlen[i] = 1 + numlen(duration.parts[i]);
			if (i < 4) {
				D = true;
			} else {
				T = true;
			}
			count += durationlen[i];
		}
	}
	/*
	 * Special case for seconds which is not taken into account in the
	 * above for loop: Count the length of the seconds part if it is
	 * non-zero, or if all the other parts are also zero.  In the latter
	 * case this function will print "PT0S".
	 */
	if (duration.parts[6] > 0 ||
	    (!D && !duration.parts[4] && !duration.parts[5]))
	{
		durationlen[6] = 1 + numlen(duration.parts[6]);
		T = true;
		count += durationlen[6];
	}
	/* Add one character for the time indicator. */
	if (T) {
		count++;
	}
	INSIST(count < CFG_DURATION_MAXLEN);

	/* Now print the duration. */
	for (i = 0; i < 6; i++) {
		/*
		 * We don't check here if weeks and other time indicator are
		 * used mutually exclusively.
		 */
		if (duration.parts[i] > 0) {
			snprintf(str, durationlen[i] + 2, "%u%c",
				 (uint32_t)duration.parts[i], indicators[i]);
			str += durationlen[i];
		}
		if (i == 3 && T) {
			snprintf(str, 2, "T");
			str += 1;
		}
	}
	/* Special case for seconds. */
	if (duration.parts[6] > 0 ||
	    (!D && !duration.parts[4] && !duration.parts[5]))
	{
		snprintf(str, durationlen[6] + 2, "%u%c",
			 (uint32_t)duration.parts[6], indicators[6]);
	}
	cfg_print_chars(pctx, buf, strlen(buf));
}

void
cfg_print_duration_or_unlimited(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_duration_t duration;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	duration = obj->value.duration;

	if (duration.unlimited) {
		cfg_print_cstr(pctx, "unlimited");
	} else {
		cfg_print_duration(pctx, obj);
	}
}

bool
cfg_obj_isduration(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_duration);
}

uint32_t
cfg_obj_asduration(const cfg_obj_t *obj) {
	uint64_t seconds = 0;
	cfg_duration_t duration;

	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_duration);

	duration = obj->value.duration;

	seconds += (uint64_t)duration.parts[6];		    /* Seconds */
	seconds += (uint64_t)duration.parts[5] * 60;	    /* Minutes */
	seconds += (uint64_t)duration.parts[4] * 3600;	    /* Hours */
	seconds += (uint64_t)duration.parts[3] * 86400;	    /* Days */
	seconds += (uint64_t)duration.parts[2] * 86400 * 7; /* Weeks */
	/*
	 * The below additions are not entirely correct
	 * because days may vary per month and per year.
	 */
	seconds += (uint64_t)duration.parts[1] * 86400 * 31;  /* Months */
	seconds += (uint64_t)duration.parts[0] * 86400 * 365; /* Years */

	return (seconds > UINT32_MAX ? UINT32_MAX : (uint32_t)seconds);
}

static isc_result_t
duration_fromtext(isc_textregion_t *source, cfg_duration_t *duration) {
	char buf[CFG_DURATION_MAXLEN] = { 0 };
	char *P, *X, *T, *W, *str;
	bool not_weeks = false;
	int i;
	long long int lli;

	/*
	 * Copy the buffer as it may not be NULL terminated.
	 */
	if (source->length > sizeof(buf) - 1) {
		return (ISC_R_BADNUMBER);
	}
	/* Copy source->length bytes and NULL terminate. */
	snprintf(buf, sizeof(buf), "%.*s", (int)source->length, source->base);
	str = buf;

	/* Clear out duration. */
	for (i = 0; i < 7; i++) {
		duration->parts[i] = 0;
	}

	/* Every duration starts with 'P' */
	P = strpbrk(str, "Pp");
	if (P == NULL) {
		return (ISC_R_BADNUMBER);
	}

	/* Record the time indicator. */
	T = strpbrk(str, "Tt");

	/* Record years. */
	X = strpbrk(str, "Yy");
	if (X != NULL) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[0] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Record months. */
	X = strpbrk(str, "Mm");

	/*
	 * M could be months or minutes. This is months if there is no time
	 * part, or this M indicator is before the time indicator.
	 */
	if (X != NULL && (T == NULL || (size_t)(X - P) < (size_t)(T - P))) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[1] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Record days. */
	X = strpbrk(str, "Dd");
	if (X != NULL) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[3] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Time part? */
	if (T != NULL) {
		str = T;
		not_weeks = true;
	}

	/* Record hours. */
	X = strpbrk(str, "Hh");
	if (X != NULL && T != NULL) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[4] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Record minutes. */
	X = strpbrk(str, "Mm");

	/*
	 * M could be months or minutes. This is minutes if there is a time
	 * part and the M indicator is behind the time indicator.
	 */
	if (X != NULL && T != NULL && (size_t)(X - P) > (size_t)(T - P)) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[5] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Record seconds. */
	X = strpbrk(str, "Ss");
	if (X != NULL && T != NULL) {
		errno = 0;
		lli = strtoll(str + 1, NULL, 10);
		if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
			return (ISC_R_BADNUMBER);
		}
		duration->parts[6] = (uint32_t)lli;
		str = X;
		not_weeks = true;
	}

	/* Or is the duration configured in weeks? */
	W = strpbrk(buf, "Ww");
	if (W != NULL) {
		if (not_weeks) {
			/* Mix of weeks and other indicators is not allowed */
			return (ISC_R_BADNUMBER);
		} else {
			errno = 0;
			lli = strtoll(str + 1, NULL, 10);
			if (errno != 0 || lli < 0 || lli > UINT32_MAX) {
				return (ISC_R_BADNUMBER);
			}
			duration->parts[2] = (uint32_t)lli;
			str = W;
		}
	}

	/* Deal with trailing garbage. */
	if (str[1] != '\0') {
		return (ISC_R_BADNUMBER);
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
parse_duration(cfg_parser_t *pctx, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	cfg_duration_t duration;

	duration.unlimited = false;

	if (toupper((unsigned char)TOKEN_STRING(pctx)[0]) == 'P') {
		result = duration_fromtext(&pctx->token.value.as_textregion,
					   &duration);
		duration.iso8601 = true;
	} else {
		uint32_t ttl;
		result = dns_ttl_fromtext(&pctx->token.value.as_textregion,
					  &ttl);
		/*
		 * With dns_ttl_fromtext() the information on optional units.
		 * is lost, and is treated as seconds from now on.
		 */
		for (int i = 0; i < 6; i++) {
			duration.parts[i] = 0;
		}
		duration.parts[6] = ttl;
		duration.iso8601 = false;
	}

	if (result == ISC_R_RANGE) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "duration or TTL out of range");
		return (result);
	} else if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_duration, &obj));
	obj->value.duration = duration;
	*ret = obj;

	return (ISC_R_SUCCESS);

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR,
			 "expected ISO 8601 duration or TTL value");
	return (result);
}

isc_result_t
cfg_parse_duration(cfg_parser_t *pctx, const cfg_type_t *type,
		   cfg_obj_t **ret) {
	isc_result_t result;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}

	return (parse_duration(pctx, ret));

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR,
			 "expected ISO 8601 duration or TTL value");
	return (result);
}

isc_result_t
cfg_parse_duration_or_unlimited(cfg_parser_t *pctx, const cfg_type_t *type,
				cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	cfg_duration_t duration;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}

	if (strcmp(TOKEN_STRING(pctx), "unlimited") == 0) {
		for (int i = 0; i < 7; i++) {
			duration.parts[i] = 0;
		}
		duration.iso8601 = false;
		duration.unlimited = true;

		CHECK(cfg_create_obj(pctx, &cfg_type_duration, &obj));
		obj->value.duration = duration;
		*ret = obj;
		return (ISC_R_SUCCESS);
	}

	return (parse_duration(pctx, ret));

cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR,
			 "expected ISO 8601 duration, TTL value, or unlimited");
	return (result);
}

/*%
 * A duration as defined by ISO 8601 (P[n]Y[n]M[n]DT[n]H[n]M[n]S).
 * - P is the duration indicator ("period") placed at the start.
 * - Y is the year indicator that follows the value for the number of years.
 * - M is the month indicator that follows the value for the number of months.
 * - D is the day indicator that follows the value for the number of days.
 * - T is the time indicator that precedes the time components.
 * - H is the hour indicator that follows the value for the number of hours.
 * - M is the minute indicator that follows the value for the number of
 *   minutes.
 * - S is the second indicator that follows the value for the number of
 *   seconds.
 *
 * A duration can also be a TTL value (number + optional unit).
 */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_duration = {
	"duration",	  cfg_parse_duration, cfg_print_duration,
	cfg_doc_terminal, &cfg_rep_duration,  NULL
};
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_duration_or_unlimited = {
	"duration_or_unlimited",
	cfg_parse_duration_or_unlimited,
	cfg_print_duration_or_unlimited,
	cfg_doc_terminal,
	&cfg_rep_duration,
	NULL
};

/*
 * qstring (quoted string), ustring (unquoted string), astring
 * (any string), sstring (secret string)
 */

/* Create a string object from a null-terminated C string. */
static isc_result_t
create_string(cfg_parser_t *pctx, const char *contents, const cfg_type_t *type,
	      cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	int len;

	CHECK(cfg_create_obj(pctx, type, &obj));
	len = strlen(contents);
	obj->value.string.length = len;
	obj->value.string.base = isc_mem_get(pctx->mctx, len + 1);
	if (obj->value.string.base == 0) {
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
		return (ISC_R_NOMEMORY);
	}
	memmove(obj->value.string.base, contents, len);
	obj->value.string.base[len] = '\0';

	*ret = obj;
cleanup:
	return (result);
}

isc_result_t
cfg_parse_qstring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type != isc_tokentype_qstring) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected quoted string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (create_string(pctx, TOKEN_STRING(pctx), &cfg_type_qstring,
			      ret));
cleanup:
	return (result);
}

static isc_result_t
parse_ustring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, 0));
	if (pctx->token.type != isc_tokentype_string) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected unquoted string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (create_string(pctx, TOKEN_STRING(pctx), &cfg_type_ustring,
			      ret));
cleanup:
	return (result);
}

isc_result_t
cfg_parse_astring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_getstringtoken(pctx));
	return (create_string(pctx, TOKEN_STRING(pctx), &cfg_type_qstring,
			      ret));
cleanup:
	return (result);
}

isc_result_t
cfg_parse_sstring(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	CHECK(cfg_getstringtoken(pctx));
	return (create_string(pctx, TOKEN_STRING(pctx), &cfg_type_sstring,
			      ret));
cleanup:
	return (result);
}

static isc_result_t
parse_btext(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	UNUSED(type);

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_BTEXT));
	if (pctx->token.type != isc_tokentype_btext) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected bracketed text");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (create_string(pctx, TOKEN_STRING(pctx),
			      &cfg_type_bracketed_text, ret));
cleanup:
	return (result);
}

static void
print_btext(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	/*
	 * We need to print "{" instead of running print_open()
	 * in order to preserve the exact original formatting
	 * of the bracketed text. But we increment the indent value
	 * so that print_close() will leave us back in our original
	 * state.
	 */
	pctx->indent++;
	cfg_print_cstr(pctx, "{");
	cfg_print_chars(pctx, obj->value.string.base, obj->value.string.length);
	print_close(pctx);
}

static void
doc_btext(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);

	cfg_print_cstr(pctx, "{ <unspecified-text> }");
}

bool
cfg_is_enum(const char *s, const char *const *enums) {
	const char *const *p;

	REQUIRE(s != NULL);
	REQUIRE(enums != NULL);

	for (p = enums; *p != NULL; p++) {
		if (strcasecmp(*p, s) == 0) {
			return (true);
		}
	}
	return (false);
}

static isc_result_t
check_enum(cfg_parser_t *pctx, cfg_obj_t *obj, const char *const *enums) {
	const char *s = obj->value.string.base;

	if (cfg_is_enum(s, enums)) {
		return (ISC_R_SUCCESS);
	}
	cfg_parser_error(pctx, 0, "'%s' unexpected", s);
	return (ISC_R_UNEXPECTEDTOKEN);
}

isc_result_t
cfg_parse_enum(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(parse_ustring(pctx, NULL, &obj));
	CHECK(check_enum(pctx, obj, type->of));
	*ret = obj;
	return (ISC_R_SUCCESS);
cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

void
cfg_doc_enum(cfg_printer_t *pctx, const cfg_type_t *type) {
	const char *const *p;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	cfg_print_cstr(pctx, "( ");
	for (p = type->of; *p != NULL; p++) {
		cfg_print_cstr(pctx, *p);
		if (p[1] != NULL) {
			cfg_print_cstr(pctx, " | ");
		}
	}
	cfg_print_cstr(pctx, " )");
}

isc_result_t
cfg_parse_enum_or_other(cfg_parser_t *pctx, const cfg_type_t *enumtype,
			const cfg_type_t *othertype, cfg_obj_t **ret) {
	isc_result_t result;
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string &&
	    cfg_is_enum(TOKEN_STRING(pctx), enumtype->of))
	{
		CHECK(cfg_parse_enum(pctx, enumtype, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, othertype, ret));
	}
cleanup:
	return (result);
}

void
cfg_doc_enum_or_other(cfg_printer_t *pctx, const cfg_type_t *enumtype,
		      const cfg_type_t *othertype) {
	const char *const *p;
	bool first = true;

	/*
	 * If othertype is cfg_type_void, it means that enumtype is
	 * optional.
	 */

	if (othertype == &cfg_type_void) {
		cfg_print_cstr(pctx, "[ ");
	}
	cfg_print_cstr(pctx, "( ");
	for (p = enumtype->of; *p != NULL; p++) {
		if (!first) {
			cfg_print_cstr(pctx, " | ");
		}
		first = false;
		cfg_print_cstr(pctx, *p);
	}
	if (othertype != &cfg_type_void) {
		if (!first) {
			cfg_print_cstr(pctx, " | ");
		}
		cfg_doc_terminal(pctx, othertype);
	}
	cfg_print_cstr(pctx, " )");
	if (othertype == &cfg_type_void) {
		cfg_print_cstr(pctx, " ]");
	}
}

void
cfg_print_ustring(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	cfg_print_chars(pctx, obj->value.string.base, obj->value.string.length);
}

static void
print_qstring(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_cstr(pctx, "\"");
	for (size_t i = 0; i < obj->value.string.length; i++) {
		if (obj->value.string.base[i] == '"') {
			cfg_print_cstr(pctx, "\\");
		}
		cfg_print_chars(pctx, &obj->value.string.base[i], 1);
	}
	cfg_print_cstr(pctx, "\"");
}

static void
print_sstring(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_cstr(pctx, "\"");
	if ((pctx->flags & CFG_PRINTER_XKEY) != 0) {
		unsigned int len = obj->value.string.length;
		while (len-- > 0) {
			cfg_print_cstr(pctx, "?");
		}
	} else {
		cfg_print_ustring(pctx, obj);
	}
	cfg_print_cstr(pctx, "\"");
}

static void
free_string(cfg_parser_t *pctx, cfg_obj_t *obj) {
	isc_mem_put(pctx->mctx, obj->value.string.base,
		    obj->value.string.length + 1);
}

bool
cfg_obj_isstring(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_string);
}

const char *
cfg_obj_asstring(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_string);
	return (obj->value.string.base);
}

/* Quoted string only */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_qstring = {
	"quoted_string",  cfg_parse_qstring, print_qstring,
	cfg_doc_terminal, &cfg_rep_string,   NULL
};

/* Unquoted string only */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_ustring = {
	"string",	  parse_ustring,   cfg_print_ustring,
	cfg_doc_terminal, &cfg_rep_string, NULL
};

/* Any string (quoted or unquoted); printed with quotes */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_astring = {
	"string",	  cfg_parse_astring, print_qstring,
	cfg_doc_terminal, &cfg_rep_string,   NULL
};

/*
 * Any string (quoted or unquoted); printed with quotes.
 * If CFG_PRINTER_XKEY is set when printing the string will be '?' out.
 */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_sstring = {
	"string",	  cfg_parse_sstring, print_sstring,
	cfg_doc_terminal, &cfg_rep_string,   NULL
};

/*
 * Text enclosed in brackets. Used to pass a block of configuration
 * text to dynamic library or external application. Checked for
 * bracket balance, but not otherwise parsed.
 */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_bracketed_text = {
	"bracketed_text", parse_btext,	   print_btext,
	doc_btext,	  &cfg_rep_string, NULL
};

#if defined(HAVE_GEOIP2)
/*
 * "geoip" ACL element:
 * geoip [ db <database> ] search-type <string>
 */
static const char *geoiptype_enums[] = {
	"area",	      "areacode",  "asnum",	  "city",     "continent",
	"country",    "country3",  "countryname", "domain",   "isp",
	"metro",      "metrocode", "netspeed",	  "org",      "postal",
	"postalcode", "region",	   "regionname",  "timezone", "tz",
	NULL
};
static cfg_type_t cfg_type_geoiptype = { "geoiptype",	    cfg_parse_enum,
					 cfg_print_ustring, cfg_doc_enum,
					 &cfg_rep_string,   &geoiptype_enums };

static cfg_tuplefielddef_t geoip_fields[] = {
	{ "negated", &cfg_type_void, 0 },
	{ "db", &cfg_type_astring, 0 },
	{ "subtype", &cfg_type_geoiptype, 0 },
	{ "search", &cfg_type_astring, 0 },
	{ NULL, NULL, 0 }
};

static cfg_type_t cfg_type_geoip = { "geoip",	parse_geoip,	print_geoip,
				     doc_geoip, &cfg_rep_tuple, geoip_fields };

static isc_result_t
parse_geoip(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	const cfg_tuplefielddef_t *fields = type->of;

	CHECK(cfg_create_tuple(pctx, type, &obj));
	CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[0]));

	/* Parse the optional "db" field. */
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_string) {
		CHECK(cfg_gettoken(pctx, 0));
		if (strcasecmp(TOKEN_STRING(pctx), "db") == 0 &&
		    obj->value.tuple[1] == NULL)
		{
			CHECK(cfg_parse_obj(pctx, fields[1].type,
					    &obj->value.tuple[1]));
		} else {
			CHECK(cfg_parse_void(pctx, NULL, &obj->value.tuple[1]));
			cfg_ungettoken(pctx);
		}
	}

	CHECK(cfg_parse_obj(pctx, fields[2].type, &obj->value.tuple[2]));
	CHECK(cfg_parse_obj(pctx, fields[3].type, &obj->value.tuple[3]));

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
print_geoip(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (obj->value.tuple[1]->type->print != cfg_print_void) {
		cfg_print_cstr(pctx, " db ");
		cfg_print_obj(pctx, obj->value.tuple[1]);
	}
	cfg_print_obj(pctx, obj->value.tuple[2]);
	cfg_print_obj(pctx, obj->value.tuple[3]);
}

static void
doc_geoip(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);
	cfg_print_cstr(pctx, "[ db ");
	cfg_doc_obj(pctx, &cfg_type_astring);
	cfg_print_cstr(pctx, " ]");
	cfg_print_cstr(pctx, " ");
	cfg_doc_enum(pctx, &cfg_type_geoiptype);
	cfg_print_cstr(pctx, " ");
	cfg_doc_obj(pctx, &cfg_type_astring);
}
#endif /* HAVE_GEOIP2 */

static cfg_type_t cfg_type_addrmatchelt;
static cfg_type_t cfg_type_negated;

static isc_result_t
parse_addrmatchelt(cfg_parser_t *pctx, const cfg_type_t *type,
		   cfg_obj_t **ret) {
	isc_result_t result;
	UNUSED(type);

	CHECK(cfg_peektoken(pctx, CFG_LEXOPT_QSTRING));

	if (pctx->token.type == isc_tokentype_string ||
	    pctx->token.type == isc_tokentype_qstring)
	{
		if (pctx->token.type == isc_tokentype_string &&
		    (strcasecmp(TOKEN_STRING(pctx), "key") == 0))
		{
			CHECK(cfg_parse_obj(pctx, &cfg_type_keyref, ret));
		} else if (pctx->token.type == isc_tokentype_string &&
			   (strcasecmp(TOKEN_STRING(pctx), "geoip") == 0))
		{
#if defined(HAVE_GEOIP2)
			CHECK(cfg_gettoken(pctx, 0));
			CHECK(cfg_parse_obj(pctx, &cfg_type_geoip, ret));
#else  /* if defined(HAVE_GEOIP2) */
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "'geoip' "
					 "not supported in this build");
			return (ISC_R_UNEXPECTEDTOKEN);
#endif /* if defined(HAVE_GEOIP2) */
		} else {
			if (cfg_lookingat_netaddr(
				    pctx, CFG_ADDR_V4OK | CFG_ADDR_V4PREFIXOK |
						  CFG_ADDR_V6OK))
			{
				CHECK(cfg_parse_netprefix(pctx, NULL, ret));
			} else {
				CHECK(cfg_parse_astring(pctx, NULL, ret));
			}
		}
	} else if (pctx->token.type == isc_tokentype_special) {
		if (pctx->token.value.as_char == '{') {
			/* Nested match list. */
			CHECK(cfg_parse_obj(pctx, &cfg_type_bracketed_aml,
					    ret));
		} else if (pctx->token.value.as_char == '!') {
			CHECK(cfg_gettoken(pctx, 0)); /* read "!" */
			CHECK(cfg_parse_obj(pctx, &cfg_type_negated, ret));
		} else {
			goto bad;
		}
	} else {
	bad:
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected IP match list element");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
cleanup:
	return (result);
}

/*%
 * A negated address match list element (like "! 10.0.0.1").
 * Somewhat sneakily, the caller is expected to parse the
 * "!", but not to print it.
 */
static cfg_tuplefielddef_t negated_fields[] = {
	{ "negated", &cfg_type_addrmatchelt, 0 }, { NULL, NULL, 0 }
};

static void
print_negated(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	cfg_print_cstr(pctx, "!");
	cfg_print_tuple(pctx, obj);
}

static cfg_type_t cfg_type_negated = { "negated",      cfg_parse_tuple,
				       print_negated,  NULL,
				       &cfg_rep_tuple, &negated_fields };

/*% An address match list element */

static cfg_type_t cfg_type_addrmatchelt = { "address_match_element",
					    parse_addrmatchelt,
					    NULL,
					    cfg_doc_terminal,
					    NULL,
					    NULL };

/*%
 * A bracketed address match list
 */
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_bracketed_aml = {
	"bracketed_aml",
	cfg_parse_bracketed_list,
	cfg_print_bracketed_list,
	cfg_doc_bracketed_list,
	&cfg_rep_list,
	&cfg_type_addrmatchelt
};

/*
 * Optional bracketed text
 */
static isc_result_t
parse_optional_btext(cfg_parser_t *pctx, const cfg_type_t *type,
		     cfg_obj_t **ret) {
	isc_result_t result;

	UNUSED(type);

	CHECK(cfg_peektoken(pctx, ISC_LEXOPT_BTEXT));
	if (pctx->token.type == isc_tokentype_btext) {
		CHECK(cfg_parse_obj(pctx, &cfg_type_bracketed_text, ret));
	} else {
		CHECK(cfg_parse_obj(pctx, &cfg_type_void, ret));
	}
cleanup:
	return (result);
}

static void
print_optional_btext(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	if (obj->type == &cfg_type_void) {
		return;
	}

	pctx->indent++;
	cfg_print_cstr(pctx, "{");
	cfg_print_chars(pctx, obj->value.string.base, obj->value.string.length);
	print_close(pctx);
}

static void
doc_optional_btext(cfg_printer_t *pctx, const cfg_type_t *type) {
	UNUSED(type);

	cfg_print_cstr(pctx, "[ { <unspecified-text> } ]");
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_optional_bracketed_text = {
	"optional_btext",
	parse_optional_btext,
	print_optional_btext,
	doc_optional_btext,
	NULL,
	NULL
};

/*
 * Booleans
 */

bool
cfg_obj_isboolean(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_boolean);
}

bool
cfg_obj_asboolean(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_boolean);
	return (obj->value.boolean);
}

isc_result_t
cfg_parse_boolean(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	bool value;
	cfg_obj_t *obj = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	result = cfg_gettoken(pctx, 0);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (pctx->token.type != isc_tokentype_string) {
		goto bad_boolean;
	}

	if ((strcasecmp(TOKEN_STRING(pctx), "true") == 0) ||
	    (strcasecmp(TOKEN_STRING(pctx), "yes") == 0) ||
	    (strcmp(TOKEN_STRING(pctx), "1") == 0))
	{
		value = true;
	} else if ((strcasecmp(TOKEN_STRING(pctx), "false") == 0) ||
		   (strcasecmp(TOKEN_STRING(pctx), "no") == 0) ||
		   (strcmp(TOKEN_STRING(pctx), "0") == 0))
	{
		value = false;
	} else {
		goto bad_boolean;
	}

	CHECK(cfg_create_obj(pctx, &cfg_type_boolean, &obj));
	obj->value.boolean = value;
	*ret = obj;
	return (result);

bad_boolean:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "boolean expected");
	return (ISC_R_UNEXPECTEDTOKEN);

cleanup:
	return (result);
}

void
cfg_print_boolean(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	if (obj->value.boolean) {
		cfg_print_cstr(pctx, "yes");
	} else {
		cfg_print_cstr(pctx, "no");
	}
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_boolean = {
	"boolean",	  cfg_parse_boolean, cfg_print_boolean,
	cfg_doc_terminal, &cfg_rep_boolean,  NULL
};

/*
 * Lists.
 */

isc_result_t
cfg_create_list(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **obj) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(obj != NULL && *obj == NULL);

	CHECK(cfg_create_obj(pctx, type, obj));
	ISC_LIST_INIT((*obj)->value.list);
cleanup:
	return (result);
}

static isc_result_t
create_listelt(cfg_parser_t *pctx, cfg_listelt_t **eltp) {
	cfg_listelt_t *elt;

	elt = isc_mem_get(pctx->mctx, sizeof(*elt));
	elt->obj = NULL;
	ISC_LINK_INIT(elt, link);
	*eltp = elt;
	return (ISC_R_SUCCESS);
}

static void
free_listelt(cfg_parser_t *pctx, cfg_listelt_t *elt) {
	if (elt->obj != NULL) {
		cfg_obj_destroy(pctx, &elt->obj);
	}
	isc_mem_put(pctx->mctx, elt, sizeof(*elt));
}

static void
free_list(cfg_parser_t *pctx, cfg_obj_t *obj) {
	cfg_listelt_t *elt, *next;
	for (elt = ISC_LIST_HEAD(obj->value.list); elt != NULL; elt = next) {
		next = ISC_LIST_NEXT(elt, link);
		free_listelt(pctx, elt);
	}
}

isc_result_t
cfg_parse_listelt(cfg_parser_t *pctx, const cfg_type_t *elttype,
		  cfg_listelt_t **ret) {
	isc_result_t result;
	cfg_listelt_t *elt = NULL;
	cfg_obj_t *value = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(elttype != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(create_listelt(pctx, &elt));

	result = cfg_parse_obj(pctx, elttype, &value);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	elt->obj = value;

	*ret = elt;
	return (ISC_R_SUCCESS);

cleanup:
	isc_mem_put(pctx->mctx, elt, sizeof(*elt));
	return (result);
}

/*
 * Parse a homogeneous list whose elements are of type 'elttype'
 * and where each element is terminated by a semicolon.
 */
static isc_result_t
parse_list(cfg_parser_t *pctx, const cfg_type_t *listtype, cfg_obj_t **ret) {
	cfg_obj_t *listobj = NULL;
	const cfg_type_t *listof = listtype->of;
	isc_result_t result;
	cfg_listelt_t *elt = NULL;

	CHECK(cfg_create_list(pctx, listtype, &listobj));

	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == /*{*/ '}')
		{
			break;
		}
		CHECK(cfg_parse_listelt(pctx, listof, &elt));
		CHECK(parse_semicolon(pctx));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
		elt = NULL;
	}
	*ret = listobj;
	return (ISC_R_SUCCESS);

cleanup:
	if (elt != NULL) {
		free_listelt(pctx, elt);
	}
	CLEANUP_OBJ(listobj);
	return (result);
}

static void
print_list(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_list_t *list = &obj->value.list;
	const cfg_listelt_t *elt;

	for (elt = ISC_LIST_HEAD(*list); elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link))
	{
		if ((pctx->flags & CFG_PRINTER_ONELINE) != 0) {
			cfg_print_obj(pctx, elt->obj);
			cfg_print_cstr(pctx, "; ");
		} else {
			cfg_print_indent(pctx);
			cfg_print_obj(pctx, elt->obj);
			cfg_print_cstr(pctx, ";\n");
		}
	}
}

isc_result_t
cfg_parse_bracketed_list(cfg_parser_t *pctx, const cfg_type_t *type,
			 cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(cfg_parse_special(pctx, '{'));
	CHECK(parse_list(pctx, type, ret));
	CHECK(cfg_parse_special(pctx, '}'));
cleanup:
	return (result);
}

void
cfg_print_bracketed_list(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	print_open(pctx);
	print_list(pctx, obj);
	print_close(pctx);
}

void
cfg_doc_bracketed_list(cfg_printer_t *pctx, const cfg_type_t *type) {
	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	cfg_print_cstr(pctx, "{ ");
	cfg_doc_obj(pctx, type->of);
	cfg_print_cstr(pctx, "; ... }");
}

/*
 * Parse a homogeneous list whose elements are of type 'elttype'
 * and where elements are separated by space.  The list ends
 * before the first semicolon.
 */
isc_result_t
cfg_parse_spacelist(cfg_parser_t *pctx, const cfg_type_t *listtype,
		    cfg_obj_t **ret) {
	cfg_obj_t *listobj = NULL;
	const cfg_type_t *listof;
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(listtype != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	listof = listtype->of;

	CHECK(cfg_create_list(pctx, listtype, &listobj));

	for (;;) {
		cfg_listelt_t *elt = NULL;

		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special &&
		    pctx->token.value.as_char == ';')
		{
			break;
		}
		CHECK(cfg_parse_listelt(pctx, listof, &elt));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
	}
	*ret = listobj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(listobj);
	return (result);
}

void
cfg_print_spacelist(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_list_t *list = NULL;
	const cfg_listelt_t *elt = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	list = &obj->value.list;

	for (elt = ISC_LIST_HEAD(*list); elt != NULL;
	     elt = ISC_LIST_NEXT(elt, link))
	{
		cfg_print_obj(pctx, elt->obj);
		if (ISC_LIST_NEXT(elt, link) != NULL) {
			cfg_print_cstr(pctx, " ");
		}
	}
}

bool
cfg_obj_islist(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_list);
}

const cfg_listelt_t *
cfg_list_first(const cfg_obj_t *obj) {
	REQUIRE(obj == NULL || obj->type->rep == &cfg_rep_list);
	if (obj == NULL) {
		return (NULL);
	}
	return (ISC_LIST_HEAD(obj->value.list));
}

const cfg_listelt_t *
cfg_list_next(const cfg_listelt_t *elt) {
	REQUIRE(elt != NULL);
	return (ISC_LIST_NEXT(elt, link));
}

/*
 * Return the length of a list object.  If obj is NULL or is not
 * a list, return 0.
 */
unsigned int
cfg_list_length(const cfg_obj_t *obj, bool recurse) {
	const cfg_listelt_t *elt;
	unsigned int count = 0;

	if (obj == NULL || !cfg_obj_islist(obj)) {
		return (0U);
	}
	for (elt = cfg_list_first(obj); elt != NULL; elt = cfg_list_next(elt)) {
		if (recurse && cfg_obj_islist(elt->obj)) {
			count += cfg_list_length(elt->obj, recurse);
		} else {
			count++;
		}
	}
	return (count);
}

cfg_obj_t *
cfg_listelt_value(const cfg_listelt_t *elt) {
	REQUIRE(elt != NULL);
	return (elt->obj);
}

/*
 * Maps.
 */

/*
 * Parse a map body.  That's something like
 *
 *   "foo 1; bar { glub; }; zap true; zap false;"
 *
 * i.e., a sequence of option names followed by values and
 * terminated by semicolons.  Used for the top level of
 * the named.conf syntax, as well as for the body of the
 * options, view, zone, and other statements.
 */
isc_result_t
cfg_parse_mapbody(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	const cfg_clausedef_t *const *clausesets;
	isc_result_t result;
	const cfg_clausedef_t *const *clauseset;
	const cfg_clausedef_t *clause;
	cfg_obj_t *value = NULL;
	cfg_obj_t *obj = NULL;
	cfg_obj_t *eltobj = NULL;
	cfg_obj_t *includename = NULL;
	isc_symvalue_t symval;
	cfg_list_t *list = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	clausesets = type->of;

	CHECK(create_map(pctx, type, &obj));

	obj->value.map.clausesets = clausesets;

	for (;;) {
		cfg_listelt_t *elt;

	redo:
		/*
		 * Parse the option name and see if it is known.
		 */
		CHECK(cfg_gettoken(pctx, 0));

		if (pctx->token.type != isc_tokentype_string) {
			cfg_ungettoken(pctx);
			break;
		}

		/*
		 * We accept "include" statements wherever a map body
		 * clause can occur.
		 */
		if (strcasecmp(TOKEN_STRING(pctx), "include") == 0) {
			/*
			 * Turn the file name into a temporary configuration
			 * object just so that it is not overwritten by the
			 * semicolon token.
			 */
			CHECK(cfg_parse_obj(pctx, &cfg_type_qstring,
					    &includename));
			CHECK(parse_semicolon(pctx));
			CHECK(parser_openfile(pctx,
					      includename->value.string.base));
			cfg_obj_destroy(pctx, &includename);
			goto redo;
		}

		clause = NULL;
		for (clauseset = clausesets; *clauseset != NULL; clauseset++) {
			for (clause = *clauseset; clause->name != NULL;
			     clause++)
			{
				if (strcasecmp(TOKEN_STRING(pctx),
					       clause->name) == 0)
				{
					goto done;
				}
			}
		}
	done:
		if (clause == NULL || clause->name == NULL) {
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
					 "unknown option");
			/*
			 * Try to recover by parsing this option as an unknown
			 * option and discarding it.
			 */
			CHECK(cfg_parse_obj(pctx, &cfg_type_unsupported,
					    &eltobj));
			cfg_obj_destroy(pctx, &eltobj);
			CHECK(parse_semicolon(pctx));
			continue;
		}

		/* Clause is known. */

		/* Issue fatal errors if appropriate */
		if ((clause->flags & CFG_CLAUSEFLAG_ANCIENT) != 0) {
			cfg_parser_error(pctx, 0,
					 "option '%s' no longer exists",
					 clause->name);
			CHECK(ISC_R_FAILURE);
		}

		/* Issue warnings if appropriate */
		if ((pctx->flags & CFG_PCTX_NODEPRECATED) == 0 &&
		    (clause->flags & CFG_CLAUSEFLAG_DEPRECATED) != 0)
		{
			cfg_parser_warning(pctx, 0, "option '%s' is deprecated",
					   clause->name);
		}
		if ((clause->flags & CFG_CLAUSEFLAG_OBSOLETE) != 0) {
			cfg_parser_warning(pctx, 0,
					   "option '%s' is obsolete and "
					   "should be removed ",
					   clause->name);
		}
		if ((clause->flags & CFG_CLAUSEFLAG_NOTIMP) != 0) {
			cfg_parser_warning(pctx, 0,
					   "option '%s' is not implemented",
					   clause->name);
		}
		if ((clause->flags & CFG_CLAUSEFLAG_NYI) != 0) {
			cfg_parser_warning(pctx, 0,
					   "option '%s' is not implemented",
					   clause->name);
		}
		if ((clause->flags & CFG_CLAUSEFLAG_NOOP) != 0) {
			cfg_parser_warning(pctx, 0,
					   "option '%s' was not "
					   "enabled at compile time "
					   "(ignored)",
					   clause->name);
		}

		if ((clause->flags & CFG_CLAUSEFLAG_NOTCONFIGURED) != 0) {
			cfg_parser_error(pctx, 0,
					 "option '%s' was not "
					 "enabled at compile time",
					 clause->name);
			CHECK(ISC_R_FAILURE);
		}

		/*
		 * Don't log options with CFG_CLAUSEFLAG_NEWDEFAULT
		 * set here - we need to log the *lack* of such an option,
		 * not its presence.
		 */

		/* See if the clause already has a value; if not create one. */
		result = isc_symtab_lookup(obj->value.map.symtab, clause->name,
					   0, &symval);

		if ((clause->flags & CFG_CLAUSEFLAG_MULTI) != 0) {
			/* Multivalued clause */
			cfg_obj_t *listobj = NULL;
			if (result == ISC_R_NOTFOUND) {
				CHECK(cfg_create_list(pctx,
						      &cfg_type_implicitlist,
						      &listobj));
				symval.as_pointer = listobj;
				result = isc_symtab_define(
					obj->value.map.symtab, clause->name, 1,
					symval, isc_symexists_reject);
				if (result != ISC_R_SUCCESS) {
					cfg_parser_error(pctx, CFG_LOG_NEAR,
							 "isc_symtab_define(%s)"
							 " "
							 "failed",
							 clause->name);
					isc_mem_put(pctx->mctx, list,
						    sizeof(cfg_list_t));
					goto cleanup;
				}
			} else {
				INSIST(result == ISC_R_SUCCESS);
				listobj = symval.as_pointer;
			}

			elt = NULL;
			CHECK(cfg_parse_listelt(pctx, clause->type, &elt));
			CHECK(parse_semicolon(pctx));

			ISC_LIST_APPEND(listobj->value.list, elt, link);
		} else {
			/* Single-valued clause */
			if (result == ISC_R_NOTFOUND) {
				bool callback = ((clause->flags &
						  CFG_CLAUSEFLAG_CALLBACK) !=
						 0);
				CHECK(parse_symtab_elt(
					pctx, clause->name, clause->type,
					obj->value.map.symtab, callback));
				CHECK(parse_semicolon(pctx));
			} else if (result == ISC_R_SUCCESS) {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
						 "'%s' redefined",
						 clause->name);
				result = ISC_R_EXISTS;
				goto cleanup;
			} else {
				cfg_parser_error(pctx, CFG_LOG_NEAR,
						 "isc_symtab_define() failed");
				goto cleanup;
			}
		}
	}

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(value);
	CLEANUP_OBJ(obj);
	CLEANUP_OBJ(eltobj);
	CLEANUP_OBJ(includename);
	return (result);
}

static isc_result_t
parse_symtab_elt(cfg_parser_t *pctx, const char *name, cfg_type_t *elttype,
		 isc_symtab_t *symtab, bool callback) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_symvalue_t symval;

	CHECK(cfg_parse_obj(pctx, elttype, &obj));

	if (callback && pctx->callback != NULL) {
		CHECK(pctx->callback(name, obj, pctx->callbackarg));
	}

	symval.as_pointer = obj;
	CHECK(isc_symtab_define(symtab, name, 1, symval, isc_symexists_reject));
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

/*
 * Parse a map; e.g., "{ foo 1; bar { glub; }; zap true; zap false; }"
 */
isc_result_t
cfg_parse_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(cfg_parse_special(pctx, '{'));
	CHECK(cfg_parse_mapbody(pctx, type, ret));
	CHECK(cfg_parse_special(pctx, '}'));
cleanup:
	return (result);
}

/*
 * Subroutine for cfg_parse_named_map() and cfg_parse_addressed_map().
 */
static isc_result_t
parse_any_named_map(cfg_parser_t *pctx, cfg_type_t *nametype,
		    const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *idobj = NULL;
	cfg_obj_t *mapobj = NULL;

	REQUIRE(pctx != NULL);
	REQUIRE(nametype != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	CHECK(cfg_parse_obj(pctx, nametype, &idobj));
	CHECK(cfg_parse_map(pctx, type, &mapobj));
	mapobj->value.map.id = idobj;
	*ret = mapobj;
	return (result);
cleanup:
	CLEANUP_OBJ(idobj);
	CLEANUP_OBJ(mapobj);
	return (result);
}

/*
 * Parse a map identified by a string name.  E.g., "name { foo 1; }".
 * Used for the "key" and "channel" statements.
 */
isc_result_t
cfg_parse_named_map(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_astring, type, ret));
}

/*
 * Parse a map identified by a network address.
 * Used to be used for the "server" statement.
 */
isc_result_t
cfg_parse_addressed_map(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_netaddr, type, ret));
}

/*
 * Parse a map identified by a network prefix.
 * Used for the "server" statement.
 */
isc_result_t
cfg_parse_netprefix_map(cfg_parser_t *pctx, const cfg_type_t *type,
			cfg_obj_t **ret) {
	return (parse_any_named_map(pctx, &cfg_type_netprefix, type, ret));
}

static void
print_symval(cfg_printer_t *pctx, const char *name, cfg_obj_t *obj) {
	if ((pctx->flags & CFG_PRINTER_ONELINE) == 0) {
		cfg_print_indent(pctx);
	}

	cfg_print_cstr(pctx, name);
	cfg_print_cstr(pctx, " ");
	cfg_print_obj(pctx, obj);

	if ((pctx->flags & CFG_PRINTER_ONELINE) == 0) {
		cfg_print_cstr(pctx, ";\n");
	} else {
		cfg_print_cstr(pctx, "; ");
	}
}

void
cfg_print_mapbody(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_clausedef_t *const *clauseset;

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	for (clauseset = obj->value.map.clausesets; *clauseset != NULL;
	     clauseset++)
	{
		isc_symvalue_t symval;
		const cfg_clausedef_t *clause;

		for (clause = *clauseset; clause->name != NULL; clause++) {
			isc_result_t result;
			result = isc_symtab_lookup(obj->value.map.symtab,
						   clause->name, 0, &symval);
			if (result == ISC_R_SUCCESS) {
				cfg_obj_t *symobj = symval.as_pointer;
				if (symobj->type == &cfg_type_implicitlist) {
					/* Multivalued. */
					cfg_list_t *list = &symobj->value.list;
					cfg_listelt_t *elt;
					for (elt = ISC_LIST_HEAD(*list);
					     elt != NULL;
					     elt = ISC_LIST_NEXT(elt, link))
					{
						print_symval(pctx, clause->name,
							     elt->obj);
					}
				} else {
					/* Single-valued. */
					print_symval(pctx, clause->name,
						     symobj);
				}
			} else if (result == ISC_R_NOTFOUND) {
				/* do nothing */
			} else {
				UNREACHABLE();
			}
		}
	}
}

static struct flagtext {
	unsigned int flag;
	const char *text;
} flagtexts[] = { { CFG_CLAUSEFLAG_NOTIMP, "not implemented" },
		  { CFG_CLAUSEFLAG_NYI, "not yet implemented" },
		  { CFG_CLAUSEFLAG_OBSOLETE, "obsolete" },
		  { CFG_CLAUSEFLAG_NEWDEFAULT, "default changed" },
		  { CFG_CLAUSEFLAG_TESTONLY, "test only" },
		  { CFG_CLAUSEFLAG_NOTCONFIGURED, "not configured" },
		  { CFG_CLAUSEFLAG_MULTI, "may occur multiple times" },
		  { CFG_CLAUSEFLAG_EXPERIMENTAL, "experimental" },
		  { CFG_CLAUSEFLAG_NOOP, "non-operational" },
		  { CFG_CLAUSEFLAG_DEPRECATED, "deprecated" },
		  { CFG_CLAUSEFLAG_ANCIENT, "ancient" },
		  { 0, NULL } };

void
cfg_print_clauseflags(cfg_printer_t *pctx, unsigned int flags) {
	struct flagtext *p;
	bool first = true;
	for (p = flagtexts; p->flag != 0; p++) {
		if ((flags & p->flag) != 0) {
			if (first) {
				cfg_print_cstr(pctx, " // ");
			} else {
				cfg_print_cstr(pctx, ", ");
			}
			cfg_print_cstr(pctx, p->text);
			first = false;
		}
	}
}

void
cfg_doc_mapbody(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_clausedef_t *const *clauseset;
	const cfg_clausedef_t *clause;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	for (clauseset = type->of; *clauseset != NULL; clauseset++) {
		for (clause = *clauseset; clause->name != NULL; clause++) {
			if (((pctx->flags & CFG_PRINTER_ACTIVEONLY) != 0) &&
			    (((clause->flags & CFG_CLAUSEFLAG_OBSOLETE) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_ANCIENT) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_NYI) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_TESTONLY) != 0)))
			{
				continue;
			}
			cfg_print_cstr(pctx, clause->name);
			cfg_print_cstr(pctx, " ");
			cfg_doc_obj(pctx, clause->type);
			cfg_print_cstr(pctx, ";");
			cfg_print_clauseflags(pctx, clause->flags);
			cfg_print_cstr(pctx, "\n\n");
		}
	}
}

void
cfg_print_map(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	if (obj->value.map.id != NULL) {
		cfg_print_obj(pctx, obj->value.map.id);
		cfg_print_cstr(pctx, " ");
	}
	print_open(pctx);
	cfg_print_mapbody(pctx, obj);
	print_close(pctx);
}

void
cfg_doc_map(cfg_printer_t *pctx, const cfg_type_t *type) {
	const cfg_clausedef_t *const *clauseset;
	const cfg_clausedef_t *clause;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	if (type->parse == cfg_parse_named_map) {
		cfg_doc_obj(pctx, &cfg_type_astring);
		cfg_print_cstr(pctx, " ");
	} else if (type->parse == cfg_parse_addressed_map) {
		cfg_doc_obj(pctx, &cfg_type_netaddr);
		cfg_print_cstr(pctx, " ");
	} else if (type->parse == cfg_parse_netprefix_map) {
		cfg_doc_obj(pctx, &cfg_type_netprefix);
		cfg_print_cstr(pctx, " ");
	}

	print_open(pctx);

	for (clauseset = type->of; *clauseset != NULL; clauseset++) {
		for (clause = *clauseset; clause->name != NULL; clause++) {
			if (((pctx->flags & CFG_PRINTER_ACTIVEONLY) != 0) &&
			    (((clause->flags & CFG_CLAUSEFLAG_OBSOLETE) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_ANCIENT) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_NYI) != 0) ||
			     ((clause->flags & CFG_CLAUSEFLAG_TESTONLY) != 0)))
			{
				continue;
			}
			cfg_print_indent(pctx);
			cfg_print_cstr(pctx, clause->name);
			if (clause->type->print != cfg_print_void) {
				cfg_print_cstr(pctx, " ");
			}
			cfg_doc_obj(pctx, clause->type);
			cfg_print_cstr(pctx, ";");
			cfg_print_clauseflags(pctx, clause->flags);
			cfg_print_cstr(pctx, "\n");
		}
	}
	print_close(pctx);
}

bool
cfg_obj_ismap(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_map);
}

isc_result_t
cfg_map_get(const cfg_obj_t *mapobj, const char *name, const cfg_obj_t **obj) {
	isc_result_t result;
	isc_symvalue_t val;
	const cfg_map_t *map;

	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);
	REQUIRE(name != NULL);
	REQUIRE(obj != NULL && *obj == NULL);

	map = &mapobj->value.map;

	result = isc_symtab_lookup(map->symtab, name, MAP_SYM, &val);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	*obj = val.as_pointer;
	return (ISC_R_SUCCESS);
}

const cfg_obj_t *
cfg_map_getname(const cfg_obj_t *mapobj) {
	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);
	return (mapobj->value.map.id);
}

unsigned int
cfg_map_count(const cfg_obj_t *mapobj) {
	const cfg_map_t *map;

	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);

	map = &mapobj->value.map;
	return (isc_symtab_count(map->symtab));
}

const char *
cfg_map_firstclause(const cfg_type_t *map, const void **clauses,
		    unsigned int *idx) {
	cfg_clausedef_t *const *clauseset;

	REQUIRE(map != NULL && map->rep == &cfg_rep_map);
	REQUIRE(idx != NULL);
	REQUIRE(clauses != NULL && *clauses == NULL);

	clauseset = map->of;
	if (*clauseset == NULL) {
		return (NULL);
	}
	*clauses = *clauseset;
	*idx = 0;
	while ((*clauseset)[*idx].name == NULL) {
		*clauses = (*++clauseset);
		if (*clauses == NULL) {
			return (NULL);
		}
	}
	return ((*clauseset)[*idx].name);
}

const char *
cfg_map_nextclause(const cfg_type_t *map, const void **clauses,
		   unsigned int *idx) {
	cfg_clausedef_t *const *clauseset;

	REQUIRE(map != NULL && map->rep == &cfg_rep_map);
	REQUIRE(idx != NULL);
	REQUIRE(clauses != NULL && *clauses != NULL);

	clauseset = map->of;
	while (*clauseset != NULL && *clauseset != *clauses) {
		clauseset++;
	}
	INSIST(*clauseset == *clauses);
	(*idx)++;
	while ((*clauseset)[*idx].name == NULL) {
		*idx = 0;
		*clauses = (*++clauseset);
		if (*clauses == NULL) {
			return (NULL);
		}
	}
	return ((*clauseset)[*idx].name);
}

/* Parse an arbitrary token, storing its raw text representation. */
static isc_result_t
parse_token(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *obj = NULL;
	isc_result_t result;
	isc_region_t r;

	UNUSED(type);

	CHECK(cfg_create_obj(pctx, &cfg_type_token, &obj));
	CHECK(cfg_gettoken(pctx, CFG_LEXOPT_QSTRING));
	if (pctx->token.type == isc_tokentype_eof) {
		cfg_ungettoken(pctx);
		result = ISC_R_EOF;
		goto cleanup;
	}

	isc_lex_getlasttokentext(pctx->lexer, &pctx->token, &r);

	obj->value.string.base = isc_mem_get(pctx->mctx, r.length + 1);
	obj->value.string.length = r.length;
	memmove(obj->value.string.base, r.base, r.length);
	obj->value.string.base[r.length] = '\0';
	*ret = obj;
	return (result);

cleanup:
	if (obj != NULL) {
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	}
	return (result);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_token = {
	"token",	  parse_token,	   cfg_print_ustring,
	cfg_doc_terminal, &cfg_rep_string, NULL
};

/*
 * An unsupported option.  This is just a list of tokens with balanced braces
 * ending in a semicolon.
 */

static isc_result_t
parse_unsupported(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *listobj = NULL;
	isc_result_t result;
	int braces = 0;

	CHECK(cfg_create_list(pctx, type, &listobj));

	for (;;) {
		cfg_listelt_t *elt = NULL;

		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_special) {
			if (pctx->token.value.as_char == '{') {
				braces++;
			} else if (pctx->token.value.as_char == '}') {
				braces--;
			} else if (pctx->token.value.as_char == ';') {
				if (braces == 0) {
					break;
				}
			}
		}
		if (pctx->token.type == isc_tokentype_eof || braces < 0) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "unexpected token");
			result = ISC_R_UNEXPECTEDTOKEN;
			goto cleanup;
		}

		CHECK(cfg_parse_listelt(pctx, &cfg_type_token, &elt));
		ISC_LIST_APPEND(listobj->value.list, elt, link);
	}
	INSIST(braces == 0);
	*ret = listobj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(listobj);
	return (result);
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_unsupported = {
	"unsupported",	  parse_unsupported, cfg_print_spacelist,
	cfg_doc_terminal, &cfg_rep_list,     NULL
};

/*
 * Try interpreting the current token as a network address.
 *
 * If CFG_ADDR_WILDOK is set in flags, "*" can be used as a wildcard
 * and at least one of CFG_ADDR_V4OK and CFG_ADDR_V6OK must also be set.  The
 * "*" is interpreted as the IPv4 wildcard address if CFG_ADDR_V4OK is
 * set (including the case where CFG_ADDR_V4OK and CFG_ADDR_V6OK are both set),
 * and the IPv6 wildcard address otherwise.
 */
static isc_result_t
token_addr(cfg_parser_t *pctx, unsigned int flags, isc_netaddr_t *na) {
	char *s;
	struct in_addr in4a;
	struct in6_addr in6a;

	if (pctx->token.type != isc_tokentype_string) {
		return (ISC_R_UNEXPECTEDTOKEN);
	}

	s = TOKEN_STRING(pctx);
	if ((flags & CFG_ADDR_WILDOK) != 0 && strcmp(s, "*") == 0) {
		if ((flags & CFG_ADDR_V4OK) != 0) {
			isc_netaddr_any(na);
			return (ISC_R_SUCCESS);
		} else if ((flags & CFG_ADDR_V6OK) != 0) {
			isc_netaddr_any6(na);
			return (ISC_R_SUCCESS);
		} else {
			UNREACHABLE();
		}
	} else {
		if ((flags & (CFG_ADDR_V4OK | CFG_ADDR_V4PREFIXOK)) != 0) {
			if (inet_pton(AF_INET, s, &in4a) == 1) {
				isc_netaddr_fromin(na, &in4a);
				return (ISC_R_SUCCESS);
			}
		}
		if ((flags & CFG_ADDR_V4PREFIXOK) != 0 && strlen(s) <= 15U) {
			char buf[64];
			int i;

			strlcpy(buf, s, sizeof(buf));
			for (i = 0; i < 3; i++) {
				strlcat(buf, ".0", sizeof(buf));
				if (inet_pton(AF_INET, buf, &in4a) == 1) {
					isc_netaddr_fromin(na, &in4a);
					return (ISC_R_IPV4PREFIX);
				}
			}
		}
		if ((flags & CFG_ADDR_V6OK) != 0 && strlen(s) <= 127U) {
			char buf[128];	   /* see lib/bind9/getaddresses.c */
			char *d;	   /* zone delimiter */
			uint32_t zone = 0; /* scope zone ID */

			strlcpy(buf, s, sizeof(buf));
			d = strchr(buf, '%');
			if (d != NULL) {
				*d = '\0';
			}

			if (inet_pton(AF_INET6, buf, &in6a) == 1) {
				if (d != NULL) {
					isc_result_t result;

					result = isc_netscope_pton(
						AF_INET6, d + 1, &in6a, &zone);
					if (result != ISC_R_SUCCESS) {
						return (result);
					}
				}

				isc_netaddr_fromin6(na, &in6a);
				isc_netaddr_setzone(na, zone);
				return (ISC_R_SUCCESS);
			}
		}
	}
	return (ISC_R_UNEXPECTEDTOKEN);
}

isc_result_t
cfg_parse_rawaddr(cfg_parser_t *pctx, unsigned int flags, isc_netaddr_t *na) {
	isc_result_t result;
	const char *wild = "";
	const char *prefix = "";

	REQUIRE(pctx != NULL);
	REQUIRE(na != NULL);

	CHECK(cfg_gettoken(pctx, 0));
	result = token_addr(pctx, flags, na);
	if (result == ISC_R_UNEXPECTEDTOKEN) {
		if ((flags & CFG_ADDR_WILDOK) != 0) {
			wild = " or '*'";
		}
		if ((flags & CFG_ADDR_V4PREFIXOK) != 0) {
			wild = " or IPv4 prefix";
		}
		if ((flags & CFG_ADDR_MASK) == CFG_ADDR_V4OK) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IPv4 address%s%s", prefix,
					 wild);
		} else if ((flags & CFG_ADDR_MASK) == CFG_ADDR_V6OK) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IPv6 address%s%s", prefix,
					 wild);
		} else {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected IP address%s%s", prefix,
					 wild);
		}
	}
cleanup:
	return (result);
}

bool
cfg_lookingat_netaddr(cfg_parser_t *pctx, unsigned int flags) {
	isc_result_t result;
	isc_netaddr_t na_dummy;

	REQUIRE(pctx != NULL);

	result = token_addr(pctx, flags, &na_dummy);
	return (result == ISC_R_SUCCESS || result == ISC_R_IPV4PREFIX);
}

isc_result_t
cfg_parse_rawport(cfg_parser_t *pctx, unsigned int flags, in_port_t *port) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(port != NULL);

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER));

	if ((flags & CFG_ADDR_WILDOK) != 0 &&
	    pctx->token.type == isc_tokentype_string &&
	    strcmp(TOKEN_STRING(pctx), "*") == 0)
	{
		*port = 0;
		return (ISC_R_SUCCESS);
	}
	if (pctx->token.type != isc_tokentype_number) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "expected port number or '*'");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	if (pctx->token.value.as_ulong >= 65536U) {
		cfg_parser_error(pctx, CFG_LOG_NEAR,
				 "port number out of range");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	*port = (in_port_t)(pctx->token.value.as_ulong);
	return (ISC_R_SUCCESS);
cleanup:
	return (result);
}

void
cfg_print_rawaddr(cfg_printer_t *pctx, const isc_netaddr_t *na) {
	isc_result_t result;
	char text[128];
	isc_buffer_t buf;

	REQUIRE(pctx != NULL);
	REQUIRE(na != NULL);

	isc_buffer_init(&buf, text, sizeof(text));
	result = isc_netaddr_totext(na, &buf);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	cfg_print_chars(pctx, isc_buffer_base(&buf),
			isc_buffer_usedlength(&buf));
}

isc_result_t
cfg_parse_dscp(cfg_parser_t *pctx, isc_dscp_t *dscp) {
	isc_result_t result;

	REQUIRE(pctx != NULL);
	REQUIRE(dscp != NULL);

	CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER | ISC_LEXOPT_CNUMBER));

	if (pctx->token.type != isc_tokentype_number) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected number");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	if (pctx->token.value.as_ulong > 63U) {
		cfg_parser_error(pctx, CFG_LOG_NEAR, "dscp out of range");
		return (ISC_R_RANGE);
	}
	*dscp = (isc_dscp_t)(pctx->token.value.as_ulong);
	return (ISC_R_SUCCESS);
cleanup:
	return (result);
}

/* netaddr */

static unsigned int netaddr_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK;
static unsigned int netaddr4_flags = CFG_ADDR_V4OK;
static unsigned int netaddr4wild_flags = CFG_ADDR_V4OK | CFG_ADDR_WILDOK;
static unsigned int netaddr6_flags = CFG_ADDR_V6OK;
static unsigned int netaddr6wild_flags = CFG_ADDR_V6OK | CFG_ADDR_WILDOK;

static isc_result_t
parse_netaddr(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	cfg_obj_t *obj = NULL;
	isc_netaddr_t netaddr;
	unsigned int flags = *(const unsigned int *)type->of;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(cfg_parse_rawaddr(pctx, flags, &netaddr));
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, 0);
	obj->value.sockaddrdscp.dscp = -1;
	*ret = obj;
	return (ISC_R_SUCCESS);
cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static void
cfg_doc_netaddr(cfg_printer_t *pctx, const cfg_type_t *type) {
	const unsigned int *flagp = type->of;
	int n = 0;
	if (*flagp != CFG_ADDR_V4OK && *flagp != CFG_ADDR_V6OK) {
		cfg_print_cstr(pctx, "( ");
	}
	if ((*flagp & CFG_ADDR_V4OK) != 0) {
		cfg_print_cstr(pctx, "<ipv4_address>");
		n++;
	}
	if ((*flagp & CFG_ADDR_V6OK) != 0) {
		if (n != 0) {
			cfg_print_cstr(pctx, " | ");
		}
		cfg_print_cstr(pctx, "<ipv6_address>");
		n++;
	}
	if ((*flagp & CFG_ADDR_WILDOK) != 0) {
		if (n != 0) {
			cfg_print_cstr(pctx, " | ");
		}
		cfg_print_cstr(pctx, "*");
		n++;
		POST(n);
	}
	if (*flagp != CFG_ADDR_V4OK && *flagp != CFG_ADDR_V6OK) {
		cfg_print_cstr(pctx, " )");
	}
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netaddr = {
	"netaddr",	 parse_netaddr,	    cfg_print_sockaddr,
	cfg_doc_netaddr, &cfg_rep_sockaddr, &netaddr_flags
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netaddr4 = {
	"netaddr4",	 parse_netaddr,	    cfg_print_sockaddr,
	cfg_doc_netaddr, &cfg_rep_sockaddr, &netaddr4_flags
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netaddr4wild = {
	"netaddr4wild",	 parse_netaddr,	    cfg_print_sockaddr,
	cfg_doc_netaddr, &cfg_rep_sockaddr, &netaddr4wild_flags
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netaddr6 = {
	"netaddr6",	 parse_netaddr,	    cfg_print_sockaddr,
	cfg_doc_netaddr, &cfg_rep_sockaddr, &netaddr6_flags
};

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netaddr6wild = {
	"netaddr6wild",	 parse_netaddr,	    cfg_print_sockaddr,
	cfg_doc_netaddr, &cfg_rep_sockaddr, &netaddr6wild_flags
};

/* netprefix */

isc_result_t
cfg_parse_netprefix(cfg_parser_t *pctx, const cfg_type_t *type,
		    cfg_obj_t **ret) {
	cfg_obj_t *obj = NULL;
	isc_result_t result;
	isc_netaddr_t netaddr;
	unsigned int addrlen = 0, prefixlen;
	bool expectprefix;

	REQUIRE(pctx != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	UNUSED(type);

	result = cfg_parse_rawaddr(
		pctx, CFG_ADDR_V4OK | CFG_ADDR_V4PREFIXOK | CFG_ADDR_V6OK,
		&netaddr);
	if (result != ISC_R_SUCCESS && result != ISC_R_IPV4PREFIX) {
		CHECK(result);
	}
	switch (netaddr.family) {
	case AF_INET:
		addrlen = 32;
		break;
	case AF_INET6:
		addrlen = 128;
		break;
	default:
		UNREACHABLE();
	}
	expectprefix = (result == ISC_R_IPV4PREFIX);
	CHECK(cfg_peektoken(pctx, 0));
	if (pctx->token.type == isc_tokentype_special &&
	    pctx->token.value.as_char == '/')
	{
		CHECK(cfg_gettoken(pctx, 0)); /* read "/" */
		CHECK(cfg_gettoken(pctx, ISC_LEXOPT_NUMBER));
		if (pctx->token.type != isc_tokentype_number) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "expected prefix length");
			return (ISC_R_UNEXPECTEDTOKEN);
		}
		prefixlen = pctx->token.value.as_ulong;
		if (prefixlen > addrlen) {
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
					 "invalid prefix length");
			return (ISC_R_RANGE);
		}
		result = isc_netaddr_prefixok(&netaddr, prefixlen);
		if (result != ISC_R_SUCCESS) {
			char buf[ISC_NETADDR_FORMATSIZE + 1];
			isc_netaddr_format(&netaddr, buf, sizeof(buf));
			cfg_parser_error(pctx, CFG_LOG_NOPREP,
					 "'%s/%u': address/prefix length "
					 "mismatch",
					 buf, prefixlen);
			return (ISC_R_FAILURE);
		}
	} else {
		if (expectprefix) {
			cfg_parser_error(pctx, CFG_LOG_NEAR,
					 "incomplete IPv4 address or prefix");
			return (ISC_R_FAILURE);
		}
		prefixlen = addrlen;
	}
	CHECK(cfg_create_obj(pctx, &cfg_type_netprefix, &obj));
	obj->value.netprefix.address = netaddr;
	obj->value.netprefix.prefixlen = prefixlen;
	*ret = obj;
	return (ISC_R_SUCCESS);
cleanup:
	cfg_parser_error(pctx, CFG_LOG_NEAR, "expected network prefix");
	return (result);
}

static void
print_netprefix(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	const cfg_netprefix_t *p = &obj->value.netprefix;

	cfg_print_rawaddr(pctx, &p->address);
	cfg_print_cstr(pctx, "/");
	cfg_print_rawuint(pctx, p->prefixlen);
}

bool
cfg_obj_isnetprefix(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_netprefix);
}

void
cfg_obj_asnetprefix(const cfg_obj_t *obj, isc_netaddr_t *netaddr,
		    unsigned int *prefixlen) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_netprefix);
	REQUIRE(netaddr != NULL);
	REQUIRE(prefixlen != NULL);

	*netaddr = obj->value.netprefix.address;
	*prefixlen = obj->value.netprefix.prefixlen;
}

LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_netprefix = {
	"netprefix",	  cfg_parse_netprefix, print_netprefix,
	cfg_doc_terminal, &cfg_rep_netprefix,  NULL
};

static isc_result_t
parse_sockaddrsub(cfg_parser_t *pctx, const cfg_type_t *type, int flags,
		  cfg_obj_t **ret) {
	isc_result_t result;
	isc_netaddr_t netaddr;
	in_port_t port = 0;
	isc_dscp_t dscp = -1;
	cfg_obj_t *obj = NULL;
	int have_port = 0, have_dscp = 0;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(cfg_parse_rawaddr(pctx, flags, &netaddr));
	for (;;) {
		CHECK(cfg_peektoken(pctx, 0));
		if (pctx->token.type == isc_tokentype_string) {
			if (strcasecmp(TOKEN_STRING(pctx), "port") == 0) {
				CHECK(cfg_gettoken(pctx, 0)); /* read "port" */
				CHECK(cfg_parse_rawport(pctx, flags, &port));
				++have_port;
			} else if ((flags & CFG_ADDR_DSCPOK) != 0 &&
				   strcasecmp(TOKEN_STRING(pctx), "dscp") == 0)
			{
				if ((pctx->flags & CFG_PCTX_NODEPRECATED) == 0)
				{
					cfg_parser_warning(
						pctx, 0,
						"token 'dscp' is deprecated");
				}
				CHECK(cfg_gettoken(pctx, 0)); /* read "dscp" */
				CHECK(cfg_parse_dscp(pctx, &dscp));
				++have_dscp;
			} else {
				break;
			}
		} else {
			break;
		}
	}
	if (have_port > 1) {
		cfg_parser_error(pctx, 0, "expected at most one port");
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}

	if (have_dscp > 1) {
		cfg_parser_error(pctx, 0, "expected at most one dscp");
		result = ISC_R_UNEXPECTEDTOKEN;
		goto cleanup;
	}
	isc_sockaddr_fromnetaddr(&obj->value.sockaddr, &netaddr, port);
	obj->value.sockaddrdscp.dscp = dscp;
	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	CLEANUP_OBJ(obj);
	return (result);
}

static unsigned int sockaddr_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK;
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_sockaddr = {
	"sockaddr",	  cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr,  &sockaddr_flags
};

static unsigned int sockaddrdscp_flags = CFG_ADDR_V4OK | CFG_ADDR_V6OK |
					 CFG_ADDR_DSCPOK;
LIBISCCFG_EXTERNAL_DATA cfg_type_t cfg_type_sockaddrdscp = {
	"sockaddr",	  cfg_parse_sockaddr, cfg_print_sockaddr,
	cfg_doc_sockaddr, &cfg_rep_sockaddr,  &sockaddrdscp_flags
};

isc_result_t
cfg_parse_sockaddr(cfg_parser_t *pctx, const cfg_type_t *type,
		   cfg_obj_t **ret) {
	const unsigned int *flagp;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	flagp = type->of;

	return (parse_sockaddrsub(pctx, &cfg_type_sockaddr, *flagp, ret));
}

void
cfg_print_sockaddr(cfg_printer_t *pctx, const cfg_obj_t *obj) {
	isc_netaddr_t netaddr;
	in_port_t port;
	char buf[ISC_NETADDR_FORMATSIZE];

	REQUIRE(pctx != NULL);
	REQUIRE(obj != NULL);

	isc_netaddr_fromsockaddr(&netaddr, &obj->value.sockaddr);
	isc_netaddr_format(&netaddr, buf, sizeof(buf));
	cfg_print_cstr(pctx, buf);
	port = isc_sockaddr_getport(&obj->value.sockaddr);
	if (port != 0) {
		cfg_print_cstr(pctx, " port ");
		cfg_print_rawuint(pctx, port);
	}
	if (obj->value.sockaddrdscp.dscp != -1) {
		cfg_print_cstr(pctx, " dscp ");
		cfg_print_rawuint(pctx, obj->value.sockaddrdscp.dscp);
	}
}

void
cfg_doc_sockaddr(cfg_printer_t *pctx, const cfg_type_t *type) {
	const unsigned int *flagp;
	int n = 0;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	flagp = type->of;

	cfg_print_cstr(pctx, "( ");
	if ((*flagp & CFG_ADDR_V4OK) != 0) {
		cfg_print_cstr(pctx, "<ipv4_address>");
		n++;
	}
	if ((*flagp & CFG_ADDR_V6OK) != 0) {
		if (n != 0) {
			cfg_print_cstr(pctx, " | ");
		}
		cfg_print_cstr(pctx, "<ipv6_address>");
		n++;
	}
	if ((*flagp & CFG_ADDR_WILDOK) != 0) {
		if (n != 0) {
			cfg_print_cstr(pctx, " | ");
		}
		cfg_print_cstr(pctx, "*");
		n++;
		POST(n);
	}
	cfg_print_cstr(pctx, " ) ");
	if ((*flagp & CFG_ADDR_WILDOK) != 0) {
		cfg_print_cstr(pctx, "[ port ( <integer> | * ) ]");
	} else {
		cfg_print_cstr(pctx, "[ port <integer> ]");
	}
	if ((*flagp & CFG_ADDR_DSCPOK) != 0) {
		cfg_print_cstr(pctx, " [ dscp <integer> ]");
	}
}

bool
cfg_obj_issockaddr(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);
	return (obj->type->rep == &cfg_rep_sockaddr);
}

const isc_sockaddr_t *
cfg_obj_assockaddr(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_sockaddr);
	return (&obj->value.sockaddr);
}

isc_dscp_t
cfg_obj_getdscp(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL && obj->type->rep == &cfg_rep_sockaddr);
	return (obj->value.sockaddrdscp.dscp);
}

isc_result_t
cfg_gettoken(cfg_parser_t *pctx, int options) {
	isc_result_t result;

	REQUIRE(pctx != NULL);

	if (pctx->seen_eof) {
		return (ISC_R_SUCCESS);
	}

	options |= (ISC_LEXOPT_EOF | ISC_LEXOPT_NOMORE);

redo:
	pctx->token.type = isc_tokentype_unknown;
	result = isc_lex_gettoken(pctx->lexer, options, &pctx->token);
	pctx->ungotten = false;
	pctx->line = isc_lex_getsourceline(pctx->lexer);

	switch (result) {
	case ISC_R_SUCCESS:
		if (pctx->token.type == isc_tokentype_eof) {
			result = isc_lex_close(pctx->lexer);
			INSIST(result == ISC_R_NOMORE ||
			       result == ISC_R_SUCCESS);

			if (isc_lex_getsourcename(pctx->lexer) != NULL) {
				/*
				 * Closed an included file, not the main file.
				 */
				cfg_listelt_t *elt;
				elt = ISC_LIST_TAIL(
					pctx->open_files->value.list);
				INSIST(elt != NULL);
				ISC_LIST_UNLINK(pctx->open_files->value.list,
						elt, link);
				ISC_LIST_APPEND(pctx->closed_files->value.list,
						elt, link);
				goto redo;
			}
			pctx->seen_eof = true;
		}
		break;

	case ISC_R_NOSPACE:
		/* More understandable than "ran out of space". */
		cfg_parser_error(pctx, CFG_LOG_NEAR, "token too big");
		break;

	case ISC_R_IOERROR:
		cfg_parser_error(pctx, 0, "%s", isc_result_totext(result));
		break;

	default:
		cfg_parser_error(pctx, CFG_LOG_NEAR, "%s",
				 isc_result_totext(result));
		break;
	}
	return (result);
}

void
cfg_ungettoken(cfg_parser_t *pctx) {
	REQUIRE(pctx != NULL);

	if (pctx->seen_eof) {
		return;
	}
	isc_lex_ungettoken(pctx->lexer, &pctx->token);
	pctx->ungotten = true;
}

isc_result_t
cfg_peektoken(cfg_parser_t *pctx, int options) {
	isc_result_t result;

	REQUIRE(pctx != NULL);

	CHECK(cfg_gettoken(pctx, options));
	cfg_ungettoken(pctx);
cleanup:
	return (result);
}

/*
 * Get a string token, accepting both the quoted and the unquoted form.
 * Log an error if the next token is not a string.
 */
static isc_result_t
cfg_getstringtoken(cfg_parser_t *pctx) {
	isc_result_t result;

	result = cfg_gettoken(pctx, CFG_LEXOPT_QSTRING);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (pctx->token.type != isc_tokentype_string &&
	    pctx->token.type != isc_tokentype_qstring)
	{
		cfg_parser_error(pctx, CFG_LOG_NEAR, "expected string");
		return (ISC_R_UNEXPECTEDTOKEN);
	}
	return (ISC_R_SUCCESS);
}

void
cfg_parser_error(cfg_parser_t *pctx, unsigned int flags, const char *fmt, ...) {
	va_list args;

	REQUIRE(pctx != NULL);
	REQUIRE(fmt != NULL);

	va_start(args, fmt);
	parser_complain(pctx, false, flags, fmt, args);
	va_end(args);
	pctx->errors++;
}

void
cfg_parser_warning(cfg_parser_t *pctx, unsigned int flags, const char *fmt,
		   ...) {
	va_list args;

	REQUIRE(pctx != NULL);
	REQUIRE(fmt != NULL);

	va_start(args, fmt);
	parser_complain(pctx, true, flags, fmt, args);
	va_end(args);
	pctx->warnings++;
}

#define MAX_LOG_TOKEN 30 /* How much of a token to quote in log messages. */

static bool
have_current_file(cfg_parser_t *pctx) {
	cfg_listelt_t *elt;
	if (pctx->open_files == NULL) {
		return (false);
	}

	elt = ISC_LIST_TAIL(pctx->open_files->value.list);
	if (elt == NULL) {
		return (false);
	}

	return (true);
}

static char *
current_file(cfg_parser_t *pctx) {
	static char none[] = "none";
	cfg_listelt_t *elt;
	cfg_obj_t *fileobj;

	if (!have_current_file(pctx)) {
		return (none);
	}

	elt = ISC_LIST_TAIL(pctx->open_files->value.list);
	if (elt == NULL) { /* shouldn't be possible, but... */
		return (none);
	}

	fileobj = elt->obj;
	INSIST(fileobj->type == &cfg_type_qstring);
	return (fileobj->value.string.base);
}

static void
parser_complain(cfg_parser_t *pctx, bool is_warning, unsigned int flags,
		const char *format, va_list args) {
	char tokenbuf[MAX_LOG_TOKEN + 10];
	static char where[PATH_MAX + 100];
	static char message[2048];
	int level = ISC_LOG_ERROR;
	const char *prep = "";
	size_t len;

	if (is_warning) {
		level = ISC_LOG_WARNING;
	}

	where[0] = '\0';
	if (have_current_file(pctx)) {
		snprintf(where, sizeof(where), "%s:%u: ", current_file(pctx),
			 pctx->line);
	} else if (pctx->buf_name != NULL) {
		snprintf(where, sizeof(where), "%s: ", pctx->buf_name);
	}

	len = vsnprintf(message, sizeof(message), format, args);
#define ELLIPSIS " ... "
	if (len >= sizeof(message)) {
		message[sizeof(message) - sizeof(ELLIPSIS)] = 0;
		strlcat(message, ELLIPSIS, sizeof(message));
	}

	if ((flags & (CFG_LOG_NEAR | CFG_LOG_BEFORE | CFG_LOG_NOPREP)) != 0) {
		isc_region_t r;

		if (pctx->ungotten) {
			(void)cfg_gettoken(pctx, 0);
		}

		if (pctx->token.type == isc_tokentype_eof) {
			snprintf(tokenbuf, sizeof(tokenbuf), "end of file");
		} else if (pctx->token.type == isc_tokentype_unknown) {
			flags = 0;
			tokenbuf[0] = '\0';
		} else {
			isc_lex_getlasttokentext(pctx->lexer, &pctx->token, &r);
			if (r.length > MAX_LOG_TOKEN) {
				snprintf(tokenbuf, sizeof(tokenbuf),
					 "'%.*s...'", MAX_LOG_TOKEN, r.base);
			} else {
				snprintf(tokenbuf, sizeof(tokenbuf), "'%.*s'",
					 (int)r.length, r.base);
			}
		}

		/* Choose a preposition. */
		if ((flags & CFG_LOG_NEAR) != 0) {
			prep = " near ";
		} else if ((flags & CFG_LOG_BEFORE) != 0) {
			prep = " before ";
		} else {
			prep = " ";
		}
	} else {
		tokenbuf[0] = '\0';
	}
	isc_log_write(pctx->lctx, CAT, MOD, level, "%s%s%s%s", where, message,
		      prep, tokenbuf);
}

void
cfg_obj_log(const cfg_obj_t *obj, isc_log_t *lctx, int level, const char *fmt,
	    ...) {
	va_list ap;
	char msgbuf[2048];

	REQUIRE(obj != NULL);
	REQUIRE(fmt != NULL);

	if (!isc_log_wouldlog(lctx, level)) {
		return;
	}

	va_start(ap, fmt);
	vsnprintf(msgbuf, sizeof(msgbuf), fmt, ap);
	va_end(ap);

	if (obj->file != NULL) {
		isc_log_write(lctx, CAT, MOD, level, "%s:%u: %s", obj->file,
			      obj->line, msgbuf);
	} else {
		isc_log_write(lctx, CAT, MOD, level, "%s", msgbuf);
	}
}

const char *
cfg_obj_file(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);

	return (obj->file);
}

unsigned int
cfg_obj_line(const cfg_obj_t *obj) {
	REQUIRE(obj != NULL);

	return (obj->line);
}

isc_result_t
cfg_create_obj(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	cfg_obj_t *obj;

	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);
	REQUIRE(ret != NULL && *ret == NULL);

	obj = isc_mem_get(pctx->mctx, sizeof(cfg_obj_t));

	obj->type = type;
	obj->file = current_file(pctx);
	obj->line = pctx->line;
	obj->pctx = pctx;

	isc_refcount_init(&obj->references, 1);

	*ret = obj;

	return (ISC_R_SUCCESS);
}

static void
map_symtabitem_destroy(char *key, unsigned int type, isc_symvalue_t symval,
		       void *userarg) {
	cfg_obj_t *obj = symval.as_pointer;
	cfg_parser_t *pctx = (cfg_parser_t *)userarg;

	UNUSED(key);
	UNUSED(type);

	cfg_obj_destroy(pctx, &obj);
}

static isc_result_t
create_map(cfg_parser_t *pctx, const cfg_type_t *type, cfg_obj_t **ret) {
	isc_result_t result;
	isc_symtab_t *symtab = NULL;
	cfg_obj_t *obj = NULL;

	CHECK(cfg_create_obj(pctx, type, &obj));
	CHECK(isc_symtab_create(pctx->mctx, 5, /* XXX */
				map_symtabitem_destroy, pctx, false, &symtab));
	obj->value.map.symtab = symtab;
	obj->value.map.id = NULL;

	*ret = obj;
	return (ISC_R_SUCCESS);

cleanup:
	if (obj != NULL) {
		isc_mem_put(pctx->mctx, obj, sizeof(*obj));
	}
	return (result);
}

static void
free_map(cfg_parser_t *pctx, cfg_obj_t *obj) {
	CLEANUP_OBJ(obj->value.map.id);
	isc_symtab_destroy(&obj->value.map.symtab);
}

bool
cfg_obj_istype(const cfg_obj_t *obj, const cfg_type_t *type) {
	REQUIRE(obj != NULL);
	REQUIRE(type != NULL);

	return (obj->type == type);
}

/*
 * Destroy 'obj', a configuration object created in 'pctx'.
 */
void
cfg_obj_destroy(cfg_parser_t *pctx, cfg_obj_t **objp) {
	REQUIRE(objp != NULL && *objp != NULL);
	REQUIRE(pctx != NULL);

	cfg_obj_t *obj = *objp;
	*objp = NULL;

	if (isc_refcount_decrement(&obj->references) == 1) {
		obj->type->rep->free(pctx, obj);
		isc_refcount_destroy(&obj->references);
		isc_mem_put(pctx->mctx, obj, sizeof(cfg_obj_t));
	}
}

void
cfg_obj_attach(cfg_obj_t *src, cfg_obj_t **dest) {
	REQUIRE(src != NULL);
	REQUIRE(dest != NULL && *dest == NULL);

	isc_refcount_increment(&src->references);
	*dest = src;
}

static void
free_noop(cfg_parser_t *pctx, cfg_obj_t *obj) {
	UNUSED(pctx);
	UNUSED(obj);
}

void
cfg_doc_obj(cfg_printer_t *pctx, const cfg_type_t *type) {
	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	type->doc(pctx, type);
}

void
cfg_doc_terminal(cfg_printer_t *pctx, const cfg_type_t *type) {
	REQUIRE(pctx != NULL);
	REQUIRE(type != NULL);

	cfg_print_cstr(pctx, "<");
	cfg_print_cstr(pctx, type->name);
	cfg_print_cstr(pctx, ">");
}

void
cfg_print_grammar(const cfg_type_t *type, unsigned int flags,
		  void (*f)(void *closure, const char *text, int textlen),
		  void *closure) {
	cfg_printer_t pctx;

	pctx.f = f;
	pctx.closure = closure;
	pctx.indent = 0;
	pctx.flags = flags;
	cfg_doc_obj(&pctx, type);
}

isc_result_t
cfg_parser_mapadd(cfg_parser_t *pctx, cfg_obj_t *mapobj, cfg_obj_t *obj,
		  const char *clausename) {
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_map_t *map;
	isc_symvalue_t symval;
	cfg_obj_t *destobj = NULL;
	cfg_listelt_t *elt = NULL;
	const cfg_clausedef_t *const *clauseset;
	const cfg_clausedef_t *clause;

	REQUIRE(pctx != NULL);
	REQUIRE(mapobj != NULL && mapobj->type->rep == &cfg_rep_map);
	REQUIRE(obj != NULL);
	REQUIRE(clausename != NULL);

	map = &mapobj->value.map;

	clause = NULL;
	for (clauseset = map->clausesets; *clauseset != NULL; clauseset++) {
		for (clause = *clauseset; clause->name != NULL; clause++) {
			if (strcasecmp(clause->name, clausename) == 0) {
				goto breakout;
			}
		}
	}

breakout:
	if (clause == NULL || clause->name == NULL) {
		return (ISC_R_FAILURE);
	}

	result = isc_symtab_lookup(map->symtab, clausename, 0, &symval);
	if (result == ISC_R_NOTFOUND) {
		if ((clause->flags & CFG_CLAUSEFLAG_MULTI) != 0) {
			CHECK(cfg_create_list(pctx, &cfg_type_implicitlist,
					      &destobj));
			CHECK(create_listelt(pctx, &elt));
			cfg_obj_attach(obj, &elt->obj);
			ISC_LIST_APPEND(destobj->value.list, elt, link);
			symval.as_pointer = destobj;
		} else {
			symval.as_pointer = obj;
		}

		CHECK(isc_symtab_define(map->symtab, clausename, 1, symval,
					isc_symexists_reject));
	} else {
		cfg_obj_t *destobj2 = symval.as_pointer;

		INSIST(result == ISC_R_SUCCESS);

		if (destobj2->type == &cfg_type_implicitlist) {
			CHECK(create_listelt(pctx, &elt));
			cfg_obj_attach(obj, &elt->obj);
			ISC_LIST_APPEND(destobj2->value.list, elt, link);
		} else {
			result = ISC_R_EXISTS;
		}
	}

	destobj = NULL;
	elt = NULL;

cleanup:
	if (elt != NULL) {
		free_listelt(pctx, elt);
	}
	CLEANUP_OBJ(destobj);

	return (result);
}

isc_result_t
cfg_pluginlist_foreach(const cfg_obj_t *config, const cfg_obj_t *list,
		       isc_log_t *lctx, pluginlist_cb_t *callback,
		       void *callback_data) {
	isc_result_t result = ISC_R_SUCCESS;
	const cfg_listelt_t *element;

	REQUIRE(config != NULL);
	REQUIRE(callback != NULL);

	for (element = cfg_list_first(list); element != NULL;
	     element = cfg_list_next(element))
	{
		const cfg_obj_t *plugin = cfg_listelt_value(element);
		const cfg_obj_t *obj;
		const char *type, *library;
		const char *parameters = NULL;

		/* Get the path to the plugin module. */
		obj = cfg_tuple_get(plugin, "type");
		type = cfg_obj_asstring(obj);

		/* Only query plugins are supported currently. */
		if (strcasecmp(type, "query") != 0) {
			cfg_obj_log(obj, lctx, ISC_LOG_ERROR,
				    "unsupported plugin type");
			return (ISC_R_FAILURE);
		}

		library = cfg_obj_asstring(cfg_tuple_get(plugin, "library"));

		obj = cfg_tuple_get(plugin, "parameters");
		if (obj != NULL && cfg_obj_isstring(obj)) {
			parameters = cfg_obj_asstring(obj);
		}

		result = callback(config, obj, library, parameters,
				  callback_data);
		if (result != ISC_R_SUCCESS) {
			break;
		}
	}

	return (result);
}
