/*	Id: unicode.c,v 1.7 2014/06/06 15:31:56 plunky Exp 	*/	
/*	$NetBSD: unicode.c,v 1.1.1.1.6.2 2014/08/19 23:52:09 tls Exp $	*/
/*
 * Copyright (c) 2014 Eric Olson <ejolson@renomath.org>
 * Some rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <ctype.h>
#include "pass1.h"
#include "manifest.h"
#include "unicode.h"

#if 0
/*
 * encode a 32-bit code point as UTF-8
 * return end position
 */
char *
cp2u8(char *p,unsigned int c)
{
	unsigned char *s=(unsigned char *)p;
	if(c>0x7F){
		if(c>0x07FF){
			if(c>0xFFFF){
				if(c>0x1FFFFF){
					if(c>0x3FFFFFF){
						if(c>0x7FFFFFFF){
							u8error("invalid unicode code point");
						} else {
							*s++=0xF8|(c>>30);
							*s++=0x80|((c>>24)&0x3F);
						}
					} else {
						*s++=0xF8|(c>>24);
					}
					*s++=0x80|((c>>18)&0x3F);
				} else {
					*s++=0xF0|(c>>18);
				}
				*s++=0x80|((c>>12)&0x3F);
			} else {
				*s++=0xE0|(c>>12);
			}
			*s++=0x80|((c>>6)&0x3F);
		} else {
			*s++=0xC0|(c>>6);
		}
		*s++=0x80|(c&0x3F);
	} else {
		*s++=c;
	}
	return (char *)s;
}
#endif

/*
 * decode 32-bit code point from UTF-8
 * move pointer
 */
unsigned int 
u82cp(char **q)
{
	unsigned char *t=(unsigned char *)*q;
	unsigned int c=*t;
	unsigned int r;
	if(c>0x7F){
		int sz;
		if((c&0xE0)==0xC0){
			sz=2;
			r=c&0x1F;
		} else if((c&0xF0)==0xE0){
			sz=3;
			r=c&0x0F;
		} else if((c&0xF8)==0xF0){
			sz=4;
			r=c&0x07;
		} else if((c&0xFC)==0xF8){
			sz=5;
			r=c&0x03;
		} else if((c&0xFE)==0xFC){
			sz=6;
			r=c&0x01;
		} else {
			u8error("invalid utf-8 prefix");
			(*q)++;
			return 0xFFFF;
		}
		t++;
		int i;
		for(i=1;i<sz;i++){
			if((*t&0xC0)==0x80){
				r=(r<<6)+(*t++&0x3F);
			} else {
				u8error("utf-8 encoding %d bytes too short",sz-i);
				(*q)++;
				return 0xFFFF;
			}
		}
	} else {
		r=*t++;
	}
	*q=(char *)t;
	return r;
}

/*
 * return length of UTF-8 code point
 */
int 
u8len(char *t)
{
	unsigned int c=(unsigned char)*t;
	if(c>0x7F){
		int sz;
		if((c&0xE0)==0xC0) sz=2;
		else if((c&0xF0)==0xE0) sz=3;
		else if((c&0xF8)==0xF0) sz=4;
		else if((c&0xFC)==0xF8) sz=5;
		else if((c&0xFE)==0xFC) sz=6;
		else return 1;
		int i;
		for(i=1;i<sz;i++){
			c=(unsigned char)*++t;
			if((c&0xC0)!=0x80) return 1;
		}
		return sz;
	}
	return 1;
}

unsigned int 
esc2char(char **q)
{
	unsigned int v;
	unsigned char *t=(unsigned char *)*q;
	unsigned int c=*t;
	if(c=='\\') {
		int i;
		switch(v=*++t){
case 'a':
			c='\a'; break;
case 'b':
			c='\b'; break;
#ifdef GCC_COMPAT
case 'e':
			c='\033'; break;
#endif
case 't':
			c='\t'; break;
case 'n':
			c='\n'; break;
case 'v':
			c='\v'; break;
case 'f':
			c='\f'; break;
case 'r':
			c='\r'; break;
case '\\':
			c='\\'; break;
case '\'':
			c='\''; break;
case '\"':
			c='\"'; break;
case '\?':
			c='\?'; break;
case 'x':
			v=*++t;
			for(i=0,c=0;;v=t[++i]){
				v=toupper(v);
				if(v>='0' && v<='9') c=(c<<4)+v-'0';
				else if(v>='A' && v<='F') c=(c<<4)+v-'A'+10;
				else break;
			}
			*q=(char *)t+i;
			return (xuchar == 0 && c > MAX_CHAR) ? (c - MAX_UCHAR - 1) : (c);
default:
			for(i=0,c=0;i<3;v=t[++i]){
				if(v>='0' && v<='7') c=(c<<3)+v-'0';
				else if(i==0) {
					u8error("unknown escape sequence \\%c",v);
					c='\\';
					break;
				} else {
					break;
				}
			}
			*q=(char *)t+i;
			return (xuchar == 0 && c > MAX_CHAR) ? (c - MAX_UCHAR - 1) : (c);
		}
	}
	*q=(char *)t+1;
	return c;
}
