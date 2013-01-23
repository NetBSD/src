/*	$NetBSD: postconf_service.c,v 1.1.1.1.2.2 2013/01/23 00:05:06 yamt Exp $	*/

/*++
/* NAME
/*	postconf_service 3
/* SUMMARY
/*	service-defined parameter name support
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	register_service_parameters()
/* DESCRIPTION
/*	Service-defined parameter names are created by appending
/*	postfix-defined suffixes to master.cf service names. All
/*	service-defined parameters have default values that are
/*	defined by a built-in parameter.
/*
/*	register_service_parameters() adds the service-defined parameters
/*	to the global name space. This function must be called after
/*	the built-in parameters are added to the global name space,
/*	and after the master.cf file is read.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <vstring.h>
#include <stringops.h>
#include <argv.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Basename of programs in $daemon_directory. XXX These belong in a header
  * file, or they should be made configurable.
  */
#ifndef MAIL_PROGRAM_LOCAL
#define MAIL_PROGRAM_LOCAL	"local"
#define MAIL_PROGRAM_ERROR	"error"
#define MAIL_PROGRAM_VIRTUAL	"virtual"
#define MAIL_PROGRAM_SMTP	"smtp"
#define MAIL_PROGRAM_LMTP	"lmtp"
#define MAIL_PROGRAM_PIPE	"pipe"
#define MAIL_PROGRAM_SPAWN	"spawn"
#endif

 /*
  * Ad-hoc name-value string pair.
  */
typedef struct {
    const char *name;
    const char *value;
} PC_STRING_NV;

#define STR(x) vstring_str(x)

/* convert_service_parameter - get service parameter string value */

static const char *convert_service_parameter(char *ptr)
{
    return (STR(vstring_sprintf(param_string_buf, "$%s", ptr)));
}

/* register_service_parameter - add one service parameter name and default */

static void register_service_parameter(const char *service, const char *suffix,
				               const char *defparam)
{
    char   *name = concatenate(service, suffix, (char *) 0);
    PC_PARAM_NODE *node;

    /*
     * Skip service parameter names that have built-in definitions. This
     * happens with message delivery transports that have a non-default
     * per-destination concurrency or recipient limit, such as local(8).
     * 
     * Some parameters were tentatively flagged as built-in, but they are
     * service parameters with their own default value. We don't change the
     * default but we correct the parameter class.
     */
    if ((node = PC_PARAM_TABLE_FIND(param_table, name)) != 0) {
	PC_PARAM_CLASS_OVERRIDE(node, PC_PARAM_FLAG_SERVICE);
    } else {
	PC_PARAM_TABLE_ENTER(param_table, name, PC_PARAM_FLAG_SERVICE,
			     (char *) defparam, convert_service_parameter);
    }
    myfree(name);
}

/* register_service_parameters - add all service parameters with defaults */

void    register_service_parameters(void)
{
    const char *myname = "register_service_parameters";
    static const PC_STRING_NV pipe_params[] = {
	/* suffix, default parameter name */
	_MAXTIME, VAR_COMMAND_MAXTIME,
#define service_params (pipe_params + 1)
	_XPORT_RCPT_LIMIT, VAR_XPORT_RCPT_LIMIT,
	_STACK_RCPT_LIMIT, VAR_STACK_RCPT_LIMIT,
	_XPORT_REFILL_LIMIT, VAR_XPORT_REFILL_LIMIT,
	_XPORT_REFILL_DELAY, VAR_XPORT_REFILL_DELAY,
	_DELIVERY_SLOT_COST, VAR_DELIVERY_SLOT_COST,
	_DELIVERY_SLOT_LOAN, VAR_DELIVERY_SLOT_LOAN,
	_DELIVERY_SLOT_DISCOUNT, VAR_DELIVERY_SLOT_DISCOUNT,
	_MIN_DELIVERY_SLOTS, VAR_MIN_DELIVERY_SLOTS,
	_INIT_DEST_CON, VAR_INIT_DEST_CON,
	_DEST_CON_LIMIT, VAR_DEST_CON_LIMIT,
	_DEST_RCPT_LIMIT, VAR_DEST_RCPT_LIMIT,
	_CONC_POS_FDBACK, VAR_CONC_POS_FDBACK,
	_CONC_NEG_FDBACK, VAR_CONC_NEG_FDBACK,
	_CONC_COHORT_LIM, VAR_CONC_COHORT_LIM,
	_DEST_RATE_DELAY, VAR_DEST_RATE_DELAY,
	0,
    };
    static const PC_STRING_NV spawn_params[] = {
	/* suffix, default parameter name */
	_MAXTIME, VAR_COMMAND_MAXTIME,
	0,
    };
    typedef struct {
	const char *progname;
	const PC_STRING_NV *params;
    } PC_SERVICE_DEF;
    static const PC_SERVICE_DEF service_defs[] = {
	MAIL_PROGRAM_LOCAL, service_params,
	MAIL_PROGRAM_ERROR, service_params,
	MAIL_PROGRAM_VIRTUAL, service_params,
	MAIL_PROGRAM_SMTP, service_params,
	MAIL_PROGRAM_LMTP, service_params,
	MAIL_PROGRAM_PIPE, pipe_params,
	MAIL_PROGRAM_SPAWN, spawn_params,
	0,
    };
    const PC_STRING_NV *sp;
    const char *progname;
    const char *service;
    PC_MASTER_ENT *masterp;
    ARGV   *argv;
    const PC_SERVICE_DEF *sd;

    /*
     * Sanity checks.
     */
    if (param_table == 0)
	msg_panic("%s: global parameter table is not initialized", myname);
    if (master_table == 0)
	msg_panic("%s: master table is not initialized", myname);

    /*
     * Extract service names from master.cf and generate service parameter
     * information.
     */
    for (masterp = master_table; (argv = masterp->argv) != 0; masterp++) {

	/*
	 * Add service parameters for message delivery transports or spawn
	 * programs.
	 */
	progname = argv->argv[7];
	for (sd = service_defs; sd->progname; sd++) {
	    if (strcmp(sd->progname, progname) == 0) {
		service = argv->argv[0];
		for (sp = sd->params; sp->name; sp++)
		    register_service_parameter(service, sp->name, sp->value);
		break;
	    }
	}
    }
}
