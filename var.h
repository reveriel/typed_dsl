#include "dag.h"
namespace typed_dsl {

//  define a set of types for basic types variables

// basic types
using VarInt = Var<int>;
using VarUInt = Var<unsigned int>;
using VarLong = Var<long>;
using VarULong = Var<unsigned long>;
using VarInt32 = Var<int32_t>;
using VarUInt32 = Var<uint32_t>;
using VarInt64 = Var<int64_t>;
using VarUInt64 = Var<uint64_t>;
using VarFloat = Var<float>;
using VarF32 = Var<float>;
using VarF64 = Var<double>;
using VarDouble = Var<double>;
using VarStr = Var<std::string>;
using VarBool = Var<bool>;

// vector types
template <typename T>
using VarVec = Var<std::vector<T>>;
using VarVecInt = VarVec<int>;
using VarVecUInt = VarVec<unsigned int>;
using VarVecLong = VarVec<long>;
using VarVecULong = VarVec<unsigned long>;
using VarVecInt32 = VarVec<int32_t>;
using VarVecUInt32 = VarVec<uint32_t>;
using VarVecInt64 = VarVec<int64_t>;
using VarVecUInt64 = VarVec<uint64_t>;
using VarVecStr = VarVec<std::string>;
using VarVecF32 = VarVec<float>;
using VarVecF64 = VarVec<double>;

}  // namespace typed_dsl

