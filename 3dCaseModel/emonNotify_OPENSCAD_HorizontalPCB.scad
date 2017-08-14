  
//emontx v2.2 circuit board measures
//63.3m width by 56mm high, 1.6mm thick
//we use in portrait format



sphere_scale=[2.1,1.0,2.1];
lcd_screen_hole = [28,15,22];

module my_model() {

/*
//Circuit board horizontal with USB power on bottom
translate([0,10,-0.5])
rotate([90,0,0]) 
union() {
color("green",0.25) 
cube(size = [63.3, 56, 1.6], center = true);
color("purple",0.25) 
translate([0,0,(18-1.6)/2])
cube(size = [63.3, 56, 18-1.6], center = true);
}
*/



difference() {
    //egg shape around circuit board
    //chop bottom off egg but leave solid base
    color("red",0.2) scale(sphere_scale) difference() {

        //Outside the egg deliberately not smooth so you can see
        //the polygons
        difference() {
            sphere(r=25.0, $fa=15, $fs=0.1); 
            translate([-30,-30, -75]) cube([60,60,60], false); 
                                
        }
        
          //Cut middle out of egg
        difference() {
            sphere(r=24.0, $fa=15, $fs=0.1); 
            translate([-30,-30, -74]) cube([60,60,60], false);           
        }    
        
}

 union() {
//Cut hole in base for cables and programming socket
     
     //25mm wide slit for usb program
translate([-18,4.5,-35]) cube([22, 6, 12], center = true);
     //hole for usb cable to power unit (usb plug is 8mm by 16mm)
translate([0,23-7,-32]) cube([8.5, 30, 16], center = true);
}


union() {   
//Vent holes in the roof - vertical so can be 3d printed
 for (x =[-18:4.5:22]) {    
     //15mm high slits, 1.5mm wide
    translate([x,20,35]) cube([1.5, 20, 15], center = true);
 }
}//end union


//Hole for LCD screen with rounded corners
translate([0,-25,8.01]) 
    minkowski()
    {
       cube(lcd_screen_hole,true); 
       sphere(r=2,$fn=50);
    }         
}//end of difference




difference() {
intersection() {
color("blue",0.1) scale(sphere_scale)
    difference() {
            sphere(r=25.0, $fa=15, $fs=0.1); 
            translate([-30,-30, -75]) cube([60,60,60], false); 
                                
        }

 //LCD screen holder
translate([0,-20+5,5])    
difference() {
    translate([0,-2,-22]) cube(size = [60, 13, 80], center = true);
    translate([0,5,0]) cube(size = [40, 15, 40.1], center = true);

    union() {        
        //mock up of LCD display
        //PCB
    translate([0,1.9,1]) cube(size = [43, 1.9, 45.25], center = true);
    //Actual size of metal surround on screen
    cube(size = [40, 0.1+4.5, 034.5], center = true);
    }
}//end difference

}




//Hole for LCD screen with rounded corners
translate([0,-25,8.01]) 
    minkowski()
    {
       cube(lcd_screen_hole,true); 
       sphere(r=2,$fn=50);
    }  
}   

//PCB holder in base
pcb_holder_height=10;
color("red",1.0) 
translate([0,9.25,-25.5])
rotate([90,90,0]) 
difference() {
    cube(size = [pcb_holder_height, 68, 5], center = true);
    //Gap for pcb is 2.0mm
    cube(size = [pcb_holder_height+0.1, 65.0, 2.0], center = true);  
    cube(size = [pcb_holder_height+0.1, 65.0 - 6, 5.1], center = true);    
}//end difference


} //end module

//my_model();

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
