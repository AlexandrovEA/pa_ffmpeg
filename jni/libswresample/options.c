/*
 * Copyright (C) 2011-2013 Michael Niedermayer (michaelni@gmx.at)
 *
 * This file is part of libswresample
 *
 * libswresample is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libswresample is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libswresample; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/opt.h"
#include "swresample_internal.h"

#include <float.h>

#define  C30DB  M_SQRT2
#define  C15DB  1.189207115
#define C__0DB  1.0
#define C_15DB  0.840896415
#define C_30DB  M_SQRT1_2
#define C_45DB  0.594603558
#define C_60DB  0.5

#define OFFSET(x) offsetof(SwrContext,x)
#define PARAM AV_OPT_FLAG_AUDIO_PARAM

// Begin PAMP change: use NULL_IF_CONFIG_SMALL() for options help labels
#define N(s) NULL_IF_CONFIG_SMALL(s)

// PAMP change - commented out options which are 0 by default
// NOTE: some are used, e.g. in opusdec, be careful

static const AVOption options[]={
{"ich"                  , N("set input channel count")     , OFFSET(user_in_ch_count  ), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"in_channel_count"     , N("set input channel count")     , OFFSET(user_in_ch_count  ), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"och"                  , N("set output channel count")    , OFFSET(user_out_ch_count ), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"out_channel_count"    , N("set output channel count")    , OFFSET(user_out_ch_count ), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"uch"                  , N("set used channel count")      , OFFSET(user_used_ch_count), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"used_channel_count"   , N("set used channel count")      , OFFSET(user_used_ch_count), AV_OPT_TYPE_INT, {.i64=0                    }, 0      , SWR_CH_MAX, PARAM},
{"isr"                  , N("set input sample rate")       , OFFSET( in_sample_rate), AV_OPT_TYPE_INT  , {.i64=0                     }, 0      , INT_MAX   , PARAM},
{"in_sample_rate"       , N("set input sample rate")       , OFFSET( in_sample_rate), AV_OPT_TYPE_INT  , {.i64=0                     }, 0      , INT_MAX   , PARAM},
{"osr"                  , N("set output sample rate")      , OFFSET(out_sample_rate), AV_OPT_TYPE_INT  , {.i64=0                     }, 0      , INT_MAX   , PARAM},
{"out_sample_rate"      , N("set output sample rate")      , OFFSET(out_sample_rate), AV_OPT_TYPE_INT  , {.i64=0                     }, 0      , INT_MAX   , PARAM},
{"isf"                  , N("set input sample format")     , OFFSET( in_sample_fmt ), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"in_sample_fmt"        , N("set input sample format")     , OFFSET( in_sample_fmt ), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"osf"                  , N("set output sample format")    , OFFSET(out_sample_fmt ), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"out_sample_fmt"       , N("set output sample format")    , OFFSET(out_sample_fmt ), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"tsf"                  , N("set internal sample format")  , OFFSET(user_int_sample_fmt), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"internal_sample_fmt"  , N("set internal sample format")  , OFFSET(user_int_sample_fmt), AV_OPT_TYPE_SAMPLE_FMT , {.i64=AV_SAMPLE_FMT_NONE}, -1   , INT_MAX, PARAM},
{"icl"                  , N("set input channel layout")    , OFFSET(user_in_ch_layout ), AV_OPT_TYPE_CHANNEL_LAYOUT, {.i64=0           }, INT64_MIN, INT64_MAX , PARAM, "channel_layout"},
{"in_channel_layout"    , N("set input channel layout")    , OFFSET(user_in_ch_layout ), AV_OPT_TYPE_CHANNEL_LAYOUT, {.i64=0           }, INT64_MIN, INT64_MAX , PARAM, "channel_layout"},
{"ocl"                  , N("set output channel layout")   , OFFSET(user_out_ch_layout), AV_OPT_TYPE_CHANNEL_LAYOUT, {.i64=0           }, INT64_MIN, INT64_MAX , PARAM, "channel_layout"},
{"out_channel_layout"   , N("set output channel layout")   , OFFSET(user_out_ch_layout), AV_OPT_TYPE_CHANNEL_LAYOUT, {.i64=0           }, INT64_MIN, INT64_MAX , PARAM, "channel_layout"},
// MaxMP: these are needed for proper 6ch mixing
{"clev"                 , N("set center mix level")        , OFFSET(clev           ), AV_OPT_TYPE_FLOAT, {.dbl=C_30DB                }, -32    , 32        , PARAM},
{"center_mix_level"     , N("set center mix level")        , OFFSET(clev           ), AV_OPT_TYPE_FLOAT, {.dbl=C_30DB                }, -32    , 32        , PARAM},
{"slev"                 , N("set surround mix level")      , OFFSET(slev           ), AV_OPT_TYPE_FLOAT, {.dbl=C_30DB                }, -32    , 32        , PARAM},
{"surround_mix_level"   , N("set surround mix Level")      , OFFSET(slev           ), AV_OPT_TYPE_FLOAT, {.dbl=C_30DB                }, -32    , 32        , PARAM},
{"lfe_mix_level"        , N("set LFE mix level")           , OFFSET(lfe_mix_level  ), AV_OPT_TYPE_FLOAT, {.dbl=0                     }, -32    , 32        , PARAM},
{"rmvol"                , N("set rematrix volume")         , OFFSET(rematrix_volume), AV_OPT_TYPE_FLOAT, {.dbl=1.0                   }, -1000  , 1000      , PARAM},
{"rematrix_volume"      , N("set rematrix volume")         , OFFSET(rematrix_volume), AV_OPT_TYPE_FLOAT, {.dbl=1.0                   }, -1000  , 1000      , PARAM},
{"rematrix_maxval"      , N("set rematrix maxval" )        , OFFSET(rematrix_maxval), AV_OPT_TYPE_FLOAT, {.dbl=0.0                   }, 0      , 1000      , PARAM},
{"flags"                , N("set flags")                   , OFFSET(flags          ), AV_OPT_TYPE_FLAGS, {.i64=0                     }, 0      , UINT_MAX  , PARAM, "flags"},
{"swr_flags"            , N("set flags")                   , OFFSET(flags          ), AV_OPT_TYPE_FLAGS, {.i64=0                     }, 0      , UINT_MAX  , PARAM, "flags"},
{"res"                  , N("force resampling")            , 0                      , AV_OPT_TYPE_CONST, {.i64=0/*SWR_FLAG_RESAMPLE*/     }, INT_MIN, INT_MAX   , PARAM, "flags"}, // PAMP change - force 0 flag by default
{"dither_scale"         , N("set dither scale")            , OFFSET(dither.scale   ), AV_OPT_TYPE_FLOAT, {.dbl=1                     }, 0      , INT_MAX   , PARAM},
#if !PAMP_CHANGES
{"dither_method"        , N("set dither method")           , OFFSET(user_dither_method),AV_OPT_TYPE_INT, {.i64=0                     }, 0      , SWR_DITHER_NB-1, PARAM, "dither_method"},
{"rectangular"          , N("select rectangular dither")   , 0                      , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_RECTANGULAR}, INT_MIN, INT_MAX   , PARAM, "dither_method"},
{"triangular"           , N("select triangular dither")    , 0                      , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_TRIANGULAR }, INT_MIN, INT_MAX   , PARAM, "dither_method"},
{"triangular_hp"        , N("select triangular dither with high pass") , 0          , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_TRIANGULAR_HIGHPASS }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"lipshitz"             , N("select Lipshitz noise shaping dither") , 0             , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_LIPSHITZ}, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"shibata"              , N("select Shibata noise shaping dither") , 0              , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_SHIBATA }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"low_shibata"          , N("select low Shibata noise shaping dither") , 0          , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_LOW_SHIBATA }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"high_shibata"         , N("select high Shibata noise shaping dither") , 0         , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_HIGH_SHIBATA }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"f_weighted"           , N("select f-weighted noise shaping dither") , 0           , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_F_WEIGHTED }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"modified_e_weighted"  , N("select modified-e-weighted noise shaping dither") , 0  , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_MODIFIED_E_WEIGHTED }, INT_MIN, INT_MAX, PARAM, "dither_method"},
{"improved_e_weighted"  , N("select improved-e-weighted noise shaping dither") , 0  , AV_OPT_TYPE_CONST, {.i64=SWR_DITHER_NS_IMPROVED_E_WEIGHTED }, INT_MIN, INT_MAX, PARAM, "dither_method"},
#endif
{"filter_size"          , N("set swr resampling filter size"), OFFSET(filter_size)  , AV_OPT_TYPE_INT  , {.i64=32                    }, 0      , INT_MAX   , PARAM },
{"phase_shift"          , N("set swr resampling phase shift"), OFFSET(phase_shift)  , AV_OPT_TYPE_INT  , {.i64=10                    }, 0      , 24        , PARAM },
{"linear_interp"        , N("enable linear interpolation") , OFFSET(linear_interp)  , AV_OPT_TYPE_BOOL , {.i64=1                     }, 0      , 1         , PARAM },
{"exact_rational"       , N("enable exact rational")       , OFFSET(exact_rational) , AV_OPT_TYPE_BOOL , {.i64=1                     }, 0      , 1         , PARAM },
#if !PAMP_CHANGES
{"cutoff"               , N("set cutoff frequency ratio")  , OFFSET(cutoff)         , AV_OPT_TYPE_DOUBLE,{.dbl=0.                    }, 0      , 1         , PARAM },

/* duplicate option in order to work with avconv */
{"resample_cutoff"      , N("set cutoff frequency ratio")  , OFFSET(cutoff)         , AV_OPT_TYPE_DOUBLE,{.dbl=0.                    }, 0      , 1         , PARAM },

{"resampler"            , N("set resampling Engine")       , OFFSET(engine)         , AV_OPT_TYPE_INT  , {.i64=0                     }, 0      , SWR_ENGINE_NB-1, PARAM, "resampler"},
{"swr"                  , N("select SW Resampler")         , 0                      , AV_OPT_TYPE_CONST, {.i64=SWR_ENGINE_SWR        }, INT_MIN, INT_MAX   , PARAM, "resampler"},
{"soxr"                 , N("select SoX Resampler")        , 0                      , AV_OPT_TYPE_CONST, {.i64=SWR_ENGINE_SOXR       }, INT_MIN, INT_MAX   , PARAM, "resampler"},
#endif
{"precision"            , N("set soxr resampling precision (in bits)")
                                                        , OFFSET(precision)      , AV_OPT_TYPE_DOUBLE,{.dbl=20.0                  }, 15.0   , 33.0      , PARAM },
{"cheby"                , N("enable soxr Chebyshev passband & higher-precision irrational ratio approximation")
                                                        , OFFSET(cheby)          , AV_OPT_TYPE_BOOL , {.i64=0                     }, 0      , 1         , PARAM },
{"min_comp"             , N("set minimum difference between timestamps and audio data (in seconds) below which no timestamp compensation of either kind is applied")
                                                        , OFFSET(min_compensation),AV_OPT_TYPE_FLOAT ,{.dbl=FLT_MAX               }, 0      , FLT_MAX   , PARAM },
{"min_hard_comp"        , N("set minimum difference between timestamps and audio data (in seconds) to trigger padding/trimming the data.")
                                                        , OFFSET(min_hard_compensation),AV_OPT_TYPE_FLOAT ,{.dbl=0.1                   }, 0      , INT_MAX   , PARAM },
{"comp_duration"        , N("set duration (in seconds) over which data is stretched/squeezed to make it match the timestamps.")
                                                        , OFFSET(soft_compensation_duration),AV_OPT_TYPE_FLOAT ,{.dbl=1                     }, 0      , INT_MAX   , PARAM },
{"max_soft_comp"        , N("set maximum factor by which data is stretched/squeezed to make it match the timestamps.")
                                                        , OFFSET(max_soft_compensation),AV_OPT_TYPE_FLOAT ,{.dbl=0                     }, INT_MIN, INT_MAX   , PARAM },
{"async"                , N("simplified 1 parameter audio timestamp matching, 0(disabled), 1(filling and trimming), >1(maximum stretch/squeeze in samples per second)")
                                                        , OFFSET(async)          , AV_OPT_TYPE_FLOAT ,{.dbl=0                     }, INT_MIN, INT_MAX   , PARAM },
{"first_pts"            , N("Assume the first pts should be this value (in samples).")
                                                        , OFFSET(firstpts_in_samples), AV_OPT_TYPE_INT64 ,{.i64=AV_NOPTS_VALUE    }, INT64_MIN,INT64_MAX, PARAM },

{ "matrix_encoding"     , N("set matrixed stereo encoding") , OFFSET(matrix_encoding), AV_OPT_TYPE_INT   ,{.i64 = AV_MATRIX_ENCODING_NONE}, AV_MATRIX_ENCODING_NONE,     AV_MATRIX_ENCODING_NB-1, PARAM, "matrix_encoding" },
#if !PAMP_CHANGES
    { "none",  N("select none"),               0, AV_OPT_TYPE_CONST, { .i64 = AV_MATRIX_ENCODING_NONE  }, INT_MIN, INT_MAX, PARAM, "matrix_encoding" },
    { "dolby", N("select Dolby"),              0, AV_OPT_TYPE_CONST, { .i64 = AV_MATRIX_ENCODING_DOLBY }, INT_MIN, INT_MAX, PARAM, "matrix_encoding" },
    { "dplii", N("select Dolby Pro Logic II"), 0, AV_OPT_TYPE_CONST, { .i64 = AV_MATRIX_ENCODING_DPLII }, INT_MIN, INT_MAX, PARAM, "matrix_encoding" },
#endif
{ "filter_type"         , N("select swr filter type")      , OFFSET(filter_type)    , AV_OPT_TYPE_INT  , { .i64 = SWR_FILTER_TYPE_KAISER }, SWR_FILTER_TYPE_CUBIC, SWR_FILTER_TYPE_KAISER, PARAM, "filter_type" },
#if !PAMP_CHANGES
    { "cubic"           , N("select cubic")                , 0                      , AV_OPT_TYPE_CONST, { .i64 = SWR_FILTER_TYPE_CUBIC            }, INT_MIN, INT_MAX, PARAM, "filter_type" },
    { "blackman_nuttall", N("select Blackman Nuttall windowed sinc"), 0             , AV_OPT_TYPE_CONST, { .i64 = SWR_FILTER_TYPE_BLACKMAN_NUTTALL }, INT_MIN, INT_MAX, PARAM, "filter_type" },
    { "kaiser"          , N("select Kaiser windowed sinc") , 0                      , AV_OPT_TYPE_CONST, { .i64 = SWR_FILTER_TYPE_KAISER           }, INT_MIN, INT_MAX, PARAM, "filter_type" },
#endif
{ "kaiser_beta"         , N("set swr Kaiser window beta")  , OFFSET(kaiser_beta)    , AV_OPT_TYPE_DOUBLE  , {.dbl=9                     }, 2      , 16        , PARAM },
#if !PAMP_CHANGES
{ "output_sample_bits"  , N("set swr number of output sample bits"), OFFSET(dither.output_sample_bits), AV_OPT_TYPE_INT  , {.i64=0   }, 0      , 64        , PARAM },
#endif
{0}
};

static const char* context_to_name(void* ptr) {
    return "SWR";
}

static const AVClass av_class = {
    .class_name                = "SWResampler",
    .item_name                 = context_to_name,
    .option                    = options,
    .version                   = LIBAVUTIL_VERSION_INT,
    .log_level_offset_offset   = OFFSET(log_level_offset),
    .parent_log_context_offset = OFFSET(log_ctx),
    .category                  = AV_CLASS_CATEGORY_SWRESAMPLER,
};

const AVClass *swr_get_class(void)
{
    return &av_class;
}

av_cold struct SwrContext *swr_alloc(void){
    SwrContext *s= av_mallocz(sizeof(SwrContext));
    if(s){
        s->av_class= &av_class;
        av_opt_set_defaults(s);
    }
    return s;
}
