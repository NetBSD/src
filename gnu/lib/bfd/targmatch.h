/*	$NetBSD: targmatch.h,v 1.1.1.1.2.1 1998/11/09 08:04:28 cgd Exp $	*/

/*
 * This file was copied by hand from the result of configure with a
 * binutils-2.8.1/bfd source directory.
 */

#ifdef BFD64
#if !defined (SELECT_VECS) || defined (HAVE_ecoffalpha_little_vec)
{ "alpha-*-netware*",
&ecoffalpha_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoffalpha_little_vec)

{ "alpha-*-linuxecoff*",
&ecoffalpha_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf64_alpha_vec)

{ "alpha-*-linux*", NULL },{ "alpha-*-elf*",
&bfd_elf64_alpha_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_evax_alpha_vec)

{ "alpha-*-*vms*",
&evax_alpha_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_ecoffalpha_little_vec)

{ "alpha-*-*",
&ecoffalpha_little_vec },
#endif

    
#endif /* BFD64 */

#if !defined (SELECT_VECS) || defined (HAVE_riscix_vec)

{ "arm-*-riscix*",
&riscix_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_armpe_little_vec)

{ "arm-*-pe*",
&armpe_little_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_aout_arm_little_vec)

{ "arm-*-aout", NULL },{ "armel-*-aout",	
&aout_arm_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_aout_arm_big_vec)

{ "armeb-*-aout",
&aout_arm_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_armcoff_little_vec)

{ "arm-*-coff",
&armcoff_little_vec },
#endif



    

#if !defined (SELECT_VECS) || defined (HAVE_a29kcoff_big_vec)

{ "a29k-*-ebmon*", NULL },{ "a29k-*-udi*", NULL },{ "a29k-*-coff*", NULL },{ "a29k-*-sym1*", NULL },
{ "a29k-*-vxworks*", NULL },{ "a29k-*-sysv*",
&a29kcoff_big_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "a29k-*-aout*", NULL },{ "a29k-*-bsd*", NULL },{ "a29k-*-vsta*",
&sunos_big_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_d10v_vec)

{ "d10v-*-*",
&bfd_elf32_d10v_vec },
#endif

    


#if !defined (SELECT_VECS) || defined (HAVE_h8300coff_vec)

{ "h8300*-*-*",
&h8300coff_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_h8500coff_vec)

{ "h8500-*-*",
&h8500coff_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_hppa_vec)

{ "hppa*-*-*elf*", NULL },{ "hppa*-*-lites*", NULL },{ "hppa*-*-sysv4*", NULL },{ "hppa*-*-rtems*",
&bfd_elf32_hppa_vec },
#endif

    
#if defined (HOST_HPPAHPUX) || defined (HOST_HPPABSD) || defined (HOST_HPPAOSF)
#if !defined (SELECT_VECS) || defined (HAVE_som_vec)

{ "hppa*-*-bsd*",
&som_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_som_vec)

{ "hppa*-*-hpux*", NULL },{ "hppa*-*-hiux*",
&som_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_som_vec)

{ "hppa*-*-osf*",
&som_vec },
#endif


    
#endif /* defined (HOST_HPPAHPUX) || defined (HOST_HPPABSD) || defined (HOST_HPPAOSF) */

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i386_vec)

{ "i[3456]86-*-sysv4*", NULL },{ "i[3456]86-*-unixware", NULL },{ "i[3456]86-*-solaris2*", NULL },
{ "i[3456]86-*-elf", NULL },{ "i[3456]86-*-sco*elf*", NULL },{ "i[3456]86-*-freebsdelf*", NULL },
{ "i[3456]86-*-dgux*",
&bfd_elf32_i386_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386coff_vec)

{ "i[3456]86-*-sysv*", NULL },{ "i[3456]86-*-isc*", NULL },{ "i[3456]86-*-sco*", NULL },{ "i[3456]86-*-coff", NULL },
{ "i[3456]86-*-aix*", NULL },{ "i[3456]86-*-go32*", NULL },{ "i[3456]86*-*-rtems*",
&i386coff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_i386dynix_vec)

{ "i[3456]86-sequent-bsd*",
&i386dynix_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386bsd_vec)

{ "i[3456]86-*-bsd*",
&i386bsd_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386freebsd_vec)

{ "i[3456]86-*-freebsd*",
&i386freebsd_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_i386netbsd_vec)

{ "i[3456]86-*-netbsd*", NULL },{ "i[3456]86-*-openbsd*",
&i386netbsd_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i386_vec)

{ "i[3456]86-*-netware*",
&bfd_elf32_i386_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386linux_vec)

{ "i[3456]86-*-linux*aout*",
&i386linux_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i386_vec)

{ "i[3456]86-*-linux*",
&bfd_elf32_i386_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386lynx_coff_vec)

{ "i[3456]86-*-lynxos*",
&i386lynx_coff_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i386_vec)

{ "i[3456]86-*-gnu*",
&bfd_elf32_i386_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_i386mach3_vec)

{ "i[3456]86-*-mach*", NULL },{ "i[3456]86-*-osf1mk*",
&i386mach3_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_i386os9k_vec)

{ "i[3456]86-*-os9k",
&i386os9k_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_i386aout_vec)

{ "i[3456]86-*-msdos*",
&i386aout_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i386_vec)

{ "i[3456]86-*-moss*",
&bfd_elf32_i386_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386pe_vec)

{ "i[3456]86-*-cygwin32", NULL },{ "i[3456]86-*-winnt", NULL },{ "i[3456]86-*-pe",
&i386pe_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_i386coff_vec)

{ "i[3456]86-none-*",
&i386coff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_i386aout_vec)

{ "i[3456]86-*-aout*", NULL },{ "i[3456]86*-*-vsta*",
&i386aout_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_i860coff_vec)

{ "i860-*-mach3*", NULL },{ "i860-*-osf1*", NULL },{ "i860-*-coff*",
&i860coff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_i860_vec)

{ "i860-*-sysv4*", NULL },{ "i860-*-elf*",
&bfd_elf32_i860_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_b_out_vec_little_host)

{ "i960-*-vxworks4*", NULL },{ "i960-*-vxworks5.0",
&b_out_vec_little_host },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_icoff_little_vec)

{ "i960-*-vxworks5.*", NULL },{ "i960-*-coff*", NULL },{ "i960-*-sysv*", NULL },{ "i960-*-rtems*",
&icoff_little_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_b_out_vec_little_host)

{ "i960-*-vxworks*", NULL },{ "i960-*-aout*", NULL },{ "i960-*-bout*", NULL },{ "i960-*-nindy*",
&b_out_vec_little_host },
#endif



    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m32r_vec)

{ "m32r-*-*",
&bfd_elf32_m32r_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_apollocoff_vec)

{ "m68*-apollo-*",
&apollocoff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_m68kcoffun_vec)

{ "m68*-bull-sysv*",
&m68kcoffun_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68ksysvcoff_vec)

{ "m68*-motorola-sysv*",
&m68ksysvcoff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_hp300bsd_vec)

{ "m68*-hp-bsd*",
&hp300bsd_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_aout0_big_vec)

{ "m68*-*-aout*",
&aout0_big_vec },
#endif






    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m68k_vec)

{ "m68*-*-elf*", NULL },{ "m68*-*-sysv4*",
&bfd_elf32_m68k_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68kcoff_vec)

{ "m68*-*-coff*", NULL },{ "m68*-*-sysv*", NULL },{ "m68*-*-rtems*",
&m68kcoff_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_hp300hpux_vec)

{ "m68*-*-hpux*",
&hp300hpux_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68klinux_vec)

{ "m68*-*-linux*aout*",
&m68klinux_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m68k_vec)

{ "m68*-*-linux*",
&bfd_elf32_m68k_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68klynx_coff_vec)

{ "m68*-*-lynxos*",
&m68klynx_coff_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68k4knetbsd_vec)

{ "m68*-hp*-netbsd*",
&m68k4knetbsd_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_m68knetbsd_vec)

{ "m68*-*-netbsd*", NULL },{ "m68*-*-openbsd*",
&m68knetbsd_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "m68*-*-sunos*", NULL },{ "m68*-*-os68k*", NULL },{ "m68*-*-vxworks*", NULL },{ "m68*-netx-*", NULL },
{ "m68*-*-bsd*", NULL },{ "m68*-*-vsta*",
&sunos_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "m68*-ericsson-*",
&sunos_big_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m68k_vec)

{ "m68*-cbm-*",
&bfd_elf32_m68k_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m68kaux_coff_vec)

{ "m68*-apple-aux*",
&m68kaux_coff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m68k_vec)

{ "m68*-*-psos*",
&bfd_elf32_m68k_vec },
#endif



    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_m88k_vec)

{ "m88*-harris-cxux*", NULL },{ "m88*-*-dgux*", NULL },{ "m88*-*-sysv4*",
&bfd_elf32_m88k_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m88kmach3_vec)

{ "m88*-*-mach3*",
&m88kmach3_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_m88kbcs_vec)

{ "m88*-*-*",
&m88kbcs_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_ecoff_big_vec)

{ "mips*-big-*",
&ecoff_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_littlemips_vec)

{ "mipsel-*-netbsd*", NULL },{ "mips-dec-netbsd*",
&bfd_elf32_littlemips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mipseb-*-netbsd*",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_aout_mips_little_vec)

{ "mips*-dec-bsd*",
&aout_mips_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_aout_mips_little_vec)

{ "mips*-dec-mach3*",
&aout_mips_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoff_little_vec)

{ "mips*-dec-*", NULL },{ "mips*el-*-ecoff*",
&ecoff_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoff_big_vec)

{ "mips*-*-ecoff*",
&ecoff_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-irix6*",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-irix5*",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoff_big_vec)

{ "mips*-sgi-*", NULL },{ "mips*-*-bsd*",
&ecoff_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoff_biglittle_vec)

{ "mips*-*-lnews*",
&ecoff_biglittle_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_aout_mips_little_vec)

{ "mips*-*-mach3*",
&aout_mips_little_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-sysv4*",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_ecoff_big_vec)

{ "mips*-*-sysv*", NULL },{ "mips*-*-riscos*",
&ecoff_big_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_littlemips_vec)

{ "mips*el-*-elf*",
&bfd_elf32_littlemips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-elf*", NULL },{ "mips*-*-rtems*",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-none",
&bfd_elf32_bigmips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_littlemips_vec)

{ "mips*el*-*-linux*", NULL },{ "mips*el*-*-openbsd*",
&bfd_elf32_littlemips_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_bigmips_vec)

{ "mips*-*-linux*", NULL },{ "mips*-*-openbsd*",
&bfd_elf32_bigmips_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_mn10200_vec)

{ "mn10200-*-*",
&bfd_elf32_mn10200_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_mn10300_vec)

{ "mn10300-*-*",
&bfd_elf32_mn10300_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_pc532machaout_vec)

{ "ns32k-pc532-mach*", NULL },{ "ns32k-pc532-ux*",
&pc532machaout_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_pc532netbsd_vec)

{ "ns32k-*-netbsd*", NULL },{ "ns32k-*-lites*", NULL },{ "ns32k-*-openbsd*",
&pc532netbsd_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_rs6000coff_vec)

{ "powerpc-*-aix*", NULL },{ "powerpc-*-beos*",
&rs6000coff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_powerpc_vec)

{ "powerpc-*-*bsd*", NULL },{ "powerpc-*-elf*", NULL },{ "powerpc-*-sysv4*", NULL },{ "powerpc-*-eabi*", NULL },
{ "powerpc-*-solaris2*", NULL },{ "powerpc-*-linux*", NULL },{ "powerpc-*-rtems*",
&bfd_elf32_powerpc_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_pmac_xcoff_vec)

{ "powerpc-*-macos*", NULL },{ "powerpc-*-mpw*",
&pmac_xcoff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_powerpc_vec)

{ "powerpc-*-netware*",
&bfd_elf32_powerpc_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_powerpcle_vec)

{ "powerpcle-*-elf*", NULL },{ "powerpcle-*-sysv4*", NULL },{ "powerpcle-*-eabi*", NULL },
{ "powerpcle-*-solaris2*", NULL },{ "powerpcle-*-linux*",
&bfd_elf32_powerpcle_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_powerpcle_pe_vec)

{ "powerpcle-*-pe", NULL },{ "powerpcle-*-winnt*", NULL },{ "powerpcle-*-cygwin32",
&bfd_powerpcle_pe_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_rs6000coff_vec)

{ "rs6000-*-*",
&rs6000coff_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_sh_vec)

{ "sh-*-elf*",
&bfd_elf32_sh_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_shcoff_vec)

{ "sh-*-*",
&shcoff_vec },
#endif



    

#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "sparclet-*-aout*",
&sunos_big_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_sparclinux_vec)

{ "sparc-*-linux*aout*",
&sparclinux_vec },
#endif



    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_sparc_vec)

{ "sparc-*-linux*",
&bfd_elf32_sparc_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_sparclynx_coff_vec)

{ "sparc-*-lynxos*",
&sparclynx_coff_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_sparcnetbsd_vec)

{ "sparc-*-netbsd*", NULL },{ "sparc-*-openbsd*",
&sparcnetbsd_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_sparc_vec)

{ "sparc-*-elf*", NULL },{ "sparc-*-solaris2*",
&bfd_elf32_sparc_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_sparc_vec)

{ "sparc-*-sysv4*",
&bfd_elf32_sparc_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "sparc64-*-aout*",
&sunos_big_vec },
#endif


    
#ifdef BFD64
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf64_sparc_vec)

{ "sparc64-*-elf*",
&bfd_elf64_sparc_vec },
#endif


        
#endif /* BFD64 */
#if !defined (SELECT_VECS) || defined (HAVE_bfd_elf32_sparc_vec)

{ "sparc-*-netware*",
&bfd_elf32_sparc_vec },
#endif


    
#if !defined (SELECT_VECS) || defined (HAVE_sparccoff_vec)

{ "sparc*-*-coff*",
&sparccoff_vec },
#endif

    
#if !defined (SELECT_VECS) || defined (HAVE_sunos_big_vec)

{ "sparc*-*-*", NULL },{ "sparc*-*-rtems*",
&sunos_big_vec },
#endif


    

#if HAVE_host_aout_vec
#if !defined (SELECT_VECS) || defined (HAVE_host_aout_vec)

{ "tahoe-*-*",
&host_aout_vec },
#endif


    
#endif

#if HAVE_host_aout_vec
#if !defined (SELECT_VECS) || defined (HAVE_host_aout_vec)

{ "vax-*-bsd*", NULL },{ "vax-*-ultrix*",
&host_aout_vec },
#endif


    
#endif

#if !defined (SELECT_VECS) || defined (HAVE_we32kcoff_vec)

{ "we32k-*-*",
&we32kcoff_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_w65_vec)

{ "w65-*-*",
&w65_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_z8kcoff_vec)

{ "z8k*-*-*",
&z8kcoff_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_ieee_vec)

{ "*-*-ieee*",
&ieee_vec },
#endif

    

#if !defined (SELECT_VECS) || defined (HAVE_a_out_adobe_vec)

{ "*-adobe-*",
&a_out_adobe_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_newsos3_vec)

{ "*-sony-*",
&newsos3_vec },
#endif


    

#if !defined (SELECT_VECS) || defined (HAVE_m68kcoff_vec)

{ "*-tandem-*",
&m68kcoff_vec },
#endif


    
