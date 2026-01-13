#!/bin/bash
cd "$(dirname "$0")"
openscad -o "OpenSCAD/stl/XIAO-ESP32S3 v2.stl" "OpenSCAD/scripts/XIAO-ESP32S3 v2.scad"
