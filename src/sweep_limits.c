/*
   -	sweep_limits.c --
   -		Print limits of a full circle sweep in map coordinates
   -	
   .	Usage:
   .	sweep_limits radar_lon radar_lat ray_length [projection ...]
   .
   .	If map projection is absent, use SWEEP_LIMITS_PROJ environment variable.
   .	If no projection specified, use a default.
   .
   .	Standard output:
   .	x_min x_max y_min y_max
   .
   .	Angles are in degrees. ray_length is in meters.
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
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "alloc.h"
#include "geog_lib.h"
#include "geog_proj.h"

#define SWEEP_LIMITS_PROJ "SWEEP_LIMITS_PROJ"

int main(int argc, char *argv[])
{
    char *argv0 = argv[0];
    char *radar_lon_s;			/* Radar longitude on command line */
    char *radar_lat_s;			/* Radar latitude on command line */
    double radar_lon, radar_lat;	/* Radar location */
    char *ray_len_s;			/* Ray length on command line */
    double ray_len;			/* Ray length */
    char *proj_s;			/* Projection from command line */
    struct GeogProj proj;		/* Geographic projection */
    double lon, lat;			/* Point on edge of sweep */
    double x, y;			/* Point on edge of sweep */
    double a0;				/* Earth radius */
    double dirn;			/* Ray direction */
    double x_min, x_max, y_min, y_max;	/* Sweep limits */
    size_t l, l1;

    if ( argc < 4 ) {
	fprintf(stderr, "%s radar_lon radar_lat ray_length [projection ...]\n",
		argv0);
	exit(EXIT_FAILURE);
    }
    radar_lon_s = argv[1];
    if ( sscanf(radar_lon_s, "%lf", &radar_lon) != 1 ) {
	fprintf(stderr, "%s: expected float value for radar longitude, "
		"got %s\n", argv0, radar_lon_s);
	exit(EXIT_FAILURE);
    }
    radar_lat_s = argv[2];
    if ( sscanf(radar_lat_s, "%lf", &radar_lat) != 1 ) {
	fprintf(stderr, "%s: expected float value for radar latitude, "
		"got %s\n", argv0, radar_lat_s);
	exit(EXIT_FAILURE);
    }
    ray_len_s = argv[3];
    if ( sscanf(ray_len_s, "%lf", &ray_len) != 1 ) {
	fprintf(stderr, "%s: expected float value for ray length, "
		"got %s\n", argv0, ray_len_s);
	exit(EXIT_FAILURE);
    }
    if ( argc > 4 ) {
	/* Set projection for command line arguments */
	int a;

	for (l = 0, a = 4; a < argc; a++) {
	    l += strlen(argv[a]) + 1;	/* word " " */
	}
	l++;				/* nul */
	if ( !(proj_s = CALLOC(l, 1))  ) {
	    fprintf(stderr, "%s: could not allocate %zd bytes for "
		    "projection.\n", argv0, l);
	    exit(EXIT_FAILURE);
	}
	for (a = 4; a < argc; a++) {
	    strcat(proj_s, argv[a]);
	    strcat(proj_s, " ");
	}
    } else if ( (proj_s = getenv(SWEEP_LIMITS_PROJ)) ) {
	/* Set projection from environment variable */
    } else {
	/* Default projection */
	l = strlen("CylEqDist ") + 2 * (15 + 1) + 1;
	if ( !(proj_s = CALLOC(l, 1))  ) {
	    fprintf(stderr, "%s: could not allocate %zd bytes for "
		    "projection.\n", argv0, l);
	    exit(EXIT_FAILURE);
	}
	l1 = snprintf(proj_s, l, "CylEqDist %.5lf %.5lf", radar_lon, radar_lat);
	if ( l1 > l ) {
	    fprintf(stderr, "%s: could not set default projection.\n", argv0);
	    exit(EXIT_FAILURE);
	}
    }
    if ( !GeogProjSetFmStr(proj_s, &proj) ) {
	fprintf(stderr, "%s: could not set projection from %s.\n",
		argv0, proj_s);
	exit(EXIT_FAILURE);
    }

    /* International Standard Mile = 1852 meters = 1/60 great circle degree */
    a0 = 180.0 * 60.0 * 1852.0 / M_PI;
    ray_len /= a0;
    radar_lon *= RAD_DEG;
    radar_lat *= RAD_DEG;
    x_min = y_min = INFINITY;
    x_max = y_max = -INFINITY;
    for (dirn = 0.0; dirn < 2.0 * M_PI; dirn += 2.0 * M_PI / 400) {
	GeogStep(radar_lon, radar_lat, dirn, ray_len, &lon, &lat);
	if ( GeogProjLonLatToXY(lon, lat, &x, &y, &proj) && isfinite(x + y) ) {
	    x_min = (x < x_min) ? x : x_min;
	    y_min = (y < y_min) ? y : y_min;
	    x_max = (x > x_max) ? x : x_max;
	    y_max = (y > y_max) ? y : y_max;
	}
    }
    printf("%lf %lf %lf %lf\n", x_min, x_max, y_min, y_max);

    return 0;
}
