/*	$NetBSD: sockio.h,v 1.21 2022/09/28 15:32:09 msaitoh Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#ifndef _COMPAT_SYS_SOCKIO_H_
#define	_COMPAT_SYS_SOCKIO_H_

#include <sys/ioccom.h>
#include <compat/net/if.h>

#define	OSIOCSIFADDR	 _IOW('i', 12, struct oifreq)	/* set ifnet address */
#define	OOSIOCGIFADDR	 _IOWR('i', 13, struct oifreq)	/* get ifnet address */
#define	OSIOCSIFDSTADDR	 _IOW('i', 14, struct oifreq)	/* set p-p address */
#define	OOSIOCGIFDSTADDR _IOWR('i', 15, struct oifreq)	/* get p-p address */
#define OSIOCSIFFLAGS    _IOW('i', 16, struct oifreq)	/* set ifnet flags */
#define	OSIOCGIFFLAGS    _IOWR('i', 17, struct oifreq)	/* get ifnet flags */
#define	OOSIOCGIFBRDADDR _IOWR('i', 18, struct oifreq)	/* get bcast addr */
#define	OSIOCSIFBRDADDR	 _IOW('i', 19, struct oifreq)	/* set bcast addr */
#define	OOSIOCGIFCONF	 _IOWR('i', 20, struct ifconf)	/* get ifnet list */
#define	OOSIOCGIFNETMASK _IOWR('i', 21, struct oifreq)	/* get net addr mask */
#define	OSIOCSIFNETMASK	 _IOW('i', 22, struct oifreq)	/* set net addr mask */
#define	OSIOCGIFCONF	 _IOWR('i', 36, struct ifconf)	/* get ifnet list */
#define	OSIOCADDMULTI	 _IOW('i', 49, struct oifreq)	/* add m'cast addr */
#define	OSIOCDELMULTI	 _IOW('i', 50, struct oifreq)	/* del m'cast addr */
#define	SIOCSIFMEDIA_43	 _IOWR('i', 53, struct oifreq)	/* set net media */
#define	SIOCSIFMEDIA_80	 _IOWR('i', 53, struct ifreq)	/* set net media */
#define	SIOCGIFMEDIA_80	 _IOWR('i', 54, struct ifmediareq) /* set net media */
#define	OSIOCGIFMTU	 _IOWR('i', 126, struct oifreq)	/* get ifnet mtu */
#define	OSIOCGIFDATA	 _IOWR('i', 128, struct ifdatareq50) /* get if_data */
#define	OSIOCZIFDATA	 _IOWR('i', 129, struct ifdatareq50) /* get if_data then
							     zero ctrs*/

#define	OBIOCGETIF	 _IOR('B', 107, struct oifreq)
#define	OBIOCSETIF	 _IOW('B', 108, struct oifreq)
#define	OTAPGIFNAME	 _IOR('e', 0, struct oifreq)

#define IFREQN2O_43(oi, ni)					\
	do {							\
		(void)memcpy((oi)->ifr_name, (ni)->ifr_name,	\
		    sizeof((oi)->ifr_name));			\
		(void)memcpy(&(oi)->ifr_ifru, &(ni)->ifr_ifru,	\
		    sizeof((oi)->ifr_ifru));			\
	} while (/*CONSTCOND*/0)

#define IFREQO2N_43(oi, ni)					\
	do {							\
		(void)memcpy((ni)->ifr_name, (oi)->ifr_name,	\
		    sizeof((oi)->ifr_name));			\
		(void)memcpy(&(ni)->ifr_ifru, &(oi)->ifr_ifru,	\
		    sizeof((oi)->ifr_ifru));			\
	} while (/*CONSTCOND*/0)

#define ifdatan2o(oi, ni)					\
	do {							\
		(void)memcpy((oi), (ni),  sizeof(*(oi)));	\
		(oi)->ifi_lastchange.tv_sec =			\
		    (int32_t)(ni)->ifi_lastchange.tv_sec;	\
		(oi)->ifi_lastchange.tv_usec =			\
		    (ni)->ifi_lastchange.tv_nsec / 1000;	\
	} while (/*CONSTCOND*/0)

#define ifdatao2n(oi, ni)						   \
	do {								   \
		(void)memcpy((ni), (oi),  sizeof(*(oi)));		   \
		(ni)->ifi_lastchange.tv_sec = (oi)->ifi_lastchange.tv_sec; \
		(ni)->ifi_lastchange.tv_nsec =				   \
		    (oi)->ifi_lastchange.tv_usec * 1000;		   \
	} while (/*CONSTCOND*/0)

#endif /* _COMPAT_SYS_SOCKIO_H_ */
