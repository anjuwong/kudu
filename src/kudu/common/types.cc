// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "kudu/common/types.h"

#include <memory>
#include <unordered_map>

#include "kudu/gutil/singleton.h"

using std::shared_ptr;
using std::unordered_map;

namespace kudu {

template<typename TypeTraitsClass>
TypeInfo::TypeInfo(TypeTraitsClass t)
  : type_(TypeTraitsClass::type),
    physical_type_(TypeTraitsClass::physical_type),
    name_(TypeTraitsClass::name()),
    size_(TypeTraitsClass::size),
    min_value_(TypeTraitsClass::min_value()),
    append_func_(TypeTraitsClass::AppendDebugStringForValue),
    compare_func_(TypeTraitsClass::Compare),
    are_consecutive_func_(TypeTraitsClass::AreConsecutive) {
}

void TypeInfo::AppendDebugStringForValue(const void *ptr, string *str) const {
  append_func_(ptr, str);
}

int TypeInfo::Compare(const void *lhs, const void *rhs) const {
  return compare_func_(lhs, rhs);
}

// @andrwng
int TypeInfo::CompareBlock(PredicateType pred, const ColumnBlock& block, SelectionVector *sel) const {
  if (block.is_nullable()) {
    for (size_t i = 0; i < block.nrows(); i++) {
      if (!sel->IsRowSelected(i)) continue;
      const void *cell = block.nullable_cell_ptr(i);
    }
  }
}

bool TypeInfo::AreConsecutive(const void* a, const void* b) const {
  return are_consecutive_func_(a, b);
}

class TypeInfoResolver {
 public:
  const TypeInfo* GetTypeInfo(DataType t) {
    const TypeInfo *type_info = mapping_[t].get();
    CHECK(type_info != nullptr) <<
      "Bad type: " << t;
    return type_info;
  }

 private:
  TypeInfoResolver() {
    AddMapping<UINT8>();
    AddMapping<INT8>();
    AddMapping<UINT16>();
    AddMapping<INT16>();
    AddMapping<UINT32>();
    AddMapping<INT32>();
    AddMapping<UINT64>();
    AddMapping<INT64>();
    AddMapping<TIMESTAMP>();
    AddMapping<STRING>();
    AddMapping<BOOL>();
    AddMapping<FLOAT>();
    AddMapping<DOUBLE>();
    AddMapping<BINARY>();
  }

  template<DataType type> void AddMapping() {
    TypeTraits<type> traits;
    mapping_.insert(make_pair(type, shared_ptr<TypeInfo>(new TypeInfo(traits))));
  }

  unordered_map<DataType,
                shared_ptr<const TypeInfo>,
                std::hash<size_t> > mapping_;

  friend class Singleton<TypeInfoResolver>;
  DISALLOW_COPY_AND_ASSIGN(TypeInfoResolver);
};

const TypeInfo* GetTypeInfo(DataType type) {
  return Singleton<TypeInfoResolver>::get()->GetTypeInfo(type);
}

} // namespace kudu
