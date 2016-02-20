#!/usr/bin/awk -f
#
#	Fetch sweep information from volume index, given in standard input.
#
# The following variables must be set at invocation:
#	vol		- volume specifier, of form raxpol_fl,YYYYMMDD-HHMMSS
#	                  where raxpol_fl is path to a raxpol file, vol_idx is
#			  the index of a volume in raxpol_fl.
#	swp_angl	- desired sweep angle if volume is a PPI. Actual sweep
#		          angle will be for nearest sweep.
# Output, printed in form "key=value"
#	nr_swp_ray0	= index raxpol_fl of first ray in sweep
#	nr_swp_num_rays = ray count
#       nr_swp_dirn	= near sweep direction, "incr" or "decr", for increasing
#			  or decreasing, respectively.
#       nr_swp_ymd	= near sweep YYYY/MM/DD
#       nr_swp_hms	= near sweep HH:MM:SS
#	swp_angl	= actual sweep angle
#	sweep_angles	= list of sweeps in volume
#	prev_vol	= previous volume, as "raxpol_file_name,index"
#	next_vol	= next volume, as "raxpol_file_name,index"

function fabs(x)
{
    return (x > 0.0) ? x : -x;
}
BEGIN {
    prev_vol = "";
    next_vol = "";
    nr_swp_ray0 = 0;
    nr_swp_num_rays = 0;
    swp_idx = 0;
}
$1 == "File" {
    raxpol_path = $2;
}
$1 == "Vol" && $4 == vol {
    # Found vol
    scan_mode = $3;
    da = 361.0;
    swp_idx = 0;

    # Read sweep and ray information for vol
    while (getline && length(next_vol) == 0) {

	# Make lists of sweep angles, indeces of first ray in each
	# sweep, and sweep ray counts.
	if ( $1 == "Sweep" && length(next_vol) == 0 ) {
	    dirn[swp_idx] = $4;
	    sweep_angles[swp_idx] = $6;
	    ymd[swp_idx] = $9;
	    hms[swp_idx] = $10;
	    ray0[swp_idx] = $12;
	    num_rays[swp_idx] = $14 - ray0[swp_idx] + 1;
	    swp_idx++;
	}

	# Search for next volumes after vol.
	if ( $1 == "Vol" ) {
	    if ( length(next_vol) == 0 ) {
		next_scan_mode = $3;
		next_vol = $4;
	    }
	}
    }
    num_sweeps = swp_idx;

    # If request is "default", use low PPI tilt of middle of RHI.
    # If request is a number, search vol for sweep nearest request.
    if ( scan_mode == "PPI" && swp_angl == "default" ) {
	swp_idx = 0;
    } else if ( scan_mode == "RHI" && swp_angl == "default" ) {
	swp_idx = int(num_sweeps / 2);
    } else {
	for (s = 0; s < num_sweeps; s++) {
	    da_new = fabs(sweep_angles[s] - swp_angl);
	    if ( da_new < da ) {
		swp_idx = s;
		da = da_new;
	    }
	}
    }
    nr_swp_ray0 = ray0[swp_idx];
    nr_swp_num_rays = num_rays[swp_idx];
    nr_swp_dirn = dirn[swp_idx];
    nr_swp_ymd = ymd[swp_idx];
    nr_swp_hms = hms[swp_idx];
    swp_angl_actl = sweep_angles[swp_idx];
    exit
}
$1 == "Vol" {
    prev_scan_mode = $3;
    prev_vol = $4;
}
$1 == "Config" {
    $1 = "";
    config = $0;
    sub(/^[ 	]*/, "", config);
    sub(/[ 	]*$/, "", config);
}
END {
    printf "raxpol_path=%s\n", raxpol_path
    printf "scan_mode=%s\n", scan_mode;
    if (length(prev_vol) == 0) {
	printf "prev_vol=none%%none\n";
    } else {
	printf "prev_vol=%s%%%s\n", prev_vol, prev_scan_mode;
    }
    if (length(next_vol) == 0) {
	printf "next_vol=none%%none\n";
    } else {
	printf "next_vol=%s%%%s\n", next_vol, next_scan_mode;
    }
    printf "nr_swp_ray0=%d\n", nr_swp_ray0;
    printf "nr_swp_num_rays=%d\n", nr_swp_num_rays;
    printf "nr_swp_dirn=%s\n", nr_swp_dirn;
    printf "nr_swp_ymd=%s\n", nr_swp_ymd;
    printf "nr_swp_hms=%s\n", nr_swp_hms;
    printf "swp_angl=%s\n", swp_angl_actl;
    printf "swp_idx=%d\n", swp_idx;
    printf "sweep_angles=\"";
    w = "";
    for (s = 0; s < num_sweeps; s++) {
	printf "%s%.1f", w, sweep_angles[s];
	w = " ";
    }
    printf "\"\n";
    printf "config=\"%s\"\n", config;
}
