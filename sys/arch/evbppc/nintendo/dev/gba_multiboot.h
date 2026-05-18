#ifndef _GBA_MULTIBOOT_H_
#define _GBA_MULTIBOOT_H_

#include "si.h"

#define SI_TRANS_DELAY	1236

static uint32_t status;

struct gba_multiboot_p {
	long		size;
	unsigned	chan;
	unsigned char	*rom;
	struct si_softc	*sc;
};

static inline uint32_t
gba_reset(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = SI_RESET;
	data.chan = chan;
	data.outsize = 1;
	data.insize = 3;
	data.in = in;
	data.out = out;

	delay(SI_TRANS_DELAY);
	__si_send(sc, &data);
	return in[0];
}

static inline uint32_t
gba_identify(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = SI_IDENTIFY;
	data.chan = chan;
	data.outsize = 1;
	data.insize = 3;
	data.in = in;
	data.out = out;

	delay(SI_TRANS_DELAY);
	__si_send(sc, &data);
	return in[0];
}

static inline uint32_t
gba_read32(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint8_t out[1];
	uint8_t in[5];

	out[0] = SI_GBARD;
	data.chan = chan;
	data.outsize = 1;
	data.insize = 5;
	data.in = in;
	data.out = out;

	delay(SI_TRANS_DELAY);
	__si_send(sc, &data);
	status = data.status;
	/* first four bytes are the data. 5th is status */
	return *(uint32_t *)in;
}

static inline uint32_t
gba_send32(struct si_softc *sc, unsigned chan, uint32_t msg)
{
	struct siio_send data;
	uint8_t out[5];
	uint8_t in[1];

	out[0] = SI_GBAWR;
	out[1] = (msg>>0)&0xFF;
	out[2] = (msg>>8)&0xFF;
	out[3] = (msg>>16)&0xFF;
	out[4] = (msg>>24)&0xFF;

	data.chan = chan;
	data.outsize = 5;
	data.insize = 1;
	data.in = in;
	data.out = out;

	delay(SI_TRANS_DELAY);
	__si_send(sc, &data);
	status = data.status;
	return (uint32_t)in[0];
}

static unsigned int
calckey(unsigned int size)
{
	unsigned int ret = 0;
	size=(size-0x200) >> 3;
	int res1 = (size&0x3F80) << 1;
	res1 |= (size&0x4000) << 2;
	res1 |= (size&0x7F);
	res1 |= 0x380000;
	int res2 = res1;
	res1 = res2 >> 0x10;
	int res3 = res2 >> 8;
	res3 += res1;
	res3 += res2;
	res3 <<= 24;
	res3 |= res2;
	res3 |= 0x80808080;

	if((res3&0x200) == 0) {
		ret |= (((res3)&0xFF)^0x4B)<<24;
		ret |= (((res3>>8)&0xFF)^0x61)<<16;
		ret |= (((res3>>16)&0xFF)^0x77)<<8;
		ret |= (((res3>>24)&0xFF)^0x61);
	} else {
		ret |= (((res3)&0xFF)^0x73)<<24;
		ret |= (((res3>>8)&0xFF)^0x65)<<16;
		ret |= (((res3>>16)&0xFF)^0x64)<<8;
		ret |= (((res3>>24)&0xFF)^0x6F);
	}
	return ret;
}

static unsigned int
docrc(uint32_t crc, uint32_t val)
{
	int i;
	for(i = 0; i < 0x20; i++)
	{
		if((crc^val)&1)
		{
			crc>>=1;
			crc^=0xa1c1;
		}
		else
			crc>>=1;
		val>>=1;
	}
	return crc;
}

static unsigned int
__gba_multiboot(struct gba_multiboot_p *gbm)
{
	uint32_t res;
	int count, i;
	unsigned chan = gbm->chan;
	unsigned char *rom = gbm->rom;
	long size = gbm->size;
	struct si_softc *sc = gbm->sc;

	count = 0;
	for (;;) {
		if (count == 500) {
			aprint_normal("GIVING UP\n");	
			return -1; // TODO better return val
		}

		gba_reset(sc, chan);
		res = gba_identify(sc, chan);
		aprint_normal("res is 0x%08X\n", res);
		if (res & 0x00001000) {
			aprint_normal("WE FOUND OUR MATCH!\n");
			break;
		}
		count++;
	}

	unsigned int sendsize = ((size+7)&~7);
	unsigned int ourkey = calckey(sendsize);

	uint32_t sessionkeyraw = gba_read32(sc, chan);
	uint32_t sessionkey = bswap32(sessionkeyraw^0x7365646F); // No idea this magic number
	gba_send32(sc, chan, bswap32(ourkey));
	unsigned int fcrc = 0x15A0; // i have no idea this one either..
	aprint_normal("SENDING HEADER\n");
	/* send header */
	for (i = 0; i < 0xC0; i += 4) {
		gba_send32(sc, chan, bswap32(*(uint32_t*)(rom+i)));
	}

	aprint_normal("SENDING ROM\n");
	for (i = 0xC0; i < sendsize; i += 4) {
		uint32_t enc = ((rom[i+3]<<24)|(rom[i+2]<<16)|(rom[i+1]<<8)|(rom[i]));
		fcrc = docrc(fcrc, enc);
		sessionkey = (sessionkey*0x6177614B)+1;
		enc^=sessionkey;
		enc^=((~(i+(0x20<<20)))+1);
		enc^=0x20796220;
		gba_send32(sc, chan, enc);
	}

	fcrc |= (sendsize<<16);
	sessionkey = (sessionkey*0x6177614B)+1;
	fcrc^=sessionkey;
	fcrc^=((~(i+(0x20<<20)))+1);
	fcrc^=0x20796220;

	gba_send32(sc, chan, fcrc);
	gba_read32(sc, chan);

	return 0;
}

#endif
