# Process this file with automake to generate Makefile.in

# As far as I can tell automake testing support assumes that the build
# system and the host system are the same.  So these tests will not
# work when building with a cross-compiler.

# Ignore warning about AM_PROG_CC_C_O due to large_CFLAGS
AUTOMAKE_OPTIONS = foreign -Wno-portability

# The two_file_test tests -fmerge-constants, so we simply always turn
# it on.  This may need to be controlled by a configure option
# eventually.
AM_CFLAGS = $(WARN_CFLAGS) $(LFS_CFLAGS) -fmerge-constants
AM_CXXFLAGS = $(WARN_CXXFLAGS) $(LFS_CFLAGS) -fmerge-constants

AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/.. -I$(srcdir)/../../include \
	-I$(srcdir)/../../elfcpp -I.. \
	-DLOCALEDIR="\"$(datadir)/locale\"" \
	@INCINTL@

TEST_READELF = $(top_builddir)/../binutils/readelf
TEST_OBJDUMP = $(top_builddir)/../binutils/objdump
TEST_CXXFILT = $(top_builddir)/../binutils/cxxfilt
TEST_STRIP = $(top_builddir)/../binutils/strip-new
TEST_AR = $(top_builddir)/../binutils/ar
TEST_NM = $(top_builddir)/../binutils/nm-new
TEST_AS = $(top_builddir)/../gas/as-new

if PLUGINS
LIBDL = -ldl
endif

if THREADS
THREADSLIB = -lpthread
endif

if OMP_SUPPORT
TLS_TEST_C_CFLAGS = -fopenmp
endif

# 'make clean' is good about deleting some intermediate files (such as
# .o's), but not all of them (such as .so's and .err files).  We
# improve on that here.  automake-1.9 info docs say "mostlyclean" is
# the right choice for files 'make' builds that people rebuild.
MOSTLYCLEANFILES = *.so *.syms *.stdout


# We will add to these later, for each individual test.  Note
# that we add each test under check_SCRIPTS or check_PROGRAMS;
# the TESTS variable is automatically populated from these.
check_SCRIPTS =
check_DATA =
check_PROGRAMS =
BUILT_SOURCES =

TESTS = $(check_SCRIPTS) $(check_PROGRAMS)

# ---------------------------------------------------------------------
# These tests test the internals of gold (unittests).

# Infrastucture needed for the unittests
check_LIBRARIES = libgoldtest.a
libgoldtest_a_SOURCES = test.cc testmain.cc testfile.cc

DEPENDENCIES = \
	libgoldtest.a ../libgold.a ../../libiberty/libiberty.a $(LIBINTL_DEP)
LDADD = libgoldtest.a ../libgold.a ../../libiberty/libiberty.a $(LIBINTL) \
	$(THREADSLIB) $(LIBDL)


# The unittests themselves
if NATIVE_OR_CROSS_LINKER
if GCC

# Infrastucture needed for the unittests: a directory where the linker
# is named 'ld'.  This is because the -B flag appends 'ld' to its arg.
gcctestdir/ld: ../ld-new
	test -d gcctestdir || mkdir -p gcctestdir
	rm -f gcctestdir/ld
	(cd gcctestdir && $(LN_S) ../../ld-new ld)

# Some tests require the latest features of an in-tree assembler.
gcctestdir/as: $(TEST_AS)
	test -d gcctestdir || mkdir -p gcctestdir
	rm -f gcctestdir/as
	(cd gcctestdir && $(LN_S) $(abs_top_builddir)/../gas/as-new as)

endif GCC

check_PROGRAMS += object_unittest
object_unittest_SOURCES = object_unittest.cc

check_PROGRAMS += binary_unittest
binary_unittest_SOURCES = binary_unittest.cc

check_PROGRAMS += leb128_unittest
leb128_unittest_SOURCES = leb128_unittest.cc

endif NATIVE_OR_CROSS_LINKER

# ---------------------------------------------------------------------
# These tests test the output of gold (end-to-end tests).  In
# particular, they make sure that gold can link "difficult" object
# files, and the resulting object files run correctly.  These can only
# run if we've built ld-new for the native architecture (that is,
# we're not cross-compiling it), since we run ld-new as part of these
# tests.  We use the gcc-specific flag '-B' to use our linker instead
# of the default linker, which is why we only run our tests under gcc.

if NATIVE_LINKER
if GCC

# Each of these .o's is a useful, small complete program.  They're
# particularly useful for making sure ld-new's flags do what they're
# supposed to (hence their names), but are used for many tests that
# don't actually involve analyzing input data.
flagstest_debug.o: constructor_test.cc
	$(CXXCOMPILE) -O0 -g -c -o $@ $<
flagstest_ndebug.o: constructor_test.cc
	$(CXXCOMPILE) -O0 -c -o $@ $<

check_SCRIPTS += incremental_test.sh
check_DATA += incremental_test.stdout
MOSTLYCLEANFILES += incremental_test
incremental_test_1.o: incremental_test_1.c
	$(COMPILE) -O0 -c -ffunction-sections -g -o $@ $<
incremental_test_2.o: incremental_test_2.c
	$(COMPILE) -O0 -c -ffunction-sections -g -o $@ $<
incremental_test: incremental_test_1.o incremental_test_2.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -Wl,--incremental-full incremental_test_1.o incremental_test_2.o -Wl,-debug 2> incremental_test.cmdline
incremental_test.stdout: incremental_test ../incremental-dump
	../incremental-dump incremental_test > $@

check_SCRIPTS += gc_comdat_test.sh
check_DATA += gc_comdat_test.stdout
MOSTLYCLEANFILES += gc_comdat_test
gc_comdat_test_1.o: gc_comdat_test_1.cc 
	$(CXXCOMPILE) -O0 -c -ffunction-sections -g -o $@ $<
gc_comdat_test_2.o: gc_comdat_test_2.cc 
	$(CXXCOMPILE) -O0 -c -ffunction-sections -g -o $@ $<
gc_comdat_test: gc_comdat_test_1.o gc_comdat_test_2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gc-sections gc_comdat_test_1.o gc_comdat_test_2.o
gc_comdat_test.stdout: gc_comdat_test
	$(TEST_NM) -C gc_comdat_test > gc_comdat_test.stdout

check_SCRIPTS += gc_tls_test.sh
check_DATA += gc_tls_test.stdout
MOSTLYCLEANFILES += gc_tls_test
gc_tls_test.o: gc_tls_test.cc
	$(CXXCOMPILE) -O0 -c -g -o $@ $<
gc_tls_test:gc_tls_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gc-sections gc_tls_test.o
gc_tls_test.stdout: gc_tls_test
	$(TEST_NM) -C gc_tls_test > gc_tls_test.stdout

check_SCRIPTS += gc_orphan_section_test.sh
check_DATA += gc_orphan_section_test.stdout
MOSTLYCLEANFILES += gc_orphan_section_test
gc_orphan_section_test.o: gc_orphan_section_test.cc
	$(CXXCOMPILE) -O0 -c -g -o $@ $<
gc_orphan_section_test:gc_orphan_section_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gc-sections gc_orphan_section_test.o
gc_orphan_section_test.stdout: gc_orphan_section_test
	$(TEST_NM) gc_orphan_section_test > gc_orphan_section_test.stdout

check_SCRIPTS += icf_test.sh
check_DATA += icf_test.stdout
MOSTLYCLEANFILES += icf_test
icf_test.o: icf_test.cc 
	$(CXXCOMPILE) -O0 -c -ffunction-sections -g -o $@ $<
icf_test: icf_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all icf_test.o
icf_test.stdout: icf_test
	$(TEST_NM) -C icf_test > icf_test.stdout

check_SCRIPTS += icf_keep_unique_test.sh
check_DATA += icf_keep_unique_test.stdout
MOSTLYCLEANFILES += icf_keep_unique_test
icf_keep_unique_test.o: icf_keep_unique_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -g -o $@ $<
icf_keep_unique_test: icf_keep_unique_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all -Wl,--keep-unique,_Z11unique_funcv icf_keep_unique_test.o
icf_keep_unique_test.stdout: icf_keep_unique_test
	$(TEST_NM) -C icf_keep_unique_test > icf_keep_unique_test.stdout

check_SCRIPTS += icf_safe_test.sh
check_DATA += icf_safe_test_1.stdout icf_safe_test_2.stdout
MOSTLYCLEANFILES += icf_safe_test
icf_safe_test.o: icf_safe_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -g -o $@ $<
icf_safe_test: icf_safe_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=safe icf_safe_test.o
icf_safe_test_1.stdout: icf_safe_test
	$(TEST_NM) icf_safe_test > icf_safe_test_1.stdout
icf_safe_test_2.stdout: icf_safe_test
	$(TEST_READELF) -h icf_safe_test > icf_safe_test_2.stdout

check_SCRIPTS += icf_safe_so_test.sh
check_DATA += icf_safe_so_test_1.stdout icf_safe_so_test_2.stdout
MOSTLYCLEANFILES += icf_safe_so_test
icf_safe_so_test.o: icf_safe_so_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIC -g -o $@ $<
icf_safe_so_test: icf_safe_so_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=safe icf_safe_so_test.o -fPIC -shared
icf_safe_so_test_1.stdout: icf_safe_so_test
	$(TEST_NM) icf_safe_so_test > icf_safe_so_test_1.stdout
icf_safe_so_test_2.stdout: icf_safe_so_test
	$(TEST_READELF) -h icf_safe_so_test > icf_safe_so_test_2.stdout

check_SCRIPTS += final_layout.sh
check_DATA += final_layout.stdout
MOSTLYCLEANFILES += final_layout
final_layout.o: final_layout.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections  -fdata-sections -g -o $@ $<
final_layout_sequence.txt:
	(echo "*_Z3barv*" && echo ".text._Z3bazv" && echo "*_Z3foov*" && echo "*global_varb*" && echo "*global_vara*" && echo "*global_varc*") > final_layout_sequence.txt
final_layout: final_layout.o final_layout_sequence.txt gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--section-ordering-file,final_layout_sequence.txt final_layout.o
final_layout.stdout: final_layout
	$(TEST_NM) -n final_layout > final_layout.stdout

check_PROGRAMS += icf_virtual_function_folding_test
MOSTLYCLEANFILES += icf_virtual_function_folding_test
icf_virtual_function_folding_test.o: icf_virtual_function_folding_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIE -g -o $@ $<
icf_virtual_function_folding_test: icf_virtual_function_folding_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all icf_virtual_function_folding_test.o -pie

check_SCRIPTS += icf_preemptible_functions_test.sh
check_DATA += icf_preemptible_functions_test.stdout
MOSTLYCLEANFILES += icf_preemptible_functions_test
icf_preemptible_functions_test.o: icf_preemptible_functions_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIC -g -o $@ $<
icf_preemptible_functions_test: icf_preemptible_functions_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all icf_preemptible_functions_test.o -fPIC -shared
icf_preemptible_functions_test.stdout: icf_preemptible_functions_test
	$(TEST_NM) icf_preemptible_functions_test > icf_preemptible_functions_test.stdout

check_SCRIPTS += icf_string_merge_test.sh
check_DATA += icf_string_merge_test.stdout
MOSTLYCLEANFILES += icf_string_merge_test
icf_string_merge_test.o: icf_string_merge_test.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIC -g -o $@ $<
icf_string_merge_test: icf_string_merge_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all icf_string_merge_test.o
icf_string_merge_test.stdout: icf_string_merge_test
	$(TEST_NM) icf_string_merge_test > icf_string_merge_test.stdout

check_SCRIPTS += icf_sht_rel_addend_test.sh
check_DATA += icf_sht_rel_addend_test.stdout
MOSTLYCLEANFILES += icf_sht_rel_addend_test
icf_sht_rel_addend_test_1.o: icf_sht_rel_addend_test_1.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIC -g -o $@ $<
icf_sht_rel_addend_test_2.o: icf_sht_rel_addend_test_2.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections -fPIC -g -o $@ $<
icf_sht_rel_addend_test: icf_sht_rel_addend_test_1.o icf_sht_rel_addend_test_2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--icf=all icf_sht_rel_addend_test_1.o icf_sht_rel_addend_test_2.o
icf_sht_rel_addend_test.stdout: icf_sht_rel_addend_test
	$(TEST_NM) icf_sht_rel_addend_test > icf_sht_rel_addend_test.stdout

check_PROGRAMS += basic_test
check_PROGRAMS += basic_pic_test
basic_test.o: basic_test.cc
	$(CXXCOMPILE) -O0 -c -o $@ $<
basic_test: basic_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ basic_test.o

if HAVE_STATIC
check_PROGRAMS += basic_static_test
basic_static_test: basic_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -static basic_test.o
endif

basic_pic_test.o: basic_test.cc
	$(CXXCOMPILE) -O0 -c -fpic -o $@ $<
basic_pic_test: basic_pic_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ basic_pic_test.o

if HAVE_STATIC
check_PROGRAMS += basic_static_pic_test
basic_static_pic_test: basic_pic_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -static basic_pic_test.o
endif

check_PROGRAMS += basic_pie_test
basic_pie_test.o: basic_test.cc
	$(CXXCOMPILE) -O0 -c -fpie -o $@ $<
basic_pie_test: basic_pie_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -pie basic_pie_test.o

check_PROGRAMS += constructor_test
constructor_test_SOURCES = constructor_test.cc
constructor_test_DEPENDENCIES = gcctestdir/ld
constructor_test_LDFLAGS = -Bgcctestdir/
constructor_test_LDADD =

if HAVE_STATIC
check_PROGRAMS += constructor_static_test
constructor_static_test_SOURCES = $(constructor_test_SOURCES)
constructor_static_test_DEPENDENCIES = $(constructor_test_DEPENDENCIES)
constructor_static_test_LDFLAGS = $(constructor_test_LDFLAGS) -static
constructor_static_test_LDADD = $(constructor_test_LDADD)
endif

check_PROGRAMS += two_file_test
check_PROGRAMS += two_file_pic_test
two_file_test_SOURCES = \
	two_file_test_1.cc \
	two_file_test_1b.cc \
	two_file_test_2.cc \
	two_file_test_main.cc \
	two_file_test.h
two_file_test_DEPENDENCIES = gcctestdir/ld
two_file_test_LDFLAGS = -Bgcctestdir/
two_file_test_LDADD =

if HAVE_STATIC
check_PROGRAMS += two_file_static_test
two_file_static_test_SOURCES = $(two_file_test_SOURCES)
two_file_static_test_DEPENDENCIES = $(two_file_test_DEPENDENCIES)
two_file_static_test_LDFLAGS = $(two_file_test_LDFLAGS) -static
two_file_static_test_LDADD = $(two_file_test_LDADD)
endif

two_file_pic_test_SOURCES = two_file_test_main.cc
two_file_pic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2_pic.o
two_file_pic_test_LDFLAGS = -Bgcctestdir/
two_file_pic_test_LDADD = two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2_pic.o


check_PROGRAMS += two_file_shared_1_test
check_PROGRAMS += two_file_shared_2_test
check_PROGRAMS += two_file_shared_1_pic_2_test
check_PROGRAMS += two_file_shared_2_pic_1_test
check_PROGRAMS += two_file_same_shared_test
check_PROGRAMS += two_file_separate_shared_12_test
check_PROGRAMS += two_file_separate_shared_21_test
two_file_test_1_pic.o: two_file_test_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
two_file_test_1b_pic.o: two_file_test_1b.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
two_file_test_2_pic.o: two_file_test_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
two_file_shared_1.so: two_file_test_1_pic.o two_file_test_1b_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1_pic.o two_file_test_1b_pic.o
two_file_shared_2.so: two_file_test_2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_2_pic.o
two_file_shared.so: two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2_pic.o

two_file_shared_1_test_SOURCES = two_file_test_2.cc two_file_test_main.cc
two_file_shared_1_test_DEPENDENCIES = gcctestdir/ld two_file_shared_1.so
two_file_shared_1_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_1_test_LDADD = two_file_shared_1.so

two_file_shared_2_test_SOURCES = two_file_test_1.cc two_file_test_1b.cc two_file_test_main.cc
two_file_shared_2_test_DEPENDENCIES = gcctestdir/ld two_file_shared_2.so
two_file_shared_2_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_2_test_LDADD = two_file_shared_2.so

two_file_shared_1_pic_2_test_SOURCES = two_file_test_main.cc
two_file_shared_1_pic_2_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_2.so two_file_test_1_pic.o two_file_test_1b_pic.o
two_file_shared_1_pic_2_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_1_pic_2_test_LDADD = two_file_test_1_pic.o two_file_test_1b_pic.o two_file_shared_2.so

two_file_shared_2_pic_1_test_SOURCES = two_file_test_main.cc
two_file_shared_2_pic_1_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_2.so two_file_test_2_pic.o
two_file_shared_2_pic_1_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_2_pic_1_test_LDADD = two_file_test_2_pic.o two_file_shared_1.so

two_file_same_shared_test_SOURCES = two_file_test_main.cc
two_file_same_shared_test_DEPENDENCIES = gcctestdir/ld two_file_shared.so
two_file_same_shared_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_same_shared_test_LDADD = two_file_shared.so

two_file_separate_shared_12_test_SOURCES = two_file_test_main.cc
two_file_separate_shared_12_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_1.so two_file_shared_2.so
two_file_separate_shared_12_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_separate_shared_12_test_LDADD = \
	two_file_shared_1.so two_file_shared_2.so

two_file_separate_shared_21_test_SOURCES = two_file_test_main.cc
two_file_separate_shared_21_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_1.so two_file_shared_2.so
two_file_separate_shared_21_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_separate_shared_21_test_LDADD = \
	two_file_shared_2.so two_file_shared_1.so

check_PROGRAMS += two_file_relocatable_test
two_file_relocatable_test_SOURCES = two_file_test_main.cc
two_file_relocatable_test_DEPENDENCIES = \
	gcctestdir/ld two_file_relocatable.o
two_file_relocatable_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_relocatable_test_LDADD = two_file_relocatable.o
two_file_relocatable.o: gcctestdir/ld two_file_test_1.o two_file_test_1b.o two_file_test_2.o
	gcctestdir/ld -r -o $@ two_file_test_1.o two_file_test_1b.o two_file_test_2.o

check_PROGRAMS += two_file_pie_test
two_file_test_1_pie.o: two_file_test_1.cc
	$(CXXCOMPILE) -c -fpie -o $@ $<
two_file_test_1b_pie.o: two_file_test_1b.cc
	$(CXXCOMPILE) -c -fpie -o $@ $<
two_file_test_2_pie.o: two_file_test_2.cc
	$(CXXCOMPILE) -c -fpie -o $@ $<
two_file_test_main_pie.o: two_file_test_main.cc
	$(CXXCOMPILE) -c -fpie -o $@ $<
two_file_pie_test: two_file_test_1_pie.o two_file_test_1b_pie.o \
		two_file_test_2_pie.o two_file_test_main_pie.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -pie two_file_test_1_pie.o two_file_test_1b_pie.o two_file_test_2_pie.o two_file_test_main_pie.o

check_SCRIPTS += two_file_shared.sh
check_DATA += two_file_shared.dbg
MOSTLYCLEANFILES += two_file_shared.dbg
two_file_shared.dbg: two_file_shared.so
	$(TEST_READELF) -w $< >$@ 2>/dev/null

# The nonpic tests will fail on platforms which can not put non-PIC
# code into shared libraries, so we just don't run them in that case.
if FN_PTRS_IN_SO_WITHOUT_PIC

check_PROGRAMS += two_file_shared_1_nonpic_test
check_PROGRAMS += two_file_shared_2_nonpic_test
check_PROGRAMS += two_file_same_shared_nonpic_test
check_PROGRAMS += two_file_separate_shared_12_nonpic_test
check_PROGRAMS += two_file_separate_shared_21_nonpic_test
check_PROGRAMS += two_file_mixed_shared_test
check_PROGRAMS += two_file_mixed_2_shared_test
two_file_shared_1_nonpic.so: two_file_test_1.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1.o two_file_test_1b.o -Wl,-z,notext
two_file_shared_2_nonpic.so: two_file_test_2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_2.o
two_file_shared_nonpic.so: two_file_test_1.o two_file_test_1b.o two_file_test_2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1.o two_file_test_1b.o two_file_test_2.o -Wl,-z,notext
two_file_shared_mixed.so: two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1_pic.o two_file_test_1b_pic.o two_file_test_2.o -Wl,-z,notext
two_file_shared_mixed_1.so: two_file_test_1.o two_file_test_1b_pic.o two_file_shared_2.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1.o two_file_test_1b_pic.o two_file_shared_2.so -Wl,-z,notext

two_file_shared_1_nonpic_test_SOURCES = \
	two_file_test_2.cc two_file_test_main.cc
two_file_shared_1_nonpic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_1_nonpic.so
two_file_shared_1_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_1_nonpic_test_LDADD = two_file_shared_1_nonpic.so

two_file_shared_2_nonpic_test_SOURCES = \
	two_file_test_1.cc two_file_test_1b.cc two_file_test_main.cc
two_file_shared_2_nonpic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_2_nonpic.so
two_file_shared_2_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_shared_2_nonpic_test_LDADD = two_file_shared_2_nonpic.so

two_file_same_shared_nonpic_test_SOURCES = two_file_test_main.cc
two_file_same_shared_nonpic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_nonpic.so
two_file_same_shared_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_same_shared_nonpic_test_LDADD = two_file_shared_nonpic.so

two_file_separate_shared_12_nonpic_test_SOURCES = two_file_test_main.cc
two_file_separate_shared_12_nonpic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_1_nonpic.so two_file_shared_2_nonpic.so
two_file_separate_shared_12_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_separate_shared_12_nonpic_test_LDADD = \
	two_file_shared_1_nonpic.so two_file_shared_2_nonpic.so

two_file_separate_shared_21_nonpic_test_SOURCES = two_file_test_main.cc
two_file_separate_shared_21_nonpic_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_1_nonpic.so two_file_shared_2_nonpic.so
two_file_separate_shared_21_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_separate_shared_21_nonpic_test_LDADD = \
	two_file_shared_2_nonpic.so two_file_shared_1_nonpic.so

two_file_mixed_shared_test_SOURCES = two_file_test_main.cc
two_file_mixed_shared_test_DEPENDENCIES = gcctestdir/ld two_file_shared_mixed.so
two_file_mixed_shared_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_mixed_shared_test_LDADD = two_file_shared_mixed.so

two_file_mixed_2_shared_test_SOURCES = two_file_test_main.cc
two_file_mixed_2_shared_test_DEPENDENCIES = gcctestdir/ld two_file_shared_mixed_1.so two_file_shared_2.so
two_file_mixed_2_shared_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
two_file_mixed_2_shared_test_LDADD = two_file_shared_mixed_1.so two_file_shared_2.so

check_PROGRAMS += two_file_mixed_pie_test
two_file_mixed_pie_test: two_file_test_1.o two_file_test_1b_pie.o \
		two_file_test_main_pie.o two_file_shared_2.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,-R,. -pie two_file_test_1.o two_file_test_1b_pie.o two_file_test_main_pie.o two_file_shared_2.so

endif FN_PTRS_IN_SO_WITHOUT_PIC

check_PROGRAMS += two_file_strip_test
two_file_strip_test: two_file_test
	$(TEST_STRIP) -o two_file_strip_test two_file_test

check_PROGRAMS += two_file_same_shared_strip_test
two_file_same_shared_strip_test_SOURCES = two_file_test_main.cc
two_file_same_shared_strip_test_DEPENDENCIES = \
	gcctestdir/ld two_file_shared_strip.so
two_file_same_shared_strip_test_LDFLAGS = -Bgcctestdir/ -Wl,-R.
two_file_same_shared_strip_test_LDADD = two_file_shared_strip.so
two_file_shared_strip.so: two_file_shared.so
	$(TEST_STRIP) -S -o two_file_shared_strip.so two_file_shared.so

check_PROGRAMS += common_test_1
common_test_1_SOURCES = common_test_1.c
common_test_1_DEPENDENCIES = gcctestdir/ld
common_test_1_LDFLAGS = -Bgcctestdir/
common_test_1_LDADD =

check_PROGRAMS += common_test_2
common_test_2_SOURCES = common_test_1.c
common_test_2_DEPENDENCIES = common_test_2.so common_test_3.so gcctestdir/ld
common_test_2_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
common_test_2_LDADD = common_test_2.so common_test_3.so
common_test_2_pic.o: common_test_2.c
	$(COMPILE) -c -fpic -o $@ $<
common_test_2.so: common_test_2_pic.o common_test_3.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared common_test_2_pic.o common_test_3.so
common_test_3_pic.o: common_test_3.c
	$(COMPILE) -c -fpic -o $@ $<
common_test_3.so: common_test_3_pic.o ver_test_2.script gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared common_test_3_pic.o -Wl,--version-script,$(srcdir)/ver_test_2.script

check_PROGRAMS += exception_test
check_PROGRAMS += exception_shared_1_test
check_PROGRAMS += exception_shared_2_test
check_PROGRAMS += exception_same_shared_test
check_PROGRAMS += exception_separate_shared_12_test
check_PROGRAMS += exception_separate_shared_21_test
exception_test_1_pic.o: exception_test_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
exception_test_2_pic.o: exception_test_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
exception_shared_1.so: exception_test_1_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared exception_test_1_pic.o
exception_shared_2.so: exception_test_2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared exception_test_2_pic.o
exception_shared.so: exception_test_1_pic.o exception_test_2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared exception_test_1_pic.o exception_test_2_pic.o

exception_test_SOURCES = \
	exception_test_main.cc \
	exception_test_1.cc \
	exception_test_2.cc \
	exception_test.h
exception_test_DEPENDENCIES = gcctestdir/ld
exception_test_LDFLAGS = -Bgcctestdir/
exception_test_LDADD =

if HAVE_STATIC
check_PROGRAMS += exception_static_test
exception_static_test_SOURCES = $(exception_test_SOURCES)
exception_static_test_DEPENDENCIES = $(exception_test_DEPENDENCIES)
exception_static_test_LDFLAGS = $(exception_test_LDFLAGS) -static
exception_static_test_LDADD = $(exception_test_LDADD)
endif

exception_shared_1_test_SOURCES = exception_test_2.cc exception_test_main.cc
exception_shared_1_test_DEPENDENCIES = gcctestdir/ld exception_shared_1.so
exception_shared_1_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
exception_shared_1_test_LDADD = exception_shared_1.so

exception_shared_2_test_SOURCES = exception_test_1.cc exception_test_main.cc
exception_shared_2_test_DEPENDENCIES = gcctestdir/ld exception_shared_2.so
exception_shared_2_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
exception_shared_2_test_LDADD = exception_shared_2.so

exception_same_shared_test_SOURCES = exception_test_main.cc
exception_same_shared_test_DEPENDENCIES = gcctestdir/ld exception_shared.so
exception_same_shared_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
exception_same_shared_test_LDADD = exception_shared.so

exception_separate_shared_12_test_SOURCES = exception_test_main.cc
exception_separate_shared_12_test_DEPENDENCIES = \
	gcctestdir/ld exception_shared_1.so exception_shared_2.so
exception_separate_shared_12_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
exception_separate_shared_12_test_LDADD = \
	exception_shared_1.so exception_shared_2.so

exception_separate_shared_21_test_SOURCES = exception_test_main.cc
exception_separate_shared_21_test_DEPENDENCIES = \
	gcctestdir/ld exception_shared_1.so exception_shared_2.so
exception_separate_shared_21_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
exception_separate_shared_21_test_LDADD = \
	exception_shared_2.so exception_shared_1.so


check_PROGRAMS += weak_test
weak_test_SOURCES = weak_test.cc
weak_test_DEPENDENCIES = gcctestdir/ld
weak_test_LDFLAGS = -Bgcctestdir/
weak_test_LDADD =

check_PROGRAMS += weak_undef_test
MOSTLYCLEANFILES += alt/weak_undef_lib.so
weak_undef_test_SOURCES = weak_undef_test.cc
weak_undef_test_DEPENDENCIES = gcctestdir/ld weak_undef_lib.so alt/weak_undef_lib.so
weak_undef_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,alt
weak_undef_test_LDADD = -L . weak_undef_lib.so
weak_undef_file1.o: weak_undef_file1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_undef_file2.o: weak_undef_file2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_undef_lib.so: weak_undef_file1.o
	$(CXXLINK) -Bgcctestdir/ -shared weak_undef_file1.o
alt/weak_undef_lib.so: weak_undef_file2.o
	test -d alt || mkdir -p alt
	$(CXXLINK) -Bgcctestdir/ -shared weak_undef_file2.o

if FN_PTRS_IN_SO_WITHOUT_PIC
check_PROGRAMS += weak_undef_nonpic_test
MOSTLYCLEANFILES += alt/weak_undef_lib_nonpic.so
weak_undef_nonpic_test_SOURCES = weak_undef_test.cc
weak_undef_nonpic_test_DEPENDENCIES = gcctestdir/ld weak_undef_lib_nonpic.so alt/weak_undef_lib_nonpic.so
weak_undef_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,alt
weak_undef_nonpic_test_LDADD = -L . weak_undef_lib_nonpic.so
weak_undef_file1_nonpic.o: weak_undef_file1.cc
	$(CXXCOMPILE) -c -o $@ $<
weak_undef_file2_nonpic.o: weak_undef_file2.cc
	$(CXXCOMPILE) -c -o $@ $<
weak_undef_lib_nonpic.so: weak_undef_file1_nonpic.o
	$(CXXLINK) -Bgcctestdir/ -shared weak_undef_file1_nonpic.o -Wl,-z,notext
alt/weak_undef_lib_nonpic.so: weak_undef_file2_nonpic.o
	test -d alt || mkdir -p alt
	$(CXXLINK) -Bgcctestdir/ -shared weak_undef_file2_nonpic.o -Wl,-z,notext
endif FN_PTRS_IN_SO_WITHOUT_PIC


check_PROGRAMS += weak_alias_test
weak_alias_test_SOURCES = weak_alias_test_main.cc
weak_alias_test_DEPENDENCIES = \
	gcctestdir/ld weak_alias_test_1.so weak_alias_test_2.so \
	weak_alias_test_3.o weak_alias_test_4.so weak_alias_test_5.so
weak_alias_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
weak_alias_test_LDADD = \
	weak_alias_test_1.so weak_alias_test_2.so weak_alias_test_3.o \
	weak_alias_test_4.so weak_alias_test_5.so
weak_alias_test_1_pic.o: weak_alias_test_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_alias_test_1.so: weak_alias_test_1_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared weak_alias_test_1_pic.o
weak_alias_test_2_pic.o: weak_alias_test_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_alias_test_2.so: weak_alias_test_2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared weak_alias_test_2_pic.o
weak_alias_test_3.o: weak_alias_test_3.cc
	$(CXXCOMPILE) -c -o $@ $<
weak_alias_test_4_pic.o: weak_alias_test_4.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_alias_test_4.so: weak_alias_test_4_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared weak_alias_test_4_pic.o
weak_alias_test_5_pic.o: weak_alias_test_5.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_alias_test_5.so: weak_alias_test_5_pic.o $(srcdir)/weak_alias_test.script gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared weak_alias_test_5_pic.o \
		-Wl,--version-script,$(srcdir)/weak_alias_test.script

check_SCRIPTS += weak_plt.sh
check_PROGRAMS += weak_plt
check_DATA += weak_plt_shared.so
weak_plt_main_pic.o: weak_plt_main.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_plt: weak_plt_main_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ weak_plt_main_pic.o
weak_plt_shared_pic.o: weak_plt_shared.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
weak_plt_shared.so: weak_plt_shared_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared weak_plt_shared_pic.o

check_PROGRAMS += copy_test
copy_test_SOURCES = copy_test.cc
copy_test_DEPENDENCIES = gcctestdir/ld copy_test_1.so copy_test_2.so
copy_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
copy_test_LDADD = copy_test_1.so copy_test_2.so
copy_test_1_pic.o: copy_test_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
copy_test_1.so: gcctestdir/ld copy_test_1_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared copy_test_1_pic.o
copy_test_2_pic.o: copy_test_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
copy_test_2.so: gcctestdir/ld copy_test_2_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared copy_test_2_pic.o

if TLS

check_PROGRAMS += tls_test
check_PROGRAMS += tls_pic_test
check_PROGRAMS += tls_pie_test
check_PROGRAMS += tls_pie_pic_test
check_PROGRAMS += tls_shared_test
check_PROGRAMS += tls_shared_ie_test
check_PROGRAMS += tls_shared_gd_to_ie_test
tls_test_pic.o: tls_test.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
tls_test_file2_pic.o: tls_test_file2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
tls_test_c_pic.o: tls_test_c.c
	$(COMPILE) -c -fpic $(TLS_TEST_C_CFLAGS) -o $@ $<
tls_test_shared.so: tls_test_pic.o tls_test_file2_pic.o tls_test_c_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test_pic.o tls_test_file2_pic.o tls_test_c_pic.o -Wl,-z,defs
tls_test_shared2.so: tls_test_file2_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test_file2_pic.o

tls_test_pic_ie.o: tls_test.cc
	$(CXXCOMPILE) -c -fpic -ftls-model=initial-exec -o $@ $<
tls_test_file2_pic_ie.o: tls_test_file2.cc
	$(CXXCOMPILE) -c -fpic -ftls-model=initial-exec -o $@ $<
tls_test_c_pic_ie.o: tls_test_c.c
	$(COMPILE) -c -fpic -ftls-model=initial-exec $(TLS_TEST_C_CFLAGS) -o $@ $<
tls_test_ie_shared.so: tls_test_pic_ie.o tls_test_file2_pic_ie.o tls_test_c_pic_ie.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test_pic_ie.o tls_test_file2_pic_ie.o tls_test_c_pic_ie.o

tls_test_SOURCES = tls_test.cc tls_test_file2.cc tls_test_main.cc tls_test.h
tls_test_DEPENDENCIES = gcctestdir/ld tls_test_c.o
tls_test_LDFLAGS = -Bgcctestdir/
tls_test_LDADD = tls_test_c.o -lpthread
tls_test_c.o: tls_test_c.c
	$(COMPILE) -c $(TLS_TEST_C_CFLAGS) -o $@ $<

tls_pic_test_SOURCES = tls_test_main.cc
tls_pic_test_DEPENDENCIES = gcctestdir/ld tls_test_pic.o tls_test_file2_pic.o \
	tls_test_c_pic.o
tls_pic_test_LDFLAGS = -Bgcctestdir/
tls_pic_test_LDADD = tls_test_pic.o tls_test_file2_pic.o tls_test_c_pic.o \
	-lpthread

tls_test_main_pie.o: tls_test_main.cc tls_test.h
	$(CXXCOMPILE) -c -fpie -o $@ $<
tls_test_pie.o: tls_test.cc tls_test.h
	$(CXXCOMPILE) -c -fpie -o $@ $<
tls_test_file2_pie.o: tls_test_file2.cc tls_test.h
	$(CXXCOMPILE) -c -fpie -o $@ $<
tls_test_c_pie.o: tls_test_c.c
	$(COMPILE) -c -fpic $(TLS_TEST_C_CFLAGS) -o $@ $<
tls_pie_test: tls_test_main_pie.o tls_test_pie.o tls_test_file2_pie.o \
		tls_test_c_pie.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -pie tls_test_main_pie.o tls_test_pie.o tls_test_file2_pie.o tls_test_c_pie.o -lpthread

tls_pie_pic_test: tls_test_main_pie.o tls_test_pic.o tls_test_file2_pic.o \
		tls_test_c_pic.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -pie tls_test_main_pie.o tls_test_pic.o tls_test_file2_pic.o tls_test_c_pic.o -lpthread

tls_shared_test_SOURCES = tls_test_main.cc
tls_shared_test_DEPENDENCIES = gcctestdir/ld tls_test_shared.so
tls_shared_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_test_LDADD = tls_test_shared.so -lpthread

tls_shared_ie_test_SOURCES = tls_test_main.cc
tls_shared_ie_test_DEPENDENCIES = gcctestdir/ld tls_test_ie_shared.so
tls_shared_ie_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_ie_test_LDADD = tls_test_ie_shared.so -lpthread

tls_shared_gd_to_ie_test_SOURCES = tls_test_main.cc
tls_shared_gd_to_ie_test_DEPENDENCIES = gcctestdir/ld tls_test_pic.o \
	tls_test_c_pic.o tls_test_shared2.so
tls_shared_gd_to_ie_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_gd_to_ie_test_LDADD = tls_test_pic.o tls_test_c_pic.o \
	tls_test_shared2.so -lpthread

if TLS_GNU2_DIALECT

check_PROGRAMS += tls_shared_gnu2_gd_to_ie_test

tls_test_gnu2.o: tls_test.cc
	$(CXXCOMPILE) -c -fpic -mtls-dialect=gnu2 -o $@ $<
tls_test_file2_gnu2.o: tls_test_file2.cc
	$(CXXCOMPILE) -c -fpic -mtls-dialect=gnu2 -o $@ $<
tls_test_c_gnu2.o: tls_test_c.c
	$(COMPILE) -c -fpic -mtls-dialect=gnu2 $(TLS_TEST_C_CFLAGS) -o $@ $<
tls_test_gnu2_shared2.so: tls_test_file2_gnu2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test_file2_gnu2.o

tls_shared_gnu2_gd_to_ie_test_SOURCES = tls_test_main.cc
tls_shared_gnu2_gd_to_ie_test_DEPENDENCIES = gcctestdir/ld tls_test_gnu2.o \
	tls_test_c_gnu2.o tls_test_gnu2_shared2.so
tls_shared_gnu2_gd_to_ie_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_gnu2_gd_to_ie_test_LDADD = tls_test_gnu2.o tls_test_c_gnu2.o \
	tls_test_gnu2_shared2.so -lpthread

if TLS_DESCRIPTORS

check_PROGRAMS += tls_shared_gnu2_test

tls_test_gnu2_shared.so: tls_test_gnu2.o tls_test_file2_gnu2.o tls_test_c_gnu2.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test_gnu2.o tls_test_file2_gnu2.o tls_test_c_gnu2.o

tls_shared_gnu2_test_SOURCES = tls_test_main.cc
tls_shared_gnu2_test_DEPENDENCIES = gcctestdir/ld tls_test_gnu2_shared.so
tls_shared_gnu2_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_gnu2_test_LDADD = tls_test_gnu2_shared.so -lpthread

endif TLS_DESCRIPTORS

endif TLS_GNU2_DIALECT

if HAVE_STATIC
if STATIC_TLS
check_PROGRAMS += tls_static_test
check_PROGRAMS += tls_static_pic_test

tls_static_test_SOURCES = $(tls_test_SOURCES)
tls_static_test_DEPENDENCIES = $(tls_test_DEPENDENCIES)
tls_static_test_LDFLAGS = $(tls_test_LDFLAGS) -static
tls_static_test_LDADD = $(tls_test_LDADD)

tls_static_pic_test_SOURCES = $(tls_pic_test_SOURCES)
tls_static_pic_test_DEPENDENCIES = $(tls_pic_test_DEPENDENCIES)
tls_static_pic_test_LDFLAGS = $(tls_pic_test_LDFLAGS) -static
tls_static_pic_test_LDADD = $(tls_pic_test_LDADD)
endif
endif

if FN_PTRS_IN_SO_WITHOUT_PIC
check_PROGRAMS += tls_shared_nonpic_test
tls_test_shared_nonpic.so: tls_test.o tls_test_file2.o tls_test_c.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared tls_test.o tls_test_file2.o tls_test_c.o -Wl,-z,notext

tls_shared_nonpic_test_SOURCES = tls_test_main.cc
tls_shared_nonpic_test_DEPENDENCIES = gcctestdir/ld tls_test_shared_nonpic.so
tls_shared_nonpic_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
tls_shared_nonpic_test_LDADD = tls_test_shared_nonpic.so -lpthread
endif FN_PTRS_IN_SO_WITHOUT_PIC

endif TLS

check_PROGRAMS += many_sections_test
many_sections_test_SOURCES = many_sections_test.cc
many_sections_test_DEPENDENCIES = gcctestdir/ld
many_sections_test_LDFLAGS = -Bgcctestdir/ -rdynamic
many_sections_test_LDADD =

BUILT_SOURCES += many_sections_define.h
MOSTLYCLEANFILES += many_sections_define.h
many_sections_define.h:
	(for i in `seq 1 70000`; do \
	   echo "int var_$$i __attribute__((section(\"section_$$i\"))) = $$i;"; \
	 done) > $@.tmp
	mv -f $@.tmp $@

BUILT_SOURCES += many_sections_check.h
MOSTLYCLEANFILES += many_sections_check.h
many_sections_check.h:
	(for i in `seq 1 1000 70000`; do \
	   echo "assert(var_$$i == $$i);"; \
	 done) > $@.tmp
	mv -f $@.tmp $@

check_PROGRAMS += many_sections_r_test
many_sections_r_test.o: many_sections_test.o gcctestdir/ld
	gcctestdir/ld -r -o $@ many_sections_test.o
many_sections_r_test: many_sections_r_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ many_sections_r_test.o $(LIBS)

check_PROGRAMS += initpri1
initpri1_SOURCES = initpri1.c
initpri1_DEPENDENCIES = gcctestdir/ld
initpri1_LDFLAGS = -Bgcctestdir/
initpri1_LDADD =

check_PROGRAMS += initpri2
initpri2_SOURCES = initpri2.c
initpri2_DEPENDENCIES = gcctestdir/ld
initpri2_LDFLAGS = -Bgcctestdir/ -Wl,--ctors-in-init-array
initpri2_LDADD =

check_PROGRAMS += initpri3a
initpri3a_SOURCES = initpri3.c
initpri3a_DEPENDENCIES = gcctestdir/ld
initpri3a_LDFLAGS = -Bgcctestdir/
initpri3a_LDADD =

# This test fails on targets not using .ctors and .dtors sections (e.g. ARM
# EABI). Given that gcc is moving towards using .init_array in all cases,
# this test is commented out.  A better fix would be checking whether gcc
# uses .ctors or .init_array sections in configure.

# check_PROGRAMS += initpri3b
# initpri3b_SOURCES = initpri3.c
# initpri3b_DEPENDENCIES = gcctestdir/ld
# initpri3b_LDFLAGS = -Bgcctestdir/ -Wl,--no-ctors-in-init-array
# initpri3b_LDADD =

# Test --detect-odr-violations
check_SCRIPTS += debug_msg.sh

# Create the data files that debug_msg.sh analyzes.
check_DATA += debug_msg.err
MOSTLYCLEANFILES += debug_msg.err
debug_msg.o: debug_msg.cc
	$(CXXCOMPILE) -O0 -g -c -w -o $@ $(srcdir)/debug_msg.cc
odr_violation1.o: odr_violation1.cc
	$(CXXCOMPILE) -O0 -g -c -w -o $@ $(srcdir)/odr_violation1.cc
# Compile with different optimization flags to check that rearranged
# instructions don't cause a false positive.
odr_violation2.o: odr_violation2.cc
	$(CXXCOMPILE) -O2 -g -c -w -o $@ $(srcdir)/odr_violation2.cc
debug_msg.err: debug_msg.o odr_violation1.o odr_violation2.o gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg debug_msg.o odr_violation1.o odr_violation2.o "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg debug_msg.o odr_violation1.o odr_violation2.o 2>$@; \
	then \
	  echo 1>&2 "Link of debug_msg should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi


if HAVE_ZLIB

# Check that --detect-odr-violations works with compressed debug sections.
check_DATA += debug_msg_cdebug.err
MOSTLYCLEANFILES += debug_msg_cdebug.err
debug_msg_cdebug.o: debug_msg.cc gcctestdir/as
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g -Wa,--compress-debug-sections -c -w -o $@ $(srcdir)/debug_msg.cc
odr_violation1_cdebug.o: odr_violation1.cc gcctestdir/as
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g -Wa,--compress-debug-sections -c -w -o $@ $(srcdir)/odr_violation1.cc
odr_violation2_cdebug.o: odr_violation2.cc gcctestdir/as
	$(CXXCOMPILE) -Bgcctestdir/ -O2 -g -Wa,--compress-debug-sections -c -w -o $@ $(srcdir)/odr_violation2.cc
debug_msg_cdebug.err: debug_msg_cdebug.o odr_violation1_cdebug.o odr_violation2_cdebug.o gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_cdebug debug_msg_cdebug.o odr_violation1_cdebug.o odr_violation2_cdebug.o "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_cdebug debug_msg_cdebug.o odr_violation1_cdebug.o odr_violation2_cdebug.o 2>$@; \
	then \
	  echo 1>&2 "Link of debug_msg_cdebug should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi

endif HAVE_ZLIB

# See if we can also detect problems when we're linking .so's, not .o's.
check_DATA += debug_msg_so.err
MOSTLYCLEANFILES += debug_msg_so.err
debug_msg.so: debug_msg.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g -shared -fPIC -w -o $@ $(srcdir)/debug_msg.cc
odr_violation1.so: odr_violation1.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g -shared -fPIC -w -o $@ $(srcdir)/odr_violation1.cc
odr_violation2.so: odr_violation2.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O2 -g -shared -fPIC -w -o $@ $(srcdir)/odr_violation2.cc
debug_msg_so.err: debug_msg.so odr_violation1.so odr_violation2.so gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_so debug_msg.so odr_violation1.so odr_violation2.so "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_so debug_msg.so odr_violation1.so odr_violation2.so 2>$@; \
	then \
	  echo 1>&2 "Link of debug_msg_so should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi

# We also want to make sure we do something reasonable when there's no
# debug info available.  For the best test, we use .so's.
check_DATA += debug_msg_ndebug.err
MOSTLYCLEANFILES += debug_msg_ndebug.err
debug_msg_ndebug.so: debug_msg.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g0 -shared -fPIC -w -o $@ $(srcdir)/debug_msg.cc
odr_violation1_ndebug.so: odr_violation1.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g0 -shared -fPIC -w -o $@ $(srcdir)/odr_violation1.cc
odr_violation2_ndebug.so: odr_violation2.cc gcctestdir/ld
	$(CXXCOMPILE) -Bgcctestdir/ -O2 -g0 -shared -fPIC -w -o $@ $(srcdir)/odr_violation2.cc
debug_msg_ndebug.err: debug_msg_ndebug.so odr_violation1_ndebug.so odr_violation2_ndebug.so gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_ndebug debug_msg_ndebug.so odr_violation1_ndebug.so odr_violation2_ndebug.so "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -Wl,--detect-odr-violations -o debug_msg_ndebug debug_msg_ndebug.so odr_violation1_ndebug.so odr_violation2_ndebug.so 2>$@; \
	then \
	  echo 1>&2 "Link of debug_msg_ndebug should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi


# Similar to --detect-odr-violations: check for undefined symbols in .so's
check_SCRIPTS += undef_symbol.sh
check_DATA += undef_symbol.err
MOSTLYCLEANFILES += undef_symbol.err
undef_symbol.o: undef_symbol.cc
	$(CXXCOMPILE) -O0 -g -c -fPIC $<
undef_symbol.so: undef_symbol.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared undef_symbol.o
undef_symbol.err: undef_symbol_main.o undef_symbol.so gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -o undef_symbol_test undef_symbol_main.o undef_symbol.so "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -o undef_symbol_test undef_symbol_main.o undef_symbol.so 2>$@; \
	then \
	  echo 1>&2 "Link of undef_symbol_test should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi


# Test -o when emitting to a special file (such as something in /dev).
check_PROGRAMS += flagstest_o_specialfile
flagstest_o_specialfile: flagstest_debug.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -o /dev/stdout $< 2>&1 | cat > $@
	chmod a+x $@
	test -s $@

if HAVE_ZLIB

# Test --compress-debug-sections.  FIXME: check we actually compress.
check_PROGRAMS += flagstest_compress_debug_sections
flagstest_compress_debug_sections: flagstest_debug.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -o $@ $< -Wl,--compress-debug-sections=zlib
	test -s $@


# The specialfile output has a tricky case when we also compress debug
# sections, because it requires output-file resizing.
check_PROGRAMS += flagstest_o_specialfile_and_compress_debug_sections
flagstest_o_specialfile_and_compress_debug_sections: flagstest_debug.o \
		gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -o /dev/stdout $< -Wl,--compress-debug-sections=zlib 2>&1 | cat > $@
	chmod a+x $@
	test -s $@

endif HAVE_ZLIB

# Test -TText and -Tdata.
check_PROGRAMS += flagstest_o_ttext_1
flagstest_o_ttext_1: flagstest_debug.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -o $@ $< -Wl,-Ttext,0x400000 -Wl,-Tdata,0x800000

# This version won't be runnable, because there is no way to put the
# PT_PHDR segment at file offset 0.  We just make sure that we can
# build it without error.
check_DATA += flagstest_o_ttext_2
flagstest_o_ttext_2: flagstest_debug.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -o $@ $< -Wl,-Ttext,0x400010 -Wl,-Tdata,0x800010

# Test symbol versioning.
check_PROGRAMS += ver_test
ver_test_SOURCES = ver_test_main.cc
ver_test_DEPENDENCIES = gcctestdir/ld ver_test_1.so ver_test_2.so ver_test_4.so
ver_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_LDADD = ver_test_1.so ver_test_2.so ver_test_4.so
ver_test_1.so: ver_test_1.o ver_test_2.so ver_test_3.o ver_test_4.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared ver_test_1.o ver_test_2.so ver_test_3.o ver_test_4.so
ver_test_2.so: ver_test_2.o $(srcdir)/ver_test_2.script ver_test_4.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_2.script -Wl,-R,. ver_test_2.o ver_test_4.so
ver_test_4.so: ver_test_4.o $(srcdir)/ver_test_4.script gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_4.script ver_test_4.o
ver_test_1.o: ver_test_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
ver_test_2.o: ver_test_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
ver_test_3.o: ver_test_3.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
ver_test_4.o: ver_test_4.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<

check_SCRIPTS += ver_test_1.sh
check_DATA += ver_test_1.syms
ver_test_1.syms: ver_test_1.so
	$(TEST_READELF) -s $< >$@ 2>/dev/null

check_PROGRAMS += ver_test_2
ver_test_2_SOURCES = ver_test_main_2.cc
ver_test_2_DEPENDENCIES = gcctestdir/ld ver_test_4.so ver_test_2.so
ver_test_2_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_2_LDADD = ver_test_4.so ver_test_2.so

check_SCRIPTS += ver_test_2.sh
check_DATA += ver_test_2.syms
ver_test_2.syms: ver_test_2
	$(TEST_READELF) -s $< >$@ 2>/dev/null

check_SCRIPTS += ver_test_4.sh
check_DATA += ver_test_4.syms
ver_test_4.syms: ver_test_4.so
	$(TEST_READELF) -s $< >$@ 2>/dev/null

ver_test_5.so: ver_test_5.o $(srcdir)/ver_test_5.script ver_test_4.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_5.script ver_test_5.o ver_test_4.so
ver_test_5.o: ver_test_5.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
check_SCRIPTS += ver_test_5.sh
check_DATA += ver_test_5.syms
ver_test_5.syms: ver_test_5.so
	$(TEST_READELF) -s $< >$@ 2>/dev/null

check_PROGRAMS += ver_test_6
ver_test_6_SOURCES = ver_test_6.c
ver_test_6_DEPENDENCIES = gcctestdir/ld ver_test_2.so
ver_test_6_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_6_LDADD = ver_test_2.so

ver_test_7.so: ver_test_4.o $(srcdir)/ver_test_4.script ver_test_7.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_4.script ver_test_4.o ver_test_7.o
ver_test_7.o: ver_test_7.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
check_SCRIPTS += ver_test_7.sh
check_DATA += ver_test_7.syms
ver_test_7.syms: ver_test_7.so
	$(TEST_READELF) -s $< >$@ 2>/dev/null

check_PROGRAMS += ver_test_8
ver_test_8_SOURCES = two_file_test_main.cc
ver_test_8_DEPENDENCIES = gcctestdir/ld ver_test_8_1.so ver_test_8_2.so
ver_test_8_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_8_LDADD = ver_test_8_1.so ver_test_8_2.so
ver_test_8_1.so: two_file_test_1_pic.o two_file_test_1b_pic.o ver_test_8_2.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared two_file_test_1_pic.o two_file_test_1b_pic.o ver_test_8_2.so
ver_test_8_2.so: two_file_test_2_pic.o $(srcdir)/ver_test_8.script gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_8.script two_file_test_2_pic.o

check_PROGRAMS += ver_test_9
ver_test_9_SOURCES = ver_test_main.cc
ver_test_9_DEPENDENCIES = gcctestdir/ld ver_test_9.so
ver_test_9_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_9_LDADD = ver_test_9.so
ver_test_9.so: ver_test_9.o ver_test_4.so ver_test_5.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,-R,. ver_test_9.o ver_test_5.so ver_test_4.so
ver_test_9.o: ver_test_9.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<

check_SCRIPTS += ver_test_10.sh
check_DATA += ver_test_10.syms
ver_test_10.syms: ver_test_10.so
	$(TEST_READELF) -s $< >$@ 2>/dev/null
ver_test_10.so: gcctestdir/ld ver_test_2.o ver_test_10.script
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,--version-script,$(srcdir)/ver_test_10.script ver_test_2.o

check_PROGRAMS += ver_test_11
MOSTLYCLEANFILES += ver_test_11.a
ver_test_11_SOURCES = ver_test_main_2.cc
ver_test_11_DEPENDENCIES = gcctestdir/ld ver_test_11.a
ver_test_11_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_11_LDADD = ver_test_11.a
ver_test_11.a: ver_test_1.o ver_test_2.o ver_test_4.o
	$(TEST_AR) rc $@ $^

check_PROGRAMS += ver_test_12
ver_test_12_SOURCES = ver_test_main_2.cc
ver_test_12_DEPENDENCIES = gcctestdir/ld ver_test_12.o
ver_test_12_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ver_test_12_LDADD = ver_test_12.o
ver_test_12.o: gcctestdir/ld ver_test_1.o ver_test_2.o ver_test_4.o
	gcctestdir/ld -r -o $@ ver_test_1.o ver_test_2.o ver_test_4.o

check_PROGRAMS += protected_1
protected_1_SOURCES = \
	protected_main_1.cc protected_main_2.cc protected_main_3.cc
protected_1_DEPENDENCIES = gcctestdir/ld protected_1.so
protected_1_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
protected_1_LDADD = protected_1.so

protected_1.so: gcctestdir/ld protected_1_pic.o protected_2_pic.o protected_3_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared protected_1_pic.o protected_2_pic.o protected_3_pic.o
protected_1_pic.o: protected_1.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
protected_2_pic.o: protected_2.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
protected_3_pic.o: protected_3.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<

check_PROGRAMS += protected_2
protected_2_SOURCES = protected_main_1.cc protected_3.cc
protected_2_DEPENDENCIES = gcctestdir/ld protected_1.so
protected_2_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
protected_2_LDADD = protected_1.so

check_DATA += protected_3.err
MOSTLYCLEANFILES += protected_3.err
protected_4_pic.o: protected_4.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
protected_3.err: protected_4_pic.o gcctestdir/ld
	@echo $(CXXLINK) -Bgcctestdir/ -shared -o protected_4.so protected_4_pic.o "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -shared -o protected_4.so protected_4_pic.o 2>$@; then \
	  echo 1>&2 "Link of protected_4.so should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi

check_PROGRAMS += relro_test
check_SCRIPTS += relro_test.sh
check_DATA += relro_test.stdout
relro_test_SOURCES = relro_test_main.cc
relro_test_DEPENDENCIES = gcctestdir/ld relro_test.so
relro_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
relro_test_LDADD = relro_test.so
relro_test.so: gcctestdir/ld relro_test_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,-z,relro relro_test_pic.o
relro_test_pic.o: relro_test.cc
	$(CXXCOMPILE) -c -fpic -o $@ $<
relro_test.stdout: relro_test.so
	$(TEST_READELF) -SlW relro_test.so > relro_test.stdout

check_PROGRAMS += relro_now_test
relro_now_test_SOURCES = relro_test_main.cc
relro_now_test_DEPENDENCIES = gcctestdir/ld relro_now_test.so
relro_now_test_LDFLAGS = -Bgcctestdir -Wl,-R,. -Wl,-z,relro -Wl,-z,now
relro_now_test_LDADD = relro_now_test.so
relro_now_test.so: gcctestdir/ld relro_test_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,-z,relro -Wl,-z,now relro_test_pic.o

check_PROGRAMS += relro_strip_test
relro_strip_test_SOURCES = relro_test_main.cc
relro_strip_test_DEPENDENCIES = gcctestdir/ld relro_strip_test.so
relro_strip_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
relro_strip_test_LDADD = relro_strip_test.so
relro_strip_test.so: relro_test.so
	$(TEST_STRIP) -o $@ $<

check_PROGRAMS += relro_script_test
relro_script_test_SOURCES = relro_test_main.cc
relro_script_test_DEPENDENCIES = gcctestdir/ld relro_script_test.so
relro_script_test_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
relro_script_test_LDADD = relro_script_test.so
relro_script_test.so: gcctestdir/ld relro_script_test.t relro_test_pic.o
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,-z,relro -T $(srcdir)/relro_script_test.t relro_test_pic.o

check_PROGRAMS += script_test_1
script_test_1_SOURCES = script_test_1.cc
script_test_1_DEPENDENCIES = gcctestdir/ld script_test_1.t
script_test_1_LDFLAGS = -Bgcctestdir/ -Wl,-R,. -T $(srcdir)/script_test_1.t
script_test_1_LDADD =

check_PROGRAMS += script_test_2
script_test_2_SOURCES = script_test_2.cc script_test_2a.cc script_test_2b.cc
script_test_2_DEPENDENCIES = gcctestdir/ld script_test_2.t
script_test_2_LDFLAGS = -Bgcctestdir/ -Wl,-R,. -T $(srcdir)/script_test_2.t
script_test_2_LDADD =

check_PROGRAMS += justsyms
justsyms_SOURCES = justsyms_1.cc
justsyms_DEPENDENCIES = gcctestdir/ld justsyms_2r.o
justsyms_LDFLAGS = -Bgcctestdir/ -Wl,-R,justsyms_2r.o
justsyms_LDADD =
justsyms_2.o: justsyms_2.cc
	$(CXXCOMPILE) -c -o $@ $<
justsyms_2r.o: justsyms_2.o gcctestdir/ld $(srcdir)/justsyms.t
	gcctestdir/ld -o $@ -r -T $(srcdir)/justsyms.t justsyms_2.o

check_PROGRAMS += justsyms_exec
justsyms_exec_SOURCES = justsyms_exec.c
justsyms_exec_DEPENDENCIES = gcctestdir/ld justsyms_lib
justsyms_exec_LDFLAGS = -Bgcctestdir/ -Wl,-R,justsyms_lib
justsyms_exec_LDADD =
justsyms_lib.o: justsyms_lib.c
	$(COMPILE) -c -o $@ $<
justsyms_lib: justsyms_lib.o gcctestdir/ld
	gcctestdir/ld -o $@ -Ttext=0x1000200 -Tdata=0x2000000 -e exported_func justsyms_lib.o

check_PROGRAMS += binary_test
MOSTLYCLEANFILES += binary.txt
binary_test_SOURCES = binary_test.cc
binary_test_DEPENDENCIES = gcctestdir/ld binary.txt
binary_test_LDFLAGS = -Bgcctestdir/ -Wl,--format,binary,binary.txt,--format,elf
binary_test_LDADD =
# Copy the file to the build directory to avoid worrying about the
# full pathname in the generated symbols.
binary.txt: $(srcdir)/binary.in
	rm -f $@
	$(LN_S) $< $@

check_SCRIPTS += ver_matching_test.sh
check_DATA += ver_matching_test.stdout
MOSTLYCLEANFILES += ver_matching_test.stdout
ver_matching_def.so: ver_matching_def_pic.o $(srcdir)/version_script.map gcctestdir/ld
	$(CXXLINK) -O0 -Bgcctestdir/ -shared ver_matching_def_pic.o -Wl,--version-script=$(srcdir)/version_script.map
ver_matching_def_pic.o: ver_matching_def.cc
	$(CXXCOMPILE) -O0 -c -fpic -o $@ $<
ver_matching_test.stdout: ver_matching_def.so
	$(TEST_OBJDUMP) -T ver_matching_def.so | $(TEST_CXXFILT) > ver_matching_test.stdout

check_PROGRAMS += script_test_3
check_SCRIPTS += script_test_3.sh
check_DATA += script_test_3.stdout
MOSTLYCLEANFILES += script_test_3.stdout
script_test_3: basic_test.o gcctestdir/ld script_test_3.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o -T $(srcdir)/script_test_3.t
script_test_3.stdout: script_test_3
	$(TEST_READELF) -SlW script_test_3 > script_test_3.stdout

check_PROGRAMS += tls_phdrs_script_test
tls_phdrs_script_test_SOURCES = $(tls_test_SOURCES)
tls_phdrs_script_test_DEPENDENCIES = $(tls_test_DEPENDENCIES) $(srcdir)/script_test_3.t
tls_phdrs_script_test_LDFLAGS = $(tls_test_LDFLAGS) -T $(srcdir)/script_test_3.t
tls_phdrs_script_test_LDADD = $(tls_test_LDADD)

check_SCRIPTS += script_test_4.sh
check_DATA += script_test_4.stdout
MOSTLYCLEANFILES += script_test_4
script_test_4: basic_test.o gcctestdir/ld $(srcdir)/script_test_4.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o -T $(srcdir)/script_test_4.t
script_test_4.stdout: script_test_4
	$(TEST_READELF) -SlW script_test_4 > script_test_4.stdout

check_PROGRAMS += tls_script_test
tls_script_test_SOURCES = $(tls_test_SOURCES)
tls_script_test_DEPENDENCIES = $(tls_test_DEPENDENCIES) $(srcdir)/script_test_4.t
tls_script_test_LDFLAGS = $(tls_test_LDFLAGS) -T $(srcdir)/script_test_4.t
tls_script_test_LDADD = $(tls_test_LDADD)

check_SCRIPTS += script_test_5.sh
check_DATA += script_test_5.stdout
MOSTLYCLEANFILES += script_test_5
script_test_5: script_test_5.o gcctestdir/ld $(srcdir)/script_test_5.t
	$(CXXLINK) -Bgcctestdir/ script_test_5.o -T $(srcdir)/script_test_5.t
script_test_5.stdout: script_test_5
	$(TEST_READELF) -SW script_test_5 > script_test_5.stdout

check_SCRIPTS += script_test_6.sh
check_DATA += script_test_6.stdout
MOSTLYCLEANFILES += script_test_6
script_test_6: basic_test.o gcctestdir/ld $(srcdir)/script_test_6.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o -T $(srcdir)/script_test_6.t \
	-Wl,-Ttext=0x10001000 -Wl,-Tdata=0x10200000 -Wl,-Tbss=0x10400000
script_test_6.stdout: script_test_6
	$(TEST_READELF) -SlW script_test_6 > script_test_6.stdout

check_SCRIPTS += script_test_7.sh
check_DATA += script_test_7.stdout
MOSTLYCLEANFILES += script_test_7
script_test_7: basic_test.o gcctestdir/ld $(srcdir)/script_test_7.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o -T $(srcdir)/script_test_7.t
script_test_7.stdout: script_test_7
	$(TEST_READELF) -SlW script_test_7 > script_test_7.stdout

check_SCRIPTS += script_test_8.sh
check_DATA += script_test_8.stdout
MOSTLYCLEANFILES += script_test_8
script_test_8: basic_test.o gcctestdir/ld $(srcdir)/script_test_7.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o -T $(srcdir)/script_test_7.t \
	-Wl,-Ttext=0x20001000 -Wl,-Tdata=0x20200000 -Wl,-Tbss=0x20400000
script_test_8.stdout: script_test_8
	$(TEST_READELF) -SlW script_test_8 > script_test_8.stdout


check_SCRIPTS += script_test_9.sh
check_DATA += script_test_9.stdout
script_test_9.o: script_test_9.cc
	$(CXXCOMPILE) -O0 -c -o $@ $<
script_test_9: gcctestdir/ld $(srcdir)/script_test_9.t script_test_9.o
	$(CXXLINK) -Bgcctestdir/ script_test_9.o -T $(srcdir)/script_test_9.t
script_test_9.stdout: script_test_9
	$(TEST_READELF) -lW script_test_9 > script_test_9.stdout


# Test --dynamic-list, --dynamic-list-data, --dynamic-list-cpp-new,
# and --dynamic-list-cpp-typeinfo

check_SCRIPTS += dynamic_list.sh
check_DATA += dynamic_list.stdout
MOSTLYCLEANFILES += dynamic_list dynamic_list.stdout
dynamic_list: basic_test.o gcctestdir/ld $(srcdir)/dynamic_list.t
	$(CXXLINK) -Bgcctestdir/ basic_test.o \
	  -Wl,--dynamic-list $(srcdir)/dynamic_list.t \
	  -Wl,--dynamic-list-data \
	  -Wl,--dynamic-list-cpp-new \
	  -Wl,--dynamic-list-cpp-typeinfo
dynamic_list.stdout: dynamic_list
	$(TEST_READELF) -W --dyn-syms dynamic_list > dynamic_list.stdout

check_PROGRAMS += thin_archive_test_1
MOSTLYCLEANFILES += libthin1.a libthin3.a libthinall.a \
	alt/thin_archive_test_2.o alt/thin_archive_test_4.o \
	alt/libthin2.a alt/libthin4.a
thin_archive_test_1_SOURCES = thin_archive_main.cc
thin_archive_test_1_DEPENDENCIES = gcctestdir/ld libthin1.a alt/libthin2.a
thin_archive_test_1_LDFLAGS = -Bgcctestdir/ -Lalt
thin_archive_test_1_LDADD = libthin1.a -lthin2

check_PROGRAMS += thin_archive_test_2
thin_archive_test_2_SOURCES = thin_archive_main.cc
thin_archive_test_2_DEPENDENCIES = gcctestdir/ld libthinall.a
thin_archive_test_2_LDFLAGS = -Bgcctestdir/ -L.
thin_archive_test_2_LDADD = -lthinall

libthin1.a: thin_archive_test_1.o alt/thin_archive_test_2.o
	rm -f $@
	$(TEST_AR) crT $@ $^
alt/libthin2.a: thin_archive_test_3.o alt/thin_archive_test_4.o
	rm -f $@
	$(TEST_AR) crT $@ $^
libthin3.a: thin_archive_test_1.o alt/thin_archive_test_4.o
	rm -f $@
	$(TEST_AR) crT $@ $^
alt/libthin4.a: alt/thin_archive_test_2.o thin_archive_test_3.o
	rm -f $@
	$(TEST_AR) crT $@ $^
libthinall.a: libthin3.a alt/libthin4.a
	rm -f $@
	$(TEST_AR) crT $@ $^
alt/thin_archive_test_2.o: thin_archive_test_2.cc
	test -d alt || mkdir -p alt
	$(CXXCOMPILE) -c -o $@ $<
alt/thin_archive_test_4.o: thin_archive_test_4.cc
	test -d alt || mkdir -p alt
	$(CXXCOMPILE) -c -o $@ $<

if PLUGINS

check_PROGRAMS += plugin_test_1
check_SCRIPTS += plugin_test_1.sh
check_DATA += plugin_test_1.err
MOSTLYCLEANFILES += plugin_test_1.err
plugin_test_1: two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms empty.syms gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle,--plugin,"./plugin_test.so",--plugin-opt,"_Z4f13iv" two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms empty.syms 2>plugin_test_1.err
plugin_test_1.err: plugin_test_1
	@touch plugin_test_1.err

check_PROGRAMS += plugin_test_2
check_SCRIPTS += plugin_test_2.sh
check_DATA += plugin_test_2.err
MOSTLYCLEANFILES += plugin_test_2.err
plugin_test_2: two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_shared_2.so gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle,-R,.,--plugin,"./plugin_test.so" two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_shared_2.so 2>plugin_test_2.err
plugin_test_2.err: plugin_test_2
	@touch plugin_test_2.err

check_PROGRAMS += plugin_test_3
check_SCRIPTS += plugin_test_3.sh
check_DATA += plugin_test_3.err
MOSTLYCLEANFILES += plugin_test_3.err
plugin_test_3: two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms empty.syms gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--export-dynamic -Wl,--no-demangle,--plugin,"./plugin_test.so",--plugin-opt,"_Z4f13iv" two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms empty.syms 2>plugin_test_3.err
plugin_test_3.err: plugin_test_3
	@touch plugin_test_3.err

check_PROGRAMS += plugin_test_4
check_SCRIPTS += plugin_test_4.sh
check_DATA += plugin_test_4.err
MOSTLYCLEANFILES += plugin_test_4.a plugin_test_4.err
plugin_test_4: two_file_test_main.o plugin_test_4.a gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle,--plugin,"./plugin_test.so",--plugin-opt,"_Z4f13iv" two_file_test_main.o -Wl,--whole-archive,plugin_test_4.a,--no-whole-archive 2>plugin_test_4.err
plugin_test_4.err: plugin_test_4
	@touch plugin_test_4.err

plugin_test_4.a: two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms
	$(TEST_AR) cr $@ $^

check_PROGRAMS += plugin_test_5
plugin_test_5: two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms unused.syms gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle,--plugin,"./plugin_test.so",--plugin-opt,"_Z4f13iv",--gc-sections two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.syms unused.syms

check_PROGRAMS += plugin_test_6
check_SCRIPTS += plugin_test_6.sh
check_DATA += plugin_test_6.err
MOSTLYCLEANFILES += plugin_test_6.err
plugin_test_6: plugin_common_test_1.syms plugin_common_test_2.syms gcctestdir/ld plugin_test.so
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle,--plugin,"./plugin_test.so" plugin_common_test_1.syms plugin_common_test_2.syms 2>plugin_test_6.err
plugin_test_6.err: plugin_test_6
	@touch plugin_test_6.err

check_PROGRAMS += plugin_test_7
check_SCRIPTS += plugin_test_7.sh
check_DATA += plugin_test_7.err plugin_test_7.syms
MOSTLYCLEANFILES += plugin_test_7.err
plugin_test_7: plugin_test_7_1.o plugin_test_7_1.syms plugin_test_7_2.o gcctestdir/ld plugin_test.so
	$(LINK) -Bgcctestdir/ -Wl,--no-demangle,--plugin,"./plugin_test.so",--gc-sections,--print-gc-sections plugin_test_7_1.syms plugin_test_7_2.o 2>plugin_test_7.err
plugin_test_7.syms: plugin_test_7
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
plugin_test_7_1.o: plugin_test_7_1.c
	$(COMPILE) -DLTO -O0 -c -ffunction-sections -fdata-sections -o $@ $<
plugin_test_7_1_orig.o: plugin_test_7_1.c
	$(COMPILE) -O0 -c -ffunction-sections -fdata-sections -o $@ $<
plugin_test_7_1.syms: plugin_test_7_1_orig.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
plugin_test_7_2.o: plugin_test_7_2.c
	$(COMPILE) -O0 -c -ffunction-sections -fdata-sections -o $@ $<
plugin_test_7.err: plugin_test_7

# Test plugins with -r.
check_PROGRAMS += plugin_test_8
plugin_test_8.o: two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.o ../ld-new plugin_test.so
	../ld-new -r -o $@ --no-demangle --plugin "./plugin_test.so" two_file_test_main.o two_file_test_1.syms two_file_test_1b.syms two_file_test_2.o
plugin_test_8: plugin_test_8.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--no-demangle plugin_test_8.o

# Test that symbols known in the IR file but not in the replacement file
# produce an unresolved symbol error.
check_DATA += plugin_test_9.err
MOSTLYCLEANFILES += plugin_test_9.err
plugin_test_9.err: two_file_test_main.o two_file_test_1c.syms two_file_test_2.syms gcctestdir/ld plugin_test.so
	@echo $(CXXLINK) -Bgcctestdir/ -o plugin_test_9 -Wl,--no-demangle,--plugin,"./plugin_test.so" two_file_test_main.o two_file_test_1c.syms two_file_test_2.syms "2>$@"
	@if $(CXXLINK) -Bgcctestdir/ -o plugin_test_9 -Wl,--no-demangle,--plugin,"./plugin_test.so" two_file_test_main.o two_file_test_1c.syms two_file_test_2.syms 2>$@; then \
	  echo 1>&2 "Link of plugin_test_9 should have failed"; \
	  rm -f $@; \
	  exit 1; \
	fi
# Make a .syms file that claims to define the symbol _Z4t16av.
two_file_test_1c.syms: two_file_test_1.syms two_file_test_1c.o
	cp two_file_test_1.syms $@.tmp
	grep "_Z4t16av" two_file_test_1b.syms >> $@.tmp
	mv -f $@.tmp $@
# Make a copy of two_file_test_1.o, which does not define the symbol _Z4t16av.
MOSTLYCLEANFILES += two_file_test_1c.o
two_file_test_1c.o: two_file_test_1.o
	cp two_file_test_1.o $@

plugin_test.so: plugin_test.o
	$(LINK) -Bgcctestdir/ -shared plugin_test.o
plugin_test.o: plugin_test.c
	$(COMPILE) -O0 -c -fpic -o $@ $<

two_file_test_main.syms: two_file_test_main.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
two_file_test_1.syms: two_file_test_1.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
two_file_test_1b.syms: two_file_test_1b.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
two_file_test_2.syms: two_file_test_2.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
plugin_common_test_1.syms: plugin_common_test_1.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
plugin_common_test_2.syms: plugin_common_test_2.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null

empty.syms:
	@echo "" >$@
	@echo "Symbol table" >>$@

MOSTLYCLEANFILES += unused.c
unused.syms: unused.o
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
	@echo "     1: 00000000     4 FUNC    GLOBAL DEFAULT    1 UNUSED" >>$@
unused.o: unused.c
	$(COMPILE) -c -o $@ $<
unused.c:
	@cp /dev/null $@

check_SCRIPTS += plugin_final_layout.sh
check_DATA += plugin_final_layout.stdout
plugin_final_layout.o: plugin_final_layout.cc
	$(CXXCOMPILE) -O0 -c -ffunction-sections  -fdata-sections -g -o $@ $<
plugin_final_layout: plugin_final_layout.o plugin_section_order.so gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--plugin,"./plugin_section_order.so" plugin_final_layout.o
plugin_final_layout.stdout: plugin_final_layout
	$(TEST_NM) -n plugin_final_layout > plugin_final_layout.stdout

plugin_section_order.so: plugin_section_order.o
	$(LINK) -Bgcctestdir/ -shared plugin_section_order.o
plugin_section_order.o: plugin_section_order.c
	$(COMPILE) -O0 -c -fpic -o $@ $<

endif PLUGINS

check_PROGRAMS += exclude_libs_test
check_SCRIPTS += exclude_libs_test.sh
check_DATA += exclude_libs_test.syms
MOSTLYCLEANFILES += exclude_libs_test.syms libexclude_libs_test_1.a \
        libexclude_libs_test_2.a alt/libexclude_libs_test_3.a
exclude_libs_test_SOURCES = exclude_libs_test.c
exclude_libs_test_DEPENDENCIES = gcctestdir/ld libexclude_libs_test_1.a \
	libexclude_libs_test_2.a alt/libexclude_libs_test_3.a
exclude_libs_test_LDFLAGS = -Bgcctestdir/ -L. -Lalt \
	-Wl,--exclude-libs,dummy:libexclude_libs_test_1 \
	-Wl,--exclude-libs,libexclude_libs_test_3
exclude_libs_test_LDADD = -lexclude_libs_test_1 -lexclude_libs_test_2 \
	alt/libexclude_libs_test_3.a
exclude_libs_test.syms: exclude_libs_test
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
libexclude_libs_test_1.a: exclude_libs_test_1.o
	$(TEST_AR) rc $@ $^
libexclude_libs_test_2.a: exclude_libs_test_2.o
	$(TEST_AR) rc $@ $^
alt/libexclude_libs_test_3.a: exclude_libs_test_3.o
	test -d alt || mkdir -p alt
	$(TEST_AR) rc $@ $^

check_PROGRAMS += local_labels_test
local_labels_test.o: ver_test_6.c
	$(COMPILE) -g -c -Wa,-L -o $@ $<
local_labels_test: local_labels_test.o
	$(LINK) -Bgcctestdir/ local_labels_test.o

check_PROGRAMS += discard_locals_test
check_SCRIPTS += discard_locals_test.sh
check_DATA += discard_locals_test.syms \
	discard_locals_relocatable_test1.syms \
	discard_locals_relocatable_test2.syms
MOSTLYCLEANFILES += discard_locals_test.syms \
	discard_locals_relocatable_test1.syms \
	discard_locals_relocatable_test2.syms \
	discard_locals_relocatable_test1.out \
	discard_locals_relocatable_test2.out
discard_locals_test_SOURCES = discard_locals_test.c
discard_locals_test_LDFLAGS = -Bgcctestdir/ -Wl,--discard-locals
discard_locals_test.syms: discard_locals_test
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
# '-Wa,-L' is required to preserve the local label used for testing.
discard_locals_test.o: discard_locals_test.c
	$(COMPILE) -c -Wa,-L -o $@ $<

discard_locals_relocatable_test1.syms: discard_locals_relocatable_test1.out
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
discard_locals_relocatable_test.o: discard_locals_relocatable_test.c
	$(COMPILE) -c -Wa,-L -fPIC -o $@ $<
discard_locals_relocatable_test1.out: discard_locals_relocatable_test.o ../ld-new
	../ld-new --discard-locals -relocatable -o $@ $<

discard_locals_relocatable_test2.syms: discard_locals_relocatable_test2.out
	$(TEST_READELF) -sW $< >$@ 2>/dev/null
discard_locals_relocatable_test2.out: discard_locals_relocatable_test.o ../ld-new
	../ld-new --discard-all -relocatable -o $@ $<

if MCMODEL_MEDIUM
check_PROGRAMS += large
large_SOURCES = large.c
large_CFLAGS = -mcmodel=medium
large_DEPENDENCIES = gcctestdir/ld
large_LDFLAGS = -Bgcctestdir/
large_LDADD =
endif MCMODEL_MEDIUM

# Test that hidden and internal symbols in the main program cannot be
# referenced by a shared library.
check_SCRIPTS += hidden_test.sh
check_DATA += hidden_test.err
MOSTLYCLEANFILES += hidden_test hidden_test.err
libhidden.so: hidden_test_1.c gcctestdir/ld
	$(COMPILE) -Bgcctestdir/ -g -shared -fPIC -w -o $@ $(srcdir)/hidden_test_1.c
hidden_test: hidden_test_main.o libhidden.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -Wl,-R,. hidden_test_main.o libhidden.so 2>hidden_test.err
hidden_test.err: hidden_test
	@touch hidden_test.err

# Test -retain-symbols-file.
check_SCRIPTS += retain_symbols_file_test.sh
check_DATA += retain_symbols_file_test.stdout
MOSTLYCLEANFILES += retain_symbols_file_test retain_symbols_file_test.in \
                    retain_symbols_file_test.stdout
retain_symbols_file_test.so: basic_pic_test.o gcctestdir/ld
	echo 'main' > retain_symbols_file_test.in
	echo 't1' >> retain_symbols_file_test.in
	echo '_ZN4t16bC1Ev' >> retain_symbols_file_test.in
	echo '_ZNK4t20a3getEv' >> retain_symbols_file_test.in
	echo '_Z3t18v' >> retain_symbols_file_test.in
	echo '__tcf_0' >> retain_symbols_file_test.in	
	$(CXXLINK) -Bgcctestdir/ -shared -Wl,-retain-symbols-file,retain_symbols_file_test.in basic_pic_test.o
retain_symbols_file_test.stdout: retain_symbols_file_test.so
	$(TEST_NM) -C retain_symbols_file_test.so > $@


# Test that if the output file already exists and is empty,
# it will get execute permission.
check_PROGRAMS += permission_test
permission_test: basic_test.o gcctestdir/ld
	umask 022; \
	rm -f $@; \
	touch $@; \
	chmod 600 $@; \
	$(CXXLINK) -Bgcctestdir/ basic_test.o

# Check -l:foo.a
check_PROGRAMS += searched_file_test
MOSTLYCLEANFILES += searched_file_test searched_file_test_lib.o \
                    alt/searched_file_test_lib.a
searched_file_test_SOURCES = searched_file_test.cc
searched_file_test_DEPENDENCIES = alt/searched_file_test_lib.a
searched_file_test_LDFLAGS = -Bgcctestdir/ -Lalt
searched_file_test_LDADD = -l:searched_file_test_lib.a
searched_file_test_lib.o: searched_file_test_lib.cc
	$(CXXCOMPILE) -c -o $@ $<
alt/searched_file_test_lib.a: searched_file_test_lib.o
	test -d alt || mkdir -p alt
	$(TEST_AR) rc $@ $^

# Test that no .gnu.version sections are created when 
# symbol versioning is not used.
check_SCRIPTS += no_version_test.sh
check_DATA += no_version_test.stdout
MOSTLYCLEANFILES += libno_version_test.so no_version_test.stdout
# We invoke the linker directly since gcc may include additional objects that
# uses symbol versioning.
libno_version_test.so: no_version_test.o gcctestdir/ld
	gcctestdir/ld -shared -o $@ no_version_test.o
no_version_test.o: no_version_test.c
	$(COMPILE) -o $@ -c -fPIC $<
no_version_test.stdout: libno_version_test.so
	$(TEST_OBJDUMP) -h $< > $@

# Test STT_GNU_IFUNC symbols.
if IFUNC

ifuncmod1.o: ifuncmod1.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmod1.so: ifuncmod1.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared ifuncmod1.o

ifuncdep1.o: ifuncmod1.c
	$(COMPILE) -c -o $@ $<

ifuncmain1pic.o: ifuncmain1.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmain1pie.o: ifuncmain1.c
	$(COMPILE) -c -fPIE -o $@ $<

if HAVE_STATIC
if IFUNC_STATIC
check_PROGRAMS += ifuncmain1static
ifuncmain1static_SOURCES = ifuncmain1.c
ifuncmain1static_DEPENDENCIES = gcctestdir/ld ifuncdep1.o
ifuncmain1static_LDFLAGS = -Bgcctestdir/ -static
ifuncmain1static_LDADD = ifuncdep1.o

check_PROGRAMS += ifuncmain1picstatic
ifuncmain1picstatic: ifuncmain1pic.o ifuncmod1.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -static ifuncmain1pic.o ifuncmod1.o
endif
endif

check_PROGRAMS += ifuncmain1
ifuncmain1_SOURCES = ifuncmain1.c
ifuncmain1_DEPENDENCIES = gcctestdir/ld ifuncmod1.so
ifuncmain1_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ifuncmain1_LDADD = ifuncmod1.so

check_PROGRAMS += ifuncmain1pic
ifuncmain1pic: ifuncmain1pic.o ifuncmod1.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain1pic.o ifuncmod1.so -Wl,-R,.

check_PROGRAMS += ifuncmain1vis
ifuncmain1vis_SOURCES = ifuncmain1vis.c
ifuncmain1vis_DEPENDENCIES = gcctestdir/ld ifuncmod1.so
ifuncmain1vis_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ifuncmain1vis_LDADD = ifuncmod1.so

check_PROGRAMS += ifuncmain1vispic
ifuncmain1vispic.o: ifuncmain1vis.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmain1vispic: ifuncmain1vispic.o ifuncmod1.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain1pic.o ifuncmod1.so -Wl,-R,.

check_PROGRAMS += ifuncmain1staticpic
ifuncmain1staticpic: ifuncmain1pic.o ifuncmod1.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain1pic.o ifuncmod1.o

check_PROGRAMS += ifuncmain1pie
ifuncmain1pie: ifuncmain1pie.o ifuncmod1.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain1pie.o ifuncmod1.so -Wl,-R,.

check_PROGRAMS += ifuncmain1vispie
ifuncmain1vispie.o: ifuncmain1vis.c
	$(COMPILE) -c -fPIE -o $@ $<
ifuncmain1vispie: ifuncmain1vispie.o ifuncmod1.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain1vispie.o ifuncmod1.so -Wl,-R,.

check_PROGRAMS += ifuncmain1staticpie
ifuncmain1staticpie: ifuncmain1pie.o ifuncmod1.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain1pie.o ifuncmod1.o

ifuncmain2pic.o: ifuncmain2.c
	$(COMPILE) -c -fPIC -o $@ $<

ifuncdep2pic.o: ifuncdep2.c
	$(COMPILE) -c -fPIC -o $@ $<

if HAVE_STATIC
if IFUNC_STATIC
check_PROGRAMS += ifuncmain2static
ifuncmain2static_SOURCES = ifuncmain2.c ifuncdep2.c
ifuncmain2static_DEPENDENCIES = gcctestdir/ld
ifuncmain2static_LDFLAGS = -Bgcctestdir/ -static
ifuncmain2static_LDADD =

check_PROGRAMS += ifuncmain2picstatic
ifuncmain2picstatic: ifuncmain2pic.o ifuncdep2pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -static ifuncmain2pic.o ifuncdep2pic.o
endif
endif

check_PROGRAMS += ifuncmain2
ifuncmain2_SOURCES = ifuncmain2.c ifuncdep2.c
ifuncmain2_DEPENDENCIES = gcctestdir/ld
ifuncmain2_LDFLAGS = -Bgcctestdir/
ifuncmain2_LDADD =

check_PROGRAMS += ifuncmain2pic
ifuncmain2pic: ifuncmain2pic.o ifuncdep2pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain2pic.o ifuncdep2pic.o

ifuncmod3.o: ifuncmod3.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmod3.so: ifuncmod3.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared ifuncmod3.o

check_PROGRAMS += ifuncmain3
ifuncmain3_SOURCES = ifuncmain3.c
ifuncmain3_DEPENDENCIES = gcctestdir/ld ifuncmod3.so
ifuncmain3_LDFLAGS = -Bgcctestdir/ -Wl,--export-dynamic -Wl,-R,.
ifuncmain3_LDADD = -ldl

ifuncmain4pic.o: ifuncmain4.c
	$(COMPILE) -c -fPIC -o $@ $<

if HAVE_STATIC
if IFUNC_STATIC
check_PROGRAMS += ifuncmain4static
ifuncmain4static_SOURCES = ifuncmain4.c
ifuncmain4static_DEPENDENCIES = gcctestdir/ld
ifuncmain4static_LDFLAGS = -Bgcctestdir/ -static
ifuncmain4static_LDADD =

check_PROGRAMS += ifuncmain4picstatic
ifuncmain4picstatic: ifuncmain4pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -static ifuncmain4pic.o
endif
endif

check_PROGRAMS += ifuncmain4
ifuncmain4_SOURCES = ifuncmain4.c
ifuncmain4_DEPENDENCIES = gcctestdir/ld
ifuncmain4_LDFLAGS = -Bgcctestdir/
ifuncmain4_LDADD =

ifuncmain5pic.o: ifuncmain5.c
	$(COMPILE) -c -fPIC -o $@ $<

ifuncmain5pie.o: ifuncmain5.c
	$(COMPILE) -c -fPIE -o $@ $<

ifuncmod5.o: ifuncmod5.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmod5.so: ifuncmod5.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared ifuncmod5.o

ifuncdep5.o: ifuncmod5.c
	$(COMPILE) -c -o $@ $<

if HAVE_STATIC
if IFUNC_STATIC
check_PROGRAMS += ifuncmain5static
ifuncmain5static_SOURCES = ifuncmain5.c
ifuncmain5static_DEPENDENCIES = gcctestdir/ld ifuncdep5.o
ifuncmain5static_LDFLAGS = -Bgcctestdir/ -static
ifuncmain5static_LDADD = ifuncdep5.o

check_PROGRAMS += ifuncmain5picstatic
ifuncmain5picstatic: ifuncmain5pic.o ifuncmod5.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -static ifuncmain5pic.o ifuncmod5.o
endif
endif

check_PROGRAMS += ifuncmain5
ifuncmain5_SOURCES = ifuncmain5.c
ifuncmain5_DEPENDENCIES = gcctestdir/ld ifuncmod5.so
ifuncmain5_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ifuncmain5_LDADD = ifuncmod5.so

check_PROGRAMS += ifuncmain5pic
ifuncmain5pic: ifuncmain5pic.o ifuncmod5.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain5pic.o ifuncmod5.so -Wl,-R,.

check_PROGRAMS += ifuncmain5staticpic
ifuncmain5staticpic: ifuncmain5pic.o ifuncmod5.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain5pic.o ifuncmod5.o

check_PROGRAMS += ifuncmain5pie
ifuncmain5pie: ifuncmain5pie.o ifuncmod5.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain5pie.o ifuncmod5.so -Wl,-R,.

ifuncmain6pie.o: ifuncmain6pie.c
	$(COMPILE) -c -fPIE -o $@ $<

ifuncmod6.o: ifuncmod6.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncmod6.so: ifuncmod6.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared ifuncmod6.o

check_PROGRAMS += ifuncmain6pie
ifuncmain6pie: ifuncmain6pie.o ifuncmod6.so gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain6pie.o ifuncmod6.so -Wl,-R,.

ifuncmain7pic.o: ifuncmain7.c
	$(COMPILE) -c -fPIC -o $@ $<

ifuncmain7pie.o: ifuncmain7.c
	$(COMPILE) -c -fPIE -o $@ $<

if HAVE_STATIC
if IFUNC_STATIC
check_PROGRAMS += ifuncmain7static
ifuncmain7static_SOURCES = ifuncmain7.c
ifuncmain7static_DEPENDENCIES = gcctestdir/ld
ifuncmain7static_LDFLAGS = -Bgcctestdir/ -static
ifuncmain7static_LDADD =

check_PROGRAMS += ifuncmain7picstatic
ifuncmain7picstatic: ifuncmain7pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -static ifuncmain7pic.o
endif
endif

check_PROGRAMS += ifuncmain7
ifuncmain7_SOURCES = ifuncmain7.c
ifuncmain7_DEPENDENCIES = gcctestdir/ld
ifuncmain7_LDFLAGS = -Bgcctestdir/
ifuncmain7_LDADD =

check_PROGRAMS += ifuncmain7pic
ifuncmain7pic: ifuncmain7pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ ifuncmain7pic.o

check_PROGRAMS += ifuncmain7pie
ifuncmain7pie: ifuncmain7pie.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -pie ifuncmain7pie.o

check_PROGRAMS += ifuncvar
ifuncvar1_pic.o: ifuncvar1.c
	$(COMPILE) -c -fPIC -o $@ $<
ifuncvar2_pic.o: ifuncvar2.c	
	$(COMPILE) -c -fPIC -o $@ $<
ifuncvar.so: ifuncvar1_pic.o ifuncvar2_pic.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -shared ifuncvar1_pic.o ifuncvar2_pic.o
ifuncvar_SOURCES = ifuncvar3.c
ifuncvar_DEPENDENCIES = gcctestdir/ld ifuncvar.so
ifuncvar_LDFLAGS = -Bgcctestdir/ -Wl,-R,.
ifuncvar_LDADD = ifuncvar.so

endif IFUNC

# Test that strong reference to a weak symbol in a DSO remains strong.
check_SCRIPTS += strong_ref_weak_def.sh
check_DATA += strong_ref_weak_def.stdout
MOSTLYCLEANFILES += strong_ref_weak_def_1.so strong_ref_weak_def_2.so \
	strong_ref_weak_def.stdout
strong_ref_weak_def_2.o: strong_ref_weak_def_2.c
	$(COMPILE) -o $@ -c -fPIC $<
strong_ref_weak_def_2.so: strong_ref_weak_def_2.o gcctestdir/ld
	gcctestdir/ld -shared -o $@ strong_ref_weak_def_2.o
strong_ref_weak_def_1.o: strong_ref_weak_def_1.c
	$(COMPILE) -o $@ -c -fPIC $<
strong_ref_weak_def_1.so: strong_ref_weak_def_1.o strong_ref_weak_def_2.so \
	gcctestdir/ld
	gcctestdir/ld -shared -o $@ strong_ref_weak_def_1.o \
		strong_ref_weak_def_2.so
strong_ref_weak_def.stdout: strong_ref_weak_def_1.so
	$(TEST_READELF) -sWD $< > $@

# Test that a strong weak reference remains strong if there is another
# weak reference in a DSO.
check_SCRIPTS += dyn_weak_ref.sh
check_DATA += dyn_weak_ref.stdout
MOSTLYCLEANFILES += dyn_weak_ref_1.so dyn_weak_ref_2.so \
	dyn_weak_ref.stdout
dyn_weak_ref_2.o: dyn_weak_ref_2.c
	$(COMPILE) -o $@ -c -fPIC $<
dyn_weak_ref_2.so: dyn_weak_ref_2.o gcctestdir/ld
	gcctestdir/ld -shared -o $@ dyn_weak_ref_2.o
dyn_weak_ref_1.o: dyn_weak_ref_1.c
	$(COMPILE) -o $@ -c -fPIC $<
# We intentionally put dyn_weak_ref_2.so in front of dyn_weak_ref_1.o
# so that the weak ref there goes to gold's symbol table first.
dyn_weak_ref_1.so: dyn_weak_ref_1.o dyn_weak_ref_2.so gcctestdir/ld
	gcctestdir/ld -shared -o $@ dyn_weak_ref_2.so dyn_weak_ref_1.o
dyn_weak_ref.stdout: dyn_weak_ref_1.so
	$(TEST_READELF) -sWD $< > $@


# Test that --start-lib and --end-lib function correctly.
check_PROGRAMS += start_lib_test
start_lib_test: start_lib_test_main.o libstart_lib_test.a start_lib_test_2.o start_lib_test_3.o \
	gcctestdir/ld
	$(LINK) -Bgcctestdir/ -o $@ start_lib_test_main.o -L. -lstart_lib_test \
		-Wl,--start-lib start_lib_test_2.o start_lib_test_3.o -Wl,--end-lib
libstart_lib_test.a: start_lib_test_1.o
	$(TEST_AR) rc $@ $^

# Test that MEMORY region support works.
check_SCRIPTS += memory_test.sh
check_DATA += memory_test.stdout
MOSTLYCLEANFILES += memory_test.stdout memory_test memory_test.o
memory_test.o: memory_test.s
	$(COMPILE) -o $@ -c $<
memory_test: memory_test.o gcctestdir/ld $(srcdir)/memory_test.t
	$(LINK) -Bgcctestdir/ -nostartfiles -nostdlib -T $(srcdir)/memory_test.t -o $@ memory_test.o
memory_test.stdout: memory_test
	$(TEST_READELF) -lWS  $< > $@

if HAVE_PUBNAMES

# Test that --gdb-index functions correctly without gcc-generated pubnames.
check_SCRIPTS += gdb_index_test_1.sh
check_DATA += gdb_index_test_1.stdout
MOSTLYCLEANFILES += gdb_index_test_1.stdout gdb_index_test_1
gdb_index_test.o: gdb_index_test.cc
	$(CXXCOMPILE) -O0 -g -gno-pubnames -c -o $@ $<
gdb_index_test_1: gdb_index_test.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gdb-index $<
gdb_index_test_1.stdout: gdb_index_test_1
	$(TEST_READELF) --debug-dump=gdb_index $< > $@

if HAVE_ZLIB

# Test that --gdb-index functions correctly with compressed debug sections.
check_SCRIPTS += gdb_index_test_2.sh
check_DATA += gdb_index_test_2.stdout
MOSTLYCLEANFILES += gdb_index_test_2.stdout gdb_index_test_2
gdb_index_test_cdebug.o: gdb_index_test.cc
	$(CXXCOMPILE) -Bgcctestdir/ -O0 -g -Wa,--compress-debug-sections -c -o $@ $<
gdb_index_test_2: gdb_index_test_cdebug.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gdb-index $<
gdb_index_test_2.stdout: gdb_index_test_2
	$(TEST_READELF) --debug-dump=gdb_index $< > $@

endif HAVE_ZLIB

# Another simple C test (DW_AT_high_pc encoding) for --gdb-index.
check_SCRIPTS += gdb_index_test_3.sh
check_DATA += gdb_index_test_3.stdout
MOSTLYCLEANFILES += gdb_index_test_3.stdout gdb_index_test_3
gdb_index_test_3.o: gdb_index_test_3.c
	$(COMPILE) -O0 -g -c -o $@ $<
gdb_index_test_3: gdb_index_test_3.o gcctestdir/ld
	$(LINK) -Bgcctestdir/ -Wl,--gdb-index,--fatal-warnings $<
gdb_index_test_3.stdout: gdb_index_test_3
	$(TEST_READELF) --debug-dump=gdb_index $< > $@

# Test that --gdb-index functions correctly with gcc-generated pubnames.
check_SCRIPTS += gdb_index_test_4.sh
check_DATA += gdb_index_test_4.stdout
MOSTLYCLEANFILES += gdb_index_test_4.stdout gdb_index_test_4
gdb_index_test_pub.o: gdb_index_test.cc
	$(CXXCOMPILE) -O0 -g -gpubnames -c -o $@ $<
gdb_index_test_4: gdb_index_test_pub.o gcctestdir/ld
	$(CXXLINK) -Bgcctestdir/ -Wl,--gdb-index $<
gdb_index_test_4.stdout: gdb_index_test_4
	$(TEST_READELF) --debug-dump=gdb_index $< > $@

endif HAVE_PUBNAMES

# End-to-end incremental linking tests.
# Incremental linking is currently supported only on the x86_64 target.

if DEFAULT_TARGET_X86_64

two_file_test_1_v1_ndebug.o: two_file_test_1_v1.cc
	$(CXXCOMPILE) -O0 -g0 -c -o $@ $<
two_file_test_1_ndebug.o: two_file_test_1.cc
	$(CXXCOMPILE) -O0 -g0 -c -o $@ $<
two_file_test_1b_ndebug.o: two_file_test_1b.cc
	$(CXXCOMPILE) -O0 -g0 -c -o $@ $<
two_file_test_2_ndebug.o: two_file_test_2.cc
	$(CXXCOMPILE) -O0 -g0 -c -o $@ $<
two_file_test_main_ndebug.o: two_file_test_main.cc
	$(CXXCOMPILE) -O0 -g0 -c -o $@ $<

check_PROGRAMS += incremental_test_2
MOSTLYCLEANFILES += two_file_test_tmp_2.o
incremental_test_2: two_file_test_1_v1_ndebug.o two_file_test_1_ndebug.o two_file_test_1b_ndebug.o \
		    two_file_test_2_ndebug.o two_file_test_main_ndebug.o gcctestdir/ld
	cp -f two_file_test_1_v1_ndebug.o two_file_test_tmp_2.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ two_file_test_tmp_2.o two_file_test_1b_ndebug.o two_file_test_2_ndebug.o two_file_test_main_ndebug.o
	@sleep 1
	cp -f two_file_test_1_ndebug.o two_file_test_tmp_2.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ two_file_test_tmp_2.o two_file_test_1b_ndebug.o two_file_test_2_ndebug.o two_file_test_main_ndebug.o

check_PROGRAMS += incremental_test_3
MOSTLYCLEANFILES += two_file_test_tmp_3.o
incremental_test_3: two_file_test_1.o two_file_test_1b_v1.o two_file_test_1b.o \
		    two_file_test_2.o two_file_test_main.o gcctestdir/ld
	cp -f two_file_test_1b_v1.o two_file_test_tmp_3.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ two_file_test_1.o two_file_test_tmp_3.o two_file_test_2.o two_file_test_main.o
	@sleep 1
	cp -f two_file_test_1b.o two_file_test_tmp_3.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ two_file_test_1.o two_file_test_tmp_3.o two_file_test_2.o two_file_test_main.o

check_PROGRAMS += incremental_test_4
MOSTLYCLEANFILES += incremental_test_4.base two_file_test_tmp_4.o
incremental_test_4: two_file_test_1.o two_file_test_1b.o two_file_test_2_v1.o \
		    two_file_test_2.o two_file_test_main.o gcctestdir/ld
	cp -f two_file_test_2_v1.o two_file_test_tmp_4.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ two_file_test_1.o two_file_test_1b.o two_file_test_tmp_4.o two_file_test_main.o
	mv -f incremental_test_4 incremental_test_4.base
	@sleep 1
	cp -f two_file_test_2.o two_file_test_tmp_4.o
	$(CXXLINK) -Wl,--incremental-update,--incremental-base=incremental_test_4.base -Bgcctestdir/ two_file_test_1.o two_file_test_1b.o two_file_test_tmp_4.o two_file_test_main.o

check_PROGRAMS += incremental_test_5
MOSTLYCLEANFILES += two_file_test_5.a
incremental_test_5: two_file_test_1.o two_file_test_1b_v1.o two_file_test_1b.o \
		    two_file_test_2.o two_file_test_main.o gcctestdir/ld
	cp -f two_file_test_1b_v1.o two_file_test_tmp_5.o
	$(TEST_AR) rc two_file_test_5.a two_file_test_1.o two_file_test_tmp_5.o two_file_test_2.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ two_file_test_main.o two_file_test_5.a
	@sleep 1
	cp -f two_file_test_1b.o two_file_test_tmp_5.o
	$(TEST_AR) rc two_file_test_5.a two_file_test_1.o two_file_test_tmp_5.o two_file_test_2.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ two_file_test_main.o two_file_test_5.a

# Test the --incremental-unchanged flag with an archive library.
# The second link should not update the library.
check_PROGRAMS += incremental_test_6
MOSTLYCLEANFILES += two_file_test_6.a
incremental_test_6: two_file_test_1.o two_file_test_1b_v1.o two_file_test_1b.o \
		    two_file_test_2.o two_file_test_main.o gcctestdir/ld
	cp -f two_file_test_1b.o two_file_test_tmp_6.o
	$(TEST_AR) rc two_file_test_6.a two_file_test_1.o two_file_test_tmp_6.o two_file_test_2.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ two_file_test_main.o two_file_test_6.a
	@sleep 1
	cp -f two_file_test_1b_v1.o two_file_test_tmp_6.o
	$(TEST_AR) rc two_file_test_6.a two_file_test_1.o two_file_test_tmp_6.o two_file_test_2.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ two_file_test_main.o -Wl,--incremental-unchanged two_file_test_6.a -Wl,--incremental-unknown

check_PROGRAMS += incremental_copy_test
incremental_copy_test: copy_test_v1.o copy_test.o copy_test_1.so copy_test_2.so
	cp -f copy_test_v1.o copy_test_tmp.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ -Wl,-R,. copy_test_tmp.o copy_test_1.so copy_test_2.so
	@sleep 1
	cp -f copy_test.o copy_test_tmp.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ -Wl,-R,. copy_test_tmp.o copy_test_1.so copy_test_2.so

check_PROGRAMS += incremental_common_test_1
incremental_common_test_1: common_test_1_v1.o common_test_1_v2.o gcctestdir/ld
	cp -f common_test_1_v1.o common_test_1_tmp.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ common_test_1_tmp.o
	@sleep 1
	cp -f common_test_1_v2.o common_test_1_tmp.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ common_test_1_tmp.o

check_PROGRAMS += incremental_comdat_test_1
incremental_comdat_test_1: incr_comdat_test_1.o incr_comdat_test_2_v1.o incr_comdat_test_2_v2.o incr_comdat_test_2_v3.o gcctestdir/ld
	cp -f incr_comdat_test_2_v1.o incr_comdat_test_1_tmp.o
	$(CXXLINK) -Wl,--incremental-full,--incremental-patch=100 -Bgcctestdir/ incr_comdat_test_1.o incr_comdat_test_1_tmp.o
	@sleep 1
	cp -f incr_comdat_test_2_v2.o incr_comdat_test_1_tmp.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ incr_comdat_test_1.o incr_comdat_test_1_tmp.o
	@sleep 1
	cp -f incr_comdat_test_2_v3.o incr_comdat_test_1_tmp.o
	$(CXXLINK) -Wl,--incremental-update -Bgcctestdir/ incr_comdat_test_1.o incr_comdat_test_1_tmp.o

endif DEFAULT_TARGET_X86_64

endif GCC
endif NATIVE_LINKER

# These tests work with native and cross linkers.

if NATIVE_OR_CROSS_LINKER

# Test script section order.
check_SCRIPTS += script_test_10.sh
check_DATA += script_test_10.stdout
script_test_10.o: script_test_10.s
	$(TEST_AS) -o $@ $<
script_test_10: $(srcdir)/script_test_10.t script_test_10.o gcctestdir/ld
	gcctestdir/ld -o $@ script_test_10.o -T $(srcdir)/script_test_10.t
script_test_10.stdout: script_test_10
	$(TEST_READELF) -SW script_test_10 > $@

# These tests work with cross linkers only.

if DEFAULT_TARGET_I386

check_SCRIPTS += split_i386.sh
check_DATA += split_i386_1.stdout split_i386_2.stdout \
	split_i386_3.stdout split_i386_4.stdout split_i386_r.stdout
SPLIT_DEFSYMS = --defsym __morestack=0x100 --defsym __morestack_non_split=0x200
split_i386_1.o: split_i386_1.s
	$(TEST_AS) -o $@ $<
split_i386_2.o: split_i386_2.s
	$(TEST_AS) -o $@ $<
split_i386_3.o: split_i386_3.s
	$(TEST_AS) -o $@ $<
split_i386_4.o: split_i386_4.s
	$(TEST_AS) -o $@ $<
split_i386_n.o: split_i386_n.s
	$(TEST_AS) -o $@ $<
split_i386_1: split_i386_1.o split_i386_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_i386_1.o split_i386_n.o
split_i386_1.stdout: split_i386_1
	$(TEST_OBJDUMP) -d $< > $@
split_i386_2: split_i386_2.o split_i386_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_i386_2.o split_i386_n.o
split_i386_2.stdout: split_i386_2
	$(TEST_OBJDUMP) -d $< > $@
split_i386_3.stdout: split_i386_3.o split_i386_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o split_i386_3 split_i386_3.o split_i386_n.o > $@ 2>&1 || exit 0
split_i386_4: split_i386_4.o split_i386_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_i386_4.o split_i386_n.o
split_i386_4.stdout: split_i386_4
	$(TEST_OBJDUMP) -d $< > $@
split_i386_r.stdout: split_i386_1.o split_i386_n.o ../ld-new
	../ld-new -r split_i386_1.o split_i386_n.o -o split_i386_r > $@ 2>&1 || exit 0
MOSTLYCLEANFILES += split_i386_1 split_i386_2 split_i386_3 \
	split_i386_4 split_i386_r

endif DEFAULT_TARGET_I386

if DEFAULT_TARGET_X86_64

check_SCRIPTS += split_x86_64.sh
check_DATA += split_x86_64_1.stdout split_x86_64_2.stdout \
	split_x86_64_3.stdout split_x86_64_4.stdout split_x86_64_r.stdout
SPLIT_DEFSYMS = --defsym __morestack=0x100 --defsym __morestack_non_split=0x200
split_x86_64_1.o: split_x86_64_1.s
	$(TEST_AS) -o $@ $<
split_x86_64_2.o: split_x86_64_2.s
	$(TEST_AS) -o $@ $<
split_x86_64_3.o: split_x86_64_3.s
	$(TEST_AS) -o $@ $<
split_x86_64_4.o: split_x86_64_4.s
	$(TEST_AS) -o $@ $<
split_x86_64_n.o: split_x86_64_n.s
	$(TEST_AS) -o $@ $<
split_x86_64_1: split_x86_64_1.o split_x86_64_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_x86_64_1.o split_x86_64_n.o
split_x86_64_1.stdout: split_x86_64_1
	$(TEST_OBJDUMP) -d $< > $@
split_x86_64_2: split_x86_64_2.o split_x86_64_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_x86_64_2.o split_x86_64_n.o
split_x86_64_2.stdout: split_x86_64_2
	$(TEST_OBJDUMP) -d $< > $@
split_x86_64_3.stdout: split_x86_64_3.o split_x86_64_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o split_x86_64_3 split_x86_64_3.o split_x86_64_n.o > $@ 2>&1 || exit 0
split_x86_64_4: split_x86_64_4.o split_x86_64_n.o ../ld-new
	../ld-new $(SPLIT_DEFSYMS) -o $@ split_x86_64_4.o split_x86_64_n.o
split_x86_64_4.stdout: split_x86_64_4
	$(TEST_OBJDUMP) -d $< > $@
split_x86_64_r.stdout: split_x86_64_1.o split_x86_64_n.o ../ld-new
	../ld-new -r split_x86_64_1.o split_x86_64_n.o -o split_x86_64_r > $@ 2>&1 || exit 0
MOSTLYCLEANFILES += split_x86_64_1 split_x86_64_2 split_x86_64_3 \
	split_x86_64_4 split_x86_64_r

endif DEFAULT_TARGET_X86_64

if DEFAULT_TARGET_ARM

check_SCRIPTS += arm_abs_global.sh
check_DATA += arm_abs_global.stdout
arm_abs_lib.o: arm_abs_lib.s
	$(TEST_AS) -march=armv7-a -o $@ $<
libarm_abs.so: arm_abs_lib.o ../ld-new
	../ld-new -shared -o $@ arm_abs_lib.o
arm_abs_global.o: arm_abs_global.s
	$(TEST_AS) -march=armv7-a -o $@ $<
arm_abs_global: arm_abs_global.o libarm_abs.so ../ld-new
	../ld-new -o $@ arm_abs_global.o -L. -larm_abs
arm_abs_global.stdout: arm_abs_global
	$(TEST_READELF) -r $< > $@

MOSTLYCLEANFILES += arm_abs_global

check_SCRIPTS += arm_branch_in_range.sh arm_branch_out_of_range.sh
check_DATA += arm_bl_in_range.stdout arm_bl_out_of_range.stdout \
	thumb_bl_in_range.stdout thumb_bl_out_of_range.stdout \
	thumb2_bl_in_range.stdout thumb2_bl_out_of_range.stdout \
	thumb_blx_in_range.stdout thumb_blx_out_of_range.stdout \
	thumb2_blx_in_range.stdout thumb2_blx_out_of_range.stdout \
	thumb_bl_out_of_range_local.stdout arm_thm_jump11.stdout \
	arm_thm_jump8.stdout

arm_bl_in_range.stdout: arm_bl_in_range
	$(TEST_OBJDUMP) -D $< > $@

arm_bl_in_range: arm_bl_in_range.o ../ld-new
	../ld-new -T $(srcdir)/arm_branch_range.t -o $@ $<

arm_bl_in_range.o: arm_bl_in_range.s
	$(TEST_AS) -o $@ $<

arm_bl_out_of_range.stdout: arm_bl_out_of_range
	$(TEST_OBJDUMP) -S $< > $@

arm_bl_out_of_range: arm_bl_out_of_range.o ../ld-new
	../ld-new -T $(srcdir)/arm_branch_range.t -o $@ $<

arm_bl_out_of_range.o: arm_bl_out_of_range.s
	$(TEST_AS) -o $@ $<

thumb_bl_in_range.stdout: thumb_bl_in_range
	$(TEST_OBJDUMP) -D $< > $@

thumb_bl_in_range: thumb_bl_in_range.o ../ld-new
	../ld-new --no-fix-arm1176 -T $(srcdir)/thumb_branch_range.t -o $@ $<

thumb_bl_in_range.o: thumb_bl_in_range.s
	$(TEST_AS) -o $@ -march=armv5te $<

thumb_bl_out_of_range.stdout: thumb_bl_out_of_range
	$(TEST_OBJDUMP) -D $< > $@

thumb_bl_out_of_range: thumb_bl_out_of_range.o ../ld-new
	../ld-new --no-fix-arm1176 -T $(srcdir)/thumb_branch_range.t -o $@ $<

thumb_bl_out_of_range.o: thumb_bl_out_of_range.s
	$(TEST_AS) -o $@ -march=armv5te $<

thumb2_bl_in_range.stdout: thumb2_bl_in_range
	$(TEST_OBJDUMP) -D $< > $@

thumb2_bl_in_range: thumb2_bl_in_range.o ../ld-new
	../ld-new -T $(srcdir)/thumb2_branch_range.t -o $@ $<

thumb2_bl_in_range.o: thumb_bl_in_range.s
	$(TEST_AS) -o $@ -march=armv7-a $<

thumb2_bl_out_of_range.stdout: thumb2_bl_out_of_range
	$(TEST_OBJDUMP) -D $< > $@

thumb2_bl_out_of_range: thumb2_bl_out_of_range.o ../ld-new
	../ld-new -T $(srcdir)/thumb2_branch_range.t -o $@ $<

thumb2_bl_out_of_range.o: thumb_bl_out_of_range.s
	$(TEST_AS) -o $@ -march=armv7-a $<

thumb_blx_in_range.stdout: thumb_blx_in_range
	$(TEST_OBJDUMP) -D $< > $@

thumb_blx_in_range: thumb_blx_in_range.o ../ld-new
	../ld-new --no-fix-arm1176 -T $(srcdir)/thumb_branch_range.t -o $@ $<

thumb_blx_in_range.o: thumb_blx_in_range.s
	$(TEST_AS) -o $@ -march=armv5te $<

thumb_blx_out_of_range.stdout: thumb_blx_out_of_range
	$(TEST_OBJDUMP) -D $< > $@

thumb_blx_out_of_range: thumb_blx_out_of_range.o ../ld-new
	../ld-new --no-fix-arm1176 -T $(srcdir)/thumb_branch_range.t -o $@ $<

thumb_blx_out_of_range.o: thumb_blx_out_of_range.s
	$(TEST_AS) -o $@ -march=armv5te $<

thumb2_blx_in_range.stdout: thumb2_blx_in_range
	$(TEST_OBJDUMP) -D $< > $@

thumb2_blx_in_range: thumb2_blx_in_range.o ../ld-new
	../ld-new -T $(srcdir)/thumb2_branch_range.t -o $@ $<

thumb2_blx_in_range.o: thumb_blx_in_range.s
	$(TEST_AS) -o $@ -march=armv7-a $<

thumb2_blx_out_of_range.stdout: thumb2_blx_out_of_range
	$(TEST_OBJDUMP) -D $< > $@

thumb2_blx_out_of_range: thumb2_blx_out_of_range.o ../ld-new
	../ld-new -T $(srcdir)/thumb2_branch_range.t -o $@ $<

thumb2_blx_out_of_range.o: thumb_blx_out_of_range.s
	$(TEST_AS) -o $@ -march=armv7-a $<

thumb_bl_out_of_range_local.stdout: thumb_bl_out_of_range_local
	$(TEST_OBJDUMP) -D $< > $@

thumb_bl_out_of_range_local: thumb_bl_out_of_range_local.o ../ld-new
	../ld-new --no-fix-arm1176 -T $(srcdir)/thumb_branch_range.t -o $@ $<

thumb_bl_out_of_range_local.o: thumb_bl_out_of_range_local.s
	$(TEST_AS) -o $@ -march=armv5te $<

arm_thm_jump11.stdout: arm_thm_jump11
	$(TEST_OBJDUMP) -D $< > $@

arm_thm_jump11: arm_thm_jump11.o ../ld-new
	../ld-new -T $(srcdir)/arm_thm_jump11.t -o $@ $<

arm_thm_jump11.o: arm_thm_jump11.s
	$(TEST_AS) -o $@ $<

arm_thm_jump8.stdout: arm_thm_jump8
	$(TEST_OBJDUMP) -D $< > $@

arm_thm_jump8: arm_thm_jump8.o ../ld-new
	../ld-new -T $(srcdir)/arm_thm_jump8.t -o $@ $<

arm_thm_jump8.o: arm_thm_jump8.s
	$(TEST_AS) -o $@ $<

MOSTLYCLEANFILES += arm_bl_in_range arm_bl_out_of_range thumb_bl_in_range \
	thumb_bl_out_of_range thumb2_bl_in_range thumb2_bl_out_of_range \
	thumb_blx_in_range thumb_blx_out_of_range thumb2_blx_in_range \
	thumb2_blx_out_of_range thumb_bl_out_of_range_local arm_thm_jump11 \
	arm_thm_jump8

check_SCRIPTS += arm_fix_v4bx.sh
check_DATA += arm_fix_v4bx.stdout arm_fix_v4bx_interworking.stdout \
	arm_no_fix_v4bx.stdout

arm_fix_v4bx.stdout: arm_fix_v4bx
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_fix_v4bx: arm_fix_v4bx.o ../ld-new
	../ld-new --no-fix-arm1176 --fix-v4bx -o $@ $<

arm_fix_v4bx.o: arm_fix_v4bx.s
	$(TEST_AS) -o $@ $<

arm_fix_v4bx_interworking.stdout: arm_fix_v4bx_interworking
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_fix_v4bx_interworking: arm_fix_v4bx.o ../ld-new
	../ld-new --no-fix-arm1176 --fix-v4bx-interworking -o $@ $<

arm_no_fix_v4bx.stdout: arm_no_fix_v4bx
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_no_fix_v4bx: arm_fix_v4bx.o ../ld-new
	../ld-new --no-fix-arm1176 -o $@ $<

MOSTLYCLEANFILES += arm_fix_v4bx arm_fix_v4bx_interworking arm_no_fix_v4bx

check_SCRIPTS += arm_attr_merge.sh
check_DATA += arm_attr_merge_6.stdout arm_attr_merge_6r.stdout \
	arm_attr_merge_7.stdout

arm_attr_merge_6.stdout: arm_attr_merge_6
	$(TEST_READELF) -A $< > $@

arm_attr_merge_6: arm_attr_merge_6a.o arm_attr_merge_6b.o
	../ld-new -o $@ arm_attr_merge_6a.o arm_attr_merge_6b.o

arm_attr_merge_6a.o: arm_attr_merge_6a.s
	$(TEST_AS) -o $@ $<

arm_attr_merge_6b.o: arm_attr_merge_6b.s
	$(TEST_AS) -o $@ $<

arm_attr_merge_6r.stdout: arm_attr_merge_6r
	$(TEST_READELF) -A $< > $@

arm_attr_merge_6r: arm_attr_merge_6b.o arm_attr_merge_6a.o
	../ld-new -o $@ arm_attr_merge_6b.o arm_attr_merge_6a.o

arm_attr_merge_7.stdout: arm_attr_merge_7
	$(TEST_READELF) -A $< > $@

arm_attr_merge_7: arm_attr_merge_7a.o arm_attr_merge_7b.o
	../ld-new -o $@ arm_attr_merge_7a.o arm_attr_merge_7b.o

arm_attr_merge_7a.o: arm_attr_merge_7a.s
	$(TEST_AS) -o $@ $<

arm_attr_merge_7b.o: arm_attr_merge_7b.s
	$(TEST_AS) -o $@ $<

MOSTLYCLEANFILES += arm_attr_merge_6 arm_attr_merge_6r arm_attr_merge_7

# ARM1176 workaround test.
check_SCRIPTS += arm_fix_1176.sh
check_DATA += arm_fix_1176_default_v6z.stdout arm_fix_1176_on_v6z.stdout \
	arm_fix_1176_off_v6z.stdout arm_fix_1176_default_v5te.stdout \
	arm_fix_1176_default_v7a.stdout arm_fix_1176_default_1156t2f_s.stdout

arm_fix_1176_default_v6z.stdout: arm_fix_1176_default_v6z
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_default_v6z: arm_fix_1176_default_v6z.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 -o $@ $<

arm_fix_1176_default_v6z.o: arm_fix_1176.s
	$(TEST_AS) -march=armv6z -o $@ $<

arm_fix_1176_on_v6z.stdout: arm_fix_1176_on_v6z
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_on_v6z: arm_fix_1176_on_v6z.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 --fix-arm1176 -o $@ $<

arm_fix_1176_on_v6z.o: arm_fix_1176.s
	$(TEST_AS) -march=armv6z -o $@ $<

arm_fix_1176_off_v6z.stdout: arm_fix_1176_off_v6z
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_off_v6z: arm_fix_1176_off_v6z.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 --no-fix-arm1176 -o $@ $<

arm_fix_1176_off_v6z.o: arm_fix_1176.s
	$(TEST_AS) -march=armv6z -o $@ $<

arm_fix_1176_default_v5te.stdout: arm_fix_1176_default_v5te
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_default_v5te: arm_fix_1176_default_v5te.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 -o $@ $<

arm_fix_1176_default_v5te.o: arm_fix_1176.s
	$(TEST_AS) -march=armv5te -o $@ $<

arm_fix_1176_default_v7a.stdout: arm_fix_1176_default_v7a
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_default_v7a: arm_fix_1176_default_v7a.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 -o $@ $<

arm_fix_1176_default_v7a.o: arm_fix_1176.s
	$(TEST_AS) -march=armv7-a -o $@ $<

arm_fix_1176_default_1156t2f_s.stdout: arm_fix_1176_default_1156t2f_s
	$(TEST_OBJDUMP) -D -j.foo $< > $@

arm_fix_1176_default_1156t2f_s: arm_fix_1176_default_1156t2f_s.o ../ld-new
	../ld-new --section-start=.foo=0x2001014 -o $@ $<

arm_fix_1176_default_1156t2f_s.o: arm_fix_1176.s
	$(TEST_AS) -mcpu=arm1156t2f-s -o $@ $<

MOSTLYCLEANFILES += arm_fix_1176_default_v6z arm_fix_1176_on_v6z arm_fix_1176_off_v6z \
	arm_fix_1176_default_v5te arm_fix_1176_default_v7a arm_fix_1176_default_1156t2f_s

# Cortex-A8 workaround test.

check_SCRIPTS += arm_cortex_a8.sh
check_DATA += arm_cortex_a8_b_cond.stdout arm_cortex_a8_b.stdout \
	arm_cortex_a8_bl.stdout arm_cortex_a8_blx.stdout \
	arm_cortex_a8_local.stdout arm_cortex_a8_local_reloc.stdout

arm_cortex_a8_b_cond.stdout: arm_cortex_a8_b_cond
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_b_cond: arm_cortex_a8_b_cond.o ../ld-new
	../ld-new -o $@ $<

arm_cortex_a8_b_cond.o: arm_cortex_a8_b_cond.s
	$(TEST_AS) -o $@ $<

arm_cortex_a8_b.stdout: arm_cortex_a8_b
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_b: arm_cortex_a8_b.o ../ld-new
	../ld-new --fix-cortex-a8 -o $@ $<

arm_cortex_a8_b.o: arm_cortex_a8_b.s
	$(TEST_AS) -o $@ $<

arm_cortex_a8_bl.stdout: arm_cortex_a8_bl
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_bl: arm_cortex_a8_bl.o ../ld-new
	../ld-new -o $@ $<

arm_cortex_a8_bl.o: arm_cortex_a8_bl.s
	$(TEST_AS) -o $@ $<

arm_cortex_a8_blx.stdout: arm_cortex_a8_blx
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_blx: arm_cortex_a8_blx.o ../ld-new
	../ld-new -o $@ $<

arm_cortex_a8_blx.o: arm_cortex_a8_blx.s
	$(TEST_AS) -o $@ $<

arm_cortex_a8_local.stdout: arm_cortex_a8_local
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_local: arm_cortex_a8_local.o ../ld-new
	../ld-new -o $@ $<

arm_cortex_a8_local.o: arm_cortex_a8_local.s
	$(TEST_AS) -o $@ $<

arm_cortex_a8_local_reloc.stdout: arm_cortex_a8_local_reloc
	$(TEST_OBJDUMP) -D -j.text $< > $@

arm_cortex_a8_local_reloc: arm_cortex_a8_local_reloc.o ../ld-new
	../ld-new -o $@ $<

arm_cortex_a8_local_reloc.o: arm_cortex_a8_local_reloc.s
	$(TEST_AS) -o $@ $<

MOSTLYCLEANFILES += arm_cortex_a8_b_cond arm_cortex_a8_b arm_cortex_a8_bl \
	arm_cortex_a8_blx arm_cortex_a8_local arm_cortex_a8_local_reloc

check_SCRIPTS += arm_exidx_test.sh
check_DATA += arm_exidx_test.stdout

arm_exidx_test.stdout: arm_exidx_test.so
	$(TEST_READELF) -Sr $< > $@

arm_exidx_test.so: arm_exidx_test.o ../ld-new
	../ld-new -shared -o $@ $<

arm_exidx_test.o: arm_exidx_test.s
	$(TEST_AS) -o $@ $<

check_SCRIPTS += pr12826.sh
check_DATA += pr12826.stdout

pr12826.stdout: pr12826.so
	$(TEST_READELF) -A $< > $@

pr12826.so: pr12826_1.o pr12826_2.o ../ld-new
	../ld-new -shared -o $@ $<

pr12826_1.o: pr12826_1.s
	$(TEST_AS) -o $@ $<

pr12826_2.o: pr12826_2.s
	$(TEST_AS) -o $@ $<

check_SCRIPTS += arm_unaligned_reloc.sh
check_DATA += arm_unaligned_reloc.stdout arm_unaligned_reloc_r.stdout

arm_unaligned_reloc.stdout: arm_unaligned_reloc
	$(TEST_OBJDUMP) -D $< > $@

arm_unaligned_reloc_r.stdout: arm_unaligned_reloc_r
	$(TEST_OBJDUMP) -Dr $< > $@

arm_unaligned_reloc: arm_unaligned_reloc.o ../ld-new
	../ld-new -o $@ $<

arm_unaligned_reloc_r: arm_unaligned_reloc.o ../ld-new
	../ld-new -r -o $@ $<

arm_unaligned_reloc.o: arm_unaligned_reloc.s
	$(TEST_AS) -o $@ $<

MOSTLYCLEANFILES += arm_unaligned_reloc arm_unaligned_reloc_r

# Check ARM to ARM farcall veneers

check_SCRIPTS += arm_farcall_arm_arm.sh
check_DATA += arm_farcall_arm_arm.stdout

arm_farcall_arm_arm.stdout: arm_farcall_arm_arm
	$(TEST_OBJDUMP) -d $< > $@

arm_farcall_arm_arm: arm_farcall_arm_arm.o ../ld-new
	../ld-new --no-fix-arm1176 --section-start .text=0x1000 --section-start .foo=0x2001020 -o $@ $<

arm_farcall_arm_arm.o: arm_farcall_arm_arm.s
	$(TEST_AS) -o $@ $<

MOSTLYCLEANFILES += arm_farcall_arm_arm

# Check ARM to Thumb farcall veneers

check_SCRIPTS += arm_farcall_arm_thumb.sh
check_DATA += arm_farcall_arm_thumb.stdout arm_farcall_arm_thumb_5t.stdout

arm_farcall_arm_thumb.stdout: arm_farcall_arm_thumb
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_arm_thumb: arm_farcall_arm_thumb.o ../ld-new
	../ld-new --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_arm_thumb.o: arm_farcall_arm_thumb.s
	$(TEST_AS) -o $@ $<

arm_farcall_arm_thumb_5t.stdout: arm_farcall_arm_thumb_5t
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_arm_thumb_5t: arm_farcall_arm_thumb_5t.o ../ld-new
	../ld-new --no-fix-arm1176 --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_arm_thumb_5t.o: arm_farcall_arm_thumb.s
	$(TEST_AS) -march=armv5t -o $@ $<

MOSTLYCLEANFILES += arm_farcall_arm_thumb arm_farcall_arm_thumb_5t

# Check Thumb to Thumb farcall veneers

check_SCRIPTS += arm_farcall_thumb_thumb.sh
check_DATA += arm_farcall_thumb_thumb.stdout \
	      arm_farcall_thumb_thumb_5t.stdout \
	      arm_farcall_thumb_thumb_7m.stdout \
	      arm_farcall_thumb_thumb_6m.stdout

arm_farcall_thumb_thumb.stdout: arm_farcall_thumb_thumb
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_thumb: arm_farcall_thumb_thumb.o ../ld-new
	../ld-new --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_thumb.o: arm_farcall_thumb_thumb.s
	$(TEST_AS) -march=armv4t -o $@ $<

arm_farcall_thumb_thumb_5t.stdout: arm_farcall_thumb_thumb_5t
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_thumb_5t: arm_farcall_thumb_thumb_5t.o ../ld-new
	../ld-new --no-fix-arm1176 --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_thumb_5t.o: arm_farcall_thumb_thumb.s
	$(TEST_AS) -march=armv5t -o $@ $<

arm_farcall_thumb_thumb_7m.stdout: arm_farcall_thumb_thumb_7m
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_thumb_7m: arm_farcall_thumb_thumb_7m.o ../ld-new
	../ld-new --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_thumb_7m.o: arm_farcall_thumb_thumb.s
	$(TEST_AS) -march=armv7-m -o $@ $<

arm_farcall_thumb_thumb_6m.stdout: arm_farcall_thumb_thumb_6m
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_thumb_6m: arm_farcall_thumb_thumb_6m.o ../ld-new
	../ld-new --section-start .text=0x1000 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_thumb_6m.o: arm_farcall_thumb_thumb.s
	$(TEST_AS) -march=armv6-m -o $@ $<

MOSTLYCLEANFILES += arm_farcall_thumb_thumb arm_farcall_thumb_thumb_5t \
		    arm_farcall_thumb_thumb_7m arm_farcall_thumb_thumb_6m

# Check Thumb to ARM farcall veneers

check_SCRIPTS += arm_farcall_thumb_arm.sh
check_DATA += arm_farcall_thumb_arm.stdout \
	      arm_farcall_thumb_arm_5t.stdout

arm_farcall_thumb_arm.stdout: arm_farcall_thumb_arm
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_arm: arm_farcall_thumb_arm.o ../ld-new
	../ld-new --section-start .text=0x1c01010 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_arm.o: arm_farcall_thumb_arm.s
	$(TEST_AS) -o $@ $<

arm_farcall_thumb_arm_5t.stdout: arm_farcall_thumb_arm_5t
	$(TEST_OBJDUMP) -D $< > $@

arm_farcall_thumb_arm_5t: arm_farcall_thumb_arm_5t.o ../ld-new
	../ld-new --no-fix-arm1176 --section-start .text=0x1c01010 --section-start .foo=0x2001014 -o $@ $<

arm_farcall_thumb_arm_5t.o: arm_farcall_thumb_arm.s
	$(TEST_AS) -march=armv5t -o $@ $<

MOSTLYCLEANFILES += arm_farcall_thumb_arm arm_farcall_thumb_arm_5t

endif DEFAULT_TARGET_ARM

endif NATIVE_OR_CROSS_LINKER

