/**
 * bitgen.h
 * Copyright (C) 2012 Jan Viktorin
 */

#ifndef BITGEN_H
#define BITGEN_H

#include <stdlib.h>
#include <inttypes.h>

/**
 * Representation of bit generator.
 *
 * Eg.
 * I want to generate all combinations of zeros and ones
 * for 3 bits.
 *  uint64_t d[3];
 *  struct bitgen_t g;
 *  bitgen_init(&g, 4);
 *  ...
 *  while(bitgen_next64(g, d))
 *   ...
 *  bitgen_fini(g&);
 * 
 * Generates:
 *  d[0] = 0x000000aa  // 10101010b
 *  d[1] = 0x000000cc  // 11001100b
 *  d[2] = 0x000000f0  // 11110000b
 */
struct bitgen_t {
	uint64_t *last;
	size_t width;
	int valid;
};

/**
 * Creates bit generator that generates all bit combinations
 * of the given `width`.
 *
 * Returns zero when successful.
 * Returns negative value on fatal (system) error (memory).
 * Returns positive value on (user) error (invalid parameter).
 */
int bitgen_init(struct bitgen_t *g, size_t width);

/**
 * Frees the generator.
 */
void bitgen_fini(struct bitgen_t *g);

/**
 * Generates a piece of the bit-space into the array `d`.
 * The array `d` must be at least of length `width` given
 * to `bitgen_init`.
 *
 * Returns non-zero when the array `d` has been successfully
 * filled with bits.
 * Returns zero if there are no more combinations.
 *
 * If the result does not exactly fit the maximal data type
 * range (64 bits here) - that means eg.
 *  I requested generate data of width 2 bits.
 *  Then the result is just:
 *     0x000A (1010b)
 *     0x000C (1100b)
 *
 *  ...then the generator generates:
 *     0xAAAA..AAA
 *     0xCCCC..CCC
 *
 * to make it faster. The user can trim out the rest on its own.
 */
int bitgen_next(struct bitgen_t *g, uint64_t *d);

#endif

