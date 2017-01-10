//===-- DWARFFormValue.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_DWARFFORMVALUE_H
#define LLVM_DEBUGINFO_DWARFFORMVALUE_H

#include "llvm/ADT/Optional.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Dwarf.h"

namespace llvm {

template <typename T> class ArrayRef;
class DWARFUnit;
class raw_ostream;

class DWARFFormValue {
public:
  enum FormClass {
    FC_Unknown,
    FC_Address,
    FC_Block,
    FC_Constant,
    FC_String,
    FC_Flag,
    FC_Reference,
    FC_Indirect,
    FC_SectionOffset,
    FC_Exprloc
  };

private:
  struct ValueType {
    ValueType() : data(nullptr) {
      uval = 0;
    }

    union {
      uint64_t uval;
      int64_t sval;
      const char* cstr;
    };
    const uint8_t* data;
  };

  dwarf::Form Form; // Form for this value.
  ValueType Value; // Contains all data for the form.
  const DWARFUnit *U; // Remember the DWARFUnit at extract time.

public:
  DWARFFormValue(dwarf::Form F = dwarf::Form(0)) : Form(F), U(nullptr) {}
  dwarf::Form getForm() const { return Form; }
  void setForm(dwarf::Form F) { Form = F; }
  bool isFormClass(FormClass FC) const;
  const DWARFUnit *getUnit() const { return U; }
  void dump(raw_ostream &OS) const;

  /// \brief extracts a value in data at offset *offset_ptr.
  ///
  /// The passed DWARFUnit is allowed to be nullptr, in which
  /// case no relocation processing will be performed and some
  /// kind of forms that depend on Unit information are disallowed.
  /// \returns whether the extraction succeeded.
  bool extractValue(const DataExtractor &Data, uint32_t *OffsetPtr,
                    const DWARFUnit *U);
  bool isInlinedCStr() const {
    return Value.data != nullptr && Value.data == (const uint8_t*)Value.cstr;
  }

  /// getAsFoo functions below return the extracted value as Foo if only
  /// DWARFFormValue has form class is suitable for representing Foo.
  Optional<uint64_t> getAsReference() const;
  Optional<uint64_t> getAsUnsignedConstant() const;
  Optional<int64_t> getAsSignedConstant() const;
  Optional<const char *> getAsCString() const;
  Optional<uint64_t> getAsAddress() const;
  Optional<uint64_t> getAsSectionOffset() const;
  Optional<ArrayRef<uint8_t>> getAsBlock() const;
  Optional<uint64_t> getAsCStringOffset() const;
  Optional<uint64_t> getAsReferenceUVal() const;
  /// Get the fixed byte size for a given form.
  ///
  /// If the form always has a fixed valid byte size that doesn't depend on a
  /// DWARFUnit, then an Optional with a value will be returned. If the form
  /// can vary in size depending on the DWARFUnit (DWARF version, address byte
  /// size, or DWARF 32/64) and the DWARFUnit is valid, then an Optional with a
  /// valid value is returned. If the form is always encoded using a variable
  /// length storage format (ULEB or SLEB numbers or blocks) or the size
  /// depends on a DWARFUnit and the DWARFUnit is NULL, then None will be
  /// returned.
  /// \param Form The DWARF form to get the fixed byte size for
  /// \param U The DWARFUnit that can be used to help determine the byte size.
  ///
  /// \returns Optional<uint8_t> value with the fixed byte size or None if
  /// \p Form doesn't have a fixed byte size or a DWARFUnit wasn't supplied
  /// and was needed to calculate the byte size.
  static Optional<uint8_t> getFixedByteSize(dwarf::Form Form,
                                            const DWARFUnit *U = nullptr);
  /// Get the fixed byte size for a given form.
  ///
  /// If the form has a fixed byte size given a valid DWARF version and address
  /// byte size, then an Optional with a valid value is returned. If the form
  /// is always encoded using a variable length storage format (ULEB or SLEB
  /// numbers or blocks) then None will be returned.
  ///
  /// \param Form DWARF form to get the fixed byte size for
  /// \param Version DWARF version number.
  /// \param AddrSize size of an address in bytes.
  /// \param Format enum value from llvm::dwarf::DwarfFormat.
  /// \returns Optional<uint8_t> value with the fixed byte size or None if
  /// \p Form doesn't have a fixed byte size.
  static Optional<uint8_t> getFixedByteSize(dwarf::Form Form, uint16_t Version,
                                            uint8_t AddrSize,
                                            llvm::dwarf::DwarfFormat Format);

  /// Skip a form in \p debug_info_data at offset specified by \p offset_ptr.
  ///
  /// Skips the bytes for this form in the debug info and updates the offset.
  ///
  /// \param debug_info_data the .debug_info data to use to skip the value.
  /// \param offset_ptr a reference to the offset that will be updated.
  /// \param U the DWARFUnit to use when skipping the form in case the form
  /// size differs according to data in the DWARFUnit.
  /// \returns true on success, false if the form was not skipped.
  bool skipValue(DataExtractor debug_info_data, uint32_t *offset_ptr,
                 const DWARFUnit *U) const;
  /// Skip a form in \p debug_info_data at offset specified by \p offset_ptr.
  ///
  /// Skips the bytes for this form in the debug info and updates the offset.
  ///
  /// \param form the DW_FORM enumeration that indicates the form to skip.
  /// \param debug_info_data the .debug_info data to use to skip the value.
  /// \param offset_ptr a reference to the offset that will be updated.
  /// \param U the DWARFUnit to use when skipping the form in case the form
  /// size differs according to data in the DWARFUnit.
  /// \returns true on success, false if the form was not skipped.
  static bool skipValue(dwarf::Form form, DataExtractor debug_info_data,
                        uint32_t *offset_ptr, const DWARFUnit *U);
  /// Skip a form in \p debug_info_data at offset specified by \p offset_ptr.
  ///
  /// Skips the bytes for this form in the debug info and updates the offset.
  ///
  /// \param form the DW_FORM enumeration that indicates the form to skip.
  /// \param debug_info_data the .debug_info data to use to skip the value.
  /// \param offset_ptr a reference to the offset that will be updated.
  /// \param Version DWARF version number.
  /// \param AddrSize size of an address in bytes.
  /// \param Format enum value from llvm::dwarf::DwarfFormat.
  /// \returns true on success, false if the form was not skipped.
  static bool skipValue(dwarf::Form form, DataExtractor debug_info_data,
                        uint32_t *offset_ptr, uint16_t Version,
                        uint8_t AddrSize, llvm::dwarf::DwarfFormat Format);

private:
  void dumpString(raw_ostream &OS) const;
};

}

#endif
