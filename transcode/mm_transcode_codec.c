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

	handle->mux = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->mux == NULL) {
		debug_error("[NULL] mux");
		return MM_ERROR_INVALID_ARGUMENT;
	}
	handle->venc = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->venc == NULL) {
		debug_error("[NULL] venc");
		TRANSCODE_FREE(handle->mux);
		return MM_ERROR_INVALID_ARGUMENT;
	}
	handle->aenc = malloc(sizeof(gchar) * ENC_BUFFER_SIZE);
	if(handle->aenc == NULL) {
		debug_error("[NULL] aenc");
		TRANSCODE_FREE(handle->mux);
		TRANSCODE_FREE(handle->venc);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	memset(handle->mux, 0, ENC_BUFFER_SIZE);
	memset(handle->venc, 0, ENC_BUFFER_SIZE);
	memset(handle->aenc, 0, ENC_BUFFER_SIZE);

	switch(handle->containerformat) {
		case MM_CONTAINER_3GP :
		case MM_CONTAINER_MP4 :
			if(handle->videoencoder == MM_VIDEOENCODER_NO_USE && handle->audioencoder == MM_AUDIOENCODER_AAC) {
				strncpy(handle->mux, MUXAAC, ENC_BUFFER_SIZE-1);
			} else {
				strncpy(handle->mux, MUX3GP, ENC_BUFFER_SIZE-1);
			}
			break;
		default :
			debug_error("error container value");
			break;
	}

	switch(handle->videoencoder) {
		case MM_VIDEOENCODER_MPEG4 :
			strncpy(handle->venc, FFENCMPEG4, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->venc);
			break;
		case MM_VIDEOENCODER_H263 :
			strncpy(handle->venc, FFENCH263, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->venc);
			break;
		case MM_VIDEOENCODER_NO_USE :
			debug_log("No VIDEO");
			break;
		default :
			debug_error("error videoencoder value");
			break;
	}

	switch(handle->audioencoder) {
		case MM_AUDIOENCODER_AAC :
			strncpy(handle->aenc, AACENC, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->aenc);
			break;
		case MM_AUDIOENCODER_AMR :
			strncpy(handle->aenc, AMRENC, ENC_BUFFER_SIZE-1);
			debug_log("[FFMPEG] %s", handle->aenc);
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
