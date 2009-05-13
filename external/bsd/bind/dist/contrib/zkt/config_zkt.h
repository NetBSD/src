/*	$NetBSD: config_zkt.h,v 1.1.1.1.2.2 2009/05/13 18:50:35 jym Exp $	*/

/*****************************************************************
**
**	@(#) config_zkt.h -- config options for ZKT
**
**	Copyright (c) Aug 2005, Holger Zuleger HZnet. All rights reserved.
**
**	This software is open source.
**
**	Redistribution and use in source and binary forms, with or without
**	modification, are permitted provided that the following conditions
**	are met:
**
**	Redistributions of source code must retain the above copyright notice,
**	this list of conditions and the following disclaimer.
**
**	Redistributions in binary form must reproduce the above copyright notice,
**	this list of conditions and the following disclaimer in the documentation
**	and/or other materials provided with the distribution.
**
**	Neither the name of Holger Zuleger HZnet nor the names of its contributors may
**	be used to endorse or promote products derived from this software without
**	specific prior written permission.
**
**	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
**	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
**	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
**	PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
**	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
**	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
**	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
**	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
**	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
**	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
**	POSSIBILITY OF SUCH DAMAGE.
**
*****************************************************************/
#ifndef CONFIG_ZKT_H
# define CONFIG_ZKT_H

#ifndef HAS_TIMEGM
# define	HAS_TIMEGM	1
#endif

#ifndef LOG_FNAMETMPL
# define	LOG_FNAMETMPL	"/zkt-%04d-%02d-%02dT%02d%02d%02dZ.log"
#endif

/* don't change anything below this */
/* the values here are determined or settable via the ./configure script */

#ifndef HAS_UTYPES
# define	HAS_UTYPES	1
#endif

#ifndef HAVE_GETOPT_LONG
# define	HAVE_GETOPT_LONG	1
#endif

#ifndef HAVE_STRFTIME
# define	HAVE_STRFTIME	1
#endif

#ifndef TTL_IN_KEYFILE_ALLOWED
# define	TTL_IN_KEYFILE_ALLOWED	1
#endif

#ifndef PRINT_TIMEZONE
# define	PRINT_TIMEZONE	0
#endif

#ifndef PRINT_AGE_WITH_YEAR
# define	PRINT_AGE_WITH_YEAR	0
#endif

#ifndef LOG_WITH_PROGNAME
# define	LOG_WITH_PROGNAME	0
#endif

#ifndef LOG_WITH_TIMESTAMP
# define	LOG_WITH_TIMESTAMP	1
#endif

#ifndef LOG_WITH_LEVEL
# define	LOG_WITH_LEVEL		1
#endif

#ifndef CONFIG_PATH
# define	CONFIG_PATH	"/var/named/"
#endif

/* tree usage is setable by configure script parameter */
#ifndef USE_TREE
# define	USE_TREE	1
#endif

/* BIND version and utility path will be set by ./configure script */
#ifndef BIND_VERSION
# define	BIND_VERSION	942
#endif

#ifndef BIND_UTIL_PATH
# define	BIND_UTIL_PATH	"/usr/local/sbin/"
#endif

#ifndef ZKT_VERSION
# if defined(USE_TREE) && USE_TREE
#  define	ZKT_VERSION	"vT0.98 (c) Feb 2005 - Sep 2008 Holger Zuleger hznet.de"
# else
#  define	ZKT_VERSION	"v0.98 (c) Feb 2005 - Sep 2008 Holger Zuleger hznet.de"
# endif
#endif


#if !defined(HAS_UTYPES) || !HAS_UTYPES
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
#endif

#endif
