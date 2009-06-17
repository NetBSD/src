/* $NetBSD: satafisreg.h,v 1.1 2009/06/17 03:07:51 jakllsch Exp $ */

/*-
 * Copyright (c) 2009 Jonathan A. Kollasch.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_ATA_FISREG_H_
#define _DEV_ATA_FISREG_H_

#define fis_type 0

#define RHD_FISTYPE 0x27
#define RHD_FISLEN 20
#define rhd_cdpmp 1 /* Command bit and PM port */
#define rhd_command 2 /* wd_command */
#define rhd_features 3 /* wd_precomp */
#define rhd_sector 4 /* wd_sector */
#define rhd_cyl_lo 5 /* wd_cyl_lo */
#define rhd_cyl_hi 6 /* wd_cyl_hi */
#define rhd_dh 7 /* wd_sdh */
#define rhd_sector_exp 8 
#define rhd_cyl_lo_exp 9
#define rhd_cyl_hi_exp 10
#define rhd_features_exp 11
#define rhd_seccnt 12
#define rhd_seccnt_exp 13
#define rhd_control 15

#define RDH_FISTYPE 0x34
#define RDH_FISLEN 20
#define rdh_i 1
#define rdh_status 2
#define rdh_error 3
#define rdh_sector 4 /* wd_sector */
#define rdh_cyl_lo 5 /* wd_cyl_lo */
#define rdh_cyl_hi 6 /* wd_cyl_hi */
#define rdh_dh 7 /* wd_sdh */
#define rhd_sector_exp 8
#define rhd_cyl_lo_exp 9
#define rhd_cyl_hi_exp 10
#define rhd_seccnt 12
#define rhd_seccnt_exp 13

#define SDB_FISTYPE 0xA1
#define SDB_FISLEN 8
#define sdb_i 1
#define sdb_status 2
#define sdb_error 3

#endif /* _DEV_ATA_FISREG_H_ */
