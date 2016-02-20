#!/usr/bin/awk -f
#
#	pisa.awk --
#		Create a SVG document with a cartesian plot in it.
#		See pisa(1).
#
################################################################################
#
# Copyright (c) 2013, Gordon D. Carrie. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#     * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Please send feedback to dev0@trekix.net
#
################################################################################

# Initialize parameters with bogus values or reasonable defaults
BEGIN {
    svg_width_px = "nan"
    svg_height_px = "nan";
    top_mgn_px = 0.0;
    rt_mgn_px = 0.0;
    btm_mgn_px = 0.0;
    left_mgn_px = 0.0;
    x_left = "nan";
    x_right = "nan";
    y_bottom = "nan";
    y_top = "nan";
    x_prx = "3";
    y_prx = "3";
    x_title = "";
    y_title = "";
    font_px = 12.0;
    tick_len_px = 0.5 * font_px;
    pad_px = 0.5 * font_px;		# Separator between axis elements
    err = "/dev/stderr";
}

# Set parameters from standard input
/svg_width:/ {
    svg_width_px = $2 + 0.0;
    if ( svg_width_px <= 0.0 ) {
	printf "document width must be positive\n" > err;
	exit 1;
    }
}
/svg_height:/ {
    svg_height_px = $2 + 0.0;
    if ( svg_height_px <= 0.0 ) {
	printf "document height must be positive\n" > err;
	exit 1;
    }
}
/top_mgn:/ {
    top_mgn_px = $2 + 0.0;
    if ( top_mgn_px < 0.0 ) {
	printf "top margin cannot be negative\n" > err;
	exit 1;
    }
}
/right_mgn:/ {
    rt_mgn_px = $2 + 0.0;
    if ( rt_mgn_px < 0.0 ) {
	printf "right margin cannot be negative\n" > err;
	exit 1;
    }
}
/bottom_mgn:/ {
    btm_mgn_px = $2 + 0.0;
    if ( btm_mgn_px < 0.0 ) {
	printf "bottom margin cannot be negative\n" > err;
	exit 1;
    }
}
/left_mgn:/ {
    left_mgn_px = $2 + 0.0;
    if ( left_mgn_px < 0.0 ) {
	printf "left margin cannot be negative\n" > err;
	exit 1;
    }
}
/x_left:/ {
    x_left = $2 + 0.0;
}
/x_right:/ {
    x_right = $2 + 0.0;
}
/y_bottom:/ {
    y_bottom = $2 + 0.0;
}
/y_top:/ {
    y_top = $2 + 0.0;
}
/font_sz:/ {
    font_px = $2 + 0.0;
    if ( font_px <= 0.0 ) {
	printf "font size must be positive\n" > err;
	exit 1;
    }
}
/x_title:/ {
    $1 = "";
    sub(" *", "");
    x_title = $0;
}
/y_title:/ {
    $1 = "";
    sub(" *", "");
    y_title = $0;
}
/x_prx:/ {
    x_prx = $2;
}
/y_prx:/ {
    y_prx = $2;
}

# This function returns the next power of 10 greater than or equal to the
# magnitude of x.
function pow10(x)
{
    if (x == 0.0) {
	return 1.0e-100;
    } else if (x > 0.0) {
	n = int(log(x) / log(10) + 0.5);
	return exp(n * log(10.0));
    } else {
	n = int(log(-x) / log(10) + 0.5);
	return -exp(n * log(10.0));
    }
}

# Copy src array to dest
function copy_arr(dest, src)
{
    for (i in dest) {
	delete dest[i];
    }
    for (i in src) {
	dest[i] = src[i];
    }
}

# axis_lbl --
#	This function determines axis label locations.
#	x_lo	(in)	start of axis.
#	x_hi	(in)	end of axis.
#	prx	(in) 	number of significant digits in each label.
#	n_max	(in)	number of characters allowed for all labels.
#	orient  (in)	orientation, "h" or "v" for horizontal or vertical.
#	labels	(out)	Label coordinates and strings are returned in labels.
#			Each index is an x coordinate. Corresponding value is
#			the string to print there.
#	l0, l1, dx, t	local variables
#
function axis_lbl(x_lo, x_hi, prx, n_max, orient, labels,
	l0, l1, dx, t)
{
    if ( x_hi < x_lo ) {
	t = x_hi;
	x_hi = x_lo;
	x_lo = t;
    }

#   Put a tentative number of labels into l0.
#   Put more labels into l1. If l1 would need more than n_max
#   characters, return l0. Otherwise, copy l1 to l0 and try
#   a more populated l1.
    fmt="%."prx"g";
    l0[x_lo] = sprintf(fmt, x_lo);
    if ( x_lo == x_hi || length(sprintf(fmt, x_lo)) > n_max ) {
	copy_arr(labels, l0);
	return;
    }
    l0[x_hi] = sprintf(fmt, x_hi);
    if ( length(sprintf(fmt " " fmt, x_lo, x_hi)) > n_max ) {
	copy_arr(labels, l0);
	return;
    }

#   Initialize the interval dx to a power of 10 larger than the interval
#   from x_hi - x_lo, then try smaller steps until all of the labels with
#   a space character between them fit into n_max characters. The interval
#   will be a multiple of 10, 5, or 2 times some power of 10.
    dx = pow10(x_hi - x_lo);
    while (1) {
	if ( mk_lbl(x_lo, x_hi, dx, fmt, orient, l1) > n_max ) {
	    copy_arr(labels, l0);
	    return;
	} else {
	    copy_arr(l0, l1);
	}
	dx *= 0.5;
	if ( mk_lbl(x_lo, x_hi, dx, fmt, orient, l1) > n_max ) {
	    copy_arr(labels, l0);
	    return;
	} else {
	    copy_arr(l0, l1);
	}
	dx *= 0.4;
	if ( mk_lbl(x_lo, x_hi, dx, fmt, orient, l1) > n_max ) {
	    copy_arr(labels, l0);
	    return;
	} else {
	    copy_arr(l0, l1);
	}
	dx *= 0.5;
    }
}

# Functions for floor and ceiling
function floor(x)
{
    return (x > 0) ? int(x) : int(x) - 1;
}
function ceil(x) {
    return (x > 0) ? int(x) + 1 : int(x);
}

# Print labels from x_lo to x_hi with increment dx and print format fmt
# to a string. Assign the label coordinates and label strings to the labels
# array.  Each index in labels array will be an x coordinate. Array value will
# be the label to print there. If orient is "h", return the length of the
# string containing all labels. Otherwise, assume the axis is vertical and
# return the number of labels.

function mk_lbl(x_lo, x_hi, dx, fmt, orient, labels, l, x0, n, n_tot)
{
    for (l in labels) {
	delete labels[l];
    }
    x0 = floor(x_lo / dx) * dx;
    x_lo -= dx / 4;
    x_hi += dx / 4;
    for (n = n_tot = 0; n <= ceil((x_hi - x_lo) / dx); n++) {
	x = x0 + n * dx;
	if ( x >= x_lo && x <= x_hi ) {
	    labels[x] = sprintf(fmt, x);
	    if ( orient == "h" ) {
		n_tot += length(labels[x]);
	    } else {
		n_tot++;
	    }
	}
    }
    return n_tot;
}

# fabs (3)
function fabs(x)
{
    return (x > 0.0) ? x : -x;
}

# Validate input. Compute geometry. Print start of svg document.
/start_svg:/ {
    if ( !(svg_width_px > 0.0) ) {
	printf "SVG element width must be positive width" > err;
	exit 1;
    }
    if ( x_left == "nan" ) {
	printf "x_left not set\n" > err;
	exit 1;
    }
    if ( x_right == "nan" ) {
	printf "x_right not set\n" > err;
	exit 1;
    }
    if ( y_bottom == "nan" ) {
	printf "y_bottom not set\n" > err;
	exit 1;
    }
    if ( y_top == "nan" ) {
	printf "y_top not set\n" > err;
	exit 1;
    }
    if ( x_right == x_left ) {
	printf "Left and right sides cannot have same x coordinate.\n" > err;
	exit 1;
    }
    if ( y_top == y_bottom ) {
	printf "Top and bottom cannot have same y coordinate.\n" > err;
	exit 1;
    }

#   Determine space requirements for axis titles
    if ( length(x_title) > 0 ) {
	x_title_ht_px = pad_px + font_px;
    } else {
	x_title_ht_px = 0.0;
    }
    if ( length(y_title) > 0 ) {
	y_title_w_px = font_px + pad_px;
    } else {
	y_title_w_px = 0.0;
    }

#   Space below plot will have tick mark, padding, label.
    x_axis_ht_px = tick_len_px + pad_px + font_px;
    below_plot = x_axis_ht_px + x_title_ht_px + btm_mgn_px;

#   Determine plot location and dimensions assuming y axis has no title
#   or labels.
    plot_x_px = left_mgn_px;
    plot_width_px = svg_width_px - left_mgn_px - rt_mgn_px;
    if ( plot_width_px <= 0 ) {
	printf "Negative plot width.\n" > err;
	printf "(svg_width=%.1f left_margin=%.1f rt_margin=%.1f)\n", 
		svg_width_px, left_mgn_px, rt_mgn_px;
	exit 1;
    }
    if ( svg_height_px == "nan" ) {
	r = fabs((y_top - y_bottom) / (x_right - x_left));
	plot_hght_px = plot_width_px * r;
    } else {
	plot_hght_px = svg_height_px - top_mgn_px - below_plot;
	if ( plot_hght_px <= 0 ) {
	    printf "Negative plot height.\n" > err;
	    exit 1;
	}
    }

#   Create a set of labels for y axis.
#   Determine width needed for y axis labels and title.
    n_max = plot_hght_px / font_px / 2;
    axis_lbl(y_bottom, y_top, y_prx, n_max, "v", y_labels);
    max_len = 0.0;
    for (y in y_labels) {
	len = length(y_labels[y]);
	if ( len > max_len ) {
	    max_len = len;
	}
    }
    y_axis_extra = 2.0 * font_px + pad_px;
    y_axis_y_px = top_mgn_px - y_axis_extra / 2.0;
    y_axis_width_px = font_px * max_len + tick_len_px;
    y_axis_x_px = left_mgn_px + y_title_w_px;

#   Adjust plot_x_px so that it includes user specified margin plus space
#   needed for y axis element. Recompute plot width and height for the new
#   left margin. Recompute labels for the new plot height. Assume, perhaps
#   naively, that space needs for the y axis do not change. This could be a bug.
    plot_x_px += y_title_w_px + y_axis_width_px;
    plot_width_px = svg_width_px - plot_x_px - rt_mgn_px;
    if ( plot_width_px <= 0 ) {
	printf "Negative plot width.\n" > err;
	exit 1;
    }
    if ( svg_height_px == "nan" ) {
	r = fabs((y_top - y_bottom) / (x_right - x_left));
	plot_hght_px = plot_width_px * r;
	svg_height_px = plot_hght_px + top_mgn_px + below_plot;
    } else {
	plot_hght_px = svg_height_px - top_mgn_px - below_plot;
	if ( plot_hght_px <= 0 ) {
	    printf "Negative plot height.\n" > err;
	    exit 1;
	}
    }
    y_axis_hght_px = plot_hght_px + y_axis_extra;
    n_max = 0.5 * plot_hght_px / font_px;
    axis_lbl(y_bottom, y_top, y_prx, n_max, "v", y_labels);

#   Create a set of labels for the x axis;
    px_per_x = plot_width_px / (x_right - x_left);
    px_per_y = plot_hght_px / (y_top - y_bottom);
    n_max = 0.5 * plot_width_px / font_px;
    axis_lbl(x_left, x_right, x_prx, n_max, "h", x_labels);

#   Compute geometry for x axis. Add space at ends so that labels can
#   extend beyond plot as needed.
    max_len = 0.0;
    for (x in x_labels) {
	len = length(x_labels[x]);
	if ( len > max_len ) {
	    max_len = len;
	}
    }
    x_axis_extra = max_len * font_px;
    x_axis_x_px = plot_x_px - x_axis_extra / 2;
    x_axis_width_px = plot_width_px + x_axis_extra;
    x_axis_y_px = top_mgn_px + plot_hght_px;

#   Initialize the pisa SVG element and background rectangle.
    printf "<svg\n";
    printf "    width=\"%f\"\n", svg_width_px;
    printf "    height=\"%f\"\n", svg_height_px;
    printf "    xmlns=\"http://www.w3.org/2000/svg\"\n";
    printf "    xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n";
    printf "    id=\"pisa_svg\">\n\n";

    printf "<rect\n"
    printf "    width=\"%f\"\n", svg_width_px;
    printf "    height=\"%f\"\n", svg_height_px;
    printf "    fill=\"white\"\n";
    printf "    id=\"pisa_svg_bg\"\n";
    printf "    class=\"pisa_bg\""
    printf "/>\n\n";

#   Record Cartesian coordinates of edges
    printf "<defs>\n"
    printf "  <!-- Plot limits in Cartesian coordinates. -->\n";
    printf "  <!-- x_left x_right y_bottom y_top -->\n";
    printf "  <desc id=\"pisa_cartesian\">%g %g %g %g</desc>\n", \
	x_left, x_right, y_bottom, y_top;
    printf "</defs>\n"

#   Define plot area rectangle
    printf "\n<defs>\n";
    printf "  <!-- Plot area rectangle, for clipping and boundary -->\n";
    printf "  <rect\n";
    printf "      id=\"pisa_plot_rect\"\n";
    printf "      width=\"%f\"\n", plot_width_px;
    printf "      height=\"%f\" />\n", plot_hght_px;
    printf "</defs>\n";

#   Define plot area clip path
    printf "<defs>\n";
    printf "  <!-- Clip path for plot area -->\n";
    printf "  <clipPath id=\"pisa_plot_clip\">\n";
    printf "    <use\n";
    printf "        xlink:href=\"#pisa_plot_rect\"\n";
    printf "        x=\"%f\"\n", plot_x_px;
    printf "        y=\"%f\" />\n", top_mgn_px;
    printf "  </clipPath>\n";

#   X axis geometry and clip path.
    printf "  <!-- Clip path for xAxisLabels -->\n";
    printf "  <clipPath\n";
    printf "    id=\"pisa_x_axis_clip\">\n";
    printf "    <rect\n";
    printf "        id=\"pisa_x_axis_clip_rect\"\n";
    printf "        x=\"%f\"\n", x_axis_x_px;
    printf "        y=\"%f\"\n", x_axis_y_px;
    printf "        width=\"%f\"\n", x_axis_width_px;
    printf "        height=\"%f\"/>\n", x_axis_ht_px;
    printf "  </clipPath>\n";

#   Y axis geometry and clip path.
    printf "  <!-- Clip path for yAxisLabels -->\n";
    printf "  <clipPath\n";
    printf "    id=\"pisa_y_axis_clip\">\n";
    printf "    <rect\n";
    printf "        id=\"pisa_y_axis_clip_rect\"\n";
    printf "        x=\"%f\"\n", y_axis_x_px;
    printf "        y=\"%f\"\n", y_axis_y_px;
    printf "        width=\"%f\"\n", y_axis_width_px;
    printf "        height=\"%f\" />\n", y_axis_hght_px;
    printf "  </clipPath>\n";
    printf "</defs>\n";

#   Create plot area.
    printf "<!-- Clip path and SVG element for plot area -->\n";
    printf "<g clip-path=\"url(#pisa_plot_clip)\">\n";
    printf "  <svg\n";
    printf "      id=\"pisa_plot\"\n";
    printf "      x=\"%f\"\n", plot_x_px;
    printf "      y=\"%f\"\n", top_mgn_px;
    printf "      width=\"%f\"\n", plot_width_px;
    printf "      height=\"%f\"\n", plot_hght_px;
    printf "      viewBox=\"%f %f %f %f\"\n",
	   0.0, 0.0, plot_width_px, plot_hght_px;
    printf "      preserveAspectRatio=\"none\" >\n";
    printf "\n";
    printf "    <!-- Fill in plot area background -->\n";
    printf "    <rect\n";
    printf "        id=\"pisa_plot_bg\"\n";
    printf "        class=\"pisa_bg\"\n";
    printf "        x=\"%f\"\n", 0.0;
    printf "        y=\"%f\"\n", 0.0;
    printf "        width=\"%f\"\n", plot_width_px;
    printf "        height=\"%f\"\n", plot_hght_px;
    printf "        fill=\"white\" />\n";
    printf "\n"

#   Put plot elements into a group
    printf "<g id=\"pisa_plot_elements\">\n"
}

# When done plotting, terminate plot area. Draw axes and labels.
# Printing will continue, but subsequent elements will not use
# plot coordinates.
/end_svg:/ {
    printf "<!-- Done defining elements in plot area -->\n\n"
    printf "</g>"
    printf "  <!-- Terminate SVG element for plot area -->\n"
    printf "  </svg>\n";
    printf "\n";
    printf "<!-- Terminate clipping for plot area -->\n"
    printf "</g>\n";
    printf "\n";
    printf "<!-- Draw boundary around plot area -->\n";
    printf "<use\n";
    printf "    xlink:href=\"#pisa_plot_rect\"\n";
    printf "    class=\"pisa_fg\""
    printf "    x=\"%f\"\n", plot_x_px;
    printf "    y=\"%f\"\n", top_mgn_px;
    printf "    fill=\"none\"\n";
    printf "    stroke=\"black\">\n";
    printf "</use>\n";
    printf "\n";

#   Draw and label x axis
    printf "<!-- Clip area and svg element for x axis and labels -->\n";
    printf "<g clip-path=\"url(#pisa_x_axis_clip)\">\n";
    printf "  <svg\n";
    printf "      id=\"pisa_x_axis\"\n";
    printf "      x=\"%f\"\n", x_axis_x_px;
    printf "      y=\"%f\"\n", x_axis_y_px;
    printf "      width=\"%f\"\n", x_axis_width_px;
    printf "      height=\"%f\"\n", x_axis_ht_px;
    printf "      viewBox=\"%f %f %f %f\" >\n",
	   x_axis_x_px, x_axis_y_px, x_axis_width_px, x_axis_ht_px;

    y_px = x_axis_y_px + tick_len_px + pad_px + font_px;
    for (x in x_labels) {
	x_px = plot_x_px + (x - x_left) * px_per_x;
	printf "  <line\n";
	printf "      class=\"pisa_x_axis_tick pisa_fg\"\n";
	printf "      x1=\"%f\"\n", x_px;
	printf "      x2=\"%f\"\n", x_px;
	printf "      y1=\"%f\"\n", x_axis_y_px;
	printf "      y2=\"%f\"\n", x_axis_y_px + tick_len_px;
	printf "      stroke=\"black\"\n"
	printf "      stroke-width=\"1\" />\n"
	printf "  <text\n";
	printf "      class=\"pisa_x_axis_label pisa_fg\"\n";
	printf "      x=\"%f\"\n", x_px;
	printf "      y=\"%f\"\n", y_px;
	printf "      font-size=\"%.1f\"\n", font_px;
	printf "      text-anchor=\"middle\">\n";
	printf "%s", x_labels[x];
	printf "</text>\n";
    }
    printf "  </svg>\n";
    printf "</g>\n";
    printf "\n";
    if ( x_title_ht_px > 0.0 ) {
	printf "<text\n";
	printf "    id=\"pisa_x_title\"\n";
	printf "    class=\"pisa_x_axis_label pisa_fg\"\n";
	printf "    x=\"%f\"\n", x_axis_x_px + x_axis_width_px / 2.0;
	printf "    y=\"%f\"\n", x_axis_y_px + x_axis_ht_px + pad_px + font_px;
	printf "    font-size=\"%.1f\"\n", font_px;
	printf "    text-anchor=\"middle\">";
	printf "%s", x_title;
	printf "</text>\n";
    }

#   Draw and label y axis
    printf "<!-- Clip area and svg element for y axis and labels -->\n";
    printf "<g\n";
    printf "    clip-path=\"url(#pisa_y_axis_clip)\">\n";
    printf "  <svg\n";
    printf "    id=\"pisa_y_axis\"\n";
    printf "    x=\"%f\"\n", y_axis_x_px;
    printf "    y=\"%f\"\n", y_axis_y_px;
    printf "    width=\"%f\"\n", y_axis_width_px;
    printf "    height=\"%f\"\n", y_axis_hght_px;
    printf "    viewBox=\"%f %f %f %f\">\n",
	   y_axis_x_px, y_axis_y_px, y_axis_width_px, y_axis_hght_px;

    x_px = y_axis_x_px + y_axis_width_px - tick_len_px;
    for (y in y_labels) {
	y_px = top_mgn_px + (y_top - y) * px_per_y;
	printf "  <line\n";
	printf "      class=\"pisa_y_axis_tick pisa_fg\"\n";
	printf "      x1=\"%f\"\n", x_px;
	printf "      x2=\"%f\"\n", x_px + tick_len_px;
	printf "      y1=\"%f\"\n", y_px;
	printf "      y2=\"%f\"\n", y_px;
	printf "      stroke=\"black\"\n"
	printf "      stroke-width=\"1\" />\n"
	printf "  <text\n";
	printf "      class=\"pisa_y_axis_label pisa_fg\"\n";
	printf "      x=\"%f\"\n", x_px;
	printf "      y=\"%f\"\n", y_px;
	printf "      font-size=\"%.1f\"\n", font_px;
	printf "      text-anchor=\"end\"\n";
	printf "      dominant-baseline=\"mathematical\">";
	printf "%s", y_labels[y];
	printf "</text>\n";
    }
    printf "  </svg>\n";
    printf "</g>\n";
    printf "\n";
    if ( y_title_w_px > 0.0 ) {
	x = y_axis_x_px - pad_px;
	y = y_axis_y_px + y_axis_hght_px / 2.0;
	printf "<g\n";
	printf "    id=\"pisa_y_title_xform\"\n";
	printf "    transform=\"matrix(0.0, -1.0, 1.0, 0.0, %.1f, %.1f)\">\n",
	       x, y;
	printf "<text\n";
	printf "    id=\"pisa_y_title\"\n";
	printf "    class=\"pisa_y_axis_label pisa_fg\"\n";
	printf "    x=\"0.0\"\n";
	printf "    y=\"0.0\"";
	printf "    font-size=\"%.1f\"\n", font_px;
	printf "    text-anchor=\"middle\">";
	printf "%s", y_title;
	printf "</text>\n</g>\n";
    }
    printf "</svg>\n";
}

# Create a polygon element. Text after "polygon:" should specify
# attributes. Each subsequent line should present an x y coordinate
# pair. These are converted to SVG and printed until "end_poly:".
/polygon:/ {
    $1 = "";
    sub(" *", "");
    print "<polygon " $0 " points=\"";
    for (getline; $0 != "end_poly:"; getline) {
	x = $1;
	y = $2;
	printf "%g %g\n", (x - x_left) * px_per_x, (y_top - y) * px_per_y;
    }
    print "\" />"
}

# Create a polyline element. Text after "polyline:" should specify
# attributes. Each subsequent line should present an x y coordinate
# pair. These are converted to SVG and printed until "end_poly:".
/polyline:/ {
    $1 = "";
    sub(" *", "");
    print "<polyline " $0 " points=\"";
    for (getline; $0 != "end_poly:"; getline) {
	x = $1;
	y = $2;
	printf "%g %g\n", (x - x_left) * px_per_x, (y_top - y) * px_per_y;
    }
    print "\" />"
}

# Draw a circle or ellipse. 
# Usage: circle: x y [attribute="value" ...]
# Usage: ellipse: x y [attribute="value" ...]
# Circle attributes should include r.
# Ellipse attributes should include rx and ry.
# If element is in "circle" or "ellipse" class, pisa.js will adjust radii
# as plot scales.
/(circle|ellipse):/ {
    x = $2;
    y = $3;
    $1 = $2 = $3 = "";
    sub(" *", "");
    printf "<circle cx=\"%g\" cy=\"%g\" %s />\n",
	   (x - x_left) * px_per_x, (y_top - y) * px_per_y, $0;
}

# Draw a path.
# Usage: path: [attribute="value" ...]
# Subseqent lines should provide path command M, L, C, S, or Z followed by
# coordinate pairs. These are converted to SVG and printed until "end_path:".
/path:/ {
    $1 = "";
    sub(" *", "");
    print "<path " $0 " d=\"";
    for (getline; $0 != "end_path:"; getline) {
	cmd = $1;
	printf "%s ", cmd;
	for (n = 2; n < NF; n++) {
	    x = $n;
	    n++;
	    y = $n;
	    printf "%g %g\n", (x - x_left) * px_per_x, (y_top - y) * px_per_y;
	}
	if ( NF == 1 ) {
	    printf "\n";
	}
    }
    print "\" />"
}

