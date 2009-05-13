/*	$NetBSD: data.c,v 1.5.6.1 2009/05/13 19:20:42 jym Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMRUA Shin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>

#include "tpctl.h"

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: data.c,v 1.5.6.1 2009/05/13 19:20:42 jym Exp $");
#endif /* not lint */

static void *
alloc(int size)
{
	void *res;

	if ((res = malloc(size)) == NULL) {
		perror(getprogname());
		exit(EXIT_FAILURE);
	}

	return (res);
}

/*
 * duplicate string
 * trailing white space will be removed.
 */
static char *
strdup_prune(char *s)
{
	char *res, *tail;

	tail = &s[strlen(s) - 1];
	while (s <= tail && strchr(" \t", *tail) != NULL)
		tail--;

	res = alloc(tail - s + 2);
	memcpy(res, s, tail - s + 1);
	res[tail - s + 1] = '\0';

	return (res);
}

int
init_data(struct tpctl_data *data)
{
	TAILQ_INIT(&data->list);

	return (0);
}

int
read_data(const char *filename, struct tpctl_data *data)
{
	int res, len, n, i, t;
	char buf[MAXDATALEN + 2], *p, *p2;
	FILE *fp;
	struct tpctl_data_elem *elem;

	data->lineno = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return (ERR_NOFILE);

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		data->lineno++;
		buf[MAXDATALEN + 1] = '\0';
		len = strlen(buf);
		if (MAXDATALEN < len) {
			res = ERR_SYNTAX;
			goto exit_func;
		}

		/* prune trailing space and newline */;
		p = &buf[len - 1];
		while (buf <= p && strchr(" \t\n\r", *p) != NULL)
			*p-- = '\0';

		/* skip space */;
		p = buf; 
		while (*p != '\0' && strchr(" \t", *p) != NULL)
			p++;

		/* comment or empty line */
		if (*p == '#' || *p == '\0') {
			elem = alloc(sizeof(*elem));
			elem->type = TPCTL_COMMENT;
			elem->name = strdup_prune(buf);
			TAILQ_INSERT_TAIL(&data->list, elem, link);
			continue;
		}

		/* calibration parameter */
		elem = alloc(sizeof(*elem));
		elem->type = TPCTL_CALIBCOORDS;
		p2 = p;
		while (*p2 != ',' && *p2 != '\0')
			p2++;
		if (*p2 != ',') {
			/* missing ',' */
			res = ERR_SYNTAX;
			free(elem);
			goto exit_func;
		}
		*p2 = '\0';
		elem->name = strdup_prune(p);
		if (search_data(data, elem->name) != NULL) {
			free(elem);
			res = ERR_DUPNAME;
			goto exit_func;
		}
		TAILQ_INSERT_TAIL(&data->list, elem, link);
		p = p2 + 1;

		/*
		 * minX, maxX, minY, maxY
		 */
		for (i = 0; i < 4; i++) {
			t = strtol(p, &p2, 0);
			if (p == p2) {
				res = ERR_SYNTAX;
				goto exit_func;
			}
			p = p2;
			while (*p != '\0' && strchr(" \t", *p) != NULL)
				p++;
			if (*p != ',') {
				res = ERR_SYNTAX;
				goto exit_func;
			}
			p++;
			switch (i % 4) {
			case 0:
				elem->calibcoords.minx = t;
				break;
			case 1:
				elem->calibcoords.miny = t;
				break;
			case 2:
				elem->calibcoords.maxx = t;
				break;
			case 3:
				elem->calibcoords.maxy = t;
				break;
			}
		}

		/*
		 * number of samples
		 */
		n = strtol(p, &p2, 0);
		if (p == p2) {
			res = ERR_SYNTAX;
			goto exit_func;
		}
		p = p2;
		while (*p != '\0' && strchr(" \t", *p) != NULL)
			p++;

		if (WSMOUSE_CALIBCOORDS_MAX < n) {
			res = ERR_SYNTAX;
			goto exit_func;
		}
		elem->calibcoords.samplelen = n;

		/*
		 * samples
		 */
		for (i = 0; i < n * 4; i++) {
			if (*p != ',') {
				res = ERR_SYNTAX;
				goto exit_func;
			}
			p++;
			t = strtol(p, &p2, 0);
			if (p == p2) {
				res = ERR_SYNTAX;
				goto exit_func;
			}
			p = p2;
			while (*p != '\0' && strchr(" \t", *p) != NULL)
				p++;
			switch (i % 4) {
			case 0:
				elem->calibcoords.samples[i / 4].rawx = t;
				break;
			case 1:
				elem->calibcoords.samples[i / 4].rawy = t;
				break;
			case 2:
				elem->calibcoords.samples[i / 4].x = t;
				break;
			case 3:
				elem->calibcoords.samples[i / 4].y = t;
				break;
			}
		}
		if (*p != '\0') {
			res = ERR_SYNTAX;
			goto exit_func;
		}
	}

	if (ferror(fp))
		res = ERR_IO;
	else
		res = ERR_NONE;

 exit_func:
	fclose(fp);
	if (res != ERR_NONE) {
		free_data(data);
	}

	return (res);
}

int
write_data(const char *filename, struct tpctl_data *data)
{
	int res, fd;
	FILE *fp;
	struct tpctl_data_elem *elem;
	char *p, tempfile[MAXPATHLEN + 1];

	fd = 0;		/* XXXGCC -Wuninitialized [hpcarm] */

	if (filename == NULL) {
		fp = stdout;
	} else {
		strncpy(tempfile, filename, MAXPATHLEN);
		tempfile[MAXPATHLEN] = '\0';
		if ((p = strrchr(tempfile, '/')) == NULL) {
			strcpy(tempfile, TPCTL_TMP_FILENAME);
		} else {
			p++;
			if (MAXPATHLEN <
			    p - tempfile + strlen(TPCTL_TMP_FILENAME))
				return (ERR_NOFILE);/* file name is too long */
			strcat(tempfile, TPCTL_TMP_FILENAME);
		}
		if ((fd = open(tempfile, O_RDWR|O_CREAT|O_EXCL, 0644)) < 0) {
			fprintf(stderr, "%s: can't create %s\n",
			    getprogname(), tempfile);
			return (ERR_NOFILE);
		}
		if ((fp = fdopen(fd, "w")) == NULL) {
			perror("fdopen");
			exit(EXIT_FAILURE);
		}
	}

	TAILQ_FOREACH(elem, &data->list, link) {
		switch (elem->type) {
		case TPCTL_CALIBCOORDS:
			write_coords(fp, elem->name, &elem->calibcoords);
			break;
		case TPCTL_COMMENT:
			fprintf(fp, "%s\n", elem->name);
			break;
		default:
			fprintf(stderr, "%s: internal error\n", getprogname());
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (filename != NULL) {
		fclose(fp);
		close(fd);
		if (rename(tempfile, filename) < 0) {
			unlink(tempfile);
			return (ERR_NOFILE);
		}
	}
	res = ERR_NONE;

	return (res);
}

void
write_coords(FILE *fp, char *name, struct wsmouse_calibcoords *coords)
{
	int i;

	fprintf(fp, "%s,%d,%d,%d,%d,%d", name,
		coords->minx, coords->miny,
		coords->maxx, coords->maxy,
		coords->samplelen);
	for (i = 0; i < coords->samplelen; i++) {
		fprintf(fp, ",%d,%d,%d,%d",
		    coords->samples[i].rawx,
		    coords->samples[i].rawy,
		    coords->samples[i].x,
		    coords->samples[i].y);
	}
	fprintf(fp, "\n");
}

void
free_data(struct tpctl_data *data)
{
	struct tpctl_data_elem *elem;

	while (!TAILQ_EMPTY(&data->list)) {
		elem = TAILQ_FIRST(&data->list);
		TAILQ_REMOVE(&data->list, elem, link);

		switch (elem->type) {
		case TPCTL_CALIBCOORDS:
		case TPCTL_COMMENT:
			free(elem->name);
			break;
		default:
			fprintf(stderr, "%s: internal error\n", getprogname());
			exit(EXIT_FAILURE);
			break;
		}
	}
}

int
replace_data(struct tpctl_data *data, char *name, struct wsmouse_calibcoords *calibcoords)
{
	struct tpctl_data_elem *elem;

	TAILQ_FOREACH(elem, &data->list, link) {
		if (elem->type == TPCTL_CALIBCOORDS &&
		    strcmp(name, elem->name) == 0) {
			elem->calibcoords = *calibcoords;
			return (0);
		}
	}

	elem = alloc(sizeof(*elem));
	elem->type = TPCTL_CALIBCOORDS;
	elem->name = strdup(name);
	elem->calibcoords = *calibcoords;
	if (elem->name == NULL) {
		perror(getprogname());
		exit(EXIT_FAILURE);
	}
	TAILQ_INSERT_TAIL(&data->list, elem, link);

	return (1);
}

struct wsmouse_calibcoords *
search_data(struct tpctl_data *data, char *name)
{
	struct tpctl_data_elem *elem;

	TAILQ_FOREACH(elem, &data->list, link) {
		if (elem->type == TPCTL_CALIBCOORDS &&
		    strcmp(name, elem->name) == 0) {
			return (&elem->calibcoords);
		}
	}

	return (NULL);
}
