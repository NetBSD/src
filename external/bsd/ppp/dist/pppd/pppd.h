/*	$NetBSD: pppd.h,v 1.7 2025/01/08 19:59:39 christos Exp $	*/

/*
 * pppd.h - PPP daemon global declarations.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PPP_PPPD_H
#define PPP_PPPD_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "pppdconf.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Limits
 */
#define NUM_PPP		1	/* One PPP interface supported (per process) */
#define MAXWORDLEN	1024	/* max length of word in file (incl null) */
#define MAXARGS		1	/* max # args to a command */
#define MAXNAMELEN	256	/* max length of hostname or name for auth */
#define MAXSECRETLEN	256	/* max length of password or secret */


/*
 * Values for phase.
 */
typedef enum ppp_phase
{
    PHASE_DEAD,
    PHASE_INITIALIZE,
    PHASE_SERIALCONN,
    PHASE_DORMANT,
    PHASE_ESTABLISH,
    PHASE_AUTHENTICATE,
    PHASE_CALLBACK,
    PHASE_NETWORK,
    PHASE_RUNNING,
    PHASE_TERMINATE,
    PHASE_DISCONNECT,
    PHASE_HOLDOFF,
    PHASE_MASTER,
} ppp_phase_t;

/*
 * Values for exit codes
 */
typedef enum ppp_exit_code
{
    EXIT_OK                 = 0,
    EXIT_FATAL_ERROR        = 1,
    EXIT_OPTION_ERROR       = 2,
    EXIT_NOT_ROOT           = 3,
    EXIT_NO_KERNEL_SUPPORT  = 4,
    EXIT_USER_REQUEST       = 5,
    EXIT_LOCK_FAILED        = 6,
    EXIT_OPEN_FAILED        = 7,
    EXIT_CONNECT_FAILED     = 8,
    EXIT_PTYCMD_FAILED      = 9,
    EXIT_NEGOTIATION_FAILED = 10,
    EXIT_PEER_AUTH_FAILED   = 11,
    EXIT_IDLE_TIMEOUT       = 12,
    EXIT_CONNECT_TIME       = 13,
    EXIT_CALLBACK           = 14,
    EXIT_PEER_DEAD          = 15,
    EXIT_HANGUP             = 16,
    EXIT_LOOPBACK           = 17,
    EXIT_INIT_FAILED        = 18,
    EXIT_AUTH_TOPEER_FAILED = 19,
    EXIT_TRAFFIC_LIMIT      = 20,
    EXIT_CNID_AUTH_FAILED   = 21
} ppp_exit_code_t;

/*
 * Type of notifier callbacks
 */
typedef enum
{
    NF_PID_CHANGE,
    NF_PHASE_CHANGE,
    NF_EXIT,
    NF_SIGNALED,
    NF_IP_UP,
    NF_IP_DOWN,
    NF_IPV6_UP,
    NF_IPV6_DOWN,
    NF_AUTH_UP,
    NF_LINK_DOWN,
    NF_FORK,
    NF_MAX_NOTIFY
} ppp_notify_t;

typedef enum
{
    PPP_DIR_LOG,
    PPP_DIR_RUNTIME,
    PPP_DIR_CONF,
    PPP_DIR_PLUGIN,
} ppp_path_t;

/*
 * Unfortunately, the linux kernel driver uses a different structure
 * for statistics from the rest of the ports.
 * This structure serves as a common representation for the bits
 * pppd needs.
 */
struct pppd_stats
{
    uint64_t		bytes_in;
    uint64_t		bytes_out;
    unsigned int	pkts_in;
    unsigned int	pkts_out;
};
typedef struct pppd_stats ppp_link_stats_st;

/*
 * Used for storing a sequence of words.  Usually malloced.
 */
struct wordlist {
    struct wordlist	*next;
    char		*word;
};

struct option;
typedef void (*printer_func)(void *, char *, ...);

/*
 * The following struct gives the addresses of procedures to call for a particular protocol.
 */
struct protent {
    /* PPP protocol number */
    unsigned short protocol;
    /* Initialization procedure */
    void (*init)(int unit);
    /* Process a received packet */
    void (*input)(int unit, unsigned char *pkt, int len);
    /* Process a received protocol-reject */
    void (*protrej)(int unit);
    /* Lower layer has come up */
    void (*lowerup)(int unit);
    /* Lower layer has gone down */
    void (*lowerdown)(int unit);
    /* Open the protocol */
    void (*open)(int unit);
    /* Close the protocol */
    void (*close)(int unit, char *reason);
    /* Print a packet in readable form */
    int  (*printpkt)(unsigned char *pkt, int len, printer_func printer, void *arg);
    /* Process a received data packet */
    void (*datainput)(int unit, unsigned char *pkt, int len);
    /* 0 iff protocol is disabled */
    bool enabled_flag;
    /* Text name of protocol */
    char *name;
    /* Text name of corresponding data protocol */
    char *data_name;
    /* List of command-line options */
    struct option *options;
    /* Check requested options, assign defaults */
    void (*check_options)(void);
    /* Configure interface for demand-dial */
    int  (*demand_conf)(int unit);
    /* Say whether to bring up link for this pkt */
    int  (*active_pkt)(unsigned char *pkt, int len);
};

/* Table of pointers to supported protocols */
extern struct protent *protocols[];


/*
 * This struct contains pointers to a set of procedures for doing operations on a "channel".  
 * A channel provides a way to send and receive PPP packets - the canonical example is a serial 
 * port device in PPP line discipline (or equivalently with PPP STREAMS modules pushed onto it).
 */
struct channel {
	/* set of options for this channel */
	struct option *options;
	/* find and process a per-channel options file */
	void (*process_extra_options)(void);
	/* check all the options that have been given */
	void (*check_options)(void);
	/* get the channel ready to do PPP, return a file descriptor */
	int  (*connect)(void);
	/* we're finished with the channel */
	void (*disconnect)(void);
	/* put the channel into PPP `mode' */
	int  (*establish_ppp)(int);
	/* take the channel out of PPP `mode', restore loopback if demand */
	void (*disestablish_ppp)(int);
	/* set the transmit-side PPP parameters of the channel */
	void (*send_config)(int, uint32_t, int, int);
	/* set the receive-side PPP parameters of the channel */
	void (*recv_config)(int, uint32_t, int, int);
	/* cleanup on error or normal exit */
	void (*cleanup)(void);
	/* close the device, called in children after fork */
	void (*close)(void);
};

extern struct channel *the_channel;


/*
 * Functions for string formatting and debugging
 */

/* Is debug enabled */
bool debug_on(void);

/* Safe sprintf++ */
int slprintf(char *, int, const char *, ...);

/* vsprintf++ */
int vslprintf(char *, int, const char *, va_list);

/* safe strcpy */
size_t strlcpy(char *, const char *, size_t);

/* safe strncpy */
size_t strlcat(char *, const char *, size_t);

/* log a debug message */
void dbglog(const char *, ...);

/* log an informational message */
void info(const char *, ...);

/* log a notice-level message */
void notice(const char *, ...);

/* log a warning message */
void warn(const char *, ...);

/* log an error message */
void error(const char *, ...);

/* log an error message and die(1) */
void fatal(const char *, ...);

/* Say we ran out of memory, and die */
void novm(const char *);

/* Format a packet and log it with syslog */
void log_packet(unsigned char *, int, char *, int);

/* dump packet to debug log if interesting */
void dump_packet(const char *, unsigned char *, int);

/* initialize for using pr_log */
void init_pr_log(const char *, int);

/* printer fn, output to syslog */
void pr_log(void *, char *, ...);

/* finish up after using pr_log */
void end_pr_log(void);

/*
 * Get the current exist status of pppd
 */
ppp_exit_code_t ppp_status(void);

/*
 * Set the exit status
 */
void ppp_set_status(ppp_exit_code_t code);

/*
 * Configure the session's maximum number of octets
 */
void ppp_set_session_limit(unsigned int octets);

/*
 * Which direction to limit the number of octets
 */
void ppp_set_session_limit_dir(unsigned int direction);

/*
 * Get the current link stats, returns true when valid and false if otherwise
 */
bool ppp_get_link_stats(ppp_link_stats_st *stats);

/*
 * Get pppd's notion of time
 */
struct timeval;
int ppp_get_time(struct timeval *);

/*
 * Schedule a callback in s.us seconds from now
 */
typedef void (*ppp_timer_cb)(void *arg);
void ppp_timeout(ppp_timer_cb func, void *arg, int s, int us);

/*
 * Cancel any pending timer callbacks
 */
void ppp_untimeout(void (*func)(void *), void *arg);

/*
 * Clean up in a child before execing
 */
void ppp_sys_close(void);

/*
 * Fork & close stuff in child
 */
pid_t ppp_safe_fork(int, int, int);

/*
 * Get the current hostname
 */
const char *ppp_hostname(void);

/*
 * Is pppd using pty as a device (opposed to notty or pty opt).
 */
bool ppp_using_pty(void);

/*
 * Device is synchronous serial device
 */
bool ppp_sync_serial(void);

/*
 * Modem mode
 */
bool ppp_get_modem(void);

/*
 * Control the mode of the tty terminal
 */
void ppp_set_modem(bool on);

/*
 * Set the current session number, e.g. for PPPoE
 */
void ppp_set_session_number(int number);

/*
 * Set the current session number, e.g. for PPPoE
 */
int ppp_get_session_number(void);

/*
 * Check if pppd got signaled, returns 0 if not signaled, returns -1 on failure, and the signal number when signaled.
 */
bool ppp_signaled(int sig);

/*
 * Maximum connect time in seconds
 */
int ppp_get_max_connect_time(void);

/*
 * Set the maximum connect time in seconds
 */
void ppp_set_max_connect_time(unsigned int max);

/*
 * Get the link idle time before shutting the link down
 */
int ppp_get_max_idle_time(void);

/*
 * Set the link idle time before shutting the link down
 */
void ppp_set_max_idle_time(unsigned int idle);

/*
 * Get the duration the link was up (uptime)
 */
int ppp_get_link_uptime(void);

/*
 * Get the ipparam configured with pppd
 */
const char *ppp_ipparam(void);

/*
 * check if IP address is unreasonable
 */
bool ppp_bad_ip_addr(uint32_t);

/*
 * Expose an environment variable to scripts
 */
void ppp_script_setenv(char *, char *, int);

/*
 * Unexpose an environment variable to scripts
 */
void ppp_script_unsetenv(char *);

/*
 * Test whether ppp kernel support exists
 */
int ppp_check_kernel_support(void);

/*
 * Restore device setting
 */
void ppp_generic_disestablish(int dev_fd);

/*
 * Set the interface MTU
 */
void ppp_set_mtu(int, int);

/*
 * Get the interface MTU
 */
int  ppp_get_mtu(int);

/*
 * Make a ppp interface
 */
int ppp_generic_establish(int dev_fd);

/*
 * Get the peer's authentication name
 */
const char *ppp_peer_authname(char *buf, size_t bufsz);

/*
 * Get the remote name
 */
const char *ppp_remote_name(void);

/*
 * Get the remote number (if set), otherwise return NULL
 */
const char *ppp_get_remote_number(void);

/*
 * Set the remote number, typically it's a MAC address
 */
void ppp_set_remote_number(const char *buf);

/*
 * Get the current interface unit for the pppX device
 */
int ppp_ifunit(void);

/*
 * Get the current interface name
 */
const char *ppp_ifname(void);

/*
 * Get the current interface name
 */
int ppp_get_ifname(char *buf, size_t bufsz);

/*
 * Set the current interface name, ifname is a \0 terminated string
 */
void ppp_set_ifname(const char *ifname);

/*
 * Set the original devnam (prior to any renaming, etc).
 */
int ppp_set_pppdevnam(const char *name);

/*
 * Get the original devnam (prior to any renaming, etc).
 */
const char *ppp_pppdevnam(void);

/*
 * Get the current devnam, e.g. /dev/ttyS0, /dev/ptmx
 */
const char *ppp_devnam(void);

/*
 * Set the device name
 */
int ppp_set_devnam(const char *name);

/*
 * Definition for the notify callback function
 *   ctx - contextual argument provided with the registration
 *   arg - anything passed by the notification, e.g. phase, pid, etc
 */
typedef void (ppp_notify_fn)(void *ctx, int arg);

/*
 * Add a callback notification for when a given event has occured
 */
void ppp_add_notify(ppp_notify_t type, ppp_notify_fn *func, void *ctx);

/*
 * Remove a callback notification previously registered
 */
void ppp_del_notify(ppp_notify_t type, ppp_notify_fn *func, void *ctx);

/*
 * Get the path prefix in which a file is installed
 */
int ppp_get_path(ppp_path_t type, char *buf, size_t bufsz);

/*
 * Get the file with path prefix
 */
int ppp_get_filepath(ppp_path_t type, const char *name, char *buf, size_t bufsz);

/*
 * Check if pppd is to re-open link after it goes down
 */
bool ppp_persist(void);

/*
 * Hooks to enable plugins to hook into various parts of the code
 */

struct ppp_idle; /* Declared in <linux/ppp_defs.h> */
extern int (*idle_time_hook)(struct ppp_idle *);
extern int (*new_phase_hook)(int);
extern int (*holdoff_hook)(void);
extern int  (*allowed_address_hook)(uint32_t addr);
extern void (*snoop_recv_hook)(unsigned char *p, int len);
extern void (*snoop_send_hook)(unsigned char *p, int len);

#ifdef __cplusplus
}
#endif

#endif /* PPP_PPPD_H */
