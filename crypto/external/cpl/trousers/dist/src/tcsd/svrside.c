
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pwd.h>
#if (defined (__OpenBSD__) || defined (__FreeBSD__) || defined(__NetBSD__))
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsps.h"
#include "tcsd.h"
#include "req_mgr.h"

struct tcsd_config tcsd_options;
struct tpm_properties tpm_metrics;
static volatile int hup = 0, term = 0;
extern char *optarg;
char *tcsd_config_file = NULL;

struct srv_sock_info {
	int sd;
	int domain; // AF_INET or AF_INET6
	socklen_t addr_len;
};
#define MAX_IP_PROTO 2
#define INVALID_ADDR_STR "<Invalid client address>"

static void close_server_socks(struct srv_sock_info *socks_info)
{
	int i, rv;

	for (i=0; i < MAX_IP_PROTO; i++) {
		if (socks_info[i].sd != -1) {
			do {
				rv = close(socks_info[i].sd);
				if (rv == -1 && errno != EINTR) {
					LogError("Error closing server socket descriptor - %s",
							strerror(errno));
					continue;
				}
			} while (rv == -1 && errno == EINTR);
		}
	}
}

static void
tcsd_shutdown(struct srv_sock_info socks_info[])
{
	close_server_socks(socks_info);
	/* order is important here:
	 * allow all threads to complete their current request */
	tcsd_threads_final();
	PS_close_disk_cache();
	auth_mgr_final();
	(void)req_mgr_final();
	conf_file_final(&tcsd_options);
	EVENT_LOG_final();
}

static void
tcsd_signal_term(int signal)
{
	term = 1;
}

void
tcsd_signal_hup(int signal)
{
	hup = 1;
}

static TSS_RESULT
signals_init(void)
{
	int rc;
	sigset_t sigmask;
	struct sigaction sa;

	sigemptyset(&sigmask);
	if ((rc = sigaddset(&sigmask, SIGTERM))) {
		LogError("sigaddset: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if ((rc = sigaddset(&sigmask, SIGHUP))) {
		LogError("sigaddset: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if ((rc = THREAD_SET_SIGNAL_MASK(SIG_UNBLOCK, &sigmask, NULL))) {
		LogError("Setting thread signal mask: %s", strerror(rc));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = tcsd_signal_term;
	if ((rc = sigaction(SIGTERM, &sa, NULL))) {
		LogError("signal SIGTERM not registered: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	sa.sa_handler = tcsd_signal_hup;	
	if ((rc = sigaction(SIGHUP, &sa, NULL))) {
		LogError("signal SIGHUP not registered: %s", strerror(errno));
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

static TSS_RESULT
tcsd_startup(void)
{
	TSS_RESULT result;

#ifdef TSS_DEBUG
	/* Set stdout to be unbuffered to match stderr and interleave output correctly */
	setvbuf(stdout, (char *)NULL, _IONBF, 0);
#endif

	if ((result = signals_init()))
		return result;

	if ((result = conf_file_init(&tcsd_options)))
		return result;

	if ((result = tcsd_threads_init())) {
		conf_file_final(&tcsd_options);
		return result;
	}

	if ((result = req_mgr_init())) {
		conf_file_final(&tcsd_options);
		return result;
	}

	if ((result = ps_dirs_init())) {
		conf_file_final(&tcsd_options);
		(void)req_mgr_final();
		return result;
	}

	result = PS_init_disk_cache();
	if (result != TSS_SUCCESS) {
		conf_file_final(&tcsd_options);
		(void)req_mgr_final();
		return result;
	}

	if ((result = get_tpm_metrics(&tpm_metrics))) {
		conf_file_final(&tcsd_options);
		PS_close_disk_cache();
		(void)req_mgr_final();
		return result;
	}

	/* must happen after get_tpm_metrics() */
	if ((result = auth_mgr_init())) {
		conf_file_final(&tcsd_options);
		PS_close_disk_cache();
		(void)req_mgr_final();
		return result;
	}

	result = EVENT_LOG_init();
	if (result != TSS_SUCCESS) {
		auth_mgr_final();
		conf_file_final(&tcsd_options);
		PS_close_disk_cache();
		(void)req_mgr_final();
		return result;
	}

	result = owner_evict_init();
	if (result != TSS_SUCCESS) {
		auth_mgr_final();
		conf_file_final(&tcsd_options);
		PS_close_disk_cache();
		(void)req_mgr_final();
		return result;
	}

	return TSS_SUCCESS;
}


void
usage(void)
{
	fprintf(stderr, "\tusage: tcsd [-f] [-e] [-c <config file> [-h]\n\n");
	fprintf(stderr, "\t-f|--foreground\trun in the foreground. Logging goes to stderr "
			"instead of syslog.\n");
	fprintf(stderr, "\t-e| attempts to connect to software TPMs over TCP\n");
	fprintf(stderr, "\t-c|--config\tpath to configuration file\n");
	fprintf(stderr, "\t-h|--help\tdisplay this help message\n");
	fprintf(stderr, "\n");
}

static TSS_RESULT
reload_config(void)
{
	TSS_RESULT result;
	hup = 0;

	// FIXME: reload the config - work in progress
	result = TSS_SUCCESS;

	return result;
}

int setup_ipv4_socket(struct srv_sock_info ssi[])
{
	struct sockaddr_in serv_addr;
	int sd, opt;

	ssi->sd = -1;

	// Initialization of IPv4 socket.
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		LogWarn("Failed IPv4 socket: %s", strerror(errno));
		goto err;
	}

	memset(&serv_addr, 0, sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(tcsd_options.port);

	/* If no remote_ops are defined, restrict connections to localhost
	 * only at the socket. */
	if (tcsd_options.remote_ops[0] == 0)
		serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	else
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	opt = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (bind(sd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
		LogWarn("Failed IPv4 bind: %s", strerror(errno));
		goto err;
	}

	if (listen(sd, TCSD_MAX_SOCKETS_QUEUED) < 0) {
		LogWarn("Failed IPv4 listen: %s", strerror(errno));
		goto err;
	}

	ssi->domain = AF_INET;
	ssi->sd = sd;
	ssi->addr_len = sizeof(serv_addr);

	return 0;

 err:
	if (sd != -1)
		close(sd);

	return -1;
}

int setup_ipv6_socket(struct srv_sock_info *ssi)
{
	struct sockaddr_in6 serv6_addr;
	int sd6, opt;

	ssi->sd = -1;

	sd6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (sd6 < 0) {
		LogWarn("Failed IPv6 socket: %s", strerror(errno));
		goto err;
	}

	memset(&serv6_addr, 0, sizeof (serv6_addr));
	serv6_addr.sin6_family = AF_INET6;
	serv6_addr.sin6_port = htons(tcsd_options.port);

	/* If no remote_ops are defined, restrict connections to localhost
	 * only at the socket. */
	if (tcsd_options.remote_ops[0] == 0)
		serv6_addr.sin6_addr = in6addr_loopback;
	else
		serv6_addr.sin6_addr = in6addr_any;

#ifdef __linux__
	/* Linux, by default, allows one socket to be used by both IP stacks
	 * This option disables that behavior, so you must have one socket for
	 * each IP protocol. */
	opt = 1;
	if(setsockopt(sd6, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) == -1) {
		LogWarn("Could not set IPv6 socket option properly.\n");
		goto err;
	}
#endif

	opt = 1;
	setsockopt(sd6, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (bind(sd6, (struct sockaddr *) &serv6_addr, sizeof (serv6_addr)) < 0) {
		LogWarn("Failed IPv6 bind: %s", strerror(errno));
		goto err;
	}

	if (listen(sd6, TCSD_MAX_SOCKETS_QUEUED) < 0) {
		LogWarn("Failed IPv6 listen: %s", strerror(errno));
		goto err;
	}

	ssi->domain = AF_INET6;
	ssi->sd = sd6;
	ssi->addr_len = sizeof(serv6_addr);

	return 0;

 err:
	if (sd6 != -1)
		close(sd6);

	return -1;
}

int setup_server_sockets(struct srv_sock_info ssi[])
{
	int i=0;

	ssi[0].sd = ssi[1].sd = -1;
	// Only enqueue sockets successfully bound or that weren't disabled.
	if (tcsd_options.disable_ipv4) {
		LogWarn("IPv4 support disabled by configuration option");
	} else {
		if (setup_ipv4_socket(&ssi[i]) == 0)
			i++;
	}

	if (tcsd_options.disable_ipv6) {
		LogWarn("IPv6 support disabled by configuration option");
	} else {
		setup_ipv6_socket(&ssi[i]);
	}

	// It's only a failure if both sockets are unavailable.
	if ((ssi[0].sd == -1) && (ssi[1].sd == -1)) {
		return -1;
	}

	return 0;
}

char *fetch_hostname(struct sockaddr_storage *client_addr, socklen_t socklen)
{
	char buf[NI_MAXHOST];

	if (getnameinfo((struct sockaddr *)client_addr, socklen, buf,
						sizeof(buf), NULL, 0, 0) != 0) {
		LogWarn("Could not retrieve client address info");
		return NULL;
	} else {
		return strdup(buf);
	}
}

void prepare_for_select(struct srv_sock_info *socks_info, int *num_fds,
						fd_set *rdfd_set, int *nfds)
{
	int i;

	FD_ZERO(rdfd_set);
	*num_fds = 0;
	*nfds = 0;
	// Filter out socket descriptors in the queue that
	// has the -1 value.
	for (i=0; i < MAX_IP_PROTO; i++) {
		if (socks_info[i].sd == -1)
			break;

		FD_SET(socks_info[i].sd, rdfd_set);
		(*num_fds)++;
		if (*nfds < socks_info[i].sd) // grab highest sd for select call
			*nfds = socks_info[i].sd;
	}
}

int
main(int argc, char **argv)
{
	TSS_RESULT result;
	int newsd, c, rv, option_index = 0;
	int i;
	socklen_t client_len;
	char *hostname = NULL;
	fd_set rdfd_set;
	int num_fds = 0;
	int nfds = 0;
	int stor_errno;
	sigset_t sigmask, termmask, oldsigmask;
	struct sockaddr_storage client_addr;
	struct srv_sock_info socks_info[MAX_IP_PROTO];
	struct passwd *pwd;
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"foreground", 0, NULL, 'f'},
		{"config", 1, NULL, 'c'},
		{0, 0, 0, 0}
	};

	unsetenv("TCSD_USE_TCP_DEVICE");
	while ((c = getopt_long(argc, argv, "fhec:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'f':
				setenv("TCSD_FOREGROUND", "1", 1);
				break;
			case 'c':
				tcsd_config_file = optarg;
				break;
			case 'e':
				setenv("TCSD_USE_TCP_DEVICE", "1", 1);
				break;
			case 'h':
				/* fall through */
			default:
				usage();
				return -1;
				break;
		}
	}

	if (!tcsd_config_file)
		tcsd_config_file = TCSD_DEFAULT_CONFIG_FILE;

	if ((result = tcsd_startup()))
		return (int)result;

#ifdef NOUSERCHECK
    LogWarn("will not switch user or check for file permissions. "
            "(Compiled with --disable-usercheck)");
#else
#ifndef SOLARIS
	pwd = getpwnam(TSS_USER_NAME);
	if (pwd == NULL) {
		if (errno == 0) {
			LogError("User \"%s\" not found, please add this user"
					" manually.", TSS_USER_NAME);
		} else {
			LogError("getpwnam(%s): %s", TSS_USER_NAME, strerror(errno));
		}
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	setuid(pwd->pw_uid);
#endif
#endif

	if (setup_server_sockets(socks_info) == -1) {
		LogError("Could not create sockets to listen to connections. Aborting...");
		return -1;
	}

	if (getenv("TCSD_FOREGROUND") == NULL) {
		if (daemon(0, 0) == -1) {
			perror("daemon");
			tcsd_shutdown(socks_info);
			return -1;
		}
	}

	LogInfo("%s: TCSD up and running.", PACKAGE_STRING);

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGHUP);

	sigemptyset(&termmask);
	sigaddset(&termmask, SIGTERM);

	do {
		prepare_for_select(socks_info, &num_fds, &rdfd_set, &nfds);
		// Sanity check
		if (num_fds == 0) {
			LogError("No server sockets available to listen connections. Aborting...");
			return -1;
		}

		// Block TERM and HUP signals to prevent race condition
		if (sigprocmask(SIG_BLOCK, &sigmask, &oldsigmask) == -1) {
			LogError("Error setting interrupt mask before accept");
		}

		// TERM and HUP are blocked here, so its safe to test flags.
		if (hup) {
			// Config reading can be slow, so unmask SIGTERM.
			if (sigprocmask(SIG_UNBLOCK, &termmask, NULL) == -1) {
				LogError("Error unblocking SIGTERM before config reload");
			}
			if (reload_config() != TSS_SUCCESS)
				LogError("Failed reloading config");
			if (sigprocmask(SIG_BLOCK, &termmask, NULL) == -1) {
				LogError("Error blocking SIGTERM after config reload");
			}
		}
		if (term)
			break;

		// Select IPv4 and IPv6 socket descriptors with appropriate sigmask.
		LogDebug("Waiting for connections");
		rv = pselect(nfds+1, &rdfd_set, NULL, NULL, NULL, &oldsigmask);
		stor_errno = errno; // original mask must be set ASAP, so store errno.
		if (sigprocmask(SIG_SETMASK, &oldsigmask, NULL) == -1) {
			LogError("Error reseting signal mask to the original configuration.");
		}
		if (rv == -1) {
			if (stor_errno != EINTR) {
				LogError("Error monitoring server socket descriptors.");
				return -1;
			}
			continue;
		}

		for (i=0; i < num_fds; i++) { // accept connections from all IP versions (with valid sd)
			if (!FD_ISSET(socks_info[i].sd, &rdfd_set)) {
				continue;
			}
			client_len = socks_info[i].addr_len;
			newsd = accept(socks_info[i].sd, (struct sockaddr *) &client_addr, &client_len);
			if (newsd < 0) {
				if (errno != EINTR)
					LogError("Failed accept: %s", strerror(errno));
				continue;
			}
			LogDebug("accepted socket %i", newsd);

			hostname = fetch_hostname(&client_addr, client_len);
			if (hostname == NULL)
				hostname=INVALID_ADDR_STR;

			tcsd_thread_create(newsd, hostname);
			hostname = NULL;
		} // for (i=0; i < MAX_IP_PROTO; i++)
	} while (term ==0);

	/* To close correctly, we must receive a SIGTERM */
	tcsd_shutdown(socks_info);
	return 0;
}
