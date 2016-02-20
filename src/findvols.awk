#!/usr/bin/awk -f
#
#	Search findswps output for volumes.
#
# Optional variables from command line (-v option):
#	swp_angl_resoln	= maximum deviation. If sweep angles of two sweeps
#			  differ by less than swp_angl_resoln, they are assumed
#			  to be repetitions of the same sweep, i.e second sweep
#			  starts a new volue. Must be positive.
#	jump		= If angle from one sweep to next exceeds jump, assume
#			  antenna has repositioned to start of a new volume.
#
# When a volume is found, outputs a line of form:
#	Vol  index
# First volume has index 0.
# findswps output for the volume follows.
# Sweep index in findswp output line is replaced with index in volume, instead
# of index since start of input.
#
################################################################################
#
# Copyright (c) 2015, Gordon D. Carrie. All rights reserved.
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

# Absolute value
function fabs(x)
{
    return (x > 0.0) ? x : -x;
}

# +1 if nxt > prev
#  0 if nxt == prev
# -1 if nxt < prev
function dirn(prev, nxt)
{
    return (nxt - prev > 0.0) ? 1 : (prev - nxt > 0.0) ? -1 : 0;
}

BEGIN {
    nan = -999.0;

    # Set defaults. Override with -v on command line.
    if ( swp_angl_resoln == 0.0 ) {
	swp_angl_resoln = 0.5;
    }
    if ( jump == 0.0 ) {
	jump = 12.0;
    }
    i_vol = -1;
    i_swp = 0;

    # Initialize variables from first sweep. swp_angl_vol is angle of first
    # sweep in volume. If it reappears in a new sweep, assume the new sweep
    # starts a new volume.

    scan_mode_vol = "unk";
    swp_angl_vol = nan;
}
$1 == "Sweep" {
    scan_mode_curr = $3;
    swp_angl_curr = $6;
    dirn_curr = dirn(swp_angl_prev, swp_angl_curr);
    if ( dirn_vol == nan ) {
	dirn_vol = dirn_curr;
    }
    i_swp++;

    if ( scan_mode_curr != scan_mode_vol				   \
	    || ( scan_mode_vol == "PPI" && swp_angl_curr < swp_angl_prev ) \
	    || fabs(swp_angl_curr - swp_angl_vol) < swp_angl_resoln	   \
	    || fabs(swp_angl_curr - swp_angl_prev) > jump		   \
	    || dirn_curr != dirn_vol ) {
	i_vol++;
	scan_mode_vol = scan_mode_curr;
	swp_angl_vol = swp_angl_curr;
	i_swp = 0;
	dirn_vol = nan;
	print "Vol " i_vol " ScanMode " scan_mode_vol;
    }

    $2 = i_swp;
    print;
    scan_mode_prev = scan_mode_curr;
    swp_angl_prev = swp_angl_curr;
    next;
}
{
    print;
}

