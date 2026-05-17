#ifndef _GBA_SI_H
#define _GBA_SI_H

#define SI_DELAY	50

struct gba_send {
	uint32_t	status;
	uint32_t 	insize;
	uint32_t 	outsize;
	void		*in;
	void		*out;
};

struct gba_multiboot {
	unsigned long		len;
	unsigned char 		*rom;
}

static inline uint32_t
__gba_reset(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = 0xFF000000;
	data.outsize = 1;
	data.insize = 3;
	data.out = out;
	data.in = in;
	data.chan = chan;

	delay(SI_DELAY);
	__si_send(sc, &data);
	return in[0];
}


static inline uint32_t
__gba_status(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint32_t out[1];
	uint32_t in[1];
	out[0] = 0x00000000;
	data.outsize = 1;
	data.insize = 3;
	data.out = out;
	data.in = in;
	data.chan = chan;

	delay(SI_DELAY);
	__si_send(sc, &data);
	return in[0];
}

static inline uint32_t
__gba_recv(struct si_softc *sc, unsigned chan)
{
	struct siio_send data;
	uint8_t out[1];
	uint8_t in[5];

	out[0] = 0x14;
	data.outsize = 1;
	data.insize = 5;
	data.out = out;
	data.in = in;
	data.chan = chan;

	delay(SI_DELAY);
	__si_send(sc, &data);
	return *(uint32_t *)in;
}

static inline uint32_t
__gba_send(struct si_softc *sc, unsigned chan, uint32_t msg)
{
	struct siio_send data;
	uint8_t out[5];
	uint8_t in[1];

	out[0] = 0x15;
        out[1] = (msg>>0)&0xFF;
        out[2] = (msg>>8)&0xFF;
        out[3] = (msg>>16)&0xFF;
        out[4] = (msg>>24)&0xFF;

	data.outsize = 5;
	data.insize = 1;
	data.out = out;
	data.in = in;
	data.chan = chan;

	delay(SI_DELAY);
	__si_send(sc, &data);
	return (uint32_t)in[0];
}

static unsigned int
__calckey(unsigned int size)
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
__docrc(uint32_t crc, uint32_t val)
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

static void
__gba_multiboot(struct si_softc *sc, unsigned chan, unsigned char *rom, long len)
{
	int count = 0;
	uint32_t res;
	for (;;) {
		if (count > 500) {
			// giving up
			return;
		}

		__gba_reset(sc, chan);
		res = __gba_status(sc, chan);
                if (res & 0x00001000) {
			aprint_normal("FOUND A MATCH\n");
                        break;
                }
                count++;
	}

        unsigned int sendsize = ((rom.size+7)&~7);
        unsigned int ourkey = __calckey(sendsize);
        uint32_t sessionkeyraw = __gba_recv(sc, chan);
        uint32_t sessionkey = bswap32(sessionkeyraw^0x7365646F); // No idea this magic number
        gba_send32(fd, ourkey);

}

 #endif
