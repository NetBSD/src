# The Microcode for the Eicon.Diehl active ISDN cards comes in two parts.
# A tiny bootstrap loader delivered by Diehl as "dnload.bin" and a
# D-channel protocol specific file, e.g. "te_etsi.sx" for the SX card
# running E-DSS1.
# We can not deliver either of these files. The user is assumed to use
# the ones distributed with his card or download the newest version from
# http://www.diehl.de.
# The bootstrap loader is compiled into the kernel, as it is needed to 
# identify the card. The protocol specific part is later downloaded by
# the ISDN management daemon.

.NOPATH: dnload.bin
.if exists(dnload.bin)
# Real part - the bootstrap image is available. We convert it into a
# static byte array via the "gendnloadh" shell script.
dnload.h:	dnload.bin
	@rm -f dnload.h
	@sh ./gendnloadh
.else
# Fake part - user did not copy the necessary file here. Compilation will
# fail with an error explaining his fault.
dnload.h:	dnload.h-dist
	cp dnload.h-dist dnload.h
.endif

clean:
	rm -f dnload.h

cleandir:	clean

all:	includes

includes:	dnload.h

.include <bsd.kinc.mk>
