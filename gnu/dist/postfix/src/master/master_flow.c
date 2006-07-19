/*	$NetBSD: master_flow.c,v 1.1.1.3 2006/07/19 01:17:31 rpaulo Exp $	*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* Application-specific. */

#include <master.h>
#include <master_proto.h>

int     master_flow_pipe[2];

/* master_flow_init - initialize the flow control channel */

void    master_flow_init(void)
{
    const char *myname = "master_flow_init";

    if (pipe(master_flow_pipe) < 0)
	msg_fatal("%s: pipe: %m", myname);

    non_blocking(master_flow_pipe[0], NON_BLOCKING);
    non_blocking(master_flow_pipe[1], NON_BLOCKING);

    close_on_exec(master_flow_pipe[0], CLOSE_ON_EXEC);
    close_on_exec(master_flow_pipe[1], CLOSE_ON_EXEC);
}
