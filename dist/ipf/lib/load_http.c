/*	$NetBSD: load_http.c,v 1.1.1.2 2009/08/19 08:29:33 darrenr Exp $	*/

/*
 * Copyright (C) 2006 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: load_http.c,v 1.1.2.2 2009/07/23 20:01:12 darrenr Exp
 */

#include "ipf.h"

/*
 * Because the URL can be included twice into the buffer, once as the
 * full path for the "GET" and once as the "Host:", the buffer it is
 * put in needs to be larger than 512*2 to make room for the supporting
 * text. Why not just use snprintf and truncate? The warning about the
 * URL being too long tells you something is wrong and does not fetch
 * any data - just truncating the URL (with snprintf, etc) and sending
 * that to the server is allowing an unknown and unintentioned action
 * to happen.
 */
#define	MAX_URL_LEN	512
#define	LOAD_BUFSIZE	(MAX_URL_LEN * 2 + 128)

/*
 * Format expected is one addres per line, at the start of each line.
 */
alist_t *
load_http(char *url)
{
	char *s, *t, *u, buffer[LOAD_BUFSIZE], *myurl;
	int fd, len, left, port, endhdr, removed;
	alist_t *a, *rtop, *rbot;
	struct sockaddr_in sin;
	struct hostent *host;

	/*
	 * More than this would just be absurd.
	 */
	if (strlen(url) > MAX_URL_LEN) {
		fprintf(stderr, "load_http has a URL > %d bytes?!\n",
			MAX_URL_LEN);
		return NULL;
	}

	fd = -1;
	rtop = NULL;
	rbot = NULL;

	sprintf(buffer, "GET %s HTTP/1.0\r\n", url);

	myurl = strdup(url);
	if (myurl == NULL)
		goto done;

	s = myurl + 7;			/* http:// */
	t = strchr(s, '/');
	if (t == NULL) {
		fprintf(stderr, "load_http has a malformed URL '%s'\n", url);
		free(myurl);
		return NULL;
	}
	*t++ = '\0';

	/*
	 * 10 is the length of 'Host: \r\n\r\n' below.
	 */
	if (strlen(s) + strlen(buffer) + 10 > sizeof(buffer)) {
		fprintf(stderr, "load_http has a malformed URL '%s'\n", url);
		free(myurl);
		return NULL;
	}

	u = strchr(s, '@');
	if (u != NULL)
		s = u + 1;		/* AUTH */

	sprintf(buffer + strlen(buffer), "Host: %s\r\n\r\n", s);

	u = strchr(s, ':');
	if (u != NULL) {
		*u++ = '\0';
		port = atoi(u);
		if (port < 0 || port > 65535)
			goto done;
	} else {
		port = 80;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if (isdigit(*s)) {
		if (inet_aton(s, &sin.sin_addr) == -1) {
			goto done;
		}
	} else {
		host = gethostbyname(s);
		if (host == NULL)
			goto done;
		memcpy(&sin.sin_addr, host->h_addr_list[0],
		       sizeof(sin.sin_addr));
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		goto done;

	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		close(fd);
		goto done;
	}

	len = strlen(buffer);
	if (write(fd, buffer, len) != len) {
		close(fd);
		goto done;
	}

	s = buffer;
	endhdr = 0;
	left = sizeof(buffer) - 1;

	while ((len = read(fd, s, left)) > 0) {
		s[len] = '\0';
		left -= len;
		s += len;

		if (endhdr >= 0) {
			if (endhdr == 0) {
				t = strchr(buffer, ' ');
				if (t == NULL)
					continue;
				t++;
				if (*t != '2')
					break;
			}

			u = buffer;
			while ((t = strchr(u, '\r')) != NULL) {
				if (t == u) {
					if (*(t + 1) == '\n') {
						u = t + 2;
						endhdr = -1;
						break;
					} else
						t++;
				} else if (*(t + 1) == '\n') {
					endhdr++;
					u = t + 2;
				} else
					u = t + 1;
			}
			if (endhdr >= 0)
				continue;
			removed = (u - buffer) + 1;
			memmove(buffer, u, (sizeof(buffer) - left) - removed);
			s -= removed;
			left += removed;
		}

		do {
			t = strchr(buffer, '\n');
			if (t == NULL)
				break;

			*t++ = '\0';
			for (u = buffer; isdigit(*u) || (*u == '.'); u++)
				;
			if (*u == '/') {
				char *slash;

				slash = u;
				u++;
				while (isdigit(*u))
					u++;
				if (!isspace(*u) && *u)
					u = slash;
			}
			*u = '\0';

			a = alist_new(4, buffer);
			if (a != NULL) {
				if (rbot != NULL)
					rbot->al_next = a;
				else
					rtop = a;
				rbot = a;
			}

			removed = t - buffer;
			memmove(buffer, t, sizeof(buffer) - left - removed);
			s -= removed;
			left += removed;

		} while (1);
	}

done:
	if (myurl != NULL)
		free(myurl);
	if (fd != -1)
		close(fd);
	return rtop;
}
