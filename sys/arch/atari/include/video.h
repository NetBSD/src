/*	$NetBSD: video.h,v 1.8 2022/07/03 11:30:48 andvar Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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

#ifndef _MACHINE_VIDEO_H
#define _MACHINE_VIDEO_H
/*
 * Access to circuitry for video
 */

#define	VIDEO	((struct video *)AD_VIDEO)

struct video {
    volatile	char	vdb[14];	/* sparsely filled		    */
    volatile	u_short vd_line_wide;	/* Falcon line word distance        */
    volatile	u_short vd_vert_wrap;	/* Falcon line length		    */
    volatile	char	vd_fill0[45];	/* filler			    */
    volatile	u_short	vd_st_rgb[16];	/* RGB for simultaneous colors	(ST)*/
    volatile	u_char	vd_st_res;	/* ST resolution		    */
    volatile	char	vd_fill1;	/* filler			    */
    volatile	u_short	vd_tt_res;	/* TT-resultion mode		    */
    volatile	u_char  vd_ste_hscroll;	/* MEGA STe hor bitwise scroll	    */
    volatile	u_short vd_fal_res;	/* Falcon resolution		    */
    volatile	char    vd_fill2[23];
    volatile	u_short vd_h_hold_cnt;	/* Falcon horizontal hold counter   */
    volatile	u_short vd_h_hold_tim;	/* Falcon horizontal hold timer     */
    volatile	u_short vd_h_bord_beg;  /* Falcon horizontal border begin   */
    volatile	u_short vd_h_bord_end;	/* Falcon horizontal border end     */
    volatile	u_short vd_h_dis_beg;	/* Falcon horizontal display begin  */
    volatile	u_short vd_h_dis_end;	/* Falcon horizontal display end    */
    volatile	u_short vd_h_ss;	/* Falcon horizontal SS             */
    volatile    u_short vd_h_fs;	/* Falcon horizontal FS		    */
    volatile	u_short vd_h_hh;	/* Falcon horizontal HH		    */
    volatile	char    vd_fill3[13];
    volatile	u_short vd_v_freq_cnt;	/* Falcon vertical frequency count  */
    volatile	u_short vd_v_freq_tim;	/* Falcon vertical frequency timer  */
    volatile	u_short vd_v_bord_beg;	/* Falcon vertical border begin     */
    volatile	u_short vd_v_bord_end;	/* Falcon vertical border end       */
    volatile	u_short vd_v_dis_beg;	/* Falcon vertical display begin    */
    volatile	u_short vd_v_dis_end;	/* Falcon vertical display end      */
    volatile	u_short vd_v_ss;	/* Falcon vertical SS               */
    volatile	char    vd_fill4[17];
    volatile	u_short vd_fal_ctrl;	/* Falcon video control		    */
    volatile	u_short vd_fal_mode;	/* Falcon video mode	            */
    volatile	char    vd_fill5[315];
    volatile	u_short vd_tt_rgb[256];	/* RGB for simultaneous TT colors   */
    volatile	char	vd_fill6[4608];
    volatile	u_long  vd_fal_rgb[256];/* RGB for Falcon colors            */
};

#define	vd_ramh		vdb[ 1]	/* base address Video RAM, high byte	*/
#define	vd_ramm		vdb[ 3]	/* base address Video RAM, mid byte	*/
#define	vd_raml		vdb[13]	/* base address Video RAM, low byte	*/
#define	vd_ptrh		vdb[ 5]	/* scan address Video RAM, high byte	*/
#define	vd_ptrm		vdb[ 7]	/* scan address Video RAM, mid byte	*/
#define	vd_ptrl		vdb[ 9]	/* scan address Video RAM, low byte	*/
#define	vd_sync		vdb[10]	/* synchronization mode			*/

/* bits in vd_sync: */
#define	SYNC_EXT	0x01	/* extern sync				*/
#define	SYNC_50		0x02	/* 50 Hertz (used for color)		*/

/* bits in vd_st_rgb[]: */
#define	RGB_B		0x0007
#define	RGB_G		0x0070
#define	RGB_R		0x0700

/* some values for vd_st_rgb[]: */
#define	RGB_BLACK	0x0000
#define	RGB_RED		0x0700
#define	RGB_GREEN	0x0070
#define	RGB_BLUE	0x0007
#define	RGB_WHITE	0x0777
#define	RGB_MAGENTA	0x0707
#define	RGB_CYAN	0x0077
#define	RGB_YELLOW	0x0770
#define	RGB_LGREY	0x0555
#define	RGB_DGREY	0x0222

/* values for vd_st_res: */
#define	RES_LOW		0x00	/* 320x200, 16 colors			*/
#define	RES_MID		0x01	/* 640x200,  4 colors			*/
#define	RES_HIGH	0x02	/* 640x400, monochrome			*/

/* masks for vd_tt_res: */
#define	RES_STLOW	0x0000	/* 320x200, 16 colors			*/
#define	RES_STMID	0x0100	/* 640x200,  4 colors			*/
#define	RES_STHIGH	0x0200	/* 640x400, monochrome			*/
#define	RES_TTMID	0x0400	/* 640x480, 16 colors			*/
#define	RES_TTHIGH	0x0600	/* 1280x960, monochrome			*/
#define	RES_TTLOW	0x0700	/* 320x480,  256 colors			*/
#define TT_PALLET	0x000f	/* Palette number			*/
#define	TT_HYMONO	0x8000	/* Hyper mono mode			*/
#define	TT_SHOLD	0x1000	/* Sample/hold mode			*/

/* The falcon video modes */
#define RES_FALAUTO	0	/* Falcon resolution dedected at boot	*/
#define RES_VGA2	1	/* 640x480,   2 colors			*/
#define RES_VGA4	2	/* 640x480,   4 colors			*/
#define RES_VGA16	3	/* 640x480,  16 colors			*/
#define RES_VGA256	4	/* 640x480, 256 colors			*/
#define RES_DIRECT	5	/* 320x200, 65536 colors		*/
#define RES_FAL_STLOW	6	/* 320x200,  16 colors			*/
#define RES_FAL_STMID	7	/* 640x200,   4 colors			*/
#define RES_FAL_STHIGH	8	/* 640x400,   2 colors			*/
#define RES_FAL_TTLOW	9	/* 320x480, 256 colors			*/

/* Falcon monitor types */
#define FAL_SM		0x0000
#define FAL_SC		0x0100
#define FAL_VGA		0x0200
#define FAL_TV		0x0300

#endif /*  _MACHINE_VIDEO_H */
