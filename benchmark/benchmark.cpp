/*
  This file is part of cpp-ethereum.

  cpp-ethereum is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  cpp-ethereum is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file dash.cpp
 * @author Tim Hughes <tim@ethdev.org>
 * @date 2015
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "libethash/ethash.h"
#include "libethash/util.h"

uint8_t g_hash[32];

extern "C" void main(void)
{
	// params for ethash
	ethash_params params;
	ethash_params_init(&params);
	params.full_size = 262147 * 4096;	// 1GBish;
	//params.full_size = 32771 * 4096;	// 128MBish;
	params.cache_size = 8209*4096;
	//params.cache_size = 2053*4096;
	params.k = 2 * (params.full_size / params.cache_size);

	const unsigned trials = 1000;
	const bool test_full = false;
	
	// make random seed and header
	uint8_t header[32];
	srand(3248723843);
	for (unsigned i = 0; i != 32; ++i)
	{
		params.seed[i] = rand();
		header[i] = rand();
	}

	// allocate page aligned buffer for dataset
	void* mem_buf = malloc(params.full_size + 4095 + 64);
	void* mem = (void*)((uintptr_t(mem_buf) + 4095) & ~4095);

	ethash_cache cache;
	ethash_cache_init(&cache, mem);

	// compute cache or full data
	{
		clock_t startTime = clock();

		if (test_full)
			ethash_compute_full_data(cache.mem, &params);
		else
			ethash_compute_cache_data(&cache, &params);

		clock_t time = clock() - startTime;
		
		if (test_full)
			debugf("ethash_compute_full_data: %ums\n", time);
		else
			debugf("ethash_compute_cache_data: %ums\n", time);
	}

	// trial different numbers of accesses
	for (unsigned read_size = 4096; read_size <= 4096*256; read_size <<= 1)
	{
		params.hash_read_size = read_size;
		
		clock_t startTime = clock();

		// uncomment to enable multicore
		//#pragma omp parallel for
		for (int nonce = 0; nonce < trials; ++nonce)
		{
			if (test_full)
				ethash_full(g_hash, cache.mem, &params, nonce);
			else
				ethash_light(g_hash, &cache, &params, nonce);

			//if ((nonce % 500) == 0)
			//	logf("trials: %u\n", nonce);
		}
		clock_t time = clock() - startTime;

		debugf("read_size %5ukb, hashrate: %6u, bw: %5u MB/s\n",
			read_size / 1024,
			(trials*1000)/time,
			(unsigned)((((uint64_t)trials*read_size*1000)/time) / (1024*1024))
			);
	}

	free(mem_buf);
}

