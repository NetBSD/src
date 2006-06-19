/*	$NetBSD: lexer.l,v 1.1 2006/06/19 15:44:36 gdamore Exp $	*/

%{
/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * lexer.l
 *
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: lexer.l,v 1.1 2006/06/19 15:44:36 gdamore Exp $
 * $FreeBSD: src/usr.sbin/bluetooth/hcsecd/lexer.l,v 1.1 2003/05/10 21:50:35 julian Exp $
 */

#include <bluetooth.h>
#include <string.h>
#include "parser.h"

int yylex(void);
%}

%option yylineno noyywrap nounput

delim				[ \t\n]
ws				{delim}+
empty				{delim}*
comment				\#.*

hexdigit			[0-9a-fA-F]
hexbyte				{hexdigit}{hexdigit}
digit				[0-9]
value				{digit}+

device_word			device
bdaddr_word			bdaddr
name_word			name
key_word			key
pin_word			pin
control_psm_word		control_psm
control_channel_word		control_channel
interrupt_psm_word		interrupt_psm
reconnect_initiate_word		reconnect_initiate
battery_power_word		battery_power
normally_connectable_word	normally_connectable
hid_descriptor_word		hid_descriptor
true_word			true
false_word			false
type_word			type
hid_word			hid
hset_word			hset

bdaddrstring			{hexbyte}:{hexbyte}:{hexbyte}:{hexbyte}:{hexbyte}:{hexbyte}
hexstring			0x{hexbyte}{1,16}
string				\".+\"

%%

\;				return (';');
\:				return (':');
\{				return ('{');
\}				return ('}');

{ws}				;
{empty}				;
{comment}			;

{device_word}			return (T_DEVICE);
{bdaddr_word}			return (T_BDADDR);
{name_word}			return (T_NAME);
{key_word}			return (T_KEY);
{pin_word}			return (T_PIN);
{control_psm_word}		return (T_CONTROL_PSM);
{control_channel_word}		return (T_CONTROL_CHANNEL);
{interrupt_psm_word}		return (T_INTERRUPT_PSM);
{reconnect_initiate_word}	return (T_RECONNECT_INITIATE);
{battery_power_word}		return (T_BATTERY_POWER);
{normally_connectable_word}	return (T_NORMALLY_CONNECTABLE);
{hid_descriptor_word}		return (T_HID_DESCRIPTOR);
{true_word}			return (T_TRUE);
{false_word}			return (T_FALSE);
{type_word}			return (T_TYPE);
{hid_word}			return (T_HID);
{hset_word}			return (T_HSET);

{bdaddrstring}	{
		return (bt_aton(yytext, &_btyylval.bdaddr) ?
				T_BDADDRSTRING : T_ERROR);
		}

{hexstring}	{
		_btyylval.string = &yytext[2];
		return (T_HEXSTRING);
		}

{string}	{
		yytext[strlen(yytext) - 1] = 0;
		_btyylval.string = &yytext[1];
		return (T_STRING);
		}

{value}		{
		_btyylval.value = atoi(yytext);
		return (T_VALUE);
		}

.		return (T_ERROR);

%%
