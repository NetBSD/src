/*	$NetBSD: iconv.h,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#ifndef _SYS_ICONV_H_
#define _SYS_ICONV_H_

#define	ICONV_CSNMAXLEN		15	/* maximum length of charset name */

#define	ICONV_XLAT_ADD_VER	1

struct iconv_xlat_add {
	int		xa_version;
	char		xa_from[ICONV_CSNMAXLEN + 1];
	char		xa_to[ICONV_CSNMAXLEN + 1];
	u_char		xa_table[256];
};

#if !defined(_KERNEL) && !defined(_STANDALONE)

extern u_char nls_lower[256], nls_upper[256];

__BEGIN_DECLS

int   nls_setrecode(const char *, const char *);
int   nls_setlocale(const char *);
int   nls_lookupkerneltable(const char *to, const char *from);
int   nls_addkerneltable(const char *, const char *, const u_char *);
char* nls_str_toext(char *, const char *);
char* nls_str_toloc(char *, const char *);
void* nls_mem_toext(void *, const void *, int);
void* nls_mem_toloc(void *, const void *, int);
char* nls_str_upper(char *, const char *);
char* nls_str_lower(char *, const char *);

__END_DECLS

#else /* !_KERNEL */

#ifndef NetBSD
#include <sys/kobj.h>
#endif

/*
 * iconv driver class definition
 */
struct iconv_drv_class {
#ifndef NetBSD
	KOBJ_CLASS_FIELDS;
#endif
	TAILQ_ENTRY(iconv_drv_class)	dc_link;
};

TAILQ_HEAD(iconv_drv_list, iconv_drv_class);

/*
 * iconv driver instance
 */
struct iconv_drv {
#ifndef NetBSD
	KOBJ_FIELDS;
#endif
	void *			d_data;
	TAILQ_ENTRY(iconv_drv)	d_link;
};

/*
 * ccs class definition
 */
struct iconv_ccs_class {
	char **			c_names;
	char **			c_names2;
	TAILQ_ENTRY(iconv_ccs)	c_link;
};

/*
 * ccs class instance
 */
struct iconv_ccs {
/*	struct iconv_drv *	h_dp;	*//* backlink */
	void *data;
};

/*
 * xlat table
 */
struct iconv_xlat_tbl {
	const char **		x_from;
	const char **		x_to;
	u_char *		x_table;
	TAILQ_ENTRY(iconv_xlat_tbl)	x_link;
};

#define	ICONV_DRIVER(name,size) 				\
    static DEFINE_CLASS(iconv_ ## name, iconv_ ## name ## _methods, (size)); \
    static moduledata_t iconv_ ## name ## _mod = {	\
	"iconv_"#name, iconv_drvmod_handler,		\
	(void*)&iconv_ ## name ## _class		\
    };							\
    DECLARE_MODULE(iconv_ ## name, iconv_ ## name ## _mod, SI_SUB_DRIVERS, SI_ORDER_ANY);

#define	XLAT_DRIVER(name) 				\
    static moduledata_t iconv_xlat_ ## name ## _mod = {	\
	"iconv_xlat_"#name, iconv_xlatmod_handler,	\
	(void*)&iconv_xlat_ ## name ## _desc		\
    };							\
    DECLARE_MODULE(iconv_xlat_ ## name, iconv_xlat_ ## name ## _mod, SI_SUB_DRIVERS, SI_ORDER_ANY);

#ifdef MALLOC_DECLARE
MALLOC_DECLARE(M_ICONV);
#endif


int iconv_open(const char *to, const char *from, struct iconv_drv **dpp);
int iconv_close(struct iconv_drv *dp);
int iconv_conv(struct iconv_drv *dp, const char **inbuf,
	size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
char* iconv_convstr(struct iconv_drv *dp, char *dst, const char *src);
void* iconv_convmem(struct iconv_drv *dp, void *dst, const void *src, int size);

int iconv_lookupcp(const char **cpp, const char *s);
int iconv_drv_initstub(struct iconv_drv_class *dp);
int iconv_drv_donestub(struct iconv_drv_class *dp);

#ifndef NetBSD
int iconv_drvmod_handler(module_t mod, int type, void *data);
#endif

int iconv_xlat_add_table(struct iconv_xlat_tbl *xp);
int iconv_xlat_rm_table(struct iconv_xlat_tbl *xp);
#ifndef NetBSD
int iconv_xlatmod_handler(module_t mod, int type, void *data);
#endif

/*
void nls_str_upper(struct nls_tables *np, char *dst, const char *src);
void nls_str_lower(struct nls_tables *np, char *dst, const char *src);
int  nls_str_toext(struct nls_tables *np, char *dst, const char *src);
int  nls_path_toext(struct nls_tables *np, char *dst, const char *src);
int  nls_path_tolocal(struct nls_tables *np, char *dst, const char *src);
char nls_pathc_toext(struct nls_tables *np, char c);
*/
#endif /* !_KERNEL */

#endif /* _SYS_ICONV_H_ */
