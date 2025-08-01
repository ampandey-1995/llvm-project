//===-- include/flang-rt/runtime/non-tbp-dio.h ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Defines a structure used to identify the non-type-bound defined I/O
// generic interfaces that are accessible in a particular scope.  This
// table is used by some I/O APIs and is also part of the NAMELIST
// group table.
//
// A specific procedure for a particular derived type must appear in
// this table if it (a) is a dummy procedure or procedure pointer,
// (b) is part of the defined I/O generic definition in a scope other
// than the one that contains the derived type definition, or (c)
// is a null pointer signifying that some specific procedure from
// a containing scope has become inaccessible in a nested scope due
// to the use of "IMPORT, NONE" or "IMPORT, ONLY:".

#ifndef FLANG_RT_RUNTIME_NON_TBP_DIO_H_
#define FLANG_RT_RUNTIME_NON_TBP_DIO_H_

#include "flang/Common/Fortran-consts.h"
#include "flang/Common/api-attrs.h"
#include <cstddef>

namespace Fortran::runtime::typeInfo {
class DerivedType;
} // namespace Fortran::runtime::typeInfo

namespace Fortran::runtime::io {

RT_OFFLOAD_API_GROUP_BEGIN

enum NonTbpDefinedIoFlags {
  IsDtvArgPolymorphic = 1 << 0, // first dummy arg is CLASS(T)
  DefinedIoInteger8 = 1 << 1, // -fdefault-integer-8 affected UNIT= & IOSTAT=
};

struct NonTbpDefinedIo {
  const typeInfo::DerivedType &derivedType;
  void (*subroutine)(); // null means no non-TBP defined I/O here
  common::DefinedIo definedIo;
  std::uint8_t flags;
};

struct NonTbpDefinedIoTable {
  RT_API_ATTRS const NonTbpDefinedIo *Find(
      const typeInfo::DerivedType &, common::DefinedIo) const;
  std::size_t items{0};
  const NonTbpDefinedIo *item{nullptr};
  // True when the only procedures to be used are the type-bound special
  // procedures in the type information tables and any non-null procedures
  // in this table.  When false, the entries in this table override whatever
  // non-type-bound specific procedures might be in the type information,
  // but the remaining specifics remain visible.
  bool ignoreNonTbpEntries{false};
};

RT_OFFLOAD_API_GROUP_END

} // namespace Fortran::runtime::io
#endif // FLANG_RT_RUNTIME_NON_TBP_DIO_H_
