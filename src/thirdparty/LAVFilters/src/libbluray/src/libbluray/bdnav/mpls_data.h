/*
 * This file is part of libbluray
 * Copyright (C) 2009-2010  John Stebbins
 * Copyright (C) 2012-2016  Petri Hintukainen <phintuka@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined(BLURAY_MPLS_DATA_H_)
#define BLURAY_MPLS_DATA_H_

#include "uo_mask_table.h"

#include <stdint.h>

#define BD_MARK_ENTRY   0x01
#define BD_MARK_LINK    0x02

typedef struct
{
    uint8_t         stream_type;
    uint8_t         coding_type;
    uint16_t        pid;
    uint8_t         subpath_id;
    uint8_t         subclip_id;
    uint8_t         format;
    uint8_t         rate;
    uint8_t         char_code;
    char            lang[4];
    // Secondary audio specific fields
    uint8_t         sa_num_primary_audio_ref;
    uint8_t        *sa_primary_audio_ref;
    // Secondary video specific fields
    uint8_t         sv_num_secondary_audio_ref;
    uint8_t         sv_num_pip_pg_ref;
    uint8_t        *sv_secondary_audio_ref;
    uint8_t        *sv_pip_pg_ref;
    uint8_t         ss_offset_sequence_id;
} MPLS_STREAM;

typedef struct
{
    uint8_t         num_video;
    uint8_t         num_audio;
    uint8_t         num_pg;
    uint8_t         num_ig;
    uint8_t         num_secondary_audio;
    uint8_t         num_secondary_video;
    uint8_t         num_pip_pg;
    uint8_t         num_dv;
    MPLS_STREAM    *video;
    MPLS_STREAM    *audio;
    MPLS_STREAM    *pg;
    MPLS_STREAM    *ig;
    MPLS_STREAM    *secondary_audio;
    MPLS_STREAM    *secondary_video;
    MPLS_STREAM    *dv;
} MPLS_STN;

typedef struct
{
    char            clip_id[6];
    char            codec_id[5];
    uint8_t         stc_id;
} MPLS_CLIP;

typedef struct
{
    uint8_t         is_multi_angle;
    uint8_t         connection_condition;
    uint32_t        in_time;
    uint32_t        out_time;
    BD_UO_MASK      uo_mask;
    uint8_t         random_access_flag;
    uint8_t         still_mode;
    uint16_t        still_time;
    uint8_t         angle_count;
    uint8_t         is_different_audio;
    uint8_t         is_seamless_angle;
    MPLS_CLIP       *clip;
    MPLS_STN        stn;
} MPLS_PI;

typedef struct
{
    uint8_t         mark_type;
    uint16_t        play_item_ref;
    uint32_t        time;
    uint16_t        entry_es_pid;
    uint32_t        duration;
} MPLS_PLM;

typedef struct
{
    uint8_t         playback_type;
    uint16_t        playback_count;
    BD_UO_MASK      uo_mask;
    uint8_t         random_access_flag;
    uint8_t         audio_mix_flag;
    uint8_t         lossless_bypass_flag;
    uint8_t         mvc_base_view_r_flag;
} MPLS_AI;

typedef struct
{
    uint8_t         connection_condition;
    uint8_t         is_multi_clip;
    uint32_t        in_time;
    uint32_t        out_time;
    uint16_t        sync_play_item_id;
    uint32_t        sync_pts;
    uint8_t         clip_count;
    MPLS_CLIP       *clip;
} MPLS_SUB_PI;

typedef enum {
  //mpls_sub_path_        = 2,  /* Primary audio of the Browsable slideshow */
  mpls_sub_path_ig_menu   = 3,  /* Interactive Graphics presentation menu */
  mpls_sub_path_textst    = 4,  /* Text Subtitle */
  //mpls_sub_path_        = 5,  /* Out-of-mux Synchronous elementary streams */
  mpls_sub_path_async_pip = 6,  /* Out-of-mux Asynchronous Picture-in-Picture presentation */
  mpls_sub_path_sync_pip  = 7,  /* In-mux Synchronous Picture-in-Picture presentation */
  mpls_sub_path_ss_viseo  = 8,  /* SS Video */
} mpls_sub_path_type;

typedef struct
{
    uint8_t         type;       /* enum mpls_sub_path_type */
    uint8_t         is_repeat;
    uint8_t         sub_playitem_count;
    MPLS_SUB_PI     *sub_play_item;
} MPLS_SUB;

typedef enum {
    pip_scaling_none = 1,       /* unscaled */
    pip_scaling_half = 2,       /* 1:2 */
    pip_scaling_quarter = 3,    /* 1:4 */
    pip_scaling_one_half = 4,   /* 3:2 */
    pip_scaling_fullscreen = 5, /* scale to main video size */
} mpls_pip_scaling;

typedef struct {
    uint32_t        time;          /* start timestamp (clip time) when the block is valid */
    uint16_t        xpos;
    uint16_t        ypos;
    uint8_t         scale_factor;  /* mpls_pip_scaling. Note: PSR14 may override this ! */
} MPLS_PIP_DATA;

typedef enum {
    pip_timeline_sync_mainpath = 1,  /* timeline refers to main path */
    pip_timeline_async_subpath = 2,  /* timeline refers to sub-path time */
    pip_timeline_async_mainpath = 3, /* timeline refers to main path */
} mpls_pip_timeline;

typedef struct {
    uint16_t        clip_ref;             /* clip id for secondary_video_ref (STN) */
    uint8_t         secondary_video_ref;  /* secondary video stream id (STN) */
    uint8_t         timeline_type;        /* mpls_pip_timeline */
    uint8_t         luma_key_flag;        /* use luma keying */
    uint8_t         upper_limit_luma_key; /* luma key (secondary video pixels with Y <= this value are transparent) */
    uint8_t         trick_play_flag;      /* show synchronous PiP when playing trick speed */

    uint16_t        data_count;
    MPLS_PIP_DATA   *data;
} MPLS_PIP_METADATA;

typedef enum {
    primary_green,
    primary_blue,
    primary_red
} mpls_static_primaries;    /* They are stored as GBR, we would like to show them as RGB */

typedef struct mpls_static_metadata
{
    uint8_t     dynamic_range_type;
    uint16_t    display_primaries_x[3];
    uint16_t    display_primaries_y[3];
    uint16_t    white_point_x;
    uint16_t    white_point_y;
    uint16_t    max_display_mastering_luminance;
    uint16_t    min_display_mastering_luminance;
    uint16_t    max_CLL;
    uint16_t    max_FALL;
} MPLS_STATIC_METADATA;

typedef struct mpls_pl
{
    uint32_t        type_indicator;   /* 'MPLS' */
    uint32_t        type_indicator2;  /* version */
    uint32_t        list_pos;
    uint32_t        mark_pos;
    uint32_t        ext_pos;
    MPLS_AI         app_info;
    uint16_t        list_count;
    uint16_t        sub_count;
    uint16_t        mark_count;
    MPLS_PI        *play_item;
    MPLS_SUB       *sub_path;
    MPLS_PLM       *play_mark;

    // extension data (profile 5, version 2.4)
    uint16_t        ext_sub_count;
    MPLS_SUB       *ext_sub_path;  // sub path entries extension

    // extension data (Picture-In-Picture metadata)
    uint16_t           ext_pip_data_count;
    MPLS_PIP_METADATA *ext_pip_data;  // pip metadata extension

    // extension data (Static Metadata)
    uint8_t         ext_static_metadata_count;
    MPLS_STATIC_METADATA    *ext_static_metadata;

} MPLS_PL;

#endif // BLURAY_MPLS_DATA_H_
