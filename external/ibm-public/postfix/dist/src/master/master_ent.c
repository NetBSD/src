/*	$NetBSD: master_ent.c,v 1.1.1.3.8.1 2013/02/25 00:27:20 tls Exp $	*/

/*++
/* NAME
/*	master_ent 3
/* SUMMARY
/*	Postfix master - config file access
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	fset_master_ent(path)
/*	char	*path;
/*
/*	void	set_master_ent()
/*
/*	MASTER_SERV *get_master_ent()
/*
/*	void	end_master_ent()
/*
/*	void	print_master_ent(entry)
/*	MASTER_SERV *entry;
/*
/*	void	free_master_ent(entry)
/*	MASTER_SERV *entry;
/* DESCRIPTION
/*	This module implements a simple programmatic interface
/*	for accessing Postfix master process configuration files.
/*
/*	fset_master_ent() specifies the location of the master process
/*	configuration file.  The pathname is copied.
/*
/*	set_master_ent() opens the configuration file. It is an error
/*	to call this routine while the configuration file is still open.
/*	It is an error to open a configuration file without specifying
/*	its name to fset_master_ent().
/*
/*	get_master_ent() reads the next entry from an open configuration
/*	file and returns the parsed result. A null result means the end
/*	of file was reached.
/*
/*	print_master_ent() prints the specified service entry.
/*
/*	end_master_ent() closes an open configuration file. It is an error
/*	to call this routine when the configuration file is not open.
/*
/*	free_master_ent() destroys the memory used for a parsed configuration
/*	file entry.
/* DIAGNOSTICS
/*	Panics: interface violations. Fatal errors: memory allocation
/*	failure.
/* BUGS
/* SEE ALSO
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
#include <netinet/in.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility libraries. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <stringops.h>
#include <readlline.h>
#include <inet_addr_list.h>
#include <host_port.h>
#include <inet_addr_host.h>
#include <sock_addr.h>
#include <inet_proto.h>

/* Global library. */

#include <match_service.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <own_inet_addr.h>
#include <wildcard_inet_addr.h>
#include <mail_conf.h>

/* Local stuff. */

#include "master_proto.h"
#include "master.h"

static char *master_path;		/* config file name */
static VSTREAM *master_fp;		/* config file pointer */
static int master_line;			/* config file line number */
static ARGV *master_disable;		/* disabled service patterns */

static char master_blanks[] = " \t\r\n";/* field delimiters */

static NORETURN fatal_invalid_field(char *, char *);
static NORETURN fatal_with_context(char *,...);

/* fset_master_ent - specify configuration file pathname */

void    fset_master_ent(char *path)
{
    if (master_path != 0)
	myfree(master_path);
    master_path = mystrdup(path);
}

/* set_master_ent - open configuration file */

void    set_master_ent()
{
    const char *myname = "set_master_ent";
    char   *disable;

    if (master_fp != 0)
	msg_panic("%s: configuration file still open", myname);
    if (master_path == 0)
	msg_panic("%s: no configuration file specified", myname);
    if ((master_fp = vstream_fopen(master_path, O_RDONLY, 0)) == 0)
	msg_fatal("open %s: %m", master_path);
    master_line = 0;
    if (master_disable != 0)
	msg_panic("%s: service disable list still exists", myname);
    if (inet_proto_info()->ai_family_list[0] == 0) {
	msg_warn("all network protocols are disabled (%s = %s)",
		 VAR_INET_PROTOCOLS, var_inet_protocols);
	msg_warn("disabling all type \"inet\" services in master.cf");
	disable = concatenate(MASTER_XPORT_NAME_INET, ",",
			      var_master_disable, (char *) 0);
	master_disable = match_service_init(disable);
	myfree(disable);
    } else
	master_disable = match_service_init(var_master_disable);
}

/* end_master_ent - close configuration file */

void    end_master_ent()
{
    const char *myname = "end_master_ent";

    if (master_fp == 0)
	msg_panic("%s: configuration file not open", myname);
    if (vstream_fclose(master_fp) != 0)
	msg_fatal("%s: close configuration file: %m", myname);
    master_fp = 0;
    if (master_disable == 0)
	msg_panic("%s: no service disable list", myname);
    match_service_free(master_disable);
    master_disable = 0;
}

/* fatal_with_context - print fatal error with file/line context */

static NORETURN fatal_with_context(char *format,...)
{
    const char *myname = "fatal_with_context";
    VSTRING *vp = vstring_alloc(100);
    va_list ap;

    if (master_path == 0)
	msg_panic("%s: no configuration file specified", myname);

    va_start(ap, format);
    vstring_vsprintf(vp, format, ap);
    va_end(ap);
    msg_fatal("%s: line %d: %s", master_path, master_line, vstring_str(vp));
}

/* fatal_invalid_field - report invalid field value */

static NORETURN fatal_invalid_field(char *name, char *value)
{
    fatal_with_context("field \"%s\": bad value: \"%s\"", name, value);
}

/* get_str_ent - extract string field */

static char *get_str_ent(char **bufp, char *name, char *def_val)
{
    char   *value;

    if ((value = mystrtok(bufp, master_blanks)) == 0)
	fatal_with_context("missing \"%s\" field", name);
    if (strcmp(value, "-") == 0) {
	if (def_val == 0)
	    fatal_with_context("field \"%s\" has no default value", name);
	return (def_val);
    } else {
	return (value);
    }
}

/* get_bool_ent - extract boolean field */

static int get_bool_ent(char **bufp, char *name, char *def_val)
{
    char   *value;

    value = get_str_ent(bufp, name, def_val);
    if (strcmp("y", value) == 0) {
	return (1);
    } else if (strcmp("n", value) == 0) {
	return (0);
    } else {
	fatal_invalid_field(name, value);
    }
    /* NOTREACHED */
}

/* get_int_ent - extract integer field */

static int get_int_ent(char **bufp, char *name, char *def_val, int min_val)
{
    char   *value;
    int     n;

    value = get_str_ent(bufp, name, def_val);
    if (!ISDIGIT(*value) || (n = atoi(value)) < min_val)
	fatal_invalid_field(name, value);
    return (n);
}

/* get_master_ent - read entry from configuration file */

MASTER_SERV *get_master_ent()
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *junk = vstring_alloc(100);
    MASTER_SERV *serv;
    char   *cp;
    char   *name;
    char   *host = 0;
    char   *port = 0;
    char   *transport;
    int     private;
    int     unprivileged;		/* passed on to child */
    int     chroot;			/* passed on to child */
    char   *command;
    int     n;
    char   *bufp;
    char   *atmp;
    const char *parse_err;
    static char *saved_interfaces = 0;

    if (master_fp == 0)
	msg_panic("get_master_ent: config file not open");
    if (master_disable == 0)
	msg_panic("get_master_ent: no service disable list");

    /*
     * XXX We cannot change the inet_interfaces setting for a running master
     * process. Listening sockets are inherited by child processes so that
     * closing and reopening those sockets in the master does not work.
     * 
     * Another problem is that library routines still cache results that are
     * based on the old inet_interfaces setting. It is too much trouble to
     * recompute everything.
     * 
     * In order to keep our data structures consistent we ignore changes in
     * inet_interfaces settings, and issue a warning instead.
     */
    if (saved_interfaces == 0)
	saved_interfaces = mystrdup(var_inet_interfaces);

    /*
     * Skip blank lines and comment lines.
     */
    for (;;) {
	if (readlline(buf, master_fp, &master_line) == 0) {
	    vstring_free(buf);
	    vstring_free(junk);
	    return (0);
	}
	bufp = vstring_str(buf);
	if ((cp = mystrtok(&bufp, master_blanks)) == 0)
	    continue;
	name = cp;
	transport = get_str_ent(&bufp, "transport type", (char *) 0);
	vstring_sprintf(junk, "%s.%s", name, transport);
	if (match_service_match(master_disable, vstring_str(junk)) == 0)
	    break;
    }

    /*
     * Parse one logical line from the configuration file. Initialize service
     * structure members in order.
     */
    serv = (MASTER_SERV *) mymalloc(sizeof(MASTER_SERV));
    serv->next = 0;

    /*
     * Flags member.
     */
    serv->flags = 0;

    /*
     * All servers busy warning timer.
     */
    serv->busy_warn_time = 0;

    /*
     * Service name. Syntax is transport-specific.
     */
    serv->ext_name = mystrdup(name);

    /*
     * Transport type: inet (wild-card listen or virtual) or unix.
     */
#define STR_SAME	!strcmp

    if (STR_SAME(transport, MASTER_XPORT_NAME_INET)) {
	if (!STR_SAME(saved_interfaces, var_inet_interfaces)) {
	    msg_warn("service %s: ignoring %s change",
		     serv->ext_name, VAR_INET_INTERFACES);
	    msg_warn("to change %s, stop and start Postfix",
		     VAR_INET_INTERFACES);
	}
	serv->type = MASTER_SERV_TYPE_INET;
	atmp = mystrdup(name);
	if ((parse_err = host_port(atmp, &host, "", &port, (char *) 0)) != 0)
	    msg_fatal("%s: line %d: %s in \"%s\"",
		      VSTREAM_PATH(master_fp), master_line,
		      parse_err, name);
	if (*host) {
	    serv->flags |= MASTER_FLAG_INETHOST;/* host:port */
	    MASTER_INET_ADDRLIST(serv) = (INET_ADDR_LIST *)
		mymalloc(sizeof(*MASTER_INET_ADDRLIST(serv)));
	    inet_addr_list_init(MASTER_INET_ADDRLIST(serv));
	    if (inet_addr_host(MASTER_INET_ADDRLIST(serv), host) == 0)
		msg_fatal("%s: line %d: bad hostname or network address: %s",
			  VSTREAM_PATH(master_fp), master_line, name);
	    inet_addr_list_uniq(MASTER_INET_ADDRLIST(serv));
	    serv->listen_fd_count = MASTER_INET_ADDRLIST(serv)->used;
	} else {
	    MASTER_INET_ADDRLIST(serv) =
		strcasecmp(saved_interfaces, INET_INTERFACES_ALL) ?
		own_inet_addr_list() :		/* virtual */
		wildcard_inet_addr_list();	/* wild-card */
	    inet_addr_list_uniq(MASTER_INET_ADDRLIST(serv));
	    serv->listen_fd_count = MASTER_INET_ADDRLIST(serv)->used;
	}
	MASTER_INET_PORT(serv) = mystrdup(port);
	for (n = 0; /* see below */ ; n++) {
	    if (n >= MASTER_INET_ADDRLIST(serv)->used) {
		serv->flags |= MASTER_FLAG_LOCAL_ONLY;
		break;
	    }
	    if (!sock_addr_in_loopback(SOCK_ADDR_PTR(MASTER_INET_ADDRLIST(serv)->addrs + n)))
		break;
	}
    } else if (STR_SAME(transport, MASTER_XPORT_NAME_UNIX)) {
	serv->type = MASTER_SERV_TYPE_UNIX;
	serv->listen_fd_count = 1;
	serv->flags |= MASTER_FLAG_LOCAL_ONLY;
    } else if (STR_SAME(transport, MASTER_XPORT_NAME_FIFO)) {
	serv->type = MASTER_SERV_TYPE_FIFO;
	serv->listen_fd_count = 1;
	serv->flags |= MASTER_FLAG_LOCAL_ONLY;
#ifdef MASTER_SERV_TYPE_PASS
    } else if (STR_SAME(transport, MASTER_XPORT_NAME_PASS)) {
	serv->type = MASTER_SERV_TYPE_PASS;
	serv->listen_fd_count = 1;
	/* If this is a connection screener, remote clients are likely. */
#endif
    } else {
	fatal_with_context("bad transport type: %s", transport);
    }

    /*
     * Service class: public or private.
     */
    private = get_bool_ent(&bufp, "private", "y");

    /*
     * Derive an internal service name. The name may depend on service
     * attributes such as privacy.
     */
    if (serv->type == MASTER_SERV_TYPE_INET) {
	MAI_HOSTADDR_STR host_addr;
	MAI_SERVPORT_STR serv_port;
	struct addrinfo *res0;

	if (private)
	    fatal_with_context("inet service cannot be private");

	/*
	 * Canonicalize endpoint names so that we correctly handle "reload"
	 * requests after someone changes "25" into "smtp" or vice versa.
	 */
	if (*host == 0)
	    host = 0;
	/* Canonicalize numeric host and numeric or symbolic service. */
	if (hostaddr_to_sockaddr(host, port, 0, &res0) == 0) {
	    SOCKADDR_TO_HOSTADDR(res0->ai_addr, res0->ai_addrlen,
				 host ? &host_addr : (MAI_HOSTADDR_STR *) 0,
				 &serv_port, 0);
	    serv->name = (host ? concatenate("[", host_addr.buf, "]:",
					     serv_port.buf, (char *) 0) :
			  mystrdup(serv_port.buf));
	    freeaddrinfo(res0);
	}
	/* Canonicalize numeric or symbolic service. */
	else if (hostaddr_to_sockaddr((char *) 0, port, 0, &res0) == 0) {
	    SOCKADDR_TO_HOSTADDR(res0->ai_addr, res0->ai_addrlen,
				 (MAI_HOSTADDR_STR *) 0, &serv_port, 0);
	    serv->name = (host ? concatenate("[", host, "]:",
					     serv_port.buf, (char *) 0) :
			  mystrdup(serv_port.buf));
	    freeaddrinfo(res0);
	}
	/* Bad service name? */
	else
	    serv->name = mystrdup(name);
	myfree(atmp);
    } else if (serv->type == MASTER_SERV_TYPE_UNIX) {
	serv->name = mail_pathname(private ? MAIL_CLASS_PRIVATE :
				   MAIL_CLASS_PUBLIC, name);
    } else if (serv->type == MASTER_SERV_TYPE_FIFO) {
	serv->name = mail_pathname(private ? MAIL_CLASS_PRIVATE :
				   MAIL_CLASS_PUBLIC, name);
#ifdef MASTER_SERV_TYPE_PASS
    } else if (serv->type == MASTER_SERV_TYPE_PASS) {
	serv->name = mail_pathname(private ? MAIL_CLASS_PRIVATE :
				   MAIL_CLASS_PUBLIC, name);
#endif
    } else {
	msg_panic("bad transport type: %d", serv->type);
    }

    /*
     * Listen socket(s). XXX We pre-allocate storage because the number of
     * sockets is frozen anyway once we build the command-line vector below.
     */
    if (serv->listen_fd_count == 0) {
	msg_fatal("%s: line %d: no valid IP address found: %s",
		  VSTREAM_PATH(master_fp), master_line, name);
    }
    serv->listen_fd = (int *) mymalloc(sizeof(int) * serv->listen_fd_count);
    for (n = 0; n < serv->listen_fd_count; n++)
	serv->listen_fd[n] = -1;

    /*
     * Privilege level. Default is to restrict process privileges to those of
     * the mail owner.
     */
    unprivileged = get_bool_ent(&bufp, "unprivileged", "y");

    /*
     * Chroot. Default is to restrict file system access to the mail queue.
     * XXX Chroot cannot imply unprivileged service (for example, the pickup
     * service runs chrooted but needs privileges to open files as the user).
     */
    chroot = get_bool_ent(&bufp, "chroot", "y");

    /*
     * Wakeup timer. XXX should we require that var_proc_limit == 1? Right
     * now, the only services that have a wakeup timer also happen to be the
     * services that have at most one running instance: local pickup and
     * local delivery.
     */
    serv->wakeup_time = get_int_ent(&bufp, "wakeup_time", "0", 0);

    /*
     * Find out if the wakeup time is conditional, i.e., wakeup triggers
     * should not be sent until the service has actually been used.
     */
    if (serv->wakeup_time > 0 && bufp[*bufp ? -2 : -1] == '?')
	serv->flags |= MASTER_FLAG_CONDWAKE;

    /*
     * Concurrency limit. Zero means no limit.
     */
    vstring_sprintf(junk, "%d", var_proc_limit);
    serv->max_proc = get_int_ent(&bufp, "max_proc", vstring_str(junk), 0);

    /*
     * Path to command,
     */
    command = get_str_ent(&bufp, "command", (char *) 0);
    serv->path = concatenate(var_daemon_dir, "/", command, (char *) 0);

    /*
     * Idle and total process count.
     */
    serv->avail_proc = 0;
    serv->total_proc = 0;

    /*
     * Backoff time in case a service is broken.
     */
    serv->throttle_delay = var_throttle_time;

    /*
     * Shared channel for child status updates.
     */
    serv->status_fd[0] = serv->status_fd[1] = -1;

    /*
     * Child process structures.
     */
    serv->children = 0;

    /*
     * Command-line vector. Add "-n service_name" when the process name
     * basename differs from the service name. Always add the transport.
     */
    serv->args = argv_alloc(0);
    argv_add(serv->args, command, (char *) 0);
    if (serv->max_proc == 1)
	argv_add(serv->args, "-l", (char *) 0);
    if (serv->max_proc == 0)
	argv_add(serv->args, "-z", (char *) 0);
    if (strcmp(basename(command), name) != 0)
	argv_add(serv->args, "-n", name, (char *) 0);
    argv_add(serv->args, "-t", transport, (char *) 0);
    if (master_detach == 0)
	argv_add(serv->args, "-d", (char *) 0);
    if (msg_verbose)
	argv_add(serv->args, "-v", (char *) 0);
    if (unprivileged)
	argv_add(serv->args, "-u", (char *) 0);
    if (chroot)
	argv_add(serv->args, "-c", (char *) 0);
    if ((serv->flags & MASTER_FLAG_LOCAL_ONLY) == 0 && serv->max_proc > 1) {
	argv_add(serv->args, "-o", "stress=" CONFIG_BOOL_YES, (char *) 0);
	serv->stress_param_val =
	    serv->args->argv[serv->args->argc - 1] + sizeof("stress=") - 1;
	serv->stress_param_val[0] = 0;
    } else
	serv->stress_param_val = 0;
    serv->stress_expire_time = 0;
    if (serv->listen_fd_count > 1)
	argv_add(serv->args, "-s",
	    vstring_str(vstring_sprintf(junk, "%d", serv->listen_fd_count)),
		 (char *) 0);
    while ((cp = mystrtok(&bufp, master_blanks)) != 0)
	argv_add(serv->args, cp, (char *) 0);
    argv_terminate(serv->args);

    /*
     * Cleanup.
     */
    vstring_free(buf);
    vstring_free(junk);
    return (serv);
}

/* print_master_ent - show service entry contents */

void    print_master_ent(MASTER_SERV *serv)
{
    char  **cpp;

    msg_info("====start service entry");
    msg_info("flags: %d", serv->flags);
    msg_info("name: %s", serv->name);
    msg_info("type: %s",
	     serv->type == MASTER_SERV_TYPE_UNIX ? MASTER_XPORT_NAME_UNIX :
	     serv->type == MASTER_SERV_TYPE_FIFO ? MASTER_XPORT_NAME_FIFO :
	     serv->type == MASTER_SERV_TYPE_INET ? MASTER_XPORT_NAME_INET :
#ifdef MASTER_SERV_TYPE_PASS
	     serv->type == MASTER_SERV_TYPE_PASS ? MASTER_XPORT_NAME_PASS :
#endif
	     "unknown transport type");
    msg_info("listen_fd_count: %d", serv->listen_fd_count);
    msg_info("wakeup: %d", serv->wakeup_time);
    msg_info("max_proc: %d", serv->max_proc);
    msg_info("path: %s", serv->path);
    for (cpp = serv->args->argv; *cpp; cpp++)
	msg_info("arg[%d]: %s", (int) (cpp - serv->args->argv), *cpp);
    msg_info("avail_proc: %d", serv->avail_proc);
    msg_info("total_proc: %d", serv->total_proc);
    msg_info("throttle_delay: %d", serv->throttle_delay);
    msg_info("status_fd %d %d", serv->status_fd[0], serv->status_fd[1]);
    msg_info("children: 0x%lx", (long) serv->children);
    msg_info("next: 0x%lx", (long) serv->next);
    msg_info("====end service entry");
}

/* free_master_ent - destroy process entry */

void    free_master_ent(MASTER_SERV *serv)
{

    /*
     * Undo what get_master_ent() created.
     */
    if (serv->flags & MASTER_FLAG_INETHOST) {
	inet_addr_list_free(MASTER_INET_ADDRLIST(serv));
	myfree((char *) MASTER_INET_ADDRLIST(serv));
    }
    if (serv->type == MASTER_SERV_TYPE_INET)
	myfree(MASTER_INET_PORT(serv));
    myfree(serv->ext_name);
    myfree(serv->name);
    myfree(serv->path);
    argv_free(serv->args);
    myfree((char *) serv->listen_fd);
    myfree((char *) serv);
}
