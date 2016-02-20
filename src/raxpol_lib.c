/*
   -	raxpol_lib.c --
   -		This file defines functions that store and access
   -		RaXPol data files.
   -
   .	Copyright (c) 2012, Gordon D. Carrie. All rights reserved.
   .	
   .	Redistribution and use in source and binary forms, with or without
   .	modification, are permitted provided that the following conditions
   .	are met:
   .	
   .	    * Redistributions of source code must retain the above copyright
   .	    notice, this list of conditions and the following disclaimer.
   .
   .	    * Redistributions in binary form must reproduce the above copyright
   .	    notice, this list of conditions and the following disclaimer in the
   .	    documentation and/or other materials provided with the distribution.
   .	
   .	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   .	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   .	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   .	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   .	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   .	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   .	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   .	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   .	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   .	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   .	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   .
   .	Please send feedback to dev0@trekix.net
   .
   .	$Revision: 1.61 $ $Date: 2015/02/10 21:07:13 $
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <complex.h>
#include <unistd.h>
#include "alloc.h"
#include "type_nbit.h"
#include "val_buf.h"
#include "geog_lib.h"
#include "tm_calc_lib.h"
#include "raxpol.h"

/* Radar heading. If not NAN, overrides heading from ray headers. */ 
static double dflt_hdg = NAN;

/* If true, read "older format" files */
static int old_fmt;

/* Size of ray header. Varies with version. */
#define RAXPOL_RAY_HDR_SZ_OLD 276
#define RAXPOL_RAY_HDR_SZ_NEW 292
static size_t raxpol_ray_hdr_sz;

/* Math constants */ 
#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
#define DEG_PER_RAD	57.29577951308232087648
#define RAD_PER_DEG	0.01745329251994329576

/* Local functions */ 
static float *alloc_field_f(char *, size_t);
static float _Complex *alloc_field_fc(char *, size_t);
static int no_in_stub(struct RaXPol_Data *, FILE *);
static int read_spp_ray(struct RaXPol_Data *, FILE *);
static int read_spp_sum_pwr_ray(struct RaXPol_Data *, FILE *);
static int read_dpp_ray(struct RaXPol_Data *, FILE *);
static int read_dpp_sum_pwr_ray(struct RaXPol_Data *, FILE *);
static int no_out_stub(struct RaXPol_Data *, float *);
static int pp_dbm(struct RaXPol_Data *, float *z, float *);
static int pp_dbz(struct RaXPol_Data *, float *, float *);
static int pp_phidp(struct RaXPol_Data *, float _Complex *, float *);
static int pp_zdr(struct RaXPol_Data *, float *, float *, float *);
static int pp_phidp(struct RaXPol_Data *, float _Complex *, float *);
static int pp_rhohv(struct RaXPol_Data *, float *, float *, float _Complex *,
	float *);
static int pp_vel(struct RaXPol_Data *, float _Complex *, int, float *);
static int spp_dbmhc(struct RaXPol_Data *, float *);
static int spp_dbmvc(struct RaXPol_Data *, float *);
static int spp_dbz(struct RaXPol_Data *, float *);
static int spp_dbz1(struct RaXPol_Data *, float *);
static int spp_vel(struct RaXPol_Data *, float *);
static int spp_zdr(struct RaXPol_Data *, float *);
static int spp_phidp(struct RaXPol_Data *, float *);
static int spp_rhohv(struct RaXPol_Data *, float *);
static int spp_std(struct RaXPol_Data *, float *);
static int spp_snrhc(struct RaXPol_Data *, float *);
static int spp_snrvc(struct RaXPol_Data *, float *);
static int spp_sum_pwr_dbmhc(struct RaXPol_Data *, float *);
static int spp_sum_pwr_dbmvc(struct RaXPol_Data *, float *);
static int spp_sum_pwr_dbz(struct RaXPol_Data *, float *);
static int spp_sum_pwr_dbz1(struct RaXPol_Data *, float *);
static int spp_sum_pwr_vel(struct RaXPol_Data *, float *);
static int spp_sum_pwr_zdr(struct RaXPol_Data *, float *);
static int spp_sum_pwr_phidp(struct RaXPol_Data *, float *);
static int spp_sum_pwr_rhohv(struct RaXPol_Data *, float *);
static int spp_sum_pwr_std(struct RaXPol_Data *, float *);
static int spp_sum_pwr_snrhc(struct RaXPol_Data *, float *);
static int spp_sum_pwr_snrvc(struct RaXPol_Data *, float *);
static int dpp_dbmhc(struct RaXPol_Data *, float *);
static int dpp_dbmvc(struct RaXPol_Data *, float *);
static int dpp_dbz(struct RaXPol_Data *, float *);
static int dpp_dbz1(struct RaXPol_Data *, float *);
static int dpp_vel(struct RaXPol_Data *, float *);
static int dpp_zdr(struct RaXPol_Data *, float *);
static int dpp_phidp(struct RaXPol_Data *, float *);
static int dpp_rhohv(struct RaXPol_Data *, float *);
static int dpp_std(struct RaXPol_Data *, float *);
static int dpp_snrhc(struct RaXPol_Data *, float *);
static int dpp_snrvc(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_dbmhc(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_dbmvc(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_dbz(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_dbz1(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_vel(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_zdr(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_phidp(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_rhohv(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_std(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_snrhc(struct RaXPol_Data *, float *);
static int dpp_sum_pwr_snrvc(struct RaXPol_Data *, float *);
double ang_to_ref(const double, const double);
static float mean(float *, int);
static double ray_az(struct RaXPol_Ray_Hdr *, int);

/* String representations of enumerators defined in raxpol.h */ 
static char *servmode_s[RAXPOL_N_SERVMODES] = {
    "SPP", "SPP_SUM_PWR", "DPP", "DPP_SUM_PWR", "FFT", "FFT2", "FFT2I",
    "UNKNOWN"
};
static char *scan_type_descr[RAXPOL_N_SCAN_TYPES] = {
    "None", "Custom", "Soft stop", "Point", "Slew",
    "PPI", "RHI", "Az Raster", "El Raster", "Vol", "Paused"
};

/* Mandate use of "old" (2011) format */ 
void RaXPol_Old_Fmt(void)
{
    old_fmt = 1;
}

/* Initialize file header at fh_p with bogus values */
void RaXPol_Init_File_Hdr(struct RaXPol_File_Hdr *fh_p)
{
    fh_p->version_code = -1;
    fh_p->asp_chirp_bandwidth = NAN;
    fh_p->asp_chirp_center_freq = NAN;
    fh_p->asp_chirp_intrl_delay = -1;
    fh_p->asp_chirp_width = NAN;
    fh_p->asp_hop_ctr_freq = NAN;
    fh_p->asp_hop_step_freq = NAN;
    fh_p->asp_hop_width = -1;
    fh_p->asp_num_hops = -1;
    fh_p->asp_trigger_delay = -1;
    fh_p->asp_tukey_alpha = NAN;
    fh_p->asp_waveform_type = -1;
    fh_p->asp_chirp_delay = -1;
    fh_p->clutter_filter_width = NAN;
    fh_p->clutter_avg_intvl = -1;
    fh_p->data_rate = NAN;
    fh_p->fft_length = -1;
    fh_p->fft_window_type = -1;
    fh_p->auto_file_roll_num_rec = -1;
    fh_p->auto_file_roll_num_scans = -1;
    fh_p->auto_file_roll_flsz = -1;
    fh_p->auto_file_roll_start_time = -1;
    fh_p->auto_file_roll_type = -1;
    fh_p->filter_bandwidth = NAN;
    fh_p->group_interval = -1;
    fh_p->H_dBZ_dBm_offset = NAN;
    fh_p->H_noise_power = NAN;
    fh_p->integration_time = NAN;
    fh_p->max_sampled_range = NAN;
    fh_p->num_rng_gates = -1;
    fh_p->number_group_pulses = -1;
    fh_p->post_decimation_level = -1;
    fh_p->post_averaging_interval = -1;
    fh_p->pri1 = -1;
    fh_p->pri2 = -1;
    fh_p->pri_total = -1;
    fh_p->primary_decimation_CIC = -1;
    fh_p->pulse_compression = -1;
    fh_p->fir_output_len_ratio = NAN;
    fh_p->fir_output_offset_ratio = NAN;
    fh_p->range_gate_spacing = NAN;
    fh_p->range_resolution = NAN;
    fh_p->record_moments = -1;
    fh_p->record_raw = -1;
    fh_p->recording_enabled = -1;
    fh_p->servmode = RAXPOL_UNK;
    fh_p->server_state = -1;
    fh_p->skip_count = -1;
    fh_p->software_decimation_level = -1;
    fh_p->sumpower = -1;
    fh_p->switch_delay = -1;
    fh_p->switch_padding = -1;
    fh_p->switch_width = -1;
    fh_p->total_avg_intvl = -1;
    fh_p->twta_internal_delay = -1;
    fh_p->tx_pulse_bracketing = -1;
    fh_p->tx_delay = -1;
    fh_p->tx_trigger_delay = -1;
    fh_p->tx_pulse_center_offset = -1;
    fh_p->unambiguous_range = NAN;
    fh_p->clutter_filter = -1;
    fh_p->custom_TX_waveform_was_used = -1;
    fh_p->V_dBZ_dBm_offset = NAN;
    fh_p->v_noise_dBm = NAN;
    fh_p->zero_range_gate_index = NAN;
    fh_p->scan_type = -1;
    fh_p->num_sweeps = -1;
    fh_p->timestamp_sec = -1;
    fh_p->timestamp_usec = -1;
    memset(fh_p->pulse_filter_file_name, '\0', 4096);
    memset(fh_p->custom_waveform_file_name, '\0', 4096);
}

int RaXPol_Read_File_Hdr(struct RaXPol_File_Hdr *fh_p, FILE *in)
{
    char buf[RAXPOL_FILE_HDR_SZ];	/* Input buffer */
    char *buf_p;			/* Pointer into buf */
    size_t sz;				/* Number of bytes to read */

    sz = RAXPOL_FILE_HDR_SZ;
    memset(fh_p, 0, sz);
    memset(buf, 0, sz);
    if ( fread(buf, sz, 1, in) != 1 ) {
	if ( feof(in) ) {
	    return EOF;
	} else {
	    return 0;
	}
    }
    buf_p = buf;
    fh_p->version_code = ValBuf_GetI4BYT(&buf_p);
    fh_p->asp_chirp_bandwidth = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_chirp_center_freq = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_chirp_intrl_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->asp_chirp_width = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_hop_ctr_freq = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_hop_step_freq = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_hop_width = ValBuf_GetI4BYT(&buf_p);
    fh_p->asp_num_hops = ValBuf_GetI4BYT(&buf_p);
    fh_p->asp_trigger_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->asp_tukey_alpha = ValBuf_GetF8BYT(&buf_p);
    fh_p->asp_waveform_type = ValBuf_GetI4BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->asp_chirp_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->clutter_filter_width = ValBuf_GetF8BYT(&buf_p);
    fh_p->clutter_avg_intvl = ValBuf_GetI4BYT(&buf_p);
    fh_p->data_rate = ValBuf_GetF8BYT(&buf_p);
    fh_p->fft_length = ValBuf_GetI4BYT(&buf_p);
    fh_p->fft_window_type = ValBuf_GetI4BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->auto_file_roll_num_rec = ValBuf_GetI4BYT(&buf_p);
    fh_p->auto_file_roll_num_scans = ValBuf_GetI4BYT(&buf_p);
    fh_p->auto_file_roll_flsz = ValBuf_GetI4BYT(&buf_p);
    fh_p->auto_file_roll_start_time = ValBuf_GetI4BYT(&buf_p);
    fh_p->auto_file_roll_type = ValBuf_GetI4BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->filter_bandwidth = ValBuf_GetF8BYT(&buf_p);
    fh_p->group_interval = ValBuf_GetI4BYT(&buf_p);
    fh_p->H_dBZ_dBm_offset = ValBuf_GetF8BYT(&buf_p);
    fh_p->H_noise_power = ValBuf_GetF8BYT(&buf_p);
    fh_p->integration_time = ValBuf_GetF8BYT(&buf_p);
    fh_p->max_sampled_range = ValBuf_GetF8BYT(&buf_p);
    fh_p->num_rng_gates = ValBuf_GetI4BYT(&buf_p);
    if ( fh_p->num_rng_gates <= 0) {
	fprintf(stderr, "Expected positive integer for number of range gates, "
		"got %d\n", fh_p->num_rng_gates);
	return 0;
    }
    fh_p->number_group_pulses = ValBuf_GetI4BYT(&buf_p);
    fh_p->post_decimation_level = ValBuf_GetI4BYT(&buf_p);
    fh_p->post_averaging_interval = ValBuf_GetI4BYT(&buf_p);
    fh_p->pri1 = ValBuf_GetI4BYT(&buf_p);
    fh_p->pri2 = ValBuf_GetI4BYT(&buf_p);
    fh_p->pri_total = ValBuf_GetI4BYT(&buf_p);
    fh_p->primary_decimation_CIC = ValBuf_GetI4BYT(&buf_p);
    fh_p->pulse_compression = ValBuf_GetI4BYT(&buf_p);
    fh_p->fir_output_len_ratio = ValBuf_GetF8BYT(&buf_p);
    fh_p->fir_output_offset_ratio = ValBuf_GetF8BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->range_gate_spacing = ValBuf_GetF8BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->range_resolution = ValBuf_GetF8BYT(&buf_p);
    fh_p->record_moments = ValBuf_GetI4BYT(&buf_p);
    fh_p->record_raw = ValBuf_GetI4BYT(&buf_p);
    fh_p->recording_enabled = ValBuf_GetI4BYT(&buf_p);
    fh_p->servmode = ValBuf_GetI4BYT(&buf_p);
    if ( fh_p->servmode < 0 || fh_p->servmode >= RAXPOL_N_SERVMODES ) {
	fprintf(stderr, "%d is not a valid server mode.\n", fh_p->servmode);
	return 0;
    }
    buf_p += 4;			/* Skip reserved */
    fh_p->server_state = ValBuf_GetI4BYT(&buf_p);
    fh_p->skip_count = ValBuf_GetI4BYT(&buf_p);
    buf_p += 4;			/* Skip reserved */
    fh_p->software_decimation_level = ValBuf_GetI4BYT(&buf_p);
    fh_p->sumpower = ValBuf_GetI4BYT(&buf_p);
    fh_p->switch_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->switch_padding = ValBuf_GetI4BYT(&buf_p);
    fh_p->switch_width = ValBuf_GetI4BYT(&buf_p);
    fh_p->total_avg_intvl = ValBuf_GetI4BYT(&buf_p);
    fh_p->twta_internal_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->tx_pulse_bracketing = ValBuf_GetI4BYT(&buf_p);
    fh_p->tx_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->tx_trigger_delay = ValBuf_GetI4BYT(&buf_p);
    fh_p->tx_pulse_center_offset = ValBuf_GetI4BYT(&buf_p);
    fh_p->unambiguous_range = ValBuf_GetF8BYT(&buf_p);
    fh_p->clutter_filter = ValBuf_GetI4BYT(&buf_p);
    fh_p->custom_TX_waveform_was_used = ValBuf_GetI4BYT(&buf_p);
    fh_p->V_dBZ_dBm_offset = ValBuf_GetF8BYT(&buf_p);
    fh_p->v_noise_dBm = ValBuf_GetF8BYT(&buf_p);
    fh_p->zero_range_gate_index = ValBuf_GetF8BYT(&buf_p);
    fh_p->scan_type = ValBuf_GetI4BYT(&buf_p);
    if ( fh_p->scan_type < 0 || fh_p->scan_type >= RAXPOL_N_SCAN_TYPES ) {
	fprintf(stderr, "%d is not a valid scan_type\n", fh_p->scan_type);
	return 0;
    }
    fh_p->num_sweeps = ValBuf_GetI4BYT(&buf_p);
    fh_p->timestamp_sec = ValBuf_GetI4BYT(&buf_p);
    fh_p->timestamp_usec = ValBuf_GetI4BYT(&buf_p);
    memcpy(fh_p->pulse_filter_file_name, buf_p,
	    sizeof(fh_p->pulse_filter_file_name) - 1);
    buf_p += sizeof(fh_p->pulse_filter_file_name);
    memcpy(fh_p->custom_waveform_file_name, buf_p,
	    sizeof(fh_p->pulse_filter_file_name) - 1);
    buf_p += sizeof(fh_p->pulse_filter_file_name);
    return 1;
}

void RaXPol_FPrintf_File_Hdr(struct RaXPol_File_Hdr *fh_p, FILE *out)
{
    fprintf(out, "version_code %d\n", fh_p->version_code);
    fprintf(out, "asp_chirp_bandwidth %lf\n", fh_p->asp_chirp_bandwidth);
    fprintf(out, "asp_chirp_center_freq %lf\n", fh_p->asp_chirp_center_freq);
    fprintf(out, "asp_chirp_intrl_delay %d\n", fh_p->asp_chirp_intrl_delay);
    fprintf(out, "asp_chirp_width %lf\n", fh_p->asp_chirp_width);
    fprintf(out, "asp_hop_ctr_freq %lf\n", fh_p->asp_hop_ctr_freq);
    fprintf(out, "asp_hop_step_freq %lf\n", fh_p->asp_hop_step_freq);
    fprintf(out, "asp_hop_width %d\n", fh_p->asp_hop_width);
    fprintf(out, "asp_num_hops %d\n", fh_p->asp_num_hops);
    fprintf(out, "asp_trigger_delay %d\n", fh_p->asp_trigger_delay);
    fprintf(out, "asp_tukey_alpha %lf\n", fh_p->asp_tukey_alpha);
    fprintf(out, "asp_waveform_type %d\n", fh_p->asp_waveform_type);
    fprintf(out, "asp_chirp_delay %d\n", fh_p->asp_chirp_delay);
    fprintf(out, "clutter_filter_width %lf\n", fh_p->clutter_filter_width);
    fprintf(out, "clutter_avg_intvl %d\n", fh_p->clutter_avg_intvl);
    fprintf(out, "data_rate %lf\n", fh_p->data_rate);
    fprintf(out, "fft_length %d\n", fh_p->fft_length);
    fprintf(out, "fft_window_type %d\n", fh_p->fft_window_type);
    fprintf(out, "auto_file_roll_num_rec %d\n", fh_p->auto_file_roll_num_rec);
    fprintf(out, "auto_file_roll_num_scans %d\n",
	    fh_p->auto_file_roll_num_scans);
    fprintf(out, "auto_file_roll_flsz %d\n", fh_p->auto_file_roll_flsz);
    fprintf(out, "auto_file_roll_start_time %d\n",
	    fh_p->auto_file_roll_start_time);
    fprintf(out, "auto_file_roll_type %d\n", fh_p->auto_file_roll_type);
    fprintf(out, "filter_bandwidth %lf\n", fh_p->filter_bandwidth);
    fprintf(out, "group_interval %d\n", fh_p->group_interval);
    fprintf(out, "H_dBZ_dBm_offset %lf\n", fh_p->H_dBZ_dBm_offset);
    fprintf(out, "H_noise_power %lf\n", fh_p->H_noise_power);
    fprintf(out, "integration_time %lf\n", fh_p->integration_time);
    fprintf(out, "max_sampled_range %lf\n", fh_p->max_sampled_range);
    fprintf(out, "num_rng_gates %d\n", fh_p->num_rng_gates);
    fprintf(out, "number_group_pulses %d\n", fh_p->number_group_pulses);
    fprintf(out, "post_decimation_level %d\n", fh_p->post_decimation_level);
    fprintf(out, "post_averaging_interval %d\n",
	    fh_p->post_averaging_interval);
    fprintf(out, "pri1 %d\n", fh_p->pri1);
    fprintf(out, "pri2 %d\n", fh_p->pri2);
    fprintf(out, "pri_total %d\n", fh_p->pri_total);
    fprintf(out, "primary_decimation_CIC %d\n", fh_p->primary_decimation_CIC);
    fprintf(out, "pulse_compression %d\n", fh_p->pulse_compression);
    fprintf(out, "fir_output_len_ratio %lf\n", fh_p->fir_output_len_ratio);
    fprintf(out, "fir_output_offset_ratio %lf\n",
	    fh_p->fir_output_offset_ratio);
    fprintf(out, "range_gate_spacing %lf\n", fh_p->range_gate_spacing);
    fprintf(out, "range_resolution %lf\n", fh_p->range_resolution);
    fprintf(out, "record_moments %d\n", fh_p->record_moments);
    fprintf(out, "record_raw %d\n", fh_p->record_raw);
    fprintf(out, "recording_enabled %d\n", fh_p->recording_enabled);
    fprintf(out, "servmode %d (%s)\n",
	    fh_p->servmode, servmode_s[fh_p->servmode]);
    fprintf(out, "server_state %d\n", fh_p->server_state);
    fprintf(out, "skip_count %d\n", fh_p->skip_count);
    fprintf(out, "software_decimation_level %d\n",
	    fh_p->software_decimation_level);
    fprintf(out, "sumpower %d\n", fh_p->sumpower);
    fprintf(out, "switch_delay %d\n", fh_p->switch_delay);
    fprintf(out, "switch_padding %d\n", fh_p->switch_padding);
    fprintf(out, "switch_width %d\n", fh_p->switch_width);
    fprintf(out, "total_avg_intvl %d\n", fh_p->total_avg_intvl);
    fprintf(out, "twta_internal_delay %d\n", fh_p->twta_internal_delay);
    fprintf(out, "tx_pulse_bracketing %d\n", fh_p->tx_pulse_bracketing);
    fprintf(out, "tx_delay %d\n", fh_p->tx_delay);
    fprintf(out, "tx_trigger_delay %d\n", fh_p->tx_trigger_delay);
    fprintf(out, "tx_pulse_center_offset %d\n", fh_p->tx_pulse_center_offset);
    fprintf(out, "unambiguous_range %lf\n", fh_p->unambiguous_range);
    fprintf(out, "clutter_filter %d\n", fh_p->clutter_filter);
    fprintf(out, "custom_TX_waveform_was_used %d\n",
	    fh_p->custom_TX_waveform_was_used);
    fprintf(out, "V_dBZ_dBm_offset %lf\n", fh_p->V_dBZ_dBm_offset);
    fprintf(out, "v_noise_dBm %lf\n", fh_p->v_noise_dBm);
    fprintf(out, "zero_range_gate_index %lf\n", fh_p->zero_range_gate_index);
    fprintf(out, "scan_type %d (%s)\n",
	    fh_p->scan_type, scan_type_descr[fh_p->scan_type]);
    fprintf(out, "num_sweeps %d\n", fh_p->num_sweeps);
    fprintf(out, "timestamp_sec %d\n", fh_p->timestamp_sec);
    fprintf(out, "timestamp_usec %d\n", fh_p->timestamp_usec);
    fprintf(out, "pulse_filter_file_name %s\n", fh_p->pulse_filter_file_name);
    fprintf(out, "custom_waveform_file_name %s\n",
	    fh_p->custom_waveform_file_name);
}

/* Initialize ray header structure at at rh_p with bogus values */
void RaXPol_Init_Ray_Hdr(struct RaXPol_Ray_Hdr *rh_p) {
    rh_p->timestamp_seconds = -1;
    rh_p->timestamp_useconds = -1;
    rh_p->radar_temperatures[0] = rh_p->radar_temperatures[1]
	= rh_p->radar_temperatures[2] = rh_p->radar_temperatures[3] = -1;
    rh_p->inclinometer_roll = -1;
    rh_p->inclinometer_pitch = -1;
    rh_p->fuel_sensor = -1;
    rh_p->cpu_temperature = NAN;
    rh_p->pedestal_scan_type = RAXPOL_NONE;
    rh_p->tx_power = NAN;
    rh_p->osc_lock = -1;
    rh_p->az = NAN;
    rh_p->el = NAN;
    rh_p->az_vel = NAN;
    rh_p->elev_vel = NAN;
    rh_p->az_current = NAN;
    rh_p->elev_current = NAN;
    rh_p->sweep_count = -1;
    rh_p->volume_count = -1;
    rh_p->flags = -1;
    rh_p->lat_ref = -1;
    rh_p->lat = NAN;
    rh_p->lon_ref = -1;
    rh_p->lon = NAN;
    rh_p->alt = NAN;
    rh_p->hdg_ref = -1;
    rh_p->hdg = NAN;
    rh_p->speed = NAN;
    memset(rh_p->nmea_msg_gpgga, '\0', 96);
    rh_p->utc_time_sec = -1;
    rh_p->utc_time_usec = -1;
    rh_p->data_type = -1;
    rh_p->data_size = -1;
}

/*
   Read ray header from in. Return 1/0 on success failure.
   If input fails, feof(in) or ferror(in) can indicate type of failure.
   (This function does not call clearerr(in).)
 */ 

int RaXPol_Read_Ray_Hdr(struct RaXPol_Ray_Hdr *rh_p, FILE *in)
{
    static char *buf;			/* Input buffer */
    char *buf_p;			/* Pointer into buf */

    memset(rh_p, 0, sizeof(struct RaXPol_Ray_Hdr));
    raxpol_ray_hdr_sz = old_fmt ? RAXPOL_RAY_HDR_SZ_OLD : RAXPOL_RAY_HDR_SZ_NEW;
    if ( !buf ) {
	if ( !(buf = MALLOC(raxpol_ray_hdr_sz)) ) {
	    fprintf(stderr, "Could not allocate %zu bytes for ray header "
		    "input buffer.\n", raxpol_ray_hdr_sz);
	    return 0;
	}
    }
    memset(buf, 0, raxpol_ray_hdr_sz);
    if ( fread(buf, raxpol_ray_hdr_sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Input error while reading ray header.\n");
	}
	return 0;
    }
    buf_p = buf;

    rh_p->timestamp_seconds = ValBuf_GetI4BYT(&buf_p);
    rh_p->timestamp_useconds = ValBuf_GetI4BYT(&buf_p);
    rh_p->radar_temperatures[0] = ValBuf_GetI4BYT(&buf_p);
    rh_p->radar_temperatures[1] = ValBuf_GetI4BYT(&buf_p);
    rh_p->radar_temperatures[2] = ValBuf_GetI4BYT(&buf_p);
    rh_p->radar_temperatures[3] = ValBuf_GetI4BYT(&buf_p);
    rh_p->inclinometer_roll = ValBuf_GetI4BYT(&buf_p);
    rh_p->inclinometer_pitch = ValBuf_GetI4BYT(&buf_p);
    rh_p->fuel_sensor = ValBuf_GetI4BYT(&buf_p);
    rh_p->cpu_temperature = ValBuf_GetF4BYT(&buf_p);
    rh_p->pedestal_scan_type = ValBuf_GetI4BYT(&buf_p);
    if ( rh_p->pedestal_scan_type < 0
	    || rh_p->pedestal_scan_type > RAXPOL_PAUSED) {
	fprintf(stderr, "%d not allowed value for pedestal scan type\n",
		rh_p->pedestal_scan_type);
	return 0;
    }
    rh_p->tx_power = ValBuf_GetF4BYT(&buf_p);
    rh_p->osc_lock = ValBuf_GetI4BYT(&buf_p);
    buf_p += 4 * 4;			/* Skip reserved */
    rh_p->az = ValBuf_GetF8BYT(&buf_p);
    rh_p->el = ValBuf_GetF8BYT(&buf_p);
    rh_p->az_vel = ValBuf_GetF8BYT(&buf_p);
    rh_p->elev_vel = ValBuf_GetF8BYT(&buf_p);
    if ( !old_fmt ) {
	rh_p->az_current = ValBuf_GetF8BYT(&buf_p);
	rh_p->elev_current = ValBuf_GetF8BYT(&buf_p);
    }
    rh_p->sweep_count = ValBuf_GetI4BYT(&buf_p);
    rh_p->volume_count = ValBuf_GetI4BYT(&buf_p);
    rh_p->flags = ValBuf_GetI4BYT(&buf_p);
    rh_p->lat_ref = ValBuf_GetI4BYT(&buf_p);
    rh_p->lat = ValBuf_GetF8BYT(&buf_p);
    rh_p->lon_ref = ValBuf_GetI4BYT(&buf_p);
    rh_p->lon = ValBuf_GetF8BYT(&buf_p);
    rh_p->alt = ValBuf_GetF8BYT(&buf_p);
    rh_p->hdg_ref = ValBuf_GetI4BYT(&buf_p);
    rh_p->hdg = ValBuf_GetF8BYT(&buf_p);
    rh_p->speed = ValBuf_GetF8BYT(&buf_p);
    memcpy(rh_p->nmea_msg_gpgga, buf_p, sizeof(rh_p->nmea_msg_gpgga) - 1);
    buf_p += sizeof(rh_p->nmea_msg_gpgga);
    rh_p->utc_time_sec = ValBuf_GetI4BYT(&buf_p);
    rh_p->utc_time_usec = ValBuf_GetI4BYT(&buf_p);
    rh_p->data_type = ValBuf_GetI4BYT(&buf_p);
    rh_p->data_size = ValBuf_GetI4BYT(&buf_p);
    return 1;
}

/* Write contents of ray header at rh_p to standard output in native binary */
int RaXPol_FWrite_Ray_Hdr(struct RaXPol_Ray_Hdr *rh_p, FILE *out)
{
    static char *buf;			/* Output buffer */
    char *buf_p;			/* Pointer into buf */

    raxpol_ray_hdr_sz = old_fmt ? RAXPOL_RAY_HDR_SZ_OLD : RAXPOL_RAY_HDR_SZ_NEW;
    if ( !buf ) {
	if ( !(buf = MALLOC(raxpol_ray_hdr_sz)) ) {
	    fprintf(stderr, "Could not allocate %zu bytes for ray header "
		    "input buffer.\n", raxpol_ray_hdr_sz);
	    return 0;
	}
    }
    memset(buf, 0, raxpol_ray_hdr_sz);
    buf_p = buf;
    ValBuf_PutI4BYT(&buf_p, rh_p->timestamp_seconds);
    ValBuf_PutI4BYT(&buf_p, rh_p->timestamp_useconds);
    ValBuf_PutI4BYT(&buf_p, rh_p->radar_temperatures[0]);
    ValBuf_PutI4BYT(&buf_p, rh_p->radar_temperatures[1]);
    ValBuf_PutI4BYT(&buf_p, rh_p->radar_temperatures[2]);
    ValBuf_PutI4BYT(&buf_p, rh_p->radar_temperatures[3]);
    ValBuf_PutI4BYT(&buf_p, rh_p->inclinometer_roll);
    ValBuf_PutI4BYT(&buf_p, rh_p->inclinometer_pitch);
    ValBuf_PutI4BYT(&buf_p, rh_p->fuel_sensor);
    ValBuf_PutF4BYT(&buf_p, rh_p->cpu_temperature);
    ValBuf_PutI4BYT(&buf_p, rh_p->pedestal_scan_type);
    ValBuf_PutF4BYT(&buf_p, rh_p->tx_power);
    ValBuf_PutI4BYT(&buf_p, rh_p->osc_lock);
    buf_p += 4 * 4;			/* Skip reserved */
    ValBuf_PutF8BYT(&buf_p, rh_p->az);
    ValBuf_PutF8BYT(&buf_p, rh_p->el);
    ValBuf_PutF8BYT(&buf_p, rh_p->az_vel);
    ValBuf_PutF8BYT(&buf_p, rh_p->elev_vel);
    ValBuf_PutF8BYT(&buf_p, rh_p->az_current);
    ValBuf_PutF8BYT(&buf_p, rh_p->elev_current);
    ValBuf_PutI4BYT(&buf_p, rh_p->sweep_count);
    ValBuf_PutI4BYT(&buf_p, rh_p->volume_count);
    ValBuf_PutI4BYT(&buf_p, rh_p->flags);
    ValBuf_PutI4BYT(&buf_p, rh_p->lat_ref);
    ValBuf_PutF8BYT(&buf_p, rh_p->lat);
    ValBuf_PutI4BYT(&buf_p, rh_p->lon_ref);
    ValBuf_PutF8BYT(&buf_p, rh_p->lon);
    ValBuf_PutF8BYT(&buf_p, rh_p->alt);
    ValBuf_PutI4BYT(&buf_p, rh_p->hdg_ref);
    ValBuf_PutF8BYT(&buf_p, rh_p->hdg);
    ValBuf_PutF8BYT(&buf_p, rh_p->speed);
    ValBuf_PutBytes(&buf_p, rh_p->nmea_msg_gpgga, sizeof(rh_p->nmea_msg_gpgga));
    ValBuf_PutI4BYT(&buf_p, rh_p->utc_time_sec);
    ValBuf_PutI4BYT(&buf_p, rh_p->utc_time_usec);
    ValBuf_PutI4BYT(&buf_p, rh_p->data_type);
    ValBuf_PutI4BYT(&buf_p, rh_p->data_size);
    if ( fwrite(buf, raxpol_ray_hdr_sz, 1, out) != 1 ) {
	fprintf(stderr, "Could not write ray header\n%s\n", strerror(errno));
	return 0;
    }
    return 1;
}

/* Write contents of ray header at rh_p to out in ascii. */ 
int RaXPol_FPrint_Ray_Hdr(struct RaXPol_Ray_Hdr *rh_p, FILE *out)
{
    double jul0;			/* 1970/01/01 00:00:00 */
    double ray_time;			/* Ray time, Julian day */
    int yr, mon, day, hr, min;		/* Ray time, calendar values */
    double sec;

    jul0 = Tm_CalToJul(1970, 1, 1, 0, 0, 0);

    fprintf(out, "timestamp_seconds %d\n", rh_p->timestamp_seconds);
    fprintf(out, "timestamp_useconds %d\n", rh_p->timestamp_useconds);
    ray_time = jul0
	+ (rh_p->timestamp_seconds + 1.0e-6 * rh_p->timestamp_useconds)
	/ 86400.0;
    if ( Tm_JulToCal(ray_time, &yr, &mon, &day, &hr, &min, &sec) ) {
	fprintf(out, "timestamp_cal %04d/%02d/%02d %02d:%02d:%05.2f\n",
		yr, mon, day, hr, min, sec);
    } else {
	fprintf(stderr, "Could not convert ray time to calendar format.\n");
	return 0;
    }
    fprintf(out, "radar_temperatures %d %d %d %d\n",
	    rh_p->radar_temperatures[0], rh_p->radar_temperatures[1],
	    rh_p->radar_temperatures[2], rh_p->radar_temperatures[3]);
    fprintf(out, "inclinometer_roll %d\n", rh_p->inclinometer_roll);
    fprintf(out, "inclinometer_pitch %d\n", rh_p->inclinometer_pitch);
    fprintf(out, "fuel_sensor %d\n", rh_p->fuel_sensor);
    fprintf(out, "cpu_temperature %f\n", rh_p->cpu_temperature);
    if ( rh_p->pedestal_scan_type >= 0
	    && rh_p->pedestal_scan_type < RAXPOL_N_SCAN_TYPES) {
	fprintf(out, "pedestal_scan_type %d (%s)\n",
		rh_p->pedestal_scan_type,
		scan_type_descr[rh_p->pedestal_scan_type]);
    } else {
	fprintf(out, "pedestal_scan_type %d (unknown)\n",
		rh_p->pedestal_scan_type);
    }
    fprintf(out, "tx_power %f\n", rh_p->tx_power);
    fprintf(out, "osc_lock %d\n", rh_p->osc_lock);
    fprintf(out, "az %.3lf\n", rh_p->az);
    fprintf(out, "el %.3lf\n", rh_p->el);
    fprintf(out, "az_vel %lf\n", rh_p->az_vel);
    fprintf(out, "elev_vel %lf\n", rh_p->elev_vel);
    fprintf(out, "az_current %lf\n", rh_p->az_current);
    fprintf(out, "elev_current %lf\n", rh_p->elev_current);
    fprintf(out, "volume_count %d\n", rh_p->volume_count);
    fprintf(out, "sweep_count %d\n", rh_p->sweep_count);
    fprintf(out, "flags %d\n", rh_p->flags);
    fprintf(out, "lat_ref %c\n", rh_p->lat_ref);
    fprintf(out, "lat %lf\n", rh_p->lat);
    fprintf(out, "lon_ref %c\n", rh_p->lon_ref);
    fprintf(out, "lon %lf\n", rh_p->lon);
    fprintf(out, "alt %lf\n", rh_p->alt);
    fprintf(out, "hdg_ref %c\n", rh_p->hdg_ref);
    fprintf(out, "hdg %lf\n", rh_p->hdg);
    fprintf(out, "speed %lf\n", rh_p->speed);
    fprintf(out, "nmea_msg_gpgga %s\n", rh_p->nmea_msg_gpgga);
    fprintf(out, "utc_time_sec %d\n", rh_p->utc_time_sec);
    fprintf(out, "utc_time_usec %d\n", rh_p->utc_time_usec);
    fprintf(out, "data_type %d\n", rh_p->data_type);
    fprintf(out, "data_size %d\n", rh_p->data_size);
    return 1;
}

/* Print abbreviated ray header information */ 
void RaXPol_FPrint_Abbrv_Ray_Hdr(struct RaXPol_Ray_Hdr *rh_p, FILE *out)
{
    double jul0;			/* 1970/01/01 00:00:00 */
    double ray_tm;			/* Ray time, Julian day */
    int yr, mon, day, hr, min;		/* Ray time, calendar values */
    double sec;
    double dsec;			/* Time resolution */

    jul0 = Tm_CalToJul(1970, 1, 1, 0, 0, 0);
    dsec = 0.01;
    Tm_DSec(&dsec);
    ray_tm = jul0
	+ (rh_p->timestamp_seconds + 1.0e-6 * rh_p->timestamp_useconds)
	/ 86400.0;
    if ( Tm_JulToCal(ray_tm, &yr, &mon, &day, &hr, &min, &sec) ) {
	fprintf(out, " %04d/%02d/%02d %02d:%02d:%05.2f ",
		yr, mon, day, hr, min, sec);
    }
    fprintf(out, "lon %-10.5f ",
	    rh_p->lon * (rh_p->lon_ref == 'E' ? 1.0 : -1.0));
    fprintf(out, "lat %-9.5f ",
	    rh_p->lat * (rh_p->lat_ref == 'N' ? 1.0 : -1.0));
    fprintf(out, "az %-7.2lf ", ray_az(rh_p, 0));
    fprintf(out, "el %-6.2lf\n", rh_p->el);
}

/*
   Initialize data structure at dat_p. If in is NULL, dat_p is left
   bogus. If in points to a file, it must be at the start of a RaXPol
   file, at the start of the file header. Then process reads and stores
   the file header, initializes the rest of dat_p with appropriate values,
   and initializes the data arrays in dat_p. thres_val, cal_vv_val, cal_hh_val
   are set from RAXPOL_THRES_VAL, RAXPOL_CAL_HH_VAL, RAXPOL_CAL_VV_VAL
   environment variables if available, otherwise they are set to defaults.
   If RAXPOL_CAL_HH_VAL or RAXPOL_CAL_VV_VAL is "FILE_HEADER", value is taken
   from file header.
   
   Returns 1/0 on success/failure. Prints error messages to stderr on failure.
 */

int RaXPol_Init_Data(struct RaXPol_Data *dat_p, FILE *in)
{
    int num_gates;
    char *s;


    memset(dat_p, '\0', sizeof(struct RaXPol_Data));
    RaXPol_Init_File_Hdr(&dat_p->file_hdr);
    dat_p->servmode = RAXPOL_UNK;
    dat_p->v_noise = dat_p->h_noise = 0.0;
    dat_p->thres_val = NAN;
    dat_p->cal_vv_val = NAN;
    dat_p->cal_hh_val = NAN;
    RaXPol_Init_Ray_Hdr(&dat_p->ray_hdr);

    if ( !in ) {
	return 1;
    }

    /* Read and use file header */
    if ( !RaXPol_Read_File_Hdr(&dat_p->file_hdr, in) ) {
	fprintf(stderr, "Failed to read file header while initializing "
		"ray data structure.\n");
	return 0;
    }
    switch (dat_p->file_hdr.servmode) {
	case 0:
	    dat_p->servmode = (dat_p->file_hdr.sumpower)
		? RAXPOL_SPP_SUM_PWR : RAXPOL_SPP;
	    break;
	case 1:
	    dat_p->servmode = (dat_p->file_hdr.sumpower)
		? RAXPOL_DPP_SUM_PWR : RAXPOL_DPP;
	    break;
	case 2:
	    dat_p->servmode = RAXPOL_FFT;
	    break;
	case 3:
	    dat_p->servmode = RAXPOL_FFT2;
	    break;
	case 4:
	    dat_p->servmode = RAXPOL_FFT2I;
	    break;
	default:
	    fprintf(stderr, "Unknown server mode (%d) in file header\n",
		    dat_p->file_hdr.servmode);
	    return 0;
    }

    /* Allocate input fields */
    num_gates = dat_p->file_hdr.num_rng_gates;
    switch (dat_p->servmode) {
	case RAXPOL_SPP:
	    dat_p->dat_in.spp.zv1 = alloc_field_f("zv1", num_gates);
	    dat_p->dat_in.spp.zv2 = alloc_field_f("zv2", num_gates);
	    dat_p->dat_in.spp.zh1 = alloc_field_f("zh1", num_gates);
	    dat_p->dat_in.spp.zh2 = alloc_field_f("zh2", num_gates);
	    dat_p->dat_in.spp.pp_v = alloc_field_fc("pp_v", num_gates);
	    dat_p->dat_in.spp.pp_h = alloc_field_fc("pp_h", num_gates);
	    dat_p->dat_in.spp.cc = alloc_field_fc("cc", num_gates);
	    break;
	case RAXPOL_SPP_SUM_PWR:
	    dat_p->dat_in.spp_sum_pwr.zv = alloc_field_f("zv", num_gates);
	    dat_p->dat_in.spp_sum_pwr.zh = alloc_field_f("zh", num_gates);
	    dat_p->dat_in.spp_sum_pwr.pp_v = alloc_field_fc("pp_v", num_gates);
	    dat_p->dat_in.spp_sum_pwr.pp_h = alloc_field_fc("pp_h", num_gates);
	    dat_p->dat_in.spp_sum_pwr.cc = alloc_field_fc("cc", num_gates);
	    break;
	case RAXPOL_DPP:
	    dat_p->dat_in.dpp.zv1 = alloc_field_f("zv1", num_gates);
	    dat_p->dat_in.dpp.zv2 = alloc_field_f("zv2", num_gates);
	    dat_p->dat_in.dpp.zv3 = alloc_field_f("zv3", num_gates);
	    dat_p->dat_in.dpp.zh1 = alloc_field_f("zh1", num_gates);
	    dat_p->dat_in.dpp.zh2 = alloc_field_f("zh2", num_gates);
	    dat_p->dat_in.dpp.zh3 = alloc_field_f("zh3", num_gates);
	    dat_p->dat_in.dpp.pp_v1 = alloc_field_fc("pp_v1", num_gates);
	    dat_p->dat_in.dpp.pp_v2 = alloc_field_fc("pp_v2", num_gates);
	    dat_p->dat_in.dpp.pp_h1 = alloc_field_fc("pp_h1", num_gates);
	    dat_p->dat_in.dpp.pp_h2 = alloc_field_fc("pp_h2", num_gates);
	    dat_p->dat_in.dpp.cc = alloc_field_fc("cc", num_gates);
	    break;
	case RAXPOL_DPP_SUM_PWR:
	    dat_p->dat_in.dpp_sum_pwr.zv = alloc_field_f("zv", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.zh = alloc_field_f("zh", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.pp_v1
		= alloc_field_fc("pp_v1", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.pp_v2
		= alloc_field_fc("pp_v2", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.pp_h1
		= alloc_field_fc("pp_h1", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.pp_h2
		= alloc_field_fc("pp_h2", num_gates);
	    dat_p->dat_in.dpp_sum_pwr.cc = alloc_field_fc("cc", num_gates);
	    break;
	case RAXPOL_FFT:
	    fprintf(stderr, "Cannot process FFT server mode.\n");
	    return 0;
	case RAXPOL_FFT2:
	    fprintf(stderr, "Cannot process FFT2 server mode.\n");
	    return 0;
	case RAXPOL_FFT2I:
	    fprintf(stderr, "Cannot process FFT2I server mode.\n");
	    return 0;
	case RAXPOL_UNK:
	    fprintf(stderr, "Unknown RaXPol server mode.\n");
	    return 0;
    }

    /* Set "global" values. See gui_1.pro */
    if ( (s = getenv("RAXPOL_THRES_VAL")) ) {
	if ( sscanf(s, "%lf", &dat_p->thres_val) != 1 ) {
	    fprintf(stderr, "Expected float value in RAXPOL_THRES_VAL, "
		    "got %s", s);
	    exit(EXIT_FAILURE);
	}
    } else {
	dat_p->thres_val = 3.0;
    }
    if ( (s = getenv("RAXPOL_CAL_HH_VAL")) ) {
	if ( strcmp(s, "FILE_HEADER") == 0 ) {
	    dat_p->cal_hh_val = dat_p->file_hdr.H_dBZ_dBm_offset;
	} else if ( sscanf(s, "%lf", &dat_p->cal_hh_val) != 1 ) {
	    fprintf(stderr, "Expected float value in RAXPOL_CAL_HH_VAL, "
		    "got %s", s);
	    exit(EXIT_FAILURE);
	}
    } else {
	dat_p->cal_hh_val = 68.8;
    }
    if ( (s = getenv("RAXPOL_CAL_VV_VAL")) ) {
	if ( strcmp(s, "FILE_HEADER") == 0 ) {
	    dat_p->cal_hh_val = dat_p->file_hdr.V_dBZ_dBm_offset;
	} else if ( sscanf(s, "%lf", &dat_p->cal_vv_val) != 1 ) {
	    fprintf(stderr, "Expected float value in RAXPOL_CAL_VV_VAL, "
		    "got %s", s);
	    exit(EXIT_FAILURE);
	}
    } else {
	dat_p->cal_vv_val = 68.8;
    }

    /* Assign methods to read a ray and compute output moments */
    switch (dat_p->servmode) {
	case RAXPOL_SPP:
	    dat_p->read_ray = read_spp_ray;
	    dat_p->dbmhc = spp_dbmhc;
	    dat_p->dbmvc = spp_dbmvc;
	    dat_p->dbz = spp_dbz;
	    dat_p->dbz1 = spp_dbz1;
	    dat_p->vel = spp_vel;
	    dat_p->zdr = spp_zdr;
	    dat_p->phidp = spp_phidp;
	    dat_p->rhohv = spp_rhohv;
	    dat_p->std = spp_std;
	    dat_p->snrhc = spp_snrhc;
	    dat_p->snrvc = spp_snrvc;
	    break;
	case RAXPOL_SPP_SUM_PWR:
	    dat_p->read_ray = read_spp_sum_pwr_ray;
	    dat_p->dbmhc = spp_sum_pwr_dbmhc;
	    dat_p->dbmvc = spp_sum_pwr_dbmvc;
	    dat_p->dbz = spp_sum_pwr_dbz;
	    dat_p->dbz1 = spp_sum_pwr_dbz1;
	    dat_p->vel = spp_sum_pwr_vel;
	    dat_p->zdr = spp_sum_pwr_zdr;
	    dat_p->phidp = spp_sum_pwr_phidp;
	    dat_p->rhohv = spp_sum_pwr_rhohv;
	    dat_p->std = spp_sum_pwr_std;
	    dat_p->snrhc = spp_sum_pwr_snrhc;
	    dat_p->snrvc = spp_sum_pwr_snrvc;
	    break;
	case RAXPOL_DPP:
	    dat_p->read_ray = read_dpp_ray;
	    dat_p->dbmhc = dpp_dbmhc;
	    dat_p->dbmvc = dpp_dbmvc;
	    dat_p->dbz = dpp_dbz;
	    dat_p->dbz1 = dpp_dbz1;
	    dat_p->vel = dpp_vel;
	    dat_p->zdr = dpp_zdr;
	    dat_p->phidp = dpp_phidp;
	    dat_p->rhohv = dpp_rhohv;
	    dat_p->std = dpp_std;
	    dat_p->snrhc = dpp_snrhc;
	    dat_p->snrvc = dpp_snrvc;
	    break;
	case RAXPOL_DPP_SUM_PWR:
	    dat_p->read_ray = read_dpp_sum_pwr_ray;
	    dat_p->dbmhc = dpp_sum_pwr_dbmhc;
	    dat_p->dbmvc = dpp_sum_pwr_dbmvc;
	    dat_p->dbz = dpp_sum_pwr_dbz;
	    dat_p->dbz1 = dpp_sum_pwr_dbz1;
	    dat_p->vel = dpp_sum_pwr_vel;
	    dat_p->zdr = dpp_sum_pwr_zdr;
	    dat_p->phidp = dpp_sum_pwr_phidp;
	    dat_p->rhohv = dpp_sum_pwr_rhohv;
	    dat_p->std = dpp_sum_pwr_std;
	    dat_p->snrhc = dpp_sum_pwr_snrhc;
	    dat_p->snrvc = dpp_sum_pwr_snrvc;
	    break;
	case RAXPOL_FFT:
	case RAXPOL_FFT2:
	case RAXPOL_FFT2I:
	case RAXPOL_UNK:
	    dat_p->read_ray = no_in_stub;
	    dat_p->dbmhc = dat_p->dbmvc = dat_p->dbz = dat_p->dbz1
		= dat_p->vel = dat_p->zdr = dat_p->phidp = dat_p->rhohv
		= dat_p->std = dat_p->snrhc = dat_p->snrvc = no_out_stub;
	    break;
    }
    return 1;
}

/*
   Allocate memory for an output field named nm with space for n floats
   Exit process on failure.
 */ 

static float *alloc_field_f(char *nm, size_t n)
{
    float *dat;

    if ( !(dat = calloc(n, sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %u values for %s field\n",
		(unsigned)n, nm);
	exit(EXIT_FAILURE);
    }
    return dat;
}

/*
   Allocate memory for an output field named nm with space for n complext
   floats. Exit process on failure.
 */ 

static float _Complex *alloc_field_fc(char *nm, size_t n)
{
    float _Complex *dat;

    if ( !(dat = calloc(n, sizeof(float _Complex))) ) {
	fprintf(stderr, "Could not allocate %u values for %s field\n",
		(unsigned)n, nm);
	exit(EXIT_FAILURE);
    }
    return dat;
}

/*
   Return string representation of a scan type.
   Caller should not free return value or modify contents.
 */

const char *RaXPol_Scan_Type_Descr(enum RAXPOL_SCAN_TYPE scan_type)
{
    return scan_type_descr[scan_type];
}

/*
   Read data from stream in into dat_p.
   dat_p should have been initialized with a call to RaXPol_Init_Data.
   in must be at the start of the ray header for a ray.
   Return 1/0 on success/failure.
 */

int RaXPol_Read_Ray(struct RaXPol_Data *dat_p, FILE *in)
{
    return dat_p->read_ray(dat_p, in);
}

static int no_in_stub(struct RaXPol_Data *dat_p, FILE *in)
{
    fprintf(stderr, "Cannot compute read %s server mode.\n",
	    servmode_s[dat_p->servmode]);
    return 0;
}

/*
   Read single pulse pair ray from in into dat_p. dat_p must be initialized
   and arrays for all input fields must be allocated.

   dat_p->v_noise and dat_p->h_noise are updated.

   Return value is 1/0 on success/failure.
   If input fails, error and eof flags are retained in, and caller must
   eventually call clearerr to restore in.

   This function stupidly assumes float and float _Complex types are the
   same size and encoding on all platforms.
 */

static int read_spp_ray(struct RaXPol_Data *dat_p, FILE *in)
{
    size_t sz;
    double w = 0.1;			/* Weighting factor for noise
					   computation. See RaXpol_disp1.pro */
    struct RaXPol_SPP spp = dat_p->dat_in.spp;
    int num_gates = dat_p->file_hdr.num_rng_gates;
    float *zv1 = spp.zv1;
    float *zv2 = spp.zv2;
    float *zh1 = spp.zh1;
    float *zh2 = spp.zh2;
    float _Complex *pp_v = spp.pp_v;
    float _Complex *pp_h = spp.pp_h;
    float _Complex *cc = spp.cc;
    static float v_noise, h_noise;

    if ( !RaXPol_Read_Ray_Hdr(&dat_p->ray_hdr, in) ) {
	return 0;
    }
    sz = num_gates * sizeof(float);
    if ( fread(zv1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zv1\n");
	}
	return 0;
    }
    if ( fread(zv2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zv2\n");
	}
	return 0;
    }
    if ( fread(zh1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zh1\n");
	}
	return 0;
    }
    if ( fread(zh2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zh2\n");
	}
	return 0;
    }
    sz = num_gates * sizeof(float _Complex);
    if ( fread(pp_v, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v\n");
	}
	return 0;
    }
    if ( fread(pp_h, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h\n");
	}
	return 0;
    }
    if ( fread(cc, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read cc\n");
	}
	return 0;
    }

    /* Update noise */
    if ( v_noise == 0.0 ) {
	v_noise = mean(zv1, 10);
    }
    v_noise = dat_p->v_noise = (1.0 - w) * v_noise + w * mean(zv1, 10);
    if ( h_noise == 0.0 ) {
	h_noise = mean(zh1, 10);
    }
    h_noise = dat_p->h_noise = (1.0 - w) * h_noise + w * mean(zh1, 2);

    return 1;
}

/*
   Read one pulse pair sum power ray from in.  dat_p must be initialized
   and arrays for all input fields must be allocated.

   dat_p->v_noise and dat_p->h_noise are updated.

   Return value is 1/0 on success/failure. If input fails, error and eof flags
   are retained, and caller must eventually call clearerr to restore in.

   This function stupidly assumes float and float _Complex types are the
   same size and encoding on all platforms.
 */

static int read_spp_sum_pwr_ray(struct RaXPol_Data *dat_p, FILE *in)
{
    size_t sz;
    double w = 0.1;			/* Weighting factor for noise
					   computation. See RaXpol_disp1.pro */
    int num_gates = dat_p->file_hdr.num_rng_gates;
    struct RaXPol_SPP_SumPwr spp_sum_pwr = dat_p->dat_in.spp_sum_pwr;
    float *zv = spp_sum_pwr.zv;
    float *zh = spp_sum_pwr.zh;
    float _Complex *pp_v = spp_sum_pwr.pp_v;
    float _Complex *pp_h = spp_sum_pwr.pp_h;
    float _Complex *cc = spp_sum_pwr.cc;
    static float v_noise, h_noise;

    if ( !RaXPol_Read_Ray_Hdr(&dat_p->ray_hdr, in) ) {
	return 0;
    }
    sz = num_gates * sizeof(float);
    if ( fread(zv, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read v\n");
	}
	return 0;
    }
    if ( fread(zh, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read h\n");
	}
	return 0;
    }
    sz = num_gates * sizeof(float _Complex);
    if ( fread(pp_v, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v\n");
	}
	return 0;
    }
    if ( fread(pp_h, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h\n");
	}
	return 0;
    }
    if ( fread(cc, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read cc\n");
	}
	return 0;
    }

    /* Update noise */
    if ( v_noise == 0.0 ) {
	v_noise = mean(zv, 10);
    }
    v_noise = dat_p->v_noise = (1.0 - w) * v_noise + w * mean(zv, 10);
    if ( h_noise == 0.0 ) {
	h_noise = mean(zh, 10);
    }
    h_noise = dat_p->h_noise = (1.0 - w) * h_noise + w * mean(zh, 10);
    return 1;
}

/*
   Read dual pulse pair ray from in into dat_p. dat_p must be initialized
   and arrays for all input fields must be allocated.

   dat_p->v_noise and dat_p->h_noise are calculated.

   Return value is 1/0 on success/failure.
   If input fails, error and eof flags are retained in, and caller must
   eventually call clearerr to restore in.

   This function stupidly assumes float and float _Complex types are the
   same size and encoding on all platforms.
 */

static int read_dpp_ray(struct RaXPol_Data *dat_p, FILE *in)
{
    size_t sz;
    double w = 0.1;			/* Weighting factor for noise
					   computation. See RaXpol_disp1.pro */
    int num_gates = dat_p->file_hdr.num_rng_gates;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;
    float *zv1 = dpp.zv1;
    float *zv2 = dpp.zv2;
    float *zv3 = dpp.zv3;
    float *zh1 = dpp.zh1;
    float *zh2 = dpp.zh2;
    float *zh3 = dpp.zh3;
    float _Complex *pp_v1 = dpp.pp_v1;
    float _Complex *pp_v2 = dpp.pp_v2;
    float _Complex *pp_h1 = dpp.pp_h1;
    float _Complex *pp_h2 = dpp.pp_h2;
    float _Complex *cc = dpp.cc;
    static float v_noise, h_noise;

    if ( !RaXPol_Read_Ray_Hdr(&dat_p->ray_hdr, in) ) {
	return 0;
    }
    sz = num_gates * sizeof(float);
    if ( fread(zv1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zv1\n");
	}
	return 0;
    }
    if ( fread(zv2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zv2\n");
	}
	return 0;
    }
    if ( fread(zv3, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zv3\n");
	}
	return 0;
    }
    if ( fread(zh1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zh1\n");
	}
	return 0;
    }
    if ( fread(zh2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zh2\n");
	}
	return 0;
    }
    if ( fread(zh3, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read zh3\n");
	}
	return 0;
    }
    sz = num_gates * sizeof(float _Complex);
    if ( fread(pp_v1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v1\n");
	}
	return 0;
    }
    if ( fread(pp_v2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v2\n");
	}
	return 0;
    }
    if ( fread(pp_h1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h1\n");
	}
	return 0;
    }
    if ( fread(pp_h2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h2\n");
	}
	return 0;
    }
    if ( fread(cc, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read cc\n");
	}
	return 0;
    }

    /* Update noise */
    if ( v_noise == 0.0 ) {
	v_noise = mean(zv1, 10);
    }
    v_noise = dat_p->v_noise = (1.0 - w) * v_noise + w * mean(zv1, 10);
    if ( h_noise == 0.0 ) {
	h_noise = mean(zh1, 10);
    }
    h_noise = dat_p->h_noise = (1.0 - w) * h_noise + w * mean(zh1, 2);

    return 1;
}

/*
   Read one pulse pair sum power ray from in.  dat_p must be initialized
   and arrays for all input fields must be allocated.

   dat_p->v_noise and dat_p->h_noise are calculated.

   Return value is 1/0 on success/failure. If input fails, error and eof flags
   are retained, and caller must eventually call clearerr to restore in.

   This function stupidly assumes float and float _Complex types are the
   same size and encoding on all platforms.
 */

static int read_dpp_sum_pwr_ray(struct RaXPol_Data *dat_p, FILE *in)
{
    size_t sz;
    double w = 0.1;			/* Weighting factor for noise
					   computation. See RaXpol_disp1.pro */
    int num_gates = dat_p->file_hdr.num_rng_gates;
    struct RaXPol_DPP_SumPwr dpp_sum_pwr = dat_p->dat_in.dpp_sum_pwr;
    float *zv = dpp_sum_pwr.zv;
    float *zh = dpp_sum_pwr.zh;
    float _Complex *pp_v1 = dpp_sum_pwr.pp_v1;
    float _Complex *pp_v2 = dpp_sum_pwr.pp_v2;
    float _Complex *pp_h1 = dpp_sum_pwr.pp_h1;
    float _Complex *pp_h2 = dpp_sum_pwr.pp_h2;
    float _Complex *cc = dpp_sum_pwr.cc;
    static float v_noise, h_noise;

    if ( !RaXPol_Read_Ray_Hdr(&dat_p->ray_hdr, in) ) {
	return 0;
    }
    sz = num_gates * sizeof(float);
    if ( fread(zv, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read v\n");
	}
	return 0;
    }
    if ( fread(zh, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read h\n");
	}
	return 0;
    }
    sz = num_gates * sizeof(float _Complex);
    if ( fread(pp_v1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v1\n");
	}
	return 0;
    }
    if ( fread(pp_v2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_v2\n");
	}
	return 0;
    }
    if ( fread(pp_h1, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h1\n");
	}
	return 0;
    }
    if ( fread(pp_h2, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read pp_h2\n");
	}
	return 0;
    }
    if ( fread(cc, sz, 1, in) != 1 ) {
	if ( ferror(in) ) {
	    fprintf(stderr, "Failed to read cc\n");
	}
	return 0;
    }

    /* Update noise */
    if ( v_noise == 0.0 ) {
	v_noise = mean(zv, 10);
    }
    v_noise = dat_p->v_noise = (1.0 - w) * v_noise + w * mean(zv, 10);
    if ( h_noise == 0.0 ) {
	h_noise = mean(zh, 10);
    }
    h_noise = dat_p->h_noise = (1.0 - w) * h_noise + w * mean(zh, 10);
    return 1;
}

/*
   The following functions compute output moments for ray data in RaXPol_Data
   structures.
 */

int no_out_stub(struct RaXPol_Data *dat_p, float *dbmhc)
{
    fprintf(stderr, "Cannot compute output moments for %s server mode.\n",
	    servmode_s[dat_p->servmode]);
    return 0;
}

/* Calculate DBMHC and DBMVC for various server modes */
static int pp_dbm(struct RaXPol_Data *dat_p, float *z, float *dbm)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double min = 1.0e-20;		/* Minimum value */

    if ( !dat_p || !dbm ) {
	fprintf(stderr, "Attempted to compute DBMHC for bogus data set.\n");
	return 0;
    }
    g0 = floor(dat_p->file_hdr.zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    for (g = 0; g < g0; g++) {
	dbm[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	dbm[g] = 10.0 * log10(z[g]);
	if ( dbm[g] < min ) {
	    dbm[g] = min;
	}
    }
    return 1;
}
static int spp_dbmvc(struct RaXPol_Data *dat_p, float *dbmvc)
{
    return pp_dbm(dat_p, dat_p->dat_in.spp.zv1, dbmvc);
}
static int spp_sum_pwr_dbmvc(struct RaXPol_Data *dat_p, float *dbmvc)
{
    return pp_dbm(dat_p, dat_p->dat_in.spp_sum_pwr.zv, dbmvc);
}
static int spp_dbmhc(struct RaXPol_Data *dat_p, float *dbmhc)
{
    return pp_dbm(dat_p, dat_p->dat_in.spp.zh1, dbmhc);
}
static int spp_sum_pwr_dbmhc(struct RaXPol_Data *dat_p, float *dbmhc)
{
    return pp_dbm(dat_p, dat_p->dat_in.spp_sum_pwr.zh, dbmhc);
}
static int dpp_dbmvc(struct RaXPol_Data *dat_p, float *dbmvc)
{
    return pp_dbm(dat_p, dat_p->dat_in.dpp.zv1, dbmvc);
}
static int dpp_sum_pwr_dbmvc(struct RaXPol_Data *dat_p, float *dbmvc)
{
    return pp_dbm(dat_p, dat_p->dat_in.dpp_sum_pwr.zv, dbmvc);
}
static int dpp_dbmhc(struct RaXPol_Data *dat_p, float *dbmhc)
{
    return pp_dbm(dat_p, dat_p->dat_in.dpp.zh1, dbmhc);
}
static int dpp_sum_pwr_dbmhc(struct RaXPol_Data *dat_p, float *dbmhc)
{
    return pp_dbm(dat_p, dat_p->dat_in.dpp_sum_pwr.zh, dbmhc);
}

/* Calculate reflectivity. Assume read_*_ray updated the noise members. */
static int pp_dbz(struct RaXPol_Data *dat_p, float *zh, float *dbz)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    double dr;				/* Range gate spacing */
    double r;				/* Distance along ray */
    double r_min = 1.0e-1;		/* Minimum r value, from gui_1.pro */
    double thres_val, cave;
    double postave;
    double thres = INFINITY;
    double rres;			/* From dat_p->file_hdr */

    if ( !dat_p || !dbz ) {
	fprintf(stderr, "Attempted to compute DBZ for bogus data set.\n");
	return 0;
    }
    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    dr = dat_p->file_hdr.range_gate_spacing;
    thres_val = dat_p->thres_val;
    cave = dat_p->file_hdr.clutter_avg_intvl;
    postave = dat_p->file_hdr.post_averaging_interval;
    thres = dat_p->h_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    if ( dat_p->file_hdr.sumpower ) {
	thres /= sqrt(2.0);
    }
    rres = dat_p->file_hdr.range_resolution;
    for (g = 0; g < g0; g++) {
	dbz[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	double z = zh[g] - dat_p->h_noise;
	z = (z < thres) ? NAN : z;
	r = 0.001 * dr * (g - zero_range_gate_index);
	r = (r < r_min) ? r_min : r;
	dbz[g] = 10.0 * log10(z) + 20.0 * log10(r) - 10.0 * log10(rres)
	    + dat_p->cal_hh_val;
    }
    return 1;
}
static int spp_dbz(struct RaXPol_Data *dat_p, float *dbz)
{
    int g, num_gates;
    float *zh1, *zh2;
    static float *zh;
    float *z_tmp;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zh1 = spp.zh1;
    zh2 = spp.zh2;
    for (g = 0; g < num_gates; g++) {
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    return pp_dbz(dat_p, zh, dbz);
}
static int dpp_dbz(struct RaXPol_Data *dat_p, float *dbz)
{
    int g, num_gates;
    float *zh1, *zh2, *zh3;
    static float *zh;
    float *z_tmp;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zh1 = dpp.zh1;
    zh2 = dpp.zh2;
    zh3 = dpp.zh3;
    for (g = 0; g < num_gates; g++) {
	zh[g] = (zh1[g] + zh2[g] + zh3[g]) / 3.0;
    }
    return pp_dbz(dat_p, zh, dbz);
}
static int spp_sum_pwr_dbz(struct RaXPol_Data *dat_p, float *dbz)
{
    return pp_dbz(dat_p, dat_p->dat_in.spp_sum_pwr.zh, dbz);
}
static int dpp_sum_pwr_dbz(struct RaXPol_Data *dat_p, float *dbz)
{
    return pp_dbz(dat_p, dat_p->dat_in.dpp_sum_pwr.zh, dbz);
}
static int spp_dbz1(struct RaXPol_Data *dat_p, float *dbz1)
{
    return pp_dbz(dat_p, dat_p->dat_in.spp.zh1, dbz1);
}
static int dpp_dbz1(struct RaXPol_Data *dat_p, float *dbz1)
{
    return pp_dbz(dat_p, dat_p->dat_in.dpp.zh1, dbz1);
}
static int spp_sum_pwr_dbz1(struct RaXPol_Data *dat_p, float *dbz1)
{
    return pp_dbz(dat_p, dat_p->dat_in.spp_sum_pwr.zh, dbz1);
}
static int dpp_sum_pwr_dbz1(struct RaXPol_Data *dat_p, float *dbz1)
{
    return pp_dbz(dat_p, dat_p->dat_in.dpp_sum_pwr.zh, dbz1);
}

/* Calculate velocity for single pulse pair */ 
static int pp_vel(struct RaXPol_Data *dat_p, float _Complex *pp, int pri,
	float *vel)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    double c = 2.9979e8;		/* Speed of light, m/s */
    double l = c / RAXPOL_FREQUENCY;	/* Wavelength */

    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    for (g = 0; g < g0; g++) {
	vel[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	vel[g] = -l * cargf(pp[g]) / (4.0e-6 * pri * M_PI);
    }
    return 1;
}
static int spp_vel(struct RaXPol_Data *dat_p, float *vel)
{
    int g;
    int num_gates;
    float _Complex *pp_v, *pp_h;
    static float _Complex *pp;		/* Receive average pp */
    float _Complex *pp_tmp;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(pp_tmp = REALLOC(pp, num_gates * sizeof(float _Complex))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    pp = pp_tmp;
    pp_v = spp.pp_v;
    pp_h = spp.pp_h;
    for (g = 0; g < num_gates; g++) {
	pp[g] = (pp_v[g] + pp_h[g]) / 2.0;
    }
    return pp_vel(dat_p, pp, dat_p->file_hdr.pri1, vel);
}
static int spp_sum_pwr_vel(struct RaXPol_Data *dat_p, float *vel)
{
    int g;
    int num_gates;
    float _Complex *pp_v;
    float _Complex *pp_h;
    static float _Complex *pp;		/* Receive average pp */
    float _Complex *pp_tmp;
    struct RaXPol_SPP_SumPwr spp_sum_pwr = dat_p->dat_in.spp_sum_pwr;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(pp_tmp = REALLOC(pp, num_gates * sizeof(float _Complex))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    pp = pp_tmp;
    pp_v = spp_sum_pwr.pp_v;
    pp_h = spp_sum_pwr.pp_h;
    for (g = 0; g < num_gates; g++) {
	pp[g] = (pp_v[g] + pp_h[g]) / 2.0;
    }
    return pp_vel(dat_p, pp, dat_p->file_hdr.pri1, vel);
}

/* Calculate velocity for dual pulse pair */
static int dpp_vel(struct RaXPol_Data *dat_p, float *vel)
{
    int g;
    int num_gates;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;
    float _Complex *pp_v1, *pp_v2, *pp_h1, *pp_h2;
    float _Complex pp1_ave, pp2_ave;
    static float _Complex *pp;		/* Receive average pp */
    float _Complex *pp_tmp;
    int pri1, pri2;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(pp_tmp = REALLOC(pp, num_gates * sizeof(float _Complex))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    pp = pp_tmp;
    pp_v1 = dpp.pp_v1;
    pp_v2 = dpp.pp_v2;
    pp_h1 = dpp.pp_h1;
    pp_h2 = dpp.pp_h2;
    pri1 = dat_p->file_hdr.pri1;
    pri2 = dat_p->file_hdr.pri2;
    for (g = 0; g < num_gates; g++) {
	pp1_ave = (pp_v1[g] + pp_h1[g]) / 2.0;
	pp2_ave = (pp_v2[g] + pp_h2[g]) / 2.0;

	/* Nonsense from gui_1.pro */
	if ( pri1 > pri2 ) {
	    pp[g] = pp1_ave * conjf(pp2_ave);
	} else {
	    pp[g] = pp1_ave * conjf(pp2_ave);
	}
    }
    return pp_vel(dat_p, pp, pri2 - pri1, vel);
}
static int dpp_sum_pwr_vel(struct RaXPol_Data *dat_p, float *vel)
{
    int g;
    int num_gates;
    float _Complex *pp_v1, *pp_v2, *pp_h1, *pp_h2;
    float _Complex pp1_ave, pp2_ave;
    static float _Complex *pp;		/* Receive average pp */
    float _Complex *pp_tmp;
    int pri1, pri2;
    struct RaXPol_DPP_SumPwr dpp_sum_pwr = dat_p->dat_in.dpp_sum_pwr;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(pp_tmp = REALLOC(pp, num_gates * sizeof(float _Complex))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    pp = pp_tmp;
    pp_v1 = dpp_sum_pwr.pp_v1;
    pp_v2 = dpp_sum_pwr.pp_v2;
    pp_h1 = dpp_sum_pwr.pp_h1;
    pp_h2 = dpp_sum_pwr.pp_h2;
    pri1 = dat_p->file_hdr.pri1;
    pri2 = dat_p->file_hdr.pri2;
    for (g = 0; g < num_gates; g++) {
	pp1_ave = (pp_v1[g] + pp_h1[g]) / 2.0;
	pp2_ave = (pp_v2[g] + pp_h2[g]) / 2.0;

	/* Nonsense from gui_1.pro */
	if ( pri1 > pri2 ) {
	    pp[g] = pp1_ave * conjf(pp2_ave);
	} else {
	    pp[g] = pp1_ave * conjf(pp2_ave);
	}
    }
    return pp_vel(dat_p, pp, pri2 - pri1, vel);
}

/* Calculate zdr. Assume read_*_ray updated the noise members. */
static int pp_zdr(struct RaXPol_Data *dat_p, float *zv, float *zh, float *zdr)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    double zv_, zh_;
    double v_noise, h_noise;		/* Power noise, should have been
					   updated when ray was read in */
    double thres_val, cave, postave,
	   cal_vv_val, cal_hh_val;	/* dat_p->file_hdr members */
    double thres_h, thres_v;		/* Power threshold */

    if ( !dat_p || !zdr ) {
	fprintf(stderr, "Attempted to compute DBZ for bogus data set.\n");
	return 0;
    }
    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    v_noise = dat_p->v_noise;
    h_noise = dat_p->h_noise;
    thres_val = dat_p->thres_val;
    cal_vv_val = dat_p->cal_vv_val;
    cal_hh_val = dat_p->cal_hh_val;
    cave = dat_p->file_hdr.clutter_avg_intvl;
    postave = dat_p->file_hdr.post_averaging_interval;
    thres_v = v_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    thres_h = h_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    if ( dat_p->file_hdr.sumpower ) {
	thres_v /= sqrt(2.0);
	thres_h /= sqrt(2.0);
    }

    for (g = 0; g < g0; g++) {
	zdr[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	zv_ = zv[g] - v_noise;
	zv_ = (zv_ < thres_v) ? NAN : zv_;
	zh_ = zh[g] - h_noise;
	zh_ = (zh_ < thres_h) ? NAN : zh_;
	zdr[g] = 10.0 * (log10(zh_) + cal_hh_val - log10(zv_) - cal_vv_val);
    }
    return 1;
}
static int spp_zdr(struct RaXPol_Data *dat_p, float *zdr)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zh1, *zh2;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = spp.zv1;
    zv2 = spp.zv2;
    zh1 = spp.zh1;
    zh2 = spp.zh2;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g]) / 2.0;
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    return pp_zdr(dat_p, zv, zh, zdr);
}
static int spp_sum_pwr_zdr(struct RaXPol_Data *dat_p, float *zdr)
{
    float *zv = dat_p->dat_in.spp_sum_pwr.zv;
    float *zh = dat_p->dat_in.spp_sum_pwr.zh;
    return pp_zdr(dat_p, zv, zh, zdr);
}
static int dpp_zdr(struct RaXPol_Data *dat_p, float *zdr)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zv3, *zh1, *zh2, *zh3;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = dpp.zv1;
    zv2 = dpp.zv2;
    zv3 = dpp.zv3;
    zh1 = dpp.zh1;
    zh2 = dpp.zh2;
    zh3 = dpp.zh3;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g] + zv3[g]) / 3.0;
	zh[g] = (zh1[g] + zh2[g] + zh3[g]) / 3.0;
    }
    return pp_zdr(dat_p, zv, zh, zdr);
}

static int dpp_sum_pwr_zdr(struct RaXPol_Data *dat_p, float *zdr)
{
    float *zv = dat_p->dat_in.dpp_sum_pwr.zv;
    float *zh = dat_p->dat_in.dpp_sum_pwr.zh;
    return pp_zdr(dat_p, zv, zh, zdr);
}

/* Calculate pp_phidp */
static int pp_phidp(struct RaXPol_Data *dat_p, float _Complex *cc,
	float *phidp)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;

    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    for (g = 0; g < g0; g++) {
	phidp[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	phidp[g] = -180.0 * cargf(cc[g]) / M_PI;
    }
    return 1;
}

static int spp_phidp(struct RaXPol_Data *dat_p, float *phidp)
{
    float _Complex *cc = dat_p->dat_in.spp.cc;
    return pp_phidp(dat_p, cc, phidp);
}
static int spp_sum_pwr_phidp(struct RaXPol_Data *dat_p, float *phidp)
{
    float _Complex *cc = dat_p->dat_in.spp_sum_pwr.cc;
    return pp_phidp(dat_p, cc, phidp);
}
static int dpp_phidp(struct RaXPol_Data *dat_p, float *phidp)
{
    float _Complex *cc = dat_p->dat_in.dpp.cc;
    return pp_phidp(dat_p, cc, phidp);
}
static int dpp_sum_pwr_phidp(struct RaXPol_Data *dat_p, float *phidp)
{
    float _Complex *cc = dat_p->dat_in.dpp_sum_pwr.cc;
    return pp_phidp(dat_p, cc, phidp);
}

/* Calculate RhoHV */
static int pp_rhohv(struct RaXPol_Data *dat_p, float *zv, float *zh,
	float _Complex *cc, float *rhohv)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    double zh_, zv_;
    double v_noise, h_noise;		/* Power noise, should have been
					   updated when ray was read in */
    double thres_val, cave, postave;	/* dat_p->file_hdr members */
    double thres_h, thres_v;		/* Power threshold */

    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    v_noise = dat_p->v_noise;
    h_noise = dat_p->h_noise;
    cave = dat_p->file_hdr.clutter_avg_intvl;
    postave = dat_p->file_hdr.post_averaging_interval;
    thres_val = dat_p->thres_val;
    thres_v = v_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    thres_h = h_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    if ( dat_p->file_hdr.sumpower ) {
	thres_v /= sqrt(2.0);
	thres_h /= sqrt(2.0);
    }
    for (g = 0; g < g0; g++) {
	rhohv[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	zv_ = zv[g] - v_noise;
	zv_ = (zv_ < thres_v) ? thres_v : zv_;
	zh_ = zh[g] - h_noise;
	zh_ = (zh_ < thres_h) ? thres_h : zh_;
	rhohv[g] = cabs(cc[g]) / sqrt(zh_ * zv_);
	if ( zh_ == thres_h || zv_ == thres_v ) {
	    rhohv[g] = NAN;
	}
    }

    return 1;
}
static int spp_rhohv(struct RaXPol_Data *dat_p, float *rhohv)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zh1, *zh2;
    float _Complex *cc;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = spp.zv1;
    zv2 = spp.zv2;
    zh1 = spp.zh1;
    zh2 = spp.zh2;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g]) / 2.0;
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    cc = spp.cc;
    return pp_rhohv(dat_p, zv, zh, cc, rhohv);
}
static int spp_sum_pwr_rhohv(struct RaXPol_Data *dat_p, float *rhohv)
{
    float *zv = dat_p->dat_in.spp_sum_pwr.zv;
    float *zh = dat_p->dat_in.spp_sum_pwr.zh;
    float _Complex *cc = dat_p->dat_in.spp_sum_pwr.cc;
    return pp_rhohv(dat_p, zv, zh, cc, rhohv);
}
static int dpp_rhohv(struct RaXPol_Data *dat_p, float *rhohv)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zv3, *zh1, *zh2, *zh3;
    float _Complex *cc;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = dpp.zv1;
    zv2 = dpp.zv2;
    zv3 = dpp.zv3;
    zh1 = dpp.zh1;
    zh2 = dpp.zh2;
    zh3 = dpp.zh3;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g] + zv3[g]) / 3.0;
	zh[g] = (zh1[g] + zh2[g] + zh3[g]) / 3.0;
    }
    cc = dpp.cc;
    return pp_rhohv(dat_p, zv, zh, cc, rhohv);
}
static int dpp_sum_pwr_rhohv(struct RaXPol_Data *dat_p, float *rhohv)
{
    float *zv = dat_p->dat_in.dpp_sum_pwr.zv;
    float *zh = dat_p->dat_in.dpp_sum_pwr.zh;
    float _Complex *cc = dat_p->dat_in.dpp_sum_pwr.cc;
    return pp_rhohv(dat_p, zv, zh, cc, rhohv);
}

/* Calculate standard deviation, a.k.a. spectrum width */
static int pp_std(struct RaXPol_Data *dat_p, float *zv, float *zh,
	float _Complex *pp_v, float _Complex *pp_h, float *std)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    double v_noise, h_noise;		/* Power noise, should have been
					   updated when ray was read in */
    double zv_, zh_;
    double thres_val, cave, postave;	/* dat_p->file_hdr members */
    double thres_h, thres_v;		/* Power threshold */
    double pri1;

    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    v_noise = dat_p->v_noise;
    h_noise = dat_p->h_noise;
    cave = dat_p->file_hdr.clutter_avg_intvl;
    postave = dat_p->file_hdr.post_averaging_interval;
    thres_val = dat_p->thres_val;
    thres_v = v_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    thres_h = h_noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    if ( dat_p->file_hdr.sumpower ) {
	thres_v /= sqrt(2.0);
	thres_h /= sqrt(2.0);
    }
    pri1 = dat_p->file_hdr.pri1;
    for (g = 0; g < g0; g++) {
	std[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	zv_ = zv[g] - v_noise;
	zh_ = zh[g] - h_noise;
	std[g] = ( zh_ < thres_h || zv_ < thres_v ) ? NAN
	    : 0.03
	    * sqrt((1.0 - ((cabs(pp_v[g]) + cabs(pp_h[g])) / (zh_ + zv_))))
	    / (2.0 * M_PI * sqrt(2.0) * 1.0e-6 * pri1);
    }
    return 1;
}
static int spp_std(struct RaXPol_Data *dat_p, float *std)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zh1, *zh2;
    float _Complex *pp_v, *pp_h;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = spp.zv1;
    zv2 = spp.zv2;
    zh1 = spp.zh1;
    zh2 = spp.zh2;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g]) / 2.0;
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    pp_v = spp.pp_v;
    pp_h = spp.pp_h;
    return pp_std(dat_p, zv, zh, pp_v, pp_h, std);
}
static int spp_sum_pwr_std(struct RaXPol_Data *dat_p, float *std)
{
    float *zv = dat_p->dat_in.spp_sum_pwr.zv;
    float *zh = dat_p->dat_in.spp_sum_pwr.zh;
    float _Complex *pp_v = dat_p->dat_in.spp_sum_pwr.pp_v;
    float _Complex *pp_h = dat_p->dat_in.spp_sum_pwr.pp_h;
    return pp_std(dat_p, zv, zh, pp_v, pp_h, std);
}
static int dpp_std(struct RaXPol_Data *dat_p, float *std)
{
    int g, num_gates;
    static float *zv, *zh;
    float *z_tmp;
    float *zv1, *zv2, *zv3, *zh1, *zh2, *zh3;
    float _Complex *pp_v, *pp_h;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zh when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zv1 = dpp.zv1;
    zv2 = dpp.zv2;
    zv3 = dpp.zv3;
    zh1 = dpp.zh1;
    zh2 = dpp.zh2;
    zh3 = dpp.zh3;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g] + zv3[g]) / 3.0;
	zh[g] = (zh1[g] + zh2[g] + zh3[g]) / 3.0;
    }
    pp_v = dpp.pp_v1;
    pp_h = dpp.pp_h1;
    return pp_std(dat_p, zv, zh, pp_v, pp_h, std);
}
static int dpp_sum_pwr_std(struct RaXPol_Data *dat_p, float *std)
{
    float *zv = dat_p->dat_in.dpp_sum_pwr.zv;
    float *zh = dat_p->dat_in.dpp_sum_pwr.zh;
    float _Complex *pp_v = dat_p->dat_in.dpp_sum_pwr.pp_v1;
    float _Complex *pp_h = dat_p->dat_in.dpp_sum_pwr.pp_h1;
    return pp_std(dat_p, zv, zh, pp_v, pp_h, std);
}

/* Signal to noise ratio, horizontal channel */
static int pp_snr(struct RaXPol_Data *dat_p, float *z, double noise, float *snr)
{
    int g;				/* Gate index */
    int g0;				/* Zero range gate index */
    int num_gates;			/* Total gate count */
    double zero_range_gate_index;
    float z_;
    double thres_val, cave, postave;	/* dat_p->file_hdr members */
    double thres;			/* Power threshold */

    cave = dat_p->file_hdr.clutter_avg_intvl;
    postave = dat_p->file_hdr.post_averaging_interval;
    thres_val = dat_p->thres_val;
    thres = noise * pow(10.0, 0.1 * thres_val) / sqrt(cave * postave);
    if ( dat_p->file_hdr.sumpower ) {
	thres /= sqrt(2.0);
    }
    zero_range_gate_index = dat_p->file_hdr.zero_range_gate_index;
    g0 = floor(zero_range_gate_index + 1);
    num_gates = dat_p->file_hdr.num_rng_gates;
    for (g = 0; g < g0; g++) {
	snr[g] = NAN;
    }
    for (g = g0; g < num_gates; g++) {
	z_ = z[g] - noise;
	if ( z_ < thres ) {
	    z_ = thres;
	}
	snr[g] = 10.0 * (log10(z_) - log10(noise));
    }
    return 1;
}
static int spp_snrhc(struct RaXPol_Data *dat_p, float *snrhc)
{
    int g;
    int num_gates;
    float *zh1, *zh2;
    static float *zh;			/* Average power */
    float *z_tmp;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zh1 = spp.zh1;
    zh2 = spp.zh2;
    for (g = 0; g < num_gates; g++) {
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    return pp_snr(dat_p, zh, dat_p->h_noise, snrhc);
}
static int spp_sum_pwr_snrhc(struct RaXPol_Data *dat_p, float *snrhc)
{
    return pp_snr(dat_p, dat_p->dat_in.spp_sum_pwr.zh, dat_p->h_noise, snrhc);
}
static int dpp_snrhc(struct RaXPol_Data *dat_p, float *snrhc)
{
    int g;
    int num_gates;
    float *zh1, *zh2;
    static float *zh;			/* Average power */
    float *z_tmp;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zh, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zh = z_tmp;
    zh1 = dpp.zh1;
    zh2 = dpp.zh2;
    for (g = 0; g < num_gates; g++) {
	zh[g] = (zh1[g] + zh2[g]) / 2.0;
    }
    return pp_snr(dat_p, zh, dat_p->h_noise, snrhc);
}
static int dpp_sum_pwr_snrhc(struct RaXPol_Data *dat_p, float *snrhc)
{
    return pp_snr(dat_p, dat_p->dat_in.dpp_sum_pwr.zh, dat_p->h_noise, snrhc);
}

/* Signal to noise ratio, vertical channel */
static int spp_snrvc(struct RaXPol_Data *dat_p, float *snrvc)
{
    int g;
    int num_gates;
    float *zv1, *zv2;
    static float *zv;			/* Average power */
    float *z_tmp;
    struct RaXPol_SPP spp = dat_p->dat_in.spp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    zv1 = spp.zv1;
    zv2 = spp.zv2;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g]) / 2.0;
    }
    return pp_snr(dat_p, zv, dat_p->v_noise, snrvc);
}
static int spp_sum_pwr_snrvc(struct RaXPol_Data *dat_p, float *snrvc)
{
    return pp_snr(dat_p, dat_p->dat_in.spp_sum_pwr.zv, dat_p->v_noise, snrvc);
}
static int dpp_snrvc(struct RaXPol_Data *dat_p, float *snrvc)
{
    int g;
    int num_gates;
    float *zv1, *zv2;
    static float *zv;			/* Average power */
    float *z_tmp;
    struct RaXPol_DPP dpp = dat_p->dat_in.dpp;

    num_gates = dat_p->file_hdr.num_rng_gates;
    if ( !(z_tmp = REALLOC(zv, num_gates * sizeof(float))) ) {
	fprintf(stderr, "Could not allocate %d gates for zv when calculating"
		" dbz\n", num_gates);
	return 0;
    }
    zv = z_tmp;
    zv1 = dpp.zv1;
    zv2 = dpp.zv2;
    for (g = 0; g < num_gates; g++) {
	zv[g] = (zv1[g] + zv2[g]) / 2.0;
    }
    return pp_snr(dat_p, zv, dat_p->v_noise, snrvc);
}
static int dpp_sum_pwr_snrvc(struct RaXPol_Data *dat_p, float *snrvc)
{
    return pp_snr(dat_p, dat_p->dat_in.dpp_sum_pwr.zv, dat_p->v_noise, snrvc);
}

/*
   Impose default radar heading, overriding value from ray headers.
   Set to NAN to use ray header values.
 */

void RaXPol_Set_Hdg(double hdg)
{
    dflt_hdg = hdg;
}

/* Return true azimuth of ray r */ 
static double ray_az(struct RaXPol_Ray_Hdr *ray_hdrs, int r)
{
    double hdg;

    if ( isfinite(dflt_hdg) ) {
	hdg = dflt_hdg;
    } else if ( ray_hdrs[r].hdg_ref == 'T' ) {
	hdg = ray_hdrs[r].hdg;
    } else {
	hdg = NAN;
    }
    return ang_to_ref(hdg + 180.0 + ray_hdrs[r].az, 180.0);
}

/* Put angle l into the interval [r - 180, r + 180) */ 
double ang_to_ref(const double l, const double r)
{
    double l1 = fmod(l, 360.0);
    l1 = (l1 < r - 180.0) ? l1 + 360.0 : (l1 >= r + 180.0) ? l1 - 360.0 : l1;
    return (l1 == -0.0) ? 0.0 : l1;
}

static float mean(float *f, int n)
{
    double m;
    int i;

    for (m = 0.0, i = 0; i <= n; i++) {
	m += f[i];
    }
    return m / (n + 1);
}
