# $NetBSD: Makefile,v 1.4 2008/02/26 21:46:38 rafal Exp $

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

# No MIPS support for eVC++ 4 yet (only supports MIPS II, MIPS IV or MIPS16)
evc4: clean
	TYPE=application sh gen_template.sh evc4 ARMV4 SH3 SH4
	TYPE=static_library sh gen_template.sh evc4 ARMV4 SH3 SH4

clean:
	rm -f application.tmpl static_library.tmpl property.* asm_build.*

distclean:	clean
