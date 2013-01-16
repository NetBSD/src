
# ooh! test input command file for elftosb 2!

options {
	coalesce = yes;

	# most elf files are GHS produced
	toolset = "GHS";

	# set versions
	productVersion = "111.222.333";
	componentVersion = "999.888.777";

	# set file flags
	flags = (1 << 0) | (1 << 1);
}

constants {
	ocram_start = 0;
	ocram_size = 256K;
	ocram_end = ocram_start + ocram_size - 1;

#	ocram = ocram_start .. ocram_end;
#
#	ocram = ocram_start +.. ocram_size;

	string_addr = 0x4500;

	# boot modes
	USB_BM = 0;
	JTAG_BM = 7;
	newBootMode = USB_BM;
}

sources {
	hello = extern(0);  # elf
	redboot = extern(1);  # srec
	hostlink = extern(2); # elf
	sd_player = extern(3) ( toolset="GCC" ); # elf
	datasrc = "test0.key"; # binary
}

section (0) {
	# load dcd
	load dcd {{ 00 11 22 33 }} > 0;

	# same load without dcd
	load {{ 00 11 22 33 }} > 0;

	call 0xf000;

    hab call 0xf0000000 (128);
	hab jump 0;

/*
	# load a simple IVT to an absolute address
	# this fills in the IVT self address field from the target address
	load ivt (entry=hello:_start) > 0x1000;

	# load simple IVT. the IVT self address is set explicitly in the IVT declaration,
	# giving the IVT a natural address so you don't have to tell where to load it.
	load ivt (entry=hello:_start, self=0x1000);

	load ivt (entry=hello:_start, self=0x1000, csf=0x2000, dcd=0);

	# Setting IVT entry point to the default entry point of a source file.
	load ivt (entry=hostlink) > 0;

	load ivt (entry=hello:_start, self=0x1000);
	hab call 0x1000;

	# Special syntax that combines the load and call into one statement.
	hab call ivt(entry=hello:_start, self=0x1000);



	load ivt (
	    entry = hello:_start,
	    csf = 0x2000
	) > 0x1000;

	# All supported IVT fields.
	load ivt (
	    entry = hello:,
	    dcd = 0,
	    boot_data = 0,
	    self = 0,
	    csf = 0
	);

	hab call ivt; # Call the last loaded IVT. */
}

section (32) {
	# load a string to some address
	load "some string" > string_addr;

	# byte fill a region
	load 0x55.b > ocram_start..ocram_end;

	from hostlink {
		load $*;
	}

	# jump to a symbol
	jump hostlink:_start (100);
}

section (100)
{
	load redboot;
	call redboot;
}

section(0x5a)
{
	load datasrc > 0..8;

	from hostlink
	{
#		load $* ( $.ocram.*, ~$.sdram.* );

#		load $.sdram.*;
#		load $.ocram.*;

		call :main;
		call :_start;
	}

#	goto 0x5b;
}

section (0x5b)
{
#	load $* from sd_player;

#	load hello$.text @ 64K;
#	load hello$*

	load $.text from hello > 0x10000;
	call sd_player (0xdeadbeef);
}

section (0x5c)
{
	# these loads should produce fill commands with a
	# length equal to the pattern word size
	load 0x55.b > 0x200;
	load 0x55aa.h > 0x400;
	load 0x55aa66bb.w > 0x800;

#	load 0x55.b to 0x200;
#	load 0x55aa.h to 0x400;
#	load 0x55aa66bb.w to 0x800;

#	load 0x55.b @ 0x200;
#	load 0x55aa.h @ 0x400;
#	load 0x55aa66bb.w @ 0x800;

#	load $.text from hello @ .;
#	load hello$.text @ .;
#	load hello$* @ .

	# this should produce a fill command with a length
	# of 0x100
	load 0x4c8e.h > 0x100..0x200;

#	load [ 0a 9b 77 66 55 44 33 22 11 00 ] @ 0x100;
}

# non-bootable section
section (0x100) <= datasrc;

#section (1K)
#{
#	load 0xff.b > hostlink:g_wUsbVID + 4;
#
#	load "Fred's Auto Service\x00" > hostlink:g_wUsbVendorName;
#
#	from sd_player
#	{
#		load [ 00 11 22 33 44 55 66 77 ] > :g_data + 16;
#	}
#
##	(0x10 .. 0x20) + 5  ==  0x15 .. 0x25
#}

# section that switches modes
section (2K)
{
	mode newBootMode;
}
