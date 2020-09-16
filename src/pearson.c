/**
 * (C) 2007-20 - ntop.org and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not see see <http://www.gnu.org/licenses/>
 *
 */

// taken from https://github.com/Logan007/pearson
// This is free and unencumbered software released into the public domain.


#include "pearson.h"


// AES S-Box table -- allows for eventually supported hardware accelerated look-up
static const uint8_t t[256] ={
 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

/*
// table as in original paper "Fast Hashing of Variable-Length Text Strings" by Peter K. Pearson
// as published in The Communications of the ACM  Vol.33, No.  6 (June 1990), pp. 677-680.
static const uint8_t t[256] ={
  0x01, 0x57, 0x31, 0x0c, 0xb0, 0xb2, 0x66, 0xa6, 0x79, 0xc1, 0x06, 0x54, 0xf9, 0xe6, 0x2c, 0xa3,
  0x0e, 0xc5, 0xd5, 0xb5, 0xa1, 0x55, 0xda, 0x50, 0x40, 0xef, 0x18, 0xe2, 0xec, 0x8e, 0x26, 0xc8,
  0x6e, 0xb1, 0x68, 0x67, 0x8d, 0xfd, 0xff, 0x32, 0x4d, 0x65, 0x51, 0x12, 0x2d, 0x60, 0x1f, 0xde,
  0x19, 0x6b, 0xbe, 0x46, 0x56, 0xed, 0xf0, 0x22, 0x48, 0xf2, 0x14, 0xd6, 0xf4, 0xe3, 0x95, 0xeb,
  0x61, 0xea, 0x39, 0x16, 0x3c, 0xfa, 0x52, 0xaf, 0xd0, 0x05, 0x7f, 0xc7, 0x6f, 0x3e, 0x87, 0xf8,
  0xae, 0xa9, 0xd3, 0x3a, 0x42, 0x9a, 0x6a, 0xc3, 0xf5, 0xab, 0x11, 0xbb, 0xb6, 0xb3, 0x00, 0xf3,
  0x84, 0x38, 0x94, 0x4b, 0x80, 0x85, 0x9e, 0x64, 0x82, 0x7e, 0x5b, 0x0d, 0x99, 0xf6, 0xd8, 0xdb,
  0x77, 0x44, 0xdf, 0x4e, 0x53, 0x58, 0xc9, 0x63, 0x7a, 0x0b, 0x5c, 0x20, 0x88, 0x72, 0x34, 0x0a,
  0x8a, 0x1e, 0x30, 0xb7, 0x9c, 0x23, 0x3d, 0x1a, 0x8f, 0x4a, 0xfb, 0x5e, 0x81, 0xa2, 0x3f, 0x98,
  0xaa, 0x07, 0x73, 0xa7, 0xf1, 0xce, 0x03, 0x96, 0x37, 0x3b, 0x97, 0xdc, 0x5a, 0x35, 0x17, 0x83,
  0x7d, 0xad, 0x0f, 0xee, 0x4f, 0x5f, 0x59, 0x10, 0x69, 0x89, 0xe1, 0xe0, 0xd9, 0xa0, 0x25, 0x7b,
  0x76, 0x49, 0x02, 0x9d, 0x2e, 0x74, 0x09, 0x91, 0x86, 0xe4, 0xcf, 0xd4, 0xca, 0xd7, 0x45, 0xe5,
  0x1b, 0xbc, 0x43, 0x7c, 0xa8, 0xfc, 0x2a, 0x04, 0x1d, 0x6c, 0x15, 0xf7, 0x13, 0xcd, 0x27, 0xcb,
  0xe9, 0x28, 0xba, 0x93, 0xc6, 0xc0, 0x9b, 0x21, 0xa4, 0xbf, 0x62, 0xcc, 0xa5, 0xb4, 0x75, 0x4c,
  0x8c, 0x24, 0xd2, 0xac, 0x29, 0x36, 0x9f, 0x08, 0xb9, 0xe8, 0x71, 0xc4, 0xe7, 0x2f, 0x92, 0x78,
  0x33, 0x41, 0x1c, 0x90, 0xfe, 0xdd, 0x5d, 0xbd, 0xc2, 0x8b, 0x70, 0x2b, 0x47, 0x6d, 0xb8, 0xd1 };
*/


#if defined (__AES__) && defined (__SSSE3__) // AES-NI & SSSE3 ----------------------------


void pearson_hash_256 (uint8_t *out, const uint8_t *in, size_t len) {

	size_t i;

        uint8_t upper[8] = { 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
        uint8_t lower[8] = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

        uint64_t upper_hash_mask = *(uint64_t*)&upper;
        uint64_t lower_hash_mask = *(uint64_t*)&lower;

        __m128i tmp = _mm_set1_epi8(0x10);

        __m128i hash_mask = _mm_set_epi64 ((__m64)lower_hash_mask, (__m64)upper_hash_mask);
        __m128i high_hash_mask = _mm_xor_si128 (tmp, hash_mask);
        __m128i hash= _mm_setzero_si128();
        __m128i high_hash= _mm_setzero_si128(); hash;

	__m128i ZERO = _mm_setzero_si128();
	__m128i ISOLATE_SBOX_MASK = _mm_set_epi32(0x0306090C, 0x0F020508, 0x0B0E0104, 0x070A0D00);

        for (i = 0; i < len; i++) {
                // broadcast the character, xor into hash, make them different permutations
                __m128i cc = _mm_set1_epi8 (in[i]);
                hash = _mm_xor_si128 (hash, cc);
                high_hash = _mm_xor_si128 (high_hash, cc);
                hash = _mm_xor_si128 (hash, hash_mask);
                high_hash = _mm_xor_si128 (high_hash, high_hash_mask);
                // table lookup
                hash = _mm_shuffle_epi8(hash, ISOLATE_SBOX_MASK);           // re-order along AES round
                high_hash = _mm_shuffle_epi8(high_hash, ISOLATE_SBOX_MASK); // re-order along AES round
                hash = _mm_aesenclast_si128(hash, ZERO);
                high_hash = _mm_aesenclast_si128(high_hash, ZERO);
        }
        // store output
        _mm_store_si128 ((__m128i*)out , high_hash);
        _mm_store_si128 ((__m128i*)&out[16] , hash);
}


void pearson_hash_128 (uint8_t *out, const uint8_t *in, size_t len) {

        size_t i;

        uint8_t upper[8] = { 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
        uint8_t lower[8] = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

        uint64_t upper_hash_mask = *(uint64_t*)&upper;
        uint64_t lower_hash_mask = *(uint64_t*)&lower;

        __m128i hash_mask = _mm_set_epi64 ((__m64)lower_hash_mask, (__m64)upper_hash_mask);
        __m128i hash = _mm_setzero_si128(); // _mm_xor_si128 (hash, hash);

        __m128i ZERO = _mm_setzero_si128();
        __m128i ISOLATE_SBOX_MASK = _mm_set_epi32(0x0306090C, 0x0F020508, 0x0B0E0104, 0x070A0D00);

        for (i = 0; i < len; i++) {
                // broadcast the character, xor into hash, make them different permutations
                __m128i cc = _mm_set1_epi8 (in[i]);
                hash = _mm_xor_si128 (hash, cc);
                hash = _mm_xor_si128 (hash, hash_mask);
                // table lookup
                hash = _mm_shuffle_epi8(hash, ISOLATE_SBOX_MASK); // re-order along AES round
                hash = _mm_aesenclast_si128(hash, ZERO);
        }
        // store output
        _mm_store_si128 ((__m128i*)out , hash);
}


// so far, only used internally
static uint64_t pearson_hash_64 (const uint8_t *in, size_t len) {

        size_t i;

        __m128i hash_mask = _mm_cvtsi64_si128(0x0706050403020100);

        __m128i hash = _mm_setzero_si128();

        __m128i ZERO = _mm_setzero_si128();
        __m128i ISOLATE_SBOX_MASK = _mm_set_epi32(0x0306090C, 0x0F020508, 0x0B0E0104, 0x070A0D00);

        for (i = 0; i < len; i++) {
                // broadcast the character, xor into hash, make them different permutations
                __m128i cc = _mm_set1_epi8 (in[i]);
                hash = _mm_xor_si128 (hash, cc);
                hash = _mm_xor_si128 (hash, hash_mask);
                // table lookup
                hash = _mm_shuffle_epi8(hash, ISOLATE_SBOX_MASK); // re-order along AES round
                hash = _mm_aesenclast_si128(hash, ZERO);
        }
        // store output
	return _mm_cvtsi128_si64 (hash);
}


uint32_t pearson_hash_32 (const uint8_t *in, size_t len) {

  return pearson_hash_64(in, len);
}


uint16_t pearson_hash_16 (const uint8_t *in, size_t len) {

  return pearson_hash_64(in, len);
}


#else // plain C --------------------------------------------------------------------------


static uint16_t t16[65536]; // 16-bit look-up table

#define ROR64(x,r) (((x)>>(r))|((x)<<(64-(r))))
#define ROR32(x,r) (((x)>>(r))|((x)<<(32-(r))))


void pearson_hash_256 (uint8_t *out, const uint8_t *in, size_t len) {

  size_t i;
  /* initial values -  astonishingly, assembling using SHIFTs and ORs (in register)
   * works faster on well pipelined CPUs than loading the 64-bit value from memory.
   * however, there is one advantage to loading from memory: as we also store back to
   * memory at the end, we do not need to care about endianess! */
  uint8_t upper[8] = { 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
  uint8_t lower[8] = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

  uint64_t upper_hash_mask = *(uint64_t*)&upper;
  uint64_t lower_hash_mask = *(uint64_t*)&lower;
  uint64_t high_upper_hash_mask = upper_hash_mask + 0x1010101010101010;
  uint64_t high_lower_hash_mask = lower_hash_mask + 0x1010101010101010;

  uint64_t upper_hash = 0;
  uint64_t lower_hash = 0;
  uint64_t high_upper_hash = 0;
  uint64_t high_lower_hash = 0;

  for (i = 0; i < len; i++) {
    // broadcast the character, xor into hash, make them different permutations
    uint64_t c = (uint8_t)in[i];
    c |= c <<  8;
    c |= c << 16;
    c |= c << 32;
    upper_hash ^= c ^ upper_hash_mask;
    lower_hash ^= c ^ lower_hash_mask;
    high_upper_hash ^= c ^ high_upper_hash_mask;
    high_lower_hash ^= c ^ high_lower_hash_mask;

    // table lookup
    uint64_t h = 0;
    uint16_t x;
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    upper_hash = h;

    h = 0;
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    lower_hash = h;

    h = 0;
    x = high_upper_hash; x = t16[x]; high_upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_upper_hash; x = t16[x]; high_upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_upper_hash; x = t16[x]; high_upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_upper_hash; x = t16[x]; high_upper_hash >>= 16; h |= x; h=ROR64(h,16);
    high_upper_hash = h;

    h = 0;
    x = high_lower_hash; x = t16[x]; high_lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_lower_hash; x = t16[x]; high_lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_lower_hash; x = t16[x]; high_lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = high_lower_hash; x = t16[x]; high_lower_hash >>= 16; h |= x; h=ROR64(h,16);
    high_lower_hash = h;
  }
  // store output
  uint64_t *o;
  o = (uint64_t*)&out[0];
  *o = high_upper_hash;
  o = (uint64_t*)&out[8];
  *o = high_lower_hash;
  o = (uint64_t*)&out[16];
  *o = upper_hash;
  o = (uint64_t*)&out[24];
  *o = lower_hash;
}


void pearson_hash_128 (uint8_t *out, const uint8_t *in, size_t len) {

  size_t i;
  /* initial values -  astonishingly, assembling using SHIFTs and ORs (in register)
   * works faster on well pipelined CPUs than loading the 64-bit value from memory.
   * however, there is one advantage to loading from memory: as we also store back to
   * memory at the end, we do not need to care about endianess! */
  uint8_t upper[8] = { 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
  uint8_t lower[8] = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

  uint64_t upper_hash_mask = *(uint64_t*)&upper;
  uint64_t lower_hash_mask = *(uint64_t*)&lower;

  uint64_t upper_hash = 0;
  uint64_t lower_hash = 0;

  for (i = 0; i < len; i++) {
    // broadcast the character, xor into hash, make them different permutations
    uint64_t c = (uint8_t)in[i];
    c |= c <<  8;
    c |= c << 16;
    c |= c << 32;
    upper_hash ^= c ^ upper_hash_mask;
    lower_hash ^= c ^ lower_hash_mask;
    // table lookup
    uint64_t h = 0;
    uint16_t x;
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    x = upper_hash; x = t16[x]; upper_hash >>= 16; h |= x; h=ROR64(h,16);
    upper_hash = h;

    h = 0;
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    x = lower_hash; x = t16[x]; lower_hash >>= 16; h |= x; h=ROR64(h,16);
    lower_hash = h;
  }
  // store output
  uint64_t *o;
  o = (uint64_t*)&out[0];
  *o = upper_hash;
  o = (uint64_t*)&out[8];
  *o = lower_hash;
}


// 32-bit hash: the return value has to be interpreted as uint32_t and
// follows machine-specific endianess in memory
uint32_t pearson_hash_32 (const uint8_t *in, size_t len) {

  size_t i;
  uint32_t hash = 0;
  uint32_t hash_mask = 0x03020100;

  for (i = 0; i < len; i++) {
    // broadcast the character, xor into hash, make them different permutations
    uint32_t c = (uint8_t)in[i];
    c |= c <<  8;
    c |= c << 16;
    hash ^= c ^ hash_mask;
    // table lookup
    uint32_t h = 0;
    uint8_t x;
    x = hash; x = t[x]; hash >>= 8; h |= x; h=ROR32(h,8);
    x = hash; x = t[x]; hash >>= 8; h |= x; h=ROR32(h,8);
    x = hash; x = t[x]; hash >>= 8; h |= x; h=ROR32(h,8);
    x = hash; x = t[x]; hash >>= 8; h |= x; h=ROR32(h,8);
    hash = h;
  }
  // output
  return hash;
}


// 16-bit hash: the return value has to be interpreted as uint16_t and
// follows machine-specific endianess in memory
uint16_t pearson_hash_16 (const uint8_t *in, size_t len) {
  size_t i;
  uint16_t hash = 0;
  uint16_t hash_mask = 0x0100;

  for (i = 0; i < len; i++) {
    // broadcast the character, xor into hash, make them different permutations
    uint16_t c = (uint8_t)in[i];
    c |= c <<  8;
    hash ^= c ^ hash_mask;
    // table lookup
    hash = t[(uint8_t)hash] + (t[hash >> 8] << 8);
  }
  // output
  return hash;
}


#endif // AES-NI & SSSE3, plain C ---------------------------------------------------------


void pearson_hash_init () {

#if defined (__AES__) && (__SSSE3__)

#else
  size_t i;

  for (i = 0; i < 65536; i++)
    t16[i] = (t[i >> 8] << 8) + t[(uint8_t)i];
#endif
}
