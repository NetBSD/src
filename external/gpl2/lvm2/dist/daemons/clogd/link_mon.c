/*	$NetBSD: link_mon.c,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#include <stdlib.h>
#include <errno.h>
#include <poll.h>

#include "logging.h"

struct link_callback {
	int fd;
	char *name;
	void *data;
	int (*callback)(void *data);

	struct link_callback *next;
};

static int used_pfds = 0;
static int free_pfds = 0;
static struct pollfd *pfds = NULL;
static struct link_callback *callbacks = NULL;

int links_register(int fd, char *name, int (*callback)(void *data), void *data)
{
	int i;
	struct link_callback *lc;

	for (i = 0; i < used_pfds; i++) {
		if (fd == pfds[i].fd) {
			LOG_ERROR("links_register: Duplicate file descriptor");
			return -EINVAL;
		}
	}

	lc = malloc(sizeof(*lc));
	if (!lc)
		return -ENOMEM;

	lc->fd = fd;
	lc->name = name;
	lc->data = data;
	lc->callback = callback;

	if (!free_pfds) {
		struct pollfd *tmp;
		tmp = realloc(pfds, sizeof(struct pollfd) * ((used_pfds*2) + 1));
		if (!tmp) {
			free(lc);
			return -ENOMEM;
		}
		
		pfds = tmp;
		free_pfds = used_pfds + 1;
	}

	free_pfds--;
	pfds[used_pfds].fd = fd;
	pfds[used_pfds].events = POLLIN;
	pfds[used_pfds].revents = 0;
	used_pfds++;

	lc->next = callbacks;
	callbacks = lc;
	LOG_DBG("Adding %s/%d", lc->name, lc->fd);
	LOG_DBG(" used_pfds = %d, free_pfds = %d",
		used_pfds, free_pfds);

	return 0;
}

int links_unregister(int fd)
{
	int i;
	struct link_callback *p, *c;

	for (i = 0; i < used_pfds; i++)
		if (fd == pfds[i].fd) {
			/* entire struct is copied (overwritten) */
			pfds[i] = pfds[used_pfds - 1];
			used_pfds--;
			free_pfds++;
		}

	for (p = NULL, c = callbacks; c; p = c, c = c->next)
		if (fd == c->fd) {
			LOG_DBG("Freeing up %s/%d", c->name, c->fd);
			LOG_DBG(" used_pfds = %d, free_pfds = %d",
				used_pfds, free_pfds);
			if (p)
				p->next = c->next;
			else
				callbacks = c->next;
			free(c);
			break;
		}

	return 0;
}

int links_monitor(void)
{
	int i, r;

	for (i = 0; i < used_pfds; i++) {
		pfds[i].revents = 0;
	}

	r = poll(pfds, used_pfds, -1);
	if (r <= 0)
		return r;

	r = 0;
	/* FIXME: handle POLLHUP */
	for (i = 0; i < used_pfds; i++)
		if (pfds[i].revents & POLLIN) {
			LOG_DBG("Data ready on %d", pfds[i].fd);
				
			/* FIXME: Add this back return 1;*/
			r++;
		}

	return r;
}

int links_issue_callbacks(void)
{
	int i;
	struct link_callback *lc;

	for (i = 0; i < used_pfds; i++)
		if (pfds[i].revents & POLLIN)
			for (lc = callbacks; lc; lc = lc->next)
				if (pfds[i].fd == lc->fd) {
					LOG_DBG("Issuing callback on %s/%d",
						lc->name, lc->fd);
					lc->callback(lc->data);
					break;
				}
	return 0;
}
