dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.in)
sinclude(../common/acinclude.m4)

SIM_AC_COMMON

SIM_AC_OPTION_ENDIAN(LITTLE)
SIM_AC_OPTION_ALIGNMENT(STRICT_ALIGNMENT,STRICT_ALIGNMENT)
SIM_AC_OPTION_DEFAULT_MODEL(bf537)
SIM_AC_OPTION_WARNINGS
SIM_AC_OPTION_HARDWARE(yes,,\
	bfin_cec \
	bfin_ctimer \
	bfin_dma \
	bfin_dmac \
	bfin_ebiu_amc \
	bfin_ebiu_ddrc \
	bfin_ebiu_sdc \
	bfin_emac \
	bfin_eppi \
	bfin_evt \
	bfin_gpio \
	bfin_gpio2 \
	bfin_gptimer \
	bfin_jtag \
	bfin_mmu \
	bfin_nfc \
	bfin_otp \
	bfin_pfmon \
	bfin_pint \
	bfin_pll \
	bfin_ppi \
	bfin_rtc \
	bfin_sic \
	bfin_spi \
	bfin_trace \
	bfin_twi \
	bfin_uart \
	bfin_uart2 \
	bfin_wdog \
	bfin_wp \
	eth_phy \
)

AC_CHECK_FUNCS([getuid getgid geteuid getegid setuid setgid mmap munmap kill pread])
AC_CHECK_HEADERS([ \
	linux/if_tun.h \
	linux/mii.h \
	linux/types.h \
	net/if.h \
	sys/ioctl.h \
	sys/mman.h \
])

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(SDL, sdl, [
	AC_CHECK_LIB(dl, dlopen, [
		SDL_CFLAGS="${SDL_CFLAGS} -DHAVE_SDL"
		SDL_LIBS="-ldl"
		], [SDL_CFLAGS= SDL_LIBS=])
	], [:])
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

SIM_AC_OUTPUT
