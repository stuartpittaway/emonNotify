  
//emontx v2.2 circuit board measures
//63.3m width by 56mm high, 1.6mm thick
//we use in portrait format

/*
translate([0,10,2])
rotate([90,90,0]) 
union() {
color("green",0.25) 
cube(size = [63.3, 56, 1.6], center = true);
color("purple",0.25) 
translate([0,0,(18-1.6)/2])
cube(size = [63.3, 56, 18-1.6], center = true);
}
*/


module my_model() {

difference() {
    //egg shape around circuit board
    //chop bottom off egg but leave solid base
    color("red",0.2) scale([2.0,1.0,2.2]) difference() {

        //Outside the egg deliberately not smooth so you can see
        //the polygons
        difference() {
            sphere(r=25.0, $fa=15, $fs=0.1); 
            translate([-30,-30, -75]) cube([60,60,60], false); 
        }
        
          //Cut middle out of egg
        difference() {
            sphere(r=23.0, $fa=15, $fs=0.1); 
            translate([-30,-30, -74]) cube([60,60,60], false); 
        }        
        
}

    


//Chop of top
//translate([-55,-50, 21]) cube([110,110,100], false);       

//Hole for LCD screen with rounded corners
translate([0,-25,8.01]) 
    minkowski()
    {
       cube([32,15,22],true); 
       sphere(r=2,$fn=50);
    }
    
        
}//end of difference


 //LCD screen holder
translate([0,-20+5,5])
difference() {
    union(){
    translate([0,0,2.5]) cube(size = [50, 8, 29.5], center = true);

    translate([0,3.5,-9+0.1]) cube(size = [50, 5, 58], center = true);
    }

    translate([0,-1.1,0]) cube(size = [40, 15, 40.1], center = true);

    union() {
    translate([0,1.9,1]) color("green") cube(size = [43, 1.9, 45.25], center = true);
    //Actual size of metal surround on screen
    color("red") cube(size = [40, 0.1+4.5, 34.5], center = true);
    }
}


//PCB holder in base
color("red",1.0) 
translate([0,10,-28])
rotate([90,90,0]) 
difference() {
    cube(size = [8, 60, 5], center = true);
    translate([-2,0,0]) cube(size = [8, 56, 1.6], center = true);  
    translate([-2,0,3]) cube(size = [6, 45, 5], center = true);    
}
       

}

//Clone model and chop into two
translate([0,0,0])
difference() {
my_model();
translate([0,00,-29.0]) cube(size = [120, 120, 100], center = true);
}

translate([110,0,0])
difference() {
my_model();
translate([0,00,71.0]) cube(size = [120, 120, 100], center = true);
}
