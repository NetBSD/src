/*-
 * Copyright (c) 1999 
 *	Contributed to NetBSD by Alexandre Wennmacher
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
 */

#include <sys/param.h>
#include <vis.h>

#define STRVISFLAG (VIS_CSTYLE | VIS_WHITE)

static char visbuf[4*MAXPATHLEN + 1];

char *
encode(dst, src)
        char *dst;
        const char *src;
{
        char *pv, *pd;

        (void)strvis(visbuf, src, STRVISFLAG);
        for (pv = visbuf, pd = dst; *pv != '\0';) {
                if (*pv == '\\') *pd++ = *pv++;     /* already encoded, skip */
                if (*pv == '#') *pd++ = '\\';       /* encode '#' */
                *pd++ = *pv++;
        }
        *pd = '\0';
        return dst;
}

char *
decode(dst, src)
        char *dst;
        const char *src;
{
        char *pv, *ps;

        for (ps = (char *)src, pv = visbuf; *ps != '\0';) {
                if (*ps == '\\' && *(ps + 1) == '#') ps++; /* encoded '#' */
                *pv++ = *ps++;
        }
        *pv = '\0';
        (void)strunvis(dst, visbuf);
        return dst;
}
