/*	$NetBSD: bootxx.c,v 1.7 2001/10/14 19:43:44 leo Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#define	boot_BSD	bsd_startup

#include <stand.h>
#include <atari_stand.h>
#include <string.h>
#include <libkern.h>
#include <tosdefs.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <machine/cpu.h>


typedef int      (*bxxx_t) __P((void *, void *, struct osdsc *));

void	boot_BSD __P((struct kparamb *)__attribute__((noreturn)));
int	bootxxx __P((void *, void *, struct osdsc *));
int	load_booter __P((struct osdsc *));
int	usr_info __P((struct osdsc *));

int
bootxx(readsector, disklabel, autoboot)
	void	*readsector,
		*disklabel;
	int	autoboot;
{
	static osdsc_t	os_desc;
	extern char	end[], edata[];
	osdsc_t		*od = &os_desc;
	bxxx_t		bootxxx = (bxxx_t)(LOADADDR3);

	bzero(edata, end - edata);
	setheap(end, (void*)(LOADADDR3 - 4));

	printf("\033v\nNetBSD/Atari secondary bootloader"
						" ($Revision: 1.7 $)\n\n");

	if (init_dskio(readsector, disklabel, -1))
		return(-1);


	for (;;) {
		od->rootfs = 0;			/* partition a */
		od->osname = "/netbsd";
		od->ostype = &od->osname[1];
		od->boothowto = (RB_RDONLY);

		if (!autoboot) {
			int	pref;

			od->boothowto = (RB_RDONLY|RB_SINGLE);
			pref = usr_info(od);
			if (pref < 0)
				continue;
			if (pref > 0)
				return(pref);
		}
		autoboot = 0;			/* in case auto boot fails */

		
		if (init_dskio(readsector, disklabel, od->rootfs))
			continue;

		if (load_booter(od))
			continue;

		(*bootxxx)(readsector, disklabel, od);
	}
	/* NOTREACHED */
}


int
usr_info(od)
	osdsc_t	*od;
{
	static char	line[800];
	char		c, *p = line;

	printf("\nEnter os-type [.%s] root-fs [:a] kernel [%s]"
	       " options [none]:\n\033e", od->ostype, od->osname);
	gets(p);
	printf("\033f");

	for (;;) {
		while (isspace(*p))
			*p++ = '\0';

		switch (*p++) {
		  case '\0':
			goto done;
		  case ':':
			if ((c = *p) >= 'a' && c <= 'z')
				od->rootfs = c - 'a';
			else if (c >= 'A' && c <= 'Z')
				od->rootfs = c - ('A' - 27);
			else return(-1);

			if (!od->rootfs)
				break;
			*p = 'b';
			/* FALLTHROUGH */
		  case '-':
			if ((c = *p) == 'a')
				od->boothowto &= ~RB_SINGLE;
			else if (c == 'b')
				od->boothowto |= RB_ASKNAME;
			else
				BOOT_FLAG(c, od->boothowto);
			break;
		  case '.':
			od->ostype = p;
			break;
		  case '/':
			od->osname = --p;
			break;
		  default:
			return(-1);
		}

		while ((c = *p) && !isspace(c))
			p += 1;
	}

done:
	c = od->ostype[0];
	if (isupper(c))
		c = tolower(c);

	switch (c) {
	  case 'n':		/* NetBSD */
		return(0);
	  case 'l':		/* Linux  */
		return(0x10);
	  case 'a':		/* ASV    */
		return(0x40);
	  case 't':		/* TOS    */
		return(0x80);
	  default:
		return(-1);
	}
}

int
load_booter(od)
	osdsc_t		*od;
{
	int		fd;
	u_char		*bstart = (u_char *)(LOADADDR3);
	int		bsize;

	/*
	 * Read booter's exec-header.
	 */
	if ((fd = open("/boot.atari", 0)) < 0) {
		if ((fd = open("/boot", 0)) < 0) {
			printf("Cannot open /boot{.atari}\n");
			return (-1);
		}
	}
	while((bsize = read(fd, bstart, 1024)) > 0) {
		bstart += bsize;
	
	}
	close(fd);
	return 0;
}

void
_rtt()
{
	printf("Halting...\n");
	for(;;)
		;
}
