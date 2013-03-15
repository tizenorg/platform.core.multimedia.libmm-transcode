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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->encodebin->encvideopad) {
		gst_object_unref (GST_OBJECT(handle->encodebin->encvideopad));
		handle->encodebin->encvideopad = NULL;
		debug_log("Success - gst_object_unref (encvideopad)");
	}

	if(handle->encodebin->encaudiopad) {
		gst_object_unref (GST_OBJECT(handle->encodebin->encaudiopad));
		handle->encodebin->encaudiopad = NULL;
		debug_log("Success - gst_object_unref (encaudiopad)");
	}

	if(handle->property->caps) {
		gst_caps_unref (handle->property->caps);
		handle->property->caps = NULL;
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

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->property->_MMHandle = 0;

	/* g_thread_exit(handle->thread); */
	debug_log("g_thread_exit");
	if(handle->property->thread) {
		g_thread_join(handle->property->thread);
		debug_log("Success - join (thread)");
	}

	/* disconnecting bus watch */
	if ( handle->property->bus_watcher ) {
		g_source_remove(handle->property->bus_watcher);
		debug_log("g_source_remove");
		handle->property->bus_watcher = 0;
	}

	if(handle->property->queue) {
		g_async_queue_unref(handle->property->queue);
		handle->property->queue = NULL;
		debug_log("Success - g_async_queue_unref(queue)");
	}

	if(handle->property->thread_mutex) {
		g_mutex_free (handle->property->thread_mutex);
		handle->property->thread_mutex = NULL;
		debug_log("Success - free (thread_mutex)");
	}

	if(handle->property->thread_cond) {
		g_cond_free (handle->property->thread_cond);
		handle->property->thread_cond = NULL;
		debug_log("Success - free (thread_cond)");
	}

	if(handle->property->thread_exit_mutex) {
		g_mutex_free (handle->property->thread_exit_mutex);
		handle->property->thread_exit_mutex = NULL;
		debug_log("Success - free (thread_exit_mutex)");
	}

	/* release videopad */
	if(handle->decoder_vidp->decvideosinkpad) {
		gst_object_unref(GST_OBJECT(handle->decoder_vidp->decvideosinkpad));
		handle->decoder_vidp->decvideosinkpad = NULL;
		debug_log("Success - gt_object_unref (decvideosinkpad)");
	}

	if(handle->decoder_vidp->decvideosrcpad) {
		gst_object_unref(GST_OBJECT(handle->decoder_vidp->decvideosrcpad));
		handle->decoder_vidp->decvideosrcpad = NULL;
		debug_log("Success - gst_object_unref (decvideosrcpad)");
	}

	if(handle->decoder_vidp->srcdecvideopad) {
		gst_object_unref (GST_OBJECT(handle->decoder_vidp->srcdecvideopad));
		handle->decoder_vidp->srcdecvideopad = NULL;
		debug_log("Success - gst_object_unref (srcdecvideopad)");
	}

	if(handle->decoder_vidp->sinkdecvideopad) {
		gst_object_unref (GST_OBJECT(handle->decoder_vidp->sinkdecvideopad));
		handle->decoder_vidp->sinkdecvideopad = NULL;
		debug_log("Success - gst_object_unref (sinkdecvideopad)");
	}

	/* release audiopad */
	if(handle->decoder_audp->decaudiosinkpad) {
		gst_object_unref (GST_OBJECT(handle->decoder_audp->decaudiosinkpad));
		handle->decoder_audp->decaudiosinkpad = NULL;
		debug_log("Success - gst_object_unref (decaudiosinkpad)");
	}

	if(handle->decoder_audp->decaudiosrcpad) {
		gst_object_unref (GST_OBJECT(handle->decoder_audp->decaudiosrcpad));
		handle->decoder_audp->decaudiosrcpad=NULL;
		debug_log("Success - gst_object_unref (decaudiosrcpad)");
	}

	if(handle->decoder_audp->srcdecaudiopad) {
		gst_object_unref (GST_OBJECT(handle->decoder_audp->srcdecaudiopad));
		handle->decoder_audp->srcdecaudiopad = NULL;
		debug_log("Success - gst_object_unref (srcdecaudiopad)");
	}

	if(handle->decoder_audp->sinkdecaudiopad) {
		gst_object_unref (GST_OBJECT(handle->decoder_audp->sinkdecaudiopad));
		handle->decoder_audp->sinkdecaudiopad = NULL;
		debug_log("Success - gst_object_unref (sinkdecaudiopad)");
	}

	if(_mm_cleanup_encodebin(handle) != MM_ERROR_NONE) {
		debug_error("ERROR -Play Pipeline");
		return FALSE;
	} else {
		debug_log("Success -clean encodebin");
	}

	if(handle->property->sink_elements) {
		g_list_free (handle->property->sink_elements);
		handle->property->sink_elements = NULL;
		debug_log("Success - g_list_free (sink_elements)");
	}

	if(handle->pipeline) {
		gst_object_unref (handle->pipeline);
		handle->pipeline = NULL;
		debug_log("Success - gst_object_unref (pipeline)");
	}

	if(handle->property->audio_cb_probe_id) {
		g_source_remove (handle->property->audio_cb_probe_id);
		handle->property->audio_cb_probe_id = 0;
		debug_log("g_source_remove (audio_cb_probe_id)");
	}

	if(handle->property->video_cb_probe_id) {
		g_source_remove (handle->property->video_cb_probe_id);
		handle->property->video_cb_probe_id = 0;
		debug_log("g_source_remove (video_cb_probe_id)");
	}

	if(handle->property->progrss_event_id) {
		g_source_remove (handle->property->progrss_event_id);
		handle->property->progrss_event_id = 0;
		debug_log("g_source_remove (progrss_event_id)");
	}

	TRANSCODE_FREE (handle->property->mux);
	TRANSCODE_FREE (handle->property->venc);
	TRANSCODE_FREE (handle->property->aenc);
	TRANSCODE_FREE (handle->decoder_vidp);
	TRANSCODE_FREE (handle->decoder_audp);
	TRANSCODE_FREE (handle->encodebin);
	TRANSCODE_FREE (handle->property);
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

	if (!handle->decoder_audp) {
		debug_error("[ERROR] - handle decoder audio process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_audp->decaudiobin = gst_bin_new ("audiobin");
	if (!handle->decoder_audp->decaudiobin) {
		debug_error("decaudiobin could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_audp->decsinkaudioqueue = gst_element_factory_make("queue","decsinkaudioqueue");
	if (!handle->decoder_audp->decsinkaudioqueue) {
		debug_error("decsinkaudioqueue could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_audp->decaudiosinkpad = gst_element_get_static_pad (handle->decoder_audp->decsinkaudioqueue, "sink");
	if (!handle->decoder_audp->decaudiosinkpad) {
		debug_error("decaudiosinkpad could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->audioencoder == MM_AUDIOENCODER_NO_USE) {
		debug_log("[MM_AUDIOENCODER_NO_USE] fakesink create");

		handle->decoder_audp->audiofakesink = gst_element_factory_make ("fakesink", "audiofakesink");
		if (!handle->decoder_audp->audiofakesink) {
			debug_error("fakesink element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}
	} else {
		handle->decoder_audp->aconv = gst_element_factory_make ("audioconvert", "aconv");
		if (!handle->decoder_audp->aconv) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->decoder_audp->valve = gst_element_factory_make ("valve", "valve");
		if (!handle->decoder_audp->valve) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_set(handle->decoder_audp->valve,"drop", FALSE, NULL);

		handle->decoder_audp->resample = gst_element_factory_make ("audioresample", "audioresample");
		if (!handle->decoder_audp->resample) {
			debug_error("decaudiobin element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->decoder_audp->audflt = gst_element_factory_make ("capsfilter", "afilter");
		if (!handle->decoder_audp->audflt) {
			debug_error("audflt element could not be created");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		handle->decoder_audp->decaudiosrcpad = gst_element_get_static_pad (handle->decoder_audp->audflt, "src");
		if (!handle->decoder_audp->decaudiosrcpad) {
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

	if (!handle->decoder_audp) {
		debug_error("[ERROR] - handle decoder audio process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->audioencoder == MM_AUDIOENCODER_NO_USE) {
		debug_log("[MM_AUDIOENCODER_NO_USE] fakesink pad create");

		gst_bin_add_many (GST_BIN (handle->decoder_audp->decaudiobin), handle->decoder_audp->decsinkaudioqueue, handle->decoder_audp->audiofakesink, NULL);
		if(!gst_element_link_many(handle->decoder_audp->decsinkaudioqueue, handle->decoder_audp->audiofakesink, NULL)) {
			debug_error("[Audio Output Bin] gst_element_link_many failed");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		gst_element_add_pad (handle->decoder_audp->decaudiobin, gst_ghost_pad_new("decbin_audiosink", handle->decoder_audp->decaudiosinkpad));
		handle->decoder_audp->sinkdecaudiopad = gst_element_get_static_pad (handle->decoder_audp->decaudiobin, "decbin_audiosink"); /* get sink audiopad of decodebin */
	} else {
		gst_bin_add_many (GST_BIN (handle->decoder_audp->decaudiobin), handle->decoder_audp->decsinkaudioqueue, handle->decoder_audp->valve, handle->decoder_audp->aconv, handle->decoder_audp->resample, handle->decoder_audp->audflt, NULL);
		if(!gst_element_link_many(handle->decoder_audp->decsinkaudioqueue, handle->decoder_audp->valve, handle->decoder_audp->aconv, handle->decoder_audp->resample, handle->decoder_audp->audflt, NULL)) {
			debug_error("[Audio Output Bin] gst_element_link_many failed");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		gst_element_add_pad (handle->decoder_audp->decaudiobin, gst_ghost_pad_new("decbin_audiosink", handle->decoder_audp->decaudiosinkpad));
		gst_element_add_pad (handle->decoder_audp->decaudiobin, gst_ghost_pad_new("decbin_audiosrc", handle->decoder_audp->decaudiosrcpad));

		handle->decoder_audp->sinkdecaudiopad = gst_element_get_static_pad (handle->decoder_audp->decaudiobin, "decbin_audiosink"); /* get sink audiopad of decodebin */
		handle->decoder_audp->srcdecaudiopad = gst_element_get_static_pad (handle->decoder_audp->decaudiobin, "decbin_audiosrc"); /* get src audiopad of decodebin */

		handle->property->audio_cb_probe_id = gst_pad_add_buffer_probe (handle->decoder_audp->sinkdecaudiopad, G_CALLBACK (_mm_cb_audio_output_stream_probe), handle);
		debug_log("audio_cb_probe_id: %d", handle->property->audio_cb_probe_id); /* must use sinkpad (sinkpad => srcpad) for normal resized video buffer*/
	}

	gst_bin_add (GST_BIN (handle->pipeline), handle->decoder_audp->decaudiobin);
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

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp->decvideobin= gst_bin_new("videobin");
	if (!handle->decoder_vidp->decvideobin) {
		debug_error("decvideobin could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp->decsinkvideoqueue = gst_element_factory_make("queue","decsinkvideoqueue");
	if (!handle->decoder_vidp->decsinkvideoqueue) {
		debug_error("decsinkvideoqueue element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp->decvideosinkpad = gst_element_get_static_pad(handle->decoder_vidp->decsinkvideoqueue,"sink");
	if (!handle->decoder_vidp->decvideosinkpad) {
		debug_error("decvideosinkpad element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp->videorate = gst_element_factory_make("videorate", "videorate");
	if (!handle->decoder_vidp->videorate) {
		debug_error("videorate element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	g_object_set (handle->decoder_vidp->videorate, "drop-only", TRUE,"average-period", GST_SECOND/2, NULL);
	g_object_set (handle->decoder_vidp->videorate, "max-rate", 30, NULL);

	handle->decoder_vidp->videoscale = gst_element_factory_make("videoscale", "scaler");
	if (!handle->decoder_vidp->videoscale) {
		debug_error("videoscale element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}
	/* Configure videoscale to use 4-tap scaling for higher quality */
	g_object_set (handle->decoder_vidp->videoscale, "method", 2, NULL);

	handle->decoder_vidp->vidflt = gst_element_factory_make("capsfilter", "vfilter");
	if (!handle->decoder_vidp->vidflt) {
		debug_error("One element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->decoder_vidp->decvideosrcpad = gst_element_get_static_pad(handle->decoder_vidp->vidflt,"src");
	if (!handle->decoder_vidp->decvideosrcpad) {
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

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	gst_bin_add_many(GST_BIN(handle->decoder_vidp->decvideobin), handle->decoder_vidp->decsinkvideoqueue, handle->decoder_vidp->videoscale,  handle->decoder_vidp->videorate, handle->decoder_vidp->vidflt, NULL);
	if(!gst_element_link_many(handle->decoder_vidp->decsinkvideoqueue, handle->decoder_vidp->videoscale, handle->decoder_vidp->videorate, handle->decoder_vidp->vidflt, NULL)) {
		debug_error("[Video Output Bin] gst_element_link_many failed");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	gst_element_add_pad(handle->decoder_vidp->decvideobin, gst_ghost_pad_new("decbin_videosink", handle->decoder_vidp->decvideosinkpad));
	gst_element_add_pad(handle->decoder_vidp->decvideobin, gst_ghost_pad_new("decbin_videosrc", handle->decoder_vidp->decvideosrcpad));

	handle->decoder_vidp->sinkdecvideopad = gst_element_get_static_pad(handle->decoder_vidp->decvideobin,"decbin_videosink");
	handle->decoder_vidp->srcdecvideopad = gst_element_get_static_pad(handle->decoder_vidp->decvideobin,"decbin_videosrc");

	handle->property->video_cb_probe_id = gst_pad_add_buffer_probe (handle->decoder_vidp->sinkdecvideopad, G_CALLBACK (_mm_cb_video_output_stream_probe), handle);
	debug_log("video_cb_probe_sink_id: %d", handle->property->video_cb_probe_id); /* must use sinkpad (sinkpad => srcpad) */

	gst_bin_add(GST_BIN(handle->pipeline), handle->decoder_vidp->decvideobin);

	return ret;
}

static int
_mm_decodebin_pipeline_create(handle_s *handle)
{
	int ret = MM_ERROR_NONE;

	handle->decodebin = gst_element_factory_make ("decodebin2", "decoder"); /* autoplug-select is not worked when decodebin */

	if (!handle->decodebin) {
		debug_error("decbin element could not be created. Exiting");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	g_signal_connect (handle->decodebin, "new-decoded-pad",G_CALLBACK(_mm_cb_decoder_newpad_encoder), handle);
	g_signal_connect (handle->decodebin, "autoplug-select", G_CALLBACK(_mm_cb_decode_bin_autoplug_select), handle);

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

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->decoder_audp) {
		debug_error("[ERROR] - handle decoder audio process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	ret = _mm_filesrc_pipeline_create(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create filesrc pipeline");
	} else{
		debug_error("ERROR -Create filesrc pipeline");
		return ret;
	}

	if(handle->property->has_audio_stream) {
		ret = _mm_decode_audio_output_create(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Create audiobin pipeline: 0x%2x",handle->decoder_audp->decaudiobin);
		} else{
			debug_error("ERROR - Create audiobin pipeline");
			return ret;
		}
	}

	if(handle->property->has_video_stream) {
		ret = _mm_decode_video_output_create(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Create videobin pipeline: 0x%2x",handle->decoder_vidp->decvideobin);
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

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	ret=_mm_filesrc_decodebin_link(handle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - _mm_filesrc_decodebin_link");
	} else{
		debug_error("ERROR - _mm_filesrc_decodebin_link");
		return ret;
	}

	if(handle->property->has_audio_stream) {
		ret = _mm_decode_audio_output_link(handle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - _mm_decode_audio_output_link");
		} else{
			debug_error("ERROR - _mm_decode_audio_output_link");
			return ret;
		}
	}

	if(handle->property->has_video_stream) {
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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	handle->encodebin->encbin = gst_element_factory_make ("encodebin", "encodebin");

	if (!handle->encodebin->encbin ) {
		debug_error("encbin element could not be created");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->videoencoder != MM_VIDEOENCODER_NO_USE && handle->property->audioencoder != MM_AUDIOENCODER_NO_USE) {
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
	} else if(handle->property->videoencoder != MM_VIDEOENCODER_NO_USE) {
		ret = _mm_encodebin_set_video_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin video property");
			return ret;
		}
	} else if(handle->property->audioencoder != MM_AUDIOENCODER_NO_USE) {
		handle->encodebin->encodebin_profile = 1;
		debug_log("[AUDIO ONLY ENCODE]");
		ret = _mm_encodebin_set_audio_property(handle);
		if(ret != MM_ERROR_NONE) {
			debug_error("ERROR -Setup encodebin video property");
			return ret;
		}
	}

	debug_log("[Encodebin Profile: %d]", handle->encodebin->encodebin_profile);
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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	/* Add encodebin to pipeline */
	gst_bin_add(GST_BIN(handle->pipeline), handle->encodebin->encbin);

	if(handle->property->has_video_stream) {
		if(handle->property->videoencoder != MM_VIDEOENCODER_NO_USE) {
			handle->encodebin->encvideopad = gst_element_get_request_pad(handle->encodebin->encbin, "video");
			if(handle->encodebin->encvideopad) {
				debug_log("encvideopad: 0x%2x", handle->encodebin->encvideopad);
			} else {
				debug_error("error encvideopad");
				return MM_ERROR_TRANSCODE_INTERNAL;
			}
		}
	}

	if(handle->property->has_audio_stream) {
		if(handle->property->audioencoder != MM_AUDIOENCODER_NO_USE) {
			handle->encodebin->encaudiopad = gst_element_get_request_pad(handle->encodebin->encbin, "audio");
			if(handle->encodebin->encaudiopad) {
				debug_log("encaudiopad: 0x%2x", handle->encodebin->encaudiopad);
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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->has_audio_stream) {
		/* Audio */
		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), ARS)) {
			g_object_set(G_OBJECT(handle->encodebin->encbin), ARS, 0, NULL);
			debug_log("[AUDIO RESAMPLE] encbin set auto-audio-resample");
		} else {
			debug_error("error [AUDIO RESAMPLE] encbin set auto-audio-resample");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), ACON)) {
			g_object_set(G_OBJECT(handle->encodebin->encbin), ACON, 1, NULL);
			debug_log("encbin set auto-audio-convert");
		} else {
			debug_error("error encbin set auto-audio-convert");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), AENC)) {
			g_object_set(G_OBJECT(handle->encodebin->encbin), AENC, handle->property->aenc, NULL);
			debug_log("[AUDIOENCODER] encbin set [%s: %s]",AENC, handle->property->aenc);
		} else {
			debug_error("error [AUDIOENCODER] encbin set [%s: %s]",AENC, handle->property->aenc);
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_get(G_OBJECT(handle->encodebin->encbin), "use-aenc-queue", &(handle->encodebin->use_aencqueue), NULL);
		debug_log("use_aencqueue : %d", handle->encodebin->use_aencqueue);
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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), PROFILE)) {
		g_object_set(G_OBJECT(handle->encodebin->encbin), PROFILE, handle->encodebin->encodebin_profile, NULL);
		debug_log("encbin set profile");
	} else {
		debug_error("error handle->encbin set profile");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), MUX)) {
		g_object_set(G_OBJECT(handle->encodebin->encbin), MUX, handle->property->mux, NULL);
		debug_log("[MUX] encbin set [%s: %s]", MUX, handle->property->mux);
	} else {
		debug_error("error [MUX] set [%s: %s]", MUX, handle->property->mux);
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

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if(handle->property->has_video_stream) {
		/* Video */
		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), ACS)) {
			g_object_set(G_OBJECT(handle->encodebin->encbin), ACS, 0, NULL);
			debug_log("[AUTO COLORSPACE] encbin set auto-colorspace");
		} else {
			debug_error("error [AUTO COLORSPACE] encbin set auto-colorspace");
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		if(g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(handle->encodebin->encbin)), VENC)) {
			g_object_set(G_OBJECT(handle->encodebin->encbin), VENC, handle->property->venc, NULL);
			debug_log("[VIDEOENCODER] encbin set [%s: %s]", VENC, handle->property->venc);
		} else {
			debug_error("error [VIDEOENCODER] set [%s: %s]", VENC, handle->property->venc);
			return MM_ERROR_TRANSCODE_INTERNAL;
		}

		g_object_get(G_OBJECT(handle->encodebin->encbin), "use-venc-queue", &(handle->encodebin->use_vencqueue), NULL);
		debug_log("vencqueue : %d", handle->encodebin->use_vencqueue);
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

	if (!handle) {
		debug_error("[ERROR] - handle");
		return MM_ERROR_INVALID_ARGUMENT;
	}

	if (!handle->encodebin) {
		debug_error("[ERROR] - handle encodebin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	/* Add encodesinkbin to pipeline */
	gst_bin_add(GST_BIN(handle->pipeline), handle->filesink);

	/* link encodebin and filesink */
	if(!gst_element_link(handle->encodebin->encbin, handle->filesink)) {
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

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	debug_log("sourcefile: %s", handle->property->sourcefile);
	g_object_set (G_OBJECT (handle->filesrc), "location", handle->property->sourcefile, NULL);

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
	gst_bin_add_many(GST_BIN(handle->pipeline), handle->filesrc, handle->decodebin, NULL);

	if(!gst_element_link_many(handle->filesrc, handle->decodebin, NULL)) {
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

	if (!handle->decoder_vidp) {
		debug_error("[ERROR] - handle decoder video process bin");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (!handle->property) {
		debug_error("[ERROR] - handle property");
		return MM_ERROR_TRANSCODE_INTERNAL;
	}

	if (handle->decoder_vidp->vidflt) {
		debug_log("[Resolution] Output Width: [%d], Output Height: [%d]", resolution_width, resolution_height);
		g_object_set (G_OBJECT (handle->decoder_vidp->vidflt), "caps", gst_caps_new_simple("video/x-raw-yuv",
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
