/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "trace.h"
#include "list.h"
#include "eloop.h"

#ifdef CONFIG_ELOOP_POLL
#include <assert.h>
#include <poll.h>
#endif /* CONFIG_ELOOP_POLL */


struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	eloop_sock_handler handler;
	WPA_TRACE_REF(eloop)
	WPA_TRACE_REF(user)
	WPA_TRACE_INFO
};

struct eloop_timeout {
	struct dl_list list;
	struct os_time time;
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
	WPA_TRACE_REF(eloop)
	WPA_TRACE_REF(user)
	WPA_TRACE_INFO
};

struct eloop_signal {
	int sig;
	void *user_data;
	eloop_signal_handler handler;
	int signaled;
};

struct eloop_sock_table {
	int count;
	struct eloop_sock *table;
	int changed;
};

struct eloop_data {
	int max_sock;

	int count; /* sum of all table counts */
#ifdef CONFIG_ELOOP_POLL
	int max_pollfd_map; /* number of pollfds_map currently allocated */
	int max_poll_fds; /* number of pollfds currently allocated */
	struct pollfd *pollfds;
	struct pollfd **pollfds_map;
#endif /* CONFIG_ELOOP_POLL */
	struct eloop_sock_table readers;
	struct eloop_sock_table writers;
	struct eloop_sock_table exceptions;

	struct dl_list timeout;

	int signal_count;
	struct eloop_signal *signals;
	int signaled;
	int pending_terminate;

	int terminate;
	int reader_table_changed;
};

static struct eloop_data eloop;


#ifdef WPA_TRACE

static void eloop_sigsegv_handler(int sig)
{
	wpa_trace_show("eloop SIGSEGV");
	abort();
}

static void eloop_trace_sock_add_ref(struct eloop_sock_table *table)
{
	int i;
	if (table == NULL || table->table == NULL)
		return;
	for (i = 0; i < table->count; i++) {
		wpa_trace_add_ref(&table->table[i], eloop,
				  table->table[i].eloop_data);
		wpa_trace_add_ref(&table->table[i], user,
				  table->table[i].user_data);
	}
}


static void eloop_trace_sock_remove_ref(struct eloop_sock_table *table)
{
	int i;
	if (table == NULL || table->table == NULL)
		return;
	for (i = 0; i < table->count; i++) {
		wpa_trace_remove_ref(&table->table[i], eloop,
				     table->table[i].eloop_data);
		wpa_trace_remove_ref(&table->table[i], user,
				     table->table[i].user_data);
	}
}

#else /* WPA_TRACE */

#define eloop_trace_sock_add_ref(table) do { } while (0)
#define eloop_trace_sock_remove_ref(table) do { } while (0)

#endif /* WPA_TRACE */


int eloop_init(void)
{
	os_memset(&eloop, 0, sizeof(eloop));
	dl_list_init(&eloop.timeout);
#ifdef WPA_TRACE
	signal(SIGSEGV, eloop_sigsegv_handler);
#endif /* WPA_TRACE */
	return 0;
}


static int eloop_sock_table_add_sock(struct eloop_sock_table *table,
                                     int sock, eloop_sock_handler handler,
                                     void *eloop_data, void *user_data)
{
	struct eloop_sock *tmp;
	int new_max_sock;

	if (sock > eloop.max_sock)
		new_max_sock = sock;
	else
		new_max_sock = eloop.max_sock;

	if (table == NULL)
		return -1;

#ifdef CONFIG_ELOOP_POLL
	if (new_max_sock >= eloop.max_pollfd_map) {
		struct pollfd **nmap;
		nmap = os_realloc_array(eloop.pollfds_map, new_max_sock + 50,
					sizeof(struct pollfd *));
		if (nmap == NULL)
			return -1;

		eloop.max_pollfd_map = new_max_sock + 50;
		eloop.pollfds_map = nmap;
	}

	if (eloop.count + 1 > eloop.max_poll_fds) {
		struct pollfd *n;
		int nmax = eloop.count + 1 + 50;
		n = os_realloc_array(eloop.pollfds, nmax,
				     sizeof(struct pollfd));
		if (n == NULL)
			return -1;

		eloop.max_poll_fds = nmax;
		eloop.pollfds = n;
	}
#endif /* CONFIG_ELOOP_POLL */

	eloop_trace_sock_remove_ref(table);
	tmp = os_realloc_array(table->table, table->count + 1,
			       sizeof(struct eloop_sock));
	if (tmp == NULL)
		return -1;

	tmp[table->count].sock = sock;
	tmp[table->count].eloop_data = eloop_data;
	tmp[table->count].user_data = user_data;
	tmp[table->count].handler = handler;
	wpa_trace_record(&tmp[table->count]);
	table->count++;
	table->table = tmp;
	eloop.max_sock = new_max_sock;
	eloop.count++;
	table->changed = 1;
	eloop_trace_sock_add_ref(table);

	return 0;
}


static void eloop_sock_table_remove_sock(struct eloop_sock_table *table,
                                         int sock)
{
	int i;

	if (table == NULL || table->table == NULL || table->count == 0)
		return;

	for (i = 0; i < table->count; i++) {
		if (table->table[i].sock == sock)
			break;
	}
	if (i == table->count)
		return;
	eloop_trace_sock_remove_ref(table);
	if (i != table->count - 1) {
		os_memmove(&table->table[i], &table->table[i + 1],
			   (table->count - i - 1) *
			   sizeof(struct eloop_sock));
	}
	table->count--;
	eloop.count--;
	table->changed = 1;
	eloop_trace_sock_add_ref(table);
}


#ifdef CONFIG_ELOOP_POLL

static struct pollfd * find_pollfd(struct pollfd **pollfds_map, int fd, int mx)
{
	if (fd < mx && fd >= 0)
		return pollfds_map[fd];
	return NULL;
}


static int eloop_sock_table_set_fds(struct eloop_sock_table *readers,
				    struct eloop_sock_table *writers,
				    struct eloop_sock_table *exceptions,
				    struct pollfd *pollfds,
				    struct pollfd **pollfds_map,
				    int max_pollfd_map)
{
	int i;
	int nxt = 0;
	int fd;
	struct pollfd *pfd;

	/* Clear pollfd lookup map. It will be re-populated below. */
	os_memset(pollfds_map, 0, sizeof(struct pollfd *) * max_pollfd_map);

	if (readers && readers->table) {
		for (i = 0; i < readers->count; i++) {
			fd = readers->table[i].sock;
			assert(fd >= 0 && fd < max_pollfd_map);
			pollfds[nxt].fd = fd;
			pollfds[nxt].events = POLLIN;
			pollfds[nxt].revents = 0;
			pollfds_map[fd] = &(pollfds[nxt]);
			nxt++;
		}
	}

	if (writers && writers->table) {
		for (i = 0; i < writers->count; i++) {
			/*
			 * See if we already added this descriptor, update it
			 * if so.
			 */
			fd = writers->table[i].sock;
			assert(fd >= 0 && fd < max_pollfd_map);
			pfd = pollfds_map[fd];
			if (!pfd) {
				pfd = &(pollfds[nxt]);
				pfd->events = 0;
				pfd->fd = fd;
				pollfds[i].revents = 0;
				pollfds_map[fd] = pfd;
				nxt++;
			}
			pfd->events |= POLLOUT;
		}
	}

	/*
	 * Exceptions are always checked when using poll, but I suppose it's
	 * possible that someone registered a socket *only* for exception
	 * handling. Set the POLLIN bit in this case.
	 */
	if (exceptions && exceptions->table) {
		for (i = 0; i < exceptions->count; i++) {
			/*
			 * See if we already added this descriptor, just use it
			 * if so.
			 */
			fd = exceptions->table[i].sock;
			assert(fd >= 0 && fd < max_pollfd_map);
			pfd = pollfds_map[fd];
			if (!pfd) {
				pfd = &(pollfds[nxt]);
				pfd->events = POLLIN;
				pfd->fd = fd;
				pollfds[i].revents = 0;
				pollfds_map[fd] = pfd;
				nxt++;
			}
		}
	}

	return nxt;
}


static int eloop_sock_table_dispatch_table(struct eloop_sock_table *table,
					   struct pollfd **pollfds_map,
					   int max_pollfd_map,
					   short int revents)
{
	int i;
	struct pollfd *pfd;

	if (!table || !table->table)
		return 0;

	table->changed = 0;
	for (i = 0; i < table->count; i++) {
		pfd = find_pollfd(pollfds_map, table->table[i].sock,
				  max_pollfd_map);
		if (!pfd)
			continue;

		if (!(pfd->revents & revents))
			continue;

		table->table[i].handler(table->table[i].sock,
					table->table[i].eloop_data,
					table->table[i].user_data);
		if (table->changed)
			return 1;
	}

	return 0;
}


static void eloop_sock_table_dispatch(struct eloop_sock_table *readers,
				      struct eloop_sock_table *writers,
				      struct eloop_sock_table *exceptions,
				      struct pollfd **pollfds_map,
				      int max_pollfd_map)
{
	if (eloop_sock_table_dispatch_table(readers, pollfds_map,
					    max_pollfd_map, POLLIN | POLLERR |
					    POLLHUP))
		return; /* pollfds may be invalid at this point */

	if (eloop_sock_table_dispatch_table(writers, pollfds_map,
					    max_pollfd_map, POLLOUT))
		return; /* pollfds may be invalid at this point */

	eloop_sock_table_dispatch_table(exceptions, pollfds_map,
					max_pollfd_map, POLLERR | POLLHUP);
}

#else /* CONFIG_ELOOP_POLL */

static void eloop_sock_table_set_fds(struct eloop_sock_table *table,
				     fd_set *fds)
{
	int i;

	FD_ZERO(fds);

	if (table->table == NULL)
		return;

	for (i = 0; i < table->count; i++)
		FD_SET(table->table[i].sock, fds);
}


static void eloop_sock_table_dispatch(struct eloop_sock_table *table,
				      fd_set *fds)
{
	int i;

	if (table == NULL || table->table == NULL)
		return;

	table->changed = 0;
	for (i = 0; i < table->count; i++) {
		if (FD_ISSET(table->table[i].sock, fds)) {
			table->table[i].handler(table->table[i].sock,
						table->table[i].eloop_data,
						table->table[i].user_data);
			if (table->changed)
				break;
		}
	}
}

#endif /* CONFIG_ELOOP_POLL */


static void eloop_sock_table_destroy(struct eloop_sock_table *table)
{
	if (table) {
		int i;
		for (i = 0; i < table->count && table->table; i++) {
			wpa_printf(MSG_INFO, "ELOOP: remaining socket: "
				   "sock=%d eloop_data=%p user_data=%p "
				   "handler=%p",
				   table->table[i].sock,
				   table->table[i].eloop_data,
				   table->table[i].user_data,
				   table->table[i].handler);
			wpa_trace_dump_funcname("eloop unregistered socket "
						"handler",
						table->table[i].handler);
			wpa_trace_dump("eloop sock", &table->table[i]);
		}
		os_free(table->table);
	}
}


int eloop_register_read_sock(int sock, eloop_sock_handler handler,
			     void *eloop_data, void *user_data)
{
	return eloop_register_sock(sock, EVENT_TYPE_READ, handler,
				   eloop_data, user_data);
}


void eloop_unregister_read_sock(int sock)
{
	eloop_unregister_sock(sock, EVENT_TYPE_READ);
}


static struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type)
{
	switch (type) {
	case EVENT_TYPE_READ:
		return &eloop.readers;
	case EVENT_TYPE_WRITE:
		return &eloop.writers;
	case EVENT_TYPE_EXCEPTION:
		return &eloop.exceptions;
	}

	return NULL;
}


int eloop_register_sock(int sock, eloop_event_type type,
			eloop_sock_handler handler,
			void *eloop_data, void *user_data)
{
	struct eloop_sock_table *table;

	table = eloop_get_sock_table(type);
	return eloop_sock_table_add_sock(table, sock, handler,
					 eloop_data, user_data);
}


void eloop_unregister_sock(int sock, eloop_event_type type)
{
	struct eloop_sock_table *table;

	table = eloop_get_sock_table(type);
	eloop_sock_table_remove_sock(table, sock);
}


int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp;
	os_time_t now_sec;

	timeout = os_zalloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;
	if (os_get_time(&timeout->time) < 0) {
		os_free(timeout);
		return -1;
	}
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) {
		/*
		 * Integer overflow - assume long enough timeout to be assumed
		 * to be infinite, i.e., the timeout would never happen.
		 */
		wpa_printf(MSG_DEBUG, "ELOOP: Too long timeout (secs=%u) to "
			   "ever happen - ignore it", secs);
		os_free(timeout);
		return 0;
	}
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	wpa_trace_add_ref(timeout, eloop, eloop_data);
	wpa_trace_add_ref(timeout, user, user_data);
	wpa_trace_record(timeout);

	/* Maintain timeouts in order of increasing time */
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (os_time_before(&timeout->time, &tmp->time)) {
			dl_list_add(tmp->list.prev, &timeout->list);
			return 0;
		}
	}
	dl_list_add_tail(&eloop.timeout, &timeout->list);

	return 0;
}


static void eloop_remove_timeout(struct eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	wpa_trace_remove_ref(timeout, eloop, timeout->eloop_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);
	os_free(timeout);
}


int eloop_cancel_timeout(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev;
	int removed = 0;

	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
			      struct eloop_timeout, list) {
		if (timeout->handler == handler &&
		    (timeout->eloop_data == eloop_data ||
		     eloop_data == ELOOP_ALL_CTX) &&
		    (timeout->user_data == user_data ||
		     user_data == ELOOP_ALL_CTX)) {
			eloop_remove_timeout(timeout);
			removed++;
		}
	}

	return removed;
}


int eloop_is_timeout_registered(eloop_timeout_handler handler,
				void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp;

	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data)
			return 1;
	}

	return 0;
}


#ifndef CONFIG_NATIVE_WINDOWS
static void eloop_handle_alarm(int sig)
{
	wpa_printf(MSG_ERROR, "eloop: could not process SIGINT or SIGTERM in "
		   "two seconds. Looks like there\n"
		   "is a bug that ends up in a busy loop that "
		   "prevents clean shutdown.\n"
		   "Killing program forcefully.\n");
	exit(1);
}
#endif /* CONFIG_NATIVE_WINDOWS */


static void eloop_handle_signal(int sig)
{
	int i;

#ifndef CONFIG_NATIVE_WINDOWS
	if ((sig == SIGINT || sig == SIGTERM) && !eloop.pending_terminate) {
		/* Use SIGALRM to break out from potential busy loops that
		 * would not allow the program to be killed. */
		eloop.pending_terminate = 1;
		signal(SIGALRM, eloop_handle_alarm);
		alarm(2);
	}
#endif /* CONFIG_NATIVE_WINDOWS */

	eloop.signaled++;
	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].sig == sig) {
			eloop.signals[i].signaled++;
			break;
		}
	}
}


static void eloop_process_pending_signals(void)
{
	int i;

	if (eloop.signaled == 0)
		return;
	eloop.signaled = 0;

	if (eloop.pending_terminate) {
#ifndef CONFIG_NATIVE_WINDOWS
		alarm(0);
#endif /* CONFIG_NATIVE_WINDOWS */
		eloop.pending_terminate = 0;
	}

	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].signaled) {
			eloop.signals[i].signaled = 0;
			eloop.signals[i].handler(eloop.signals[i].sig,
						 eloop.signals[i].user_data);
		}
	}
}


int eloop_register_signal(int sig, eloop_signal_handler handler,
			  void *user_data)
{
	struct eloop_signal *tmp;

	tmp = os_realloc_array(eloop.signals, eloop.signal_count + 1,
			       sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	tmp[eloop.signal_count].signaled = 0;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);

	return 0;
}


int eloop_register_signal_terminate(eloop_signal_handler handler,
				    void *user_data)
{
	int ret = eloop_register_signal(SIGINT, handler, user_data);
	if (ret == 0)
		ret = eloop_register_signal(SIGTERM, handler, user_data);
	return ret;
}


int eloop_register_signal_reconfig(eloop_signal_handler handler,
				   void *user_data)
{
#ifdef CONFIG_NATIVE_WINDOWS
	return 0;
#else /* CONFIG_NATIVE_WINDOWS */
	return eloop_register_signal(SIGHUP, handler, user_data);
#endif /* CONFIG_NATIVE_WINDOWS */
}


void eloop_run(void)
{
#ifdef CONFIG_ELOOP_POLL
	int num_poll_fds;
	int timeout_ms = 0;
#else /* CONFIG_ELOOP_POLL */
	fd_set *rfds, *wfds, *efds;
	struct timeval _tv;
#endif /* CONFIG_ELOOP_POLL */
	int res;
	struct os_time tv, now;

#ifndef CONFIG_ELOOP_POLL
	rfds = os_malloc(sizeof(*rfds));
	wfds = os_malloc(sizeof(*wfds));
	efds = os_malloc(sizeof(*efds));
	if (rfds == NULL || wfds == NULL || efds == NULL)
		goto out;
#endif /* CONFIG_ELOOP_POLL */

	while (!eloop.terminate &&
	       (!dl_list_empty(&eloop.timeout) || eloop.readers.count > 0 ||
		eloop.writers.count > 0 || eloop.exceptions.count > 0)) {
		struct eloop_timeout *timeout;
		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout,
					list);
		if (timeout) {
			os_get_time(&now);
			if (os_time_before(&now, &timeout->time))
				os_time_sub(&timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
#ifdef CONFIG_ELOOP_POLL
			timeout_ms = tv.sec * 1000 + tv.usec / 1000;
#else /* CONFIG_ELOOP_POLL */
			_tv.tv_sec = tv.sec;
			_tv.tv_usec = tv.usec;
#endif /* CONFIG_ELOOP_POLL */
		}

#ifdef CONFIG_ELOOP_POLL
		num_poll_fds = eloop_sock_table_set_fds(
			&eloop.readers, &eloop.writers, &eloop.exceptions,
			eloop.pollfds, eloop.pollfds_map,
			eloop.max_pollfd_map);
		res = poll(eloop.pollfds, num_poll_fds,
			   timeout ? timeout_ms : -1);

		if (res < 0 && errno != EINTR && errno != 0) {
			perror("poll");
			goto out;
		}
#else /* CONFIG_ELOOP_POLL */
		eloop_sock_table_set_fds(&eloop.readers, rfds);
		eloop_sock_table_set_fds(&eloop.writers, wfds);
		eloop_sock_table_set_fds(&eloop.exceptions, efds);
		res = select(eloop.max_sock + 1, rfds, wfds, efds,
			     timeout ? &_tv : NULL);
		if (res < 0 && errno != EINTR && errno != 0) {
			perror("select");
			goto out;
		}
#endif /* CONFIG_ELOOP_POLL */
		eloop_process_pending_signals();

		/* check if some registered timeouts have occurred */
		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout,
					list);
		if (timeout) {
			os_get_time(&now);
			if (!os_time_before(&now, &timeout->time)) {
				void *eloop_data = timeout->eloop_data;
				void *user_data = timeout->user_data;
				eloop_timeout_handler handler =
					timeout->handler;
				eloop_remove_timeout(timeout);
				handler(eloop_data, user_data);
			}

		}

		if (res <= 0)
			continue;

#ifdef CONFIG_ELOOP_POLL
		eloop_sock_table_dispatch(&eloop.readers, &eloop.writers,
					  &eloop.exceptions, eloop.pollfds_map,
					  eloop.max_pollfd_map);
#else /* CONFIG_ELOOP_POLL */
		eloop_sock_table_dispatch(&eloop.readers, rfds);
		eloop_sock_table_dispatch(&eloop.writers, wfds);
		eloop_sock_table_dispatch(&eloop.exceptions, efds);
#endif /* CONFIG_ELOOP_POLL */
	}

out:
#ifndef CONFIG_ELOOP_POLL
	os_free(rfds);
	os_free(wfds);
	os_free(efds);
#endif /* CONFIG_ELOOP_POLL */
	return;
}


void eloop_terminate(void)
{
	eloop.terminate = 1;
}


void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;
	struct os_time now;

	os_get_time(&now);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
			      struct eloop_timeout, list) {
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) {
			sec--;
			usec += 1000000;
		}
		wpa_printf(MSG_INFO, "ELOOP: remaining timeout: %d.%06d "
			   "eloop_data=%p user_data=%p handler=%p",
			   sec, usec, timeout->eloop_data, timeout->user_data,
			   timeout->handler);
		wpa_trace_dump_funcname("eloop unregistered timeout handler",
					timeout->handler);
		wpa_trace_dump("eloop timeout", timeout);
		eloop_remove_timeout(timeout);
	}
	eloop_sock_table_destroy(&eloop.readers);
	eloop_sock_table_destroy(&eloop.writers);
	eloop_sock_table_destroy(&eloop.exceptions);
	os_free(eloop.signals);

#ifdef CONFIG_ELOOP_POLL
	os_free(eloop.pollfds);
	os_free(eloop.pollfds_map);
#endif /* CONFIG_ELOOP_POLL */
}


int eloop_terminated(void)
{
	return eloop.terminate;
}


void eloop_wait_for_read_sock(int sock)
{
#ifdef CONFIG_ELOOP_POLL
	struct pollfd pfd;

	if (sock < 0)
		return;

	os_memset(&pfd, 0, sizeof(pfd));
	pfd.fd = sock;
	pfd.events = POLLIN;

	poll(&pfd, 1, -1);
#else /* CONFIG_ELOOP_POLL */
	fd_set rfds;

	if (sock < 0)
		return;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	select(sock + 1, &rfds, NULL, NULL, NULL);
#endif /* CONFIG_ELOOP_POLL */
}
