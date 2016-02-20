/*
   -	sweep_img.c --
   -		Print outlines of sweep features given sweep geometry and
   -		values.
   -	
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

/*
   Standard input has form:
   .	scan_type: "PPI"or"RHI"
   .	radar_lon: float
   .	radar_lat: float
   .	num_rays: integer
   .	az: float float float ...	(PPI)
   .	az_width: float float float ...	(PPI)
   .	el: float float float ...	(RHI and PPI)
   .	el_width: float float float ...	(RHI and PPI)
   .	num_gates: integer
   .	gates: float float float ...
   .	colors: color_file_name
   .	data: float float float ...
   .
   .	Angles are measured in degrees.
   .	
   .	rearth = Earth radius, optional. If absent, use GeogREarth(NULL).
   .	See geog_lib (3).
   .
   .	RHI heights use 4/3 rule for refraction, set in refrac variable, below.
   .
   .	radar_lon, radar_lat, and projection are used for PPI only.
   .
   .	For PPI, map projection can be set from SWEEP_IMG_PROJ environment
   .	variable. Projection must be known to GeogProjSetFmStr. See
   .	geog_proj (3). If projection is absent, it is set to
   .	"CylEqDist radar_lon radar_lat". If radar location is also absent,
   .	projection is "CylEqDist 0.0 0.0"
   .
   .	az must provide num_rays values.
   .
   .	az_width must provide num_rays values.
   .	Ray[i] extends from az[i] - az_width[i] / 2 to az[i] + az_width[i] / 2
   .
   .	If az_width absent, assume ray extends half way to next ray.
   .
   .	az_width and az are not used in RHI sweep.
   .
   .	el must provide num_rays values.
   .
   .	el_width must provide num_rays values. Ray[i] extends vertically from
   .	el[i] - ray_el_width[i] / 2 to el[i] + ray_el_width[i] / 2
   .
   .	If el_width absent, assume ray extends half way to next ray.
   .
   .	gates give distance to start of each gate, for num_gates gates.
   .
   .	colors must give number of colors followed by data interval for each
   .	color. Each color_name is preceded and followed by the data boundaries
   .	for that color. Colors names are strings with up to COLOR_NM_LEN_S - 1
   .	characters.
   .
   .	data must provide num_rays * num_gates values for ray 0 gate 0, ray 0
   .	gate 1, ray 0 gate 2, ... ray1 gate 0 ray 1 gate 1, ... and so on.
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "geog_lib.h"
#include "geog_proj.h"
#include "get_colors.h"
#include "bisearch_lib.h"
#include "alloc.h"

#define LEN 256
#define LEN_S "255"

#define SWEEP_IMG_PROJ "SWEEP_IMG_PROJ"

/* Name of transparent color */ 
#define TRANSPARENT "none"

/*
   Point in some coordinate system.
   Could be longitude, latitude, or x, y.
   Point is bogus if x == NAN or y == NAN.
 */

struct Point {
    double x, y;
};

/*
   Map coordinates of corners of a gate.
   Orientation is if viewing a ray from above with radar to the left.
 */

struct GateCoords {
    struct Point ll;			/* Lower left */
    struct Point lr;			/* Lower right */
    struct Point ur;			/* Upper right */
    struct Point ul;			/* Upper left */
};

/* Local functions */
static struct GateCoords ppi_gate_corners(int, int, int, double *, double *,
	double *, double *, double, double, struct GeogProj *);
static struct GateCoords rhi_gate_corners(int, int, int, double *, double *,
	double *);
static int is_point(struct Point);
static float **calloc2f(long, long);
static void free2f(float **);

int main(int argc, char *argv[])
{
    char *argv0 = argv[0];
    char scan_type_s[4];		/* "PPI" or "RHI" */
    enum {PPI, RHI, UNK} scan_type = UNK;
    double rearth;			/* Earth radius */
    double radar_lon, radar_lat;	/* Radar longitude, latitude */
    char *proj_s;			/* Projection description for
					   GeogProjSetFmStr */
    struct GeogProj proj;		/* Geographic projection */
    int num_rays;			/* Number of rays in sweep */
    int num_gates;			/* Number of gates in each ray */
    double *gate_dist;			/* Distance to each gate, dimensioned
					   [gate] */
    double *az;				/* Ray azimuth, dimensioned [ray] */
    double *el;				/* Ray elevation, dimensioned [ray] */
    double *d_az, *d_el;		/* Ray width, dimensioned [ray] */
    float **dat;			/* Input data, dimensioned [ray][gate]
					 */
    int num_colors;			/* Number of colors */
    int num_bnds;			/* Number of boundaries */
    char **colors = NULL;		/* Color names, e.g. "#rrggbb" */
    float *dbnds = NULL;		/* Data bounds for each color */
    int *lists = NULL;			/* Lists of indeces of bounded data.
					   See bisearch_lib (3) */
    struct GateCoords cnrs;		/* Gate corner */
    char key[LEN];			/* Input word, says what comes next */
    int r, g, d, c;			/* Ray, gate, datum, color index */

    memset(scan_type_s, 0, 4);
    rearth = GeogREarth(NULL);
    radar_lon = radar_lat = NAN;
    num_rays = num_gates = num_bnds = -1;
    gate_dist = az = el = d_az = d_el = NULL;
    dat = NULL;

    /* Get "key value" pairs from standard input */
    while ( scanf(" %" LEN_S "s", key) == 1 ) {
	if ( strcmp(key, "scan_type:") == 0 ) {
	    if ( scanf(" %s", scan_type_s) != 1 ) {
		fprintf(stderr, "%s: could not read scan type.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( strcmp(scan_type_s, "PPI") == 0 ) {
		scan_type = PPI;
	    } else if ( strcmp(scan_type_s, "RHI") == 0 ) {
		scan_type = RHI;
	    } else {
		fprintf(stderr, "%s: scan type must be \"PPI\" or \"RHI\"\n",
			argv0);
		exit(EXIT_FAILURE);
	    }
	} else if ( strcmp(key, "rearth:") == 0 ) {
	    if ( scanf(" %lf", &rearth) != 1 ) {
		fprintf(stderr, "%s: could not read radar longitude\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    GeogREarth(&rearth);
	} else if ( strcmp(key, "radar_lon:") == 0 ) {
	    if ( scanf(" %lf", &radar_lon) != 1 ) {
		fprintf(stderr, "%s: could not read radar longitude\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    radar_lon *= RAD_DEG;
	} else if ( strcmp(key, "radar_lat:") == 0 ) {
	    if (scanf(" %lf", &radar_lat) != 1 ) {
		fprintf(stderr, "%s: could not read radar latitude\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    radar_lat *= RAD_DEG;
	} else if ( strcmp(key, "num_rays:") == 0 ) {
	    if ( scanf(" %d", &num_rays) != 1 ) {
		fprintf(stderr, "%s: could not read ray count\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(num_rays > 0) ) {
		fprintf(stderr, "%s: ray count must be positive.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	} else if ( strcmp(key, "az:") == 0 ) {
	    if ( num_rays == -1 ) {
		fprintf(stderr, "%s: ray azimuths given before ray count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( az ) {
		fprintf(stderr, "%s: ray azimuth centers given more than "
			"once.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(az = CALLOC(num_rays, sizeof(double))) ) {
		fprintf(stderr, "%s: could not allocate memory for ray "
			"azimuth low limits for %d rays.\n", argv0, num_rays);
		exit(EXIT_FAILURE);
	    }
	    for (r = 0; r < num_rays; r++) {
		if ( scanf(" %lf", az + r) != 1 ) {
		    fprintf(stderr, "%s: could not read azimuth center for "
			    "ray %d\n", argv0, r);
		    exit(EXIT_FAILURE);
		}
		az[r] *= RAD_DEG;
	    }
	} else if ( strcmp(key, "az_width:") == 0 ) {
	    if ( num_rays == -1 ) {
		fprintf(stderr, "%s: ray azimuths given before ray count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( d_az ) {
		fprintf(stderr, "%s: ray azimuth limits given more than "
			"once.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(d_az = CALLOC(num_rays, sizeof(double))) ) {
		fprintf(stderr, "%s: could not allocate memory for ray "
			"azimuth low limits for %d rays.\n", argv0, num_rays);
		exit(EXIT_FAILURE);
	    }
	    for (r = 0; r < num_rays; r++) {
		if ( scanf(" %lf", d_az + r) != 1 ) {
		    fprintf(stderr, "%s: could not read azimuth limits for "
			    "ray %d\n", argv0, r);
		    exit(EXIT_FAILURE);
		}
		d_az[r] *= RAD_DEG;
	    }
	} else if ( strcmp(key, "el_width:") == 0 ) {
	    if ( num_rays == -1 ) {
		fprintf(stderr, "%s: ray elevations given before ray count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( d_el ) {
		fprintf(stderr, "%s: ray elevation limits given more than "
			"once.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(d_el = CALLOC(num_rays, sizeof(double))) ) {
		fprintf(stderr, "%s: could not allocate memory for ray "
			"elevation low limits for %d rays.\n", argv0, num_rays);
		exit(EXIT_FAILURE);
	    }
	    for (r = 0; r < num_rays; r++) {
		if ( scanf(" %lf", d_el + r) != 1 ) {
		    fprintf(stderr, "%s: could not read elevation limits for "
			    "ray %d\n", argv0, r);
		    exit(EXIT_FAILURE);
		}
		d_el[r] *= RAD_DEG;
	    }
	} else if ( strcmp(key, "el:") == 0 ) {
	    if ( num_rays == -1 ) {
		fprintf(stderr, "%s: ray elevations given before ray count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( el ) {
		fprintf(stderr, "%s: ray elevation centers given more than "
			"once.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(el = CALLOC(num_rays, sizeof(double))) ) {
		fprintf(stderr, "%s: could not allocate memory for ray "
			"elevation low limits for %d rays.\n", argv0, num_rays);
		exit(EXIT_FAILURE);
	    }
	    for (r = 0; r < num_rays; r++) {
		if ( scanf(" %lf", el + r) != 1 ) {
		    fprintf(stderr, "%s: could not read elevation center for "
			    "ray %d\n", argv0, r);
		    exit(EXIT_FAILURE);
		}
		el[r] *= RAD_DEG;
	    }
	} else if ( strcmp(key, "num_gates:") == 0 ) {
	    if ( scanf(" %d", &num_gates) != 1 ) {
		fprintf(stderr, "%s: could not read gate count\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(num_gates > 0) ) {
		fprintf(stderr, "%s: gate count must be positive.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	} else if ( strcmp(key, "gates:") == 0 ) {
	    if ( num_gates == -1 ) {
		fprintf(stderr, "%s: ray elevations given before ray count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( gate_dist ) {
		fprintf(stderr, "%s: gate spacing given more than once.\n",
			argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(gate_dist = CALLOC(num_gates + 1, sizeof(double))) ) {
		fprintf(stderr, "%s: could not allocate memory for gate "
			"distances for %d gates.\n", argv0, num_gates);
		exit(EXIT_FAILURE);
	    }
	    for (g = 0; g < num_gates; g++) {
		if ( scanf(" %lf", gate_dist + g) != 1 ) {
		    fprintf(stderr, "%s: could not read gate distance for "
			    "gate %d\n", argv0, g);
		    exit(EXIT_FAILURE);
		}
	    }
	    /* Assume last gate is same length as second to last */
	    gate_dist[num_gates] = gate_dist[num_gates - 1]
		    + (gate_dist[num_gates - 1] - gate_dist[num_gates - 2]);
	} else if ( strcmp(key, "colors:") == 0 ) {
	    if ( !GetColors(stdin, &num_colors, &colors, &dbnds) ) {
		fprintf(stderr, "%s: could not read colors.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    num_bnds = num_colors + 1;
	} else if ( strcmp(key, "data:") == 0 ) {
	    if ( num_rays == -1 && num_gates == -1 ) {
		fprintf(stderr, "%s: data found before ray and gate count "
			"known.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    if ( !(dat = calloc2f(num_rays, num_gates)) ) {
		fprintf(stderr, "%s: could not allocate memory for input "
			"data values for %d rays and %d gates.\n",
			argv0, num_rays, num_gates);
		exit(EXIT_FAILURE);
	    }
	    for (g = 0; g < num_rays * num_gates; g++) {
		if ( scanf(" %f", dat[0] + g) != 1 ) {
		    /* scanf in OpenBSD 5.7 does not read "inf" */
		    char nan[10];
		    if ( scanf(" %9s", nan) == 1 ) {
			dat[0][g] = strtof(nan, NULL);
		    } else {
			fprintf(stderr, "%s: could not read data value for "
				"ray %d, gate %d\n", argv0,
				g / num_gates, g % num_gates);
			exit(EXIT_FAILURE);
		    }
		}
	    }
	} else {
	    fprintf(stderr, "%s: unknown key word %s\n", argv0, key);
	    exit(EXIT_FAILURE);
	}
    }
    if ( strlen(scan_type_s) == 0 ) {
	fprintf(stderr, "%s: no scan type.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !(num_rays > 0) ) {
	fprintf(stderr, "%s: no ray count.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !(num_gates > 0) ) {
	fprintf(stderr, "%s: no gate count.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !gate_dist ) {
	fprintf(stderr, "%s: no gate distances.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !(num_colors > 0) || !colors) {
	fprintf(stderr, "%s: no colors.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !az ) {
	fprintf(stderr, "%s: no azimuths.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !el ) {
	fprintf(stderr, "%s: no elevations.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !dat ) {
	fprintf(stderr, "%s: no data.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( isnan(GeogREarth(NULL)) || GeogREarth(NULL) == 0.0 ) {
	fprintf(stderr, "%s: Earth radius not set.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( scan_type == PPI ) {
	if ( isnan(radar_lon) || isnan(radar_lat) ) {
	    fprintf(stderr, "%s: PPI requires radar location.\n", argv0);
	    exit(EXIT_FAILURE);
	}
	if ( (proj_s = getenv(SWEEP_IMG_PROJ)) ) {
	    if ( !GeogProjSetFmStr(proj_s, &proj) ) {
		fprintf(stderr, "Could not set default projection.\n");
		exit(EXIT_FAILURE);
	    }
	} else {
	    char dproj_s[LEN];
	    double lon, lat;

	    lon = ( isfinite(radar_lon) ) ? radar_lon * DEG_RAD : 0.0;
	    lat = ( isfinite(radar_lat) ) ? radar_lat * DEG_RAD : 0.0;
	    if ( snprintf(dproj_s, LEN, "CylEqDist %.9g %.9g", lon, lat) > LEN
		    || !GeogProjSetFmStr(dproj_s, &proj) ) {
		fprintf(stderr, "Could not set default projection.\n");
		exit(EXIT_FAILURE);
	    }
	}
    }

    /* Print outlines of gates for each color. Skip TRANSPARENT color. */ 
    lists = CALLOC((size_t)(num_bnds + num_rays * num_gates), sizeof(int));
    if ( !lists ) {
	fprintf(stderr, "%s: could not allocate color lists.\n", argv0);
	exit(EXIT_FAILURE);
    }
    BiSearch_FDataToList(dat[0], num_rays * num_gates, dbnds, num_bnds, lists);
    for (c = 0; c < num_colors; c++) {
	if ( BiSearch_1stIndex(lists, c) != -1
		&& strcmp(colors[c], TRANSPARENT) != 0 ) {
	    printf("color %s\n", colors[c]);
	    for (d = BiSearch_1stIndex(lists, c);
		    d != -1;
		    d = BiSearch_NextIndex(lists, d)) {
		r = d / num_gates;
		g = d % num_gates;
		switch (scan_type) {
		    case PPI:
			cnrs = ppi_gate_corners(r, g, num_rays, az, d_az, el,
				gate_dist, radar_lon, radar_lat, &proj);
			break;
		    case RHI:
			cnrs = rhi_gate_corners(r, g, num_rays, el, d_el,
				gate_dist);
			break;
		    case UNK:
			fprintf(stderr, "%s: scan type not set\n", argv0);
			exit(EXIT_FAILURE);
			break;
		}
		if ( is_point(cnrs.ll) && is_point(cnrs.lr)
			&& is_point(cnrs.ul) && is_point(cnrs.ur) ) {
		    printf("gate " "%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
			    cnrs.ll.x, cnrs.ll.y, cnrs.lr.x, cnrs.lr.y,
			    cnrs.ur.x, cnrs.ur.y, cnrs.ul.x, cnrs.ul.y);
		}
	    }
	}
    }

    /* Clean up and exit */ 
    free(az);
    free(d_az);
    free(el);
    free(d_el);
    free2f(dat);
    FREE(colors);
    FREE(dbnds);
    FREE(lists);
    return 0;
}

/*
   Return map coordinates of corners of gate at azimuth index r, gate index g.
   num_rays	= number of rays in sweep
   az		= azimuths of ray centers, dimensioned [num_rays]
   d_az		= ray widths, dimesioned [num_rays]
   el		= azimuths of ray centers, dimensioned [num_rays]
   gate_dist	= distance to start of each gate
   radar_lon	= radar longitude
   radar_lat	= radar latitude
   proj_p	= projection, converts longitude, latitude to map x,y.
   Assume elevation is constant.
 */

static struct GateCoords ppi_gate_corners(int r, int g, int num_rays,
	double *az, double *d_az, double *el, double *gate_dist,
	double radar_lon, double radar_lat, struct GeogProj *proj_p)
{
    double rearth;			/* Earth radius */
    double d0, d1;			/* Distance along ray to start and
					   end of gate. */
    double s0, s1;			/* Distance along ground to start and
					   end of gate. */
    double daz;				/* ray width */
    double az0;				/* Azimuth at edge of current ray
					   towards previous ray */
    double az1;				/* Azimuth at edge of current ray
					   towards next ray */
    struct Point c00;			/* Corner towards previous ray, previous
					   gate */
    struct Point c01;			/* Corner towards previous ray, next
					   gate */
    struct Point c11;			/* Corner towards next ray, next gate */
    struct Point c10;			/* Corner towards next ray, previous
					   gate */
    struct GateCoords corners;		/* Return value */

    /* .                previous ray            .
       . -------------- c00 ---- c01 ---------- .
       .  previous gate  |        |  next gate  .
       .--------------  c10 ---- c11 ---------- .	Azimuth increases cw
       .                  next ray              . */

    rearth = GeogREarth(NULL);
    if ( d_az) {
	az0 = (az[r] - 0.5 * d_az[r]);
	az1 = (az[r] + 0.5 * d_az[r]);
    } else {
	if ( r == 0 ) {
	    daz = GeogLonDiff(az[r + 1], az[r]);
	    az0 = (az[r] - 0.5 * daz);
	    az1 = (az[r] + 0.5 * daz);
	} else if ( r == num_rays - 1 ) {
	    daz = GeogLonDiff(az[r], az[r - 1]);
	    az0 = (az[r] - 0.5 * daz);
	    az1 = (az[r] + 0.5 * daz);
	} else {
	    az0 = 0.5 * (GeogLonR(az[r - 1], az[r]) + az[r]);
	    az1 = 0.5 * (az[r] + GeogLonR(az[r + 1], az[r]));
	}
    }
    d0 = gate_dist[g];
    d1 = gate_dist[g + 1];
    s0 = atan(d0 * cos(el[r]) / (rearth + d0 * sin(el[r])));
    s1 = atan(d1 * cos(el[r]) / (rearth + d1 * sin(el[r])));
    GeogStep(radar_lon, radar_lat, az0, s0, &c00.x, &c00.y);
    GeogStep(radar_lon, radar_lat, az0, s1, &c01.x, &c01.y);
    GeogStep(radar_lon, radar_lat, az1, s1, &c11.x, &c11.y);
    GeogStep(radar_lon, radar_lat, az1, s0, &c10.x, &c10.y);
    GeogProjLonLatToXY(c00.x, c00.y, &c00.x, &c00.y, proj_p);
    GeogProjLonLatToXY(c01.x, c01.y, &c01.x, &c01.y, proj_p);
    GeogProjLonLatToXY(c10.x, c10.y, &c10.x, &c10.y, proj_p);
    GeogProjLonLatToXY(c11.x, c11.y, &c11.x, &c11.y, proj_p);
    if ( GeogLonDiff(az1, az0) > 0.0 ) {
	corners.ll = c10;
	corners.lr = c11;
	corners.ur = c01;
	corners.ul = c00;
    } else {
	corners.ll = c00;
	corners.lr = c01;
	corners.ur = c11;
	corners.ul = c10;
    }
    return corners;
}

/*
   Return (distance_down_range altitude) coordinates of corners of gate at
   azimuth index r, gate index g.
   num_rays	= number of rays in sweep
   el		= azimuths of ray centers, dimensioned [num_rays]
   d_el		= ray widths, dimesioned [num_rays]
   gate_dist	= distance to start of each gate
 */

static struct GateCoords rhi_gate_corners(int r, int g, int num_rays,
	double *el, double *d_el, double *gate_dist)
{
    double rearth;			/* Earth radius */
    double refrac = 4.0 / 3.0;		/* Refraction adjustment */
    double d0, d1;			/* Distance along ray to start and
					   end of gate. */
    double el0;				/* Elevation at edge of current ray
					   towards previous ray */
    double el1;				/* Elevation at edge of current ray
					   towards next ray */
    double del;				/* Vertical ray width */
    struct Point c00;			/* Corner toward previous ray, previous
					   gate */
    struct Point c01;			/* Corner toward previous ray, next
					   gate */
    struct Point c11;			/* Corner toward next ray, gate */
    struct Point c10;			/* Corner toward next ray, previous
					   gate */
    struct GateCoords corners;		/* Return value */

    /* .                 next ray              .
       . -------------- c10 ---- c11 --------- .
       .  previous gate  |        |  next gate .
       . -------------  c00 ---- c01 --------- .
       .                previous ray           . */

    rearth = GeogREarth(NULL);
    if ( d_el) {
	el0 = (el[r] - 0.5 * d_el[r]);
	el1 = (el[r] + 0.5 * d_el[r]);
    } else {
	if ( r == 0 ) {
	    del = el[r + 1] - el[r];
	    el0 = (el[r] - 0.5 * del);
	    el1 = (el[r] + 0.5 * del);
	} else if ( r == num_rays - 1 ) {
	    del = el[r] - el[r - 1];
	    el0 = (el[r] - 0.5 * del);
	    el1 = (el[r] + 0.5 * del);
	} else {
	    el0 = (0.5 * (el[r - 1] + el[r]));
	    el1 = (0.5 * (el[r] + el[r + 1]));
	}
    }
    d0 = gate_dist[g];
    d1 = gate_dist[g + 1];
    c00.y = GeogBeamHt(d0, el0, rearth * refrac);
    c00.x = rearth * asin(d0 * cos(el0) / (rearth + c00.y));
    c01.y = GeogBeamHt(d1, el0, rearth * refrac);
    c01.x = rearth * asin(d1 * cos(el0) / (rearth + c01.y));
    c11.y = GeogBeamHt(d1, el1, rearth * refrac);
    c11.x = rearth * asin(d1 * cos(el1) / (rearth + c11.y));
    c10.y = GeogBeamHt(d0, el1, rearth * refrac);
    c10.x = rearth * asin(d0 * cos(el1) / (rearth + c10.y));
    if ( el1 < el0 ) {
	corners.ll = c10;
	corners.lr = c11;
	corners.ur = c01;
	corners.ul = c00;
    } else {
	corners.ll = c00;
	corners.lr = c01;
	corners.ur = c11;
	corners.ul = c10;
    }
    return corners;
}

static int is_point(struct Point p)
{
    return isfinite(p.x + p.y);
}

/* Allocate a 2 dimensional array of floats, initialized to NAN */
static float ** calloc2f(long j, long i)
{
    float **dat = NULL, *dat_p;
    long n;
    size_t jj, ii;		/* Addends for pointer arithmetic */
    size_t ji;

    /* Make sure casting to size_t does not overflow anything.  */
    if (j <= 0 || i <= 0) {
	fprintf(stderr, "Array dimensions must be positive.\n");
	return NULL;
    }
    jj = (size_t)j;
    ii = (size_t)i;
    ji = jj * ii;
    if (ji / jj != ii) {
	fprintf(stderr, "Dimensions, [%ld][%ld] too big "
		"for pointer arithmetic.\n", j, i);
	return NULL;
    }

    dat = (float **)CALLOC(jj + 2, sizeof(float *));
    if ( !dat ) {
	fprintf(stderr, "Could not allocate memory for 1st dimension of "
		"two dimensional array.\n");
	return NULL;
    }
    dat[0] = (float *)CALLOC(ji, sizeof(float));
    if ( !dat[0] ) {
	FREE(dat);
	fprintf(stderr, "Could not allocate memory for values "
		"of two dimensional array.\n");
	return NULL;
    }
    for (n = 1; n <= j; n++) {
	dat[n] = dat[n - 1] + i;
    }
    for (dat_p = dat[0]; dat_p < dat[0] + ji; dat_p++) {
	*dat_p = NAN;
    }
    return dat;
}

/* Free memory allocated with calloc2f */
static void free2f(float **dat)
{
    if (dat && dat[0]) {
	FREE(dat[0]);
    }
    FREE(dat);
}

