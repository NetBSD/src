/*	$NetBSD: openpam_impl.h,v 1.2.4.2 2012/04/17 00:03:59 yamt Exp $	*/

/*-
 * Copyright (c) 2001-2003 Networks Associates Technology, Inc.
 * Copyright (c) 2004-2011 Dag-Erling Smørgrav
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by ThinkSec AS and
 * Network Associates Laboratories, the Security Research Division of
 * Network Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Id: openpam_impl.h 499 2011-11-22 11:51:50Z des
 */

#ifndef OPENPAM_IMPL_H_INCLUDED
#define OPENPAM_IMPL_H_INCLUDED

#include <security/openpam.h>

extern int openpam_debug;

/*
 * Control flags
 */
typedef enum {
	PAM_BINDING,
	PAM_REQUIRED,
	PAM_REQUISITE,
	PAM_SUFFICIENT,
	PAM_OPTIONAL,
	PAM_NUM_CONTROL_FLAGS
} pam_control_t;

/*
 * Facilities
 */
typedef enum {
	PAM_FACILITY_ANY = -1,
	PAM_AUTH = 0,
	PAM_ACCOUNT,
	PAM_SESSION,
	PAM_PASSWORD,
	PAM_NUM_FACILITIES
} pam_facility_t;

/*
 * Module chains
 */
typedef struct pam_chain pam_chain_t;
struct pam_chain {
	pam_module_t	*module;
	int		 flag;
	int		 optc;
	char	       **optv;
	pam_chain_t	*next;
};

/*
 * Service policies
 */
#if defined(OPENPAM_EMBEDDED)
typedef struct pam_policy pam_policy_t;
struct pam_policy {
	const char	*service;
	pam_chain_t	*chains[PAM_NUM_FACILITIES];
};
extern pam_policy_t *pam_embedded_policies[];
#endif

/*
 * Module-specific data
 */
typedef struct pam_data pam_data_t;
struct pam_data {
	char		*name;
	void		*data;
	void		(*cleanup)(pam_handle_t *, void *, int);
	pam_data_t	*next;
};

/*
 * PAM context
 */
struct pam_handle {
	char		*service;

	/* chains */
	pam_chain_t	*chains[PAM_NUM_FACILITIES];
	pam_chain_t	*current;
	int		 primitive;

	/* items and data */
	void		*item[PAM_NUM_ITEMS];
	pam_data_t	*module_data;

	/* environment list */
	char	       **env;
	int		 env_count;
	int		 env_size;
};

#ifdef NGROUPS_MAX
/*
 * Saved credentials
 */
#define PAM_SAVED_CRED "pam_saved_cred"
struct pam_saved_cred {
	uid_t	 euid;
	gid_t	 egid;
	gid_t	 groups[NGROUPS_MAX];
	int	 ngroups;
};
#endif

/*
 * Default policy
 */
#define PAM_OTHER	"other"

/*
 * Internal functions
 */
int		 openpam_configure(pam_handle_t *, const char *);
int		 openpam_dispatch(pam_handle_t *, int, int);
int		 openpam_findenv(pam_handle_t *, const char *, size_t);
pam_module_t	*openpam_load_module(const char *);
void		 openpam_clear_chains(pam_chain_t **);

int		 openpam_check_desc_owner_perms(const char *, int);
int		 openpam_check_path_owner_perms(const char *);

#ifdef OPENPAM_STATIC_MODULES
pam_module_t	*openpam_static(const char *);
#endif
pam_module_t	*openpam_dynamic(const char *);

#define	FREE(p) do { free((p)); (p) = NULL; } while (/*CONSTCOND*/0)

#include "openpam_constants.h"
#include "openpam_debug.h"

#endif
