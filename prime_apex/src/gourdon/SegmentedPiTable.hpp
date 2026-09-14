///
/// @file  SegmentedPiTable.hpp
/// @brief The A and C formulas in Xavier Gourdon's prime counting
///        algorithm require looking up PrimePi[x] values with
///        x < n^(1/2). Since a PrimePi[x] lookup table of size n^(1/2)
///        would use too much memory we need a segmented PrimePi[x]
///        lookup table that uses only O(n^(1/4)) memory.
///
///        The SegmentedPiTable class is a compressed lookup table of
///        prime counts. Since the size of SegmentedPiTable is very
///        small and will always fit into the CPU's cache, we don't
///        use a bit array with maximum compression because this adds
///        significant overhead. Instead we use a bit array where each
///        bit corresponds to an odd integer. This compression scheme
///        provides very fast access since the bit array index can be
///        calculated using a single right shift instruction.
///
///        The algorithm of the easy special leaves and the usage of
///        the SegmentedPiTable are described in more detail in:
///        https://github.com/kimwalisch/primecount/blob/master/doc/Easy-Special-Leaves.pdf
///
/// Copyright (C) 2026 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef SEGMENTEDPITABLE_HPP
#define SEGMENTEDPITABLE_HPP

#include <primesieve.hpp>
#include <macros.hpp>
#include <Vector.hpp>
#include <popcnt.hpp>

#include <stdint.h>
#include <algorithm>

namespace primecount {

class SegmentedPiTable
{
public:
  void init(uint64_t low, uint64_t high, uint64_t limit);

  uint64_t low() const
  {
    return low_;
  }

  uint64_t high() const
  {
    return high_;
  }

  static constexpr uint64_t numbers_per_byte()
  {
    return 128 / (sizeof(uint64_t) * 2);
  }

  /// Make sure size % 128 == 0
  static uint64_t align_segment_size(uint64_t size)
  {
    size = std::max<uint64_t>(128, size);

    if (size % 128)
      size += 128 - size % 128;

    return size;
  }

  /// Get number of primes <= x
  ALWAYS_INLINE uint64_t operator[](uint64_t x) const
  {
    ASSERT(x >= low_);
    ASSERT(x < high_);

    // Workaround needed for prime 2 since
    // we are sieving with primes >= 3.
    if (x < 2)
      return 0;

    x -= low_;
    uint64_t bits = bits_[x / 128];
    uint64_t bitmask = unset_larger_[x % 128];
    uint64_t count_bits = popcnt64(bits & bitmask);
    return pi_[x / 128] + count_bits;
  }

  /// Interleaved 4-way lookup for high ILP on Zen 5
  ALWAYS_INLINE uint64_t query4(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3) const
  {
    if_unlikely(x0 < 2 || x1 < 2 || x2 < 2 || x3 < 2)
      return (*this)[x0] + (*this)[x1] + (*this)[x2] + (*this)[x3];

    uint64_t d0 = x0 - low_;
    uint64_t d1 = x1 - low_;
    uint64_t d2 = x2 - low_;
    uint64_t d3 = x3 - low_;

    uint64_t b0 = bits_[d0 >> 7] & unset_larger_[d0 & 127];
    uint64_t b1 = bits_[d1 >> 7] & unset_larger_[d1 & 127];
    uint64_t b2 = bits_[d2 >> 7] & unset_larger_[d2 & 127];
    uint64_t b3 = bits_[d3 >> 7] & unset_larger_[d3 & 127];

    uint64_t c0 = popcnt64_native(b0) + pi_[d0 >> 7];
    uint64_t c1 = popcnt64_native(b1) + pi_[d1 >> 7];
    uint64_t c2 = popcnt64_native(b2) + pi_[d2 >> 7];
    uint64_t c3 = popcnt64_native(b3) + pi_[d3 >> 7];

    return (c0 + c1) + (c2 + c3);
  }

private:
  void init_bits(uint64_t limit);
  void init_count(uint64_t pi_low);

  static const Array<uint64_t, 128> unset_larger_;
  Vector<uint64_t> bits_;
  Vector<uint64_t> pi_;
  uint64_t low_ = 0;
  uint64_t high_ = 0;
  uint64_t next_prime_ = 0;
  primesieve::iterator iterator_;
};

} // namespace

#endif
