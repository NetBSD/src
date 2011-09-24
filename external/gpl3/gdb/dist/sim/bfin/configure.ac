dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)dnl
AC_INIT(Makefile.in)
AC_CONFIG_HEADER(config.h:config.in)

sinclude(../common/aclocal.m4)

# Bugs in autoconf 2.59 break the call to SIM_AC_COMMON, hack around
# it by inlining the macro's contents.
sinclude(../common/common.m4)

SIM_AC_OPTION_ENDIAN(LITTLE_ENDIAN)
SIM_AC_OPTION_ALIGNMENT(STRICT_ALIGNMENT,STRICT_ALIGNMENT)
SIM_AC_OPTION_HOSTENDIAN
SIM_AC_OPTION_DEFAULT_MODEL(bf537)
SIM_AC_OPTION_ENVIRONMENT
SIM_AC_OPTION_INLINE
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
	bfin_gptimer \
	bfin_jtag \
	bfin_mmu \
	bfin_nfc \
	bfin_otp \
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
AC_CHECK_HEADERS([sys/ioctl.h sys/mman.h net/if.h linux/if_tun.h linux/mii.h])

BFIN_SIM_EXTRA_OBJS=

dnl make sure the dv-sockser code can be supported (i.e. windows)
case ${host} in
  *mingw32*) ;;
  *)
    AC_DEFINE_UNQUOTED([HAVE_DV_SOCKSER], 1, [Define if dv-sockser is usable.])
    BFIN_SIM_EXTRA_OBJS="${BFIN_SIM_EXTRA_OBJS} dv-sockser.o"
    ;;
esac

AC_SUBST([BFIN_SIM_EXTRA_OBJS], ${BFIN_SIM_EXTRA_OBJS})

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
