//===- Pipeline.cpp - Support Pipeline ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Support/Pipeline.h"

using namespace offloadtest;

namespace llvm {
namespace yaml {
void MappingTraits<offloadtest::Pipeline>::mapping(IO &I,
                                                   offloadtest::Pipeline &P) {
  I.mapRequired("Shaders", P.Shaders);
  // Runtime-specific settings.
  I.mapOptional("RuntimeSettings", P.Settings);

  I.mapRequired("Buffers", P.Buffers);
  I.mapRequired("DescriptorSets", P.Sets);

  if (!I.outputting()) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        R.BufferPtr = P.getBuffer(R.Name);
        if (!R.BufferPtr)
          I.setError(Twine("Referenced buffer ") + R.Name + " not found!");
      }
    }
    uint32_t DescriptorTableCount = 0;
    for (auto &R : P.Settings.DX.RootParams) {
      switch (R.Kind) {
      case dx::RootParamKind::DescriptorTable:
        ++DescriptorTableCount;
        break;
      case dx::RootParamKind::Constant: {
        auto &Constant = std::get<dx::RootConstant>(R.Data);
        Constant.BufferPtr = P.getBuffer(Constant.Name);
        if (!Constant.BufferPtr)
          I.setError(Twine("Referenced buffer in root constant ") +
                     Constant.Name + " not found!");
        break;
      }
      case dx::RootParamKind::RootDescriptor: {
        auto &Resource = std::get<dx::RootResource>(R.Data);
        Resource.BufferPtr = P.getBuffer(Resource.Name);
        if (!Resource.BufferPtr)
          I.setError(Twine("Referenced buffer in root descriptor ") +
                     Resource.Name + " not found!");
        break;
      }
      }
    }
    if (P.Settings.DX.RootParams.size() != 0 &&
        DescriptorTableCount != P.Sets.size())
      I.setError(Twine("Expected ") + std::to_string(P.Sets.size()) +
                 " DescriptorTable root parameters, found " +
                 std::to_string(DescriptorTableCount));
  }
}

void MappingTraits<offloadtest::DescriptorSet>::mapping(
    IO &I, offloadtest::DescriptorSet &D) {
  I.mapRequired("Resources", D.Resources);
}

void MappingTraits<offloadtest::Buffer>::mapping(IO &I,
                                                 offloadtest::Buffer &B) {
  I.mapRequired("Name", B.Name);
  I.mapRequired("Format", B.Format);
  I.mapOptional("Channels", B.Channels, 1);
  I.mapOptional("Stride", B.Stride, 0);
  if (!I.outputting() && B.Stride != 0 && B.Channels != 1)
    I.setError("Cannot set a structure stride and more than one channel.");
  switch (B.Format) {
#define DATA_CASE(Enum, Type)                                                  \
  case DataFormat::Enum: {                                                     \
    if (I.outputting()) {                                                      \
      llvm::MutableArrayRef<Type> Arr(reinterpret_cast<Type *>(B.Data.get()),  \
                                      B.Size / sizeof(Type));                  \
      I.mapRequired("Data", Arr);                                              \
    } else {                                                                   \
      int64_t ZeroInitSize;                                                    \
      I.mapOptional("ZeroInitSize", ZeroInitSize, 0);                          \
      if (ZeroInitSize > 0) {                                                  \
        B.Size = ZeroInitSize;                                                 \
        B.Data.reset(new char[B.Size]);                                        \
        memset(B.Data.get(), 0, B.Size);                                       \
        break;                                                                 \
      }                                                                        \
      llvm::SmallVector<Type, 64> Arr;                                         \
      I.mapRequired("Data", Arr);                                              \
      B.Size = Arr.size() * sizeof(Type);                                      \
      B.Data.reset(new char[B.Size]);                                          \
      memcpy(B.Data.get(), Arr.data(), B.Size);                                \
    }                                                                          \
    break;                                                                     \
  }
    DATA_CASE(Hex8, llvm::yaml::Hex8)
    DATA_CASE(Hex16, llvm::yaml::Hex16)
    DATA_CASE(Hex32, llvm::yaml::Hex32)
    DATA_CASE(Hex64, llvm::yaml::Hex64)
    DATA_CASE(UInt16, uint16_t)
    DATA_CASE(UInt32, uint32_t)
    DATA_CASE(UInt64, uint64_t)
    DATA_CASE(Int16, int16_t)
    DATA_CASE(Int32, int32_t)
    DATA_CASE(Int64, int64_t)
    DATA_CASE(Float32, float)
    DATA_CASE(Float64, double)
  }

  I.mapOptional("OutputProps", B.OutputProps);
}

void MappingTraits<offloadtest::Resource>::mapping(IO &I,
                                                   offloadtest::Resource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  I.mapRequired("DirectXBinding", R.DXBinding);
}

void MappingTraits<offloadtest::DirectXBinding>::mapping(
    IO &I, offloadtest::DirectXBinding &B) {
  I.mapRequired("Register", B.Register);
  I.mapRequired("Space", B.Space);
}

void MappingTraits<offloadtest::OutputProperties>::mapping(
    IO &I, offloadtest::OutputProperties &P) {
  I.mapRequired("Height", P.Height);
  I.mapRequired("Width", P.Width);
  I.mapRequired("Depth", P.Depth);
}

void MappingTraits<offloadtest::dx::RootResource>::mapping(
    IO &I, offloadtest::dx::RootResource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  R.DXBinding = {0, 0};
  if (!I.outputting() && !R.isRaw())
    I.setError("Root descriptors must be raw resource types.");
}

void MappingTraits<offloadtest::dx::RootParameter>::mapping(
    IO &I, offloadtest::dx::RootParameter &P) {
  I.mapRequired("Kind", P.Kind);
  switch (P.Kind) {
  case dx::RootParamKind::Constant:
    if (!I.outputting())
      P.Data = dx::RootConstant();
    I.mapRequired("Name", std::get<dx::RootConstant>(P.Data).Name);
    break;
  case dx::RootParamKind::RootDescriptor:
    if (!I.outputting())
      P.Data = dx::RootResource();
    I.mapRequired("Resource", std::get<dx::RootResource>(P.Data));
    break;
  case dx::RootParamKind::DescriptorTable:
    break;
  }
}

void MappingTraits<offloadtest::dx::Settings>::mapping(
    IO &I, offloadtest::dx::Settings &S) {
  I.mapOptional("RootParameters", S.RootParams);
}

void MappingTraits<offloadtest::RuntimeSettings>::mapping(
    IO &I, offloadtest::RuntimeSettings &S) {
  I.mapOptional("DirectX", S.DX);
}

void MappingTraits<offloadtest::Shader>::mapping(IO &I,
                                                 offloadtest::Shader &S) {
  I.mapRequired("Stage", S.Stage);
  I.mapRequired("Entry", S.Entry);

  if (S.Stage == Stages::Compute) {
    // Stage-specific data, not sure if this should be optional
    // or moved into the Shaders structure.
    MutableArrayRef<int> MutableDispatchSize(S.DispatchSize);
    I.mapRequired("DispatchSize", MutableDispatchSize);
  }
}
} // namespace yaml
} // namespace llvm
