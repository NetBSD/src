/*	$NetBSD: libsa.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*	$OpenBSD: libsa.h,v 1.7 1999/12/23 04:07:47 mickey Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <dev/cons.h>

#define	EXEC_ELF
/* #define	EXEC_ECOFF */
#define	EXEC_SOM

#define	NENTS(a)	(sizeof(a) / sizeof(a[0]))

typedef unsigned int btdev_t;

extern	btdev_t bootdev;

extern	struct consdev constab[];
extern	struct consdev *cn_tab;
int	cngetc(void);
int	tcngetc(void);
void	cnputc(int);
void	cninit(void);
int	tgetchar(void);

void pdc_init(void);
struct pz_device;
struct pz_device *pdc_findev(int, int);

int iodcstrategy(void *, int, daddr_t, size_t, void *, size_t *);

int ctopen(struct open_file *, ...);
int ctclose(struct open_file *);

int dkopen(struct open_file *, ...);
int dkclose(struct open_file *);

int lfopen(struct open_file *, ...);
int lfstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int lfclose(struct open_file *);

void ite_probe(struct consdev *);
void ite_init(struct consdev *);
int ite_getc(dev_t);
void ite_putc(dev_t, int);
void ite_pollc(dev_t, int);

void machdep(void);
void devboot(btdev_t, char *);
void fcacheall(void);

int     lif_open(const char *, struct open_file *);
int     lif_close(struct open_file *);
int     lif_read(struct open_file *, void *, size_t, size_t *);
int     lif_write(struct open_file *, void *, size_t, size_t *);
off_t   lif_seek(struct open_file *, off_t, int);
int     lif_stat(struct open_file *, struct stat *);
int     lif_readdir(struct open_file *, char *);

union x_header;
struct x_param;
int	som_probe(int, union x_header *);
int	som_load(int, struct x_param *);
int	som_ldsym(int, struct x_param *);

extern int debug;
extern	char bootprog_name[], bootprog_rev[];
