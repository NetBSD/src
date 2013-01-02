/*	$NetBSD: master.h,v 1.1.1.3 2013/01/02 18:59:01 tron Exp $	*/

/*++
/* NAME
/*	master 3h
/* SUMMARY
/*	Postfix master - data structures and prototypes
/* SYNOPSIS
/*	#include "master.h"
/* DESCRIPTION
/* .nf

 /*
  * Server processes that provide the same service share a common "listen"
  * socket to accept connection requests, and share a common pipe to the
  * master process to send status reports. Server processes die voluntarily
  * when idle for a configurable amount of time, or after servicing a
  * configurable number of requests; the master process spawns new processes
  * on demand up to a configurable concurrency limit and/or periodically.
  * 
  * The canonical service name is what we use internally, so that we correctly
  * handle a request to "reload" after someone changes "smtp" into "25".
  * 
  * We use the external service name from master.cf when reporting problems, so
  * that the user can figure out what we are talking about. Of course we also
  * include the canonical service name so that the UNIX-domain smtp service
  * can be distinguished from the Internet smtp service.
  */
typedef struct MASTER_SERV {
    int     flags;			/* status, features, etc. */
    char   *ext_name;			/* service endpoint name (master.cf) */
    char   *name;			/* service endpoint name (canonical) */
    int     type;			/* UNIX-domain, INET, etc. */
    time_t  busy_warn_time;		/* limit "all servers busy" warning */
    int     wakeup_time;		/* wakeup interval */
    int    *listen_fd;			/* incoming requests */
    int     listen_fd_count;		/* nr of descriptors */
    union {
	struct {
	    char   *port;		/* inet listen port */
	    struct INET_ADDR_LIST *addr;/* inet listen address */
	}       inet_ep;
#define MASTER_INET_ADDRLIST(s)	((s)->endpoint.inet_ep.addr)
#define MASTER_INET_PORT(s)	((s)->endpoint.inet_ep.port)
    }       endpoint;
    int     max_proc;			/* upper bound on # processes */
    char   *path;			/* command pathname */
    struct ARGV *args;			/* argument vector */
    char   *stress_param_val;		/* stress value: "yes" or empty */
    time_t  stress_expire_time;		/* stress pulse stretcher */
    int     avail_proc;			/* idle processes */
    int     total_proc;			/* number of processes */
    int     throttle_delay;		/* failure recovery parameter */
    int     status_fd[2];		/* child status reports */
    struct BINHASH *children;		/* linkage */
    struct MASTER_SERV *next;		/* linkage */
} MASTER_SERV;

 /*
  * Per-service flag bits. We assume trouble when a child process terminates
  * before completing its first request: either the program is defective,
  * some configuration is wrong, or the system is out of resources.
  */
#define MASTER_FLAG_THROTTLE	(1<<0)	/* we're having trouble */
#define MASTER_FLAG_MARK	(1<<1)	/* garbage collection support */
#define MASTER_FLAG_CONDWAKE	(1<<2)	/* wake up if actually used */
#define MASTER_FLAG_INETHOST	(1<<3)	/* endpoint name specifies host */
#define MASTER_FLAG_LOCAL_ONLY	(1<<4)	/* no remote clients */
#define MASTER_FLAG_LISTEN	(1<<5)	/* monitor this port */

#define MASTER_THROTTLED(f)	((f)->flags & MASTER_FLAG_THROTTLE)
#define MASTER_MARKED_FOR_DELETION(f) ((f)->flags & MASTER_FLAG_MARK)
#define MASTER_LISTENING(f)	((f)->flags & MASTER_FLAG_LISTEN)

#define MASTER_LIMIT_OK(limit, count) ((limit) == 0 || ((count) < (limit)))

 /*
  * Service types.
  */
#define MASTER_SERV_TYPE_UNIX	1	/* AF_UNIX domain socket */
#define MASTER_SERV_TYPE_INET	2	/* AF_INET domain socket */
#define MASTER_SERV_TYPE_FIFO	3	/* fifo (named pipe) */
#define MASTER_SERV_TYPE_PASS	4	/* AF_UNIX domain socket */

 /*
  * Default process management policy values. This is only the bare minimum.
  * Most policy management is delegated to child processes. The process
  * manager runs at high privilege level and has to be kept simple.
  */
#define MASTER_DEF_MIN_IDLE	1	/* preferred # of idle processes */

 /*
  * Structure of child process.
  */
typedef int MASTER_PID;			/* pid is key into binhash table */

typedef struct MASTER_PROC {
    MASTER_PID pid;			/* child process id */
    unsigned gen;			/* child generation number */
    int     avail;			/* availability */
    MASTER_SERV *serv;			/* parent linkage */
    int     use_count;			/* number of service requests */
} MASTER_PROC;

 /*
  * Other manifest constants.
  */
#define MASTER_BUF_LEN	2048		/* logical config line length */

 /*
  * master.c
  */
extern int master_detach;

 /*
  * master_ent.c
  */
extern void fset_master_ent(char *);
extern void set_master_ent(void);
extern void end_master_ent(void);
extern void print_master_ent(MASTER_SERV *);
extern MASTER_SERV *get_master_ent(void);
extern void free_master_ent(MASTER_SERV *);

 /*
  * master_conf.c
  */
extern void master_config(void);
extern void master_refresh(void);

 /*
  * master_vars.c
  */
extern void master_vars_init(void);

 /*
  * master_service.c
  */
extern MASTER_SERV *master_head;
extern void master_start_service(MASTER_SERV *);
extern void master_stop_service(MASTER_SERV *);
extern void master_restart_service(MASTER_SERV *, int);

#define DO_CONF_RELOAD	1	/* config files were reloaded */
#define NO_CONF_RELOAD	0	/* no config file was reloaded */

 /*
  * master_events.c
  */
extern int master_gotsighup;
extern int master_gotsigchld;
extern void master_sigsetup(void);

 /*
  * master_status.c
  */
extern void master_status_init(MASTER_SERV *);
extern void master_status_cleanup(MASTER_SERV *);

 /*
  * master_wakeup.c
  */
extern void master_wakeup_init(MASTER_SERV *);
extern void master_wakeup_cleanup(MASTER_SERV *);


 /*
  * master_listen.c
  */
extern void master_listen_init(MASTER_SERV *);
extern void master_listen_cleanup(MASTER_SERV *);

 /*
  * master_avail.c
  */
extern void master_avail_listen(MASTER_SERV *);
extern void master_avail_cleanup(MASTER_SERV *);
extern void master_avail_more(MASTER_SERV *, MASTER_PROC *);
extern void master_avail_less(MASTER_SERV *, MASTER_PROC *);

 /*
  * master_spawn.c
  */
extern struct BINHASH *master_child_table;
extern void master_spawn(MASTER_SERV *);
extern void master_reap_child(void);
extern void master_delete_children(MASTER_SERV *);

 /*
  * master_flow.c
  */
extern void master_flow_init(void);
extern int master_flow_pipe[2];

 /*
  * master_watch.c
  * 
  * Support to warn about main.cf parameters that can only be initialized but
  * not updated, and to initialize or update data structures that derive
  * values from main.cf parameters.
  */
typedef struct {
    const char *name;			/* parameter name */
    char  **value;			/* current main.cf value */
    char  **backup;			/* actual value that is being used */
    int     flags;			/* see below */
    void    (*notify) (void);		/* init or update data structure */
} MASTER_STR_WATCH;

typedef struct {
    const char *name;			/* parameter name */
    int    *value;			/* current main.cf value */
    int     backup;			/* actual value that is being used */
    int     flags;			/* see below */
    void    (*notify) (void);		/* init or update data structure */
} MASTER_INT_WATCH;

#define MASTER_WATCH_FLAG_UPDATABLE (1<<0)	/* support update after init */
#define MASTER_WATCH_FLAG_ISSET    (1<<1)	/* backup is initialized */

extern void master_str_watch(const MASTER_STR_WATCH *);
extern void master_int_watch(MASTER_INT_WATCH *);

/* DIAGNOSTICS
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
