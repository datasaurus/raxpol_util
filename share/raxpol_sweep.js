/*
   -	raxpol_sweep.js --
   -		This script adds interactive behavior to a plot made by
   -		raxpol_swps_svg.
   -
   .	Copyright (c) 2013, Gordon D. Carrie. All rights reserved.
   .
   .	Redistribution and use in source and binary forms, with or without
   .	modification, are permitted provided that the following conditions
   .	are met:
   .
   .	* Redistributions of source code must retain the above copyright
   .	notice, this list of conditions and the following disclaimer.
   .	* Redistributions in binary form must reproduce the above copyright
   .	notice, this list of conditions and the following disclaimer in the
   .	documentation and/or other materials provided with the distribution.
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
   This function adds variables and functions related to the SVG plot
   to its call object. It sets up several event listeners, where the
   plot variables and functions persist anonymously in closures.
 */

window.addEventListener("load", function (evt)
{
    "use strict";
    /*jslint browser:true */

    /* Pisa plot elements. See pisa.1 and pisa.awk. */
    var PisaPlot = document.getElementById("pisa_plot");
    var XAxis = document.getElementById("pisa_x_axis");
    var YAxis = document.getElementById("pisa_y_axis");

    function CreateSVGElement(name)
    {
	var ns = "http://www.w3.org/2000/svg";
	return document.createElementNS(ns, name);
    }

    /* Stringify x with precision prx, removing trailing "." and 0's */ 
    function ToPrx(x, prx)
    {
	var s = x.toPrecision(prx);
	s = s.replace(/(\.\d*[1-9])0*$/, "$1");
	s = s.replace(/\.0*$/, "");
	return s.replace(/\.?$/, "");
    }

    /* Length of axis tick marks, in pixels */
    var Tick = (document.getElementsByClassName("pisa_x_axis_tick"))[0];
    var TickLen = Tick.y2.baseVal.value - Tick.y1.baseVal.value;

    /* Font size, in pixels */
    function FontSz()
    {
	var lbl = document.getElementsByClassName("pisa_x_axis_label")[0];
	if ( lbl ) {
	    var fs = Number(lbl.getAttribute("font-size"));
	    if ( fs ) {
		return fs;
	    } else {
		var bbox = lbl.getBBox();
		return bbox.height;
	    }
	}
	return 12.0;
    }

    /* Cartesian coordinates */
    function GetCart()
    {
	var elem = document.getElementById("pisa_cartesian");
	var cart = elem.textContent.split(/\s+/);
	return {
	    x_left : Number(cart[0]),
	    x_rght : Number(cart[1]),
	    y_btm : Number(cart[2]),
	    y_top : Number(cart[3])
	};
    }
    function SetCart(cart)
    {
	var x_left = cart.x_left;
	var x_rght = cart.x_rght;
	var y_top = cart.y_top;
	var y_btm = cart.y_btm;
	var t = x_left + " " + x_rght + " " + y_btm + " " + y_top;
	document.getElementById("pisa_cartesian").textContent = t;
    }
    function CartXToSVG(cartX)
    {
	var c = GetCart();
	var plotSVGX = PisaPlot.x.baseVal.value;
	var plotSVGWidth = PisaPlot.width.baseVal.value;
	var pxPerM = plotSVGWidth / (c.x_rght - c.x_left);
	return plotSVGX + (cartX - c.x_left) * pxPerM;
    }
    function SVGXToCart(svgX)
    {
	var c = GetCart();
	var plotSVGX = PisaPlot.x.baseVal.value;
	var plotSVGWidth = PisaPlot.width.baseVal.value;
	var mPerPx = (c.x_rght - c.x_left) / plotSVGWidth;
	return c.x_left + (svgX - plotSVGX) * mPerPx;
    }
    function CartYToSVG(cartY)
    {
	var c = GetCart();
	var plotSVGY = PisaPlot.y.baseVal.value;
	var plotSVGHght = PisaPlot.height.baseVal.value;
	var pxPerM = plotSVGHght / (c.y_btm - c.y_top);
	return plotSVGY + (cartY - c.y_top) * pxPerM;
    }
    function SVGYToCart(svgY)
    {
	var c = GetCart();
	var plotSVGY = PisaPlot.y.baseVal.value;
	var plotSVGHght = PisaPlot.height.baseVal.value;
	var mPerPx = (c.y_btm - c.y_top) / plotSVGHght
	return c.y_top + (svgY - plotSVGY) * mPerPx;
    }

    /*
       Enable zooming. ZoomPlot applies zoom factor s to the plot.
       s < 1 => zooming in, s > 1 => zooming out.
     */

    function ZoomPlot(sx, sy)
    {
	var dx, dy, fx, fy;
	var c;
	var viewBox = PisaPlot.viewBox;
	var viewBoxX = viewBox.baseVal.x;
	var viewBoxY = viewBox.baseVal.y;
	var viewBoxWidth = viewBox.baseVal.width;
	var viewBoxHght = viewBox.baseVal.height;

	fx = (1.0 - sx) / 2.0;
	dx = viewBoxWidth * fx;
	fy = (1.0 - sy) / 2.0;
	dy = viewBoxHght * fy;
	viewBox = (viewBoxX + dx) + " " + (viewBoxY + dy)
	    + " " + viewBoxWidth * sx + " " + viewBoxHght * sy;
	PisaPlot.setAttribute("viewBox", viewBox);
	c = GetCart();
	dx = (c.x_rght - c.x_left) * fx;
	c.x_left += dx;
	c.x_rght -= dx;
	dy = (c.y_top - c.y_btm) * fy;
	c.y_btm += dy;
	c.y_top -= dy;
	SetCart(c);
	UpdateBG();
	UpdateAxes();
	UnZoom(0.5 * (sx + sy));
    }

    /*
       ReSize function resizes plot to preserve original margins
       if window resizes.
     */

    function ReSize(evt)
    {
	var pisaSVG = document.getElementById("pisa_svg");
	var pisaBG = document.getElementById("pisa_svg_bg");
	var plotRect = document.getElementById("pisa_plot_rect");
	var prevSVGWidth, prevSVGHght, newSVGWidth, newSVGHght;
	var prevPlotWidth, prevPlotHght, newPlotWidth, newPlotHght;
	var leftMgn = PisaPlot.x.baseVal.value;
	var plotSVGWidth = PisaPlot.width.baseVal.value;
	var plotSVGHght = PisaPlot.height.baseVal.value;
	var pisaSVGWidth = pisaSVG.width.baseVal.value;
	var pisaSVGHght = pisaSVG.height.baseVal.value;
	var rghtMgn = pisaSVGWidth - leftMgn - plotSVGWidth;
	var topMgn = PisaPlot.y.baseVal.value;
	var btmMgn = pisaSVGHght - topMgn - plotSVGHght;
	var s;

	prevSVGWidth = pisaSVGWidth;
	prevSVGHght = pisaSVGHght;
	newSVGWidth = this.innerWidth;
	newSVGHght = this.innerHeight;
	pisaSVG.setAttribute("width", newSVGWidth);
	pisaSVG.setAttribute("height", newSVGHght);
	pisaBG.setAttribute("width", newSVGWidth);
	pisaBG.setAttribute("height", newSVGHght);
	if ( document.rootElement != pisaSVG ) {
	    document.rootElement.setAttribute("width", newSVGWidth);
	    document.rootElement.setAttribute("height", newSVGHght);
	}
	prevPlotWidth = plotSVGWidth;
	prevPlotHght = plotSVGHght;
	newPlotWidth = newSVGWidth - leftMgn - rghtMgn;
	newPlotHght = newSVGHght - topMgn - btmMgn;
	PisaPlot.setAttribute("width", newPlotWidth);
	PisaPlot.setAttribute("height", newPlotHght);
	plotRect.setAttribute("width", newPlotWidth);
	plotRect.setAttribute("height", newPlotHght);
	ZoomPlot(newPlotWidth / prevPlotWidth, newPlotHght / prevPlotHght);
	s = (prevSVGWidth / newSVGWidth + prevSVGHght / newSVGHght) / 2.0;
	UnZoom(s);
	UpdateBG();
	UpdateAxes();
    }

    /*
       Adjust sizes of certain attributes so objects do not become fat or
       thin during zooming and resizing. s = scale factor.
       s < 1 => elements become smaller.
       s > 1 => elements become larger.
     */

    function UnZoom(s)
    {

	var elems, attrs, n, a, dash, dashes, e;

	elems = document.getElementsByClassName("line");
	for (e = 0; e < elems.length; e++) {
	    attrs = ["stroke-width", "stroke-dashoffset",
		"markerWidth", "markerHeight"];
	    for (n = 0; n < attrs.length; n++) {
		a = Number(elems[e].getAttribute(attrs[n]));
		if ( a && a > 0.0 ) {
		    elems[e].setAttribute(attrs[n], a * s);
		}
	    }
	    a = Number(elems[e].getAttribute("stroke-dasharray"));
	    if ( a ) {
		dashes = "";
		for (dash in a.split(/\s+|,/)) {
		    dashes = dashes + " " + Number(dash) * s;
		}
		elems[e].setAttribute("stroke-dasharray", dashes);
	    }
	}
	elems = PisaPlot.getElementsByClassName("circle");
	for (e = 0; e < elems.length; e++) {
	    a = Number(elems[e].getAttribute("r"));
	    if ( a && a > 0.0 ) {
		elems[e].setAttribute("r", a * s);
	    }
	}
	elems = PisaPlot.getElementsByClassName("ellipse");
	for (e = 0; e < elems.length; e++) {
	    a = Number(elems[e].getAttribute("rx"));
	    if ( a && a > 0.0 ) {
		elems[e].setAttribute("rx", a * s);
	    }
	    a = Number(elems[e].getAttribute("ry"));
	    if ( a && a > 0.0 ) {
		elems[e].setAttribute("ry", a * s);
	    }
	}
    }

    /*
       Enable plot dragging.

       StartPlotDrag is called at mouse down. It records the location of
       the plot in its parent as members x0 and y0. It records the initial
       cursor location in DragSVGX0 and DragSVGY0, which remain constant
       throughout the drag. It also records the cursor location in members
       PrevEvtSVGX and PrevEvtSVGY, which change at every mousemove during the
       drag.

       PlotDrag is called at each mouse move while the plot is being
       dragged. It determines how much the mouse has moved since the last
       event, and shifts the dragable elements by that amount.

       EndPlotDrag is called at mouse up. It determines how much the
       viewBox has changed since the start of the drag. It restores dragged
       elements to their initial coordinates, but with a shifted viewBox.
     */


    function StartPlotDrag(evt)
    {
	var DragSVGX0, DragSVGY0;	/* SVG coordinates of mouse at start
					   of drag */
	var PrevEvtSVGX, PrevEvtSVGY;	/* SVG coordinates of mouse at previous
					   mouse event during drag */
	var PlotSVGX0, PlotSVGY0;	/* SVG coordinates of plot at start
					   of drag */
	var XAxisSVGX0, YAxisSVGY0;	/* SVG coordinates of axes at start
					   of drag */

	evt.preventDefault();
	PrevEvtSVGX = DragSVGX0 = evt.clientX;
	PrevEvtSVGY = DragSVGY0 = evt.clientY;
	PlotSVGX0 = PisaPlot.x.baseVal.value;
	PlotSVGY0 = PisaPlot.y.baseVal.value;
	XAxisSVGX0 = XAxis.x.baseVal.value;
	YAxisSVGY0 = YAxis.y.baseVal.value;
	PisaPlot.addEventListener("mousemove", PlotDrag, false);
	PisaPlot.addEventListener("mouseup", EndPlotDrag, false);

	function PlotDrag(evt)
	{
	    var dx, dy;			/* How much to move the elements */

	    evt.preventDefault();
	    dx = evt.clientX - PrevEvtSVGX;
	    dy = evt.clientY - PrevEvtSVGY;
	    PisaPlot.setAttribute("x", PisaPlot.x.baseVal.value + dx);
	    PisaPlot.setAttribute("y", PisaPlot.y.baseVal.value + dy);
	    XAxis.setAttribute("x", XAxis.x.baseVal.value + dx);
	    YAxis.setAttribute("y", YAxis.y.baseVal.value + dy);
	    PrevEvtSVGX = evt.clientX;
	    PrevEvtSVGY = evt.clientY;
	}
	function EndPlotDrag(evt)
	{
	    var plotSVGWidth = PisaPlot.width.baseVal.value;
	    var plotSVGHght = PisaPlot.height.baseVal.value;
	    var viewBox = PisaPlot.viewBox;
	    var viewBoxX = viewBox.baseVal.x;
	    var viewBoxY = viewBox.baseVal.y;
	    var viewBoxWidth = viewBox.baseVal.width;
	    var viewBoxHght = viewBox.baseVal.height;
	    var c;
	    var dx, dy;

	    evt.preventDefault();

	    /*
	       Move plot element back to location at start of drag. Shift plot
	       within to drag destination to by shifting viewBox. Update
	       Cartesian coordinates.
	     */

	    PisaPlot.setAttribute("x", PlotSVGX0);
	    PisaPlot.setAttribute("y", PlotSVGY0);
	    dx = (evt.clientX - DragSVGX0) * viewBoxWidth / plotSVGWidth;
	    dy = (evt.clientY - DragSVGY0) * viewBoxHght / plotSVGHght;
	    viewBox = (viewBoxX - dx) + " " + (viewBoxY - dy)
		+ " " + viewBoxWidth + " " + viewBoxHght;
	    PisaPlot.setAttribute("viewBox", viewBox);
	    c = GetCart();
	    dx = (evt.clientX - DragSVGX0) * (c.x_rght - c.x_left) / plotSVGWidth;
	    c.x_left -= dx;
	    c.x_rght -= dx;
	    dy = (evt.clientY - DragSVGY0) * (c.y_btm - c.y_top) / plotSVGHght;
	    c.y_btm -= dy;
	    c.y_top -= dy;
	    SetCart(c);

	    UpdateBG();
	    XAxis.setAttribute("x", XAxisSVGX0);
	    YAxis.setAttribute("y", YAxisSVGY0);
	    UpdateAxes();
	    PisaPlot.removeEventListener("mousemove", PlotDrag, false);
	    PisaPlot.removeEventListener("mouseup", EndPlotDrag, false);
	}
    }

    /* Draw the background */
    function UpdateBG()
    {
	var plotBG = document.getElementById("pisa_plot_bg");
	plotBG.setAttribute("x", PisaPlot.viewBox.baseVal.x);
	plotBG.setAttribute("width", PisaPlot.viewBox.baseVal.width);
	plotBG.setAttribute("y", PisaPlot.viewBox.baseVal.y);
	plotBG.setAttribute("height", PisaPlot.viewBox.baseVal.height);
    }

    /* Distances axis labels can go beyond plot edges, pixels */ 
    var XOverHang = XAxis.width.baseVal.value - PisaPlot.width.baseVal.value;
    var YOverHang = YAxis.height.baseVal.value - PisaPlot.height.baseVal.value;

    /* Apply tick marks and labels to axes. */
    function UpdateAxes()
    {
	var plotSVGWidth = PisaPlot.width.baseVal.value;
	var plotSVGHght = PisaPlot.height.baseVal.value;
	var xAxisClip = document.getElementById("pisa_x_axis_clip_rect");
	var yAxisClip = document.getElementById("pisa_y_axis_clip_rect");
	var x, y, w, h;
	var c = GetCart();

	x = XAxis.x.baseVal.value;
	y = PisaPlot.y.baseVal.value + plotSVGHght;
	w = plotSVGWidth + XOverHang;
	h = XAxis.viewBox.baseVal.height;
	XAxis.setAttribute("y", y);
	XAxis.setAttribute("width", w);
	xAxisClip.setAttribute("y", y);
	xAxisClip.setAttribute("width", w);
	XAxis.setAttribute("viewBox", x + " " + y + " " + w + " " + h);
	var xTitle = document.getElementById("pisa_x_title");
	if ( xTitle ) {
	    xTitle.setAttribute("x", x + w / 2.0);
	    xTitle.setAttribute("y", y + 2.0 * h - TickLen);
	}
	MkLabels(c.x_left, c.x_rght, ApplyXCoords, plotSVGWidth / 4);

	x = YAxis.viewBox.baseVal.x;
	y = YAxis.y.baseVal.value;
	w = YAxis.viewBox.baseVal.width;
	h = plotSVGHght + YOverHang;
	YAxis.setAttribute("height", h);
	yAxisClip.setAttribute("height", h);
	YAxis.setAttribute("viewBox", x + " " + y + " " + w + " " + h);
	var yTitle = document.getElementById("pisa_y_title");
	if ( yTitle ) {
	    var elem = document.getElementById("pisa_y_title_xform");
	    var transForm = elem.transform.baseVal.getItem(0).matrix;
	    transForm.f = y + h / 2.0;
	}
	MkLabels(c.y_btm, c.y_top, ApplyYCoords, plotSVGHght / 4);
    }

    /*
       Produce a set of labels for coordinates ranging from lo to hi.
       apply_coords must be a function that creates the labels in the
       document and returns the amount of space they use. max_sz must
       specify the maximum amount of space they are allowed to use.
     */

    function MkLabels(lo, hi, apply_coords, max_sz)
    {
	/*
	   Initialize dx with smallest power of 10 larger in magnitude
	   than hi - lo. Decrease magnitude of dx. Place
	   label set for the smaller dx into l1. If printing the labels
	   in l1 would overflow the axis with characters, restore and
	   use l0. Otherwise, replace l0 with l1 and retry with a
	   smaller dx.
	 */

	var dx, have_labels, l0, l1, t;

	if ( lo === hi ) {
	    apply_coords([l0]);
	    return;
	}
	if ( lo > hi ) {
	    t = hi;
	    hi = lo;
	    lo = t;
	}
	dx = Math.pow(10.0, Math.ceil(Math.log(hi - lo) / Math.LN10));
	for (have_labels = false; !have_labels; ) {
	    l1 = CoordList(lo, hi, dx);
	    if ( apply_coords(l1) > max_sz ) {
		apply_coords(l0);
		have_labels = true;
	    } else {
		l0 = l1;
	    }
	    dx *= 0.5;			/* If dx was 10, now it is 5 */
	    l1 = CoordList(lo, hi, dx);
	    if ( apply_coords(l1) > max_sz ) {
		apply_coords(l0);
		have_labels = true;
	    } else {
		l0 = l1;
	    }
	    dx *= 0.4;			/* If dx was 5, now it is 2 */
	    l1 = CoordList(lo, hi, dx);
	    if ( apply_coords(l1) > max_sz ) {
		apply_coords(l0);
		have_labels = true;
	    } else {
		l0 = l1;
	    }
	    dx *= 0.5;			/* If dx was 2, now it is 1 */
	}
    }

    /*
       Create a set of axis labels for an axis ranging from x_min to x_max
       with increment dx. Returns an array of coordinate values.
     */

    function CoordList(x_min, x_max, dx)
    {
	var x0;				/* Coordinate of first label = nearest
					   multiple of dx less than x_min */
	var x;				/* x coordinate */
	var coords;			/* Return value */
	var n, m;			/* Loop indeces */

	coords = [];
	x0 = Math.floor(x_min / dx) * dx;
	for (n = m = 0; n <= Math.ceil((x_max - x_min) / dx); n++) {
	    x = x0 + n * dx;
	    if ( x >= x_min - dx / 4 && x <= x_max + dx / 4 ) {
		coords[m] = x;
		m++;
	    }
	}
	return coords;
    }

    /*
       Axis labels:
       For each axis there will be an array of objects.
       Each object will have as members:
       lbl	- a text element with the text of the label
       tick	- a line element with a tick mark
     */

    var XLabels = [];
    var YLabels = [];

    /*
       Apply coordinate list coords to x axis. Return total display length
       of the labels.
     */

    function ApplyXCoords(coords)
    {
	var x;				/* Label location */
	var y;				/* Label text location */
	var y1, y2;			/* Limits of tick */
	var l;				/* Label index */
	var textLength;			/* SVG width required to display text of
					   all labels */
	var lbl, tick;			/* Label and tick elements */
	var bbox;			/* Bounding box for a text label */
	var plotSVGX;
	var plotSVGWidth;

	y1 = XAxis.y.baseVal.value;
	y2 = y1 + TickLen;
	y = y2 + XAxis.height.baseVal.value - TickLen;
	plotSVGX = PisaPlot.x.baseVal.value;
	plotSVGWidth = PisaPlot.width.baseVal.value;
	for (l = 0, textLength = 0.0; l < coords.length; l++) {
	    if ( !XLabels[l] ) {
		lbl = CreateSVGElement("text");
		lbl.setAttribute("class", "pisa_x_axis_label pisa_fg");
		lbl.setAttribute("text-anchor", "middle");
		lbl.setAttribute("font-size", FontSz());
		XAxis.appendChild(lbl);
		tick = CreateSVGElement("line");
		tick.setAttribute("class", "pisa_x_axis_tick pisa_fg");
		XAxis.appendChild(tick);
		XLabels[l] = {};
		XLabels[l].lbl = lbl;
		XLabels[l].tick = tick;
	    }
	    x = CartXToSVG(coords[l]);
	    if ( x >= plotSVGX && x <= plotSVGX + plotSVGWidth ) {
		XLabels[l].lbl.setAttribute("x", x);
		XLabels[l].lbl.setAttribute("y", y);
		XLabels[l].lbl.textContent = ToPrx(coords[l], 5);
		XLabels[l].tick.setAttribute("x1", x);
		XLabels[l].tick.setAttribute("x2", x);
		XLabels[l].tick.setAttribute("y1", y1);
		XLabels[l].tick.setAttribute("y2", y2);
		textLength += XLabels[l].lbl.getComputedTextLength();
	    } else {
		HideLabel(XLabels[l]);
	    }
	}
	for ( ; l < XLabels.length; l++) {
	    HideLabel(XLabels[l]);
	}
	return textLength;
    }

    /*
       Apply coordinate list coords to y axis. Return total display height
       of the labels.
     */

    function ApplyYCoords(coords)
    {
	var yAxisRght;			/* SVG x coordinates of RIGHT side of
					   y axis element */
	var x;				/* Label text location */
	var x1, x2;			/* Limits of tick */
	var y;				/* SVG y coordinate of a label */
	var l;				/* Label, coordinate index */
	var bbox;			/* Bounding box for an element */
	var textHght;			/* Total display height */
	var lbl, tick;
	var plotSVGY, plotSVGHght;
	var ns = "http://www.w3.org/2000/svg";

	yAxisRght = YAxis.x.baseVal.value + YAxis.width.baseVal.value;
	x = yAxisRght - 1.5 * TickLen;
	x1 = yAxisRght - TickLen;
	x2 = yAxisRght;
	plotSVGY = PisaPlot.y.baseVal.value;
	plotSVGHght = PisaPlot.height.baseVal.value;
	for (l = 0, textHght = 0.0; l < coords.length; l++) {
	    if ( !YLabels[l] ) {
		lbl = CreateSVGElement("text");
		lbl.setAttribute("class", "pisa_y_axis_label pisa_fg");
		lbl.setAttribute("text-anchor", "end");
		lbl.setAttribute("dominant-baseline", "mathematical");
		lbl.setAttribute("font-size", FontSz());
		YAxis.appendChild(lbl);
		tick = CreateSVGElement("line");
		tick.setAttribute("class", "pisa_y_axis_tick pisa_fg");
		YAxis.appendChild(tick);
		YLabels[l] = {};
		YLabels[l].lbl = lbl;
		YLabels[l].tick = tick;
	    }
	    y = CartYToSVG(coords[l]);
	    if ( y >= plotSVGY && y <= plotSVGY + plotSVGHght ) {
		YLabels[l].lbl.setAttribute("x", x);
		YLabels[l].lbl.setAttribute("y", y);
		YLabels[l].lbl.textContent = ToPrx(coords[l], 5);
		YLabels[l].tick.setAttribute("x1", x1);
		YLabels[l].tick.setAttribute("x2", x2);
		YLabels[l].tick.setAttribute("y1", y);
		YLabels[l].tick.setAttribute("y2", y);
		bbox = YLabels[l].lbl.getBBox();
		textHght += bbox.height;
	    } else {
		HideLabel(YLabels[l]);
	    }
	}
	for ( ; l < YLabels.length; l++) {
	    HideLabel(YLabels[l]);
	}
	return textHght;
    }

    /*
       Hide label, which must be a label object with text and tick elements.
       The elements still exist in the document.
     */ 

    function HideLabel(label)
    {
	label.lbl.setAttribute("x", -80.0);
	label.lbl.setAttribute("y", -80.0);
	label.lbl.textContent = "";
	label.tick.setAttribute("x1", -80.0);
	label.tick.setAttribute("x2", -90.0);
	label.tick.setAttribute("y1", -80.0);
	label.tick.setAttribute("y2", -90.0);
    }

    /*
       RaXPol SVG file must have elements with the following id's.
       They are replaced when a new sweep is put into the display.
       Other elements, such as axes, are left in place.

       "case_id"			MMDDYY of case, if site uses cases.
       					Identifies image directory at site.
       "raxpol_path"			Descriptor element. Text content
       					gives path to the RaXPol moment file
       					on host.
       "vol_id"				Descriptor element. Text content
					identifies volume in form
					YYYYMMDD-HHMMSS.
       "scan_mode"			Descriptor element. Text content gives
					scan geometry and limit, e.g. "PPI".
       "site_loc_caption"		Text element displays site name and
					location. 
       "sweep_time_caption"		Text element displays sweep time 
       "data_type_caption"		Text element displays data type. 
       "data_type"			Descriptor element whose text content
					is the current data type, e.g. "DB_DBZ"
       "data_types"			Descriptor element whose text content
					is a space separated list of all data
					types in volume, e.g.
					"DB_DBZ DB_VEL DB_WIDTH"
       "sweep_angle_caption"		Text element displays sweep angle 
       "swp_angl"			Descriptor element. Text content gives
					sweep angle of displayed sweep, in
					degrees.
       "swp_idx"			Descriptor element. Text content gives
					sweep index of displayed sweep. First
					sweep has index 0.
       "sweep_angles"			Descriptor element whose text content
					is a space separated list of sweep
					angles in the volume, e.g. "1.0 2.0 4.0"
       "pisa_x_title"			Axis labels
       "pisa_y_title"

	Document has angle, bounds, and viewBox requests for both scan types
	since previous and next volumes might differ from current scan type.

       "prev_vol"			Descriptor element Text content must
					identify of form previous volume.
       "next_vol"			Descriptor element Text content must
					identify of form next volume.
       "color_legend"			Color legend 
     */

    var Id, SwpElems = {};
    SwpElems["case_id"] = null;
    SwpElems["raxpol_path"] = null;
    SwpElems["vol_id"] = null;
    SwpElems["scan_mode"] = null;
    SwpElems["site_loc_caption"] = null;
    SwpElems["sweep_time_caption"] = null;
    SwpElems["data_type_caption"] = null;
    SwpElems["sweep_angle_caption"] = null;
    SwpElems["data_type"] = null;
    SwpElems["data_types"] = null;
    SwpElems["swp_angl"] = null;
    SwpElems["swp_idx"] = null;
    SwpElems["sweep_angles"] = null;
    SwpElems["pisa_x_title"] = null;
    SwpElems["pisa_y_title"] = null;
    SwpElems["prev_vol"] = null;
    SwpElems["next_vol"] = null;
    SwpElems["color_legend"] = null;
    for (Id in SwpElems) {
	SwpElems[Id] = document.getElementById(Id);
    }

    /* Default sweep angle, depends on scan type */
    var SwpAngl = {};
    switch (SwpElems["scan_mode"].textContent) {
	case "PPI":
	    SwpAngl["PPI"] = SwpElems["swp_angl"].textContent;
	    SwpAngl["RHI"] = "default";
	    break;
	case "RHI":
	    SwpAngl["PPI"] = "default";
	    SwpAngl["RHI"] = SwpElems["swp_angl"].textContent;
	    break;
    }

    /* Convert degrees to radians and back */
    var RadPerDeg = Math.PI / 180.0;
    var DegPerRad = 180.0 / Math.PI;

    /* Sweep angle in radians */
    var SweepAz, SweepTilt;

    /* These functions update the cursor location display. */ 
    var CursorLoc = document.getElementById("raxpol_cursor_loc");
    CursorLoc.setAttribute("visibility", "visible");
    CursorLoc.setAttribute("display", "inline");
    var REarth = document.getElementById("rearth").textContent - 0.0;
    var RadarLon = document.getElementById("radar_lon").textContent - 0.0;
    var RadarLat = document.getElementById("radar_lat").textContent - 0.0;
    RadarLon *= RadPerDeg;
    RadarLat *= RadPerDeg;
    function UpdateRHILoc(evt)
    {
	var x = SVGXToCart(evt.clientX);
	var y = SVGYToCart(evt.clientY);
	var cursorLoc = {};

	/* Cursor longitude and latitude */
	GeogStep(RadarLon, RadarLat, SweepAz, x / REarth, cursorLoc);
	var lon = cursorLoc.lon * DegPerRad;
	var lat = cursorLoc.lat * DegPerRad;
	var ew = (lon > 0.0) ? " E " : " W ";
	var ns = (lat > 0.0) ? " N " : " S ";
	lon = Math.abs(lon);
	lat = Math.abs(lat);

	/* Ray length and elevation */
	var h = REarth + y;
	var delta = x / REarth;
	var sind = Math.sin(delta);
	var sind2 = Math.sin(delta / 2.0);
	var el = Math.atan((y + 2.0 * REarth * sind2 * sind2)
		/ ((y + 2.0 * REarth) * sind)) * DegPerRad;
	var txt = "Cursor: "
	    + lon.toFixed(3) + ew
	    + lat.toFixed(3) + ns
	    + y.toFixed(0) + " m above ground,"
	    + " From radar " + x.toFixed(0) + " m along ground"
	    + " at " + el.toFixed(1) + " deg";
	CursorLoc.textContent = txt;
    }
    function UpdatePPILoc(evt)
    {
	var x = SVGXToCart(evt.clientX);
	var y = SVGYToCart(evt.clientY);
	var az = Math.atan2(x, y);		/* From North = up */
	if ( az < 0.0 ) {
	    az += 2.0 * Math.PI;
	}
	var dist = Math.sqrt(x * x + y * y);
	var delta = dist / REarth;
	var cursorLoc = {};
	GeogStep(RadarLon, RadarLat, az, delta, cursorLoc);
	var lon = cursorLoc.lon * DegPerRad;
	var lat = cursorLoc.lat * DegPerRad;
	var ew = (lon > 0.0) ? " E " : " W ";
	var ns = (lat > 0.0) ? " N " : " S ";
	lon = Math.abs(lon);
	lat = Math.abs(lat);
	var ht = 2.0 * REarth * Math.sin(delta / 2.0)
	    * Math.sin(SweepTilt + delta / 2.0)
	    / Math.cos(SweepTilt + delta);
	var txt = "Cursor: "
	    + "(" + x.toFixed(1) + "," + y.toFixed(1) + ") "
	    + lon.toFixed(3) + ew
	    + lat.toFixed(3) + ns
	    + ht.toFixed(0) + " m above ground"
	    + " From radar "
	    + (az * DegPerRad).toFixed(3) + " deg, "
	    + dist.toFixed(1) + " m along ground";
	CursorLoc.textContent = txt;
    }

    /*
       This function sets radar location, volume mode (RHI or PPI),
       sweep angle, and Cartesian limits of display area.
     */

    function SetVolGeom()
    {
	switch (SwpElems["scan_mode"].textContent) {
	    case "RHI":
		SweepAz = SwpElems["swp_angl"].textContent * RadPerDeg;
		PisaPlot.removeEventListener("mousemove", UpdatePPILoc, false);
		PisaPlot.addEventListener("mousemove", UpdateRHILoc, false);
		break;
	    case "PPI":
		SweepTilt = SwpElems["swp_angl"].textContent * RadPerDeg;
		PisaPlot.removeEventListener("mousemove", UpdateRHILoc, false);
		PisaPlot.addEventListener("mousemove", UpdatePPILoc, false);
		break;
	}
    }
    SetVolGeom();

    /*
       This function centers the captions over the plot. It should be
       called when caption or plot width changes.
     */

    function CenterCaptions()
    {
	var c, captions, x;

	captions = document.getElementsByClassName("raxpol_caption");
	x = PisaPlot.x.baseVal.value + PisaPlot.width.baseVal.value / 2.0;
	for (c = 0; c < captions.length; c++) {
	    captions[c].setAttribute("x", x);
	}
    }

    /*
       When window changes size, update Cartesian limits, and adjust
       captions and margin.
     */ 

    function RaXPol_ReSize(evt)
    {
	var xRight, legend, transform;

	xRight = PisaPlot.x.baseVal.value + PisaPlot.width.baseVal.value;
	CenterCaptions();
	if ( SwpElems["color_legend"] ) {
	    legend = SwpElems["color_legend"];
	    transform = legend.transform.baseVal.getItem(0);
	    transform.setTranslate(xRight + 24, transform.matrix.f);
	}
    }

    /*
       Compute destination point at given separation delta and
       direction dir from point at longitude lon1, latitude lat1.
       Destination longitude and latitude will be placed in loc
       as lon and lat members. All angles are in radians. delta in
       great circle radians.

       Ref.
       Sinnott, R. W., "Virtues of the Haversine",
       Sky and Telescope, vol. 68, no. 2, 1984, p. 159
       cited in: http://www.census.gov/cgi-bin/geo/gisfaq?Q5.1
     */

    function GeogStep(lon1, lat1, dir, delta, loc)
    {
	var sin_s, sin_d, cos_d, a, x, y, lon;

	sin_s = Math.sin(delta);
	sin_d = Math.sin(dir);
	cos_d = Math.cos(dir);
	a = 0.5 * (Math.sin(lat1 + delta) * (1.0 + cos_d)
		+ Math.sin(lat1 - delta) * (1.0 - cos_d));
	loc.lat = (a > 1.0) ? Math.PI / 2
	    : (a < -1.0) ? -Math.PI / 2 : Math.asin(a);
	y = sin_s * sin_d;
	x = 0.5 * (Math.cos(lat1 + delta) * (1 + cos_d)
		+ Math.cos(lat1 - delta) * (1 - cos_d));
	lon = lon1 + Math.atan2(y, x);
	loc.lon = ( lon > Math.PI ) ? lon - 2.0 * Math.PI
	    : ( lon < -Math.PI ) ? lon + 2.0 * Math.PI : lon;
    }

    /* Resize plot for current window */ 
    CenterCaptions();
    RaXPol_ReSize.call(this, {});

    /*
       CGI_Dir specifies URL of the CGI directory, e.g.
       "http://smartr.metr.ou.edu/smartr2/cgi-bin"
    */

    var CGI_Dir = "http://" + window.location.host + "/cgi-bin"
    var CGI_Script = CGI_Dir + "/raxpol_sweep.cgi";

    /* Show "updating" element when awaiting new image */
    var Updating = document.getElementById("updating");
    function ShowUpdating()
    {
	Updating.setAttribute("visibility", "visible");
	Updating.setAttribute("display", "inline");
    }
    function HideUpdating()
    {
	Updating.setAttribute("visibility", "hidden");
	Updating.setAttribute("display", "none");
    }

    /* PlotBnds stores Cartesian and pixel limits of plot for each scan type. */
    var PlotBnds = { PPI : {}, RHI : {} };
    switch (SwpElems["scan_mode"].textContent) {
	case "PPI":
	    PlotBnds["PPI"].cart = GetCart();
	    PlotBnds["PPI"].vbox = PisaPlot.getAttribute("viewBox");
	    PlotBnds["RHI"].cart = null;
	    PlotBnds["RHI"].vbox = null;
	    break;
	case "RHI":
	    PlotBnds["PPI"].cart = null;
	    PlotBnds["PPI"].vbox = null;
	    PlotBnds["RHI"].cart = GetCart();
	    PlotBnds["RHI"].vbox = PisaPlot.getAttribute("viewBox");
	    break;
    }

    /* Fetch a new image */ 
    function GetImg(vol_id, swp_angl, data_type)
    {
	var scan_mode, req_url, request;

	ShowUpdating();
	scan_mode = SwpElems["scan_mode"].textContent;
	PlotBnds[scan_mode].cart = GetCart();
	PlotBnds[scan_mode].vbox = PisaPlot.getAttribute("viewBox");
	req_url = CGI_Script + "?" + "&vol_id=" + vol_id 
	    + "&swp_angl=" + swp_angl + "&data_type=" + data_type;
	if ( SwpElems["case_id"] ) {
	    req_url += "&case_id=" + SwpElems["case_id"].textContent;
	}
	request = new XMLHttpRequest();
	request.open("GET", req_url, true);
	request.onreadystatechange = DpyImg;
	request.send(null);
    }

    /* Install new image sent by XMLHttpRequest */
    function DpyImg(evt)
    {
	var request = this;
	var id;
	var new_doc, new_svg;
	var curr_elems, new_elems;
	var new_cart;
	var curr_child, new_child;
	var scan_mode;
	var bnds, x_left, x_rght, y_btm, y_top, vbox;
	var menus;
	var elem, cart, pisaPlot;

	if ( request.readyState == 4 ) {
	    if ( !request.responseXML ) {
		HideUpdating();
		return;
	    }
	    new_doc = request.responseXML;
	    if ( new_doc.getElementById("no_more_sweeps") ) {
		HideUpdating();
		return;
	    }
	    new_svg = new_doc.childNodes[2];
	    new_elems = new_svg.getElementById("pisa_plot_elements");
	    if ( !new_elems ) {
		HideUpdating();
		return;
	    }
	    RemoveEventListeners();
	    curr_elems = document.getElementById("pisa_plot_elements");
	    curr_elems.parentNode.replaceChild(new_elems, curr_elems);
	    for (id in SwpElems) {
		new_child = new_svg.getElementById(id);
		curr_child = SwpElems[id];
		if ( curr_child ) {
		    curr_child.parentNode.replaceChild(new_child, curr_child);
		}
		SwpElems[id] = document.getElementById(id);
	    }
	    scan_mode = SwpElems["scan_mode"].textContent;
	    if ( !PlotBnds[scan_mode].cart ) {
		elem = new_svg.getElementById("pisa_cartesian");
		cart = elem.textContent.split(/\s+/);
		PlotBnds[scan_mode].cart = { x_left : Number(cart[0]),
		    x_rght : Number(cart[1]), y_btm : Number(cart[2]),
		    y_top : Number(cart[3]) };
	    }
	    SetCart(PlotBnds[scan_mode].cart);
	    if ( !PlotBnds[scan_mode].vbox ) {
		pisaPlot = new_svg.getElementById("pisa_plot");
		PlotBnds[scan_mode].vbox = pisaPlot.getAttribute("viewBox");
	    }
	    PisaPlot.setAttribute("viewBox", PlotBnds[scan_mode].vbox);
	    menus = document.getElementById("raxpol_menus");
	    while ( menus.lastChild ) {
		menus.removeChild(menus.lastChild);
	    }
	    SwpElems["data_type_caption"].onclick = ShowDataTypeMenu;
	    SwpElems["sweep_angle_caption"].onclick = ShowSweepAngleMenu;
	    UpdateAxes();
	    SetVolGeom();
	    CenterCaptions();
	    if ( SwpElems["color_legend"] ) {
		var legend = SwpElems["color_legend"];
		var transform = legend.transform.baseVal.getItem(0);
		var x0 = PisaPlot.x.baseVal.value;
		var w = PisaPlot.width.baseVal.value;
		transform.setTranslate(x0 + w + 24, transform.matrix.f);
	    }
	    HideUpdating();
	    AddEventListeners();
	}
    }

    /* Request image for previous or next volume in case */
    function PrevVolImg(evt)
    {
	var s = SwpElems["prev_vol"].textContent.split("%");
	var prev_vol = s[0];
	var prev_scan_type = s[1];
	var data_type = SwpElems["data_type"].textContent;
	GetImg(prev_vol, SwpAngl[prev_scan_type], data_type);
    }
    function NextVolImg(evt)
    {
	var s = SwpElems["next_vol"].textContent.split("%");
	var next_vol = s[0];
	var next_scan_type = s[1];
	var data_type = SwpElems["data_type"].textContent;
	GetImg(next_vol, SwpAngl[next_scan_type], data_type);
    }

    /* Request image for previous or next sweep in volume */
    function PrevSwpImg(evt)
    {
	var vol_id, swp_angl, data_type;
	var elem, sweep_angles, num_sweep_angles, scan_mode, swp_idx; 

	elem = SwpElems["sweep_angles"];
	sweep_angles = elem.textContent.trim().split(" ");
	swp_idx = SwpElems["swp_idx"].textContent;
	if ( --swp_idx >= 0 ) {
	    swp_angl = sweep_angles[swp_idx];
	} else {
	    return;
	}
	scan_mode = SwpElems["scan_mode"].textContent;
	SwpAngl[scan_mode] = swp_angl;
	vol_id = SwpElems["vol_id"].textContent;
	data_type = SwpElems["data_type"].textContent;
	GetImg(vol_id, swp_angl, data_type);
    }
    function NextSwpImg(evt)
    {
	var vol_id, swp_angl, data_type;
	var elem, sweep_angles, num_sweep_angles, scan_mode, swp_idx; 

	elem = SwpElems["sweep_angles"];
	sweep_angles = elem.textContent.trim().split(" ");
	swp_idx = SwpElems["swp_idx"].textContent;
	if ( ++swp_idx < sweep_angles.length ) {
	    swp_angl = sweep_angles[swp_idx];
	} else {
	    return;
	}
	scan_mode = SwpElems["scan_mode"].textContent;
	SwpAngl[scan_mode] = swp_angl;
	vol_id = SwpElems["vol_id"].textContent;
	data_type = SwpElems["data_type"].textContent;
	GetImg(vol_id, swp_angl, data_type);
    }

    /* Create data type selection menu */
    var DataTypeMenu = {
	svg : CreateSVGElement("svg"),
	items : []
    };
    function ShowDataTypeMenu(evt)
    {
	var caption, fs, data_types, num_data_types, d;
	var item_width, item_ht, x, y, rects, lbls;
	var pisaSVG = document.getElementById("pisa_svg");
	var menus;

	caption = SwpElems["data_type_caption"];
	fs = Number(caption.getAttribute("font-size"));
	item_width = ("DB_XXX".length + 2.0) * fs;
	item_ht = 2.0 * fs;

	data_types = SwpElems["data_types"].textContent.trim().split(" ");
	num_data_types = data_types.length;

	for (d = 0; d < DataTypeMenu.items.length; d++) {
	    DataTypeMenu.svg.removeChild(DataTypeMenu.items[d].g);
	}
	DataTypeMenu.svg.setAttribute("x", evt.clientX);
	DataTypeMenu.svg.setAttribute("y", evt.clientY);
	DataTypeMenu.svg.setAttribute("width", item_width);
	DataTypeMenu.svg.setAttribute("height", num_data_types * item_ht);
	DataTypeMenu.svg.setAttribute("viewBox",
		"0.0 0.0 " + item_width + " " + num_data_types * item_ht);
	DataTypeMenu.items = [];
	for (x = y = 0.0, d = 0; d < num_data_types; d++, y += item_ht) {
	    var item = DataTypeMenu.items[d];
	    if ( !item ) {
		item = {
		    g : CreateSVGElement("g"),
		    rect : CreateSVGElement("rect"),
		    lbl : CreateSVGElement("text")
		};
		DataTypeMenu.items[d] = item;
	    }

	    item.rect.setAttribute("x", x);
	    item.rect.setAttribute("y", y);
	    item.rect.setAttribute("width", item_width);
	    item.rect.setAttribute("height", item_ht);
	    item.rect.setAttribute("stroke", "#000");
	    item.rect.setAttribute("stroke-width", "2.0");
	    item.rect.setAttribute("fill", "#888");

	    item.lbl.setAttribute("x", x + 0.5 * item_width);
	    item.lbl.setAttribute("y", y + 1.5 * fs);
	    item.lbl.setAttribute("text-anchor", "middle");
	    item.lbl.setAttribute("font-size", fs);
	    item.lbl.setAttribute("class", "label");
	    item.lbl.textContent = data_types[d];

	    item.g.appendChild(item.rect);
	    item.g.appendChild(item.lbl);
	    DataTypeMenu.svg.appendChild(item.g);

	    item.g.onclick = function (evt)
	    {
		var item = this;
		var label, vol_id;
		var swp_angl;
		var data_type;

		vol_id = SwpElems["vol_id"].textContent;
		swp_angl= SwpElems["swp_angl"].textContent;
		label = item.getElementsByClassName("label")[0];
		data_type = label.textContent;
		GetImg(vol_id, swp_angl, data_type);
		RemoveDataTypeMenu(evt);
	    };
	}
	menus = document.getElementById("raxpol_menus");
	while ( menus.lastChild ) {
	    menus.removeChild(menus.lastChild);
	}
	menus.appendChild(DataTypeMenu.svg);
	pisaSVG.addEventListener("click", RemoveDataTypeMenu, false);
    }
    function RemoveDataTypeMenu(evt)
    {
	var pisaSVG = document.getElementById("pisa_svg");
	var menus = document.getElementById("raxpol_menus");

	menus.removeChild(DataTypeMenu.svg);
	pisaSVG.removeEventListener("click", RemoveDataTypeMenu, false);
    }

    /* Create sweep angle selection menu */
    var SweepAngleMenu = {
	svg : CreateSVGElement("svg"),
	items : []
    };
    function ShowSweepAngleMenu(evt)
    {
	var caption, fs, elem, sweep_angles, num_sweeps, s;
	var item_width, item_ht, x, y, rects, lbls;
	var pisaSVG = document.getElementById("pisa_svg");
	var menus;

	caption = SwpElems["sweep_angle_caption"];
	fs = Number(caption.getAttribute("font-size"));
	item_width = ("360.0".length + 2.0) * fs;
	item_ht = 2.0 * fs;

	elem = SwpElems["sweep_angles"];
	sweep_angles = elem.textContent.trim().split(" ");
	num_sweeps = sweep_angles.length;

	for (s = 0; s < SweepAngleMenu.items.length; s++) {
	    SweepAngleMenu.svg.removeChild(SweepAngleMenu.items[s].g);
	}
	SweepAngleMenu.svg.setAttribute("x", evt.clientX);
	SweepAngleMenu.svg.setAttribute("y", evt.clientY);
	SweepAngleMenu.svg.setAttribute("width", item_width);
	SweepAngleMenu.svg.setAttribute("height", num_sweeps * item_ht);
	SweepAngleMenu.svg.setAttribute("viewBox", "0.0 0.0 "
		+ item_width + " " + num_sweeps * item_ht);
	SweepAngleMenu.items = [];
	for (x = y = 0.0, s = 0; s < num_sweeps; s++, y += item_ht) {
	    var item = SweepAngleMenu.items[s];
	    if ( !item ) {
		item = {
		    g : CreateSVGElement("g"),
		    rect : CreateSVGElement("rect"),
		    lbl : CreateSVGElement("text")
		};
		SweepAngleMenu.items[s] = item;
	    }

	    item.rect.setAttribute("x", x);
	    item.rect.setAttribute("y", y);
	    item.rect.setAttribute("width", item_width);
	    item.rect.setAttribute("height", item_ht);
	    item.rect.setAttribute("stroke", "#000");
	    item.rect.setAttribute("stroke-width", "2.0");
	    item.rect.setAttribute("fill", "#888");

	    item.lbl.setAttribute("x", x + 0.5 * item_width);
	    item.lbl.setAttribute("y", y + 1.5 * fs);
	    item.lbl.setAttribute("text-anchor", "middle");
	    item.lbl.setAttribute("font-size", fs);
	    item.lbl.setAttribute("class", "label");
	    item.lbl.textContent = sweep_angles[s];

	    item.g.appendChild(item.rect);
	    item.g.appendChild(item.lbl);
	    SweepAngleMenu.svg.appendChild(item.g);

	    item.g.onclick = function (evt)
	    {
		var item = this;
		var label, vol_id, scan_mode;
		var swp_angl;
		var data_type;

		vol_id = SwpElems["vol_id"].textContent;
		scan_mode = SwpElems["scan_mode"].textContent;
		label = item.getElementsByClassName("label")[0];
		swp_angl = label.textContent;
		SwpAngl[scan_mode] = swp_angl;
		data_type = SwpElems["data_type"].textContent;
		GetImg(vol_id, swp_angl, data_type);
		RemoveSweepAngleMenu(evt);
	    }
	}
	menus = document.getElementById("raxpol_menus");
	while ( menus.lastChild ) {
	    menus.removeChild(menus.lastChild);
	}
	menus.appendChild(SweepAngleMenu.svg);
	pisaSVG.addEventListener("click", RemoveSweepAngleMenu, false);
    }
    function RemoveSweepAngleMenu(evt)
    {
	var pisaSVG = document.getElementById("pisa_svg");
	var menus = document.getElementById("raxpol_menus");

	menus.removeChild(SweepAngleMenu.svg);
	pisaSVG.removeEventListener("click", RemoveSweepAngleMenu, false);
    }

    /* Event listeners */
    function ZoomWheel(evt)
    {
	var s = (evt.deltaY > 0.0) ? 1.1 : 0.9;
	ZoomPlot(s, s);
    }
    function ZoomKey(evt)
    {
	var zoom_in = 3.0 / 4.0;
	var zoom_out = 1.0 / zoom_in;
	if ( evt.shiftKey && ( evt.keyCode == 61 || evt.keyCode == 187 ) ) {
	    ZoomPlot(zoom_in, zoom_in);
	} else if ( evt.keyCode == 43
		|| evt.keyCode == 173
		|| evt.keyCode == 189 ) {
	    ZoomPlot(zoom_out, zoom_out);
	}
    }
    function BrowseVol(evt)
    {
	var arrowLeft = 37, h = 72;	/* Non-standard */
	var arrowRght = 39, l = 76;
	switch (evt.keyCode) {
	    case arrowLeft:
	    case h:
		PrevVolImg(evt);
		return;
	    case arrowRght:
	    case l:
		NextVolImg(evt);
		return;
	}
    }
    function BrowseSweep(evt)
    {
	var arrowDown = 40, j = 74;	/* Non-standard */
	var arrowUp = 38, k = 75;
	switch (evt.keyCode) {
	    case arrowDown:
	    case j:
		PrevSwpImg(evt);
		break;
	    case arrowUp:
	    case k:
		NextSwpImg(evt);
		break;
	}
    }
    function AddEventListeners() {
	PisaPlot.addEventListener("mousedown", StartPlotDrag, false);
	PisaPlot.addEventListener("wheel", ZoomWheel, false);
	window.addEventListener("keydown", ZoomKey, false);
	window.addEventListener("resize", ReSize, true);
	window.addEventListener("resize", RaXPol_ReSize, true);
	if ( CGI_Script ) {
	    window.addEventListener("keydown", BrowseVol, false);
	    window.addEventListener("keydown", BrowseSweep, false);
	    SwpElems["sweep_angle_caption"].onclick = ShowSweepAngleMenu;
	    SwpElems["data_type_caption"].onclick = ShowDataTypeMenu;
	}
    }
    AddEventListeners();
    function RemoveEventListeners() {
	PisaPlot.removeEventListener("mousedown", StartPlotDrag, false);
	PisaPlot.removeEventListener("wheel", ZoomWheel, false);
	window.removeEventListener("keydown", ZoomKey, false);
	window.removeEventListener("resize", ReSize, true);
	window.removeEventListener("resize", RaXPol_ReSize, true);
	if ( CGI_Script ) {
	    window.removeEventListener("keydown", BrowseVol, false);
	    window.removeEventListener("keydown", BrowseSweep, false);
	    SwpElems["sweep_angle_caption"].onclick = null;
	    SwpElems["data_type_caption"].onclick = null;
	}
    }

    /*
       Redraw with javascript. This prevents sudden changes
       in the image if the static document produced by pisa.awk
       noticeably differs from the Javascript rendition.
     */

    while ( XAxis.lastChild ) {
	XAxis.removeChild(XAxis.lastChild);
    }
    while ( YAxis.lastChild ) {
	YAxis.removeChild(YAxis.lastChild);
    }
    ReSize.call(this, {});
    RaXPol_ReSize.call(this, {});

}, false);

