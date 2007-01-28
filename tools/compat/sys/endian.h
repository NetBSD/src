/*	$NetBSD: endian.h,v 1.1 2007/01/28 09:19:33 dogcow Exp $	*/

static __inline uint16_t __unused
be16dec(const void *buf)
{
        const uint8_t *p = (const uint8_t *)buf;

        return ((p[0] << 8) | p[1]);
}

static __inline uint16_t __unused
le16dec(const void *buf)
{
        const uint8_t *p = (const uint8_t *)buf;

        return ((p[1] << 8) | p[0]);
}

be32dec(const void *buf)
{
        const uint8_t *p = (const uint8_t *)buf;

        return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static __inline uint32_t __unused
le32dec(const void *buf)
{
        const uint8_t *p = (const uint8_t *)buf;

        return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}
