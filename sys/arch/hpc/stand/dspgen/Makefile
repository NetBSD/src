# $NetBSD: Makefile,v 1.1 2001/02/09 18:34:08 uch Exp $

BUILD_ARCH	= ARM SH3 MIPS

vc5: clean
	TYPE=application sh gen_template.sh vc5 SH3 MIPS
	TYPE=static_library sh gen_template.sh vc5 SH3 MIPS

vc6: clean
	TYPE=application sh gen_template.sh vc6 $(BUILD_ARCH)
	TYPE=static_library sh gen_template.sh vc6 $(BUILD_ARCH)

evc3: clean
	TYPE=application sh gen_template.sh evc3 $(BUILD_ARCH)
	TYPE=static_library sh gen_template.sh evc3 $(BUILD_ARCH)

clean:
	rm -f application.tmpl static_library.tmpl property.* asm_build.*

distclean:	clean
