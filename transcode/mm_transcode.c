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
mm_transcode_create(MMHandleType* MMHandle)
{
	int ret = MM_ERROR_NONE;
	handle_s *handle = NULL;

	/* Check argument here */
	if(MMHandle == NULL) {
		debug_error ("Invalid arguments [tag null]\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Init Transcode */
	gst_init (NULL, NULL);
	handle = g_new0 (handle_s, 1); /*handle = g_malloc(sizeof(handle_s));*/
	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp= g_new0 (handle_vidp_plugin_s, 1);
	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_audp= g_new0 (handle_audp_plugin_s, 1);
	if (!handle->decoder_audp) {
		debug_error("[ERROR] - handle decoder audio process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->encodebin= g_new0 (handle_encode_s, 1);
	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->property = g_new0 (handle_property_s, 1);
	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	*MMHandle = (MMHandleType)handle;

	if(MMHandle) {
		debug_log("MMHandle: 0x%2x", handle);
		handle->property->_MMHandle = 0;
	} else {
		debug_error("handle create Fail");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}

int
mm_transcode_prepare (MMHandleType MMHandle, const char *in_Filename, mm_containerformat_e containerformat, mm_videoencoder_e videoencoder,
mm_audioencoder_e audioencoder)
{
	int ret = MM_ERROR_NONE;

	handle_s *handle = (handle_s *) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if((in_Filename == NULL) || (strlen (in_Filename) == 0)) {
		debug_error("Invalid Input file");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(videoencoder == MM_VIDEOENCODER_NO_USE && audioencoder == MM_AUDIOENCODER_NO_USE) {
		debug_error("No encoder resource");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(videoencoder == MM_VIDEOENCODER_H264) {
		debug_error("Can't support encoding to H264");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* set element*/
	ret = _mm_transcode_set_handle_element(handle, in_Filename, containerformat, videoencoder, audioencoder);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR -Set element");
		return ret;
	}

	debug_log("%s == %s", handle->property->sourcefile, in_Filename);
	if(0 == strlen(handle->property->sourcefile) || 0 == strcmp(handle->property->sourcefile, in_Filename)) { /* protect the case of changing input file during transcoding */
		/* setup */
		ret = _mm_setup_pipeline(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Setup Pipeline");
		} else{
			debug_error("ERROR - Setup Pipeline");
			return ret;
		}

		/* video / auido stream */
		ret = _mm_transcode_get_stream_info(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Get stream info");
		} else{
			debug_error("ERROR - Get stream info");
			return ret;
		}

		/* create pipeline */
		ret = _mm_transcode_create(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Create Pipeline");
		} else{
			debug_error("ERROR -Create Pipeline");
			return ret;
		}

		/*link pipeline */
		ret = _mm_transcode_link(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Link pipeline");
		} else{
			debug_error("ERROR - Link pipeline");
			return ret;
		}

		/* flush param */
		ret = _mm_transcode_param_flush(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Init parameter");
		} else{
			debug_error("ERROR - Init parameter");
			return ret;
		}

		/* create thread */
		ret = _mm_transcode_thread(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Link pipeline");
		} else{
			debug_error("ERROR - Link pipeline");
			return ret;
		}

		/* Add_watcher Transcode Bus*/
		GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (handle->pipeline));
		handle->property->bus_watcher = gst_bus_add_watch (bus, (GstBusFunc)_mm_cb_transcode_bus, handle);
		gst_object_unref (GST_OBJECT(bus));
		debug_log("Success - gst_object_unref (bus)");
	}

	handle->property->_MMHandle++;

	return ret;
}

int
mm_transcode (MMHandleType MMHandle, unsigned int resolution_width, unsigned int resolution_height, unsigned int fps_value, unsigned long start_position,
	unsigned long duration, mm_seek_mode_e seek_mode, const char *out_Filename, mm_transcode_progress_callback progress_callback, mm_transcode_completed_callback completed_callback, void *user_param)
{
	int ret = MM_ERROR_NONE;
	handle_s *handle = (handle_s *) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(!completed_callback) {
		debug_error("[ERROR] - completed_callback");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(!out_Filename) {
		debug_error("[ERROR] - out_Filename");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(duration < MIN_DURATION && duration !=0) {
		debug_error("The minimum seek duration is 1000 msec ");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->property->progress_cb= progress_callback;
	handle->property->progress_cb_param = user_param;
	debug_log("[MMHandle] 0x%2x [progress_cb] 0x%2x [progress_cb_param] 0x%2x", MMHandle, handle->property->progress_cb, handle->property->progress_cb_param);

	handle->property->completed_cb = completed_callback;
	handle->property->completed_cb_param = user_param;
	debug_log("[MMHandle] 0x%2x [completed_cb] 0x%2x [completed_cb_param] 0x%2x", MMHandle, handle->property->completed_cb, handle->property->completed_cb_param);

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->_MMHandle == 1) { /* check if prepare is called during transcoding */
		handle_param_s *param = g_new0 (handle_param_s, 1);
		if(param) {
			/*g_value_init (param, G_TYPE_INT);*/
			debug_log("[param] 0x%2x", param);
		}

		/* set parameter*/
		ret = _mm_transcode_set_handle_parameter(param, resolution_width, resolution_height, fps_value, start_position, duration, seek_mode, out_Filename);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Set parameter");
			return ret;
		}

		ret = _mm_transcode_preset_capsfilter(handle, resolution_width, resolution_height);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Preset Capsfilter");
		} else{
			debug_error("ERROR - Preset Capsfilter");
			return ret;
		}

		handle->property->is_busy = TRUE;

		/*push data to handle */
		g_async_queue_push (handle->property->queue, GINT_TO_POINTER(param));
	}

	return ret;
}

int
mm_transcode_is_busy (MMHandleType MMHandle, bool *is_busy)
{
	int ret = MM_ERROR_NONE;

	handle_s *handle = (handle_s *) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!is_busy) {
		debug_error("[ERROR] - is_busy");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	*is_busy = handle->property->is_busy;
	debug_log("[Transcoding....] %d", *is_busy);

	return ret;
}

int
mm_transcode_cancel (MMHandleType MMHandle)
{
	int ret = MM_ERROR_NONE;
	handle_s *handle = (handle_s *) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->is_busy) {
		debug_log("Cancel - [IS BUSY]");
		ret = _mm_transcode_state_change(handle, GST_STATE_NULL);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR - Null Pipeline");
			return ret;
		}

		if((handle->param) && (strlen(handle->param->outputfile) > 0)) {
			unlink(handle->param->outputfile);
			debug_log("[Cancel] unlink %s", handle->param->outputfile);
		} else {
			debug_error("unlink error");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_cond_signal(handle->property->thread_cond);
		debug_log("===> send completed signal <-cancel");
		g_mutex_unlock (handle->property->thread_mutex);
	}

	handle->property->is_busy = FALSE;

	return ret;
}

int
mm_transcode_destroy (MMHandleType MMHandle)
{
	int ret = MM_ERROR_NONE;

	handle_s *handle = (handle_s*) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	g_mutex_lock(handle->property->thread_exit_mutex);
	handle->property->repeat_thread_exit = TRUE;
	if(handle->property->is_busy) {
		ret = mm_transcode_cancel(MMHandle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Cancel Transcode");
		} else{
			debug_error("ERROR - Cancel Transcode");
			return FALSE;
		}
	}
	/* handle->property->is_busy = FALSE; */
	g_mutex_unlock(handle->property->thread_exit_mutex);

	handle_param_s *param = g_new0 (handle_param_s, 1);
	if(param) {
		debug_log("[Try to Push Last Queue]");
		g_async_queue_push (handle->property->queue, GINT_TO_POINTER(param));
	} else {
		debug_error("Fail to create Last Queue");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = _mm_transcode_state_change(handle, GST_STATE_NULL);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR - Null Pipeline");
		return ret;
	} else {
		debug_log("SUCCESS - Null Pipeline");
	}

	if((handle->param) && (!handle->param->completed)) {
		g_cond_signal(handle->property->thread_cond);
		debug_log("===> send completed signal <-destroy");
		g_mutex_unlock (handle->property->thread_mutex);
		debug_log("unlock destory");
		if(strlen(handle->param->outputfile) > 0) {
			unlink(handle->param->outputfile);
			debug_log("[Cancel] unlink %s", handle->param->outputfile);
		}
	}

	ret = _mm_cleanup_pipeline(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - CleanUp Pipeline");
	} else{
		debug_error("ERROR - CleanUp Pipeline");
		return ret;
	}
	TRANSCODE_FREE (param);
	debug_log("[param] free");

	return ret;
}

int
mm_transcode_get_attrs(MMHandleType MMHandle, mm_containerformat_e *containerformat, mm_videoencoder_e *videoencoder,
	mm_audioencoder_e *audioencoder, unsigned long *current_pos, unsigned long *duration, unsigned int *resolution_width, unsigned int *resolution_height)
{

	int ret = MM_ERROR_NONE;
	handle_s *handle = (handle_s *) MMHandle;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(!containerformat || !videoencoder ||!audioencoder || !current_pos || !duration || !resolution_width || !resolution_height) {
		debug_error("[ERROR] - Invalid argument pointer");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	*containerformat = handle->property->containerformat;
	*videoencoder = handle->property->videoencoder;
	*audioencoder = handle->property->audioencoder;

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->current_pos > handle->param->duration) {
		*current_pos = handle->param->duration;
	} else {
		*current_pos = handle->property->current_pos;
	}
	*duration = handle->property->real_duration;
	*resolution_width = handle->param->resolution_width;
	*resolution_height = handle->param->resolution_height;

	debug_log("containerformat : %d, videoencoder : %d, audioencoder : %d, current_pos : %d, duration : %d, resolution_width : %d, resolution_height : %d",
		*containerformat, *videoencoder, *audioencoder, *current_pos, *duration, *resolution_width, *resolution_height);

	return ret;
}

int
mm_transcode_get_supported_container_format(mm_transcode_support_type_callback type_callback, void *user_param)
{
	int idx = 0;

	for(idx = 0; idx < MM_CONTAINER_NUM; idx++) {
		if(type_callback(idx, user_param) == false)
		{
			debug_error("error occured. idx[%d]", idx);
			break;
		}
	}

	return MM_ERROR_NONE;
}

int
mm_transcode_get_supported_video_encoder(mm_transcode_support_type_callback type_callback, void *user_param)
{
	int idx = 0;

	for(idx = 0; idx < MM_VIDEOENCODER_H264; idx++) {
		if(type_callback(idx, user_param) == false)
		{
			debug_error("error occured. idx[%d]", idx);
			break;
		}
	}

	return MM_ERROR_NONE;
}

int
mm_transcode_get_supported_audio_encoder(mm_transcode_support_type_callback type_callback, void *user_param)
{
	int idx = 0;

	for(idx = 0; idx < MM_AUDIOENCODER_NO_USE; idx++) {
		if(type_callback(idx, user_param) == false)
		{
			debug_error("error occured. idx[%d]", idx);
			break;
		}
	}

	return MM_ERROR_NONE;
}
