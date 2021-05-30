#	$NetBSD: Makefile,v 1.8 2021/05/30 01:56:59 joerg Exp $

.include <bsd.init.mk>

SUBDIR=	experimental

.PATH: ${LIBCXX_SRCDIR}/include

INCSDIR=	/usr/include/c++

INCS=	algorithm \
	any \
	array \
	atomic \
	__availability \
	barrier \
	bit \
	__bit_reference \
	__bits \
	bitset \
	__bsd_locale_defaults.h \
	__bsd_locale_fallbacks.h \
	cassert \
	ccomplex \
	cctype \
	cerrno \
	cfenv \
	cfloat \
	charconv \
	chrono \
	cinttypes \
	ciso646 \
	climits \
	clocale \
	cmath \
	codecvt \
	compare \
	complex \
	complex.h \
	concepts \
	condition_variable \
	__config \
	__config_site \
	csetjmp \
	csignal \
	cstdarg \
	cstdbool \
	cstddef \
	cstdint \
	cstdio \
	cstdlib \
	cstring \
	ctgmath \
	ctime \
	cwchar \
	cwctype \
	__debug \
	deque \
	__errc \
	exception \
	execution \
	filesystem \
	format \
	forward_list \
	fstream \
	functional \
	__functional_03 \
	__functional_base \
	__functional_base_03 \
	future \
	__hash_table \
	initializer_list \
	iomanip \
	ios \
	iosfwd \
	iostream \
	istream \
	iterator \
	latch \
	__libcpp_version \
	limits \
	list \
	__locale \
	locale \
	map \
	memory \
	module.modulemap \
	mutex \
	__mutex_base \
	new \
	__node_handle \
	__nullptr \
	numbers \
	numeric \
	optional \
	ostream \
	queue \
	random \
	ranges \
	ratio \
	regex \
	scoped_allocator \
	semaphore \
	set \
	shared_mutex \
	span \
	__split_buffer \
	sstream \
	stack \
	stdexcept \
	__std_stream \
	streambuf \
	__string \
	string \
	string_view \
	strstream \
	system_error \
	tgmath.h \
	thread \
	__threading_support \
	__tree \
	__tuple \
	tuple \
	typeindex \
	typeinfo \
	type_traits \
	__undef_macros \
	unordered_map \
	unordered_set \
	utility \
	valarray \
	variant \
	vector \
	version

INCS+=	ctype.h \
	errno.h \
	fenv.h \
	float.h \
	inttypes.h \
	limits.h \
	locale.h \
	math.h \
	optional \
	setjmp.h \
	stdbool.h \
	stddef.h \
	stdint.h \
	stdio.h \
	stdlib.h \
	string.h \
	wchar.h \
	wctype.h

INCS+=		cxxabi.h

INCS+=	ext/__hash \
	ext/hash_map \
	ext/hash_set

INCS+=	__iterator/concepts.h \
	__iterator/incrementable_traits.h \
	__iterator/iter_move.h \
	__iterator/iterator_traits.h \
	__iterator/readable_traits.h

INCS+=	__memory/addressof.h \
	__memory/allocation_guard.h \
	__memory/allocator.h \
	__memory/allocator_traits.h \
	__memory/auto_ptr.h \
	__memory/compressed_pair.h \
	__memory/construct_at.h \
	__memory/pointer_safety.h \
	__memory/pointer_traits.h \
	__memory/raw_storage_iterator.h \
	__memory/shared_ptr.h \
	__memory/temporary_buffer.h \
	__memory/uninitialized_algorithms.h \
	__memory/unique_ptr.h

INCS+=	__ranges/access.h \
	__ranges/concepts.h \
	__ranges/data.h \
	__ranges/empty.h \
	__ranges/enable_borrowed_range.h \
	__ranges/size.h \
	__ranges/view.h

INCS+=	__utility/to_underlying.h


cxxabi.h: ${LIBCXXRT_SRCDIR}/src/cxxabi.h
	cp ${LIBCXXRT_SRCDIR}/src/cxxabi.h .

DPSRCS+=	cxxabi.h
CLEANFILES+=	cxxabi.h

.include <bsd.clean.mk>
.include <bsd.inc.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
