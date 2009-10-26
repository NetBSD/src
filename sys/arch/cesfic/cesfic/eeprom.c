/* $NetBSD: eeprom.c,v 1.6 2009/10/26 19:16:55 cegger Exp $ */

/*
 * Copyright (c) 1997, 1999
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eeprom.c,v 1.6 2009/10/26 19:16:55 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>

/* Physical EEPROM */
#define EEPROMBASE 0x5a800000
#define EEPROMSIZE 0x2000
#define EEPROMEND (EEPROMBASE + 4*EEPROMSIZE)

/* Part of EEPROM used for system parameters: last 256 bytes */
#define EEPSIZE 256
#define EEPSTART (EEPROMEND - 4*EEPSIZE)

struct MemRange {
        unsigned short Start;
        unsigned short End;
        unsigned short Attr;
};

#define MAX_EXP 2
#define MAX_CACHE 3

struct uEEPROMDATA {
        unsigned long uSignature;
        unsigned char uMemA;
        unsigned char uMemB;
        unsigned char uEtherAddr[6];
        unsigned short uEthNFM;
        unsigned short uBusNFM;
        unsigned long uInternet;
        unsigned long uInterbroad;
        unsigned long uIntermask;
        struct MemRange uMemList[2][MAX_EXP];
        struct MemRange uCacheList[2][MAX_CACHE];
};

typedef unsigned short CHECKSUM;

struct EEPROMData {
        struct uEEPROMDATA u;
        char pad[EEPSIZE - sizeof(struct uEEPROMDATA) - sizeof(CHECKSUM)];
        CHECKSUM CheckSum;
};

#define Signature       u.uSignature
#define MemA            u.uMemA
#define MemB            u.uMemB
#define EtherAddr       u.uEtherAddr
#define EthNFM          u.uEthNFM
#define BusNFM          u.uBusNFM
#define Internet        u.uInternet
#define Interbroad      u.uInterbroad
#define Intermask       u.uIntermask
#define MemList         u.uMemList
#define CacheList       u.uCacheList

#define SIGNATURE (('D'<<24) + ('a'<<16) + ('t'<<8) + 'a')

static CHECKSUM CheckSum(CHECKSUM *);
static int GetEEPROMData(struct EEPROMData *);

static CHECKSUM
CheckSum(CHECKSUM *data)
{
	CHECKSUM c;
	int i;
	c = -1;
	for (i = 0; i < (EEPSIZE / sizeof(CHECKSUM) - 1); i++)
		c -= data[i];
	return (c);
}

static int
GetEEPROMData(struct EEPROMData *data)
{
	unsigned char *eep;
	int i;

	mainbus_map(EEPSTART, 4*EEPSIZE, 0, (void *)&eep);

	for (i = 0; i < EEPSIZE; i++)
		((char *)data)[i] = eep[4*i + 3];
	if (data->Signature != SIGNATURE)
		return (1);
	if (CheckSum((void *)data) != data->CheckSum)
		return (2);

	return (0);
}

static struct EEPROMData eeprom;
static int eeprom_read;

int
cesfic_getetheraddr(unsigned char *buf)
{
	int res;

	if (!eeprom_read) {
		res =  GetEEPROMData(&eeprom);
		if (res) {
			printf("error %d reading EEPROM\n", res);
			return (-1);
		}
		eeprom_read = 1;
	}
	memcpy(buf, eeprom.EtherAddr, 6);
	return (0);
}
