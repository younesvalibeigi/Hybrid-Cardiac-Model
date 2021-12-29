#version 300 es 

precision highp float ;
precision highp int ;

uniform sampler2D   in_txt ;
uniform sampler2D	in_itxt;		//* Added
uniform vec2        clickPosition ;
uniform vec2		clickShiftPosition;
uniform vec2		clickCtrlPosition;
uniform float       clickRadius ;
uniform bool 		shiftClick ;
uniform bool 		ctrlClick ;

in vec2 pixPos ;

layout (location =0) out vec4 out_txt ;
layout (location =1) out vec4 out_itxt ;

void main(){
    
    
    vec4 txt  = texture(in_txt, pixPos) ;		
    vec4 itxt = texture(in_itxt, pixPos);		//* Added

    if (shiftClick){
    	if ( length(pixPos-clickShiftPosition)<clickRadius){ //* We cannot check the equality of two foats
    		itxt.z = 0.;	
    	}						//* This it the only way
    }else if(ctrlClick){
        if ( length(pixPos-clickCtrlPosition)<clickRadius){ //* We cannot check the equality of two foats
    		itxt.z = 1.;	
    	}
    }else{
    	if ( length(pixPos-clickPosition)<clickRadius){
        	txt.r = 0.98 ;
    	}
    }
    

    

    out_txt = txt ;
    out_itxt = itxt;
}
