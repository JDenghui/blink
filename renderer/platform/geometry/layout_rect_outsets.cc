/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/geometry/layout_rect_outsets.h"

#include <algorithm>
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

void LayoutRectOutsets::ClampNegativeToZero() {
  top_ = top_.ClampNegativeToZero();
  right_ = right_.ClampNegativeToZero();
  bottom_ = bottom_.ClampNegativeToZero();
  left_ = left_.ClampNegativeToZero();
}

void LayoutRectOutsets::Unite(const LayoutRectOutsets& other) {
  top_ = std::max(top_, other.top_);
  right_ = std::max(right_, other.right_);
  bottom_ = std::max(bottom_, other.bottom_);
  left_ = std::max(left_, other.left_);
}

LayoutRectOutsets LayoutRectOutsets::LineOrientationOutsets(
    WritingMode writing_mode) const {
  if (!IsHorizontalWritingMode(writing_mode))
    return LayoutRectOutsets(left_, bottom_, right_, top_);
  return *this;
}

LayoutRectOutsets LayoutRectOutsets::LineOrientationOutsetsWithFlippedLines(
    WritingMode writing_mode) const {
  LayoutRectOutsets outsets = LineOrientationOutsets(writing_mode);
  if (IsFlippedLinesWritingMode(writing_mode))
    std::swap(outsets.top_, outsets.bottom_);
  return outsets;
}

}  // namespace blink
