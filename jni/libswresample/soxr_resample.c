/*
 * audio resampling with soxr
 * Copyright (c) 2012 Rob Sykes <robs@users.sourceforge.net>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * audio resampling with soxr
 */

#include "libavutil/log.h"
#include "swresample_internal.h"

#include <soxr.h>

#include <android/log.h>
#define LOG_TAG "soxr_resample.c"
#define DLOG(...) //__android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


static struct ResampleContext *create(struct ResampleContext *c, int out_rate, int in_rate, int filter_size, int phase_shift, int linear,
        double cutoff, enum AVSampleFormat format, enum SwrFilterType filter_type, double kaiser_beta, double precision, int cheby, int exact_rational){
    soxr_error_t error;

    soxr_datatype_t type =
        format == AV_SAMPLE_FMT_S16P? SOXR_INT16_S :
        format == AV_SAMPLE_FMT_S16 ? SOXR_INT16_I :
        format == AV_SAMPLE_FMT_S32P? SOXR_INT32_S :
        format == AV_SAMPLE_FMT_S32 ? SOXR_INT32_I :
        format == AV_SAMPLE_FMT_FLTP? SOXR_FLOAT32_S :
        format == AV_SAMPLE_FMT_FLT ? SOXR_FLOAT32_I :
        format == AV_SAMPLE_FMT_DBLP? SOXR_FLOAT64_S :
        format == AV_SAMPLE_FMT_DBL ? SOXR_FLOAT64_I : (soxr_datatype_t)-1;

    soxr_io_spec_t io_spec = soxr_io_spec(type, type);

    soxr_quality_spec_t q_spec = soxr_quality_spec((int)((precision-2)/4), (SOXR_HI_PREC_CLOCK|SOXR_ROLLOFF_NONE)*!!cheby);
    q_spec.precision = precision;
#if !defined SOXR_VERSION /* Deprecated @ March 2013: */
    q_spec.bw_pc = cutoff? FFMAX(FFMIN(cutoff,.995),.8)*100 : q_spec.bw_pc;
#else
    q_spec.passband_end = cutoff? FFMAX(FFMIN(cutoff,.995),.8) : q_spec.passband_end;
#endif


    soxr_delete((soxr_t)c);

    c = (struct ResampleContext *)
        soxr_create(in_rate, out_rate, 0, &error, &io_spec, &q_spec, 0);

    DLOG("%s new soxr=%p", __func__, c);

    if (!c)
        av_log(NULL, AV_LOG_ERROR, "soxr_create: %s\n", error);
    return c;
}

static void destroy(struct ResampleContext * *c){
    DLOG("%s soxr=%p", __func__, c);
	soxr_delete((soxr_t)*c);
    *c = NULL;
}

static int flush(struct SwrContext *s){
#if !PAMP_CHANGES
    s->delayed_samples_fixup = soxr_delay((soxr_t)s->resample);

    soxr_process((soxr_t)s->resample, NULL, 0, NULL, NULL, 0, NULL);

    {
        float f;
        size_t idone, odone;
        soxr_process((soxr_t)s->resample, &f, 0, &idone, &f, 0, &odone);
        s->delayed_samples_fixup -= soxr_delay((soxr_t)s->resample);
    }
#else
    // PAMP change: properly flush soxr. NOTE: after flushing, soxr is unable to process further inbound samples as it stays in flushing mode and it will crash
    // if more in samples feeded into it. soxr_clear clears it without complete rebuilding, though with issues.

    // NOTE: don't do anything here, handle everything in process
    // This is due to flush happening before next process, as soxr flushing state should be carefully handled in one point

//    soxr_clear((soxr_t)s->resample);
//
//    // NOTE: soxr_clear clears internal soxr resamplers struct and few other components, so soxr is in uninitialized state now. This happens for some quality / SR combinations, not always.
//    // Force it to initialize state (as otherwise, calling flush again will crash us)
//
//    soxr_error_t error = soxr_set_error((soxr_t)s->resample, soxr_set_num_channels((soxr_t)s->resample, s->in.ch_count));
//    error = soxr_set_error((soxr_t)s->resample, soxr_set_io_ratio((soxr_t)s->resample, 1.0, 0));

    DLOG("%s soxr=%p", __func__, s->resample);
#endif

    return 0;
}

static int process(
        struct ResampleContext * c, AudioData *dst, int dst_size,
        AudioData *src, int src_size, int *consumed){
    size_t idone, odone;

    soxr_error_t error = soxr_set_error((soxr_t)c, soxr_set_num_channels((soxr_t)c, src->ch_count));
    if (!error) {
    	// NOTE: as soon as soxr is set to flush mode, we can't continue with it with any non zero src buffer
    	// In such case, soxr should be completely reinited
    	// ? no access to soxr flushing
    	// ? no access to s->flushed

    	soxr_t p = (soxr_t)c;

    	if(src_size > 0) {
    		// This is not a flush. If soxr is in flushing state, need to reinitialize it completely
    		if(soxr_is_flushing(p)) {
    			DLOG("%s WAS flushing REINIT src_size=%d dst_size=%d", __func__, src_size, dst_size);
				soxr_clear(p);
				soxr_error_t error = soxr_set_error(p, soxr_set_num_channels(p, src->ch_count));
				error = soxr_set_error(p, soxr_set_io_ratio(p, 1.0, 0));
    		}

    		error = soxr_process((soxr_t)c, src->ch, (size_t)src_size, &idone, dst->ch, (size_t)dst_size, &odone);
    	} else {
    		// This is flush request. We need to specifically apply NULL in buffer, just src_size == 0 is not enough
    		DLOG("%s FLUSH requested src_size=%d dst_size=%d", __func__, src_size, dst_size);
    		error = soxr_process(p, NULL, 0, &idone, dst->ch, (size_t)dst_size, &odone);
    	}
    }
    else
        idone = 0;

    *consumed = (int)idone;
    return error? -1 : odone;
}

static int64_t get_delay(struct SwrContext *s, int64_t base){
    double delayed_samples = soxr_delay((soxr_t)s->resample);
    double delay_s;

    if (s->flushed)
        delayed_samples += s->delayed_samples_fixup;

    delay_s = delayed_samples / s->out_sample_rate;

    return (int64_t)(delay_s * base + .5);
}

static int invert_initial_buffer(struct ResampleContext *c, AudioData *dst, const AudioData *src,
                                 int in_count, int *out_idx, int *out_sz){
    return 0;
}

static int64_t get_out_samples(struct SwrContext *s, int in_samples){
    double out_samples = (double)s->out_sample_rate / s->in_sample_rate * in_samples;
    double delayed_samples = soxr_delay((soxr_t)s->resample);

    if (s->flushed)
        delayed_samples += s->delayed_samples_fixup;

    return (int64_t)(out_samples + delayed_samples + 1 + .5);
}

struct Resampler const swri_soxr_resampler={
    create, destroy, process, flush, NULL /* set_compensation */, get_delay,
    invert_initial_buffer, get_out_samples
};

