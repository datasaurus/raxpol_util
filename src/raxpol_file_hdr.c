/*
   -	raxpol_file_hdr.c --
   -		Print RaXPol moment file header to standard output.
   -		See raxpol_file_hdr (1).
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
 */

#include "unix_defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "raxpol.h"

#define RAXPOL_OLD_FMT "RAXPOL_OLD_FMT"

int main(int argc, char *argv[])
{
    char *argv0;			/* Name of the executable, for error
					   messages. */
    char *raxpol_fl_nm;			/* RaXPol file path */
    FILE *raxpol_fl;			/* RaXPol file */
    struct RaXPol_File_Hdr file_hdr;	/* Header from the RaXPol file */
    off_t o0;				/* Offset to start of first ray */
    off_t o1;				/* Offset at end of file */
    long ray_sz;			/* Size in file of one ray */
    struct RaXPol_Ray_Hdr ray_hdr;	/* Ray header from the RaXPol file */

    argv0 = argv[0];
    if ( atoi(getenv(RAXPOL_OLD_FMT) ? getenv(RAXPOL_OLD_FMT) : "0") ) {
	RaXPol_Old_Fmt();
    }
    if ( argc == 1 ) {
	/* raxpol_file_hdr */
	raxpol_fl_nm = "-";
    } else if ( argc == 2 ) {
	if ( strcmp(argv[1], "-V") == 0 ) {
	    printf("%s\n", RAXPOL_MOMENT_VERSION);
	    exit(EXIT_SUCCESS);
	} else if ( strcmp(argv[1], "-l") == 0 ) {
	    /* raxpol_file_hdr -l */
	    raxpol_fl_nm = "-";
	    RaXPol_Old_Fmt();
	} else {
	    /* raxpol_file_hdr raxpol_file */
	    raxpol_fl_nm = argv[1];
	}
    } else if ( argc == 3 && strcmp(argv[1], "-l") == 0 ) {
	/* raxpol_file_hdr -l raxpol_file */
	RaXPol_Old_Fmt();
	raxpol_fl_nm = argv[2];
    } else {
	fprintf(stderr, "Usage: %s raxpol_file\n", argv0);
	exit(EXIT_FAILURE);
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
    printf("************************ file header ************************\n");
    RaXPol_FPrintf_File_Hdr(&file_hdr, stdout);
    if ( (o0 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not determine file position after reading "
		"file header.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( !RaXPol_Read_Ray_Hdr(&ray_hdr, raxpol_fl) ) {
	fprintf(stderr, "%s: failed to read first ray header.\n", argv0);
	exit(EXIT_FAILURE);
    }
    if ( (o1 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not determine file position after reading "
		"first ray header.\n", argv0);
	exit(EXIT_FAILURE);
    }
    ray_sz = o1 - o0 + ray_hdr.data_size;
    if ( fseek(raxpol_fl, 0, SEEK_END) == -1
	    || (o1 = ftello(raxpol_fl)) == -1 ) {
	fprintf(stderr, "%s: could not set position for last ray of file.\n",
		argv0);
	exit(EXIT_FAILURE);
    }
    o1 = o0 + ((o1 - o0) / ray_sz) * ray_sz;
    printf("num_rays %zd\n", (size_t)((o1 - o0) / ray_sz));
    printf("********************* first ray *********************\n");
    printf("Offset %lu\n", (unsigned long)o0);
    RaXPol_FPrint_Ray_Hdr(&ray_hdr, stdout);
    printf("********************** last ray *********************\n");
    printf("Offset %lu\n", (unsigned long)o1);
    if ( fseek(raxpol_fl, o1 - ray_sz, SEEK_SET) == -1 ) {
	fprintf(stderr, "%s: could not set position for last ray of file.\n",
		argv0);
	exit(EXIT_FAILURE);
    }
    if ( !RaXPol_Read_Ray_Hdr(&ray_hdr, raxpol_fl) ) {
	fprintf(stderr, "%s: failed to read last ray header.\n", argv0);
	exit(EXIT_FAILURE);
    }
    RaXPol_FPrint_Ray_Hdr(&ray_hdr, stdout);
    return EXIT_SUCCESS;
}

