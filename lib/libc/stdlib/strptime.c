/*	$NetBSD: strptime.c,v 1.2 1997/04/23 00:01:17 jtc Exp $	*/

/*
 * Copyright (c) 1994 Powerdog Industries.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *      This product includes software developed by Powerdog Industries.
 * 4. The name of Powerdog Industries may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY POWERDOG INDUSTRIES ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE POWERDOG INDUSTRIES BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
#if 0
static char copyright[] =
"@(#) Copyright (c) 1994 Powerdog Industries.  All rights reserved.";
static char sccsid[] = "@(#)strptime.c  0.1 (Powerdog) 94/03/27";
#else
static char rcsid[] = "$NetBSD: strptime.c,v 1.2 1997/04/23 00:01:17 jtc Exp $";
#endif
#endif /* not lint */

#include <sys/localedef.h>      
#include <locale.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

char    *
strptime(const char *buf, const char *fmt, struct tm *tm)
{
	const char *ptr;
        char    c;
	int     i, len;

        ptr = fmt;
        while (*ptr != 0) {
                if (*buf == 0)
                        break;

                c = *ptr++;

                if (c != '%') {
                        if (isspace(c))
                                while (*buf != 0 && isspace(*buf))
                                        buf++;
                        else if (c != *buf++)
                                return 0;
                        continue;
                }

                c = *ptr++;
                switch (c) {
                case 0:
                case '%':
                        if (*buf++ != '%')
                                return 0;
                        break;

                case 'c':
                        buf = strptime(buf, _CurrentTimeLocale->d_t_fmt, tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'D':
                        buf = strptime(buf, "%m/%d/%y", tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'R':
                        buf = strptime(buf, "%H:%M", tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'r':
                        buf = strptime(buf, _CurrentTimeLocale->t_fmt_ampm,tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'T':
                        buf = strptime(buf, "%H:%M:%S", tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'X':
                        buf = strptime(buf, _CurrentTimeLocale->t_fmt, tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'x':
                        buf = strptime(buf, _CurrentTimeLocale->d_fmt, tm);
                        if (buf == 0)
                                return 0;
                        break;

                case 'j':
                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (i > 366)
                                return 0;

                        tm->tm_yday = i;
                        break;

                case 'M':
                case 'S':
                        if (*buf == 0 || isspace(*buf))
                                break;

                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (i > 59)
                                return 0;

                        if (c == 'M')
                                tm->tm_min = i;
                        else
                                tm->tm_sec = i;

                        if (*buf != 0 && isspace(*buf))
                                while (*ptr != 0 && !isspace(*ptr))
                                        ptr++;
                        break;

                case 'H':
                case 'I':
                case 'k':
                case 'l':
                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (c == 'H' || c == 'k') {
                                if (i > 23)
                                        return 0;
                        } else if (i > 11)
                                return 0;

                        tm->tm_hour = i;

                        if (*buf != 0 && isspace(*buf))
                                while (*ptr != 0 && !isspace(*ptr))
                                        ptr++;
                        break;

                case 'p':
                        len = strlen(_CurrentTimeLocale->am_pm[0]);
                        if (strncasecmp(buf, _CurrentTimeLocale->am_pm[0],
					len) == 0) {
                                if (tm->tm_hour > 12)
                                        return 0;
                                if (tm->tm_hour == 12)
                                        tm->tm_hour = 0;
                                buf += len;
                                break;
                        }

                        len = strlen(_CurrentTimeLocale->am_pm[1]);
                        if (strncasecmp(buf, _CurrentTimeLocale->am_pm[1],
					len) == 0) {
                                if (tm->tm_hour > 12)
                                        return 0;
                                if (tm->tm_hour != 12)
                                        tm->tm_hour += 12;
                                buf += len;
                                break;
                        }

                        return 0;

                case 'A':
                case 'a':
                        for (i = 0; i < 7; i++) {
                                len = strlen(_CurrentTimeLocale->day[i]);
                                if (strncasecmp(buf,
						_CurrentTimeLocale->day[i],
						len) == 0)
					break;

                                len = strlen(_CurrentTimeLocale->abday[i]);
                                if (strncasecmp(buf,
						_CurrentTimeLocale->abday[i],
						len) == 0)
					break;
                        }
                        if (i == 7)
                                return 0;

                        tm->tm_wday = i;
                        buf += len;
                        break;

                case 'd':
                case 'e':
                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (i > 31)
                                return 0;

                        tm->tm_mday = i;

                        if (*buf != 0 && isspace(*buf))
                                while (*ptr != 0 && !isspace(*ptr))
                                        ptr++;
                        break;

                case 'B':
                case 'b':
                case 'h':
                        for (i = 0; i < 12; i++) {
				len = strlen(_CurrentTimeLocale->mon[i]);
                                if (strncasecmp(buf,
						_CurrentTimeLocale->mon[i],
                                                len) == 0)
                                        break;

				len = strlen(_CurrentTimeLocale->abmon[i]);
                                if (strncasecmp(buf,
						_CurrentTimeLocale->abmon[i],
                                                len) == 0)
                                        break;
                        }
                        if (i == 12)
                                return 0;

                        tm->tm_mon = i;
                        buf += len;
                        break;

                case 'm':
                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (i < 1 || i > 12)
                                return 0;

                        tm->tm_mon = i - 1;

                        if (*buf != 0 && isspace(*buf))
                                while (*ptr != 0 && !isspace(*ptr))
                                        ptr++;
                        break;

                case 'Y':
                case 'y':
                        if (*buf == 0 || isspace(*buf))
                                break;

                        if (!isdigit(*buf))
                                return 0;

                        for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                                i *= 10;
                                i += *buf - '0';
                        }
                        if (c == 'Y')
                                i -= 1900;
                        if (i < 0)
                                return 0;

                        tm->tm_year = i;

                        if (*buf != 0 && isspace(*buf))
                                while (*ptr != 0 && !isspace(*ptr))
                                        ptr++;
                        break;
                }
        }

        return (char *) buf;
}


