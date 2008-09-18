/* $OpenLDAP: pkg/ldap/servers/slapd/back-perl/perl_back.h,v 1.15.2.3 2008/02/11 23:26:47 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2008 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef PERL_BACK_H
#define PERL_BACK_H 1

#include <EXTERN.h>
#include <perl.h>
#undef _	/* #defined by both Perl and ac/localize.h */
#include "asperl_undefs.h"

#include "portable.h"

#include "slap.h"

LDAP_BEGIN_DECL

/*
 * From Apache mod_perl: test for Perl version.
 */

#if defined(pTHX_) || (PERL_REVISION > 5 || (PERL_REVISION == 5 && PERL_VERSION >= 6))
#define PERL_IS_5_6
#endif

#define EVAL_BUF_SIZE 500

extern ldap_pvt_thread_mutex_t  perl_interpreter_mutex;

#ifdef PERL_IS_5_6
/* We should be using the PL_errgv, I think */
/* All the old style variables are prefixed with PL_ now */
# define errgv	PL_errgv
# define na	PL_na
#endif

#if defined( HAVE_WIN32_ASPERL ) || defined( USE_ITHREADS )
/* pTHX is needed often now */
# define PERL_INTERPRETER			my_perl
# define PERL_BACK_XS_INIT_PARAMS		pTHX
# define PERL_BACK_BOOT_DYNALOADER_PARAMS	pTHX, CV *cv
#else
# define PERL_INTERPRETER			perl_interpreter
# define PERL_BACK_XS_INIT_PARAMS		void
# define PERL_BACK_BOOT_DYNALOADER_PARAMS	CV *cv
#endif

extern PerlInterpreter *PERL_INTERPRETER;


typedef struct perl_backend_instance {
	char	*pb_module_name;
	SV	*pb_obj_ref;
	int	pb_filter_search_results;
} PerlBackend;

LDAP_END_DECL

#include "proto-perl.h"

#endif /* PERL_BACK_H */
