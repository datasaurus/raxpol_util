/*
   -	raxpol_ray_hdrs.c --
   -		This program defines an application prints file header
   -		and ray headers from a raxpol file.
   .
   .	Usage:
   .		raxpol_ray_hdrs [-a] [-l] [-h angle] [-s start] [-c count]
   .			[raxpol_file]
   .
   .	Options:
   .		-a requests abbreviated output.
   .		-l input file is "old" (2011) format.
   .		-h sets heading, overriding GPS heading in ray headers.
   .		-s index of first ray to read. First ray is 0.
   .		-c number of rays to read
   .
   .	Non-zero RAXPOL_OLD_FMT environment variable is same as -l.
   .
   .	Reading proceeds until count or end of file.
   .
   .	Ray headers are written to standard output.
   .
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
   .	$Revision: 1.8 $ $Date: 2015/02/10 21:07:56 $
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "raxpol.h"

#define RAXPOL_OLD_FMT "RAXPOL_OLD_FMT"

int main(int argc, char *argv[])
{
    char *argv0;			/* Name of the executable, for error
					   messages. */
    int c;				/* Index into argv */
    extern char *optarg;		/* See getopt (3) */
    double dflt_hdg;			/* Default heading */
    char *raxpol_fl_nm;			/* RaXPol file path */
    FILE *raxpol_fl;			/* RaXPol file */
    long r0, num_rays;			/* First ray, number of rays to read */
    long num_rays_max;			/* Number of rays from r0 to EOF */
    off_t o0;				/* Offset to start of first ray */
    off_t o;				/* Offset somewhere in raxpol_fl */
    off_t ray_sz;			/* Size in file of one ray */
    int abbrv;				/* If true, abbreviate */
    struct RaXPol_File_Hdr file_hdr;	/* Header from the RaXPol file */
    struct RaXPol_Ray_Hdr ray_hdr;
    long r;

    argv0 = argv[0];
    abbrv = 0;
    r0 = 0;
    num_rays = LONG_MAX;
    if ( atoi(getenv(RAXPOL_OLD_FMT) ? getenv(RAXPOL_OLD_FMT) : "0") ) {
	RaXPol_Old_Fmt();
    }
    while ((c = getopt(argc, argv, ":Valh:s:c:")) != -1) {
	switch(c) {
	    case 'V':
		printf("%s\n", RAXPOL_MOMENT_VERSION);
		exit(EXIT_SUCCESS);
		break;
	    case 'a':
		abbrv = 1;
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
	    case 's':
		if ( sscanf(optarg, "%ld", &r0) != 1 ) {
		    fprintf(stderr, "%s: expected integer for index of first "
			    "ray, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
	    case 'c':
		if ( sscanf(optarg, "%ld", &num_rays) != 1 ) {
		    fprintf(stderr, "%s: expected integer for desired ray "
			    "count, got %s\n", argv0, optarg);
		    exit(EXIT_FAILURE);
		}
		break;
	    case '?':
		fprintf(stderr, "Usage: %s [-a] [-s YYYYMMDD-HHMMSS] "
			"[-e YYYYMMDD-HHMMSS] [raxpol_file]\n", argv0);
		exit(EXIT_FAILURE);
		break;
	}
    }
    if ( optind == argc ) {
	raxpol_fl_nm = "-";
    } else {
	raxpol_fl_nm = argv[optind];
    }
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

    /* Determine ray size and num_rays_max */
    RaXPol_Init_Ray_Hdr(&ray_hdr);
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

    /* Print ray headers */
    if ( !abbrv ) {
	printf("Reading %s\n",
		(raxpol_fl == stdin) ? "standard input" : raxpol_fl_nm);
	printf("File header:\n");
	RaXPol_FPrintf_File_Hdr(&file_hdr, stdout);
    }
    for (r = r0;
	    r < r0 + num_rays && RaXPol_Read_Ray_Hdr(&ray_hdr, raxpol_fl);
	    r++) {
	if ( abbrv ) {
	    printf("ray %-9ld ", r);
	    RaXPol_FPrint_Abbrv_Ray_Hdr(&ray_hdr, stdout);
	} else {
	    printf("******************* ray *******************\n");
	    printf("ray %ld\n", r);
	    RaXPol_FPrint_Ray_Hdr(&ray_hdr, stdout);
	}
	if ( fseeko(raxpol_fl, ray_hdr.data_size, SEEK_CUR) == -1 ) {
	    fprintf(stderr, "%s: could not skip ray data\n%s\n",
		    argv0, strerror(errno));
	    exit(EXIT_FAILURE);
	}
    }
    if ( feof(raxpol_fl) || ferror(raxpol_fl) ) {
	fprintf(stderr, "%s: failed to read ray header for ray %ld of %s\n",
		argv0, r, raxpol_fl_nm);
	exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

