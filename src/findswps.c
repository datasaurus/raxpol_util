/*
   -	findswps.c --
   -		Look for sweeps in a set of rays. A sweep is a set of rays
   -		with constant azimuth/elevation and monotonically varying
   -		elevation/azimuth. See findswps (1).
   -
   .
   ........................................................................
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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define FINDSWPS_VERSION "0.1"

/* Defaults for angles measured in degrees. Change defaults for other units. */ 
static double AngResoln = 0.01;		/* Angular resolution. Angles that
					   differ by less than AngResoln are
					   assumed to be the same angle. */ 
static double MinSpanP = 10.0;		/* Minimum PPI sweep size, degrees */
static double MinSpanR = 10.0;		/* Minimum RHI sweep size, degrees */
static double MaxStDv = 0.5;		/* Maximum allowed variation in fixed
					   angle, degrees */
#define D360 360.0			/* 360 degrees */
#define D180 180.0			/* 180 degrees */
#define D90 90.0			/* 90 degrees */
#define ANG_UNIT "degrees"

/*
   If antenna does not move MinStep degrees in NStep rays, assume it
   is not moving.
 */

static long NStep = 8;
static double MinStep = 0.04;

/*
   Scan types
   PPI_I	plan position indicator, increasing azimuth
   PPI_D	plan position indicator, decreasing azimuth
   RHI_I	range height indicator, increasing azimuth
   RHI_D	range height indicator, decreasing azimuth
   UNK		unknown
 */

#define NUM_SCAN_TYPES 5
enum SCAN_TYPE {PPI_I, PPI_D, RHI_I, RHI_D, UNK};

/* One ray */
#define LEN 512
struct ray_hdr {
    struct ray_hdr *next;		/* Next ray */
    struct ray_hdr *prev;		/* Previous ray */
    unsigned long i;			/* Ray index. First ray read and kept
					   has index 0. */
    double az;				/* Azimuth, deg, -180.0 <= az < 180.0 */
    double el;				/* Elevation, deg */
    double daz;				/* Azimuth increment from previous
					   ray */
    double del;				/* Elevation increment from previous
					   ray */
    char ln[LEN];			/* Other header information */
};

/* Rays of interest */
static unsigned long i_tot;		/* Total number of rays read in */
static struct ray_hdr *RayHdrs;		/* Array of ray headers under
					   consideration */
static struct ray_hdr *Ray0;		/* First ray in candidate sweep */

/* Local functions */
static void init_ray(struct ray_hdr *);
static struct ray_hdr *next_after(struct ray_hdr *);
static int is_ppi(double, double, double, double);
static int ppi_moves(struct ray_hdr *, double);
static int is_rhi(double, double, double, double);
static int rhi_moves(struct ray_hdr *, double);
static int read_ray(struct ray_hdr *);
static void print_sweep(enum SCAN_TYPE, double, struct ray_hdr *,
	struct ray_hdr *);
static double ang_to_ref(double, double);
static double diff_az(double, double);
static double phi(double);

static char *argv0;			/* Name of the executable */

int main(int argc, char *argv[])
{
    int c;				/* Index into argv */
    extern char *optarg;		/* See getopt (3) */
    extern int optind;			/* See getopt (3) */
    double sweep_ang_req = NAN;		/* Sweep angle requested from command
					   line */
    double swp_angl = NAN;		/* Azimuth of RHI, or elevation of PPI,
					   obtained as average for all rays in
					   the sweep */
    struct ray_hdr *ray1;		/* Last ray in candidate sweep */
    enum SCAN_TYPE scan_type = UNK;	/* Scan type for sweep in input */
    int want_ppi = 1;			/* If true, look for PPI scans */
    int want_rhi = 1;			/* If true, look for RHI scans */
    double daz;				/* How far antenna has traveled since
					   start of (possible) sweep, degrees */
    double del;				/* How far antenna has traveled since
					   start of (possible) sweep, degrees */
    double sum_az, sum2_az;		/* Azimuth sum, sum of squares */
    double mn_az, mn2_az, var_az;	/* Azimuth mean, mean square, variance
					 */
    double sum_el, sum2_el;		/* Elevation sum, sum of squares */
    double mn_el, mn2_el, var_el;	/* Elevation mean, square mean, variance
					 */
    double dirn;			/* Indicator of what the non-fixed angle
					   is doing. If dirn > 0.0, the
					   non-fixed angle is increasing
					   throughout the sweep. If dirn < 0.0,
					   the non-fixed angle is decreasing. */
    struct ray_hdr *ray;
    int n;

    argv0 = argv[0];
    while ((c = getopt(argc, argv, ":i:a:r:n:x:v")) != -1) {
	switch(c) {
	    case 'v':
		printf("%s %s: ", argv0, FINDSWPS_VERSION);
		if ( isnan(sweep_ang_req) ) {
		    printf("sweep_angle=\"ALL\" ");
		} else {
		    printf("sweep_angle=%.2lf ", sweep_ang_req);
		}
		printf("angular_resolution=%.4g MinSpanPpi=%.2lf "
			"MinSpanRhi=%.2lf max_deviation=%.2lf\n",
			AngResoln, MinSpanP, MinSpanR, MaxStDv);
		exit(EXIT_SUCCESS);
		break;
	    case 'i':
		if ( strcmp(optarg, "PPI") != 0
			&& strcmp(optarg, "RHI") != 0
			&& strcmp(optarg, "ALL") != 0 ) {
		    fprintf(stderr, "%s: scan type must be \"PPI\", \"RHI\", "
			    "or \"ALL\"\n", argv0);
		    exit(EXIT_FAILURE);
		}
		if ( strcmp(optarg, "PPI") == 0
			|| strcmp(optarg, "ALL") == 0 ) {
		    want_ppi = 1;
		} else {
		    want_ppi = 0;
		}
		if ( strcmp(optarg, "RHI") == 0
			|| strcmp(optarg, "ALL") == 0 ) {
		    want_rhi = 1;
		} else {
		    want_rhi = 0;
		}
		break;
	    case 'a':
		if ( sscanf(optarg, "%lf", &sweep_ang_req) != 1
			&& strcmp(optarg, "ALL") != 0 ) {
		    fprintf(stderr, "%s: expected float value or \"ALL\" for "
			    "sweep angle, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'r':
		if ( sscanf(optarg, "%lf", &AngResoln) != 1 ) {
		    fprintf(stderr, "%s: expected float value for angular "
			    "resolution, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		if ( AngResoln < 0.0 ) {
		    fprintf(stderr, "%s: angular resolution cannot be "
			    "negative.\n", argv0);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'n':
		if ( sscanf(optarg, "%lf,%lf", &MinSpanP, &MinSpanR)
			== 2 ) {
		    if ( MinSpanP < 0.0 ) {
			fprintf(stderr, "%s: PPI minimum span cannot be "
				"negative.\n", argv0);
			exit(EXIT_FAILURE);
		    }
		    if ( MinSpanR < 0.0 ) {
			fprintf(stderr, "%s: RHI minimum span cannot be "
				"negative.\n", argv0);
			exit(EXIT_FAILURE);
		    }
		} else if ( sscanf(optarg, "%lf", &MinSpanP) == 1 ) {
		    if ( MinSpanP < 0.0 ) {
			fprintf(stderr, "%s: minimum span cannot be "
				"negative.\n", argv0);
			exit(EXIT_FAILURE);
		    }
		    MinSpanR = MinSpanP;
		} else {
		    fprintf(stderr, "%s: expected float value for minimum "
			    "sweep span, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 'x':
		if ( sscanf(optarg, "%lf", &MaxStDv) != 1 ) {
		    fprintf(stderr, "%s: expected float value for maximum "
			    "variation, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		if ( MaxStDv < 0.0 ) {
		    fprintf(stderr, "%s: maximum variation of fixed angle "
			    "cannot be negative.\n", argv0);
		    exit(EXIT_FAILURE);
		}
		break;
	    default:
		fprintf(stderr, "%s: unknown option\n", argv0);
		exit(EXIT_FAILURE);
		break;
	}
    }
    if ( optind + 1 == argc ) {
	fprintf(stderr, "Usage: %s [-v] [-i scan_type] [-a sweep_angle] "
		"[-r resolution]  [-n MinSpanP[,MinSpanR]] [-x max_dev]\n",
		argv0);
	exit(EXIT_FAILURE);
    }

    /* Read and process rays */ 
    for (Ray0 = next_after(NULL); Ray0; ) {

	/*
	   Read rays until end of input, or until antenna traverses at least
	   min_span degrees. As each ray is read, update azimuth and elevation
	   moments for the span. These will determine whether the span is a PPI
	   sweep or RHI sweep.
	 */

	scan_type = UNK;
	daz = del = 0.0;
	sum_el = sum2_el = 0.0;
	sum_az = sum2_az = 0.0;
	mn_az = mn2_az = var_az = mn_el = mn2_el = var_el = NAN;
	n = 0;
	ray1 = NULL;
	for (ray = next_after(Ray0);
		ray && fabs(daz) < MinSpanP && fabs(del) < MinSpanR;
		ray = next_after(ray)) {
	    ray1 = ray;
	    n++;
	    daz += ray1->daz;
	    del += ray1->del;
	    sum_az += ray1->az;
	    sum2_az += ray1->az * ray1->az;
	    sum_el += ray1->el;
	    sum2_el += ray1->el * ray1->el;
	}
	mn_az = sum_az / n;
	mn2_az = sum2_az / n;
	var_az = mn2_az - mn_az * mn_az;
	mn_el = sum_el / n;
	mn2_el = sum2_el / n;
	var_el = mn2_el - mn_el * mn_el;

	/*
	   Either input has ended, or rays have traversed enough azimuth
	   or elevation to comprise a possible sweep.
	 */

	if ( !ray1 ) {
	    /* Nothing after Ray0 => end of input */
	    exit(EXIT_SUCCESS);
	} else if ( want_ppi && is_ppi(daz, mn_el, var_el, sweep_ang_req) ) {
	    /* Possible PPI. Add rays until PPI ends. */ 
	    swp_angl = mn_el;
	    scan_type = ( daz > 0.0 ) ? PPI_I : PPI_D;
	    dirn = daz;
	    for (ray = ray1;
		    ray && ppi_moves(ray, dirn)
			&& is_ppi(daz, mn_el, var_el, sweep_ang_req)
			&& daz <= D360;
		    ray = next_after(ray)) {
		ray1 = ray;
		n++;
		daz += ray1->daz;
		sum_el += ray1->el;
		sum2_el += ray1->el * ray1->el;
		mn_el = sum_el / n;
		mn2_el = sum2_el / n;
		var_el = mn2_el - mn_el * mn_el;
	    }

	    /* Discard deviant rays at start of sweep. */ 
	    for ( ;
		    Ray0 != ray1
		    && ( !ppi_moves(Ray0, dirn)
			|| fabs(Ray0->el - swp_angl) >= MaxStDv );
		    Ray0 = Ray0->next) {
	    }

	    /*
	       If azimuth span daz is still >= MinSpanP, print the sweep.
	       Set Ray0 to next ray after end of sweep.
	     */

	    for (daz = 0.0, ray = Ray0; ray != ray1; ray = next_after(ray)) {
		daz += ray->daz;
	    }
	    if ( fabs(daz) >= MinSpanP ) {
		print_sweep(scan_type, swp_angl, Ray0, ray1);
	    }
	    Ray0 = next_after(ray1);
	} else if ( want_rhi && is_rhi(del, mn_az, var_az, sweep_ang_req) ) {
	    /* Possible RHI. Add rays until RHI ends. */
	    swp_angl = mn_az;
	    scan_type = ( del > 0.0 ) ? RHI_I : RHI_D;
	    dirn = del;
	    for (ray = ray1;
		    ray && rhi_moves(ray, dirn)
		    && is_rhi(del, mn_az, var_az, sweep_ang_req);
		    ray = next_after(ray)) {
		ray1 = ray;
		n++;
		del += ray1->del;
		sum_az += ray1->az;
		sum2_az += ray1->az * ray1->az;
		sum2_el += ray1->el * ray1->el;
		mn_az = sum_az / n;
		mn2_az = sum2_az / n;
		var_az = mn2_az - mn_az * mn_az;
	    }

	    /* Discard deviant rays at start of sweep. */ 
	    for ( ;
		    Ray0 != ray1
		    && ( !rhi_moves(Ray0, dirn)
			|| fabs(diff_az(Ray0->az, swp_angl)) >= MaxStDv );
		    Ray0 = Ray0->next) {
	    }

	    /*
	       If elevation span del is still >= MinSpanR, print the sweep.
	       Set Ray0 to next ray after end of sweep.
	     */

	    for (del = 0.0, ray = Ray0; ray != ray1; ray = next_after(ray)) {
		del += ray->del;
	    }
	    if ( fabs(del) >= MinSpanP ) {
		print_sweep(scan_type, swp_angl, Ray0, ray1);
	    }
	    Ray0 = next_after(ray1);
	} else {
	    /* No sweep. Increment Ray0 and resume search. */
	    Ray0 = next_after(Ray0);
	}
    }
    return EXIT_SUCCESS;
}

/*
   Return true if azimuth span daz, elevation mean mn_el and variance var_el
   suggest a PPI with sweep angle sweep_ang_req.
 */

static int is_ppi(double daz, double mn_el, double var_el, double sweep_ang_req)
{
    if (isfinite(sweep_ang_req) ) {
	return fabs(daz) >= MinSpanP
	    && var_el < MaxStDv * MaxStDv
	    && fabs(mn_el - sweep_ang_req) < MaxStDv;
    } else {
	return fabs(daz) >= MinSpanP
	    && var_el < MaxStDv * MaxStDv;
    }
}

/* Return true if NStep rays from ray maintain direction and speed */ 
static int ppi_moves(struct ray_hdr *ray, double dirn)
{
    double daz;
    int n;
    struct ray_hdr *r;

    for (r = ray, n = 0, daz = 0.0; r && n < NStep; n++, r = next_after(r)) {
	daz += r->daz;
    }
    return daz * dirn > 0.0 && fabs(daz) > MinStep;
}

/*
   Return true if elevation span daz, elevation mean mn_el and variance var_el
   suggest a RHI with sweep angle sweep_ang_req.
 */

static int is_rhi(double del, double mn_az, double var_az, double sweep_ang_req)
{
    if ( isfinite(sweep_ang_req) ) {
	return fabs(del) >= MinSpanR
	    && var_az < MaxStDv * MaxStDv
	    && fabs(diff_az(mn_az, sweep_ang_req)) < MaxStDv;
    } else {
	return fabs(del) >= MinSpanR
	    && var_az < MaxStDv * MaxStDv;
    }
}

/* Return true if NStep rays from ray maintain direction and speed */ 
static int rhi_moves(struct ray_hdr *ray, double dirn)
{
    double del;
    int n;
    struct ray_hdr *r;

    for (r = ray, n = 0, del = 0.0; r && n < NStep; n++, r = next_after(r)) {
	del += r->del;
    }
    return del * dirn > 0.0 && fabs(del) > MinStep;
}

static void init_ray(struct ray_hdr *ray)
{
    ray->i = ULONG_MAX;
    ray->next = ray->prev = NULL;
    ray->az = ray->el = ray->daz = ray->del = NAN;
    memset(ray->ln, '\0', LEN);
    memset(ray->ln, '\0', LEN);
}

/*
   Return header for the ray after curr_ray, loading it if necessary.
   If curr_ray == NULL, read the first ray.
   Ray0 might be reassigned. Return NULL at end of input.
 */ 

static struct ray_hdr *next_after(struct ray_hdr *curr_ray)
{
    static size_t num_rays_in;		/* Total number of rays read in */
    static size_t num_rays_x;		/* Number of rays that can fit
					   allocation at RayHdrs, hence
					   also the maximum number of rays in
					   a sweep */
    struct ray_hdr *next_ray;		/* Ray after curr_ray */

    if ( !RayHdrs ) {
	/*
	   Allocate an array for the ray headers sufficient for 2 * 360 degrees
	   worth of rays separated by at least AngResoln.
	 */
	num_rays_x = 2 * D360 / AngResoln;
	if ( !(RayHdrs = calloc(num_rays_x, sizeof(struct ray_hdr))) ) {
	    fprintf(stderr, "%s: could not allocate space for  %zu ray "
		    "headers\n", argv0, num_rays_x);
	    exit(EXIT_FAILURE);
	}
	Ray0 = RayHdrs;
    }

    /* If no curr_ray, assume no rays. Read the first ray and return it. */
    if ( !curr_ray ) {
	struct ray_hdr r0, r1, *ray;
	int n;

	/* Read and discard rays until antenna moves. */
	init_ray(&r0);
	init_ray(&r1);
	if ( !read_ray(&r0) ) {
	    fprintf(stderr, "%s: failed to read first ray.\n", argv0);
	    exit(EXIT_FAILURE);
	}
	r0.i = i_tot++;
	r0.az = ang_to_ref(r0.az, D180);
	r0.el = phi(r0.el);
	do {
	    if ( !read_ray(&r1) ) {
		fprintf(stderr, "%s: failed to read second ray.\n", argv0);
		exit(EXIT_FAILURE);
	    }
	    r1.i = i_tot++;
	    r1.az = ang_to_ref(r1.az, D180);
	    r1.el = phi(r1.el);
	    r1.daz = diff_az(r1.az, r0.az);
	    r1.del = r1.el - r0.el;
	    r1.prev = &r0;
	} while ( fabs(r1.daz) < AngResoln && fabs(r1.del) < AngResoln );
	*Ray0 = r1; /* Ray0->daz and Ray0->del are now defined. */

	/* Read NStep rays for initial antenna motion */ 
	for (ray = Ray0, n = 0; ray && n < NStep; n++) {
	    ray = next_after(ray);
	}

	return Ray0;
    }

    if ( !curr_ray->next ) {

	/*
	   Set next_ray to the next available slot in RayHdrs. The RayHdrs
	   array rolls. If next_ray would be after the end of the allocation
	   at RayHdrs, set next_ray to start of RayHdrs. If next_ray collides
	   with Ray0, set Ray0 to subsequent ray. next_ray then clobbers the
	   previous Ray0, making it unavailable for the current sweep. This
	   will not be a problem if num_rays_x is large enough.
	 */

	next_ray = RayHdrs + (num_rays_in + 1) % num_rays_x;
	if ( next_ray == Ray0 ) {
	    Ray0 = Ray0->next;
	}

	/*
	   Read and discard rays until the antenna moves more than AngResoln.
	   Assign the next distinct ray to next_ray. Update links.
	 */ 

	init_ray(next_ray);
	do {
	    if ( !read_ray(next_ray) ) {
		return NULL;
	    }
	    next_ray->i = i_tot++;
	    next_ray->az = ang_to_ref(next_ray->az, D180);
	    next_ray->el = phi(next_ray->el);
	    next_ray->daz = diff_az(next_ray->az, curr_ray->az);
	    next_ray->del = next_ray->el - curr_ray->el;
	} while ( fabs(next_ray->daz) < AngResoln
		&& fabs(next_ray->del) < AngResoln );

	num_rays_in++;
	next_ray->prev = curr_ray;
	curr_ray->next = next_ray;
    }
    return curr_ray->next;
}

/*
   Read a line from standard input. Exit on failure. Return 0 on eof. If line
   is a ray, store line, azimuth, and elevation into ray_hdr at ray. If not,
   pass line to standard output.
 */

static int read_ray(struct ray_hdr *ray)
{
    char ln[LEN];
    double az, el;

    if ( !fgets(ln, LEN, stdin) ) {
	if ( ferror(stdin) ) {
	    fprintf(stderr, "%s: failed to read ray.\n", argv0);
	    exit(EXIT_FAILURE);
	} else if ( feof(stdin) ) {
	    return 0;
	}
    }
    if ( sscanf(ln, " Ray %lf %lf", &az, &el) == 2 ) {
	strcpy(ray->ln, ln);
	ray->az = az;
	ray->el = el;
	return 1;
    } else {
	fputs(ln, stdout);
    }
    return 1;
}

/*
   Print rays r0 to r1, said to be from a sweep of type scan_type at
   sweep angle swp_angl
 */ 

static void print_sweep(enum SCAN_TYPE scan_type, double swp_angl,
	struct ray_hdr *r0, struct ray_hdr *r1)
{
    static long swp_idx;

    if ( !r0 || !r1 || r0 == r1 ) {
	return;
    }
    /* Print sweep information */
    printf("Sweep %3ld ", swp_idx++);
    switch (scan_type) {
	case PPI_I:
	    printf("PPI incr El %6.1lf %s\n", swp_angl, ANG_UNIT);
	    break;
	case PPI_D:
	    printf("PPI decr El %6.1lf %s\n", swp_angl, ANG_UNIT);
	    break;
	case RHI_I:
	    printf("RHI incr Az %6.1lf %s\n", swp_angl, ANG_UNIT);
	    break;
	case RHI_D:
	    printf("RHI decr Az %6.1lf %s\n", swp_angl, ANG_UNIT);
	    break;
	case UNK:
	    break;
    }
    printf("%s", r0->ln);
    printf("%s", r1->ln);
}

/* Put angle l into the interval [r - 180, r + 180) */ 
double ang_to_ref(double l, double r)
{
    double l1 = fmod(l, D360);
    l1 = (l1 < r - D180) ? l1 + D360 : (l1 >= r + D180) ? l1 - D360 : l1;
    return (l1 == -0.0) ? 0.0 : l1;
}

/* Compute az1 - az0, in degrees */ 
double diff_az(double az1, double az0)
{
    return ang_to_ref(az1, az0) - az0;
}

/* Go l degrees north of equator */
double phi(const double l)
{
    double l1 = fmod(l, 2.0 * D90);
    l1 += (l1 < 0.0) ? 2.0 * D90 : 0.0;
    return (l1 > 1.5 * D90) ? l1 - 2.0 * D90
	: (l1 > D90 ) ? D90 - l1 : l1;
}

