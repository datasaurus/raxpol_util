#!/bin/sh
#
#	raxpol_sweep.cgi --
#		Read RaXPlo sweep parameters from QUERY_STRING.  Make a SVG
#		image and send it to standard output.
#
# QUERY_STRING must be
#	var=value&var=value ...
# where var is one of:
#	case_id		usually, but not necessarily, MMDDYY of case
#	vol_id		volume identifier, YYYYMMDD-HHMMSS
#	swp_angl	desired sweep angle. Angle in degrees or "default".
#	data_type	RaXPol data type, e.g. "DBZ"

set -e
trap 'if [ "$?" -gt 0 ];then echo "$fail_out";fi' EXIT
fail_out="<svg><desc id=\"no_more_sweeps\">No more sweeps</desc></svg>"
printf 'Content-type: image/svg+xml\n\n'

# Web server root must have directories cgi-bin, bin, share/colors and
# img.
# cgi-bin has this script.
# bin has executables that this script calls.
# share/colors has color files for raxpol_sweep_svg.
# img must have a subdirectory for each case, also named MMDD, with a
# file named vol_list. vol_list contains raxpol_mk_vols output. raxpol_idx_html
# makes it. This script will make a directory in the case directory for each
# volume, named RAXPOL-YYYYMMDD-HHMMSS (RAXPOL- volume start time), with images
# of the volume.

# mini-httpd launches CGI scripts in cgi-bin directory.
# Go up one directory to get to server root.
# Adjust for other web servers.
cd ..
root=`pwd`

# raxpol_sweep_svg images get their behavior from
# /usr/local/share/raxpol/raxpol_sweep.js by default.
# They get style directives from
# /usr/local/share/raxpol/raxpol_sweep.css by default.
# mini-httpd instead uses local server root, so the js and css files should
# be /share/raxpol/raxpol_sweep.js and /share/raxpol/raxpol_sweep.css.
# Replace default paths with server root paths in server response, printed
# below. Adjust for other web servers.
raxpol_sweep_js="/share/raxpol/raxpol_sweep.js"
raxpol_sweep_css="/share/raxpol/raxpol_sweep.css"

# Send standard error to a log file.
log_dir=${root}/log
log_fl=${log_dir}/raxpol_sweep.err
mkdir -p $log_dir
touch $log_fl
exec 2>> $log_fl

# Parse query string. Purge $ and `` syntax.
# '$' = 044 back quote '`' = 140.
QUERY_STRING=`echo $QUERY_STRING | tr -d '\044\140'`
{
    IFS='&'
    for asgn in $QUERY_STRING
    do
	eval $asgn
    done
}
if ! test "$vol_id"
then
    echo "$0: raxpol volume identifier not set" 1>&2
    echo "$fail_out"
    exit 1
fi
if ! test "$swp_angl"
then
    echo "$0: raxpol sweep angle not set" 1>&2
    echo "$fail_out"
    exit 1
fi
if ! test "$data_type"
then
    echo "$0: raxpol data type not set" 1>&2
    echo "$fail_out"
    exit 1
fi
PATH="${root}/bin:${root}/cgi-bin:${PATH}"
export PATH

# Identify local directories
case_img_dir=${root}/img/${case_id}	# img/MMDDYY
case_vol_list=${case_img_dir}/vol_list	# raxpol_mk_vols output
vol_img_dir=`printf '%s/RAXPOL-%s' $case_img_dir $vol_id`
color_fl="${root}/share/raxpol/colors/${data_type}.clrs"
if ! test -f "$color_fl"
then
    echo "$0: No color file named $color_fl" 1>&2
    echo "$fail_out"
    exit 1
fi

# raxpol_sweep.awk reads the case volume list. Its output must set scan_mode,
# nr_swp_num_rays, and swp_angl.
eval `raxpol_sweep.awk -v vol=$vol_id -v swp_angl=$swp_angl $case_vol_list`
if [ "$nr_swp_num_rays" -eq 0 ]
then
    echo "$0: Could not find sweep" 1>&2
    echo "$fail_out"
    exit 1
fi
if [ $scan_mode != "PPI" -a $scan_mode != "RHI" ]
then
    echo "$0: could not determine scan mode" 1>&2
    echo "$fail_out"
    exit 1
fi

# Run raxpol_sweep_svg with noclobber option (-n). If image already exists,
# it just puts the name of the image file into img_path. If sweep or volume
# parameters change, delete all images to force creation of new images for the
# new parameter set.
mkdir -p $vol_img_dir
cd $vol_img_dir
img_path=`raxpol_sweep_svg -n -r $root -c $color_fl $data_type $swp_angl \
	 $vol_id < $case_vol_list`

# Send the file named img_path as server response. img_path has raxpol_sweep_svg
# output. raxpol_sweep_svg just makes an image for local viewing. awk splices in
# additional elements for web browsing.
awk -v case_id=$case_id -v raxpol_sweep_js=$raxpol_sweep_js \
	       -v raxpol_sweep_css=$raxpol_sweep_css '
    /next_vol/ {
	if ( length(case_id) > 0 ) {
	    printf "<desc id=\"case_id\">%s</desc>\n", case_id;
	}
    }
    /href=.*raxpol_sweep\.css/ {
	$0 = sprintf("<?xml-stylesheet href=\"%s\" type=\"text/css\"?>\n", \
		raxpol_sweep_css);
    }
    /href=.*raxpol_sweep\.js/ {
	$0 = sprintf("    xlink:href=\"%s\"\n", raxpol_sweep_js);
    }
    /END OF ELEMENTS FOR SCRIPTS/ {
	printf "<!-- raxpol_menus group is parent of pop up menus,";
	printf " must be near top for menus to be visible. -->\n";
	printf "<g id=\"raxpol_menus\"></g>\n";
	printf "\n";
	printf "<!-- \"updating\" element shows when page is updating -->\n";
	printf "<svg\n";
	printf "    id=\"updating\"\n";
	printf "    visibility=\"hidden\"\n";
	printf "    display=\"none\"\n";
	printf "    x=\"120.0\"\n";
	printf "    y=\"40.0\"\n";
	printf "    width=\"180.0\"\n";
	printf "    height=\"48.0\"\n";
	printf "    viewBox=\"0.0 0.0 180.0 48.0\" >\n";
	printf "    <rect\n";
	printf "	x=\"0.0\"\n";
	printf "	y=\"0.0\"\n";
	printf "	width=\"180.0\"\n";
	printf "	height=\"48.0\"\n";
	printf "	fill=\"black\" />\n";
	printf "    <text\n";
	printf "	x=\"90.0\"\n";
	printf "	y=\"24.0\"\n";
	printf "	font-size=\"16.0\"\n";
	printf "	stroke=\"white\"\n";
	printf "	text-anchor=\"middle\"\n";
	printf "	dominant-baseline=\"mathematical\" >\n";
	printf "Updating...\n";
	printf "    </text>\n";
	printf "</svg>\n";
    }
    {
	print;
    }
' $img_path

