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

#include "string.h"

#include <utee_defines.h>
#include <utee_types.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <mpa.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <arm_sys_counter.h>
#include "simple_ta.h"


/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");
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
	uint64_t ta_counter_value = read_cntvct();

	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&sess_ctx;

	/* Return system counter value to Host app */
	memcpy(params[0].memref.buffer, &ta_counter_value,
			sizeof(ta_counter_value));

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	DMSG("has been called (session opened with the Simple TA)");

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
	DMSG("has been called (session with the Simple TA will be closed)");
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
	uint64_t enter_function_counter_value = read_cntvct();
	uint64_t sys_call_start_counter_value = 0;
	uint64_t sys_call_received_counter_value = 0;
	uint64_t sys_call_return_counter_value = 0;
	uint32_t exp_param_types = 0;
	struct generic_timer_info timer_info;
	size_t buffer_size = sizeof(timer_info);
	char timer_property_string[] = "ext.tee.arm.genericTimerInfo";

	DMSG("Enter Invoke Command Entry Point");

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
	case TAF_MEASURE_SYS_CALL_TIME:
		exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
								TEE_PARAM_TYPE_MEMREF_OUTPUT,
								TEE_PARAM_TYPE_MEMREF_OUTPUT,
								TEE_PARAM_TYPE_NONE);

		if (param_types != exp_param_types) {
			result = TEE_ERROR_BAD_PARAMETERS;
			break;
		}

		sys_call_start_counter_value = read_cntvct();

		result = TEE_GetPropertyAsBinaryBlock(
					TEE_PROPSET_TEE_IMPLEMENTATION,
					timer_property_string,
					&timer_info, &buffer_size);

		if (result != TEE_SUCCESS)
			break;

		sys_call_received_counter_value = timer_info.counter_value;
		sys_call_return_counter_value = read_cntvct();

		params[0].memref.size = sizeof(sys_call_start_counter_value);
		memcpy(params[0].memref.buffer, &sys_call_start_counter_value,
				params[0].memref.size);

		params[1].memref.size = sizeof(sys_call_received_counter_value);
		memcpy(params[1].memref.buffer, &sys_call_received_counter_value,
				params[1].memref.size);

		params[1].memref.size = sizeof(sys_call_return_counter_value);
		memcpy(params[2].memref.buffer, &sys_call_return_counter_value,
				params[1].memref.size);

		break;

	case TAF_REPORT_TIMESTAMP:
		exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
								TEE_PARAM_TYPE_NONE,
								TEE_PARAM_TYPE_NONE,
								TEE_PARAM_TYPE_NONE);

		if (param_types != exp_param_types) {
			result = TEE_ERROR_BAD_PARAMETERS;
			break;
		}

		memcpy(params[0].memref.buffer, &enter_function_counter_value,
				sizeof(enter_function_counter_value));
		params[0].memref.size = sizeof(enter_function_counter_value);
		break;

	default:
		result = TEE_ERROR_NOT_IMPLEMENTED;
		break;
	}

	DMSG("Leave Invoke Command Entry Point");

	return result;
}
