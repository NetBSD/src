#	$NetBSD: Makefile,v 1.1.1.1.8.1 1999/12/27 18:32:04 wrstuden Exp $

HDR=../include/platid_generated.h
MASK_HDR=../include/platid_mask.h
GENERATEDS=${HDR} ${MASK_HDR} platid_mask.c

HOST_CC ?= /usr/bin/cc

all: $(GENERATEDS)

$(GENERATEDS): platid.def platid.awk
	awk -f platid_copyright.awk  platid.def > ${HDR}
	awk -f platid_copyright.awk  platid.def > ${MASK_HDR}
	awk -f platid_copyright.awk  platid.def > platid_mask.c
	cat platid.def | \
        sed -e 's!\([{}:=?]\)! \1 !g' -e 's|\*/| */ |' -e 's|/\*| /* |' | \
        tr ' \t' '\n\n' | \
        awk -f platid.awk -v out_h=${HDR} -v out_mask_h=${MASK_HDR}

test: platid_test
	./platid_test > platid_test.out
	diff  platid_test.out  platid_test.canon

platid_test: platid.c platid_mask.c platid_test.c
	ln -sf ../include ./machine
	ln -sf ../../mips/include ./mips
	$(HOST_CC) -I. -DPLATID_TEST platid.c platid_mask.c platid_test.c -o $@

clean::
	rm -rf platid_test platid_test.out $(GENERATEDS) machine mips
