/*
   -	raxpol_seek_ray.c --
   -		This program defines an application prints the index
   -		of the ray just before given time from a raxpol file.
   .
   .	Usage:
   .		raxpol_ray_hdrs [-l] YYYYMMDD-HHMMSS [raxpol_file]
   .
   .	Options:
   .		-l input file is "old" (2011) format.
   .
   .	Non-zero RAXPOL_OLD_FMT environment variable is same as -l.
   .
   .	Copyright (c) 2015, Gordon D. Carrie. All rights reserved.
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
   .	$Revision: 1.8 $ $Date: 2015/02/10 21:07:56 $
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include "raxpol.h"
#include "tm_calc_lib.h"

#define RAXPOL_OLD_FMT "RAXPOL_OLD_FMT"

static double ray_time_jul(struct RaXPol_Ray_Hdr *ray_hdr_p);
static off_t off_tm(off_t, off_t, double, FILE *, struct RaXPol_Ray_Hdr *);
static void fseeko_e(off_t, FILE *);

int main(int argc, char *argv[])
{
    char *argv0;			/* Name of the executable, for error
					   messages. */
    char *dttm = NULL;			/* YYYYMMDD-HHMMSS from command line */
    char *raxpol_fl_nm;			/* RaXPol file path */
    FILE *raxpol_fl;			/* RaXPol file */
    int yr, mon, day, hr, min;		/* Year, month, day, hour, minute */
    double sec;				/* Second */
    double t0;				/* Ray time. Julian date */
    off_t o;				/* Offset in raxpol_fl */
    off_t o0;				/* Offset to start of first ray */
    off_t o1;				/* Offset at end of file */
    long ray_sz;			/* Size in file of one ray */
    struct RaXPol_File_Hdr file_hdr;	/* Header from the RaXPol file */
    struct RaXPol_Ray_Hdr ray_hdr;

    argv0 = argv[0];
    if ( atoi(getenv(RAXPOL_OLD_FMT) ? getenv(RAXPOL_OLD_FMT) : "0") ) {
	RaXPol_Old_Fmt();
    }
    if ( argc == 2 ) {
	if ( strcmp(argv[1], "-V") == 0 ) {
	    printf("%s\n", RAXPOL_MOMENT_VERSION);
	    exit(EXIT_SUCCESS);
	} else {
	    /* raxpol_ray_hdrs YYYYMMDD-HHMMSS */
	    dttm = argv[1];
	    raxpol_fl_nm = "-";
	}
    } else if ( argc == 3 ) {
	if ( strcmp(argv[1], "-l") == 0 ) {
	    /* raxpol_ray_hdrs -l YYYYMMDD-HHMMSS */
	    RaXPol_Old_Fmt();
	    dttm = argv[2];
	    raxpol_fl_nm = "-";
	} else {
	    /* raxpol_ray_hdrs YYYYMMDD-HHMMSS raxpol_file */
	    dttm = argv[1];
	    raxpol_fl_nm = argv[2];
	}
    } else if ( argc == 4 && strcmp(argv[1], "-l") == 0 ) {
	/* raxpol_ray_hdrs -l YYYYMMDD-HHMMSS raxpol_file */
	RaXPol_Old_Fmt();
	dttm = argv[2];
	raxpol_fl_nm = argv[3];
    } else {
	fprintf(stderr, "Usage: %s [-l] YYYYMMDD-HHMMSS [raxpol_file]", argv0);
	exit(EXIT_FAILURE);
    }
    if ( sscanf(dttm, "%4d%2d%2d-%2d%2d%lf",
		&yr, &mon, &day, &hr, &min, &sec) != 6 ) {
	fprintf(stderr, "%s: %s is not a valid time.\n",
		argv0, dttm);
	exit(EXIT_FAILURE);
    }
    t0 = Tm_CalToJul(yr, mon, day, hr, min, sec);
    if ( strcmp(raxpol_fl_nm, "-") == 0 ) {
	raxpol_fl = stdin;
    } else if ( !(raxpol_fl = fopen(raxpol_fl_nm, "r")) ) {
	fprintf(stderr, "%s: could not open %s for reading.\n",
		argv0, raxpol_fl_nm);
	exit(EXIT_FAILURE);
    }
    RaXPol_Init_File_Hdr(&file_hdr);
    if ( !RaXPol_Read_File_Hdr(&file_hdr, raxpol_fl) ) {
	fprintf(stderr, "%s: failed to read file header.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( (o0 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not determine position in file.\n%s\n",
		argv0, strerror(errno));
	exit(EXIT_FAILURE);
    }
    if ( !RaXPol_Read_Ray_Hdr(&ray_hdr, raxpol_fl)
	    || (o = ftello(raxpol_fl)) == -1
	    || fseek(raxpol_fl, 0, SEEK_END) == -1
	    || (o1 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not set position in file.\n", argv0);
	exit(EXIT_FAILURE);
    }
    ray_sz = o - o0 + ray_hdr.data_size;
    o1 = o0 + ((o1 - o0) / ray_sz) * ray_sz - ray_sz;
    o = off_tm(o0, o1, t0, raxpol_fl, &ray_hdr);
    printf("%zd\n", (size_t)((o - o0) / ray_sz));
    return EXIT_SUCCESS;
}

/*
   Find offset to ray with a given time.

   o0 and o1 must give offsets from start of file to some pair of rays in
   RAXPOL file represented by stream in. tm is a Julian day.

   This function calls itself recursively until it converges on the ray
   with nearest tm. Position of in is set to this ray.

   If tm is before the time of the ray at o0, or if the file is unseekable
   (stdin or a pipe), or something goes wrong, in is positioned at o0, and
   return value is o0.

   If tm is after the time of the ray at o1, in is positioned at o1, and
   return value is o1.

   If tm is after the time of the ray at o0, and before the time of the ray
   at o1, file is positioned at the latest ray before that at o1. Offset to
   this ray is returned.

   rh_p will receive the contents of the ray at the returned offset.

   If attempt to set in to return value position fails, process exits.
 */

static off_t off_tm(off_t o0, off_t o1, double tm, FILE *in,
	struct RaXPol_Ray_Hdr *rh_p)
{
    double t0, t1;			/* Ray times at o0 and o1 */
    long ray_sz;			/* Size of one ray */
    off_t o;
    double t;

    /* Read ray header at o0. Assign resulting file position to o. */
    if ( fseeko(in, o0, SEEK_SET) == -1
	    || !RaXPol_Read_Ray_Hdr(rh_p, in) ) {
	return o0;
    }
    if ( (o = ftello(in)) == -1 ) {
	fprintf(stderr, "Could not determine position in file.\n%s\n",
		strerror(errno));
	return o0;
    }
    ray_sz = o - o0 + rh_p->data_size;

    /* Return o0 if tm is before ray at o0, return o1 if tm is after o1. */
    t0 = ray_time_jul(rh_p);
    if ( tm < t0 ) {
	fseeko_e(o0, in);
	return o0;
    }
    if ( fseeko(in, o1, SEEK_SET) == -1
	    || !RaXPol_Read_Ray_Hdr(rh_p, in) ) {
	fseeko_e(o0, in);
	return o0;
    }
    t1 = ray_time_jul(rh_p);
    if ( tm > t1 ) {
	fseeko_e(o0, in);
	return o1;
    }

    /* tm between o0 and o1. Interpolate to a ray before tm. */
    o = o0 + ray_sz * floor((tm - t0) / (t1 - t0) * (o1 - o0) / ray_sz);
    if ( fseeko(in, o, SEEK_SET) == -1
	    || !RaXPol_Read_Ray_Hdr(rh_p, in) ) {
	fseeko_e(o0, in);
	return o0;
    }
    t = ray_time_jul(rh_p);
    if ( t < tm ) {
	/* Overshot backwards. Return o0. */
	if ( fseeko(in, o, SEEK_SET) == -1 ) {
	    fseeko_e(o0, in);
	    return o0;
	}
	return o;
    } else {
	/* Overshot forwards. Call again to push back. */
	return off_tm(o0, o, tm, in, rh_p);
    }
}

/* Set in to offset o or exit */ 
static void fseeko_e(off_t o, FILE *in)
{
    if ( fseeko(in, o, SEEK_SET) == -1 ) {
	fprintf(stderr, "Could not set position in file.\n%s\n",
		strerror(errno));
	exit(EXIT_FAILURE);
    }
}

/* Return ray time as Julian day */
static double ray_time_jul(struct RaXPol_Ray_Hdr *ray_hdr_p)
{
    double jul0;			/* 1970/01/01 00:00:00 */
    jul0 = Tm_CalToJul(1970, 1, 1, 0, 0, 0);

    return jul0
	+ (ray_hdr_p->timestamp_seconds
		+ 1.0e-6 * ray_hdr_p->timestamp_useconds) / 86400.0;
}

