/*	$NetBSD: master_conf.c,v 1.1.1.2 2013/01/02 18:59:01 tron Exp $	*/

/*++
/* NAME
/*	master_conf 3
/* SUMMARY
/*	Postfix master - master.cf file processing
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_config(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_refresh(serv)
/*	MASTER_SERV *serv;
/* DESCRIPTION
/*	Use master_config() to read the master.cf configuration file
/*	during program initialization.
/*
/*	Use master_refresh() to re-read the master.cf configuration file
/*	when the process is already running.
/* DIAGNOSTICS
/* BUGS
/* SEE ALSO
/*	master_ent(3), configuration file programmatic interface.
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

/* System libraries. */

#include <sys_defs.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <argv.h>

/* Application-specific. */

#include "master.h"

/* master_refresh - re-read configuration table */

void    master_refresh(void)
{
    MASTER_SERV *serv;
    MASTER_SERV **servp;

    /*
     * Mark all existing services.
     */
    for (serv = master_head; serv != 0; serv = serv->next)
	serv->flags |= MASTER_FLAG_MARK;

    /*
     * Read the master.cf configuration file. The master_conf() routine
     * unmarks services upon update. New services are born with the mark bit
     * off. After this, anything with the mark bit on should be removed.
     */
    master_config();

    /*
     * Delete all services that are still marked - they disappeared from the
     * configuration file and are therefore no longer needed.
     */
    for (servp = &master_head; (serv = *servp) != 0; /* void */ ) {
	if ((serv->flags & MASTER_FLAG_MARK) != 0) {
	    *servp = serv->next;
	    master_stop_service(serv);
	    free_master_ent(serv);
	} else {
	    servp = &serv->next;
	}
    }
}

/* master_config - read config file */

void    master_config(void)
{
    MASTER_SERV *entry;
    MASTER_SERV *serv;

#define STR_DIFF	strcmp
#define STR_SAME	!strcmp
#define SWAP(type,a,b)	{ type temp = a; a = b; b = temp; }

    /*
     * A service is identified by its endpoint name AND by its transport
     * type, not just by its name alone. The name is unique within its
     * transport type. XXX Service privacy is encoded in the service name.
     */
    set_master_ent();
    while ((entry = get_master_ent()) != 0) {
	if (msg_verbose)
	    print_master_ent(entry);
	for (serv = master_head; serv != 0; serv = serv->next)
	    if (STR_SAME(serv->name, entry->name) && serv->type == entry->type)
		break;

	/*
	 * Add a new service entry. We do not really care in what order the
	 * service entries are kept in memory.
	 */
	if (serv == 0) {
	    entry->next = master_head;
	    master_head = entry;
	    master_start_service(entry);
	}

	/*
	 * Update an existing service entry. Make the current generation of
	 * child processes commit suicide whenever it is convenient. The next
	 * generation of child processes will run with the new configuration
	 * settings.
	 */
	else {
	    serv->flags &= ~MASTER_FLAG_MARK;
	    if (entry->flags & MASTER_FLAG_CONDWAKE)
		serv->flags |= MASTER_FLAG_CONDWAKE;
	    else
		serv->flags &= ~MASTER_FLAG_CONDWAKE;
	    serv->wakeup_time = entry->wakeup_time;
	    serv->max_proc = entry->max_proc;
	    serv->throttle_delay = entry->throttle_delay;
	    SWAP(char *, serv->ext_name, entry->ext_name);
	    SWAP(char *, serv->path, entry->path);
	    SWAP(ARGV *, serv->args, entry->args);
	    SWAP(char *, serv->stress_param_val, entry->stress_param_val);
	    master_restart_service(serv, DO_CONF_RELOAD);
	    free_master_ent(entry);
	}
    }
    end_master_ent();
}
