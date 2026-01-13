// GC9A01 Round LCD Module Model
// All dimensions are in millimeters, based on the provided datasheet image.

// --- Parameters ---

// PCB Main Body
pcb_thickness = 1.6;
pcb_diameter = 38.03;
pcb_total_height = 45.80;

// LCD Display
lcd_aa_diameter = 32.40; // Active Area
lcd_thickness = 1.50;

// Bottom Rectangular Part
rect_pcb_width = 23.00;
rect_pcb_height = 11.64;
hole_spacing = 19.40;
hole_diameter = 2.2; // M2 screw is a common size for this
pin_header_y_offset = 5.82; // y-position from the bottom edge

// Pin Header
pin_count = 8;
pin_pitch = 2.54;
pin_size = [0.6, 0.6, 11.78]; // Square pin dimensions [x, y, z]

// --- Calculations ---
pcb_radius = pcb_diameter / 2;
// Y-coordinate for the center of the circle, so the total height is correct.
circle_y_center = pcb_total_height - pcb_radius;


// --- Modules ---

// Module for the main PCB shape
module pcb_base() {
    // Use hull() to create straight lines connecting the two shapes.
    // Their positions are corrected to overlap for a better connection.
    hull() {
        // Top circular part
        translate([0, circle_y_center, 0])
            circle(d = pcb_diameter);

        // Bottom rectangular part
        translate([-rect_pcb_width/2, 0, 0])
            square([rect_pcb_width, rect_pcb_height]);
    }
}

// Module for the pin headers
module pin_headers() {
    total_pin_width = (pin_count - 1) * pin_pitch;
    for (i = [0:pin_count-1]) {
        translate([-total_pin_width/2 + i * pin_pitch, pin_header_y_offset, pcb_thickness])
            cube(pin_size, center=true);
    }
}

// Module for the pin header holes
module pin_header_holes() {
    pin_hole_diameter = 1.0; // Standard size for pin header holes
    total_pin_width = (pin_count - 1) * pin_pitch;
    for (i = [0:pin_count-1]) {
        translate([-total_pin_width/2 + i * pin_pitch, pin_header_y_offset, 0])
            circle(d=pin_hole_diameter, $fn=16);
    }
}


// --- Assembly ---

// Create the final 3D model
union() {
    // 1. Main PCB board
    linear_extrude(height = pcb_thickness) {
        difference() {
            pcb_base();
            
            // Mounting holes
            translate([-hole_spacing/2, rect_pcb_height - 2.61, 0])
                circle(d=hole_diameter, $fn=32);
            translate([hole_spacing/2, rect_pcb_height - 2.61, 0])
                circle(d=hole_diameter, $fn=32);
            
            // Pin header holes
            pin_header_holes();
        }
    }
    
    // 2. LCD Screen representation
    translate([0, circle_y_center, pcb_thickness]) {
        cylinder(d=lcd_aa_diameter, h=lcd_thickness, $fn=64, center=false);
    }

    // 3. Pin Headers
    // pin_headers();
}

// Example of how to view the model
// Just the PCB without components
// linear_extrude(height = pcb_thickness) pcb_base();
