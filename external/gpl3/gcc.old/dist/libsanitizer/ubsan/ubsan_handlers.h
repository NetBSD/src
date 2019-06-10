//===-- ubsan_handlers.h ----------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Entry points to the runtime library for Clang's undefined behavior sanitizer.
//
//===----------------------------------------------------------------------===//
#ifndef UBSAN_HANDLERS_H
#define UBSAN_HANDLERS_H

#include "ubsan_value.h"

namespace __ubsan {

struct TypeMismatchData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
  uptr Alignment;
  unsigned char TypeCheckKind;
};

#define UNRECOVERABLE(checkname, ...) \
  extern "C" SANITIZER_INTERFACE_ATTRIBUTE NORETURN \
    void __ubsan_handle_ ## checkname( __VA_ARGS__ );

#define RECOVERABLE(checkname, ...) \
  extern "C" SANITIZER_INTERFACE_ATTRIBUTE \
    void __ubsan_handle_ ## checkname( __VA_ARGS__ ); \
  extern "C" SANITIZER_INTERFACE_ATTRIBUTE NORETURN \
    void __ubsan_handle_ ## checkname ## _abort( __VA_ARGS__ );

/// \brief Handle a runtime type check failure, caused by either a misaligned
/// pointer, a null pointer, or a pointer to insufficient storage for the
/// type.
RECOVERABLE(type_mismatch, TypeMismatchData *Data, ValueHandle Pointer)

struct OverflowData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

/// \brief Handle an integer addition overflow.
RECOVERABLE(add_overflow, OverflowData *Data, ValueHandle LHS, ValueHandle RHS)

/// \brief Handle an integer subtraction overflow.
RECOVERABLE(sub_overflow, OverflowData *Data, ValueHandle LHS, ValueHandle RHS)

/// \brief Handle an integer multiplication overflow.
RECOVERABLE(mul_overflow, OverflowData *Data, ValueHandle LHS, ValueHandle RHS)

/// \brief Handle a signed integer overflow for a unary negate operator.
RECOVERABLE(negate_overflow, OverflowData *Data, ValueHandle OldVal)

/// \brief Handle an INT_MIN/-1 overflow or division by zero.
RECOVERABLE(divrem_overflow, OverflowData *Data,
            ValueHandle LHS, ValueHandle RHS)

struct ShiftOutOfBoundsData {
  SourceLocation Loc;
  const TypeDescriptor &LHSType;
  const TypeDescriptor &RHSType;
};

/// \brief Handle a shift where the RHS is out of bounds or a left shift where
/// the LHS is negative or overflows.
RECOVERABLE(shift_out_of_bounds, ShiftOutOfBoundsData *Data,
            ValueHandle LHS, ValueHandle RHS)

struct OutOfBoundsData {
  SourceLocation Loc;
  const TypeDescriptor &ArrayType;
  const TypeDescriptor &IndexType;
};

/// \brief Handle an array index out of bounds error.
RECOVERABLE(out_of_bounds, OutOfBoundsData *Data, ValueHandle Index)

struct UnreachableData {
  SourceLocation Loc;
};

/// \brief Handle a __builtin_unreachable which is reached.
UNRECOVERABLE(builtin_unreachable, UnreachableData *Data)
/// \brief Handle reaching the end of a value-returning function.
UNRECOVERABLE(missing_return, UnreachableData *Data)

struct VLABoundData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

/// \brief Handle a VLA with a non-positive bound.
RECOVERABLE(vla_bound_not_positive, VLABoundData *Data, ValueHandle Bound)

// Keeping this around for binary compatibility with (sanitized) programs
// compiled with older compilers.
struct FloatCastOverflowData {
  const TypeDescriptor &FromType;
  const TypeDescriptor &ToType;
};

struct FloatCastOverflowDataV2 {
  SourceLocation Loc;
  const TypeDescriptor &FromType;
  const TypeDescriptor &ToType;
};

/// Handle overflow in a conversion to or from a floating-point type.
/// void *Data is one of FloatCastOverflowData* or FloatCastOverflowDataV2*
RECOVERABLE(float_cast_overflow, void *Data, ValueHandle From)

struct InvalidValueData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

/// \brief Handle a load of an invalid value for the type.
RECOVERABLE(load_invalid_value, InvalidValueData *Data, ValueHandle Val)

struct FunctionTypeMismatchData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

RECOVERABLE(function_type_mismatch,
            FunctionTypeMismatchData *Data,
            ValueHandle Val)

struct NonNullReturnData {
  SourceLocation Loc;
  SourceLocation AttrLoc;
};

/// \brief Handle returning null from function with returns_nonnull attribute.
RECOVERABLE(nonnull_return, NonNullReturnData *Data)

struct NonNullArgData {
  SourceLocation Loc;
  SourceLocation AttrLoc;
  int ArgIndex;
};

/// \brief Handle passing null pointer to function with nonnull attribute.
RECOVERABLE(nonnull_arg, NonNullArgData *Data)

struct CFIBadIcallData {
  SourceLocation Loc;
  const TypeDescriptor &Type;
};

/// \brief Handle control flow integrity failure for indirect function calls.
RECOVERABLE(cfi_bad_icall, CFIBadIcallData *Data, ValueHandle Function)

}

#endif // UBSAN_HANDLERS_H
