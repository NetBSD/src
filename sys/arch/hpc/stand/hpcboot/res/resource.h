/* -*-C++-*-	$NetBSD: resource.h,v 1.4.2.1 2002/06/23 17:36:44 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#define _APS_NO_MFC                     1

#define IDI_ICON		1
#define IDD_TIMER		2
#define IDC_CMDBAR		3

#define BOOT_BUTTON_WIDTH				50
#define IDC_PROGRESSBAR		4
#define IDC_BOOTBUTTON		5
#define IDC_CANCELBUTTON	6
// TabCtrl ID
#define IDC_BASE		7

#define TABCTRL_TAB_WIDTH				20
#define TABCTRL_TAB_HEIGHT				40
#define TABCTRL_PAD_WIDTH				2
#define TABCTRL_TAB_IMAGE_WIDTH						\
		(TABCTRL_TAB_WIDTH - TABCTRL_PAD_WIDTH * 2)
#define TABCTRL_TAB_IMAGE_HEIGHT					\
		(TABCTRL_TAB_HEIGHT - TABCTRL_PAD_WIDTH * 2)

// Child Dialog ID
#define IDC_BASE_MAIN		0
#define IDC_BASE_OPTION		1
#define IDC_BASE_CONSOLE	2

// Main Dialog ID
#define IDC_MAIN_ROOT_OPTION	100
#define IDC_MAIN_ROOT_		101
#define IDC_MAIN_ROOT_WD	101
#define IDC_MAIN_ROOT_SD	102
#define IDC_MAIN_ROOT_MD	103
#define IDC_MAIN_ROOT_NFS	104
#define IDC_MAIN_ROOT_MD_OPS	105

#define IDC_MAIN_OPTION_A	150
#define IDC_MAIN_OPTION_S	151
#define IDC_MAIN_OPTION_V	152
#define IDC_MAIN_OPTION_H	153
#define IDC_MAIN_OPTION_H_SPEED 154
#define IDC_MAIN_OPTION_D	155

#define IDC_MAIN_DIR		1001
#define IDC_MAIN_PLATFORM	1002
#define IDC_MAIN_KERNEL		1003


// Option Dialog ID
#define IDC_OPT_AUTO		200
#define IDC_OPT_AUTO_INPUT	2000
#define IDC_OPT_AUTO_UPDOWN	2001
#define IDC_OPT_VIDEO		201
#define IDC_OPT_PAUSE		202
#define IDC_OPT_DEBUG		203
#define IDC_OPT_SAFETY		205
#define IDC_OPT_EXTKOPT		206

// Console Dialog ID
#define IDC_CONS_CHK_		300
#define IDC_CONS_CHK0		300
#define IDC_CONS_CHK1		301
#define IDC_CONS_CHK2		302
#define IDC_CONS_CHK3		303
#define IDC_CONS_CHK4		304
#define IDC_CONS_CHK5		305
#define IDC_CONS_CHK6		306
#define IDC_CONS_CHK7		307
#define IDC_CONS_CHK_END	308
#define IDC_CONS_BTN_		310
#define IDC_CONS_BTN0		310
#define IDC_CONS_BTN1		311
#define IDC_CONS_BTN2		312
#define IDC_CONS_BTN3		313
#define IDC_CONS_EDIT		320
#define IDC_CONS_FILESAVE	321
#define IDC_CONS_FILENAME	322

// String table.
#define IDS_APP_TITLE			10000
#define IDS_HPCMENU			10002

#define IDS_DIR_FFS_ROOT1		10004
#define IDS_DIR_FFS_ROOT2		10006
#define IDS_DIR_FAT_ROOT		10008
#define IDS_DIR_FAT_MY_DOCUMENTS	10010
#define IDS_DIR_CARD_EN			10012
#define IDS_DIR_CARD_EN1		10014
#define IDS_DIR_CARD_EN2		10016
#define IDS_DIR_CARD_JP			10018
#define IDS_DIR_CARD_JP1		10020
#define IDS_DIR_CARD_JP2		10022
#define IDS_DIR_USER_DEFINED		10024

#define IDS_DIR_START			IDS_DIR_FFS_ROOT1
#define IDS_DIR_END			IDS_DIR_USER_DEFINED
#define IDS_DIR_SEQ(r)		(((r) - IDS_DIR_START) >> 1)
#define IDS_DIR_RES(n)		(((n) << 1) + IDS_DIR_START)
#define IDS_DIR_RES_VALID(n)						\
	((n) >= IDS_DIR_START && (n) <= IDS_DIR_END)

