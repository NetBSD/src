/*++
/* NAME
/*	master_service 3
/* SUMMARY
/*	Postfix master - start/stop services
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_start_service(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_stop_service(serv)
/*	MASTER_SERV *serv;
/*
/*	void	master_restart_service(serv)
/*	MASTER_SERV *serv;
/* DESCRIPTION
/*	master_start_service() enables the named service.
/*
/*	master_stop_service() disables named service.
/*
/*	master_restart_service() requests all running child processes to
/*	commit suicide. This is typically used after a configuration reload.
/* DIAGNOSTICS
/* BUGS
/* SEE ALSO
/*	master_avail(3), process creation policy
/*	master_wakeup(3), service automatic wakeup
/*	master_status(3), child status reports
/*	master_listen(3), unix/inet listeners
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
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Application-specific. */

#include "master.h"

MASTER_SERV *master_head;

/* master_start_service - activate service */

void    master_start_service(MASTER_SERV *serv)
{

    /*
     * Enable connection requests, wakeup timers, and status updates from
     * child processes.
     */
    master_listen_init(serv);
    master_avail_listen(serv);
    master_status_init(serv);
    master_wakeup_init(serv);
}

/* master_stop_service - deactivate service */

void    master_stop_service(MASTER_SERV *serv)
{

    /*
     * Undo the things that master_start_service() did.
     */
    master_wakeup_cleanup(serv);
    master_status_cleanup(serv);
    master_avail_cleanup(serv);
    master_listen_cleanup(serv);
}

/* master_restart_service - restart service after configuration reload */

void    master_restart_service(MASTER_SERV *serv)
{

    /*
     * Undo some of the things that master_start_service() did.
     */
    master_wakeup_cleanup(serv);
    master_status_cleanup(serv);

    /*
     * Now undo the undone.
     */
    master_status_init(serv);
    master_wakeup_init(serv);
}
