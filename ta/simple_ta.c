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

#define STR_TRACE_USER_TA "COMCAST_CRYPTO_TA"
#include "string.h"

#include <utee_defines.h>
#include <assert.h>
#include <stdint.h>
#include <mpa.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "simple_ta.h"



static uint32_t do_div(uint64_t *dividend, uint32_t divisor)
{
	mpa_word_t remainder = 0, n0, n1;
	n0 = (*dividend) & UINT_MAX;
	n1 = ((*dividend) >> WORD_SIZE) & UINT_MAX;
	*dividend = __mpa_div_dword(n0, n1, divisor, &remainder);
	return remainder;
}


static uint64_t read_cntpct(void)
{
	uint64_t val;
	uint32_t low, high;
	__asm__ volatile("mrrc	p15, 0, %0, %1, c14\n"
		: "=r"(low), "=r"(high)
		:
		: "memory");
	val = low | ((uint64_t)high << WORD_SIZE);
	return val;
}


static uint32_t read_cntfrq(void)
{
	uint32_t frq;
	__asm__ volatile("mrc	p15, 0, %0, c14, c0, 0\n"
		: "=r"(frq)
		:
		: "memory");
	return frq;
}


static TEE_Result get_sys_time(TEE_Time *time)
{
	uint64_t cntpct = read_cntpct();
	uint32_t cntfrq = read_cntfrq();
	uint32_t remainder;

	remainder = do_div(&cntpct, cntfrq);

	time->seconds = (uint32_t)cntpct;
	time->millis = remainder / (cntfrq / TEE_TIME_MILLIS_BASE);

	return TEE_SUCCESS;
}


static int32_t time_diff(TEE_Time start, TEE_Time end)
{
	return (end.seconds*1000 + end.millis) - (start.seconds*1000 + start.millis);
}

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	TEE_Time sys_time;
	get_sys_time(&sys_time);

	DMSG("[%d.%d] has been called", sys_time.seconds, sys_time.millis);
	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param  params[4], void **sess_ctx)
{
	TEE_Time sys_time;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	get_sys_time(&sys_time);
	DMSG("[%d.%d] has been called (session opened with the Comcast Crypto TA)", sys_time.seconds, sys_time.millis);

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	DMSG("has been called (session with the Comcast Crypto TA will be closed)");
}


/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	TEE_Result result = TEE_SUCCESS;
	TEE_Time sys_time, sys_time2, kernel_sys_time;
	int32_t api_round_trip_time = 0;

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	get_sys_time(&sys_time);
	DMSG("[%d.%d] Enter Invoke Command Entry Point", sys_time.seconds, sys_time.millis);

	(void)&sess_ctx; /* Unused parameter */

	/*
	 * In reality, there is no reason for having two functions here when
	 * just doing hashing. You could just pass the cmd_id to a generic hash
	 * function and in that function choose the hashing algorithm based on
	 * the cmd_id. However, to make this example TA easier to understand we
	 * have added separate functions (almost identical) for the hashing,
	 * i.e, one for SHA1 and one for SHA256.
	 */
	switch (cmd_id) {
	case TAF_MEASURE_SECURE_API_TIME:
		TEE_GetSystemTime(&kernel_sys_time);
		get_sys_time(&sys_time2);

		DMSG("kernel_sys_time=%d.%d", kernel_sys_time.seconds, kernel_sys_time.millis);
		DMSG("sys call round trip time=%d (ms)", time_diff(sys_time, sys_time2));
		DMSG("sys call single trip time=%d (ms)", time_diff(sys_time, kernel_sys_time));

		api_round_trip_time = time_diff(sys_time, sys_time2);
		memcpy(params[0].memref.buffer, &api_round_trip_time, sizeof(api_round_trip_time));
		params[0].memref.size = sizeof(api_round_trip_time);

		break;
	case TAF_REPORT_TIMESTAMP:
		get_sys_time(params[0].memref.buffer);
		params[0].memref.size = sizeof(TEE_Time);
		break;
	default:
		break;
	}

	get_sys_time(&sys_time);
	DMSG("[%d.%d] Leave Invoke Command Entry Point", sys_time.seconds, sys_time.millis);

	return result;
}
