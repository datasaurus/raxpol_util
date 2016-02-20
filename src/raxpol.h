/*
   -	raxpol.h --
   -		This header file declares structures and functions
   -		that store and access RaXPol data files.
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
 */

#ifndef RAXPOL_H_
#define RAXPOL_H_

#define RAXPOL_MOMENT_VERSION "1.0"

#include "unix_defs.h"
#include <stdio.h>

/* RaXPol transmission frequency, Hertz */
#define RAXPOL_FREQUENCY 9.73E9

#define RAXPOL_N_SERVMODES 8
enum RAXPOL_SERVMODE {
    RAXPOL_SPP, RAXPOL_SPP_SUM_PWR,
    RAXPOL_DPP, RAXPOL_DPP_SUM_PWR,
    RAXPOL_FFT, RAXPOL_FFT2, RAXPOL_FFT2I, RAXPOL_UNK
};

#define RAXPOL_N_SCAN_TYPES 11
enum RAXPOL_SCAN_TYPE {
    RAXPOL_NONE, RAXPOL_CUSTOM, RAXPOL_SOFT_STOP, RAXPOL_POINT, RAXPOL_SLEW,
    RAXPOL_PPI, RAXPOL_RHI, RAXPOL_AZ_RASTER, RAXPOL_EL_RASTER, RAXPOL_VOL,
    RAXPOL_PAUSED
};

#define RAXPOL_FILE_HDR_SZ 8596
struct RaXPol_File_Hdr {
    int version_code;			/* Version code */
    double asp_chirp_bandwidth;		/* ASP chirp bandwidth, MHz */
    double asp_chirp_center_freq;	/* ASP chirp center freq, MHz */
    int asp_chirp_intrl_delay;		/* ASP chirp intrl delay, nsec */
    double asp_chirp_width;		/* ASP chirp width, nsec */
    double asp_hop_ctr_freq;		/* ASP hop ctr freq, MHz */
    double asp_hop_step_freq;		/* ASP hop step freq, MHz */
    int asp_hop_width;			/* ASP hop width, nsec */
    int asp_num_hops;			/* ASP # hops */
    int asp_trigger_delay;		/* ASP trigger delay, nsec */
    double asp_tukey_alpha;		/* ASP Tukey alpha */
    int asp_waveform_type;		/* ASP waveform type */
    int reserved1;			/* (reserved) */
    int asp_chirp_delay;		/* ASP chirp delay */
    double clutter_filter_width;	/* Clutter filter width, m/s */
    int clutter_avg_intvl;		/* Clutter avg intvl */
    double data_rate;			/* Data rate, #prod/sec */
    int fft_length;			/* FFT length */
    int fft_window_type;		/* FFT window type */
    int reserved2;			/* (reserved) */
    int auto_file_roll_num_rec;		/* Auto file-roll: #rec */
    int auto_file_roll_num_scans;	/* Auto file-roll: #scans */
    int auto_file_roll_flsz;		/* Auto file-roll: #flsz,MB */
    int auto_file_roll_start_time;	/* Auto file-roll: #start time,s */
    int auto_file_roll_type;		/* Auto file-roll type, bitflags */
    int reserved3;			/* (reserved) */
    double filter_bandwidth;		/* Filter bandwidth, MHz */
    int group_interval;			/* Group interval, usec */
    double H_dBZ_dBm_offset;		/* 'H' dBZ/dBm offset */
    double H_noise_power;		/* 'H' noise power, dBm */
    double integration_time;		/* Integration time, sec */
    double max_sampled_range;		/* Max sampled range, m */
    int num_rng_gates;			/* Number of range gates */
    int number_group_pulses;		/* Number of group pulses */
    int post_decimation_level;		/* Post decimation level */
    int post_averaging_interval;	/* Post averaging interval */
    int pri1;				/* PRI, usec: unit 1 */
    int pri2;				/* PRI, usec: unit 2 */
    int pri_total;			/* PRI, usec: total */
    int primary_decimation_CIC;		/* Primary decimation CIC */
    int pulse_compression;		/* Pulse compression */
    double fir_output_len_ratio;	/* FIR output len ratio */
    double fir_output_offset_ratio;	/* FIR output offset ratio */
    int reserved4;			/* (reserved) */
    double range_gate_spacing;		/* Range gate spacing, m */
    int reserved5;			/* (reserved) */
    double range_resolution;		/* Range resolution, m/gate */
    int record_moments;			/* Record moments (boolean) */
    int record_raw;			/* Record raw (boolean) */
    int recording_enabled;		/* Recording enabled (boolean) */

    /* Server modes: SPP, DPP, FFT, FFT2, FFT2I */ 
    int servmode;			/* Server mode */

    int reserved6;			/* (reserved) */
    int server_state;			/* Server state */
    int skip_count;			/* Skip count */
    int reserved7;			/* (reserved) */
    int software_decimation_level;	/* Software decimation level */
    int sumpower;			/* Sum powers (boolean) */
    int switch_delay;			/* Switch delay */
    int switch_padding;			/* Switch padding, nsec */
    int switch_width;			/* Switch width */
    int total_avg_intvl;		/* Total avg intvl */
    int twta_internal_delay;		/* TWTA internal delay, nsec */
    int tx_pulse_bracketing;		/* Tx pulse bracketing, nsec */
    int tx_delay;			/* Tx delay, pulse width multi */
    int tx_trigger_delay;		/* Tx trigger delay, ns */
    int tx_pulse_center_offset;		/* Tx pulse center offset, nsec */
    double unambiguous_range;		/* Unambiguous range, km */
    int clutter_filter;			/* Clutter Filter (on/off) */
    int custom_TX_waveform_was_used;	/* Custom TX waveform was used */
    double V_dBZ_dBm_offset;		/* 'V' dBZ/dBm offset */
    double v_noise_dBm;			/* V noise dBm */
    double zero_range_gate_index;	/* Zero Range Gate Index */

    /*
       scan_types
       3 => Point
       5 => PPI
       6 => RHI
       7 => Az Ras
       8 => El Ras
       9 => Vol
     */

    int scan_type;			/* Scan type (see above) */
    int num_sweeps;			/* No. of sweeps in volume scans */
    int timestamp_sec;			/* Timestamp, sec */
    int timestamp_usec;			/* Timestamp, usec */
    char pulse_filter_file_name[4096];
    char custom_waveform_file_name[4096];
    int reserved8[4];			/* Reserved */
};

struct RaXPol_Ray_Hdr {

    /* radar_system_status */ 
    int timestamp_seconds;		/* Timestamp, seconds */
    int timestamp_useconds;		/* Timestamp, microseconds */
    int radar_temperatures[4];		/* Radar temperatures */
    int inclinometer_roll;		/* Inclinometer, roll */
    int inclinometer_pitch;		/* Inclinometer, pitch */
    int fuel_sensor;			/* Fuel sensor */
    float cpu_temperature;		/* CPU temperature */

    /*
       scan types
       3=Point
       5=PPI
       6=RHI
       7=Az Ras
       8=El Ras
       9=Vol
     */

    int pedestal_scan_type;		/* Pedestal_scan_type */
    float tx_power;			/* Tx power, mW */
    int osc_lock;			/* Oscillator lock alarm */
    int reserved[4];			/* Reserved */

    /* pedestal_status */ 
    double az;				/* Pedestal azimuth, deg */
    double el;				/* Pedestal elevation, deg */
    double az_vel;			/* Pedestal az vel, deg/sec */
    double elev_vel;			/* Pedestal elev vel, deg/sec */
    double az_current;			/* Pedestal az motor current */
    double elev_current;		/* Pedestal elev motor current */
    int sweep_count;			/* Sweep count */
    int volume_count;			/* Volume count */
    int flags;				/* Flags */

    /* gps_status */ 
    int lat_ref;			/* Lat ref ('N' or 'S') */
    double lat;				/* Lat	(degrees) */
    int lon_ref;			/* Lon ref ('E' or 'W') */
    double lon;				/* Lon	(degrees) */
    double alt;				/* Altitude (meters) */
    int hdg_ref;			/* Hdg ref ('T' or 'M') */
    double hdg;				/* Hdg (degrees) */
    double speed;			/* Speed (km/hr) */
    char nmea_msg_gpgga[96];		/* GPS GGA message text */
    int utc_time_sec;			/* UTC time, sec */
    int utc_time_usec;			/* UTC time, usec */

    int data_type;			/* Data type */
    int data_size;			/* Data size */
};

/*
   RaXPol file header + data for one ray + parameters needed to process
   the ray data. Some members might be affected by previous rays, specifically
   the noise members.
 */

struct RaXPol_SPP {
    float *zv1, *zv2, *zh1, *zh2;
    float _Complex *pp_v, *pp_h;
    float _Complex *cc;
};
struct RaXPol_SPP_SumPwr {
    float *zv, *zh;
    float _Complex *pp_v, *pp_h;
    float _Complex *cc;
} spp_sum_pwr;
struct RaXPol_DPP {
    float *zv1, *zv2, *zv3, *zh1, *zh2, *zh3;
    float _Complex *pp_v1, *pp_v2, *pp_h1, *pp_h2;
    float _Complex *cc;
};
struct RaXPol_DPP_SumPwr {
    float *zv, *zh;
    float _Complex *pp_v1, *pp_v2, *pp_h1, *pp_h2;
    float _Complex *cc;
};
struct RaXPol_Data {
    struct RaXPol_File_Hdr file_hdr;
    enum RAXPOL_SERVMODE servmode;	/* Server mode, see above */
    double h_noise, v_noise;		/* Power noise */
    double thres_val;			/* Theshold value */
    double cal_hh_val, cal_vv_val;
    struct RaXPol_Ray_Hdr ray_hdr;	/* Ray header */

    /*
       Input fields. Union has one structure for each server mode.
       Data arrays have file_hdr.num_rng_gates elements.
       See gui_1.pro for "documentation".
     */

    union {
	struct RaXPol_SPP spp;
	struct RaXPol_SPP_SumPwr spp_sum_pwr;
	struct RaXPol_DPP dpp;
	struct RaXPol_DPP_SumPwr dpp_sum_pwr;
	struct { float dum; } fft;	/* Place holder */
	struct { float dum; } fft2;	/* Place holder */
	struct { float dum; } fft2i;	/* Place holder */
    } dat_in;

    /* Function to read one ray */
    int (*read_ray)(struct RaXPol_Data *dat_p, FILE *in);

    /* Functions to compute output moments from input fields */
    int (*dbmhc)(struct RaXPol_Data *, float *);
    int (*dbmvc)(struct RaXPol_Data *, float *);
    int (*dbz)(struct RaXPol_Data *, float *);
    int (*dbz1)(struct RaXPol_Data *, float *);
    int (*vel)(struct RaXPol_Data *, float *);
    int (*zdr)(struct RaXPol_Data *, float *);
    int (*phidp)(struct RaXPol_Data *, float *);
    int (*rhohv)(struct RaXPol_Data *, float *);
    int (*std)(struct RaXPol_Data *, float *);
    int (*snrhc)(struct RaXPol_Data *, float *);
    int (*snrvc)(struct RaXPol_Data *, float *);
};

void RaXPol_Init_File_Hdr(struct RaXPol_File_Hdr *);
void RaXPol_Init_Ray_Hdr(struct RaXPol_Ray_Hdr *);
int RaXPol_Init_Data(struct RaXPol_Data *, FILE *in);
void RaXPol_Old_Fmt(void);
const char *RaXPol_Scan_Type_Descr(enum RAXPOL_SCAN_TYPE);
int RaXPol_Read_File_Hdr(struct RaXPol_File_Hdr *, FILE *);
void RaXPol_FPrintf_File_Hdr(struct RaXPol_File_Hdr *, FILE *);
void RaXPol_Set_Hdg(double);
int RaXPol_Read_Ray_Hdr(struct RaXPol_Ray_Hdr *, FILE *);
int RaXPol_FWrite_Ray_Hdr(struct RaXPol_Ray_Hdr *, FILE *);
int RaXPol_FPrint_Ray_Hdr(struct RaXPol_Ray_Hdr *, FILE *);
void RaXPol_FPrint_Abbrv_Ray_Hdr(struct RaXPol_Ray_Hdr *, FILE *);
int RaXPol_Read_Ray(struct RaXPol_Data *, FILE *);

#endif
