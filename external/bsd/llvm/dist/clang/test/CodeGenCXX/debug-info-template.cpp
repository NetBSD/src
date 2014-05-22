// RUN: %clang -S -emit-llvm -target x86_64-unknown_unknown -g %s -o - -std=c++11 | FileCheck %s

// CHECK: {{.*}}, i1 false, metadata !"", i32 0, metadata !{{[0-9]]*}}, metadata [[RETAIN:![0-9]*]], {{.*}} ; [ DW_TAG_compile_unit ]
// CHECK: [[EMPTY:![0-9]*]] = metadata !{}
// CHECK: [[RETAIN]] = metadata !{metadata !{{[0-9]]*}}, metadata [[FOO:![0-9]*]],


// CHECK: [[TC:![0-9]*]] = {{.*}}, metadata [[TCARGS:![0-9]*]], metadata !"{{.*}}"} ; [ DW_TAG_structure_type ] [TC<unsigned int, 2, &glb, &foo::e, &foo::f, &func, tmpl_impl, 1, 2, 3>]
// CHECK: [[TCARGS]] = metadata !{metadata [[TCARG1:![0-9]*]], metadata [[TCARG2:![0-9]*]], metadata [[TCARG3:![0-9]*]], metadata [[TCARG4:![0-9]*]], metadata [[TCARG5:![0-9]*]], metadata [[TCARG6:![0-9]*]], metadata [[TCARG7:![0-9]*]], metadata [[TCARG8:![0-9]*]]}
//
// We seem to be missing file/line/col info on template value parameters -
// metadata supports it but it's not populated. GCC doesn't emit it either,
// perhaps we should just drop it from the metadata.
//
// CHECK: [[TCARG1]] = {{.*}}metadata !"T", metadata [[UINT:![0-9]*]], {{.*}} ; [ DW_TAG_template_type_parameter ]
// CHECK: [[UINT:![0-9]*]] = {{.*}} ; [ DW_TAG_base_type ] [unsigned int]
// CHECK: [[TCARG2]] = {{.*}}metadata !"", metadata [[UINT]], i32 2, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCARG3]] = {{.*}}metadata !"x", metadata [[INTPTR:![0-9]*]], i32* @glb, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[INTPTR]] = {{.*}}, metadata [[INT:![0-9]*]]} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from int]
// CHECK: [[INT]] = {{.*}} ; [ DW_TAG_base_type ] [int]
// CHECK: [[TCARG4]] = {{.*}}metadata !"a", metadata [[MEMINTPTR:![0-9]*]], i64 8, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[MEMINTPTR]] = {{.*}}, metadata !"_ZTS3foo"} ; [ DW_TAG_ptr_to_member_type ] {{.*}}[from int]
//
// Currently Clang emits the pointer-to-member-function value, but LLVM doesn't
// use it (GCC doesn't emit a value for pointers to member functions either - so
// it's not clear what, if any, format would be acceptable to GDB)
//
// CHECK: [[TCARG5]] = {{.*}}metadata !"b", metadata [[MEMFUNPTR:![0-9]*]], { i64, i64 } { i64 ptrtoint (void (%struct.foo*)* @_ZN3foo1fEv to i64), i64 0 }, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[MEMFUNPTR]] = {{.*}}, metadata [[FTYPE:![0-9]*]], metadata !"_ZTS3foo"} ; [ DW_TAG_ptr_to_member_type ]
// CHECK: [[FTYPE]] = {{.*}}, metadata [[FARGS:![0-9]*]], i32 0, null, null, null} ; [ DW_TAG_subroutine_type ]
// CHECK: [[FARGS]] = metadata !{null, metadata [[FARG1:![0-9]*]]}
// CHECK: [[FARG1]] = {{.*}} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS3foo]
//
// CHECK: [[TCARG6]] = {{.*}}metadata !"f", metadata [[FUNPTR:![0-9]*]], void ()* @_Z4funcv, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[FUNPTR]] = {{.*}}, metadata [[FUNTYPE:![0-9]*]]} ; [ DW_TAG_pointer_type ]
// CHECK: [[FUNTYPE]] = {{.*}}, metadata [[FUNARGS:![0-9]*]], i32 0, null, null, null} ; [ DW_TAG_subroutine_type ]
// CHECK: [[FUNARGS]] = metadata !{null}
// CHECK: [[TCARG7]] = {{.*}}metadata !"tmpl", null, metadata !"tmpl_impl", {{.*}} ; [ DW_TAG_GNU_template_template_param ]
// CHECK: [[TCARG8]] = {{.*}}metadata !"Is", null, metadata [[TCARG8_VALS:![0-9]*]], {{.*}} ; [ DW_TAG_GNU_template_parameter_pack ]
// CHECK: [[TCARG8_VALS]] = metadata !{metadata [[TCARG8_1:![0-9]*]], metadata [[TCARG8_2:![0-9]*]], metadata [[TCARG8_3:![0-9]*]]}
// CHECK: [[TCARG8_1]] = {{.*}}metadata !"", metadata [[INT]], i32 1, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCARG8_2]] = {{.*}}metadata !"", metadata [[INT]], i32 2, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCARG8_3]] = {{.*}}metadata !"", metadata [[INT]], i32 3, {{.*}} ; [ DW_TAG_template_value_parameter ]
//
// We could just emit a declaration of 'foo' here, rather than the entire
// definition (same goes for any time we emit a member (function or data)
// pointer type)
// CHECK: [[FOO]] = {{.*}}, metadata !"_ZTS3foo"} ; [ DW_TAG_structure_type ] [foo]
// CHECK: metadata !"f", metadata !"_ZN3foo1fEv", i32 {{[0-9]*}}, metadata [[FTYPE:![0-9]*]],
//


// CHECK: [[TCNESTED:![0-9]*]] = metadata !{i32 {{[0-9]*}}, metadata !{{[0-9]*}}, metadata !"_ZTS2TCIjLj2EXadL_Z3glbEEXadL_ZN3foo1eEEEXadL_ZNS0_1fEvEEXadL_Z4funcvEE9tmpl_implJLi1ELi2ELi3EEE", {{.*}} ; [ DW_TAG_structure_type ] [nested]
// CHECK: [[TCNT:![0-9]*]] = {{.*}}, metadata [[TCNARGS:![0-9]*]], metadata !"{{.*}}"} ; [ DW_TAG_structure_type ] [TC<int, -3, nullptr, nullptr, nullptr, nullptr, tmpl_impl>]
// CHECK: [[TCNARGS]] = metadata !{metadata [[TCNARG1:![0-9]*]], metadata [[TCNARG2:![0-9]*]], metadata [[TCNARG3:![0-9]*]], metadata [[TCNARG4:![0-9]*]], metadata [[TCNARG5:![0-9]*]], metadata [[TCNARG6:![0-9]*]], metadata [[TCARG7:![0-9]*]], metadata [[TCNARG8:![0-9]*]]}
// CHECK: [[TCNARG1]] = {{.*}}metadata !"T", metadata [[INT]], {{.*}} ; [ DW_TAG_template_type_parameter ]
// CHECK: [[TCNARG2]] = {{.*}}metadata !"", metadata [[INT]], i32 -3, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCNARG3]] = {{.*}}metadata !"x", metadata [[INTPTR]], i8 0, {{.*}} ; [ DW_TAG_template_value_parameter ]

// The interesting null pointer: -1 for member data pointers (since they are
// just an offset in an object, they can be zero and non-null for the first
// member)

// CHECK: [[TCNARG4]] = {{.*}}metadata !"a", metadata [[MEMINTPTR]], i64 -1, {{.*}} ; [ DW_TAG_template_value_parameter ]
//
// In some future iteration we could possibly emit the value of a null member
// function pointer as '{ i64, i64 } zeroinitializer' as it may be handled
// naturally from the LLVM CodeGen side once we decide how to handle non-null
// member function pointers. For now, it's simpler just to emit the 'i8 0'.
//
// CHECK: [[TCNARG5]] = {{.*}}metadata !"b", metadata [[MEMFUNPTR]], i8 0, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCNARG6]] = {{.*}}metadata !"f", metadata [[FUNPTR]], i8 0, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[TCNARG8]] = {{.*}}metadata !"Is", null, metadata [[EMPTY]], {{.*}} ; [ DW_TAG_GNU_template_parameter_pack ]

// CHECK: metadata [[PTOARGS:![0-9]*]], metadata !"{{.*}}"} ; [ DW_TAG_structure_type ] [PaddingAtEndTemplate<&PaddedObj>]
// CHECK: [[PTOARGS]] = metadata !{metadata [[PTOARG1:![0-9]*]]}
// CHECK: [[PTOARG1]] = {{.*}}metadata !"", metadata [[CONST_PADDINGATEND_PTR:![0-9]*]], { i32, i8, [3 x i8] }* @PaddedObj, {{.*}} ; [ DW_TAG_template_value_parameter ]
// CHECK: [[CONST_PADDINGATEND_PTR]] = {{.*}} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS12PaddingAtEnd]

// CHECK: metadata [[TCNESTED]], i32 0, i32 1, %"struct.TC<unsigned int, 2, &glb, &foo::e, &foo::f, &func, tmpl_impl, 1, 2, 3>::nested"* @tci, null} ; [ DW_TAG_variable ] [tci]

// CHECK: metadata [[TCNT:![0-9]*]], i32 0, i32 1, %struct.TC* @tcn, null} ; [ DW_TAG_variable ] [tcn]
struct foo {
  char pad[8]; // make the member pointer to 'e' a bit more interesting (nonzero)
  int e;
  void f();
};

template<typename T, T, int *x, int foo::*a, void (foo::*b)(), void (*f)(), template<typename> class tmpl, int ...Is>
struct TC {
  struct nested {
  };
};

int glb;
void func();

template<typename>
struct tmpl_impl {
};

TC<unsigned, 2, &glb, &foo::e, &foo::f, &func, tmpl_impl, 1, 2, 3>::nested tci;
TC<int, -3, nullptr, nullptr, nullptr, nullptr, tmpl_impl> tcn;

struct PaddingAtEnd {
  int i;
  char c;
};

PaddingAtEnd PaddedObj = {};

template <const PaddingAtEnd *>
struct PaddingAtEndTemplate {
};

PaddingAtEndTemplate<&PaddedObj> PaddedTemplateObj;
