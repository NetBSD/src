
# ooh! test input command file for elftosb 2!

options {
	searchPath = "elftosb2:elftosb2/elf_test_files";
	
	maximumLoadBlock = 4K;
	sectionAlign = 4K;
	
	productVersion = "4.4.720";
	componentVersion = "4.4.999";
	
	coalesce = yes;
}

constants {
	ocram_start = 0;
	ocram_size = 256K;
	ocram_end = ocram_start + ocram_size - 1;
	
	prec_test = 1 + 5 * 10;
	paren_test = (1 + 5) * 10;
	
	a = 123;
	foo = (1 + 0x1cf1.w).w ^ 5 + a;
	
	bar1 = 0xa << 2;
	bar2 = bar1 | 0x1000;
	bar3 = bar2 & 0x5555;
	bar4 = bar3 >> 1;
	
	mod = 35 % 16;
	
	n = -1;
	x = 1 + -2;
	
	c1 = 'x';
	c2 = 'oh';
	c4 = 'shit';
	
	# test int size promotion
	x = 0xff.b;
	y = 0x1234.h;
	z = 0x55aa55aa.w;
	xx = x - (x / 2.b);	# should produce byte
	xy = x + y;			# should produce half-word
	xz = x - z;			# should produce word
	yz = y + z;			# should produce word
	xh = x.h;
	xw = x.w;
}

sources {
	hostlink = extern(0);
	redboot = extern(1);
	
	sd_player_elf="elftosb2/elf_test_files/sd_player_gcc";
	sd_player_srec="elftosb2/elf_test_files/sd_player_gcc.srec";
	
	datafile="elftosb2/elf_test_files/hello_NOR_arm";
}

#section('foo!') {
#	load 0.w > ocram_start..ocram_end;	# word fill all ocram with 0
#	
#	load hostlink;					# load all of hostlink source
#	
#	load 0x1000ffff.w > 0x1000;		# load a word to address 0x1000
#	load 0x55aa.h > 0x2000;			# load a half-word to address 0x2000
#	load redboot;					# load all sections of redboot source
#	
#	from hostlink {
#	   load $.*.text;				# load some sections to their natural location
#	   call :_start;				# call function "_start" from hostlink
#	   call hostlink:foofn;			# call function "foofn" from hostlink
#	   
#	   call :monkey (1 + 1);		# call function "monkey" from hostlink with an argument
#	   
#	   load $* > .;					# load all sections of hostlink to their natural location
#	   
#	   load $.text > 0x1000;		# load .text section to address 0x1000
#	   
#	   load 0x55.b > 0x0..0x4000;	# fill 0 through 0x4000 with byte pattern 0x55
#	}
#	
#	load $*.text from hostlink > .;	# load sections match "*.text" from hostlink to default places
#	
#	jump redboot;					# jump to entry point of redboot source
#}

## this section...
#section(2) {
#	from sd_player_elf {
#		load $* > .;
#		call :_start();
#	}
#}
#
## and this one are both equivalent except for section ids
#section(3) {
#	load $* from sd_player_elf;
#	call sd_player_elf:_start();
#}
#
#section(100) {
#    # set the value of a symbol
#    load hostlink;
#    load 0x5555.h > hostlink:g_USBPID;
#    
#    # load a string to memory
#    load "this is a string" > 0x1e0;
#}

section (32) {
	load sd_player_srec;
	call sd_player_srec;
}

#section('rsrc') <= datafile;

