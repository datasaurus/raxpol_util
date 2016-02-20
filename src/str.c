/*
   -	str.c --
   -		This file defines string manipulation
   -		functions.  See str (3).
   -	
   .	Copyright (c) 2011, Gordon D. Carrie. All rights reserved.
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
   .	$Revision: 1.26 $ $Date: 2013/01/10 21:23:36 $
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include "alloc.h"
#include "str.h"

/* Largest possible number of elements in array of strings */
static int mx = INT_MAX / (int)sizeof(char *);

char *Str_Esc(char *str)
{
    char *p, *q, t;
    int n;

    for (p = q = str; *q; p++, q++) {
	if (*q == '\\') {
	    switch (*++q) {
		/* Replace escape sequence with associated character */
		case 'a':
		    *p = '\a';
		    break;
		case 'b':
		    *p = '\b';
		    break;
		case 'f':
		    *p = '\f';
		    break;
		case 'n':
		    *p = '\n';
		    break;
		case 'r':
		    *p = '\r';
		    break;
		case 't':
		    *p = '\t';
		    break;
		case 'v':
		    *p = '\v';
		    break;
		case '\'':
		    *p = '\'';
		    break;
		case '\\':
		    *p = '\\';
		    break;
		case '0':
		    /* Escape sequence is a sequence of octal digits. */
		    n = (int)strspn(++q, "01234567");
		    t = *(q + n);
		    *(q + n) = '\0';
		    *p = (char)strtoul(q, &q, 8);
		    *q-- = t;
		    break;
		default:
		    *p = *q;
	    }
	} else {
	    *p = *q;
	}
    }
    *p = '\0';

    return str;
}

char ** Str_Words(char *l, char **argv, int *argc_p)
{
    int argc_max;		/* Number of words that can be stored at argv */
    int argc = 0;		/* Number of words in the line */
    size_t sz;			/* Temporary */
    char **t;			/* Temporary */

    if (argv) {
	argc_max = *argc_p;
	if (argc_max < 2) {
	    if ( !(t = (char **)REALLOC(argv, 2 * sizeof(char *))) ) {
		FREE(argv);
		*argc_p = -1;
		fprintf(stderr, "Could not allocate word array.\n");
		return NULL;
	    }
	    argv = t;
	    argc_max = 2;
	}
    } else {
	argc_max = 2;
	if ( !(argv = (char **)CALLOC((size_t)argc_max, sizeof(char *))) ) {
	    *argc_p = -1;
	    fprintf(stderr, "Could not allocate word array.\n");
	    return NULL;
	}
    }
    while ( *l ) {
	if ( isspace(*l) ) {
	    /* Skipping whitespace */

	    l++;
	} else {
	    /* Starting a new word */

	    if ( argc + 1 > argc_max ) {
		/* argv needs more memory */
		int argc_max_t;

		argc_max_t = 3 * argc_max / 2 + 4;
		sz = (size_t)argc_max_t * sizeof(char *);
		if (argc_max_t > mx || !(t = (char **)REALLOC(argv, sz)) ) {
		    FREE(argv);
		    *argc_p = -1;
		    fprintf(stderr, "Could not allocate word array.\n");
		    return NULL;
		}
		argv = t;
		argc_max = argc_max_t;
	    }
	    if ( *l == '"' || *l == '\'' ) {
		/* Word is a run of characters bounded by quotes */
		char q = *l;

		argv[argc++] = ++l;
		while ( *l && *l != q ) {
		    l++;
		}
		if ( !*l ) {
		    fprintf(stderr, "Unbalanced quote.\n");
		    FREE(argv);
		    *argc_p = -1;
		    return NULL;
		}
		*l++ = '\0';
	    } else {
		/* Word is a run of non-whitespace characters */
		argv[argc++] = l;
		while ( *l && !isspace(*l) ) {
		    l++;
		}
		if ( isspace(*l) ) {
		    *l++ = '\0';
		}
	    }
	}
    }
    *argc_p = argc;
    sz = (size_t)(argc + 1) * sizeof(char *);
    if ( (argc + 1) > mx || !(t = (char **)REALLOC(argv, sz)) ) {
	FREE(argv);
	*argc_p = -1;
	fprintf(stderr, "Could not allocate word array.\n");
	return NULL;
    }
    argv = t;
    argv[argc] = NULL;
    return argv;
}

char * Str_Append(char *dest, size_t *l, size_t *lx, char *src, size_t n)
{
    char *t;
    size_t lx2;

    lx2 = *l + n + 1;
    if ( *lx < lx2 ) {
	size_t tx = *lx;

	while (tx < lx2) {
	    tx = (tx * 3) / 2 + 4;
	}
	if ( !(t = REALLOC(dest, tx)) ) {
	    fprintf(stderr, "Could not grow string for appending.\n");
	    return NULL;
	}
	dest = t;
	*lx = tx;
    }
    strncpy(dest + *l, src, n);
    *l += n;
    return dest;
}

int Str_GetLn(FILE *in, char eol, char **ln, size_t *l_max)
{
    int i;			/* Input character */
    char c;			/* Input character */
    char *t, *t1;		/* Input line */
    size_t n;			/* Number of characters in ln */
    size_t nx;			/* Temporarily hold value for *l_max */

    if ( !*ln ) {
	nx = 4;
	if ( !(t = (char *)MALLOC((size_t)nx)) ) {
	    fprintf(stderr, "Could not allocate memory for line.\n");
	    return 0;
	}
    } else {
	t = *ln;
	nx = *l_max;
    }
    n = 0;
    while ( (i = fgetc(in)) != EOF && (i != eol) ) {
	c = i;
	if ( !(t1 = Str_Append(t, &n, &nx, &c, (size_t)1)) ) {
	    FREE(t);
	    fprintf(stderr, "Could not append input character to string.\n");
	    return 0;
	}
	t = t1;
    }
    if ( !(*ln = (char *)REALLOC(t, (size_t)(n + 1))) ) {
	FREE(t);
	*ln = NULL;
	*l_max = 0;
	fprintf(stderr, "Could not reallocate memory for line.\n");
	return 0;
    }
    *(*ln + n) = '\0';
    *l_max = n + 1;
    if ( ferror(in) ) {
	perror(NULL);
	return 0;
    } else if ( feof(in) ) {
	return EOF;
    } else {
	return 1;
    }
}
