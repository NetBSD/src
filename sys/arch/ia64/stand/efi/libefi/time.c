/*	$NetBSD: time.c,v 1.3.40.1 2016/10/05 20:55:29 skrll Exp $	*/

/*-
 * Copyright (c) 1999, 2000
 * Intel Corporation.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *    This product includes software developed by Intel Corporation and
 *    its contributors.
 * 
 * 4. Neither the name of Intel Corporation or its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/efi/libefi/time.c,v 1.4 2003/04/03 21:36:29 obrien Exp $");
 */
#include <efi.h>
#include <efilib.h>

#include <sys/time.h>

#include <machine/efilib.h>

/*
// Accurate only for the past couple of centuries;
// that will probably do.
//
// (#defines From FreeBSD 3.2 lib/libc/stdtime/tzfile.h)
*/

#define isleap(y)	(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define SECSPERHOUR ( 60*60 )
#define SECSPERDAY	(24 * SECSPERHOUR)

time_t
EfiTimeToUnixTime(EFI_TIME *ETime)
{
    /*
    //  These arrays give the cumulative number of days up to the first of the
    //  month number used as the index (1 -> 12) for regular and leap years.
    //  The value at index 13 is for the whole year.
    */
    static time_t CumulativeDays[2][14] = {
    {0,
     0,
     31,
     31 + 28,
     31 + 28 + 31,
     31 + 28 + 31 + 30,
     31 + 28 + 31 + 30 + 31,
     31 + 28 + 31 + 30 + 31 + 30,
     31 + 28 + 31 + 30 + 31 + 30 + 31,
     31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
     31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
     31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
     31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
     31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31 },
    {0,
     0,
     31,
     31 + 29,
     31 + 29 + 31,
     31 + 29 + 31 + 30,
     31 + 29 + 31 + 30 + 31,
     31 + 29 + 31 + 30 + 31 + 30,
     31 + 29 + 31 + 30 + 31 + 30 + 31,
     31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
     31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
     31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
     31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
     31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31 }};

    time_t  UTime; 
    int     Year;

    /*
    //  Do a santity check
    */
    if ( ETime->Year  <  1998 || ETime->Year   > 2099 ||
    	 ETime->Month ==    0 || ETime->Month  >   12 ||
    	 ETime->Day   ==    0 || ETime->Month  >   31 ||
    	                         ETime->Hour   >   23 ||
    	                         ETime->Minute >   59 ||
    	                         ETime->Second >   59 ||
    	 ETime->TimeZone  < -1440                     ||
    	 (ETime->TimeZone >  1440 && ETime->TimeZone != 2047) ) {
    	return (0);
    }

    /*
    // Years
    */
    UTime = 0;
    for (Year = 1970; Year != ETime->Year; ++Year) {
        UTime += (CumulativeDays[isleap(Year)][13] * SECSPERDAY);
    }

    /*
    // UTime should now be set to 00:00:00 on Jan 1 of the file's year.
    //
    // Months  
    */
    UTime += (CumulativeDays[isleap(ETime->Year)][ETime->Month] * SECSPERDAY);

    /*
    // UTime should now be set to 00:00:00 on the first of the file's month and year
    //
    // Days -- Don't count the file's day
    */
    UTime += (((ETime->Day > 0) ? ETime->Day-1:0) * SECSPERDAY);

    /*
    // Hours
    */
    UTime += (ETime->Hour * SECSPERHOUR);

    /*
    // Minutes
    */
    UTime += (ETime->Minute * 60);

    /*
    // Seconds
    */
    UTime += ETime->Second;

    /*
    //  EFI time is repored in local time.  Adjust for any time zone offset to
    //  get true UT
    */
    if ( ETime->TimeZone != EFI_UNSPECIFIED_TIMEZONE ) {
    	/*
    	//  TimeZone is kept in minues...
    	*/
    	UTime += (ETime->TimeZone * 60);
    }
    
    return UTime;
}

int
EFI_GetTimeOfDay(
	OUT struct timeval *tp,
	OUT struct timezone *tzp
	)
{
	EFI_TIME				EfiTime;
	EFI_TIME_CAPABILITIES	Capabilities;
	EFI_STATUS				Status;

	/*
	//  Get time from EFI
	*/

	Status = RS->GetTime( &EfiTime, &Capabilities );
	if (EFI_ERROR(Status))
		return (-1);

	/*
	//  Convert to UNIX time (ie seconds since the epoch
	*/

	tp->tv_sec  = EfiTimeToUnixTime( &EfiTime );
	tp->tv_usec = 0; /* EfiTime.Nanosecond * 1000; */

	/*
	//  Do something with the timezone if needed
	*/

	if (tzp) {
		tzp->tz_minuteswest =
			EfiTime.TimeZone == EFI_UNSPECIFIED_TIMEZONE ? 0 : EfiTime.TimeZone;
		/*
		//  This isn't quit right since it doesn't deal with EFI_TIME_IN_DAYLIGHT
		*/
		tzp->tz_dsttime =
			EfiTime.Daylight & EFI_TIME_ADJUST_DAYLIGHT ? 1 : 0;
	}

	return (0);
}

time_t
time(time_t *tloc)
{
	struct timeval tv;
	EFI_GetTimeOfDay(&tv, 0);
	
	if (tloc)
		*tloc = tv.tv_sec;
	return tv.tv_sec;
}

time_t
getsecs(void)
{
    return time(0);
}
