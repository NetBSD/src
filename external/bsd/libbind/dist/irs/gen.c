/*	$NetBSD: gen.c,v 1.1.1.1.14.1 2012/10/30 18:55:26 yamt Exp $	*/

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(LINT) && !defined(CODECENTER)
static const char rcsid[] = "Id: gen.c,v 1.7 2005/04/27 04:56:23 sra Exp ";
#endif

/*! \file
 * \brief
 * this is the top level dispatcher
 *
 * The dispatcher is implemented as an accessor class; it is an
 * accessor class that calls other accessor classes, as controlled by a
 * configuration file.
 * 
 * A big difference between this accessor class and others is that the
 * map class initializers are NULL, and the map classes are already
 * filled in with method functions that will do the right thing.
 */

/* Imports */

#include "port_before.h"

#include <isc/assertions.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/nameser.h>
#include <resolv.h>

#include <isc/memcluster.h>
#include <irs.h>

#include "port_after.h"

#include "irs_p.h"
#include "gen_p.h"

/* Definitions */

struct nameval {
	const char *	name;
	int		val;
};

static const struct nameval acc_names[irs_nacc+1] = {
	{ "local", irs_lcl },
	{ "dns", irs_dns },
	{ "nis", irs_nis },
	{ "irp", irs_irp },
	{ NULL, irs_nacc }
};

typedef struct irs_acc *(*accinit) __P((const char *options));

static const accinit accs[irs_nacc+1] = {
	irs_lcl_acc,
	irs_dns_acc,
#ifdef WANT_IRS_NIS
	irs_nis_acc,
#else
	NULL,
#endif
	irs_irp_acc,
	NULL
};

static const struct nameval map_names[irs_nmap+1] = {
	{ "group", irs_gr },
	{ "passwd", irs_pw },
	{ "services", irs_sv },
	{ "protocols", irs_pr },
	{ "hosts", irs_ho },
	{ "networks", irs_nw },
	{ "netgroup", irs_ng },
	{ NULL, irs_nmap }
};

static const struct nameval option_names[] = {
	{ "merge", IRS_MERGE },
	{ "continue", IRS_CONTINUE },
	{ NULL, 0 }
};

/* Forward */

static void		gen_close(struct irs_acc *);
static struct __res_state * gen_res_get(struct irs_acc *);
static void		gen_res_set(struct irs_acc *, struct __res_state *,
				    void (*)(void *));
static int		find_name(const char *, const struct nameval nv[]);
static void 		init_map_rules(struct gen_p *, const char *conf_file);
static struct irs_rule *release_rule(struct irs_rule *);
static int		add_rule(struct gen_p *,
				 enum irs_map_id, enum irs_acc_id,
				 const char *);

/* Public */

struct irs_acc *
irs_gen_acc(const char *options, const char *conf_file) {
	struct irs_acc *acc;
	struct gen_p *irs;
		
	if (!(acc = memget(sizeof *acc))) {
		errno = ENOMEM;
		return (NULL);
	}
	memset(acc, 0x5e, sizeof *acc);
	if (!(irs = memget(sizeof *irs))) {
		errno = ENOMEM;
		memput(acc, sizeof *acc);
		return (NULL);
	}
	memset(irs, 0x5e, sizeof *irs);
	irs->options = strdup(options);
	irs->res = NULL;
	irs->free_res = NULL;
	memset(irs->accessors, 0, sizeof irs->accessors);
	memset(irs->map_rules, 0, sizeof irs->map_rules);
	init_map_rules(irs, conf_file);
	acc->private = irs;
#ifdef WANT_IRS_GR
	acc->gr_map = irs_gen_gr;
#else
	acc->gr_map = NULL;
#endif
#ifdef WANT_IRS_PW
	acc->pw_map = irs_gen_pw;
#else
	acc->pw_map = NULL;
#endif
	acc->sv_map = irs_gen_sv;
	acc->pr_map = irs_gen_pr;
	acc->ho_map = irs_gen_ho;
	acc->nw_map = irs_gen_nw;
	acc->ng_map = irs_gen_ng;
	acc->res_get = gen_res_get;
	acc->res_set = gen_res_set;
	acc->close = gen_close;
	return (acc);
}

/* Methods */

static struct __res_state *
gen_res_get(struct irs_acc *this) {
	struct gen_p *irs = (struct gen_p *)this->private;

	if (irs->res == NULL) {
		struct __res_state *res;
		res = (struct __res_state *)malloc(sizeof *res);
		if (res == NULL)
			return (NULL);
		memset(res, 0, sizeof *res);
		gen_res_set(this, res, free);
	}

	if (((irs->res->options & RES_INIT) == 0U) && res_ninit(irs->res) < 0)
		return (NULL);

	return (irs->res);
}

static void
gen_res_set(struct irs_acc *this, struct __res_state *res,
	    void (*free_res)(void *)) {
	struct gen_p *irs = (struct gen_p *)this->private;
#if 0
	struct irs_rule *rule;
	struct irs_ho *ho;
	struct irs_nw *nw;
#endif

	if (irs->res && irs->free_res) {
		res_nclose(irs->res);
		(*irs->free_res)(irs->res);
	}

	irs->res = res;
	irs->free_res = free_res;

#if 0
	for (rule = irs->map_rules[irs_ho]; rule; rule = rule->next) {
                ho = rule->inst->ho;

                (*ho->res_set)(ho, res, NULL);
	}
	for (rule = irs->map_rules[irs_nw]; rule; rule = rule->next) {
                nw = rule->inst->nw;

                (*nw->res_set)(nw, res, NULL);
	}
#endif
}

static void
gen_close(struct irs_acc *this) {
	struct gen_p *irs = (struct gen_p *)this->private;
	int n;
	
	/* Search rules. */
	for (n = 0; n < irs_nmap; n++)
		while (irs->map_rules[n] != NULL)
			irs->map_rules[n] = release_rule(irs->map_rules[n]);

	/* Access methods. */
	for (n = 0; n < irs_nacc; n++) {
		/* Map objects. */
		if (irs->accessors[n].gr != NULL)
			(*irs->accessors[n].gr->close)(irs->accessors[n].gr);
		if (irs->accessors[n].pw != NULL)
			(*irs->accessors[n].pw->close)(irs->accessors[n].pw);
		if (irs->accessors[n].sv != NULL)
			(*irs->accessors[n].sv->close)(irs->accessors[n].sv);
		if (irs->accessors[n].pr != NULL)
			(*irs->accessors[n].pr->close)(irs->accessors[n].pr);
		if (irs->accessors[n].ho != NULL)
			(*irs->accessors[n].ho->close)(irs->accessors[n].ho);
		if (irs->accessors[n].nw != NULL)
			(*irs->accessors[n].nw->close)(irs->accessors[n].nw);
		if (irs->accessors[n].ng != NULL)
			(*irs->accessors[n].ng->close)(irs->accessors[n].ng);
		/* Enclosing accessor. */
		if (irs->accessors[n].acc != NULL)
			(*irs->accessors[n].acc->close)(irs->accessors[n].acc);
	}

	/* The options string was strdup'd. */
	free((void*)irs->options);

	if (irs->res && irs->free_res)
		(*irs->free_res)(irs->res);

	/* The private data container. */
	memput(irs, sizeof *irs);

	/* The object. */
	memput(this, sizeof *this);
}

/* Private */

static int
find_name(const char *name, const struct nameval names[]) {
	int n;

	for (n = 0; names[n].name != NULL; n++)
		if (strcmp(name, names[n].name) == 0)
			return (names[n].val);
	return (-1);
}

static struct irs_rule *
release_rule(struct irs_rule *rule) {
	struct irs_rule *next = rule->next;

	memput(rule, sizeof *rule);
	return (next);
}

static int
add_rule(struct gen_p *irs,
	 enum irs_map_id map, enum irs_acc_id acc,
	 const char *options)
{
	struct irs_rule **rules, *last, *tmp, *new;
	struct irs_inst *inst;
	const char *cp;
	int n;

#ifndef WANT_IRS_GR
	if (map == irs_gr)
		return (-1);
#endif
#ifndef WANT_IRS_PW
	if (map == irs_pw)
		return (-1);
#endif
#ifndef WANT_IRS_NIS
	if (acc == irs_nis)
		return (-1);
#endif
	new = memget(sizeof *new);
	if (new == NULL)
		return (-1);
	memset(new, 0x5e, sizeof *new);
	new->next = NULL;

	new->inst = &irs->accessors[acc];

	new->flags = 0;
	cp = options;
	while (cp && *cp) {
		char option[50], *next;

		next = strchr(cp, ',');
		if (next)
			n = next++ - cp;
		else
			n = strlen(cp);
		if ((size_t)n > sizeof option - 1)
			n = sizeof option - 1;
		strncpy(option, cp, n);
		option[n] = '\0';

		n = find_name(option, option_names);
		if (n >= 0)
			new->flags |= n;

		cp = next;
	}

	rules = &irs->map_rules[map];
	for (last = NULL, tmp = *rules;
	     tmp != NULL;
	     last = tmp, tmp = tmp->next)
		(void)NULL;
	if (last == NULL)
		*rules = new;
	else
		last->next = new;

	/* Try to instantiate map accessors for this if necessary & approp. */
	inst = &irs->accessors[acc];
	if (inst->acc == NULL && accs[acc] != NULL)
		inst->acc = (*accs[acc])(irs->options);
	if (inst->acc != NULL) {
		if (inst->gr == NULL && inst->acc->gr_map != NULL)
			inst->gr = (*inst->acc->gr_map)(inst->acc);
		if (inst->pw == NULL && inst->acc->pw_map != NULL)
			inst->pw = (*inst->acc->pw_map)(inst->acc);
		if (inst->sv == NULL && inst->acc->sv_map != NULL)
			inst->sv = (*inst->acc->sv_map)(inst->acc);
		if (inst->pr == NULL && inst->acc->pr_map != NULL)
			inst->pr = (*inst->acc->pr_map)(inst->acc);
		if (inst->ho == NULL && inst->acc->ho_map != NULL)
			inst->ho = (*inst->acc->ho_map)(inst->acc);
		if (inst->nw == NULL && inst->acc->nw_map != NULL)
			inst->nw = (*inst->acc->nw_map)(inst->acc);
		if (inst->ng == NULL && inst->acc->ng_map != NULL)
			inst->ng = (*inst->acc->ng_map)(inst->acc);
	}

	return (0);
}

static void
default_map_rules(struct gen_p *irs) {
	/* Install time honoured and proved BSD style rules as default. */
	add_rule(irs, irs_gr, irs_lcl, "");
	add_rule(irs, irs_pw, irs_lcl, "");
	add_rule(irs, irs_sv, irs_lcl, "");
	add_rule(irs, irs_pr, irs_lcl, "");
	add_rule(irs, irs_ho, irs_dns, "continue");
	add_rule(irs, irs_ho, irs_lcl, "");
	add_rule(irs, irs_nw, irs_dns, "continue");
	add_rule(irs, irs_nw, irs_lcl, "");
	add_rule(irs, irs_ng, irs_lcl, "");
}

static void
init_map_rules(struct gen_p *irs, const char *conf_file) {
	char line[1024], pattern[40], mapname[20], accname[20], options[100];
	FILE *conf;

	if (conf_file == NULL) 
		conf_file = _PATH_IRS_CONF ;

	/* A conf file of "" means compiled in defaults. Irpd wants this */
	if (conf_file[0] == '\0' || (conf = fopen(conf_file, "r")) == NULL) {
		default_map_rules(irs);
		return;
	}
	(void) sprintf(pattern, "%%%lus %%%lus %%%lus\n",
		       (unsigned long)sizeof mapname,
		       (unsigned long)sizeof accname,
		       (unsigned long)sizeof options);
	while (fgets(line, sizeof line, conf)) {
		enum irs_map_id map;
		enum irs_acc_id acc;
		char *tmp;
		int n;

		for (tmp = line;
		     isascii((unsigned char)*tmp) &&
		     isspace((unsigned char)*tmp);
		     tmp++)
			(void)NULL;
		if (*tmp == '#' || *tmp == '\n' || *tmp == '\0')
			continue;
		n = sscanf(tmp, pattern, mapname, accname, options);
		if (n < 2)
			continue;
		if (n < 3)
			options[0] = '\0';

		n = find_name(mapname, map_names);
		INSIST(n < irs_nmap);
		if (n < 0)
			continue;
		map = (enum irs_map_id) n;

		n = find_name(accname, acc_names);
		INSIST(n < irs_nacc);
		if (n < 0)
			continue;
		acc = (enum irs_acc_id) n;

		add_rule(irs, map, acc, options);
	}
	fclose(conf);
}
