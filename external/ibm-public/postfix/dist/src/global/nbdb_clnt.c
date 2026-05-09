/*	$NetBSD: nbdb_clnt.c,v 1.2 2026/05/09 18:49:16 christos Exp $	*/

/*++
/* NAME
/*	nbdb_clnt 3
/* SUMMARY
/*	Non-Berkeley-DB migration client
/* SYNOPSIS
/*	#include <nbdb_clnt.h>
/*
/*	void	nbdb_clnt_request(
/*	const char *type,
r*	const char *source_path,
/*	int maxtry,
/*	int timeout,
/*	int delay,
/*	VSTRING *why)
/* DESCRIPTION
/*	nbdb_clnt_request() asks the reindexing server to index a source
/*	file using a non-legacy database type. The result is one of
/*	the following:
/* .IP NBDB_STAT_OK
/*	The request was successful.
/* .IP NBDB_STAT_ERROR
/*	The request failed. A description of the problem is returned
/*	in \fBwhy\fR.
/*
/*	Arguments:
/* .IP type
/*	A legacy database type. The reindexing server will map it
/*	to a non-legacy type.
/* .IP path
/*	The pathname of the database source file (no '.db' suffix).
/* .IP maxtry
/*	The number of attempts to make before giving up.
/* .IP timeout
/*	The time limit (seconds) to connect, write, or read.
/*	Specify enough time to index a large table.
/* .IP delay
/*	The amount of time (seconds) between retries.
/* .IP Implicit inputs:
/*	var_nbdb_service, nbdb_reindex service name.
/* .IP why
/*	Storage that is updated with an applicable error description.
/* SEE ALSO
/*	nbdb_reindexd(8) reindexing server.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <attr.h>
#include <connect.h>
#include <msg.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_proto.h>
#include <nbdb_clnt.h>

#define STR(x)	vstring_str(x)

/* try_one - single request attempt */

static int try_one(const char *service_path, const char *type,
	             const char *path, int timeout, int delay, VSTRING *why)
{
    int     fd;
    VSTREAM *reindexd_stream = 0;
    int     status;

#define NBDB_MIGR_CLNT_RETURN(x) do { \
	if (reindexd_stream) \
	    vstream_fclose(reindexd_stream); \
	return (x); \
    } while (0)

    if ((fd = LOCAL_CONNECT(service_path, BLOCKING, timeout)) < 0) {
	vstring_sprintf(why, "connection to %s service: %m", service_path);
	NBDB_MIGR_CLNT_RETURN(NBDB_STAT_ERROR);
    }
    reindexd_stream = vstream_fdopen(fd, O_RDWR);
    vstream_control(reindexd_stream,
		    CA_VSTREAM_CTL_TIMEOUT(timeout),
		    CA_VSTREAM_CTL_START_DEADLINE,
		    CA_VSTREAM_CTL_END);
    /* Enforce the server protocol type. */
    if (attr_scan(reindexd_stream, ATTR_FLAG_STRICT,
		  RECV_ATTR_STREQ(MAIL_ATTR_PROTO,
				  MAIL_ATTR_PROTO_NBDB_REINDEX),
		  ATTR_TYPE_END) != 0) {
	vstring_sprintf(why, "error receiving %s service initial response",
			service_path);
	NBDB_MIGR_CLNT_RETURN(NBDB_STAT_ERROR);
    }
    /* Send the reindexing request. */
    attr_print(reindexd_stream, ATTR_FLAG_NONE,
	       SEND_ATTR_STR(MAIL_ATTR_NBDB_TYPE, type),
	       SEND_ATTR_STR(MAIL_ATTR_NBDB_PATH, path),
	       ATTR_TYPE_END);
    /* Do not flush the stream yet (would clobber errno). */
    if (vstream_ferror(reindexd_stream) != 0) {
	vstring_sprintf(why, "error sending request to %s service: %m",
			service_path);
	NBDB_MIGR_CLNT_RETURN(NBDB_STAT_ERROR);
    }
    /* Receive the reindexing status. */
    if (attr_scan(reindexd_stream, ATTR_FLAG_STRICT,
		  RECV_ATTR_INT(MAIL_ATTR_STATUS, &status),
		  RECV_ATTR_STR(MAIL_ATTR_WHY, why),
		  ATTR_TYPE_END) != 2) {
	vstring_sprintf(why, "error receiving response from %s service: %m",
			service_path);
	NBDB_MIGR_CLNT_RETURN(NBDB_STAT_ERROR);
    }
    NBDB_MIGR_CLNT_RETURN(status);
}

/* nbdb_clnt_request - send a request and don't hang forever */

int     nbdb_clnt_request(const char *type, const char *path,
			          int max_try, int timeout, int delay,
			          VSTRING *why)
{
    VSTRING *service_path = vstring_alloc(100);
    int     status;
    int     count;

    /*
     * A custom client endpoint that tries only a few times and that may be
     * called by:
     * 
     * - A daemon process. This should keep running if the reindexing service is
     * unavailable, and needs to specify a relative service pathname in case
     * it is chrooted.
     * 
     * - A command-line process. This almost always needs to specify an absolute
     * pathname. This usage is limited to the super-user.
     */
    vstring_sprintf(service_path, "%s/%s", MAIL_CLASS_PRIVATE,
		    var_nbdb_service);
    if (access(STR(service_path), F_OK) != 0)
	vstring_sprintf(service_path, "%s/%s/%s", var_queue_dir,
			MAIL_CLASS_PRIVATE, var_nbdb_service);
    for (count = 0; count < max_try; count++) {
	status = try_one(STR(service_path), type, path, timeout, delay, why);
	if (status == NBDB_STAT_OK)
	    break;
	if (count < max_try - 1)
	    sleep(delay);
    }
    vstring_free(service_path);
    return (status);
}
