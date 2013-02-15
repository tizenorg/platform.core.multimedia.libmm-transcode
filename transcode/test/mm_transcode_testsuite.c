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

#include "glib.h"
#include <pthread.h>
#include "mm_transcode.h"
#include "mm_transcode_internal.h"
#include "mm_log.h"
#include "mm_debug.h"
#include <mm_ta.h>
#include <unistd.h>
#include <signal.h>
#define _one_by_one 1
#define _cancel 1
MMHandleType MMHandle = 0;
GMainLoop *loop;
GstBus *bus;
static int seek_call_cnt = 0;
static int seek_cnt = 0;

bool
transcode_completed_cb(int error, void *user_param)
{
	debug_log("MMHandle: 0x%2x", MMHandle);

	if(error == MM_ERROR_NONE) {
		debug_log("completed");
		#if _one_by_one
		g_main_loop_quit (loop);
		#endif
	}
	return TRUE;
}

bool
transcode_progress_cb(unsigned long  current_position, unsigned long  real_duration, void *user_parm)
{
	debug_log("(%d msec) / (%d msec)", current_position, real_duration);

	return TRUE;
}

void
trap(int sig)
{
	debug_log("quit thread");
	signal(SIGINT, SIG_DFL);
	#if _cancel
	if(mm_transcode_cancel (MMHandle) == MM_ERROR_NONE) {
		debug_log("[Success Cancel]");
	}
	mm_transcode (MMHandle, 176, 144, 0, 0, 5000, 1,"/opt/usr/media/Videos/aftercancel.3gp", transcode_progress_cb, transcode_completed_cb, NULL);
	#endif
}

int main(int argc, char *argv[])
{
	int ret = MM_ERROR_NONE;
	void *user_param = NULL;
	bool is_busy = FALSE;

	mm_containerformat_e containerformat = 0;
	mm_videoencoder_e videoencoder = 0 ;
	mm_audioencoder_e audioencoder = 0;
	unsigned int resolution_width = 0;
	unsigned int resolution_height = 0;
	mm_seek_mode_e seek_mode = 0;
	unsigned long start_pos;
	unsigned long duration = 0;
	unsigned int fps_value = 0;

	debug_log("Success - Loop Init");
	loop = g_main_loop_new (NULL, FALSE);

	/* Create Transcode */
	ret = mm_transcode_create (&MMHandle);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Create Transcode Handle");
		debug_log("MMHandle: 0x%2x", MMHandle);
	}else{
		debug_log("ERROR - Create Transcode Handle");
		return ret;
	}

	signal(SIGINT, trap);

	containerformat = atoi(argv[3]);
	videoencoder = atoi(argv[4]);
	audioencoder = atoi(argv[5]);

	/* Transcode */
	ret = mm_transcode_prepare (MMHandle, argv[1], containerformat, videoencoder, audioencoder);
	if(ret == MM_ERROR_NONE) {
		debug_log("Success - Transcode pipeline");
	}else{
		debug_error("ERROR - Transcode pipeline");
		return ret;
	}

	seek_call_cnt = atoi(argv[2]);
	for(seek_cnt=0; seek_cnt<seek_call_cnt; seek_cnt++) {
		resolution_width = atoi(argv[6+(seek_cnt* 7)]);
		resolution_height = atoi(argv[7+(seek_cnt* 7)]);
		fps_value = atoi(argv[8+(seek_cnt* 7)]);
		start_pos = atoi(argv[9+(seek_cnt* 7)]);
		duration = atoi(argv[10+(seek_cnt* 7)]);
		seek_mode = atoi(argv[11+(seek_cnt* 7)]);

		debug_log("mm_transcode_testsuite [Input Filename] [Seek Count] [Container Format] [VENC] [AENC] [Resolution Width] [Resolution Height] [fps] [Start_position] [Duration] [Seek Accurate Mode T/F] [Output Filename]\n");
		debug_log("container format: %d videoencoder: %d  audioencoder: %d resolution_height: %d, resolution_height: %d, start_pos: %d, duration: %d, seek_mode: %d\n",
		containerformat, videoencoder, audioencoder, resolution_width, resolution_height, start_pos, duration, seek_mode);

		/* Transcode */
		if(mm_transcode_is_busy(MMHandle, &is_busy) == MM_ERROR_NONE) {
			if(is_busy == FALSE) {
				ret = mm_transcode (MMHandle, resolution_width, resolution_height, fps_value, start_pos, duration, seek_mode, argv[12+(seek_cnt* 7)], transcode_progress_cb, transcode_completed_cb, user_param);
			} else {
				debug_log("BUSY");
			}
		}
		#if _one_by_one
		g_main_loop_run (loop);
	}
		#else
	}
	g_main_loop_run (loop);
	#endif

	if(ret == MM_ERROR_NONE) {
		/* cleanup */
		mm_transcode_destroy (MMHandle);
		if(ret == MM_ERROR_NONE) {
			debug_log("Success - Transcode attribute");
		}else{
			debug_error("ERROR - Transcode attribute");
			return ret;
		}
	}else{
		debug_error("ERROR - Transcode pipeline");
		return ret;
	}

	return ret;
}

