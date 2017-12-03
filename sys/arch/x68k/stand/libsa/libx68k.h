/*	$NetBSD: libx68k.h,v 1.5.62.2 2017/12/03 11:36:49 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Prototypes for x68k-specific libsa functions.
 */

struct devspec {
	const char *ds_name;
	int ds_dev;
	int ds_maxunit;
	int ds_net;
};

/* consio.c */
int consio_init(int);
int check_getchar(void);
int awaitkey_1sec(void);
void print_title(const char *, ...);

/* parseutils.c */
char *gettrailer(char *);
int parseopts(const char *, int *);
int parseboot(char *, char **, int *);

/* sdcd.c */
struct sd_softc;
int sdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int sdopen(struct open_file *, ...);
int sdclose(struct open_file *);
int sd_getbsdpartition(int, int);
int cdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int cdopen(struct open_file *, ...);
int cdclose(struct open_file *);

/* fd.c */
int fdstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int fdopen(struct open_file *, ...);
int fdclose(struct open_file *);

/* devopen.c */
int devparse(const char *, int *, int *, int *, int *, char **);
extern int devopen_open_dir;

/* chdsk.c */
int changedisk_hook(struct open_file *);

/* clock.c */
void delay(int);
extern int rtc_offset;
