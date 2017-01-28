$fn=50;

module spodek() {
  rotate([180,0,0])
  difference() {
    cube ([53,23,13], center=true);
    translate([0,0,-0.51]) cube ([50,20,12], center=true);
    translate([25,0,0]) rotate([90,0,90]) cylinder (d=8,h=5, center=true);
  }
}

module vrsek() {

    difference() {
      cube ([50,20,2],center=true);
      translate([-10,0,-2])
      hull() {
        cube ([20,15,1], center=true);
        translate([0,0,5]) cube ([23,20,1], center=true);
      }
    
    
    translate([10,-7.2,0.6]) rotate([0,0,90]) linear_extrude(height=1) text("ppet36",size=3.5, font="Arimo");
    translate([17,-8.5,0.6]) rotate([0,0,90]) linear_extrude(height=1) text("THERM",size=3.5, font="Arimo");
    }
}

spodek();
translate([0,30,0]) vrsek();