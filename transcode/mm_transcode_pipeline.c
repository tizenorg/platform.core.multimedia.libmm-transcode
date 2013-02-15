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

static int _mm_decode_audio_output_create(handle_s *handle);
static int _mm_decode_audio_output_link(handle_s *handle);
static int _mm_decode_video_output_create(handle_s *handle);
static int _mm_decode_video_output_link(handle_s *handle);
static int _mm_decodebin_pipeline_create(handle_s *handle);
static int _mm_encodebin_set_audio_property(handle_s *handle);
static int _mm_encodebin_set_property(handle_s *handle);
static int _mm_encodebin_set_video_property(handle_s *handle);
static int _mm_filesrc_pipeline_create(handle_s *handle);
static int _mm_filesrc_decodebin_link(handle_s *handle);

const gchar*
_mm_check_media_type(GstCaps *caps)
{
	/* check media type */
	GstStructure *_str = gst_caps_get_structure (caps, 0);
	const gchar*mime = gst_structure_get_name(_str);

	return mime;
}

int
_mm_cleanup_encodebin(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* release videopad */
	if(handle->decvideosinkpad) {
		gst_object_unref(GST_OBJECT(handle->decvideosinkpad));
		handle->decvideosinkpad = NULL;
		debug_log("Success - gt_object_unref (decvideosinkpad)");
	}

	if(handle->decvideosrcpad) {
		gst_object_unref(GST_OBJECT(handle->decvideosrcpad));
		handle->decvideosrcpad = NULL;
		debug_log("Success - gst_object_unref (decvideosrcpad)");
	}

	if(handle->srcdecvideopad) {
		gst_object_unref (GST_OBJECT(handle->srcdecvideopad));
		handle->srcdecvideopad = NULL;
		debug_log("Success - gst_object_unref (srcdecvideopad)");
	}

	if(handle->encvideopad) {
		gst_object_unref (GST_OBJECT(handle->encvideopad));
		handle->encvideopad = NULL;
		debug_log("Success - gst_object_unref (encvideopad)");
	}

	/* release audiopad */
	if(handle->decaudiosinkpad) {
		gst_object_unref (GST_OBJECT(handle->decaudiosinkpad));
		handle->decaudiosinkpad=NULL;
		debug_log("Success - gst_object_unref (decaudiosinkpad)");
	}

	if(handle->decaudiosrcpad) {
		gst_object_unref (GST_OBJECT(handle->decaudiosrcpad));
		handle->decaudiosrcpad=NULL;
		debug_log("Success - gst_object_unref (decaudiosrcpad)");
	}

	if(handle->srcdecaudiopad) {
		gst_object_unref (GST_OBJECT(handle->srcdecaudiopad));
		handle->srcdecaudiopad = NULL;
		debug_log("Success - gst_object_unref (srcdecaudiopad)");
	}

	if(handle->encaudiopad) {
		gst_object_unref (GST_OBJECT(handle->encaudiopad));
		handle->encaudiopad = NULL;
		debug_log("Success - gst_object_unref (encaudiopad)");
	}

	if(handle->caps) {
		gst_caps_unref (handle->caps);
		handle->caps = NULL;
		debug_log("gst_caps_unref");
	}

	return ret;
}

int
_mm_cleanup_pipeline(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->_MMHandle = 0;

	/* g_thread_exit(handle->thread); */
	debug_log("g_thread_exit");
	if(handle->thread) {
		g_thread_join(handle->thread);
		debug_log("Success - join (thread)");
	}

	/* disconnecting bus watch */
	if ( handle->bus_watcher ) {
		g_source_remove(handle->bus_watcher);
		debug_log("g_source_remove");
		handle->bus_watcher = 0;
	}

	if(handle->queue) {
		g_async_queue_unref(handle->queue);
		handle->queue = NULL;
		debug_log("Success - g_async_queue_unref(queue)");
	}

	if(handle->thread_mutex) {
		g_mutex_free (handle->thread_mutex);
		handle->thread_mutex = NULL;
		debug_log("Success - free (thread_mutex)");
	}

	if(handle->thread_cond) {
		g_cond_free (handle->thread_cond);
		handle->thread_cond = NULL;
		debug_log("Success - free (thread_cond)");
	}

	if(handle->thread_exit_mutex) {
		g_mutex_free (handle->thread_exit_mutex);
		handle->thread_exit_mutex = NULL;
		debug_log("Success - free (thread_exit_mutex)");
	}

	if(handle->sinkdecvideopad) {
		gst_object_unref (GST_OBJECT(handle->sinkdecvideopad));
		handle->sinkdecvideopad = NULL;
		debug_log("Success - gst_object_unref (sinkdecvideopad)");
	}

	if(handle->sinkdecaudiopad) {
		gst_object_unref (GST_OBJECT(handle->sinkdecaudiopad));
		handle->sinkdecaudiopad = NULL;
		debug_log("Success - gst_object_unref (sinkdecaudiopad)");
	}

	if(_mm_cleanup_encodebin(handle) != MM_ERROR_NONE) {
		debug_error("ERROR -Play Pipeline");
		return FALSE;
	} else {
		debug_log("Success -clean encodebin");
	}

	if(handle->sink_elements) {
		g_list_free (handle->sink_elements);
		handle->sink_elements = NULL;
		debug_log("Success - g_list_free (sink_elements)");
	}

	if(handle->pipeline) {
		gst_object_unref (handle->pipeline);
		handle->pipeline = NULL;
		debug_log("Success - gst_object_unref (pipeline)");
	}

	if(handle->audio_cb_probe_id) {
		g_source_remove (handle->audio_cb_probe_id);
		handle->audio_cb_probe_id = 0;
		debug_log("g_source_remove (audio_cb_probe_id)");
	}

	if(handle->video_cb_probe_id) {
		g_source_remove (handle->video_cb_probe_id);
		handle->video_cb_probe_id = 0;
		debug_log("g_source_remove (video_cb_probe_id)");
	}

	if(handle->progrss_event_id) {
		g_source_remove (handle->progrss_event_id);
		handle->progrss_event_id = 0;
		debug_log("g_source_remove (progrss_event_id)");
	}

	TRANSCODE_FREE (handle->mux);
	TRANSCODE_FREE (handle->venc);
	TRANSCODE_FREE (handle->aenc);
	TRANSCODE_FREE (handle);

	return ret;
}

static int
_mm_decode_audio_output_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->decaudiobin = gst_bin_new ("audiobin");
	if (!handle->decaudiobin) {
		debug_error("decaudiobin could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decsinkaudioqueue = gst_element_factory_make("queue","decsinkaudioqueue");
	if (!handle->decsinkaudioqueue) {
		debug_error("decsinkaudioqueue could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decaudiosinkpad = gst_element_get_static_pad (handle->decsinkaudioqueue, "sink");
	if (!handle->decaudiosinkpad) {
		debug_error("decaudiosinkpad could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->audioencoder == MM_AUDIOENCODER_NO_USE) {
		debug_log("[MM_AUDIOENCODER_NO_USE] fakesink create");

		handle->audiofakesink = gst_element_factory_make ("fakesink", "audiofakesink");
		if (!handle->audiofakesink) {
			debug_error("fakesink element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}
	} else {
		handle->aconv = gst_element_factory_make ("audioconvert", "aconv");
		if (!handle->aconv) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->valve = gst_element_factory_make ("valve", "valve");
		if (!handle->valve) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_set(handle->valve,"drop", FALSE, NULL);

		handle->resample = gst_element_factory_make ("audioresample", "audioresample");
		if (!handle->resample) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->audflt = gst_element_factory_make ("capsfilter", "afilter");
		if (!handle->audflt) {
			debug_error("audflt element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->decaudiosrcpad = gst_element_get_static_pad (handle->audflt, "src");
		if (!handle->decaudiosrcpad) {
			debug_error("decaudiosrcpad element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}
	}

	return ret;
}

static int
_mm_decode_audio_output_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(handle->audioencoder == MM_AUDIOENCODER_NO_USE) {
		debug_log("[MM_AUDIOENCODER_NO_USE] fakesink pad create");

		gst_bin_add_many (GST_BIN (handle->decaudiobin), handle->decsinkaudioqueue, handle->audiofakesink, NULL);
		if(!gst_element_link_many(handle->decsinkaudioqueue, handle->audiofakesink, NULL)) {
			debug_error("[Audio Output Bin] gst_element_link_many failed");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		gst_element_add_pad (handle->decaudiobin, gst_ghost_pad_new("decbin_audiosink", handle->decaudiosinkpad));
		handle->sinkdecaudiopad = gst_element_get_static_pad (handle->decaudiobin, "decbin_audiosink"); /* get sink audiopad of decodebin */
	} else {
		gst_bin_add_many (GST_BIN (handle->decaudiobin), handle->decsinkaudioqueue, handle->valve, handle->aconv, handle->resample, handle->audflt, NULL);
		if(!gst_element_link_many(handle->decsinkaudioqueue, handle->valve, handle->aconv, handle->resample, handle->audflt, NULL)) {
			debug_error("[Audio Output Bin] gst_element_link_many failed");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		gst_element_add_pad (handle->decaudiobin, gst_ghost_pad_new("decbin_audiosink", handle->decaudiosinkpad));
		gst_element_add_pad (handle->decaudiobin, gst_ghost_pad_new("decbin_audiosrc", handle->decaudiosrcpad));

		handle->sinkdecaudiopad = gst_element_get_static_pad (handle->decaudiobin, "decbin_audiosink"); /* get sink audiopad of decodebin */
		handle->srcdecaudiopad = gst_element_get_static_pad (handle->decaudiobin, "decbin_audiosrc"); /* get src audiopad of decodebin */

		handle->audio_cb_probe_id = gst_pad_add_buffer_probe (handle->sinkdecaudiopad, G_CALLBACK (_mm_cb_audio_output_stream_probe), handle);
		debug_log("audio_cb_probe_id: %d", handle->audio_cb_probe_id); /* must use sinkpad (sinkpad => srcpad) for normal resized video buffer*/
	}

	gst_bin_add (GST_BIN (handle->pipeline), handle->decaudiobin);
	return ret;
}

static int
_mm_decode_video_output_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->decvideobin= gst_bin_new("videobin");
	if (!handle->decvideobin) {
		debug_error("decvideobin could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decsinkvideoqueue = gst_element_factory_make("queue","decsinkvideoqueue");
	if (!handle->decsinkvideoqueue) {
		debug_error("decsinkvideoqueue element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decvideosinkpad = gst_element_get_static_pad(handle->decsinkvideoqueue,"sink");
	if (!handle->decvideosinkpad) {
		debug_error("decvideosinkpad element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->videorate = gst_element_factory_make("videorate", "videorate");
	if (!handle->videorate) {
		debug_error("videorate element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	g_object_set (handle->videorate, "drop-only", TRUE,"average-period", GST_SECOND/2, NULL);
	g_object_set (handle->videorate, "max-rate", 30, NULL);
	g_object_set (handle->videorate, "skip-to-first", TRUE, NULL);

	handle->videoscale = gst_element_factory_make("videoscale", "scaler");
	if (!handle->videoscale) {
		debug_error("videoscale element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}
	/* Configure videoscale to use 4-tap scaling for higher quality */
	g_object_set (handle->videoscale, "method", 2, NULL);

	handle->vidflt = gst_element_factory_make("capsfilter", "vfilter");
	if (!handle->vidflt) {
		debug_error("One element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decvideosrcpad = gst_element_get_static_pad(handle->vidflt,"src");
	if (!handle->decvideosrcpad) {
		debug_error("decvideosrcpad element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}

static int
_mm_decode_video_output_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	gst_bin_add_many(GST_BIN(handle->decvideobin), handle->decsinkvideoqueue, handle->videoscale,  handle->videorate, handle->vidflt, NULL);
	if(!gst_element_link_many(handle->decsinkvideoqueue, handle->videoscale, handle->videorate, handle->vidflt, NULL)) {
		debug_error("[Video Output Bin] gst_element_link_many failed");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	gst_element_add_pad(handle->decvideobin, gst_ghost_pad_new("decbin_videosink", handle->decvideosinkpad));
	gst_element_add_pad(handle->decvideobin, gst_ghost_pad_new("decbin_videosrc", handle->decvideosrcpad));

	handle->sinkdecvideopad = gst_element_get_static_pad(handle->decvideobin,"decbin_videosink");
	handle->srcdecvideopad = gst_element_get_static_pad(handle->decvideobin,"decbin_videosrc");

	handle->video_cb_probe_id = gst_pad_add_buffer_probe (handle->sinkdecvideopad, G_CALLBACK (_mm_cb_video_output_stream_probe), handle);
	debug_log("video_cb_probe_sink_id: %d", handle->video_cb_probe_id); /* must use sinkpad (sinkpad => srcpad) */

	gst_bin_add(GST_BIN(handle->pipeline), handle->decvideobin);

	return ret;
}

static int
_mm_decodebin_pipeline_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	handle->decbin = gst_element_factory_make ("decodebin2", "decoder"); /* autoplug-select is not worked when decodebin */

	if (!handle->decbin) {
		debug_error("decbin element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	g_signal_connect (handle->decbin, "new-decoded-pad",G_CALLBACK(_mm_cb_decoder_newpad_encoder), handle);
	g_signal_connect (handle->decbin, "autoplug-select", G_CALLBACK(_mm_cb_decode_bin_autoplug_select), handle);

	return ret;
}

int
_mm_decodesrcbin_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret = _mm_filesrc_pipeline_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create filesrc pipeline");
	} else{
		debug_error("ERROR -Create filesrc pipeline");
		return ret;
	}

	if(handle->has_audio_stream) {
		ret = _mm_decode_audio_output_create(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Create audiobin pipeline: 0x%2x",handle->decaudiobin);
		} else{
			debug_error("ERROR - Create audiobin pipeline");
			return ret;
		}
	}

	if(handle->has_video_stream) {
		ret = _mm_decode_video_output_create(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Create videobin pipeline: 0x%2x",handle->decvideobin);
		} else{
			debug_error("ERROR -Create videobin pipeline");
			return ret;
		}
	}

	ret = _mm_decodebin_pipeline_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create decodebin pipeline");
	} else{
		debug_error("ERROR - Create decodebin pipeline");
		return ret;
	}

	return ret;
}

int
_mm_decodesrcbin_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	ret=_mm_filesrc_decodebin_link(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - _mm_filesrc_decodebin_link");
	} else{
		debug_error("ERROR - _mm_filesrc_decodebin_link");
		return ret;
	}

	if(handle->has_audio_stream) {
		ret = _mm_decode_audio_output_link(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - _mm_decode_audio_output_link");
		} else{
			debug_error("ERROR - _mm_decode_audio_output_link");
			return ret;
		}
	}

	if(handle->has_video_stream) {
		ret=_mm_decode_video_output_link(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - _mm_decode_video_output_link");
		} else{
			debug_error("ERROR - _mm_decode_video_output_link");
			return ret;
		}
	}

	return ret;
}

int
_mm_encodebin_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->encbin = gst_element_factory_make ("encodebin", "encodebin");

	if (!handle->encbin ) {
		debug_error("encbin element could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->videoencoder != MM_VIDEOENCODER_NO_USE && handle->audioencoder != MM_AUDIOENCODER_NO_USE) {
		ret = _mm_encodebin_set_video_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin video property");
			return ret;
		}

		ret = _mm_encodebin_set_audio_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin audio property");
			return ret;
		}
	} else if(handle->videoencoder != MM_VIDEOENCODER_NO_USE) {
		ret = _mm_encodebin_set_video_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin video property");
			return ret;
		}
	} else if(handle->audioencoder != MM_AUDIOENCODER_NO_USE) {
		handle->encodebin_profile = 1;
		debug_log("[AUDIO ONLY ENCODE]");
		ret = _mm_encodebin_set_audio_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin video property");
			return ret;
		}
	}

	debug_log("[Encodebin Profile: %d]", handle->encodebin_profile);
	ret = _mm_encodebin_set_property(handle);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR -Setup encodebin property");
		return ret;
	}

	return ret;
}

int
_mm_encodebin_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Add encodebin to pipeline */
	gst_bin_add(GST_BIN(handle->pipeline), handle->encbin);

	if(handle->has_video_stream) {
		if(handle->videoencoder != MM_VIDEOENCODER_NO_USE) {
			handle->encvideopad = gst_element_get_request_pad(handle->encbin, "video");
			if(handle->encvideopad) {
				debug_log("encvideopad: 0x%2x", handle->encvideopad);
			} else {
				debug_error("error encvideopad");
				return MM_ERROR_TRANSCODE_INTERNAL;
			}
		}
	}

	if(handle->has_audio_stream) {
		if(handle->audioencoder != MM_AUDIOENCODER_NO_USE) {
			handle->encaudiopad = gst_element_get_request_pad(handle->encbin, "audio");
			if(handle->encaudiopad) {
				debug_log("encaudiopad: 0x%2x", handle->encaudiopad);
			} else {
				debug_error("error encaudiopad");
				return MM_ERROR_TRANSCODE_INTERNAL;
			}
		}
	}

	return ret;
}

static int
_mm_encodebin_set_audio_property(handle_s* handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(handle->has_audio_stream) {
		/* Audio */
		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), ARS)) {
			g_object_set(G_OBJECT(handle->encbin), ARS, 0, NULL);
			debug_log("[AUDIO RESAMPLE] encbin set auto-audio-resample");
		} else {
			debug_error("error [AUDIO RESAMPLE] encbin set auto-audio-resample");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), ACON)) {
			g_object_set(G_OBJECT(handle->encbin), ACON, 1, NULL);
			debug_log("encbin set auto-audio-convert");
		} else {
			debug_error("error encbin set auto-audio-convert");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), AENC)) {
			g_object_set(G_OBJECT(handle->encbin), AENC, handle->aenc, NULL);
			debug_log("[AUDIOENCODER] encbin set [%s: %s]",AENC, handle->aenc);
		} else {
			debug_error("error [AUDIOENCODER] encbin set [%s: %s]",AENC, handle->aenc);
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_get(G_OBJECT(handle->encbin), "use-aenc-queue", &(handle->use_aencqueue), NULL);
		debug_log("use_aencqueue : %d", handle->use_aencqueue);
	}

	return ret;
}

static int
_mm_encodebin_set_property(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), PROFILE)) {
		g_object_set(G_OBJECT(handle->encbin), PROFILE, handle->encodebin_profile, NULL);
		debug_log("encbin set profile");
	} else {
		debug_error("error handle->encbin set profile");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), MUX)) {
		g_object_set(G_OBJECT(handle->encbin), MUX, handle->mux, NULL);
		debug_log("[MUX] encbin set [%s: %s]", MUX, handle->mux);
	} else {
		debug_error("error [MUX] set [%s: %s]", MUX, handle->mux);
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}

static int
_mm_encodebin_set_video_property(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if(handle->has_video_stream) {
		/* Video */
		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), ACS)) {
			g_object_set(G_OBJECT(handle->encbin), ACS, 0, NULL);
			debug_log("[AUTO COLORSPACE] encbin set auto-colorspace");
		} else {
			debug_error("error [AUTO COLORSPACE] encbin set auto-colorspace");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encbin)), VENC)) {
			g_object_set(G_OBJECT(handle->encbin), VENC, handle->venc, NULL);
			debug_log("[VIDEOENCODER] encbin set [%s: %s]", VENC, handle->venc);
		} else {
			debug_error("error [VIDEOENCODER] set [%s: %s]", VENC, handle->venc);
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_get(G_OBJECT(handle->encbin), "use-venc-queue", &(handle->use_vencqueue), NULL);
		debug_log("vencqueue : %d", handle->use_vencqueue);
	}

	return ret;
}

int
_mm_filesink_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->filesink = gst_element_factory_make ("filesink", "filesink");
	debug_log("[sync]");
	g_object_set (G_OBJECT (handle->filesink), "sync", TRUE, NULL);
	g_object_set (G_OBJECT (handle->filesink), "async", FALSE, NULL);

	if (!handle->filesink) {
		debug_error("filesink element could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}

int
_mm_filesink_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	/* Add encodesinkbin to pipeline */
	gst_bin_add(GST_BIN(handle->pipeline), handle->filesink);

	/* link encodebin and filesink */
	if(!gst_element_link(handle->encbin, handle->filesink)) {
		debug_error("gst_element_link [encbin ! filesink] failed");
		return MM_ERROR_TRANSCODE_INTERNAL;
	} else {
		debug_log("gst_element_link [encbin ! filesink]");
	}

	return ret;
}

static int
_mm_filesrc_pipeline_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	handle->filesrc = gst_element_factory_make ("filesrc", "source");

	if (!handle->filesrc) {
		debug_error("filesrc element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	debug_log("sourcefile: %s", handle->sourcefile);
	g_object_set (G_OBJECT (handle->filesrc), "location", handle->sourcefile, NULL);

	return ret;
}

static int
_mm_filesrc_decodebin_link(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	/* Add element(filesrc, decodebin)*/
	gst_bin_add_many(GST_BIN(handle->pipeline), handle->filesrc, handle->decbin, NULL);

	if(!gst_element_link_many(handle->filesrc, handle->decbin, NULL)) {
		debug_error("gst_element_link_many src ! decbin failed");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}

int
_mm_transcode_preset_capsfilter(handle_s *handle, unsigned int resolution_width, unsigned int resolution_height)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (handle->vidflt) {
		debug_log("[Resolution] Output Width: [%d], Output Height: [%d]", resolution_width, resolution_height);
		g_object_set (G_OBJECT (handle->vidflt), "caps", gst_caps_new_simple("video/x-raw-yuv",
								"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC ('I', '4', '2', '0'),
								"width", G_TYPE_INT, resolution_width,
								"height", G_TYPE_INT, resolution_height,
								NULL), NULL);
	};

	return ret;
}

int
_mm_setup_pipeline(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	handle->pipeline= gst_pipeline_new ("TransCode");
	debug_log("Success - pipeline");

	if (!handle->pipeline) {
		debug_error("pipeline could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	return ret;
}
