# $NetBSD: Makefile,v 1.3 2001/03/13 16:31:31 uch Exp $

BUILD_ARCH	= ARM SH3 SH4 MIPS

vc5: clean
	TYPE=application sh gen_template.sh vc5 SH MIPS
	TYPE=static_library sh gen_template.sh vc5 SH MIPS

vc6: clean
	TYPE=application sh gen_template.sh vc6 $(BUILD_ARCH)
	TYPE=static_library sh gen_template.sh vc6 $(BUILD_ARCH)

evc3: clean
	TYPE=application sh gen_template.sh evc3 $(BUILD_ARCH)
	TYPE=static_library sh gen_template.sh evc3 $(BUILD_ARCH)

clean:
	rm -f application.tmpl static_library.tmpl property.* asm_build.*

distclean:	clean
