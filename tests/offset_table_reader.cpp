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

#include <gtest/gtest.h>
#include "bitsery/details/offset_table_reader.h"
#include "bitsery/details/offset_table.h"

using namespace bitsery;
using namespace bitsery::details;
using namespace bitsery::ot;

TEST(OffsetTableReader, FailsWithoutTrailer)
{
  std::vector<uint8_t> buf(8, 0);
  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_NE(res.status, VerifyResult::Ok);
}

TEST(OffsetTableReader, DetectsTrailerAndRoot)
{
  // minimal payload + empty table + trailer
  std::vector<uint8_t> buf;
  // payload: 4 bytes
  buf.resize(4, 0x11);
  // table (empty)
  TableHdr hdr{};
  hdr.fieldCount = 0;
  hdr.typeVersion = 0;
  const auto tableOff = buf.size();
  auto hdrPtr = reinterpret_cast<const uint8_t*>(&hdr);
  buf.insert(buf.end(), hdrPtr, hdrPtr + sizeof(hdr));
  // trailer
  Trailer tr{};
  std::copy(std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), tr.magic.begin());
  tr.version = TRAILER_VERSION;
  tr.flags = static_cast<uint8_t>(TrailerFlags::OffsetsValid |
                                  TrailerFlags::CrossEndianDisallowed);
  tr.rootTableOff = static_cast<uint32_t>(tableOff);
  auto trPtr = reinterpret_cast<const uint8_t*>(&tr);
  buf.insert(buf.end(), trPtr, trPtr + sizeof(tr));

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::Ok);
  ASSERT_NE(res.rootIndex, InvalidTableIndex);
  auto* root = rootTable(res);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->hdr.fieldCount, 0);
}
