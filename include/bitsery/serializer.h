// MIT License
//
// Copyright (c) 2017 Mindaugas Vinkelis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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

#ifndef BITSERY_SERIALIZER_H
#define BITSERY_SERIALIZER_H

#include "details/serialization_common.h"
#include "details/offset_table.h"
#include <algorithm>
#include <cassert>
#include <type_traits>

namespace bitsery { namespace ext { template<typename TContainer> class Entropy; } }
namespace bitsery { namespace ext { template<typename TBase> class BaseClass; } }
namespace bitsery { namespace ext { template<typename TBase> class VirtualBaseClass; } }
namespace bitsery { namespace ext { namespace pointer_utils {
template<template<typename> class, template<typename> class, typename>
class PointerObjectExtensionBase;
}} }

// forward declarations for stream adapters
template<typename TChar, typename Config, typename CharTraits>
class BasicOutputStreamAdapter;
template<typename TChar, typename Config, typename CharTraits, typename TBuffer>
class BasicBufferedOutputStreamAdapter;

namespace bitsery {

namespace details {

template<typename T>
struct IsStreamAdapter : std::false_type
{};

template<typename C, typename Conf, typename Traits>
struct IsStreamAdapter<BasicOutputStreamAdapter<C, Conf, Traits>>
  : std::true_type
{};

template<typename C, typename Conf, typename Traits, typename Buf>
struct IsStreamAdapter<BasicBufferedOutputStreamAdapter<C, Conf, Traits, Buf>>
  : std::true_type
{};

}

template<typename TOutputAdapter, typename TContext = void>
class Serializer
  : public details::AdapterAndContextRef<TOutputAdapter, TContext>
{
public:
  // helper type, that always returns bit-packing enabled type, useful inside
  // serialize function when enabling bitpacking
  using BPEnabledType =
    Serializer<typename TOutputAdapter::BitPackingEnabled, TContext>;
  using TConfig = typename TOutputAdapter::TConfig;

  struct EmptyOffsetScope
  {
    void nestedTableIdx(details::OffsetTableRecorder::TableIndex) {}
    void cancel() {}
  };

  using OffsetScope =
    typename std::conditional<details::IsStreamAdapter<TOutputAdapter>::value,
                              EmptyOffsetScope,
                              details::FieldOffsetScope<TOutputAdapter>>::type;

  using details::AdapterAndContextRef<TOutputAdapter,
                                      TContext>::AdapterAndContextRef;

  /*
   * object function
   */
  template<typename T>
  void object(const T& obj)
  {
    auto _bits_ot_field_scope =
      makeOffsetFieldScope(details::FieldKind::NestedTable,
                           details::defaultFieldFlags<T>() |
                             details::FieldFlags::None,
                           0u);
    auto _bits_ot_type_scope = makeOffsetTypeScope<T>();
    details::SerializeFunction<Serializer, T>::invoke(*this,
                                                      const_cast<T&>(obj));
    auto _bits_ot_nested_idx = _bits_ot_type_scope.pop();
    if (_bits_ot_nested_idx != details::InvalidTableIndex)
      _bits_ot_field_scope.nestedTableIdx(_bits_ot_nested_idx);
    else
      _bits_ot_field_scope.cancel();
  }

  template<typename T, typename Fnc>
  void object(const T& obj, Fnc&& fnc)
  {
    auto _bits_ot_field_scope =
      makeOffsetFieldScope(details::FieldKind::NestedTable,
                           details::defaultFieldFlags<T>() |
                             details::FieldFlags::None,
                           0u);
    auto _bits_ot_type_scope = makeOffsetTypeScope<T>();
    fnc(*this, const_cast<T&>(obj));
    auto _bits_ot_nested_idx = _bits_ot_type_scope.pop();
    if (_bits_ot_nested_idx != details::InvalidTableIndex)
      _bits_ot_field_scope.nestedTableIdx(_bits_ot_nested_idx);
    else
      _bits_ot_field_scope.cancel();
  }

  /*
   * functionality, that enables simpler serialization syntax, by including
   * additional header
   */

  template<typename... TArgs>
  Serializer& operator()(TArgs&&... args)
  {
    archive(std::forward<TArgs>(args)...);
    return *this;
  }

  /*
   * value overloads
   */

  template<size_t VSIZE, typename T>
  void value(const T& v)
  {
    static_assert(details::IsFundamentalType<T>::value,
                  "Value must be integral, float or enum type.");
    using TValue = typename details::IntegralFromFundamental<T>::TValue;
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(details::FieldKind::Scalar,
                           details::FieldFlags::None,
                           sizeof(TValue));
    this->_adapter.template writeBytes<VSIZE>(
      reinterpret_cast<const TValue&>(v));
  }

  /*
   * enable bit-packing
   */
  template<typename Fnc>
  void enableBitPacking(Fnc&& fnc)
  {
    disableOffsetRecording();
    procEnableBitPacking(
      std::forward<Fnc>(fnc),
      std::is_same<TOutputAdapter,
                   typename TOutputAdapter::BitPackingEnabled>{},
      std::integral_constant<bool, Serializer::HasContext>{});
  }

  /*
   * extension functions
   */

  template<typename T, typename Ext, typename Fnc>
  void ext(const T& obj, const Ext& extension, Fnc&& fnc)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportLambdaOverload,
                  "extension doesn't support overload with lambda");
    if (IsUnsupportedExt<Ext>::value)
      disableOffsetRecording();
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::NestedStruct,
      details::FieldFlags::CopyOnly | details::defaultFieldFlags<T>(),
      0u);
    extension.serialize(*this, obj, std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportValueOverload,
                  "extension doesn't support overload with `value<N>`");
    if (IsUnsupportedExt<Ext>::value)
      disableOffsetRecording();
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::NestedStruct,
      details::FieldFlags::CopyOnly | details::defaultFieldFlags<T>(),
      0u);
    extension.serialize(
      *this, obj, [](Serializer& s, VType& v) { s.value<VSIZE>(v); });
  }

  template<typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(details::IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportObjectOverload,
                  "extension doesn't support overload with `object`");
    if (IsUnsupportedExt<Ext>::value)
      disableOffsetRecording();
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            details::DummyType,
                                            ExtVType>::type;
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::NestedStruct,
      details::FieldFlags::CopyOnly | details::defaultFieldFlags<T>(),
      0u);
    extension.serialize(
      *this, obj, [](Serializer& s, VType& v) { s.object(v); });
  }

  /*
   * boolValue
   */

  void boolValue(bool v)
  {
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(details::FieldKind::Scalar,
                           details::defaultFieldFlags<unsigned char>() |
                             details::FieldFlags::None,
                           1u);
    procBoolValue(v,
                  std::is_same<TOutputAdapter,
                               typename TOutputAdapter::BitPackingEnabled>{});
  }

  /*
   * text overloads
   */

  template<size_t VSIZE, typename T>
  void text(const T& str, size_t maxSize)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use text(const T&) overload without `maxSize` for static container");
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags<
        typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    procText<VSIZE>(str, maxSize);
  }

  template<size_t VSIZE, typename T>
  void text(const T& str)
  {
    static_assert(
      details::IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use text(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags<
        typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    procText<VSIZE>(str, traits::ContainerTraits<T>::size(str));
  }

  /*
   * container overloads
   */

  // dynamic size containers

  template<typename T, typename Fnc>
  void container(const T& obj, size_t maxSize, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, Fnc) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags< typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    details::writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    static_assert(VSIZE > 0, "");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags< typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));;
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    details::writeSize(this->_adapter, size);

    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize; // unused in release
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags< typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));;
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    details::writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj));
  }

  // fixed size containers

  template<
    typename T,
    typename Fnc,
    typename std::enable_if<!std::is_integral<Fnc>::value>::type* = nullptr>
  void container(const T& obj, Fnc&& fnc)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t, Fnc) overload with "
                  "`maxSize` for dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags<
        typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    static_assert(VSIZE > 0, "");
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags<
        typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj)
  {
    static_assert(
      details::IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope = makeOffsetFieldScope(
      details::FieldKind::Array,
      details::defaultFieldFlags<
        typename traits::ContainerTraits<T>::TValue>(),
      sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer(std::begin(obj), std::end(obj));
  }

  // overloads for functions with explicit type size

  template<typename T>
  void value1b(T&& v)
  {
    value<1>(std::forward<T>(v));
  }

  template<typename T>
  void value2b(T&& v)
  {
    value<2>(std::forward<T>(v));
  }

  template<typename T>
  void value4b(T&& v)
  {
    value<4>(std::forward<T>(v));
  }

  template<typename T>
  void value8b(T&& v)
  {
    value<8>(std::forward<T>(v));
  }

  template<typename T>
  void value16b(T&& v)
  {
    value<16>(std::forward<T>(v));
  }

  template<typename T, typename Ext>
  void ext1b(const T& v, Ext&& extension)
  {
    ext<1, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext2b(const T& v, Ext&& extension)
  {
    ext<2, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext4b(const T& v, Ext&& extension)
  {
    ext<4, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext8b(const T& v, Ext&& extension)
  {
    ext<8, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext16b(const T& v, Ext&& extension)
  {
    ext<16, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T>
  void text1b(const T& str, size_t maxSize)
  {
    text<1>(str, maxSize);
  }

  template<typename T>
  void text2b(const T& str, size_t maxSize)
  {
    text<2>(str, maxSize);
  }

  template<typename T>
  void text4b(const T& str, size_t maxSize)
  {
    text<4>(str, maxSize);
  }

  template<typename T>
  void text1b(const T& str)
  {
    text<1>(str);
  }

  template<typename T>
  void text2b(const T& str)
  {
    text<2>(str);
  }

  template<typename T>
  void text4b(const T& str)
  {
    text<4>(str);
  }

  template<typename T>
  void container1b(T&& obj, size_t maxSize)
  {
    container<1>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container2b(T&& obj, size_t maxSize)
  {
    container<2>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container4b(T&& obj, size_t maxSize)
  {
    container<4>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container8b(T&& obj, size_t maxSize)
  {
    container<8>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container16b(T&& obj, size_t maxSize)
  {
    container<16>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container1b(T&& obj)
  {
    container<1>(std::forward<T>(obj));
  }

  template<typename T>
  void container2b(T&& obj)
  {
    container<2>(std::forward<T>(obj));
  }

  template<typename T>
  void container4b(T&& obj)
  {
    container<4>(std::forward<T>(obj));
  }

  template<typename T>
  void container8b(T&& obj)
  {
    container<8>(std::forward<T>(obj));
  }

  template<typename T>
  void container16b(T&& obj)
  {
    container<16>(std::forward<T>(obj));
  }

private:
  // process value types
  // false_type means that we must process all elements individually
  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::false_type)
  {
    for (; first != last; ++first)
      value<VSIZE>(*first);
  }

  // process value types
  // true_type means, that we can copy whole buffer
  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::true_type)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    using TIntegral = typename details::IntegralFromFundamental<TValue>::TValue;
    if (first != last)
      this->_adapter.template writeBuffer<VSIZE>(
        reinterpret_cast<const TIntegral*>(&(*first)),
        static_cast<size_t>(std::distance(first, last)));
  }

  // process by calling functions
  template<typename It, typename Fnc>
  void procContainer(It first, It last, Fnc fnc)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    for (; first != last; ++first) {
      fnc(*this, const_cast<TValue&>(*first));
    }
  }

  // process text,
  template<size_t VSIZE, typename T>
  void procText(const T& str, size_t maxSize)
  {
    const size_t length = traits::TextTraits<T>::length(str);
    (void)maxSize; // unused in release
    assert((length + (traits::TextTraits<T>::addNUL ? 1u : 0u)) <= maxSize);
    details::writeSize(this->_adapter, length);
    auto begin = std::begin(str);
    using diff_t =
      typename std::iterator_traits<decltype(begin)>::difference_type;
    procContainer<VSIZE>(
      begin,
      std::next(begin, static_cast<diff_t>(length)),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  // process object types
  template<typename It>
  void procContainer(It first, It last)
  {
    for (; first != last; ++first)
      object(*first);
  }

  // proc bool writing bit or byte, depending on if BitPackingEnabled or not
  void procBoolValue(bool v, std::true_type)
  {
    this->_adapter.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
  }

  void procBoolValue(bool v, std::false_type)
  {
    this->_adapter.template writeBytes<1>(
      static_cast<unsigned char>(v ? 1 : 0));
  }

  // enable bit-packing or do nothing if it is already enabled
  template<typename Fnc, typename HasContext>
  void procEnableBitPacking(const Fnc& fnc, std::true_type, HasContext)
  {
    fnc(*this);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type, std::true_type)
  {
    BPEnabledType ser{ this->_context, this->_adapter };
    fnc(ser);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type, std::false_type)
  {
    BPEnabledType ser{ this->_adapter };
    fnc(ser);
  }

private:
  details::OffsetTableWriterState*
  offsetTableState()
  {
    if constexpr (details::IsStreamAdapter<TOutputAdapter>::value)
      return nullptr;
    return this->template contextOrNull<details::OffsetTableWriterState>();
  }

  template<typename Ext>
  struct IsUnsupportedExt : std::false_type
  {};

  template<typename TContainer>
  struct IsUnsupportedExt<::bitsery::ext::Entropy<TContainer>>
    : std::true_type
  {};

  template<template<typename> class TPtrManager,
           template<typename> class TPolyCtx,
           typename RTTI>
  struct IsUnsupportedExt<
    ::bitsery::ext::pointer_utils::PointerObjectExtensionBase<TPtrManager,
                                                              TPolyCtx,
                                                              RTTI>>
    : std::true_type
  {};

  template<typename TBase>
  struct IsUnsupportedExt<::bitsery::ext::BaseClass<TBase>> : std::true_type
  {};

  template<typename TBase>
  struct IsUnsupportedExt<::bitsery::ext::VirtualBaseClass<TBase>>
    : std::true_type
  {};

  struct OffsetRecordingPause
  {
    details::OffsetTableWriterState* state{};
    bool prev{};

    explicit OffsetRecordingPause(details::OffsetTableWriterState* s)
      : state{ s }
    {
      if (state) {
        prev = state->enabled;
        state->enabled = false;
      }
    }

    OffsetRecordingPause(const OffsetRecordingPause&) = delete;
    OffsetRecordingPause& operator=(const OffsetRecordingPause&) = delete;
    OffsetRecordingPause(OffsetRecordingPause&& other) noexcept
      : state{ other.state }
      , prev{ other.prev }
    {
      other.state = nullptr;
    }
    OffsetRecordingPause& operator=(OffsetRecordingPause&& other) noexcept
    {
      if (this != std::addressof(other)) {
        restore();
        state = other.state;
        prev = other.prev;
        other.state = nullptr;
      }
      return *this;
    }

    ~OffsetRecordingPause() { restore(); }

  private:
    void restore()
    {
      if (state) {
        state->enabled = prev;
        state = nullptr;
      }
    }
  };

  OffsetRecordingPause pauseOffsetRecording()
  {
    return OffsetRecordingPause{ offsetTableState() };
  }

  struct OffsetTypeScope
  {
    details::OffsetTableWriterState* state{};
    details::OffsetTableWriterState::Frame* frame{};

    details::OffsetTableWriterState::Frame::TableIndex pop()
    {
      if (!frame || !state || !state->enabled)
        return details::InvalidTableIndex;
      return details::popOffsetFrame(*state);
    }
  };

  template<typename T>
  OffsetTypeScope makeOffsetTypeScope()
  {
    OffsetTypeScope res{};
    auto* st = offsetTableState();
    if (!st || !st->enabled)
      return res;
    if constexpr (details::IsStreamAdapter<TOutputAdapter>::value) {
      st->enabled = false;
      return res;
    }
    if (TConfig::Endianness != details::getSystemEndianness()) {
      st->enabled = false;
      return res;
    }
    if (!details::FieldRegistry<T>::Enabled) {
      st->enabled = false;
      return res;
    }
    if (details::FieldRegistry<T>::FieldCount > 0 &&
        details::FieldRegistry<T>::entries() == nullptr) {
      st->enabled = false;
      return res;
    }
    res.state = st;
    res.frame = details::pushOffsetFrame<T>(*st);
    return res;
  }

  OffsetScope
  makeOffsetFieldScope(details::FieldKind kind,
                       details::FieldFlags flags,
                       uint32_t elemSize)
  {
    auto scope = makeOffsetFieldScopeImpl<TOutputAdapter>(kind, flags, elemSize);
    // If capture is enabled, we rely on finalize to fill end offsets.
    return scope;
  }

  template<typename TA>
  OffsetScope
  makeOffsetFieldScopeImpl(details::FieldKind,
                           details::FieldFlags,
                           uint32_t,
                           typename std::enable_if<
                             details::IsStreamAdapter<TA>::value,
                             int>::type* = nullptr)
  {
    auto* st = offsetTableState();
    if (st)
      st->enabled = false;
    return {};
  }

  template<typename TA>
  OffsetScope
  makeOffsetFieldScopeImpl(details::FieldKind kind,
                           details::FieldFlags flags,
                           uint32_t elemSize,
                           typename std::enable_if<
                             !details::IsStreamAdapter<TA>::value,
                             int>::type* = nullptr)
  {
    auto* st = offsetTableState();
    if (!st || !st->enabled)
      return {};
    auto* frame = details::currentOffsetFrame(*st);
    if (!frame)
      return {};
    auto* info = details::nextField(*frame);
    if (!info) {
      details::disableCurrentFrame(*st);
      return {};
    }
    if (frame->hasAligned &&
        details::hasFlag(info->flags, details::FieldFlags::Aligned) &&
        info->align > 1 &&
        details::HasCurrentWritePos<TOutputAdapter>::value) {
      const auto curr = this->_adapter.currentWritePos();
      const auto padding =
        static_cast<size_t>((info->align - (curr % info->align)) %
                            info->align);
      if (padding > 0)
        this->_adapter.currentWritePos(curr + padding);
    }
    if (info->kind != kind) {
      details::disableCurrentFrame(*st);
      return {};
    }
    auto mergedFlags = info->flags | flags;
    if (st->captureEnabled) {
      size_t begin = 0;
      if constexpr (details::HasCurrentWritePos<TOutputAdapter>::value) {
        begin = this->_adapter.currentWritePos();
      } else if constexpr (details::HasWrittenBytesCount<TOutputAdapter>::value) {
        begin = this->_adapter.writtenBytesCount();
      }
      details::OffsetTableWriterState::CapturedEntry entry{};
      entry.fieldId = info->id;
      entry.kind = info->kind;
      entry.flags = mergedFlags;
      entry.begin = begin;
      entry.elemSize = elemSize;
      st->capture.entries.push_back(entry);
      return {};
    }
    if (st->captureEnabled) {
      // Capture begin now; end will be captured on next field or finalize.
      size_t begin = 0;
      if constexpr (details::HasCurrentWritePos<TOutputAdapter>::value) {
        begin = this->_adapter.currentWritePos();
      } else if constexpr (details::HasWrittenBytesCount<TOutputAdapter>::value) {
        begin = this->_adapter.writtenBytesCount();
      }
      details::OffsetTableWriterState::CapturedEntry entry{};
      entry.fieldId = info->id;
      entry.kind = info->kind;
      entry.flags = mergedFlags;
      entry.begin = begin;
      entry.elemSize = elemSize;
      st->capture.entries.push_back(entry);
      return {};
    }
    return details::FieldOffsetScope<TOutputAdapter>(
      st->recorder, this->_adapter, info->id, info->kind, mergedFlags, elemSize);
  }

  void disableOffsetRecording()
  {
    auto* st = offsetTableState();
    if (st)
      details::disableCurrentFrame(*st);
  }

public:
  // these are dummy functions for extensions that have TValue = void
  void object(const details::DummyType&) {}

  template<size_t VSIZE>
  void value(const details::DummyType&)
  {
  }

  template<typename T, typename... TArgs>
  void archive(T&& head, TArgs&&... tail)
  {
    // serialize object
    details::BriefSyntaxFunction<Serializer, T>::invoke(*this,
                                                        std::forward<T>(head));
    // expand other elements
    archive(std::forward<TArgs>(tail)...);
  }
  // dummy function, that stops archive variadic arguments expansion
  void archive() {}
};

// helper function that set ups all the basic steps and after serialziation
// returns serialized bytes count
template<typename OutputAdapter, typename T>
size_t
quickSerialization(OutputAdapter adapter, const T& value)
{
  Serializer<OutputAdapter> ser{ std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  return ser.adapter().writtenBytesCount();
}

template<typename Context, typename OutputAdapter, typename T>
size_t
quickSerialization(Context& ctx, OutputAdapter adapter, const T& value)
{
  Serializer<OutputAdapter, Context> ser{ ctx, std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  return ser.adapter().writtenBytesCount();
}

}
#endif // BITSERY_SERIALIZER_H
