// MIT License
//
// Copyright (c) 2024 Mindaugas Vinkelis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <bitsery/adapter/buffer.h>
#include <bitsery/ext/field_registry.h>
#include <bitsery/ext/offset_table.h>
#include <bitsery/details/offset_table.h>
#include <bitsery/offset_table_inspect.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

using Buffer = std::vector<uint8_t>;

namespace model {

struct Simple
{
  uint32_t a{};
  uint16_t b{};
};

struct CopyOnlyChild
{
  std::array<uint8_t, 4> data{};
};

struct Nested
{
  CopyOnlyChild child{};
  uint32_t tail{};
};

struct Mixed
{
  uint32_t a{};
  std::array<uint16_t, 2> arr{};
  std::vector<char> text{};
  std::vector<uint8_t> bytes{};
  Nested nested{};
};

enum class Color : uint8_t
{
  Red = 1,
  Green = 2,
  Blue = 3
};

struct TrivialPod
{
  uint32_t x{};
  uint16_t y{};
  uint16_t pad{};
};

struct KitchenSink
{
  int8_t i8{};
  uint16_t u16{};
  uint32_t u32{};
  uint64_t u64{};
  float f32{};
  double f64{};
  Color color{};
  std::array<uint16_t, 3> fixed{};
  std::string text{};
  std::vector<uint8_t> bytes{};
  std::vector<TrivialPod> pods{};
  Nested nested{};
};

} // namespace model

namespace bitsery { namespace details {

template<>
struct FieldRegistry<model::Simple>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 2;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::Simple>(
      1, &model::Simple::a, FieldKind::Scalar),
    ext::makeField<model::Simple>(
      2, &model::Simple::b, FieldKind::Scalar) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<model::CopyOnlyChild>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 1;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::CopyOnlyChild>(
      1,
      &model::CopyOnlyChild::data,
      FieldKind::Array,
      FieldFlags::CopyOnly) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<model::Nested>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 2;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::Nested>(
      1, &model::Nested::child, FieldKind::NestedTable),
    ext::makeField<model::Nested>(
      2, &model::Nested::tail, FieldKind::Scalar) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<model::Mixed>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 5;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::Mixed>(
      1, &model::Mixed::a, FieldKind::Scalar),
    ext::makeField<model::Mixed>(
      2, &model::Mixed::arr, FieldKind::Array),
    ext::makeField<model::Mixed>(
      3,
      &model::Mixed::text,
      FieldKind::Array,
      FieldFlags::CopyOnly),
    ext::makeField<model::Mixed>(
      4,
      &model::Mixed::bytes,
      FieldKind::Array,
      FieldFlags::CopyOnly),
    ext::makeField<model::Mixed>(
      5, &model::Mixed::nested, FieldKind::NestedTable) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<model::KitchenSink>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 12;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::KitchenSink>(
      1, &model::KitchenSink::i8, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      2, &model::KitchenSink::u16, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      3, &model::KitchenSink::u32, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      4, &model::KitchenSink::u64, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      5, &model::KitchenSink::f32, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      6, &model::KitchenSink::f64, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      7, &model::KitchenSink::color, FieldKind::Scalar),
    ext::makeField<model::KitchenSink>(
      8, &model::KitchenSink::fixed, FieldKind::Array),
    ext::makeField<model::KitchenSink>(
      9,
      &model::KitchenSink::text,
      FieldKind::Array,
      FieldFlags::CopyOnly),
    ext::makeField<model::KitchenSink>(
      10,
      &model::KitchenSink::bytes,
      FieldKind::Array,
      FieldFlags::CopyOnly),
    ext::makeField<model::KitchenSink>(
      11,
      &model::KitchenSink::pods,
      FieldKind::Array,
      FieldFlags::CopyOnly),
    ext::makeField<model::KitchenSink>(
      12, &model::KitchenSink::nested, FieldKind::NestedTable) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<model::TrivialPod>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 3;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<model::TrivialPod>(
      1, &model::TrivialPod::x, FieldKind::Scalar),
    ext::makeField<model::TrivialPod>(
      2, &model::TrivialPod::y, FieldKind::Scalar),
    ext::makeField<model::TrivialPod>(
      3, &model::TrivialPod::pad, FieldKind::Scalar) };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

}} // namespace bitsery::details

namespace model {

template<typename S>
void serialize(S& s, Simple& v)
{
  s.value4b(v.a);
  s.value2b(v.b);
}

template<typename S>
void serialize(S& s, CopyOnlyChild& v)
{
  s.container1b(v.data);
}

template<typename S>
void serialize(S& s, Nested& v)
{
  s.object(v.child);
  s.value4b(v.tail);
}

template<typename S>
void serialize(S& s, Mixed& v)
{
  s.value4b(v.a);
  s.template container<2>(v.arr);
  s.container1b(v.text, 50);
  s.container1b(v.bytes, 10);
  s.object(v.nested);
}

template<typename S>
void serialize(S& s, TrivialPod& v)
{
  s.value4b(v.x);
  s.value2b(v.y);
  s.value2b(v.pad);
}

template<typename S>
void serialize(S& s, Color& v)
{
  s.value1b(reinterpret_cast<uint8_t&>(v));
}

template<typename S>
void serialize(S& s, KitchenSink& v)
{
  s.value1b(v.i8);
  s.value2b(v.u16);
  s.value4b(v.u32);
  s.value8b(v.u64);
  s.value4b(v.f32);
  s.value8b(v.f64);
  s.value1b(reinterpret_cast<uint8_t&>(v.color));
  s.template container<2>(v.fixed);
  s.text1b(v.text, 64);
  s.container1b(v.bytes, 64);
  s.container(
    v.pods, static_cast<size_t>(16), [](S& ser, TrivialPod& pod) {
      ser.object(pod);
    });
  s.object(v.nested);
}

} // namespace model

namespace {

template<typename T>
Buffer serializeWithOffsetTables(const T& value)
{
  Buffer buf;
  bitsery::details::OffsetTableWriterState state{};
  const auto written = bitsery::ext::serializeWithOffsetTable(
    state, bitsery::OutputBufferAdapter<Buffer>{ buf }, value);
  buf.resize(written);
  return buf;
}

} // namespace

TEST(OffsetTableInspect, BuildsTreeForSimpleType)
{
  model::Simple v{};
  v.a = 0xAABBCCDD;
  v.b = 0xEEFF;

  auto buf = serializeWithOffsetTables(v);

  auto res = bitsery::ot::inspectOffsetTable<model::Simple>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  ASSERT_EQ(res.root->childCount, 2u);

  const auto* fieldA = res.root->child(0);
  ASSERT_NE(fieldA, nullptr);
  EXPECT_EQ(fieldA->fieldId, 1);
  EXPECT_TRUE(fieldA->viewable);
  EXPECT_EQ(fieldA->reason, bitsery::ot::FieldReason::None);
  auto bytesA = fieldA->bytes();
  EXPECT_EQ(bytesA.second, sizeof(v.a));
  EXPECT_EQ(bytesA.first, buf.data() + fieldA->payloadOffset);
  const auto deserializedA = fieldA->construct<uint32_t>();
  EXPECT_EQ(deserializedA, v.a);

  const auto* fieldB = res.root->child(1);
  ASSERT_NE(fieldB, nullptr);
  EXPECT_EQ(fieldB->fieldId, 2);
  EXPECT_TRUE(fieldB->viewable);
  EXPECT_EQ(fieldB->reason, bitsery::ot::FieldReason::None);
  auto bytesB = fieldB->bytes();
  EXPECT_EQ(bytesB.second, sizeof(v.b));
  EXPECT_EQ(bytesB.first, buf.data() + fieldB->payloadOffset);
  const auto deserializedB = fieldB->construct<uint16_t>();
  EXPECT_EQ(deserializedB, v.b);
}

TEST(OffsetTableInspect, FlagsCopyOnlyNestedField)
{
  model::Nested v{};
  v.child.data = { { 1, 2, 3, 4 } };
  v.tail = 0x11223344;

  auto buf = serializeWithOffsetTables(v);

  auto res = bitsery::ot::inspectOffsetTable<model::Nested>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  ASSERT_EQ(res.root->childCount, 2u);

  const auto* childNode = res.root->child(0);
  ASSERT_NE(childNode, nullptr);
  EXPECT_EQ(childNode->fieldId, 1);
  EXPECT_FALSE(childNode->viewable);
  EXPECT_EQ(childNode->reason, bitsery::ot::FieldReason::CopyOnly);
  ASSERT_EQ(childNode->childCount, 0u);
  EXPECT_FALSE(childNode->canViewBytes());
  model::CopyOnlyChild materialized{};
  childNode->constructInto(materialized);
  EXPECT_EQ(materialized.data, v.child.data);

  const auto* tailNode = res.root->child(1);
  ASSERT_NE(tailNode, nullptr);
  EXPECT_EQ(tailNode->fieldId, 2);
  EXPECT_TRUE(tailNode->viewable);
  EXPECT_EQ(tailNode->reason, bitsery::ot::FieldReason::None);
  EXPECT_EQ(tailNode->construct<uint32_t>(), v.tail);
}

TEST(OffsetTableInspect, VisitorReceivesPath)
{
  model::Nested v{};
  v.child.data = { { 9, 8, 7, 6 } };
  v.tail = 0xABCDEEFF;

  auto buf = serializeWithOffsetTables(v);

  std::vector<std::vector<uint16_t>> seenPaths;
  auto status = bitsery::ot::inspectOffsetTable<model::Nested>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() },
    [&](const bitsery::ot::FieldNode<bitsery::InputBufferAdapter<Buffer>>& node,
        const std::vector<uint16_t>& path) {
      if (!path.empty())
        seenPaths.push_back(path);
      if (node.fieldId == 2) {
        EXPECT_EQ(path.size(), 1u);
        EXPECT_EQ(path.back(), 2u);
      }
    });

  EXPECT_EQ(status, bitsery::ot::InspectStatus::Ok);
  ASSERT_EQ(seenPaths.size(), 2u);
  EXPECT_EQ(seenPaths[0], (std::vector<uint16_t>{ 1 }));
  EXPECT_EQ(seenPaths[1], (std::vector<uint16_t>{ 2 }));
}

TEST(OffsetTableInspect, InspectComplexMixedType)
{
  model::Mixed v{};
  v.a = 0x12345678;
  v.arr = { { 0x1111, 0x2222 } };
  v.text = { 'h', 'e', 'l', 'l', 'o', ' ', 'o', 'f', 'f', 's', 'e', 't',
             ' ', 't', 'a', 'b', 'l', 'e', 's' };
  v.bytes = { 9, 8, 7, 6, 5 };
  v.nested.child.data = { { 1, 2, 3, 4 } };
  v.nested.tail = 0xCAFEBABE;

  auto buf = serializeWithOffsetTables(v);

  auto res = bitsery::ot::inspectOffsetTable<model::Mixed>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  ASSERT_EQ(res.root->childCount, 5u);

  const auto* fieldA = res.root->child(0);
  ASSERT_NE(fieldA, nullptr);
  EXPECT_TRUE(fieldA->viewable);
  EXPECT_EQ(fieldA->reason, bitsery::ot::FieldReason::None);
  EXPECT_EQ(fieldA->construct<uint32_t>(), v.a);

  const auto* fieldArr = res.root->child(1);
  ASSERT_NE(fieldArr, nullptr);
  EXPECT_TRUE(fieldArr->viewable);
  EXPECT_EQ(fieldArr->reason, bitsery::ot::FieldReason::None);
  auto arrBytes = fieldArr->bytes();
  ASSERT_NE(arrBytes.first, nullptr);
  ASSERT_EQ(arrBytes.second, sizeof(v.arr));
  std::array<uint16_t, 2> arrDes{};
  std::memcpy(arrDes.data(), arrBytes.first, arrBytes.second);
  EXPECT_EQ(arrDes, v.arr);

  const auto* fieldText = res.root->child(2);
  ASSERT_NE(fieldText, nullptr);
  EXPECT_FALSE(fieldText->viewable);
  EXPECT_EQ(fieldText->reason, bitsery::ot::FieldReason::CopyOnly);
  std::vector<char> textOut;
  fieldText->constructInto(textOut);
  EXPECT_EQ(textOut, v.text);

  const auto* fieldBytes = res.root->child(3);
  ASSERT_NE(fieldBytes, nullptr);
  EXPECT_FALSE(fieldBytes->viewable);
  EXPECT_EQ(fieldBytes->reason, bitsery::ot::FieldReason::CopyOnly);
  std::vector<uint8_t> vecOut;
  fieldBytes->constructInto(vecOut);
  EXPECT_EQ(vecOut, v.bytes);

  const auto* fieldNested = res.root->child(4);
  ASSERT_NE(fieldNested, nullptr);
  EXPECT_FALSE(fieldNested->viewable);
  ASSERT_EQ(fieldNested->childCount, 2u);
  const auto* nestedChild = fieldNested->child(0);
  ASSERT_NE(nestedChild, nullptr);
  EXPECT_EQ(nestedChild->reason, bitsery::ot::FieldReason::CopyOnly);
  model::CopyOnlyChild co{};
  nestedChild->constructInto(co);
  EXPECT_EQ(co.data, v.nested.child.data);

  const auto* nestedTail = fieldNested->child(1);
  ASSERT_NE(nestedTail, nullptr);
  EXPECT_TRUE(nestedTail->viewable);
  EXPECT_EQ(nestedTail->construct<uint32_t>(), v.nested.tail);

  // Visitor cross-check
  std::vector<std::vector<uint16_t>> visited;
  auto status = bitsery::ot::inspectOffsetTable<model::Mixed>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() },
    [&](const bitsery::ot::FieldNode<bitsery::InputBufferAdapter<Buffer>>&,
        const std::vector<uint16_t>& path) { visited.push_back(path); });
  EXPECT_EQ(status, bitsery::ot::InspectStatus::Ok);
  ASSERT_EQ(visited.size(), 7u); // 5 root children + 2 nested children
  EXPECT_EQ(visited.front(), (std::vector<uint16_t>{ 1 }));
  EXPECT_EQ(visited.back(), (std::vector<uint16_t>{ 5, 2 }));
}

TEST(OffsetTableInspect, KitchenSinkZeroCopyRoundTrip)
{
  model::KitchenSink v{};
  v.i8 = -5;
  v.u16 = 0xBEEF;
  v.u32 = 0xFEEDC0DE;
  v.u64 = 0x1122334455667788ULL;
  v.f32 = 3.25f;
  v.f64 = 9.125;
  v.color = model::Color::Blue;
  v.fixed = { { 0x1111, 0x2222, 0x3333 } };
  v.text = "kitchen sink";
  v.bytes = { 1, 2, 3, 4, 5, 6, 7 };
  v.pods = { { 0xAABBCCDDu, 0x1234u }, { 0x01020304u, 0xEEFFu } };
  v.nested.child.data = { { 9, 8, 7, 6 } };
  v.nested.tail = 0xAABBCCDD;

  auto buf = serializeWithOffsetTables(v);

  auto trailerInfo = bitsery::details::parseTrailer(buf.data(), buf.size());
  ASSERT_TRUE(trailerInfo.valid);

  auto res = bitsery::ot::inspectOffsetTable<model::KitchenSink>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  ASSERT_EQ(res.root->childCount, 12u);

  const auto* fieldI8 = res.root->child(0);
  ASSERT_NE(fieldI8, nullptr);
  EXPECT_TRUE(fieldI8->viewable);
  EXPECT_EQ(fieldI8->construct<int8_t>(), v.i8);

  const auto* fieldU16 = res.root->child(1);
  ASSERT_NE(fieldU16, nullptr);
  EXPECT_TRUE(fieldU16->viewable);
  EXPECT_EQ(fieldU16->construct<uint16_t>(), v.u16);

  const auto* fieldU32 = res.root->child(2);
  ASSERT_NE(fieldU32, nullptr);
  EXPECT_TRUE(fieldU32->viewable);
  EXPECT_EQ(fieldU32->construct<uint32_t>(), v.u32);

  const auto* fieldU64 = res.root->child(3);
  ASSERT_NE(fieldU64, nullptr);
  EXPECT_TRUE(fieldU64->viewable);
  EXPECT_EQ(fieldU64->construct<uint64_t>(), v.u64);

  const auto* fieldF32 = res.root->child(4);
  ASSERT_NE(fieldF32, nullptr);
  EXPECT_TRUE(fieldF32->viewable);
  EXPECT_FLOAT_EQ(fieldF32->construct<float>(), v.f32);

  const auto* fieldF64 = res.root->child(5);
  ASSERT_NE(fieldF64, nullptr);
  EXPECT_TRUE(fieldF64->viewable);
  EXPECT_DOUBLE_EQ(fieldF64->construct<double>(), v.f64);

  const auto* fieldColor = res.root->child(6);
  ASSERT_NE(fieldColor, nullptr);
  EXPECT_TRUE(fieldColor->viewable);
  EXPECT_EQ(fieldColor->construct<model::Color>(), v.color);

  const auto* fieldFixed = res.root->child(7);
  ASSERT_NE(fieldFixed, nullptr);
  EXPECT_TRUE(fieldFixed->viewable);
  auto fixedBytes = fieldFixed->bytes();
  ASSERT_NE(fixedBytes.first, nullptr);
  ASSERT_EQ(fixedBytes.second, sizeof(v.fixed));
  std::array<uint16_t, 3> fixedOut{};
  std::memcpy(fixedOut.data(), fixedBytes.first, fixedBytes.second);
  EXPECT_EQ(fixedOut, v.fixed);

  const auto* fieldText = res.root->child(8);
  ASSERT_NE(fieldText, nullptr);
  EXPECT_FALSE(fieldText->viewable);
  EXPECT_EQ(fieldText->reason, bitsery::ot::FieldReason::CopyOnly);
  std::string textOut;
  fieldText->constructInto(textOut);
  EXPECT_EQ(textOut, v.text);

  const auto* fieldBytes = res.root->child(9);
  ASSERT_NE(fieldBytes, nullptr);
  EXPECT_FALSE(fieldBytes->viewable);
  EXPECT_EQ(fieldBytes->reason, bitsery::ot::FieldReason::CopyOnly);
  std::vector<uint8_t> bytesOut;
  fieldBytes->constructInto(bytesOut);
  EXPECT_EQ(bytesOut, v.bytes);

  const auto* fieldPods = res.root->child(10);
  ASSERT_NE(fieldPods, nullptr);
  EXPECT_FALSE(fieldPods->viewable);
  EXPECT_EQ(fieldPods->reason, bitsery::ot::FieldReason::CopyOnly);
  std::vector<model::TrivialPod> podsOut;
  fieldPods->constructInto(podsOut);
  ASSERT_EQ(podsOut.size(), v.pods.size());
  for (size_t i = 0; i < podsOut.size(); ++i) {
    EXPECT_EQ(podsOut[i].x, v.pods[i].x);
    EXPECT_EQ(podsOut[i].y, v.pods[i].y);
  }

  const auto* fieldNested = res.root->child(11);
  ASSERT_NE(fieldNested, nullptr);
  EXPECT_FALSE(fieldNested->viewable);
  ASSERT_EQ(fieldNested->childCount, 2u);

  const auto* nestedChild = fieldNested->child(0);
  ASSERT_NE(nestedChild, nullptr);
  EXPECT_EQ(nestedChild->reason, bitsery::ot::FieldReason::CopyOnly);
  model::CopyOnlyChild coc{};
  nestedChild->constructInto(coc);
  EXPECT_EQ(coc.data, v.nested.child.data);

  const auto* nestedTail = fieldNested->child(1);
  ASSERT_NE(nestedTail, nullptr);
  EXPECT_TRUE(nestedTail->viewable);
  EXPECT_EQ(nestedTail->construct<uint32_t>(), v.nested.tail);

  // Visitor walk covers every node except the artificial root.
  std::vector<std::vector<uint16_t>> visited;
  auto status = bitsery::ot::inspectOffsetTable<model::KitchenSink>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() },
    [&](const bitsery::ot::FieldNode<bitsery::InputBufferAdapter<Buffer>>&,
        const std::vector<uint16_t>& path) { visited.push_back(path); });
  EXPECT_EQ(status, bitsery::ot::InspectStatus::Ok);
  ASSERT_EQ(visited.size(), 14u);
  EXPECT_EQ(visited.front(), (std::vector<uint16_t>{ 1 }));
  EXPECT_EQ(visited.back(), (std::vector<uint16_t>{ 12, 2 }));
}

TEST(OffsetTableInspect, SkipsConstructWhenReasonMismatch)
{
  model::Simple v{};
  v.a = 1234;
  v.b = 5678;

  auto buf = serializeWithOffsetTables(v);
  auto res = bitsery::ot::inspectOffsetTable<model::Simple>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  auto copy = res;
  ASSERT_NE(copy.root, nullptr);
  ASSERT_GE(copy.nodes.size(), 3u);

  auto& fieldA = copy.nodes[copy.root->firstChild];
  fieldA.reason = bitsery::ot::FieldReason::RegistryMismatch;

  uint32_t target = 0xFFFFFFFFu;
  fieldA.constructInto(target);
  EXPECT_EQ(target, 0xFFFFFFFFu);
}

TEST(OffsetTableInspect, ConstructDoesNotReadWhenPayloadMissing)
{
  model::Simple v{};
  v.a = 1;
  v.b = 2;

  auto buf = serializeWithOffsetTables(v);
  auto res = bitsery::ot::inspectOffsetTable<model::Simple>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  ASSERT_EQ(res.status, bitsery::ot::InspectStatus::Ok);
  ASSERT_NE(res.root, nullptr);
  auto copy = res;

  auto& fieldA = copy.nodes[copy.root->firstChild];
  fieldA.payloadSize = 0;

  uint32_t target = 42;
  fieldA.constructInto(target);
  EXPECT_EQ(target, 42u);
}

TEST(OffsetTableInspect, MissingTrailerFallsBack)
{
  model::Simple v{};
  v.a = 7;
  v.b = 9;

  Buffer buf;
  const auto written = bitsery::quickSerialization(
    bitsery::OutputBufferAdapter<Buffer>{ buf }, v);
  buf.resize(written);

  auto res = bitsery::ot::inspectOffsetTable<model::Simple>(
    buf.data(),
    buf.size(),
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.end() });

  EXPECT_EQ(res.status, bitsery::ot::InspectStatus::NoTrailer);
  EXPECT_EQ(res.root, nullptr);
}
