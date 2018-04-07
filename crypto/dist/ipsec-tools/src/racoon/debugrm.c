/*	$NetBSD: debugrm.c,v 1.3.86.1 2018/04/07 04:11:47 pgoyette Exp $	*/

/*	$KAME: debugrm.c,v 1.6 2001/12/13 16:07:46 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define NONEED_DRM

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <err.h>

#include "debugrm.h"

#include "vmbuf.h"	/* need to mask vmbuf.c functions. */

#define DRMLISTSIZE 1024

struct drm_list_t {
	const void *ptr;
	char msg[100];
};
static struct drm_list_t drmlist[DRMLISTSIZE];

static int drm_unknown;

static void DRM_add(const void *, const char *);
static void DRM_del(const void *);
static void DRM_setmsg(char *, size_t, const void *, size_t, const char *,
    size_t, const char *);

void 
DRM_init(void)
{
	size_t i;
	drm_unknown = 0;
	for (i = 0; i < __arraycount(drmlist); i++)
		drmlist[i].ptr = 0;
}

void
DRM_dump(void)
{
	FILE *fp;
	size_t i;

	fp = fopen(DRMDUMPFILE, "w");
	if (fp == NULL)
		err(1, "fopen");	/*XXX*/
	fprintf(fp, "drm_unknown=%d\n", drm_unknown);
	for (i = 0; i < __arraycount(drmlist); i++) {
		if (drmlist[i].ptr)
			fprintf(fp, "%s\n", drmlist[i].msg);
	}
	fclose(fp);
}

static void 
DRM_add(const void *p, const char *msg)
{
	size_t i;
	for (i = 0; i < __arraycount(drmlist); i++) {
		if (!drmlist[i].ptr) {
			drmlist[i].ptr = p;
			strlcpy(drmlist[i].msg, msg, sizeof(drmlist[i].msg));
			return;
		}
	}
}

static void
DRM_del(const void *p)
{
	size_t i;

	if (!p)
		return;

	for (i = 0; i < __arraycount(drmlist); i++) {
		if (drmlist[i].ptr == p) {
			drmlist[i].ptr = 0;
			return;
		}
	}
	drm_unknown++;
}

static void
DRM_setmsg(char *buf, size_t buflen, const void *ptr, size_t size,
    const char *file, size_t line, const char *func)
{
	time_t t;
	struct tm *tm;
	int len;

	t = time(NULL);
	tm = localtime(&t);
	len = strftime(buf, buflen, "%Y/%m/%d:%T ", tm);

	snprintf(buf + len, buflen - len, "%p %6d %s:%d:%s",
		ptr, size, file, line, func);
}

void *
DRM_malloc(const char *file, size_t line, const char *func, size_t size)
{
	void *p;

	p = malloc(size);
	if (p) {
		char buf[1024];
		DRM_setmsg(buf, sizeof(buf), p, size, file, line, func);
		DRM_add(p, buf);
	}

	return p;
}

void *
DRM_calloc(const char *file, size_t line, const char *func, size_t number,
    size_t size)
{
	void *p;

	p = calloc(number, size);
	if (p) {
		char buf[1024];
		DRM_setmsg(buf, sizeof(buf), p, number * size, file, line, func);
		DRM_add(p, buf);
	}
	return p;
}

void *
DRM_realloc(const char *file, size_t line, const char *func, void *ptr,
    size_t size)
{
	void *p;

	p = realloc(ptr, size);
	if (p) {
		char buf[1024];
		if (ptr && p != ptr) {
			DRM_del(ptr);
			DRM_setmsg(buf, sizeof(buf), p, size, file, line, func);
			DRM_add(p, buf);
		}
	}

	return p;
}

void
DRM_free(const char *file, size_t line, const char *func, void *ptr)
{
	DRM_del(ptr);
	free(ptr);
}

char *
DRM_strdup(const char *file, size_t line, const char *func, const char *str)
{
	char *p;

	p = strdup(str);

	if (p) {
		char buf[1024];
		DRM_setmsg(buf, sizeof(buf), p, strlen(p), file, line, func);
		DRM_add(p, buf);
	}

	return p;
}

/*
 * mask vmbuf.c functions.
 */
void *
DRM_vmalloc(const char *file, size_t line, const char *func, size_t size)
{
	void *p;

	p = vmalloc(size);
	if (p) {
		char buf[1024];
		DRM_setmsg(buf, sizeof(buf), p, size, file, line, func);
		DRM_add(p, buf);
	}

	return p;
}

void *
DRM_vrealloc(const char *file, size_t line, const char *func, void *ptr,
    size_t size)
{
	void *p;

	p = vrealloc(ptr, size);
	if (p) {
		char buf[1024];
		if (ptr && p != ptr) {
			DRM_del(ptr);
			DRM_setmsg(buf, sizeof(buf), p, size, file, line, func);
			DRM_add(p, buf);
		}
	}

	return p;
}

void
DRM_vfree(const char *file, size_t line, const char *func, void *ptr)
{
	DRM_del(ptr);
	vfree(ptr);
}

void *
DRM_vdup(const char *file, size_t line, const char *func, void *ptr)
{
	void *p;

	p = vdup(ptr);
	if (p) {
		char buf[1024];
		DRM_setmsg(buf, sizeof(buf), p, 0, file, line, func);
		DRM_add(p, buf);
	}

	return p;
}
