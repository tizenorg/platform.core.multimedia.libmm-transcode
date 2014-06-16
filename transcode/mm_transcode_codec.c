/*
 * libmm-transcode
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: YoungHun Kim <yh8004.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "mm_transcode.h"
#include "mm_transcode_internal.h"

int
_mm_encodebin_set_venc_aenc(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->property->mux = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->property->mux == NULL) {
		debug_error("[NULL] mux");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	handle->property->venc = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->property->venc == NULL) {
		debug_error("[NULL] venc");
		TRANSCODE_FREE(handle->property->mux);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	handle->property->aenc = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->property->aenc == NULL) {
		debug_error("[NULL] aenc");
		TRANSCODE_FREE(handle->property->mux);
		TRANSCODE_FREE(handle->property->venc);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	memset(handle->property->mux, 0, ENC_BUFFER_SIZE);
	memset(handle->property->venc, 0, ENC_BUFFER_SIZE);
	memset(handle->property->aenc, 0, ENC_BUFFER_SIZE);

	switch(handle->property->containerformat) {
		case MM_CONTAINER_3GP :
		case MM_CONTAINER_MP4 :
			if(handle->property->videoencoder == MM_VIDEOENCODER_NO_USE && handle->property->audioencoder == MM_AUDIOENCODER_AAC) {
				strncpy(handle->property->mux, MUXAAC, ENC_BUFFER_SIZE-1);
			} else {
				strncpy(handle->property->mux, MUX3GP, ENC_BUFFER_SIZE-1);
			}
			break;
		default :
			debug_error("error container value");
			break;
	}

	switch(handle->property->videoencoder) {
		case MM_VIDEOENCODER_MPEG4 :
			strncpy(handle->property->venc, AVENCMPEG4, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->property->venc);
			break;
		case MM_VIDEOENCODER_H263 :
			strncpy(handle->property->venc, AVENCH263, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->property->venc);
			break;
		case MM_VIDEOENCODER_NO_USE :
			debug_log("No VIDEO");
			break;
		default :
			debug_error("error videoencoder value");
			break;
	}

	switch(handle->property->audioencoder) {
		case MM_AUDIOENCODER_AAC :
			strncpy(handle->property->aenc, AACENC, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->property->aenc);
			break;
		case MM_AUDIOENCODER_AMR :
			strncpy(handle->property->aenc, AMRENC, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->property->aenc);
			break;
		case MM_AUDIOENCODER_NO_USE :
			debug_log("No AUDIO");
			break;
		default :
			debug_error("error audioencoder value");
			break;
	}
	return ret;
}
