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

#ifndef __MM_TRANSCODE_INTERNAL_H__
#define __MM_TRANSCODE_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mm_transcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mm_log.h"
#include <mm_debug.h>
#include <mm_error.h>
#include <mm_debug.h>
#include <mm_attrs.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/gstpipeline.h>
#include <gst/gstbuffer.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>
#include <gst/app/gstappsink.h>
#include <glib-2.0/glib/gprintf.h>
#include <gstreamer-0.10/gst/gstelement.h>
#include <gst/check/gstcheck.h>
#include <gst/pbutils/pbutils.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/pbutils/pbutils-enumtypes.h>
#include <gst/check/gstcheck.h>
#include <glibconfig.h>
#include <pthread.h>
#include <mm_file.h>
#include <gmodule.h>
#include <mm_ta.h>
#define BUFFER_SIZE 4096
#define ENC_BUFFER_SIZE 25

#define TRANSCODE_FREE(src) { if(src) {g_free(src); src = NULL;} }
#define AAC "aac"
#define ACON "auto-audio-convert"
#define ACS "auto-colorspace"
#define ACAPS "acaps"
#define AENC "aenc-name"
#define AMR "amr"
#define AMRENC "amrnbenc"
#define ARS "auto-audio-resample"
#define ASF "asfdemux"
#define FFENCMPEG4 "ffenc_mpeg4"
#define FFENCH263 "ffenc_h263p"
#define MUX "mux-name"
#define MUXAAC "ffmux_adts"
#define MUX3GP "ffmux_3gp"
#define MUXGPP "gppmux"
#define MUXMP4 "ffmux_mp4"
#define AACENC "ffenc_aac"
#define PROFILE "profile"
#define VENC "venc-name"
#define MIN_DURATION 1000
#define LAZY_PAUSE_TIMEOUT_MSEC 500

/*SQCIF */
#define VIDEO_RESOLUTION_WIDTH_SQCIF 128
#define VIDEO_RESOLUTION_HEIGHT_SQCIF 96

typedef enum {
	GST_AUTOPLUG_SELECT_TRY,
	GST_AUTOPLUG_SELECT_EXPOSE,
	GST_AUTOPLUG_SELECT_SKIP
} GstAutoplugSelectResult;

typedef struct _handle_param_s
{
	unsigned int resolution_width;
	unsigned int resolution_height;
	unsigned int fps_value;
	unsigned long start_pos;
	unsigned long duration;
	mm_seek_mode_e seek_mode;
	char outputfile[BUFFER_SIZE];
	gboolean seeking;
	gboolean async_done;
	gboolean segment_done;
	gboolean completed;
	unsigned int printed;
} handle_param_s;

typedef struct _handle_vidp_plugin_s
{
	/* decoder video processing pipeline */
	GstElement *decvideobin;
	GstElement *decsinkvideoqueue;
	GstPad *decvideosinkpad;
	GstPad *decvideosrcpad;
	GstElement *vidflt;
	GstElement *videoscale;
	GstElement *videorate;
	GstPad *videopad;
	GstPad *sinkdecvideopad;
	GstPad *srcdecvideopad;
} handle_vidp_plugin_s;

typedef struct _handle_audp_plugin_s
{
	/* decoder audio processing pipeline */
	GstElement *decaudiobin;
	GstElement *decsinkaudioqueue;
	GstPad *decaudiosinkpad;
	GstPad *decaudiosrcpad;
	GstElement *valve;
	GstElement *aconv;
	GstElement *resample;
	GstElement *audflt;
	GstElement *audiofakesink;
	GstPad *sinkdecaudiopad;
	GstPad *srcdecaudiopad;
	GstPad *decaudiopad;
	GstPad *decvideopad;
} handle_audp_plugin_s;

typedef struct _handle_encode_s
{
	/* encode pipeline */
	GstElement *encbin;
	GstPad *encaudiopad;
	GstPad *encvideopad;
	GstElement *ffmux;
	int use_vencqueue;
	int use_aencqueue;
	GstElement *encodepad;
	GstElement *encodevideo;
	int encodebin_profile;
} handle_encode_s;

typedef struct _handle_property_s
{
	/* pipeline property */
	guint bus_watcher;
	int AUDFLAG;
	int VIDFLAG;

	char *audiodecodename;
	char *videodecodename;
	gchar *venc;
	gchar *aenc;
	gchar *mux;
	gulong video_cb_probe_id;
	gulong audio_cb_probe_id;
	guint progrss_event_id;
	const gchar *mime;
	guint32 fourcc;
	gint in_width;
	gint in_height;
	gint width;
	gint height;
	gint fps_n;
	gint fps_d;
	gint aspect_x;
	gint aspect_y;
	unsigned int seek_count;
	unsigned long end_pos;
	unsigned long current_pos;
	unsigned long real_duration;
	unsigned long total_length;
	char sourcefile[BUFFER_SIZE];
	unsigned int _MMHandle;

	mm_containerformat_e containerformat;
	mm_videoencoder_e videoencoder;
	mm_audioencoder_e audioencoder;

	GstCaps *caps;
	GList *sink_elements;
	gboolean linked_vidoutbin;
	gboolean linked_audoutbin;
	gboolean has_video_stream;
	gboolean has_audio_stream;

	/* message callback */
	mm_transcode_completed_callback completed_cb;
	void * completed_cb_param;
	mm_transcode_progress_callback progress_cb;
	void * progress_cb_param;

	/* If transcode is busy */
	bool is_busy;

	/* repeat thread lock */
	GCond* thread_cond;
	GMutex *thread_mutex;
	GMutex *thread_exit_mutex;
	GThread* thread;
	gboolean repeat_thread_exit;
	GAsyncQueue *queue;
	int seek_idx;
} handle_property_s;

typedef struct _handle_s
{
	/* Transcode Pipeline Element*/
	GstElement *pipeline;
	GstElement *filesrc;
	GstElement *decodebin;
	handle_vidp_plugin_s *decoder_vidp;
	handle_audp_plugin_s *decoder_audp;
	handle_encode_s *encodebin;
	GstElement *filesink;

	/* seek paramerter */
	handle_param_s *param;

	/* pipeline property */
	handle_property_s *property;
} handle_s;

gboolean _mm_cb_audio_output_stream_probe(GstPad *pad, GstBuffer *buffer, gpointer user_data);
GstAutoplugSelectResult _mm_cb_decode_bin_autoplug_select(GstElement * element, GstPad * pad, GstCaps * caps, GstElementFactory * factory, handle_s *handle);
void _mm_cb_decoder_newpad_encoder(GstElement *decodebin, GstPad *pad, gboolean last, handle_s *handle);
gboolean _mm_cb_video_output_stream_probe(GstPad *pad, GstBuffer *buffer, gpointer user_data);
gboolean _mm_cb_print_position(handle_s *handle);
gboolean _mm_cb_transcode_bus(GstBus * bus, GstMessage * message, gpointer userdata);
const gchar* _mm_check_media_type(GstCaps *caps);
int _mm_cleanup_encodebin(handle_s *handle);
int _mm_cleanup_pipeline(handle_s *handle);
int _mm_decodesrcbin_create(handle_s *handle);
int _mm_decodesrcbin_link(handle_s *handle);
int _mm_encodebin_create(handle_s *handle);
int _mm_encodebin_link(handle_s *handle);
int _mm_encodebin_set_venc_aenc(handle_s *handle);
int _mm_filesink_create(handle_s *handle);
int _mm_filesink_link(handle_s *handle);
int _mm_setup_pipeline(handle_s *handle);
int _mm_transcode_create(handle_s *handle);
int _mm_transcode_get_stream_info(handle_s *handle);
int _mm_transcode_link(handle_s *handle);
int _mm_transcode_param_flush(handle_s *handle);
int _mm_transcode_preset_capsfilter(handle_s *handle, unsigned int resolution_width, unsigned int resolution_height);
int _mm_transcode_state_change(handle_s *handle, GstState gst_state);
int _mm_transcode_set_handle_element(handle_s *handle, const char * in_Filename, mm_containerformat_e containerformat,
	mm_videoencoder_e videoencoder, mm_audioencoder_e audioencoder);
int _mm_transcode_set_handle_parameter(handle_param_s *param, unsigned int resolution_width, unsigned int resolution_height, unsigned int fps_value, unsigned long start_pos,
	unsigned long duration, mm_seek_mode_e seek_mode, const char *out_Filename);
int _mm_transcode_get_stream_info(handle_s *handle);
int _mm_transcode_thread(handle_s *handle);


#ifdef __cplusplus
}
#endif

#endif	/*__MM_TRANSCODE_INTERNAL_H__*/
