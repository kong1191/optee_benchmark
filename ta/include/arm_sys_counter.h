/*
 * Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * liABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef OPTEE_BENCHMARK_LIBSYSCOUNTER_ARM_SYS_COUNTER_H_
#define OPTEE_BENCHMARK_LIBSYSCOUNTER_ARM_SYS_COUNTER_H_

#include <stdint.h>
#include <inttypes.h>

static inline uint64_t read_cntvct(void)
{
	uint64_t cntvct;

#ifdef __aarch64__

	asm volatile ("mrs %[cntvct], cntvct_el0"
			: [cntvct] "=r" (cntvct)
	);

#else

	asm volatile ("mrrc	p15, 1, %Q[cntvct], %R[cntvct], c14"
			: [cntvct] "=r" (cntvct)
	);

#endif

	return cntvct;
}

static inline uint64_t read_cntpct(void)
{
	uint64_t cntpct;

#ifdef __aarch64__

	asm volatile ("mrs %[cntpct], cntpct_el0"
			: [cntpct] "=r" (cntpct)
	);

#else

	asm volatile ("mrrc	p15, 0, %Q[cntpct], %R[cntpct], c14"
			: [cntpct] "=r" (cntpct)
	);

#endif

	return cntpct;
}

static inline uint32_t read_cntfrq(void)
{
	uint32_t cntfrq;

#ifdef __aarch64__

	asm volatile ("mrs %[cntfrq], cntfrq_el0"
			: [cntfrq] "=r" (cntfrq)
	);

#else

	asm volatile ("mrc	p15, 0, %[cntfrq], c14, c0, 0"
			: [cntfrq] "=r" (cntfrq)
	);

#endif

	return cntfrq;
}

#endif /* OPTEE_BENCHMARK_LIBSYSCOUNTER_ARM_SYS_COUNTER_H_ */
