#!/bin/sh
"exec" "python3" "$0" "$@"

import os
import sys
import cadquery as cq

def convert_step_to_stl(step_path, stl_path, tolerance=0.001, angular_tolerance=0.1):
    """
    Converts a STEP file to an STL file using CadQuery.

    :param step_path: Path to the input STEP file.
    :param stl_path: Path to the output STL file.
    :param tolerance: Linear tolerance for meshing.
    :param angular_tolerance: Angular tolerance for meshing.
    """
    # Get the directory of the script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Resolve absolute paths
    abs_step_path = os.path.abspath(os.path.join(script_dir, step_path))
    abs_stl_path = os.path.abspath(os.path.join(script_dir, stl_path))
    
    if not os.path.exists(abs_step_path):
        print(f"Error: Input file not found at {abs_step_path}")
        sys.exit(1)

    try:
        print(f"Reading STEP file: {abs_step_path}...")
        # Import the STEP file
        result = cq.importers.importStep(abs_step_path)

        # Create the output directory if it doesn't exist
        output_dir = os.path.dirname(abs_stl_path)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
            print(f"Created directory: {output_dir}")

        print(f"Writing STL file: {abs_stl_path}...")
        # Export the result to an STL file
        cq.exporters.export(
            result, 
            abs_stl_path, 
            exportType='STL',
            tolerance=tolerance,
            angularTolerance=angular_tolerance
        )

        print("Conversion successful!")

    except Exception as e:
        print(f"An error occurred during conversion: {e}")
        sys.exit(1)

if __name__ == "__main__":
    # --- Configuration ---
    # Paths are relative to the script directory
    input_file = "../OpenSCAD/step/XIAO-ESP32S3 v2.step"
    output_file = "../OpenSCAD/stl/XIAO-ESP32S3 v2.stl"
    
    # --- Conversion ---
    convert_step_to_stl(input_file, output_file)