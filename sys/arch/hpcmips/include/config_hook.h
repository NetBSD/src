/*	$NetBSD: config_hook.h,v 1.3 2000/04/03 03:48:42 sato Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

enum config_hook_mode {
	CONFIG_HOOK_SHARE,
	CONFIG_HOOK_REPLACE,
	CONFIG_HOOK_EXCLUSIVE,
};

typedef void *config_hook_tag;

void	config_hook_init __P((void));
config_hook_tag	config_hook __P((int type, long id, enum config_hook_mode mode,
				 int (*func) __P((void*, int, long, void*)), 
				 void *ctx));
void	config_unhook __P((config_hook_tag));
int	config_hook_call __P((int type, long id, void *msg));

/*
 * hook types and IDs
 */
#define CONFIG_HOOK_POWERCONTROL	0
#define CONFIG_HOOK_POWERCONTROL_COM0		0
#define CONFIG_HOOK_POWERCONTROL_COM1		1
#define CONFIG_HOOK_POWERCONTROL_COM2		2
#define CONFIG_HOOK_POWERCONTROL_COM3		3
#define CONFIG_HOOK_POWERCONTROL_LCDLIGHT	4
#define CONFIG_HOOK_POWERCONTROL_LCD		5

#define CONFIG_HOOK_BUTTONEVENT		1
#define CONFIG_HOOK_BUTTONEVENT_POWER		0
#define CONFIG_HOOK_BUTTONEVENT_OK		1
#define CONFIG_HOOK_BUTTONEVENT_CANCEL		2
#define CONFIG_HOOK_BUTTONEVENT_UP		3
#define CONFIG_HOOK_BUTTONEVENT_DOWN		4
#define CONFIG_HOOK_BUTTONEVENT_REC		5
#define CONFIG_HOOK_BUTTONEVENT_COVER		6
#define CONFIG_HOOK_BUTTONEVENT_LIGHT		7
#define CONFIG_HOOK_BUTTONEVENT_CONTRAST	8
#define CONFIG_HOOK_BUTTONEVENT_APP0		9
#define CONFIG_HOOK_BUTTONEVENT_APP1		10
#define CONFIG_HOOK_BUTTONEVENT_APP2		11
#define CONFIG_HOOK_BUTTONEVENT_APP3		12
#define CONFIG_HOOK_BUTTONEVENT_LEFT		13
#define CONFIG_HOOK_BUTTONEVENT_RIGHT		14
#define CONFIG_HOOK_BUTTONEVENT_CONTRAST_UP	15
#define CONFIG_HOOK_BUTTONEVENT_CONTRAST_DOWN	16
#define CONFIG_HOOK_BUTTONEVENT_LIGHT_UP	17
#define CONFIG_HOOK_BUTTONEVENT_LIGHT_DOWN	18

#define CONFIG_HOOK_NTYPES 		2

/*
 * nicknames for including from configration file.
 */
#ifdef CONFIG_HOOK_DEFINE_NICKNAME
#define PWCTL_COM0	CONFIG_HOOK_POWERCONTROL_COM0
#define PWCTL_COM1	CONFIG_HOOK_POWERCONTROL_COM1
#define PWCTL_COM2	CONFIG_HOOK_POWERCONTROL_COM2
#define PWCTL_COM3	CONFIG_HOOK_POWERCONTROL_COM3
#define PWCTL_LCDLIGHT	CONFIG_HOOK_POWERCONTROL_LCDLIGHT
#define PWCTL_LCD	CONFIG_HOOK_POWERCONTROL_LCD
#define BTN_POWER	CONFIG_HOOK_BUTTONEVENT_POWER
#define BTN_OK		CONFIG_HOOK_BUTTONEVENT_OK
#define BTN_CANCEL	CONFIG_HOOK_BUTTONEVENT_CANCEL
#define BTN_UP		CONFIG_HOOK_BUTTONEVENT_UP
#define BTN_DOWN	CONFIG_HOOK_BUTTONEVENT_DOWN
#define BTN_REC		CONFIG_HOOK_BUTTONEVENT_REC
#define BTN_COVER	CONFIG_HOOK_BUTTONEVENT_COVER
#define BTN_LIGHT	CONFIG_HOOK_BUTTONEVENT_LIGHT
#define BTN_CONTRAST	CONFIG_HOOK_BUTTONEVENT_CONTRAST
#define BTN_APP0	CONFIG_HOOK_BUTTONEVENT_APP0
#define BTN_APP1	CONFIG_HOOK_BUTTONEVENT_APP1
#define BTN_APP2	CONFIG_HOOK_BUTTONEVENT_APP2
#define BTN_APP3	CONFIG_HOOK_BUTTONEVENT_APP3
#define BTN_LEFT	CONFIG_HOOK_BUTTONEVENT_LEFT
#define BTN_RIGHT	CONFIG_HOOK_BUTTONEVENT_RIGHT
#define BTN_CONTRAST_UP	CONFIG_HOOK_BUTTONEVENT_CONTRAST_UP
#define BTN_CONTRAST_DN	CONFIG_HOOK_BUTTONEVENT_CONTRAST_DOWN
#define BTN_LIGHT_UP	CONFIG_HOOK_BUTTONEVENT_LIGHT_UP
#define BTN_LIGHT_DN	CONFIG_HOOK_BUTTONEVENT_LIGHT_DOWN
#endif /* CONFIG_HOOK_DEFINE_NICKNAME */
