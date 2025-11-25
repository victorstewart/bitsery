// MIT License
//
// Copyright (c) 2024 Mindaugas Vinkelis
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

#ifndef BITSERY_DETAILS_OFFSET_TABLE_H
#define BITSERY_DETAILS_OFFSET_TABLE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "adapter_common.h"

namespace bitsery {

namespace details {

constexpr char TRAILER_MAGIC[8]{ 'B', 'T', 'S', 'Y', 'O', 'T', '0', '1' };
constexpr uint8_t TRAILER_VERSION = 1;

enum class TrailerFlags : uint8_t
{
  None = 0,
  OffsetsValid = 1 << 0,
  HasNestedTables = 1 << 1,
  CrossEndianDisallowed = 1 << 2
};

constexpr TrailerFlags
operator|(TrailerFlags lhs, TrailerFlags rhs)
{
  return static_cast<TrailerFlags>(static_cast<uint8_t>(lhs) |
                                   static_cast<uint8_t>(rhs));
}

inline TrailerFlags&
operator|=(TrailerFlags& lhs, TrailerFlags rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool
hasFlag(TrailerFlags value, TrailerFlags flag)
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0u;
}

#pragma pack(push, 1)
struct Trailer
{
  std::array<char, 8> magic;
  uint8_t version;
  uint8_t flags;
  uint16_t reserved;
  uint32_t rootTableOff;
};
#pragma pack(pop)

static_assert(sizeof(Trailer) == 16, "Invalid Trailer size");

enum class FieldKind : uint8_t
{
  Scalar,
  Array,
  Span,
  NestedStruct,
  NestedTable
};

enum class FieldFlags : uint8_t
{
  None = 0,
  Optional = 1 << 0,
  CopyOnly = 1 << 1,
  Aligned = 1 << 2
};

constexpr FieldFlags
operator|(FieldFlags lhs, FieldFlags rhs)
{
  return static_cast<FieldFlags>(static_cast<uint8_t>(lhs) |
                                 static_cast<uint8_t>(rhs));
}

constexpr FieldFlags
operator&(FieldFlags lhs, FieldFlags rhs)
{
  return static_cast<FieldFlags>(static_cast<uint8_t>(lhs) &
                                 static_cast<uint8_t>(rhs));
}

inline FieldFlags&
operator|=(FieldFlags& lhs, FieldFlags rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool
hasFlag(FieldFlags value, FieldFlags flag)
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0u;
}

#pragma pack(push, 1)
struct TableHdr
{
  uint16_t fieldCount;
  uint16_t typeVersion;
};
#pragma pack(pop)

static_assert(sizeof(TableHdr) == 4, "Invalid TableHdr size");

#pragma pack(push, 1)
struct Entry
{
  uint16_t fieldId;
  FieldKind kind;
  FieldFlags flags;
  uint32_t payloadOff;
  uint32_t size;
  uint32_t elemSize;
};
#pragma pack(pop)

static_assert(sizeof(Entry) == 16, "Invalid Entry size");

constexpr size_t InvalidTableIndex = std::numeric_limits<size_t>::max();

struct RecordedEntry
{
  uint16_t fieldId{};
  FieldKind kind{ FieldKind::Scalar };
  FieldFlags flags{ FieldFlags::None };
  size_t payloadBegin{};
  size_t payloadEnd{};
  uint32_t elemSize{};
  size_t nestedTableIdx{ InvalidTableIndex };
};

struct RecordedTable
{
  uint16_t typeVersion{ 0 };
  std::vector<RecordedEntry> entries;

  RecordedTable()
    : typeVersion{ 0 }
    , entries{}
  {}
  RecordedTable(const RecordedTable&) = default;
  RecordedTable& operator=(const RecordedTable&) = default;
  RecordedTable(RecordedTable&&) = default;
  RecordedTable& operator=(RecordedTable&&) = default;
};

struct FieldInfo
{
  uint16_t id;
  FieldKind kind;
  FieldFlags flags;
  size_t offset;
  size_t size;
  size_t align;
  uint16_t nestedFieldCount;
  uint16_t nestedTypeVersion;
  const FieldInfo* nestedEntries;
};

inline constexpr FieldInfo
makeField(uint16_t id,
          FieldKind kind,
          FieldFlags flags = FieldFlags::None,
          size_t offset = 0u,
          size_t size = 0u,
          size_t align = 1u,
          uint16_t nestedFieldCount = 0u,
          uint16_t nestedTypeVersion = 0u,
          const FieldInfo* nestedEntries = nullptr)
{
  return FieldInfo{ id,
                    kind,
                    flags,
                    offset,
                    size,
                    align,
                    nestedFieldCount,
                    nestedTypeVersion,
                    nestedEntries };
}

template<typename T>
constexpr FieldFlags
defaultFieldFlags()
{
  FieldFlags flags = FieldFlags::None;
  if (!std::is_trivially_copyable<T>::value ||
      std::is_pointer<T>::value || std::is_member_pointer<T>::value)
    flags |= FieldFlags::CopyOnly;
  if (alignof(T) > 1)
    flags |= FieldFlags::Aligned;
  return flags;
}

template<typename T>
struct FieldRegistry
{
  static constexpr bool Enabled = false;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 0;
  static constexpr const FieldInfo* entries() { return nullptr; }
};

// Forward declarations for stream adapters to specialize HasWrittenBytesCount.
} // namespace details
} // namespace bitsery

namespace bitsery {
template<typename TChar, typename Config, typename CharTraits>
class BasicOutputStreamAdapter;
template<typename TChar, typename Config, typename CharTraits, typename TBuffer>
class BasicBufferedOutputStreamAdapter;
}

namespace bitsery { namespace details {

template<typename Adapter, typename = void>
struct HasWrittenBytesCount : std::false_type
{};

template<typename Adapter, typename = void>
struct HasCurrentWritePos : std::false_type
{};

template<typename Adapter>
struct HasWrittenBytesCount<
  Adapter,
  std::void_t<decltype(std::declval<const Adapter&>().writtenBytesCount())>>
  : std::true_type
{};

template<typename C, typename Conf, typename Traits>
struct HasWrittenBytesCount<
  ::bitsery::BasicOutputStreamAdapter<C, Conf, Traits>,
  void> : std::false_type
{};

template<typename C, typename Conf, typename Traits, typename Buf>
struct HasWrittenBytesCount<
  ::bitsery::BasicBufferedOutputStreamAdapter<C, Conf, Traits, Buf>,
  void> : std::false_type
{};

template<typename Adapter>
struct HasCurrentWritePos<
  Adapter,
  std::void_t<decltype(std::declval<const Adapter&>().currentWritePos())>>
  : std::true_type
{};

template<typename C, typename Conf, typename Traits>
struct HasCurrentWritePos<
  ::bitsery::BasicOutputStreamAdapter<C, Conf, Traits>,
  void> : std::false_type
{};

template<typename C, typename Conf, typename Traits, typename Buf>
struct HasCurrentWritePos<
  ::bitsery::BasicBufferedOutputStreamAdapter<C, Conf, Traits, Buf>,
  void> : std::false_type
{};

class OffsetTableRecorder
{
public:
  using TableIndex = size_t;

  OffsetTableRecorder();

  void pushTable(uint16_t typeVersion = 0);
  TableIndex popTable();

  RecordedTable& currentTable();
  const RecordedTable& table(TableIndex idx) const;
  const RecordedTable* rootTable() const;
  TableIndex rootTableIndex() const { return _rootIndex; }

  void recordField(uint16_t fieldId,
                   FieldKind kind,
                   FieldFlags flags,
                   size_t payloadBegin,
                   size_t payloadEnd,
                   uint32_t elemSize,
                   TableIndex nestedTableIdx = InvalidTableIndex);

  const std::vector<RecordedTable>& completedTables() const
  {
    return _completed;
  }

  bool hasNestedTables() const { return _hasNestedTables; }
  bool hasOpenTable() const { return !_stack.empty(); }

private:
  std::vector<RecordedTable> _stack;
  std::vector<RecordedTable> _completed;
  TableIndex _rootIndex{ InvalidTableIndex };
  bool _hasNestedTables{ false };
};

inline OffsetTableRecorder::OffsetTableRecorder()
  : _stack{}
  , _completed{}
  , _rootIndex{ InvalidTableIndex }
  , _hasNestedTables{ false }
{}

inline void
OffsetTableRecorder::pushTable(uint16_t typeVersion)
{
  _stack.emplace_back();
  _stack.back().typeVersion = typeVersion;
  if (_stack.size() > 1u)
    _hasNestedTables = true;
}

inline OffsetTableRecorder::TableIndex
OffsetTableRecorder::popTable()
{
  assert(!_stack.empty());
  const auto idx = _completed.size();
  _completed.emplace_back(std::move(_stack.back()));
  _stack.pop_back();
  if (_stack.empty())
    _rootIndex = idx;
  return idx;
}

inline RecordedTable&
OffsetTableRecorder::currentTable()
{
  assert(!_stack.empty());
  return _stack.back();
}

inline const RecordedTable&
OffsetTableRecorder::table(TableIndex idx) const
{
  assert(idx < _completed.size());
  return _completed[idx];
}

inline const RecordedTable*
OffsetTableRecorder::rootTable() const
{
  return _rootIndex == InvalidTableIndex ? nullptr : &_completed[_rootIndex];
}

inline void
OffsetTableRecorder::recordField(uint16_t fieldId,
                                 FieldKind kind,
                                 FieldFlags flags,
                                 size_t payloadBegin,
                                 size_t payloadEnd,
                                 uint32_t elemSize,
                                 TableIndex nestedTableIdx)
{
  assert(!_stack.empty());
  assert(payloadEnd >= payloadBegin);
  auto& entries = _stack.back().entries;
  assert(entries.size() < std::numeric_limits<uint16_t>::max());
  auto& entry = entries.emplace_back();
  entry.fieldId = fieldId;
  entry.kind = kind;
  entry.flags = flags;
  entry.payloadBegin = payloadBegin;
  entry.payloadEnd = payloadEnd;
  entry.elemSize = elemSize;
  entry.nestedTableIdx = nestedTableIdx;
}

class TableScope
{
public:
  using TableIndex = OffsetTableRecorder::TableIndex;

  TableScope(OffsetTableRecorder& recorder, uint16_t typeVersion = 0)
    : _recorder{ std::addressof(recorder) }
    , _popped{ false }
  {
    _recorder->pushTable(typeVersion);
  }

  TableScope(const TableScope&) = delete;
  TableScope& operator=(const TableScope&) = delete;

  TableScope(TableScope&& other) noexcept
    : _recorder{ other._recorder }
    , _popped{ other._popped }
  {
    other._recorder = nullptr;
    other._popped = true;
  }

  TableScope& operator=(TableScope&& other) noexcept
  {
    if (this != std::addressof(other)) {
      finalize();
      _recorder = other._recorder;
      _popped = other._popped;
      other._recorder = nullptr;
      other._popped = true;
    }
    return *this;
  }

  ~TableScope() { finalize(); }

  TableIndex pop()
  {
    if (!_recorder)
      return InvalidTableIndex;
    assert(_recorder);
    _popped = true;
    auto idx = _recorder->popTable();
    _recorder = nullptr;
    return idx;
  }

  void cancel()
  {
    _popped = true;
    _recorder = nullptr;
  }

private:
  void finalize()
  {
    if (_recorder && !_popped) {
      _recorder->popTable();
    }
    _recorder = nullptr;
  }

  OffsetTableRecorder* _recorder;
  bool _popped;
};

template<typename Adapter>
class FieldOffsetScope
{
public:
  using TableIndex = typename OffsetTableRecorder::TableIndex;

  FieldOffsetScope() = default;

  FieldOffsetScope(OffsetTableRecorder& recorder,
                   Adapter& adapter,
                   uint16_t fieldId,
                   FieldKind kind,
                   FieldFlags flags,
                   uint32_t elemSize,
                   TableIndex nestedTableIdx = InvalidTableIndex)
    : _recorder{ std::addressof(recorder) }
    , _adapter{ std::addressof(adapter) }
    , _fieldId{ fieldId }
    , _kind{ kind }
    , _flags{ flags }
  , _elemSize{ elemSize }
  , _nestedTableIdx{ nestedTableIdx }
  , _begin{ 0 }
{
    if constexpr (HasCurrentWritePos<Adapter>::value) {
      _begin = adapter.currentWritePos();
    } else if constexpr (HasWrittenBytesCount<Adapter>::value) {
      _begin = adapter.writtenBytesCount();
    } else {
      _recorder = nullptr;
      _adapter = nullptr;
    }
  }

  FieldOffsetScope(const FieldOffsetScope&) = delete;
  FieldOffsetScope& operator=(const FieldOffsetScope&) = delete;

  FieldOffsetScope(FieldOffsetScope&& other) noexcept
    : _recorder{ other._recorder }
    , _adapter{ other._adapter }
    , _fieldId{ other._fieldId }
    , _kind{ other._kind }
    , _flags{ other._flags }
    , _elemSize{ other._elemSize }
    , _nestedTableIdx{ other._nestedTableIdx }
    , _begin{ other._begin }
  {
    other.cancel();
  }

  FieldOffsetScope& operator=(FieldOffsetScope&& other) noexcept
  {
    if (this != std::addressof(other)) {
      _recorder = other._recorder;
      _adapter = other._adapter;
      _fieldId = other._fieldId;
      _kind = other._kind;
      _flags = other._flags;
      _elemSize = other._elemSize;
      _nestedTableIdx = other._nestedTableIdx;
      _begin = other._begin;
      other.cancel();
    }
    return *this;
  }

  ~FieldOffsetScope()
  {
    if (_recorder) {
      size_t end = _begin;
      if constexpr (HasCurrentWritePos<Adapter>::value) {
        end = _adapter->currentWritePos();
      } else if constexpr (HasWrittenBytesCount<Adapter>::value) {
        end = _adapter->writtenBytesCount();
      } else {
        return;
      }
      _recorder->recordField(
        _fieldId, _kind, _flags, _begin, end, _elemSize, _nestedTableIdx);
    }
  }

  void nestedTableIdx(TableIndex idx) { _nestedTableIdx = idx; }

  void cancel()
  {
    _recorder = nullptr;
    _adapter = nullptr;
  }

private:
  OffsetTableRecorder* _recorder{};
  Adapter* _adapter{};
  uint16_t _fieldId{};
  FieldKind _kind{ FieldKind::Scalar };
  FieldFlags _flags{ FieldFlags::None };
  uint32_t _elemSize{};
  TableIndex _nestedTableIdx{ InvalidTableIndex };
  size_t _begin{};
};

inline Entry
toEntry(const RecordedEntry& src,
        const std::vector<uint32_t>* tableOffsets,
        size_t payloadSize)
{
  assert(src.payloadEnd >= src.payloadBegin);
  assert(src.payloadBegin <= std::numeric_limits<uint32_t>::max());
  assert((src.payloadEnd - src.payloadBegin) <=
         std::numeric_limits<uint32_t>::max());
  Entry dst{};
  dst.fieldId = src.fieldId;
  dst.kind = src.kind;
  dst.flags = src.flags;
  dst.payloadOff = static_cast<uint32_t>(src.payloadBegin);
  dst.size = static_cast<uint32_t>(src.payloadEnd - src.payloadBegin);
  if (src.kind == FieldKind::NestedTable &&
      src.nestedTableIdx != InvalidTableIndex) {
    assert(tableOffsets);
    assert(src.nestedTableIdx < tableOffsets->size());
    const auto nestedOff =
      payloadSize + static_cast<size_t>((*tableOffsets)[src.nestedTableIdx]);
    assert(nestedOff <= std::numeric_limits<uint32_t>::max());
    dst.elemSize = static_cast<uint32_t>(nestedOff);
  } else {
    dst.elemSize = src.elemSize;
  }
  return dst;
}

inline std::vector<uint8_t>
serializeTable(const RecordedTable& table,
               const std::vector<uint32_t>* tableOffsets,
               size_t payloadSize)
{
  assert(table.entries.size() <= std::numeric_limits<uint16_t>::max());
  const auto totalSize =
    sizeof(TableHdr) + table.entries.size() * sizeof(Entry);
  std::vector<uint8_t> buffer;
  buffer.resize(totalSize);

  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(table.entries.size());
  hdr.typeVersion = table.typeVersion;
  std::memcpy(buffer.data(), &hdr, sizeof(hdr));

  auto* out = buffer.data() + sizeof(hdr);
  for (const auto& src : table.entries) {
    auto dst = toEntry(src, tableOffsets, payloadSize);
    std::memcpy(out, &dst, sizeof(dst));
    out += sizeof(dst);
  }

  return buffer;
}

inline std::vector<uint8_t>
serializeTable(const RecordedTable& table)
{
  return serializeTable(table, nullptr, 0u);
}

inline size_t
serializedTableSize(const RecordedTable& table)
{
  return sizeof(TableHdr) + table.entries.size() * sizeof(Entry);
}

inline void
serializeTableToBuffer(const RecordedTable& table,
                       const std::vector<uint32_t>* tableOffsets,
                       size_t payloadSize,
                       uint8_t* out)
{
  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(table.entries.size());
  hdr.typeVersion = table.typeVersion;
  std::memcpy(out, &hdr, sizeof(hdr));
  auto* ptr = out + sizeof(hdr);
  for (const auto& src : table.entries) {
    auto dst = toEntry(src, tableOffsets, payloadSize);
    std::memcpy(ptr, &dst, sizeof(dst));
    ptr += sizeof(dst);
  }
}

struct StaticCacheEntry
{
  size_t payloadSize{ 0 };
  std::vector<uint8_t> postPayload;
  uint32_t rootPostOffset{ 0 };
  bool hasNested{ false };
  size_t signature{ 0 };

  StaticCacheEntry()
    : payloadSize{ 0 }
    , postPayload{}
    , rootPostOffset{ 0 }
    , hasNested{ false }
    , signature{ 0 }
  {}
  StaticCacheEntry(const StaticCacheEntry&) = default;
  StaticCacheEntry& operator=(const StaticCacheEntry&) = default;
  StaticCacheEntry(StaticCacheEntry&&) = default;
  StaticCacheEntry& operator=(StaticCacheEntry&&) = default;
};

inline size_t
hashTables(const std::vector<RecordedTable>& tables)
{
  uint64_t h = 1469598103934665603ull;
  auto mix = [&](uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
  };
  mix(static_cast<uint64_t>(tables.size()));
  for (const auto& t : tables) {
    mix(static_cast<uint64_t>(t.typeVersion));
    mix(static_cast<uint64_t>(t.entries.size()));
    for (const auto& e : t.entries) {
      mix(static_cast<uint64_t>(e.fieldId));
      mix(static_cast<uint64_t>(e.kind));
      mix(static_cast<uint64_t>(e.flags));
      mix(static_cast<uint64_t>(e.payloadBegin));
      mix(static_cast<uint64_t>(e.payloadEnd));
      mix(static_cast<uint64_t>(e.elemSize));
      mix(static_cast<uint64_t>(e.nestedTableIdx));
    }
  }
  return static_cast<size_t>(h);
}

struct OffsetTableWriterState
{
  OffsetTableRecorder recorder{};
  std::vector<uint8_t> postPayload;
  bool enabled{ true };
  const FieldInfo* rootEntries{ nullptr };
  size_t rootCount{ 0 };
  bool rootStatic{ false };
  bool captureEnabled{ false };
  struct CapturedEntry
  {
    uint16_t fieldId{};
    FieldKind kind{ FieldKind::Scalar };
    FieldFlags flags{ FieldFlags::None };
    size_t begin{};
    size_t end{};
    uint32_t elemSize{};
  };
  struct CaptureTable
  {
    uint16_t typeVersion{ 0 };
    std::vector<CapturedEntry> entries;
    void clear()
    {
      typeVersion = 0;
      entries.clear();
    }
  };
  CaptureTable capture{};

  struct Frame
  {
    using TableIndex = OffsetTableRecorder::TableIndex;
    const FieldInfo* entries{};
    size_t count{};
    size_t next{};
    TableScope scope;
    bool active{ false };
    bool hasAligned{ false };

  Frame(OffsetTableRecorder& rec,
        const FieldInfo* e,
        size_t c,
        uint16_t typeVersion)
    : entries{ e }
    , count{ c }
    , next{ 0 }
    , scope{ rec, typeVersion }
    , active{ true }
    , hasAligned{ false }
  {
    if (entries) {
      for (size_t i = 0; i < count; ++i) {
        const auto& f = entries[i];
        if (f.align > 1 || hasFlag(f.flags, FieldFlags::Aligned)) {
          hasAligned = true;
          break;
        }
      }
    }
  }
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;
  Frame(Frame&&) noexcept = default;
  Frame& operator=(Frame&&) noexcept = default;
};
  std::vector<Frame> frames{};

  void clear()
  {
    recorder = OffsetTableRecorder{};
    postPayload.clear();
    frames.clear();
    enabled = true;
    rootEntries = nullptr;
    rootCount = 0;
    rootStatic = false;
    captureEnabled = false;
    capture.clear();
  }
};

inline bool isStaticLayout(const FieldInfo* entries, size_t count);
inline std::unordered_map<const FieldInfo*, StaticCacheEntry>& staticTableCache();

template<typename T>
inline OffsetTableWriterState::Frame*
pushOffsetFrame(OffsetTableWriterState& state)
{
  if (!FieldRegistry<T>::Enabled || !state.enabled)
    return nullptr;
  constexpr auto kCount = FieldRegistry<T>::FieldCount;
  const auto* entries = FieldRegistry<T>::entries();
  if (state.frames.empty()) {
    state.rootEntries = entries;
    state.rootCount = kCount;
    state.rootStatic = isStaticLayout(entries, kCount);
    if (state.rootStatic) {
      auto& cache = staticTableCache();
      auto it = cache.find(entries);
      if (it != cache.end()) {
        // Cache hit: skip recording; finalize will emit cached tables.
        state.enabled = false;
        return nullptr;
      }
    }
    // Enable capture for flat layouts with no nested tables; fallback to recorder otherwise.
    bool hasNested = false;
    if (entries != nullptr) {
      for (size_t i = 0; i < kCount; ++i) {
        if (entries[i].nestedFieldCount > 0 ||
            entries[i].kind == FieldKind::NestedTable) {
          hasNested = true;
          break;
        }
      }
    }
    state.captureEnabled = (!state.rootStatic && !hasNested);
    if (state.captureEnabled) {
      state.capture.clear();
      state.capture.typeVersion = FieldRegistry<T>::TypeVersion;
    }
  }
  state.frames.emplace_back(state.recorder,
                            entries,
                            kCount,
                            FieldRegistry<T>::TypeVersion);
  return &state.frames.back();
}

inline OffsetTableWriterState::Frame*
currentOffsetFrame(OffsetTableWriterState& state)
{
  if (state.frames.empty() || !state.enabled)
    return nullptr;
  return &state.frames.back();
}

inline OffsetTableWriterState::Frame::TableIndex
popOffsetFrame(OffsetTableWriterState& state)
{
  assert(!state.frames.empty());
  auto idx = state.frames.back().scope.pop();
  state.frames.pop_back();
  return idx;
}

inline const FieldInfo*
nextField(OffsetTableWriterState::Frame& frame)
{
  if (!frame.active)
    return nullptr;
  if (frame.next >= frame.count || frame.entries == nullptr) {
    frame.active = false;
    return nullptr;
  }
  return std::addressof(frame.entries[frame.next++]);
}

inline void
disableCurrentFrame(OffsetTableWriterState& state)
{
  auto* frame = currentOffsetFrame(state);
  if (frame)
    frame->active = false;
  state.enabled = false;
}

inline bool
isStaticLayout(const FieldInfo* entries, size_t count)
{
  if (entries == nullptr || count == 0)
    return false;
  for (size_t i = 0; i < count; ++i) {
    const auto& f = entries[i];
    if (f.nestedFieldCount > 0)
      return false;
    if (f.kind == FieldKind::Span || f.kind == FieldKind::NestedTable)
      return false;
    if (hasFlag(f.flags, FieldFlags::CopyOnly))
      return false;
  }
  return true;
}

inline std::unordered_map<const FieldInfo*, StaticCacheEntry>&
staticTableCache()
{
  static std::unordered_map<const FieldInfo*, StaticCacheEntry> cache;
  return cache;
}

inline size_t
appendTableToPayload(const RecordedTable& table,
                     OffsetTableWriterState& state)
{
  auto blob = serializeTable(table);
  const auto offset = state.postPayload.size();
  state.postPayload.insert(
    state.postPayload.end(), blob.begin(), blob.end());
  return offset;
}

inline void
emitTableFromCapture(const OffsetTableWriterState::CaptureTable& cap,
                     const std::vector<uint32_t>* tableOffsets,
                     size_t payloadSize,
                     std::vector<uint8_t>& out)
{
  assert(cap.entries.size() <= std::numeric_limits<uint16_t>::max());
  const auto totalSize =
    sizeof(TableHdr) + cap.entries.size() * sizeof(Entry);
  out.resize(totalSize);
  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(cap.entries.size());
  hdr.typeVersion = cap.typeVersion;
  std::memcpy(out.data(), &hdr, sizeof(hdr));
  auto* ptr = out.data() + sizeof(hdr);
  for (const auto& src : cap.entries) {
    Entry dst{};
    dst.fieldId = src.fieldId;
    dst.kind = src.kind;
    dst.flags = src.flags;
    assert(src.begin <= std::numeric_limits<uint32_t>::max());
    assert((src.end - src.begin) <= std::numeric_limits<uint32_t>::max());
    dst.payloadOff = static_cast<uint32_t>(src.begin);
    dst.size = static_cast<uint32_t>(src.end - src.begin);
    dst.elemSize = src.elemSize;
    std::memcpy(ptr, &dst, sizeof(dst));
    ptr += sizeof(dst);
  }
}

template<typename Adapter>
inline size_t
writeTablesAndTrailer(Adapter& adapter,
                      OffsetTableWriterState& state,
                      size_t payloadSize)
{
  // Capture-based fast path for flat layouts.
  if (state.captureEnabled && !state.capture.entries.empty()) {
    state.postPayload.clear();
    std::vector<uint8_t> tableBuf;
    emitTableFromCapture(state.capture, nullptr, payloadSize, tableBuf);
    const uint32_t rootPostOffset = 0;
    state.postPayload.insert(
      state.postPayload.end(), tableBuf.begin(), tableBuf.end());

    const auto totalPayloadSize = payloadSize + state.postPayload.size();
    auto flags = TrailerFlags::OffsetsValid | TrailerFlags::CrossEndianDisallowed;

    Trailer trailer{};
    std::copy(std::begin(TRAILER_MAGIC),
              std::end(TRAILER_MAGIC),
              trailer.magic.begin());
    trailer.version = TRAILER_VERSION;
    trailer.flags = static_cast<uint8_t>(flags);
    trailer.reserved = 0;
    trailer.rootTableOff =
      static_cast<uint32_t>(payloadSize + static_cast<size_t>(rootPostOffset));

    if (!state.postPayload.empty()) {
      adapter.template writeBuffer<1>(
        state.postPayload.data(), state.postPayload.size());
    }
    adapter.template writeBuffer<1>(
      reinterpret_cast<const uint8_t*>(&trailer), sizeof(trailer));
    state.enabled = true;
    return payloadSize + state.postPayload.size() + sizeof(trailer);
  }

  const StaticCacheEntry* cached = nullptr;
  if (state.rootStatic && state.rootEntries != nullptr) {
    auto& cache = staticTableCache();
    auto it = cache.find(state.rootEntries);
    if (it != cache.end() && it->second.payloadSize == payloadSize &&
        !it->second.postPayload.empty()) {
      cached = std::addressof(it->second);
    }
  }

  if (!state.enabled) {
    if (cached != nullptr) {
      state.enabled = true;
      auto flags = TrailerFlags::OffsetsValid | TrailerFlags::CrossEndianDisallowed;
      if (cached->hasNested)
        flags |= TrailerFlags::HasNestedTables;

      Trailer trailer{};
      std::copy(std::begin(TRAILER_MAGIC),
                std::end(TRAILER_MAGIC),
                trailer.magic.begin());
      trailer.version = TRAILER_VERSION;
      trailer.flags = static_cast<uint8_t>(flags);
      trailer.reserved = 0;
      trailer.rootTableOff =
        static_cast<uint32_t>(payloadSize + static_cast<size_t>(cached->rootPostOffset));

      if (!cached->postPayload.empty()) {
        adapter.template writeBuffer<1>(
          cached->postPayload.data(), cached->postPayload.size());
      }
      adapter.template writeBuffer<1>(
        reinterpret_cast<const uint8_t*>(&trailer), sizeof(trailer));
      return payloadSize + cached->postPayload.size() + sizeof(trailer);
    } else {
      for (auto& frame : state.frames) {
        frame.scope.cancel();
      }
      state.frames.clear();
      state.recorder = OffsetTableRecorder{};
      state.postPayload.clear();
      state.enabled = true;
      return payloadSize;
    }
  }
  auto& recorder = state.recorder;
  while (recorder.hasOpenTable()) {
    recorder.popTable();
  }

  const auto& tables = recorder.completedTables();
  if (tables.empty())
    return payloadSize;

  const auto rootIdx = recorder.rootTableIndex();
  if (rootIdx == InvalidTableIndex || rootIdx >= tables.size())
    return payloadSize;

  static std::unordered_map<size_t, StaticCacheEntry> staticCache;
  const size_t signature = hashTables(tables);

  uint32_t rootPostOffset = 0;
  auto cacheIt = staticCache.find(signature);
  if (cacheIt != staticCache.end() &&
      cacheIt->second.payloadSize == payloadSize &&
      !cacheIt->second.postPayload.empty()) {
    state.postPayload = cacheIt->second.postPayload;
    rootPostOffset = cacheIt->second.rootPostOffset;
  } else {
    state.postPayload.clear();
    std::vector<uint32_t> tableOffsets(tables.size(), 0u);
    std::vector<size_t> writeOrder;
    writeOrder.reserve(tables.size());
    writeOrder.push_back(rootIdx);
    for (size_t idx = 0; idx < tables.size(); ++idx) {
      if (idx != rootIdx)
        writeOrder.push_back(idx);
    }

    size_t runningOffset = 0;
    for (auto idx : writeOrder) {
      const auto sz = serializedTableSize(tables[idx]);
      assert(runningOffset <= std::numeric_limits<uint32_t>::max());
      tableOffsets[idx] = static_cast<uint32_t>(runningOffset);
      runningOffset += sz;
    }

    state.postPayload.clear();
    state.postPayload.resize(runningOffset);
    for (auto idx : writeOrder) {
      const auto offset = static_cast<size_t>(tableOffsets[idx]);
      serializeTableToBuffer(tables[idx],
                             &tableOffsets,
                             payloadSize,
                             state.postPayload.data() + offset);
    }

    rootPostOffset = tableOffsets[rootIdx];

    StaticCacheEntry entry{};
    entry.payloadSize = payloadSize;
    entry.postPayload = state.postPayload;
    entry.rootPostOffset = rootPostOffset;
    entry.hasNested = recorder.hasNestedTables();
    entry.signature = signature;
    staticCache[signature] = std::move(entry);
    if (state.rootStatic && state.rootEntries != nullptr &&
        !recorder.hasNestedTables()) {
      auto& tableCache = staticTableCache();
      tableCache[state.rootEntries] = staticCache[signature];
    }
  }

  const auto totalPayloadSize = payloadSize + state.postPayload.size();
  assert(payloadSize <= std::numeric_limits<uint32_t>::max());
  assert(totalPayloadSize <= std::numeric_limits<uint32_t>::max());

  auto flags = TrailerFlags::OffsetsValid;
  if (recorder.hasNestedTables())
    flags |= TrailerFlags::HasNestedTables;
  flags |= TrailerFlags::CrossEndianDisallowed;

  Trailer trailer{};
  std::copy(
    std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), trailer.magic.begin());
  trailer.version = TRAILER_VERSION;
  trailer.flags = static_cast<uint8_t>(flags);
  trailer.reserved = 0;
  trailer.rootTableOff =
    static_cast<uint32_t>(payloadSize + static_cast<size_t>(rootPostOffset));

  if (!state.postPayload.empty()) {
    adapter.template writeBuffer<1>(
      state.postPayload.data(), state.postPayload.size());
  }
  adapter.template writeBuffer<1>(
    reinterpret_cast<const uint8_t*>(&trailer), sizeof(trailer));

  state.enabled = true;
  return payloadSize + state.postPayload.size() + sizeof(trailer);
}

struct TrailerInfo
{
  bool valid{};
  Trailer trailer{};
  size_t payloadSize{};
  size_t tablesSize{};
};

inline TrailerInfo
parseTrailer(const uint8_t* data, size_t size)
{
  TrailerInfo info{};
  if (size < sizeof(Trailer))
    return info;
  auto* trailerPos = data + (size - sizeof(Trailer));
  std::memcpy(&info.trailer, trailerPos, sizeof(Trailer));
  info.valid = std::equal(std::begin(TRAILER_MAGIC),
                          std::end(TRAILER_MAGIC),
                          info.trailer.magic.begin()) &&
               info.trailer.version == TRAILER_VERSION;
  if (!info.valid)
    return info;
  if (info.trailer.rootTableOff > size - sizeof(Trailer)) {
    info.valid = false;
    return info;
  }
  info.payloadSize = static_cast<size_t>(info.trailer.rootTableOff);
  info.tablesSize = size - sizeof(Trailer) - info.payloadSize;
  return info;
}

template<typename Config>
inline TrailerInfo
verifyTrailer(const uint8_t* data, size_t size)
{
  auto info = parseTrailer(data, size);
  if (!info.valid)
    return info;
  if (Config::Endianness != getSystemEndianness()) {
    info.valid = false;
    return info;
  }
  return info;
}

}

}

#endif // BITSERY_DETAILS_OFFSET_TABLE_H
