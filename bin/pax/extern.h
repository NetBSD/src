/*	$NetBSD: extern.h,v 1.17 1999/08/24 07:57:06 tron Exp $	*/

/*-
 * Copyright (c) 1992 Keith Muller.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)extern.h	8.2 (Berkeley) 4/18/94
 */

/*
 * External references from each source file
 */

#include <sys/cdefs.h>

/*
 * ar_io.c
 */
extern const char *arcname;
extern const char *gzip_program;
extern time_t starttime;
int ar_open __P((const char *));
void ar_close __P((void));
void ar_drain __P((void));
int ar_set_wr __P((void));
int ar_app_ok __P((void));
int ar_read __P((char *, int));
int ar_write __P((char *, int));
int ar_rdsync __P((void));
int ar_fow __P((off_t, off_t *));
int ar_rev __P((off_t ));
int ar_next __P((void));
void ar_summary __P((int));

/*
 * ar_subs.c
 */
extern u_long flcnt;
extern ARCHD archd;
void list __P((void));
void extract __P((void));
void append __P((void));
void archive __P((void));
void copy __P((void));

/*
 * buf_subs.c
 */
extern int blksz;
extern int wrblksz;
extern int maxflt;
extern int rdblksz;
extern off_t wrlimit;
extern off_t rdcnt;
extern off_t wrcnt;
int wr_start __P((void));
int rd_start __P((void));
void cp_start __P((void));
int appnd_start __P((off_t));
int rd_sync __P((void));
void pback __P((char *, int));
int rd_skip __P((off_t));
void wr_fin __P((void));
int wr_rdbuf __P((char *, int));
int rd_wrbuf __P((char *, int));
int wr_skip __P((off_t));
int wr_rdfile __P((ARCHD *, int, off_t *));
int rd_wrfile __P((ARCHD *, int, off_t *));
void cp_file __P((ARCHD *, int, int));
int buf_fill __P((void));
int buf_flush __P((int));

/*
 * cpio.c
 */
extern int cpio_swp_head;
int cpio_strd __P((void));
int cpio_subtrail __P((ARCHD *));
int cpio_endwr __P((void));
int cpio_id __P((char *, int));
int cpio_rd __P((ARCHD *, char *));
off_t cpio_endrd __P((void));
int cpio_stwr __P((void));
int cpio_wr __P((ARCHD *));
int vcpio_id __P((char *, int));
int crc_id __P((char *, int));
int crc_strd __P((void));
int vcpio_rd __P((ARCHD *, char *));
off_t vcpio_endrd __P((void));
int crc_stwr __P((void));
int vcpio_wr __P((ARCHD *));
int bcpio_id __P((char *, int));
int bcpio_rd __P((ARCHD *, char *));
off_t bcpio_endrd __P((void));
int bcpio_wr __P((ARCHD *));

/*
 * file_subs.c
 */
int file_creat __P((ARCHD *));
void file_close __P((ARCHD *, int));
int lnk_creat __P((ARCHD *));
int cross_lnk __P((ARCHD *));
int chk_same __P((ARCHD *));
int node_creat __P((ARCHD *));
int unlnk_exist __P((char *, int));
int chk_path __P((char *, uid_t, gid_t));
void set_ftime __P((char *fnm, time_t mtime, time_t atime, int frc));
int set_ids __P((char *, uid_t, gid_t));
void set_pmode __P((char *, mode_t));
int file_write __P((int, char *, int, int *, int *, int, char *));
void file_flush __P((int, char *, int));
void rdfile_close __P((ARCHD *, int *));
int set_crc __P((ARCHD *, int));

/*
 * ftree.c
 */
int ftree_start __P((void));
int ftree_add __P((char *));
void ftree_sel __P((ARCHD *));
void ftree_chk __P((void));
int next_file __P((ARCHD *));

/*
 * gen_subs.c
 */
void ls_list __P((ARCHD *, time_t));
void ls_tty __P((ARCHD *));
void zf_strncpy __P((char *, const char *, int));
int l_strncpy __P((char *, const char *, int));
u_long asc_ul __P((char *, int, int));
int ul_asc __P((u_long, char *, int, int));
#ifndef NET2_STAT
u_quad_t asc_uqd __P((char *, int, int));
int uqd_asc __P((u_quad_t, char *, int, int));
#endif
int check_Aflag __P((void));

/* 
 * getoldopt.c
 */
int getoldopt __P((int, char **, char *));

/*
 * options.c
 */
extern FSUB fsub[];
extern int ford[];
extern int cpio_mode;
extern char *chdir_dir;
void options __P((int, char **));
OPLIST * opt_next __P((void));
int opt_add __P((const char *));
int bad_opt __P((void));

/*
 * pat_rep.c
 */
int rep_add __P((char *));
int pat_add __P((char *));
void pat_chk __P((void));
int pat_sel __P((ARCHD *));
int pat_match __P((ARCHD *));
int mod_name __P((ARCHD *));
int set_dest __P((ARCHD *, char *, int));

/*
 * pax.c
 */
extern int act;
extern FSUB *frmt;
extern int Aflag;
extern int cflag;
extern int dflag;
extern int iflag;
extern int kflag;
extern int lflag;
extern int nflag;
extern int tflag;
extern int uflag;
extern int vflag;
extern int zflag;
extern int Dflag;
extern int Hflag;
extern int Lflag;
extern int Xflag;
extern int Yflag;
extern int Zflag;
extern int vfpart;
extern int patime;
extern int pmtime;
extern int pmode;
extern int pids;
extern int exit_val;
extern int docrc;
extern char *dirptr;
extern const char *ltmfrmt;
extern char *argv0;
int main __P((int, char **));
void sig_cleanup __P((int));

/*
 * sel_subs.c
 */
int sel_chk __P((ARCHD *));
int grp_add __P((char *));
int usr_add __P((char *));
int trng_add __P((char *));

/*
 * tables.c
 */
int lnk_start __P((void));
int chk_lnk __P((ARCHD *));
void purg_lnk __P((ARCHD *));
void lnk_end __P((void));
int ftime_start __P((void));
int chk_ftime __P((ARCHD *));
int name_start __P((void));
int add_name __P((char *, int, char *));
void sub_name __P((char *, int *));
int dev_start __P((void));
int add_dev __P((ARCHD *));
int map_dev __P((ARCHD *, u_long, u_long));
int atdir_start __P((void));
void atdir_end __P((void));
void add_atdir __P((char *, dev_t, ino_t, time_t, time_t));
int get_atdir __P((dev_t, ino_t, time_t *, time_t *));
int dir_start __P((void));
void add_dir __P((char *, int, struct stat *, int));
void proc_dir __P((void));
u_int st_hash __P((char *, int, int));

/*
 * tar.c
 */
extern int is_oldgnutar;
int tar_endwr __P((void));
off_t tar_endrd __P((void));
int tar_trail __P((char *, int, int *));
int tar_id __P((char *, int));
int tar_opt __P((void));
int tar_rd __P((ARCHD *, char *));
int tar_wr __P((ARCHD *));
int ustar_strd __P((void));
int ustar_stwr __P((void));
int ustar_id __P((char *, int));
int ustar_rd __P((ARCHD *, char *));
int ustar_wr __P((ARCHD *));
int tar_gnutar_X_compat __P((const char *));

/*
 * tty_subs.c
 */
int tty_init __P((void));
void tty_prnt __P((char *, ...))
    __attribute__((format (printf, 1, 2)));
int tty_read __P((char *, int));
void tty_warn __P((int, char *, ...))
    __attribute__((format (printf, 2, 3)));
void syswarn __P((int, int, char *, ...))
    __attribute__((format (printf, 3, 4)));
