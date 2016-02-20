/*
   -	raxpol_dat.c --
   -		Write raxpol ray data. See raxpol_dat (1).
   -
   .	Copyright (c) 2014, Gordon D. Carrie. All rights reserved.
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
#include <errno.h>
#include <limits.h>
#include "alloc.h"
#include "tm_calc_lib.h"
#include "raxpol.h"

static char *argv0;			/* Name of the executable */

#define FFMT "%.5g "			/* Float format */

#define NUM_OUT_MAX 11			/* Number of output fields */

#define RAXPOL_OLD_FMT "RAXPOL_OLD_FMT"

/* Local functions */ 
static void fprintf_field(float *, size_t, char *m, FILE *);
static void fwrite_field(float *, size_t, char *, FILE *);
static float *alloc_field_f(char *, size_t);

int main(int argc, char *argv[])
{
    char *argv0;			/* argv[0] */
    extern char *optarg;		/* See getopt (3) */
    extern int optind;			/* See getopt (3) */
    int c;				/* Index into argv */
    int ray_hdrs = 0;			/* If true, print ray headers, too */
    long r0, num_rays;			/* First ray, number of rays to print */
    long num_rays_max;			/* Number of rays from r0 to EOF */
    char *raxpol_fl_nm;			/* RaXPol file path */
    FILE *raxpol_fl;			/* RaXPol file */
    double dflt_hdg;			/* Default heading */
    long r;				/* Ray index in loop */
    struct RaXPol_Ray_Hdr ray_hdr;	/* Ray header */ 
    off_t o0;				/* Offset to start of first ray */
    off_t o;				/* Offset somewhere in raxpol_fl */
    off_t ray_sz;			/* Size in file of one ray */

    int num_gates;			/* Number of gates */
    struct RaXPol_Data dat;		/* Data for one ray */

    /*
       out_dat = array of all moment names, moment arrays, and functions to
       compute the moments from input ray data.
     */

    struct {
	char *nm;
	float *f;
	int (*calc)(struct RaXPol_Data *, float *);
    } out_dat[NUM_OUT_MAX] = {
	{"DBMHC", NULL, NULL},
	{"DBMVC", NULL, NULL},
	{"DBZ", NULL, NULL},
	{"DBZ1", NULL, NULL},
	{"VEL", NULL, NULL},
	{"ZDR", NULL, NULL},
	{"PHIDP", NULL, NULL},
	{"RHOHV", NULL, NULL},
	{"STD", NULL, NULL},
	{"SNRHC", NULL, NULL},
	{"SNRVC", NULL, NULL}
    };

    int n;				/* Index from out_dat */
    size_t num_out = NUM_OUT_MAX;	/* Number of output moments */
    char *oa;				/* Point into opt_arg */
    char *optarg_m;			/* List of moments from command line */
    char *mm; 				/* Point into optarg_m */
    size_t len;				/* strlen(optarg_m) */
    char *all_moments
	= "DBMHC DBMVC DBZ DBZ1 VEL ZDR PHIDP RHOHV STD SNRHC SNRVC";

    /* Field output */
    int (*prhdr)(struct RaXPol_Ray_Hdr *, FILE *);
    void (*prfld)(float *, size_t, char *m, FILE *);

    argv0 = argv[0];
    r0 = 0;
    num_rays = LONG_MAX;
    prhdr = RaXPol_FPrint_Ray_Hdr;
    prfld =  fprintf_field;
    if ( atoi(getenv(RAXPOL_OLD_FMT) ? getenv(RAXPOL_OLD_FMT) : "0") ) {
	RaXPol_Old_Fmt();
    }
    while ((c = getopt(argc, argv, ":Vblrh:m:s:c:")) != -1) {
	switch(c) {
	    case 'V':
		printf("%s\n", RAXPOL_MOMENT_VERSION);
		exit(EXIT_SUCCESS);
		break;
	    case 'b':
		prhdr = RaXPol_FWrite_Ray_Hdr;
		prfld =  fwrite_field;
		break;
	    case 'r':
		ray_hdrs = 1;
		break;
	    case 'l':
		RaXPol_Old_Fmt();
		break;
	    case 'h':
		if ( sscanf(optarg, "%lf", &dflt_hdg) != 1 ) {
		    fprintf(stderr, "%s: expected float value for default "
			    "heading, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		RaXPol_Set_Hdg(dflt_hdg);
		break;
	    case 'm':
		/*
		   User has requested a list of moments from command line.
		   Copy optarg to optarg_m, replacing ',' separators with
		   nul string terminators. Assign the resulting strings
		   in optarg_m to the nm members in out_dat.
		 */

		len = strlen(optarg);
		if ( !(optarg_m = CALLOC(len + 1, 1)) ) {
		    fprintf(stderr, "%s: could not allocate storage for moment "
			    "names.\n", argv0);
		    exit(EXIT_FAILURE);
		}
		for (oa = optarg, mm = optarg_m; *oa; oa++, mm++) {
		    *mm = ( *oa == ',' ) ? '\0' : *oa;
		}
		for (n = 0, mm = optarg_m;
			n < NUM_OUT_MAX && mm < optarg_m + len;
			mm++) {
		    if ( *mm ) {
			/* Found start of a moment name in optarg_m */
			if ( !strstr(all_moments, mm) ) {
			    fprintf(stderr, "%s: unknown moment %s. "
				    "Moment must be one of %s\n",
				    argv0, mm, all_moments);
			    exit(EXIT_FAILURE);
			}
			out_dat[n++].nm = mm;

			/* Go to nul terminator at the end of moment name */
			for (++mm; *mm; ++mm) {
			}
		    }
		}
		if ( (num_out = n) == NUM_OUT_MAX ) {
		    fprintf(stdout, "%s: too many moments (%zu). Maximum "
			    "number is %d\n", argv0, num_out, NUM_OUT_MAX);
		    exit(EXIT_FAILURE);
		}
		break;
	    case 's':
		if ( sscanf(optarg, "%ld", &r0) != 1 ) {
		    fprintf(stderr, "%s: expected integer for index of first "
			    "ray, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
	    case 'c':
		if ( sscanf(optarg, "%ld", &num_rays) != 1 ) {
		    fprintf(stderr, "%s: expected integer for count, got %s\n",
			    argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		break;
	    case '?':
		fprintf(stderr, "Usage: %s [-b] [-l] [-h angle] [-s start] "
			"[-c count] [raxpol_file]\n", argv0);
		exit(EXIT_FAILURE);
		break;
	}
    }
    if ( optind ==  argc ) {
	raxpol_fl_nm = "-";
    } else if ( optind + 1 == argc ) {
	raxpol_fl_nm = argv[optind];
    } else {
	fprintf(stderr, "Usage: %s [-b] [-l] [-r] [-h angle] "
		"[-m moment,moment,...] [-s start] [-c count] [raxpol_file]",
		argv0);
	exit(EXIT_FAILURE);
    }
    if ( strcmp(raxpol_fl_nm, "-") == 0 ) {
	raxpol_fl = stdin;
    } else if ( !(raxpol_fl = fopen(raxpol_fl_nm, "r")) ) {
	fprintf(stderr, "%s: could not open %s for reading.\n",
		argv0, raxpol_fl_nm);
	exit(EXIT_FAILURE);
    }

    /* Read file header. Compute some convenience constants. */ 
    if ( !RaXPol_Init_Data(&dat, raxpol_fl) ) {
	fprintf(stderr, "%s: failed to initialize RaXPol data structure.\n",
		argv0);
	exit(EXIT_FAILURE);
    }
    num_gates = dat.file_hdr.num_rng_gates;
    if ( (o0 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not determine position in file.\n%s\n",
		argv0, strerror(errno));
	exit(EXIT_FAILURE);
    }

    /* Allocate requested output fields and assign associated calc members. */
    for (n = 0; n < num_out; n++) {
	if ( strcmp(out_dat[n].nm, "DBMHC") == 0 ) {
	    out_dat[n].f = alloc_field_f("DBMHC", num_gates);
	    out_dat[n].calc = dat.dbmhc;
	} else if ( strcmp(out_dat[n].nm, "DBMVC") == 0 ) {
	    out_dat[n].f = alloc_field_f("DBMVC", num_gates);
	    out_dat[n].calc = dat.dbmvc;
	} else if ( strcmp(out_dat[n].nm, "DBZ") == 0 ) {
	    out_dat[n].f = alloc_field_f("DBZ", num_gates);
	    out_dat[n].calc = dat.dbz;
	} else if ( strcmp(out_dat[n].nm, "DBZ1") == 0 ) {
	    out_dat[n].f = alloc_field_f("DBZ1", num_gates);
	    out_dat[n].calc = dat.dbz1;
	} else if ( strcmp(out_dat[n].nm, "VEL") == 0 ) {
	    out_dat[n].f = alloc_field_f("VEL", num_gates);
	    out_dat[n].calc = dat.vel;
	} else if ( strcmp(out_dat[n].nm, "ZDR") == 0 ) {
	    out_dat[n].f = alloc_field_f("ZDR", num_gates);
	    out_dat[n].calc = dat.zdr;
	} else if ( strcmp(out_dat[n].nm, "PHIDP") == 0 ) {
	    out_dat[n].f = alloc_field_f("PHIDP", num_gates);
	    out_dat[n].calc = dat.phidp;
	} else if ( strcmp(out_dat[n].nm, "RHOHV") == 0 ) {
	    out_dat[n].f = alloc_field_f("RHOHV", num_gates);
	    out_dat[n].calc = dat.rhohv;
	} else if ( strcmp(out_dat[n].nm, "STD") == 0 ) {
	    out_dat[n].f = alloc_field_f("STD", num_gates);
	    out_dat[n].calc = dat.std;
	} else if ( strcmp(out_dat[n].nm, "SNRHC") == 0 ) {
	    out_dat[n].f = alloc_field_f("SNRHC", num_gates);
	    out_dat[n].calc = dat.snrhc;
	} else if ( strcmp(out_dat[n].nm, "SNRVC") == 0 ) {
	    out_dat[n].f = alloc_field_f("SNRVC", num_gates);
	    out_dat[n].calc = dat.snrvc;
	}
    }

    /* Determine ray size and num_rays_max */
    if ( !RaXPol_Read_Ray_Hdr(&ray_hdr, raxpol_fl) ) {
	fprintf(stderr, "%s: failed to read header for first ray.\n",
		argv0);
	exit(EXIT_FAILURE);
    }
    if ( (o = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not determine position in file.\n%s\n",
		argv0, strerror(errno));
	exit(EXIT_FAILURE);
    }
    ray_sz = o - o0 + ray_hdr.data_size;
    if ( fseeko(raxpol_fl, 0, SEEK_END) == -1
	    || (o = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not position at end of file.\n"
		"%s\n", argv0, strerror(errno));
	exit(EXIT_FAILURE);
    }
    num_rays_max = (o - o0) / ray_sz - r0;
    if ( num_rays > num_rays_max ) {
	num_rays = num_rays_max;
    }

    /* Move to first ray */
    if ( r0 > 0 ) {
	if ( fseeko(raxpol_fl, o0 + r0 * ray_sz, SEEK_SET) == -1 ) {
	    fprintf(stderr, "%s: could not position at start of first ray.\n"
		    "%s\n", argv0, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    } else {
	if ( fseeko(raxpol_fl, o0, SEEK_SET) == -1) {
	    fprintf(stderr, "%s: could not position at end of file.\n"
		    "%s\n", argv0, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    }

    /* Read and print rays. */
    for (r = r0; r < r0 + num_rays; r++) {
	if ( !RaXPol_Read_Ray(&dat, raxpol_fl) ) {
	    if ( ferror(raxpol_fl) ) {
		fprintf(stderr, "%s: could not read ray %ld\n",
			argv0, r);
		exit(EXIT_FAILURE);
	    } else {
		exit(EXIT_SUCCESS);
	    }
	}
	if ( ray_hdrs ) {
	    if ( prhdr == RaXPol_FPrint_Ray_Hdr ) {
		printf("ray %ld\n", r);
	    }
	    prhdr(&ray_hdr, stdout);
	}
	for (n = 0; n < num_out; n++) {
	    if ( !out_dat[n].calc(&dat, out_dat[n].f) ) {
		fprintf(stderr, "%s: could not compute %s for ray %ld\n",
			argv0, out_dat[n].nm, r);
		exit(EXIT_FAILURE);
	    }
	    prfld(out_dat[n].f, num_gates, out_dat[n].nm, stdout);
	}
	printf("\n");
    }

    return 0;
}

/*
   If not "", print nm to out. Then print n ascii floats from f to out,
   followed by a newline.
 */

static void fprintf_field(float *f, size_t n, char *nm, FILE *out)
{
    float *f1;				/* End of array f */

    if ( !f || !nm ) {
	fprintf(stderr, "No data to print for field %s\n", nm ? nm : "unknown");
	exit(EXIT_FAILURE);
    }
    if ( strlen(nm) > 0 ) {
	fprintf(out, "%s ", nm);
    }
    for (f1 = f + n; f < f1; f++) {
	fprintf(out, FFMT, *f);
    }
    fprintf(out, "\n");
}

/* Write n floats from f to out. Field assumed to be named nm. */ 
static void fwrite_field(float *f, size_t n, char *nm, FILE *out)
{
    size_t sz = strlen(nm);

    if ( !f || !nm ) {
	fprintf(stderr, "No data to print for field %s\n", nm ? nm : "unknown");
	exit(EXIT_FAILURE);
    }
    if ( fwrite(&sz, sizeof(size_t), 1, out) != 1 ) {
	fprintf(stderr, "%s: could not write size of field name for %s.\n",
		argv0, nm);
	exit(EXIT_FAILURE);
    }
    if ( fwrite(nm, sz, 1, out) != 1 ) {
	fprintf(stderr, "%s: could not write field name for %s.\n", argv0, nm);
	exit(EXIT_FAILURE);
    }
    if ( fwrite(f, sizeof(float), n, out) != n ) {
	fprintf(stderr, "%s: could not write field name for %s.\n", argv0, nm);
	exit(EXIT_FAILURE);
    }
}

/* Allocate memory for an output field named nm with space for n floats */ 
static float *alloc_field_f(char *nm, size_t n)
{
    float *dat;

    if ( !(dat = calloc(n, sizeof(float))) ) {
	fprintf(stderr, "%s: could not allocate %u values for %s\n",
		argv0, (unsigned)n, nm);
	exit(EXIT_FAILURE);
    }
    return dat;
}

