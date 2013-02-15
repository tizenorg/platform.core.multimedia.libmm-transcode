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

#ifndef __MM_TRANSCODE_H__
#define __MM_TRANSCODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mm_types.h>
typedef bool (*mm_transcode_completed_callback)(int error, void *user_parm);
typedef bool (*mm_transcode_progress_callback)(unsigned long current_position, unsigned long real_duration, void *user_parm);
typedef bool (*mm_transcode_support_type_callback)(int encoder_type, void *user_parm);


/**
 * Video Container Formats
 */
typedef enum
{
	/* 3gp */
	MM_CONTAINER_3GP = 0x00,
	MM_CONTAINER_MP4,
	MM_CONTAINER_NUM,
} mm_containerformat_e;

/**
 * Video Encoder
 */
typedef enum
{
	MM_VIDEOENCODER_MPEG4 = 0x00,
	MM_VIDEOENCODER_H263,
	MM_VIDEOENCODER_H264,
	MM_VIDEOENCODER_NO_USE,
	MM_VIDEOENCODER_NUM,
} mm_videoencoder_e;

/**
 * Audio Encorder
 */
typedef enum
{
	MM_AUDIOENCODER_AAC = 0x00,
	MM_AUDIOENCODER_AMR,
	MM_AUDIOENCODER_NO_USE,
	MM_AUDIOENCODER_NUM,
} mm_audioencoder_e;

/**
 * Video Seek Mode
 */
typedef enum
{
	/* 3GP */
	MM_SEEK_INACCURATE = 0x00,
	MM_SEEK_ACCURATE,
	MM_SEEK_NUM,
} mm_seek_mode_e;


/**
 *
 * @remark 	Transcode Handle Creation
 *
 * @param	MMHandle		[in]			MMHandleType pointer

 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then handle creation succeed
 *		else if the result is -1, then handle creation failed
 */
int
mm_transcode_create(MMHandleType* MMHandle);




/**
 *
 * @remark 	Video Transcode Pipeline
 *
 * @param	MMHandle						[in]			MMHandleType
 * @param	in_Filename						[in]			Input Video File pointer
 * @param	containerformat					[in]			Container format (3GP/MP4)
 * @param	videoencoder						[in]			Videoencoder (ffenc_h263p/ffenc_mpeg4/savsenc_mp4/savsenc_h264)
 * @param	audioencoder						[in]			Audioencoder (amr/aac)

 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then you can use output_Filename pointer(char** value)
 *		else if the result is -1, then do not execute when the colopsapce converter is not supported
 */
int
mm_transcode_prepare(MMHandleType MMHandle, const char *in_Filename, mm_containerformat_e containerformat,
	mm_videoencoder_e videoencoder, mm_audioencoder_e audioencoder);




/**
 *
 * @remark 	Video Transcode Pipeline
 *
 * @param	MMHandle						[in]			MMHandleType
 * @param	resolution_width					[in]			Video Resolution Width
 * @param	resolution_height					[in]			Video Resolution Height
 * @param	fps_value							[in]			Video Framerate
 * @param	start_position						[in]			Start Position (msec)
 * @param	duration							[in]			Seek Duration (msec, If the value is 0, you transcode the original file totally)
 * @param	seek_mode						[in]			Seek Mode (Accurate / Inaccurate)
 * @param	out_Filename						[in]			Output Video File pointer
 * @param	progress_callback					[in]			Progress_callback
 * @param	completed_callback				[in]			Completed_callback
 * @param	user_param						[in]			User parameter which is received from user when callback function was set


 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then you can use output_Filename pointer(char** value)
 *		else if the result is -1, then do not execute when the colopsapce converter is not supported
 */
int
mm_transcode(MMHandleType MMHandle, unsigned int resolution_width, unsigned int resolution_height, unsigned int fps_value, unsigned long start_position, unsigned long duration,
	mm_seek_mode_e seek_mode, const char *out_Filename, mm_transcode_progress_callback progress_callback, mm_transcode_completed_callback completed_callback, void *user_param);




/**
 *
 * @remark 	Video Transcode Pipeline
 *
 * @param	MMHandle						[in]			MMHandleType
 * @param	is_busy							[in/out]		If transcode is executing or not

 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then it means that trascode is executing now
 *		else if the result is -1, then you can call mm_transcode_prepare for new input file
 */
int
mm_transcode_is_busy(MMHandleType MMHandle, bool *is_busy);




/**
 *
 * @remark 	Transcode Handle Cancel
 *
 * @param	MMHandle		[in]			MMHandleType

 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then handle destory succeed
 *		else if the result is -1, then handle destory failed
 */
int
mm_transcode_cancel(MMHandleType MMHandle);




/**
 *
 * @remark 	Transcode Handle Destory
 *
 * @param	MMHandle		[in]			MMHandleType

 * @return  	This function returns transcode processor result value
 *		if  the result is 0, then handle destory succeed
 *		else if the result is -1, then handle destory failed
 */
int
mm_transcode_destroy(MMHandleType MMHandle);




/**
 *
 * @remark      Transcode Handle Destory
 *
 * @param	MMHandle		[in]				MMHandleType
 * @param»	containerformat	[in/out]			Container format
 * @param»	videoencoder»	[in/out]			Videoencoder
 * @param»	audioencoder»	[in/out]			Audioencoder
 * @param»	current_pos		[in/out]»			Current Position
 * @param»	duration			[in/out]			Duration
 * @param»	resolution_width»	[in/out]			Real Resolution Width
 * @param»	resolution_height	[in/out]			Real Resolution Height

 * @return      This function returns transcode processor result value
 *              if  the result is 0, then transcode get attribute succeed
 *              else if the result is -1, then transcode get attribute failed
 */
int
mm_transcode_get_attrs(MMHandleType MMHandle, mm_containerformat_e *containerformat, mm_videoencoder_e *videoencoder,
	mm_audioencoder_e *audioencoder, unsigned long *current_pos, unsigned long *duration, unsigned int *resolution_width, unsigned int *resolution_height);

int
mm_transcode_get_supported_container_format(mm_transcode_support_type_callback type_callback, void *user_param);

int
mm_transcode_get_supported_video_encoder(mm_transcode_support_type_callback type_callback, void *user_param);

int
mm_transcode_get_supported_audio_encoder(mm_transcode_support_type_callback type_callback, void *user_param);


#ifdef __cplusplus__
};
#endif

#endif	/*__MM_TRANSCODE_H__*/
