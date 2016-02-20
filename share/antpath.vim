'a,'bw !awk '{print $10 " " $12}' | xyplot -X 'azimuth' -Y 'tilt' 0.0 360.0 0.0 90.0 > antpath.svg
