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

static void _mm_transcode_add_sink(handle_s *handle, GstElement* sink_elements);
static void _mm_transcode_audio_capsfilter(GstCaps *caps, handle_s *handle);
static void _mm_transcode_video_capsfilter(GstCaps *caps, handle_s *handle);
static void _mm_transcode_video_capsfilter_call(handle_s *handle);
static void _mm_transcode_video_capsfilter_set_parameter(GstCaps *caps, handle_s *handle);
static int _mm_transcode_exec(handle_s *handle, handle_param_s *param);
static int _mm_transcode_play (handle_s *handle);
static int _mm_transcode_seek (handle_s *handle);
static gpointer _mm_transcode_thread_repeate(gpointer data);

gboolean
_mm_cb_audio_output_stream_probe(GstPad *pad, GstBuffer *buffer, gpointer user_data)
{
	handle_s *handle = (handle_s*) user_data;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	gint64 start_pos_ts = handle->param->start_pos * G_GINT64_CONSTANT(1000000);

	if(GST_BUFFER_TIMESTAMP_IS_VALID (buffer)) {
		if(0 == handle->property->AUDFLAG++) {
			_mm_transcode_audio_capsfilter(gst_buffer_get_caps (buffer), handle); /* Need to audio caps converting when amrnbenc*/ /* Not drop buffer with 'return FALSE'*/

			if(handle->param->seeking) {
				debug_log("[AUDIO BUFFER TIMESTAMP] ([%"GST_TIME_FORMAT"])", GST_TIME_ARGS(start_pos_ts));
				GST_BUFFER_TIMESTAMP (buffer) = start_pos_ts;
			}
		}
	}
	return TRUE;
}

GstAutoplugSelectResult
_mm_cb_decode_bin_autoplug_select(GstElement * element, GstPad * pad, GstCaps * caps, GstElementFactory * factory, handle_s *handle)
{
	const gchar *feature_name = gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory));
	const gchar *caps_str = NULL;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	caps_str = _mm_check_media_type(caps);
	if(g_strrstr(caps_str, "audio")) {
		handle->property->audiodecodename = (char*)malloc(sizeof(char) * ENC_BUFFER_SIZE);
		if(handle->property->audiodecodename == NULL) {
			debug_error("audiodecodename is NULL");
			return GST_AUTOPLUG_SELECT_TRY;
		}
		memset(handle->property->audiodecodename, 0, ENC_BUFFER_SIZE);
		strncpy(handle->property->audiodecodename, feature_name, ENC_BUFFER_SIZE-1);
		debug_log ("[audio decode name %s : %s]", caps_str, handle->property->audiodecodename);
	}

	if(g_strrstr(caps_str, "video")) {
		handle->property->videodecodename = (char*)malloc(sizeof(char) * ENC_BUFFER_SIZE);
		if(handle->property->videodecodename == NULL) {
			debug_error("videodecodename is NULL");
			return GST_AUTOPLUG_SELECT_TRY;
		}
		memset(handle->property->videodecodename, 0, ENC_BUFFER_SIZE);
		strncpy(handle->property->videodecodename, feature_name, ENC_BUFFER_SIZE-1);
		debug_log ("[video decode name %s : %s]", caps_str, handle->property->videodecodename);
	}

	/* Try factory. */
	return GST_AUTOPLUG_SELECT_TRY;
}

void
_mm_cb_decoder_newpad_encoder(GstElement *decodebin, GstPad *pad, gboolean last, handle_s *handle)
{
	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder_vidp");
		return;
	}

	if (!handle->decoder_audp) {
		debug_error("[ERROR] - handle decoder_audp");
		return;
	}

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	debug_log("[============ new-decoded-pad ============]");
	handle->property->caps = gst_pad_get_caps (pad);
	const gchar *mime = _mm_check_media_type(handle->property->caps);

	if(!mime) {
		debug_error("[ERROR] - mime");
		return;
	}

	if(g_strrstr(mime,"video")) {
		handle->property->linked_vidoutbin = TRUE;

		/* link videopad */
		if(gst_pad_is_linked(pad)) {
			debug_log("pad liked");
		} else {
			if(gst_pad_link(pad, (GstPad *)handle->decoder_vidp->sinkdecvideopad) != GST_PAD_LINK_OK) {
				debug_error("Error [pad - sinkdecvideopad]");
			} else {
				debug_log("Success [pad - sinkdecvideopad]");
			}
		}

	} else if(g_strrstr(mime,"audio")) {
		handle->property->linked_audoutbin = TRUE;

		/* link audiopad */
		if(gst_pad_is_linked(pad)) {
			debug_log("pad liked");
		} else {
			if(gst_pad_link(pad, (GstPad *)handle->decoder_audp->sinkdecaudiopad) != GST_PAD_LINK_OK) {
				debug_error("Error [pad - sinkdecaudiopad]");
			} else {
				debug_log("Success [pad - sinkdecaudiopad]");
			}
		}

	} else {
		debug_log("gst structure error");
	}

	if(last) {
		if(0 == handle->property->seek_idx) {
			if(handle->property->linked_vidoutbin == TRUE) {
				if(handle->property->videoencoder != MM_VIDEOENCODER_NO_USE) {
					if(handle->param->seeking) {
						_mm_transcode_add_sink(handle, handle->decoder_vidp->decsinkvideoqueue);
					}
					if(gst_pad_link(handle->decoder_vidp->srcdecvideopad, handle->encodebin->encvideopad) != GST_PAD_LINK_OK) {
						debug_error("Error [srcdecvideopad - encvideopad]");
					} else {
						debug_log("Success [srcdecvideopad - encvideopad]");
					}
				}
			}
			if(handle->property->linked_audoutbin == TRUE) {
				if(handle->property->audioencoder != MM_AUDIOENCODER_NO_USE) {
					if(handle->param->seeking) {
						_mm_transcode_add_sink(handle, handle->decoder_audp->decsinkaudioqueue);
					}
					if(gst_pad_link(handle->decoder_audp->srcdecaudiopad, handle->encodebin->encaudiopad) != GST_PAD_LINK_OK) {
						debug_error("Error [srcdecaudiopad - encaudiopad]");
					} else {
						debug_log("Success [srcdecaudiopad - encaudiopad]");
					}
				}
			}
		}
	}
}

gboolean
_mm_cb_print_position(handle_s *handle)
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return FALSE;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return FALSE;
	}

	if(!handle->property->repeat_thread_exit) { /* To avoid gst_element_query_position bs */
		if (gst_element_query_position (handle->pipeline, &fmt, &pos)) {
			unsigned long current_pos =(unsigned long)(GST_TIME_AS_MSECONDS(pos));
			if(handle->param->seeking == FALSE) {
				handle->property->current_pos = current_pos;
				handle->property->real_duration= handle->property->total_length;
			} else if(handle->param->seeking == TRUE) {
				handle->property->current_pos = current_pos - handle->param->start_pos;
				if(handle->param->duration != 0) {
					if(handle->param->start_pos + handle->param->duration > handle->property->total_length) {
						handle->property->real_duration = handle->property->total_length - handle->param->start_pos;
					} else {
						handle->property->real_duration = handle->param->duration;
					}
				} else if(handle->param->duration == 0) { /* seek to origin file length */
					handle->property->real_duration = handle->property->total_length - handle->param->start_pos;
				}
			}

			if(handle->property->current_pos <= handle->property->real_duration) {
				if(handle->property->current_pos == 0 && handle->param->printed > 2) { /* 2 = 1000 / 500 minimum printed cnt for last buffer */
					handle->property->current_pos = handle->property->real_duration;
				}
				if(handle->property->progress_cb) {
					if(0 == handle->param->printed) {/* for first buffer */
						handle->property->current_pos = 0;
					}
					handle->property->progress_cb(handle->property->current_pos, handle->property->real_duration, handle->property->progress_cb_param);
					handle->param->printed++;
				}
			}
		}
	}

	return TRUE;
}

gboolean
_mm_cb_video_output_stream_probe(GstPad *pad, GstBuffer *buffer, gpointer user_data)
{
	handle_s *handle = (handle_s*) user_data;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return FALSE;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return FALSE;
	}

	gint64 start_pos_ts = handle->param->start_pos * G_GINT64_CONSTANT(1000000);

	if(GST_BUFFER_TIMESTAMP_IS_VALID (buffer)) {
		if(0 == handle->property->VIDFLAG++) {
			_mm_transcode_video_capsfilter(gst_buffer_get_caps (buffer), handle); /* Not drop buffer with 'return FALSE'*/

			if(handle->param->seeking) {
				debug_log("[VIDEO BUFFER TIMESTAMP] ([%"GST_TIME_FORMAT"])", GST_TIME_ARGS(start_pos_ts));
				GST_BUFFER_TIMESTAMP (buffer) = start_pos_ts;
			}
		}
	}
	return TRUE;
}

gboolean
_mm_cb_transcode_bus(GstBus * bus, GstMessage * message, gpointer userdata)
{
	handle_s* handle = (handle_s*) userdata;
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return FALSE;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return FALSE;
	}

	gint64 total_length;
	GstFormat fmt = GST_FORMAT_TIME;
	MMHandleType MMHandle = (MMHandleType) handle;

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_ERROR: {
		GError *err;
		gchar *debug;
		gst_message_parse_error (message, &err, &debug);

		debug_error("[Source: %s] Error: %s", GST_OBJECT_NAME(GST_OBJECT_CAST(GST_ELEMENT(GST_MESSAGE_SRC (message)))), err->message);

		ret = mm_transcode_cancel(MMHandle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Cancel Transcode");
		} else{
			debug_error("ERROR - Cancel Transcode");
			return FALSE;
		}

		if(err) {
			g_error_free (err);
			err = NULL;
		}
		TRANSCODE_FREE(debug);
		TRANSCODE_FREE(handle->param);
		/* g_main_loop_quit (handle->loop); */
		break;
	}

	case GST_MESSAGE_STATE_CHANGED: {
		if (GST_ELEMENT (GST_MESSAGE_SRC (message)) != handle->pipeline) {
			break;
		}
		GstState State_Old, State_New, State_Pending;
		gst_message_parse_state_changed (message, &State_Old, &State_New, &State_Pending);

		debug_log("[Source: %s] [State: %d -> %d]", GST_OBJECT_NAME(GST_OBJECT_CAST(GST_ELEMENT(GST_MESSAGE_SRC (message)))), State_Old, State_New);

		if (State_Old == GST_STATE_NULL && State_New == GST_STATE_READY) {
			debug_log("[Set State: Pause]");
			/* Pause Transcode */
			ret = _mm_transcode_state_change(handle, GST_STATE_PAUSED);
			if(ret == MM_ERROR_NONE) {
				debug_log("Success - Pause pipeline");
			} else{
				debug_error("ERROR - Pause pipeline");
				return FALSE;
			}
		}

		if (State_Old == GST_STATE_READY && State_New == GST_STATE_PAUSED) {
			/* Seek */
			debug_log("[%s] Start New Segment pipeline", handle->param->outputfile);
			ret = _mm_transcode_seek(handle);

			if(ret == MM_ERROR_NONE) {
				debug_log("Success - Set New Segment pipeline");
			} else {
				debug_log("[Null Trancode]");
				if(_mm_transcode_state_change(handle, GST_STATE_NULL) != MM_ERROR_NONE) {
					debug_error("ERROR -Null Pipeline");
					return FALSE;
				}
				g_mutex_lock (handle->property->thread_mutex);
				debug_log("[g_mutex_lock]");
				TRANSCODE_FREE(handle->param);
				debug_log("g_free(param)");
				g_cond_signal(handle->property->thread_cond);
				debug_log("[g_cond_signal]");
				g_mutex_unlock (handle->property->thread_mutex);
				debug_log("[g_mutex_unlock]");
			}
		}

		break;
	}

	case GST_MESSAGE_ASYNC_DONE: {
		if (GST_ELEMENT (GST_MESSAGE_SRC (message)) != handle->pipeline) {
			break;
		}

		if(gst_element_query_duration (handle->pipeline, &fmt, &total_length) && handle->property->total_length == 0) {
			debug_log("[GST_MESSAGE_ASYNC_DONE] Total Duration: %" GST_TIME_FORMAT " ", GST_TIME_ARGS (total_length));
			handle->property->total_length = (unsigned long)(GST_TIME_AS_MSECONDS(total_length));
		}

		handle->param->async_done = TRUE;
		debug_log("GST_MESSAGE_ASYNC_DONE");

		/* Play Transcode */
		debug_log("[Play Trancode] [%d ~ %d]", handle->param->start_pos, handle->property->end_pos);

		if(_mm_transcode_play (handle) != MM_ERROR_NONE) {
			debug_error("ERROR - Play Pipeline");
			return FALSE;
		}
		break;
	}

	case GST_MESSAGE_SEGMENT_DONE: {
		if (GST_ELEMENT (GST_MESSAGE_SRC (message)) != handle->pipeline) {
			break;
		}
		handle->param->segment_done = TRUE;
		debug_log("GST_MESSAGE_SEGMENT_DONE");
		break;
	}

	case GST_MESSAGE_EOS: {
		/* end-of-stream */
		debug_log("[GST_MESSAGE_EOS] end-of-stream");

		debug_log("[completed] %s (Transcode ID: %d)", handle->param->outputfile, handle->property->seek_idx++);
		handle->property->AUDFLAG = 0;
		handle->property->VIDFLAG = 0;

		/* Null Transcode */ /* Need to fresh filesink's property*/
		debug_log("[Null Trancode]");
		if(_mm_transcode_state_change(handle, GST_STATE_NULL) != MM_ERROR_NONE) {
			debug_error("ERROR -Null Pipeline");
			return FALSE;
		}

		if((handle->param->start_pos > handle->property->total_length && handle->property->total_length != 0 /* checkpoint once more here (eos)*/) && (handle->property->videoencoder != MM_VIDEOENCODER_NO_USE) /* Not unlink when Audio only */) {
			unlink(handle->param->outputfile);
			debug_log("[unlink] %s %d > %d", handle->param->outputfile, handle->param->start_pos, handle->property->total_length);
		}

		g_mutex_lock (handle->property->thread_mutex);
		g_free(handle->param);
		debug_log("g_free (param)");
		handle->param->completed = TRUE;
		handle->property->is_busy = FALSE;
		g_cond_signal(handle->property->thread_cond);
		debug_log("===> send completed signal");
		g_mutex_unlock (handle->property->thread_mutex);

		debug_log("[MMHandle] 0x%2x [msg_cb] 0x%2x [msg_cb_param] 0x%2x", MMHandle,handle->property->completed_cb, handle->property->completed_cb_param);

		if(handle->property->progress_cb) {
			handle->property->progress_cb(handle->property->real_duration, handle->property->real_duration, handle->property->progress_cb_param);
		}

		if(handle->property->completed_cb) {
			handle->property->completed_cb(MM_ERROR_NONE, handle->property->completed_cb_param);
		}

		break;
	}
	default:
		/* unhandle meage */
		/*debug_log("unhandle message");*/
		break;
	}
	return TRUE;
}

static void
_mm_transcode_add_sink(handle_s *handle , GstElement* sink_elements)
{

	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	if(sink_elements) {
		handle->property->sink_elements = g_list_append(handle->property->sink_elements, sink_elements);
		debug_log("g_list_append");
	}

}

static void
_mm_transcode_audio_capsfilter(GstCaps *caps, handle_s *handle)
{
	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	if (!caps) {
		debug_error("[ERROR] - caps");
		return;
	}

	if(!strcmp(handle->property->aenc, AMRENC)) {
		caps = gst_caps_new_simple("audio/x-raw-int",
				"rate", G_TYPE_INT, 8000,
				"channels", G_TYPE_INT, 1, NULL);
	} else if(!strcmp(handle->property->aenc, AACENC)) {
		caps = gst_caps_new_simple("audio/x-raw-int",
				"width", G_TYPE_INT, 16,
				"depth", G_TYPE_INT, 16,
				"rate", G_TYPE_INT, 44100,
				"channels", G_TYPE_INT, 1, NULL);
		gst_caps_set_simple (caps, "bitrate", GST_TYPE_INT_RANGE, 22050, 96000, NULL);
		debug_log("gst_caps_set_simple");
	}
	TRANSCODE_FREE(handle->property->audiodecodename);
	g_object_set(G_OBJECT(handle->encodebin->encbin), ACAPS, caps, NULL);
	debug_log("%s audiocaps: %s", handle->property->aenc, gst_caps_to_string(caps));
}

int
_mm_transcode_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = _mm_decodesrcbin_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create decodesrcbin");
	} else{
		debug_error("ERROR - Create decodesrcbin");
		return ret;
	}

	ret = _mm_encodebin_set_venc_aenc(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Setup video and audio encoder of encodebin");
	} else{
		debug_error("ERROR -Setup video and audio encoder of encodebin ");
		return ret;
	}

	ret = _mm_encodebin_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create encodebin");
	} else{
		debug_error("ERROR - Create encodebin");
		return ret;
	}

	/*create filesink */
	ret = _mm_filesink_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create Filesink");
	} else{
		debug_error("ERROR - Create Filesink");
		return ret;
	}

	return ret;
}

static int
_mm_transcode_exec(handle_s *handle, handle_param_s *param)
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

	g_mutex_lock (handle->property->thread_mutex);

	if(handle->property->repeat_thread_exit) {
		g_mutex_unlock (handle->property->thread_mutex);
		debug_log("unlock destory");
	} else {
		debug_log("start_pos: %d, duration: %d, seek_mode: %d output file name: %s\n", param->start_pos, param->duration, param->seek_mode, param->outputfile);
		handle->param = g_new0(handle_param_s, 1);
		/*g_value_init (handle->param, G_TYPE_INT);*/

		if (!handle->param) {
			debug_error("[ERROR] - handle param");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->param->resolution_width = param->resolution_width;
		handle->param->resolution_height = param->resolution_height;
		handle->param->fps_value = param->fps_value;
		handle->param->start_pos = param->start_pos;
		handle->param->duration = param->duration;
		handle->param->seek_mode = param->seek_mode;

		memset(handle->param->outputfile, 0, BUFFER_SIZE);
		strncpy(handle->param->outputfile, param->outputfile, strlen(param->outputfile));

		handle->param->seeking = param->seeking;
		handle->param->async_done = FALSE;
		handle->param->segment_done = FALSE;
		handle->param->completed = FALSE;
		handle->param->printed = 0;
		debug_log("[SEEK: %d] width: %d height: %d fps_value: %d start_pos: %d duration: %d seek_mode: %d outputfile: %s", handle->param->seeking, handle->param->resolution_width,
		handle->param->resolution_height, handle->param->fps_value, handle->param->start_pos, handle->param->duration, handle->param->seek_mode, handle->param->outputfile);

		if(handle->property->total_length != 0 && handle->param->start_pos > handle->property->total_length) {
			debug_log("[SKIP] [%s] because out of duration [%d < %d ~ %d] ", handle->param->outputfile, handle->property->total_length, handle->param->start_pos, handle->param->duration);
			g_mutex_unlock (handle->property->thread_mutex);
			debug_log("[thread_mutex unlock]");
		} else {
			g_object_set (G_OBJECT (handle->filesink), "location", handle->param->outputfile, NULL);
			debug_log("[%s] set filesink location", handle->param->outputfile);
			g_object_set (G_OBJECT (handle->filesink), "preroll-queue-len", 2, NULL);

			/* Ready Transcode */
			if(strlen(handle->param->outputfile) !=0) {
				debug_log("[Set State: Ready]");
				ret = _mm_transcode_state_change(handle, GST_STATE_READY);
				if(ret == MM_ERROR_NONE) {
					debug_log("Success - Ready pipeline");
				} else{
					debug_error("ERROR - Reay pipeline");
					g_mutex_unlock (handle->property->thread_mutex);
					return ret;
				}
			}

			if(0 == handle->property->seek_idx) {
				debug_log("Link Filesink");
				/*link filesink */
				ret = _mm_filesink_link(handle);
				if(ret == MM_ERROR_NONE) {
					debug_log("Success - Link Filesink");
				} else{
					debug_error("ERROR - Link Filesink");
					g_mutex_unlock (handle->property->thread_mutex);
					return ret;
				}
			}

			g_cond_wait(handle->property->thread_cond, handle->property->thread_mutex);
			debug_log("<=== get completed signal");
			g_mutex_unlock (handle->property->thread_mutex);
		}
	}

	return ret;
}

int
_mm_transcode_get_stream_info(handle_s *handle)
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

	if(strlen (handle->property->sourcefile) == 0) {
		debug_error ("Invalid arguments [sourcefile size 0]\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	int audio_track_num = 0;
	int video_track_num = 0;

	ret = mm_file_get_stream_info(handle->property->sourcefile, &audio_track_num, &video_track_num);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - mm_file_get_stream_info");
	} else{
		debug_error("ERROR - mm_file_get_stream_info");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(audio_track_num) {
		handle->property->has_audio_stream = TRUE;
	} else{
		handle->property->has_audio_stream = FALSE;
	}
	debug_log ("has_audio_stream: %d", handle->property->has_audio_stream);

	if(video_track_num) {
		handle->property->has_video_stream = TRUE;
	} else{
		handle->property->has_video_stream = FALSE;
	}
	debug_log ("has_video_stream: %d", handle->property->has_video_stream);

	if((handle->property->videoencoder != MM_VIDEOENCODER_NO_USE && !handle->property->has_video_stream) || (handle->property->audioencoder != MM_AUDIOENCODER_NO_USE && !handle->property->has_audio_stream)) {
		debug_error("No video || audio stream");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	return ret;
}

int
_mm_transcode_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret=_mm_decodesrcbin_link(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - decodesrcbin link");
	} else{
		debug_error("ERROR - decodesrcbin link");
		return ret;
	}

	ret = _mm_encodebin_link(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - encodebin link");
	} else{
		debug_error("ERROR - encodebin link");
		return ret;
	}

	return ret;
}

static void
_mm_transcode_video_capsfilter(GstCaps *caps, handle_s *handle)
{
	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	if(!caps) {
		debug_error("[ERROR] - caps");
		TRANSCODE_FREE(handle->property->videodecodename);
		return;
	}

	debug_log("[First Video Buffer] Set CapsFilter Parameter");
	_mm_transcode_video_capsfilter_set_parameter(caps, handle);

	/* Not support enlarge video resolution*/
	debug_log("Execute Resize");
	#if 0
	/* Not irrelevant to the ratio*/
	handle->param->resolution_height= handle->param->resolution_width * handle->in_height / handle->in_width;
	#endif

	debug_log("[Resize] resolution_width: %d, resolution_height: %d", handle->param->resolution_width, handle->param->resolution_height);
	if(0 == handle->param->resolution_width || 0 == handle->param->resolution_height) {
		debug_log("[Origin Resolution] Two resolutoin value = 0");
		handle->param->resolution_width = handle->property->in_width;
		handle->param->resolution_height = handle->property->in_height;
	}

	if(handle->param->resolution_width < VIDEO_RESOLUTION_WIDTH_SQCIF || handle->param->resolution_height < VIDEO_RESOLUTION_HEIGHT_SQCIF) {
		debug_log("The Minimun resolution is SQCIF");
		handle->param->resolution_width = VIDEO_RESOLUTION_WIDTH_SQCIF;
		handle->param->resolution_height = VIDEO_RESOLUTION_HEIGHT_SQCIF;
	}

	if(handle->property->in_width < handle->param->resolution_width || handle->property->in_height < handle->param->resolution_height) {
		debug_log("[Origin Resolution] resolutoin value > origin resolution");
		handle->param->resolution_width = handle->property->in_width;
		handle->param->resolution_height = handle->property->in_height;
	}

	debug_log("[Call CapsFilter] resolution_width: %d, resolution_height: %d", handle->param->resolution_width, handle->param->resolution_height);
	_mm_transcode_video_capsfilter_call(handle);
	TRANSCODE_FREE(handle->property->videodecodename);
}

static void
_mm_transcode_video_capsfilter_call(handle_s *handle)
{
	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	/* Configure videoscale to use 4-tap scaling for higher quality */
	debug_log("Input Width: [%d] Input Hieght: [%d] Output Width: [%d], Output Height: [%d]", handle->property->in_width, handle->property->in_height, handle->param->resolution_width, handle->param->resolution_height);

	g_object_set (G_OBJECT (handle->decoder_vidp->vidflt), "caps", gst_caps_new_simple(handle->property->mime,
										"format", GST_TYPE_FOURCC, handle->property->fourcc,
										"width", G_TYPE_INT, handle->param->resolution_width,
										"height", G_TYPE_INT, handle->param->resolution_height,
										"framerate", GST_TYPE_FRACTION, handle->property->fps_n, handle->property->fps_d,
										"pixel-aspect-ratio", GST_TYPE_FRACTION, handle->property->aspect_x, handle->property->aspect_y,
										NULL), NULL);
}

static void
_mm_transcode_video_capsfilter_set_parameter(GstCaps *caps, handle_s *handle)
{
	const GValue *par, *fps;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return;
	}

	if(!caps) {
		debug_error("[ERROR] - caps");
		return;
	}

	debug_log("caps: %s", gst_caps_to_string(caps));
	GstStructure *_str = gst_caps_get_structure (caps, 0);
	handle->property->mime = _mm_check_media_type(caps);
	debug_log("mime: %s", handle->property->mime);

	gst_structure_get_fourcc (_str, "format", &handle->property->fourcc);
	if (GST_MAKE_FOURCC ('I', '4', '2', '0') == handle->property->fourcc) {
		debug_log("GST_MAKE_FOURCC ('I', '4', '2', '0')");
	} else if(GST_MAKE_FOURCC ('R', 'G', 'B',' ') == handle->property->fourcc) {
		debug_log("GST_MAKE_FOURCC ('R', 'G', 'B', ' ')");
	} else if(GST_MAKE_FOURCC ('S', 'N', '1', '2') == handle->property->fourcc) {
		debug_log("GST_MAKE_FOURCC ('S', 'N', '1', '2')");
	} else if(GST_MAKE_FOURCC ('S', 'T', '1', '2') == handle->property->fourcc) {
		debug_log("GST_MAKE_FOURCC ('S', 'T', '1', '2')");
	} else if(GST_MAKE_FOURCC ('N', 'V', '1', '2') == handle->property->fourcc) {
		debug_log("GST_MAKE_FOURCC ('N', 'V', '1', '2')");
	}

	if(!gst_structure_get_int(_str, "width", &handle->property->in_width) || !gst_structure_get_int(_str, "height", &handle->property->in_height)) {
		debug_error("error gst_structure_get_int [width] [height]");
	} else {
		debug_log("Origin File's Width: [%u] Origin File's Hieght: [%u]", handle->property->in_width, handle->property->in_height);
	}

	fps = gst_structure_get_value (_str, "framerate");

	if (fps) {
		handle->property->fps_n = gst_value_get_fraction_numerator (fps);
		handle->property->fps_d = gst_value_get_fraction_denominator (fps);
		debug_log("[Origin framerate] gst_value_get_fraction_numerator: %d, gst_value_get_fraction_denominator: %d", handle->property->fps_n, handle->property->fps_d);
	}

	if(handle->param->fps_value>= 5 && handle->param->fps_value <= 30 && handle->param->fps_value <= handle->property->fps_n) {
		handle->property->fps_n = (gint) handle->param->fps_value;
		handle->property->fps_d = 1;
	}
	debug_log("[framerate] gst_value_get_fraction_numerator: %d, gst_value_get_fraction_denominator: %d", handle->property->fps_n, handle->property->fps_d);

	par = gst_structure_get_value (_str, "pixel-aspect-ratio");
	if (par) {
		handle->property->aspect_x= gst_value_get_fraction_numerator (par);
		handle->property->aspect_y = gst_value_get_fraction_denominator (par);
	} else {
		handle->property->aspect_x = handle->property->aspect_y = 1;
	}
	debug_log("[pixel-aspect-ratio] gst_value_get_fraction_numerator: %d, gst_value_get_fraction_denominator: %d", handle->property->aspect_x, handle->property->aspect_y);

}

int
_mm_transcode_set_handle_element(handle_s *handle, const char * in_Filename, mm_containerformat_e containerformat,
	mm_videoencoder_e videoencoder, mm_audioencoder_e audioencoder)
{
	int ret = MM_ERROR_NONE;

	if( !handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(in_Filename == NULL) {
		debug_error ("Invalid arguments [filename null]\n");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(strlen (in_Filename) == 0 || strlen (in_Filename) > BUFFER_SIZE) {
		debug_error ("Invalid arguments [filename size: %d]\n", strlen (in_Filename));
		return MM_ERROR_INVALID_ARGUMENT;
	}

	memset(handle->property->sourcefile, 0, BUFFER_SIZE);
	strncpy(handle->property->sourcefile, in_Filename, strlen (in_Filename));
	debug_log("%s(%d)", handle->property->sourcefile, strlen (in_Filename));

	handle->property->containerformat = containerformat;
	handle->property->videoencoder = videoencoder;
	handle->property->audioencoder = audioencoder;

	debug_log("container format: %d videoencoder:%d, audioencoder: %d", handle->property->containerformat, handle->property->videoencoder, handle->property->audioencoder);

	return ret;
}

int
_mm_transcode_set_handle_parameter(handle_param_s *param, unsigned int resolution_width, unsigned int resolution_height, unsigned int fps_value, unsigned long start_pos,
	unsigned long duration, mm_seek_mode_e seek_mode, const char *out_Filename)
{
	int ret = MM_ERROR_NONE;

	if(!param) {
		debug_error("param error");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	param->resolution_width = resolution_width;
	param->resolution_height = resolution_height;
	param->fps_value = fps_value;

	param->start_pos = start_pos;
	param->duration = duration;
	param->seek_mode = seek_mode;
	debug_log("resolution_width: %d, resolution_height: %d, fps_value: %d, start_pos: %d, duration: %d, seek_mode: %d \n", param->resolution_width, param->resolution_height, fps_value, param->start_pos, param->duration, param->seek_mode);

	if(start_pos == 0 && duration == 0) {
		param->seeking = FALSE;
	} else {
		param->seeking = TRUE;
	}

	if(out_Filename) {
		memset(param->outputfile, 0, BUFFER_SIZE);
		strncpy(param->outputfile, out_Filename, strlen (out_Filename));
		debug_log("%s(%d)", param->outputfile, strlen (out_Filename));
		debug_log("output file name: %s", param->outputfile);
	} else {
		debug_error("out_Filename error");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	return ret;
}

int
_mm_transcode_state_change(handle_s *handle, GstState gst_state)
{
	int ret = MM_ERROR_NONE;
	GstStateChangeReturn ret_state;

	if(gst_state == GST_STATE_NULL) {
		debug_log("Before - GST_STATE_NULL");
	} else if(gst_state == GST_STATE_READY) {
		debug_log("Before - GST_STATE_READY");
	} else if(gst_state == GST_STATE_PAUSED) {
		debug_log("Before - GST_STATE_PAUSED");
	} else if(gst_state == GST_STATE_PLAYING) {
		debug_log("Before - GST_STATE_PLAYING");
	}
	ret_state = gst_element_set_state (handle->pipeline,gst_state /* GST_STATE_READY GST_STATE_PAUSED GST_STATE_NULL*/);
	if(ret_state == GST_STATE_CHANGE_FAILURE) {
		if(gst_state == GST_STATE_NULL) {
			debug_error("ERROR - SET GST_STATE_NULL");
		} else if(gst_state == GST_STATE_READY) {
			debug_error("ERROR - SET GST_STATE_READY");
		} else if(gst_state == GST_STATE_PAUSED) {
			debug_error("ERROR - SET GST_STATE_PAUSED");
		} else if(gst_state == GST_STATE_PLAYING) {
			debug_error("ERROR - SET GST_STATE_PLAYING");
		}
		return MM_ERROR_TRANSCODE_INTERNAL;
	} else {
		if(gst_state == GST_STATE_NULL) {
			debug_log("Success - SET GST_STATE_NULL");
		} else if(gst_state == GST_STATE_READY) {
			debug_log("Success - SET GST_STATE_READY");
		} else if(gst_state == GST_STATE_PAUSED) {
			debug_log("Success - SET GST_STATE_PAUSED");
		} else if(gst_state == GST_STATE_PLAYING) {
			debug_log("Success - SET GST_STATE_PLAYING");
		}
	}

	ret_state = gst_element_get_state (handle->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	if(ret_state == GST_STATE_CHANGE_FAILURE) {
		if(gst_state == GST_STATE_NULL) {
			debug_error("ERROR - GET GST_STATE_NULL");
		} else if(gst_state == GST_STATE_READY) {
			debug_error("ERROR - GET GST_STATE_READY");
		} else if(gst_state == GST_STATE_PAUSED) {
			debug_error("ERROR - GET GST_STATE_PAUSED");
		} else if(gst_state == GST_STATE_PLAYING) {
			debug_error("ERROR - GET GST_STATE_PLAYING");
		}
		return MM_ERROR_TRANSCODE_INTERNAL;
	} else {
		if(gst_state == GST_STATE_NULL) {
			debug_log("Success - GET GST_STATE_NULL");
		} else if(gst_state == GST_STATE_READY) {
			debug_log("Success - GET GST_STATE_READY");
		} else if(gst_state == GST_STATE_PAUSED) {
			debug_log("Success - GET GST_STATE_PAUSED");
		} else if(gst_state == GST_STATE_PLAYING) {
			debug_log("Success - GET GST_STATE_PLAYING");
		}
	}

	return ret;
}

int
_mm_transcode_param_flush(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->property->linked_vidoutbin = FALSE;
	handle->property->linked_audoutbin = FALSE;
	handle->encodebin->encodebin_profile = 0;
	handle->property->AUDFLAG = 0;
	handle->property->VIDFLAG = 0;

	handle->property->total_length = 0;
	handle->property->repeat_thread_exit = FALSE;
	handle->property->is_busy = FALSE;
	handle->property->audio_cb_probe_id= 0;
	handle->property->video_cb_probe_id= 0;
	handle->property->progrss_event_id= 0;
	handle->property->seek_idx = 0;

	return ret;
}

static int
_mm_transcode_play(handle_s *handle)
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

	ret = _mm_transcode_state_change(handle, GST_STATE_PLAYING);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR -Playing Pipeline");
		return ret;
	}

	debug_log("[SEEK: %d] width: %d height: %d start_pos: %d duration: %d (%d) seek_mode: %d outputfile: %s",handle->param->seeking, handle->param->resolution_width,
		handle->param->resolution_height, handle->param->start_pos, handle->param->duration, handle->property->end_pos, handle->param->seek_mode, handle->param->outputfile);

	handle->property->progrss_event_id = g_timeout_add (LAZY_PAUSE_TIMEOUT_MSEC, (GSourceFunc) _mm_cb_print_position, handle);
	debug_log ("Timer (id=[%d], timeout=[%d ms])\n", handle->property->progrss_event_id, LAZY_PAUSE_TIMEOUT_MSEC);

	return ret;
}

static int
_mm_transcode_seek(handle_s *handle)
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

	GList *walk_element = handle->property->sink_elements;
	gint64 start_pos, end_pos;
	gdouble rate = 1.0;
	GstSeekFlags _Flags = GST_SEEK_FLAG_NONE;

	start_pos = handle->param->start_pos * G_GINT64_CONSTANT(1000000);
	handle->property->end_pos = handle->param->start_pos + handle->param->duration;

	if(handle->param->start_pos > handle->property->total_length && handle->property->seek_idx) {
		debug_error("[%d ~ %d] out of %d", handle->param->start_pos, handle->property->end_pos, handle->property->total_length);
		return MM_ERROR_TRANSCODE_SEEK_FAILED;
	} else {
		if(handle->param->duration != 0) {
			end_pos = start_pos + handle->param->duration * G_GINT64_CONSTANT(1000000);
		} else if(handle->param->duration == 0) { /* seek to origin file length */
			end_pos = handle->property->total_length * G_GINT64_CONSTANT(1000000);
		}

		debug_log("seek time : [ (%d msec) : (%d msec) ]\n", handle->param->start_pos, handle->property->end_pos);

		while (walk_element) {
			GstElement *seekable_element = GST_ELEMENT (walk_element->data);

			if(!seekable_element) {
				debug_error("ERROR - seekable");
				return MM_ERROR_TRANSCODE_INTERNAL;
			}

			if(handle->param->seek_mode == MM_SEEK_ACCURATE) {
				_Flags = GST_SEEK_FLAG_ACCURATE;
			} else if(handle->param->seek_mode == MM_SEEK_INACCURATE) {
				_Flags = GST_SEEK_FLAG_KEY_UNIT;
			}

			if(!gst_element_seek(seekable_element, rate, GST_FORMAT_TIME, _Flags, GST_SEEK_TYPE_SET, start_pos, GST_SEEK_TYPE_SET, end_pos)) {
				debug_error("ERROR - gst_element_seek (on) event : %s", GST_OBJECT_NAME(GST_OBJECT_CAST(seekable_element)));
				return MM_ERROR_TRANSCODE_SEEK_FAILED;
			}

			if(walk_element) {
				walk_element = g_list_next (walk_element);
			}
		}
	}

	return ret;
}

int
_mm_transcode_thread(handle_s *handle)
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

	if(!handle->property->thread_mutex) {
		handle->property->thread_mutex = g_mutex_new();
		debug_log("create thread_mutex: 0x%2x", handle->property->thread_mutex);
	} else {
		debug_error("ERROR - thread_mutex is already created");
	}

	if(!handle->property->thread_exit_mutex) {
		handle->property->thread_exit_mutex = g_mutex_new();
		debug_log("create exit mutex: 0x%2x", handle->property->thread_exit_mutex);
	} else {
		debug_error("ERROR - thread_exit_mutex is already created");
	}

	/*These are a communicator for thread*/
	if(!handle->property->queue) {
		handle->property->queue = g_async_queue_new();
		debug_log("create async queue: 0x%2x", handle->property->queue);
	} else {
		debug_error("ERROR - async queue is already created");
	}

	if(!handle->property->thread_cond) {
		handle->property->thread_cond = g_cond_new();
		debug_log("create thread cond: 0x%2x", handle->property->thread_cond);
	} else {
		debug_error("thread cond is already created");
	}

	/*create threads*/
	debug_log("create thread");
	handle->property->thread = g_thread_create ((GThreadFunc)_mm_transcode_thread_repeate, (gpointer)handle, TRUE, NULL);
	if(!handle->property->thread) {
		debug_error("ERROR - create thread");
		return MM_ERROR_TRANSCODE_INTERNAL;
	} else {
		debug_log("create thread: 0x%2x", handle->property->thread);
	}

	return ret;
}

static gpointer
_mm_transcode_thread_repeate(gpointer data)
{
	handle_s* handle = (handle_s*) data;
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return NULL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return NULL;
	}

	while (1) { /* thread while */
		int length = g_async_queue_length(handle->property->queue);
		if(length) {
			debug_log("[QUEUE #] %d", length);
			handle->property->is_busy = TRUE;
		}

		handle_param_s *pop_data = (handle_param_s *) g_async_queue_pop(handle->property->queue);

		if(handle->property->repeat_thread_exit || !handle->property->is_busy) {
			debug_log("[Receive Last Queue]");
			debug_log("[Destroy]");
			break;
		} else {
			debug_log("[pop queue] resolution_width: %d, resolution_height: %d, start_pos: %d, duration: %d, seek_mode: %d outputfile: %s\n",
			pop_data->resolution_width, pop_data->resolution_height, pop_data->start_pos, pop_data->duration, pop_data->seek_mode, pop_data->outputfile);

			MMTA_INIT();
			__ta__("_mm_transcode_exec",
			ret = _mm_transcode_exec(handle, pop_data); /* Need to block */
			);
			MMTA_ACUM_ITEM_SHOW_RESULT();
			MMTA_RELEASE ();
			if(ret == MM_ERROR_NONE) {
				debug_log("Success - transcode_exec");
			} else{
				debug_log("Destroy - transcode_exec");
				debug_log("<=== get exit (%d) signal", handle->property->repeat_thread_exit);
				break;
			}
		}
	}
	debug_log("exit thread");
	return NULL;
}
