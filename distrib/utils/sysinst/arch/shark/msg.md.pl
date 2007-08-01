/*	$NetBSD: msg.md.pl,v 1.7 2007/08/01 14:20:40 jmmv Exp $	*/
/* Based on english version: */
/*	NetBSD: msg.md.en,v 1.2 2002/04/02 17:02:54 thorpej Exp */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* shark machine dependent messages, Polish */


message md_hello
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.

}

message Keyboard_type {Keyboard type}
message kb_default {pl}

message fullpart
{Zainstalujemy teraz NetBSD na dysku %s. Mozesz wybrac czy chcesz
zainstalowac NetBSD na calym dysku, czy tylko na jego czesci.
Ktora instalacje chcesz zrobic?
}

message badreadbb
{Nie moge przeczytac bootbloku filecore
}

message badreadriscix
{Nie moge przeczytac tablicy partycji RISCiX
}

message notnetbsdriscix
{Nie znalazlem partycji NetBSD w tablicy partycji RISCiX - Nie moge nazwac
}

message notnetbsd
{Nie znalazlem partycji NetBSD (dysk filecore?) - Nie moge nazwac
}

message dobootblks
{Instalowanie bootblokow na %s....
}

message arm32fspart
{Partycje NetBSD na dysku %s wygladaja teraz tak (Rozmiary i Przesuniecia w %s):
}

message set_kernel_1
{Kernel (GENERIC)}

message setbootdevice
{In order to make your system boot automatically from the on-disk file
system, you need to manually configure OpenFirmware to tell it where to
load the kernel from.

OpenFirmware can load a NetBSD a.out kernel (sorry, ELF is not supported)
straight from a FFS partition on the local hard disk.  So, to configure it
to boot the just-installed kernel, run the following command from
OpenFirmware's shell:

setenv boot-device disk:\\netbsd.aout

You only need to run this once and only if the 'boot-device' property did
not already contain the value shown above.
}

message badclearmbr
{Failed to clear the disk's first sector.  If OpenFirmware cannot see the
disk, try to run the following command manually from the installer's shell
utility:

dd if=/dev/zero of=%s bs=512 count=1
}
