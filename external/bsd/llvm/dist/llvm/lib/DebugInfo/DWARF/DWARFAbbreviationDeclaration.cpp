//===-- DWARFAbbreviationDeclaration.cpp ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFAbbreviationDeclaration.h"
#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
using namespace dwarf;

void DWARFAbbreviationDeclaration::clear() {
  Code = 0;
  Tag = DW_TAG_null;
  CodeByteSize = 0;
  HasChildren = false;
  AttributeSpecs.clear();
  FixedAttributeSize.reset();
}

DWARFAbbreviationDeclaration::DWARFAbbreviationDeclaration() {
  clear();
}

bool
DWARFAbbreviationDeclaration::extract(DataExtractor Data, 
                                      uint32_t* OffsetPtr) {
  clear();
  const uint32_t Offset = *OffsetPtr;
  Code = Data.getULEB128(OffsetPtr);
  if (Code == 0) {
    return false;
  }
  CodeByteSize = *OffsetPtr - Offset;
  Tag = static_cast<llvm::dwarf::Tag>(Data.getULEB128(OffsetPtr));
  if (Tag == DW_TAG_null) {
    clear();
    return false;
  }
  uint8_t ChildrenByte = Data.getU8(OffsetPtr);
  HasChildren = (ChildrenByte == DW_CHILDREN_yes);
  // Assign a value to our optional FixedAttributeSize member variable. If
  // this member variable still has a value after the while loop below, then
  // all attribute data in this abbreviation declaration has a fixed byte size.
  FixedAttributeSize = FixedSizeInfo();

  // Read all of the abbreviation attributes and forms.
  while (true) {
    auto A = static_cast<Attribute>(Data.getULEB128(OffsetPtr));
    auto F = static_cast<Form>(Data.getULEB128(OffsetPtr));
    if (A && F) {
      auto FixedFormByteSize = DWARFFormValue::getFixedByteSize(F);
      AttributeSpecs.push_back(AttributeSpec(A, F, FixedFormByteSize));
      // If this abbrevation still has a fixed byte size, then update the
      // FixedAttributeSize as needed.
      if (FixedAttributeSize) {
        if (FixedFormByteSize)
          FixedAttributeSize->NumBytes += *FixedFormByteSize;
        else {
          switch (F) {
          case DW_FORM_addr:
            ++FixedAttributeSize->NumAddrs;
            break;

          case DW_FORM_ref_addr:
            ++FixedAttributeSize->NumRefAddrs;
            break;

          case DW_FORM_strp:
          case DW_FORM_GNU_ref_alt:
          case DW_FORM_GNU_strp_alt:
          case DW_FORM_line_strp:
          case DW_FORM_sec_offset:
          case DW_FORM_strp_sup:
          case DW_FORM_ref_sup:
            ++FixedAttributeSize->NumDwarfOffsets;
            break;

          default:
            // Indicate we no longer have a fixed byte size for this
            // abbreviation by clearing the FixedAttributeSize optional value
            // so it doesn't have a value.
            FixedAttributeSize.reset();
            break;
          }
        }
      }
    } else if (A == 0 && F == 0) {
      // We successfully reached the end of this abbreviation declaration
      // since both attribute and form are zero.
      break;
    } else {
      // Attribute and form pairs must either both be non-zero, in which case
      // they are added to the abbreviation declaration, or both be zero to
      // terminate the abbrevation declaration. In this case only one was
      // zero which is an error.
      clear();
      return false;
    }
  }
  return true;
}

void DWARFAbbreviationDeclaration::dump(raw_ostream &OS) const {
  auto tagString = TagString(getTag());
  OS << '[' << getCode() << "] ";
  if (!tagString.empty())
    OS << tagString;
  else
    OS << format("DW_TAG_Unknown_%x", getTag());
  OS << "\tDW_CHILDREN_" << (hasChildren() ? "yes" : "no") << '\n';
  for (const AttributeSpec &Spec : AttributeSpecs) {
    OS << '\t';
    auto attrString = AttributeString(Spec.Attr);
    if (!attrString.empty())
      OS << attrString;
    else
      OS << format("DW_AT_Unknown_%x", Spec.Attr);
    OS << '\t';
    auto formString = FormEncodingString(Spec.Form);
    if (!formString.empty())
      OS << formString;
    else
      OS << format("DW_FORM_Unknown_%x", Spec.Form);
    OS << '\n';
  }
  OS << '\n';
}

Optional<uint32_t>
DWARFAbbreviationDeclaration::findAttributeIndex(dwarf::Attribute Attr) const {
  for (uint32_t i = 0, e = AttributeSpecs.size(); i != e; ++i) {
    if (AttributeSpecs[i].Attr == Attr)
      return i;
  }
  return None;
}

Optional<DWARFFormValue> DWARFAbbreviationDeclaration::getAttributeValue(
    const uint32_t DIEOffset, const dwarf::Attribute Attr,
    const DWARFUnit &U) const {
  Optional<uint32_t> MatchAttrIndex = findAttributeIndex(Attr);
  if (!MatchAttrIndex)
    return None;

  auto DebugInfoData = U.getDebugInfoExtractor();

  // Add the byte size of ULEB that for the abbrev Code so we can start
  // skipping the attribute data.
  uint32_t Offset = DIEOffset + CodeByteSize;
  uint32_t AttrIndex = 0;
  for (const auto &Spec : AttributeSpecs) {
    if (*MatchAttrIndex == AttrIndex) {
      // We have arrived at the attribute to extract, extract if from Offset.
      DWARFFormValue FormValue(Spec.Form);
      if (FormValue.extractValue(DebugInfoData, &Offset, &U))
        return FormValue;
    }
    // March Offset along until we get to the attribute we want.
    if (Optional<uint8_t> FixedSize = Spec.getByteSize(U))
      Offset += *FixedSize;
    else
      DWARFFormValue::skipValue(Spec.Form, DebugInfoData, &Offset, &U);
    ++AttrIndex;
  }
  return None;
}

size_t DWARFAbbreviationDeclaration::FixedSizeInfo::getByteSize(
    const DWARFUnit &U) const {
  size_t ByteSize = NumBytes;
  if (NumAddrs)
    ByteSize += NumAddrs * U.getAddressByteSize();
  if (NumRefAddrs)
    ByteSize += NumRefAddrs * U.getRefAddrByteSize();
  if (NumDwarfOffsets)
    ByteSize += NumDwarfOffsets * U.getDwarfOffsetByteSize();
  return ByteSize;
}

Optional<uint8_t> DWARFAbbreviationDeclaration::AttributeSpec::getByteSize(
    const DWARFUnit &U) const {
  return ByteSize ? ByteSize : DWARFFormValue::getFixedByteSize(Form, &U);
}

Optional<size_t> DWARFAbbreviationDeclaration::getFixedAttributesByteSize(
    const DWARFUnit &U) const {
  if (FixedAttributeSize)
    return FixedAttributeSize->getByteSize(U);
  return None;
}
