CONF?=INSTALL
MAKE=/usr/obj/tools.pmppc/bin/nbmake-pmppc
OBJCOPY = /usr/obj/tools.pmppc/bin/powerpc--netbsd-objcopy
MDSETIMAGE = /usr/obj/tools.pmppc/bin/powerpc--netbsd-mdsetimage
NETBSD = ../compile/$(CONF)/netbsd
RAMDISK = /usr/SRC/src/distrib/pmppc/floppies/ramdisk/ramdisk.fs
BIN = /tftpboot/netbsd.bin

def: comp mdset tobin
all: conf comp mdset tobin
alldep: conf depend comp

tobin:
	cp $(NETBSD).bin $(BIN)
#	$(OBJCOPY) -O binary $(NETBSD) $(BIN)

mdset:
	$(MDSETIMAGE) -v $(NETBSD) $(RAMDISK)

comp:
	cd ../compile/$(CONF); $(MAKE) NOOBJ=y all

depend:
	cd ../compile/$(CONF); $(MAKE) NOOBJ=y depend

conf:
	config $(CONF)

art:
	$(MAKE) CONF=PMPPC conf depend comp tobin copy

artc:
	$(MAKE) CONF=PMPPC comp tobin copy

copy:
	cp $(NETBSD) $(DESTDIR)

sb:
	$(MAKE) CONF=SB conf depend comp copy

sbc:
	$(MAKE) CONF=SB conf comp

pmppc:
	$(MAKE) CONF=PMPPC conf depend comp
