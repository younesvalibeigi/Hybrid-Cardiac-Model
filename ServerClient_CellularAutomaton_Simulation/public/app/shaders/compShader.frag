#version 300 es
precision highp float ;
precision highp int ;

uniform sampler2D   in_txt ;
uniform sampler2D   in_itxt ;
layout (location =0) out float out_txt ; // output color of the shader

in vec2 cc ;


uniform float radius, threshold, Lx; 
uniform int cameraFeedback;

void main() {
    vec2 size   = vec2(textureSize(in_txt,0)) ; //size of texture
    vec2 ii     = vec2(1.,0.)/size ; // Unit vector in x  //size of pixel
    vec2 jj     = vec2(0.,1.)/size ; // .............. y

    float dx = Lx/size.x ;
    float dy = dx ;

    vec4  C = texture(in_txt, cc) ;
    vec2  pert = texture(in_itxt, cc).xy ;
    float ifBlock = texture(in_itxt, cc).z; //*--> gives me the block condition
    vec2  cellCoord = cc*Lx + pert ;
    //C(Voltate, ..., xrand, y.rand)
    
    if(ifBlock == 0.){   //* If the pixel is part of the block
        C.r = -1.;
    }else{




       // if(C.r < 0.05){

        // if(C.r>0.0){
        //     C.r = C.r - 0.01;
        // }
        // if(C.r<0.){
        //     C.r = 0.;
        // }
        
        // float radius2 = radius;

        // // Now run the C.A. Algorithm
        // float aliveNeighbors = 0.;
        // float deadNeighbors = 0.;

        // int m = int(round(radius2/dx)) ;
        // int n = int(round(radius2/dy)) ;
        // for (int i=-m; i<(m+1) ;i++){
        //     for (int j=-n; j<(n+1); j++ ){

        //         vec2 currPos =  cc+float(i)*ii+float(j)*jj ;
        //         float reachBlock = texture(in_itxt, currPos).z;
        //             if (reachBlock == 1.){      //* No block
        //                 float voltage = texture(in_txt,currPos).r ; //texture
        //                 pert = texture(in_itxt,currPos).xy ;

        //                 vec2 compCellCoord= currPos*vec2(Lx,Lx) + pert ;//physical

        //                 float distance = length(cellCoord-compCellCoord) ;
        //                 if(distance<radius2){
        //                     if ( (voltage >0.7) && (voltage<0.99) ){
        //                         aliveNeighbors+=1.;
        //                     }else{
        //                         deadNeighbors+=1.;
        //                     }
        //                 }
        //             } else if (reachBlock == 0.) {
        //                 currPos = cc+float(i)*ii*(-1.)+float(j)*jj*(-1.) ;  //* mirroring
        //                 float voltage = texture(in_txt,currPos).r ; //texture
        //                 pert = texture(in_itxt,currPos).xy ;

        //                 vec2 compCellCoord= currPos*vec2(Lx,Lx) + pert ;//physical

        //                 float distance = length(cellCoord-compCellCoord) ;
        //                 if(distance<radius2){
        //                     if ( (voltage >0.7) && (voltage<0.99) ){
        //                         aliveNeighbors+=1.;
        //                     }else{
        //                         deadNeighbors+=1.;
        //                     }
        //                 }
        //             }
                
        //     }
        // }

        // float threshold2 = 0.;
        // //if (C.r>0.1){
        //     threshold2 = threshold+C.r*10.;
        // //}

        // if ((aliveNeighbors/(deadNeighbors+aliveNeighbors))>(threshold2)){
        //     C.r =  0.98;
        // }else {
        //     //C.r = 0.;
        //     //C.r = C.r - 0.051;
        // }
        // // implement the input from camera
        // if (cameraFeedback == 1){ // This is working, put == 0 you get pacemaker
        //     if(cc.x<0.01 && 0.4<cc.y && cc.y<0.6){ // make the left side of the texture active
        //         C.r = 0.98;//C.r + 0.95;
        //     }
        // }
        
        
        // }else{  
        //     C.r = C.r - 0.051;
        // }

        
        if(C.r < 0.05){
            
            // Now run the C.A. Algorithm
            float aliveNeighbors = 0.;
            float deadNeighbors = 0.;

            int m = int(round(radius/dx)) ;
            int n = int(round(radius/dy)) ;
            for (int i=-m; i<(m+1) ;i++){
                for (int j=-n; j<(n+1); j++ ){

                    vec2 currPos =  cc+float(i)*ii+float(j)*jj ;
                    float reachBlock = texture(in_itxt, currPos).z;
                        if (reachBlock == 1.){      //* No block
                            float voltage = texture(in_txt,currPos).r ; //texture
                            pert = texture(in_itxt,currPos).xy ;

                            vec2 compCellCoord= currPos*vec2(Lx,Lx) + pert ;//physical

                            float distance = length(cellCoord-compCellCoord) ;
                            if(distance<radius){
                                if ( (voltage >0.7) && (voltage<0.99) ){
                                    aliveNeighbors+=1.;
                                }else{
                                    deadNeighbors+=1.;
                                }
                            }
                        } else if (reachBlock == 0.) {
                            currPos = cc+float(i)*ii*(-1.)+float(j)*jj*(-1.) ;  //* mirroring
                            float voltage = texture(in_txt,currPos).r ; //texture
                            pert = texture(in_itxt,currPos).xy ;

                            vec2 compCellCoord= currPos*vec2(Lx,Lx) + pert ;//physical

                            float distance = length(cellCoord-compCellCoord) ;
                            if(distance<radius){
                                if ( (voltage >0.7) && (voltage<0.99) ){
                                    aliveNeighbors+=1.;
                                }else{
                                    deadNeighbors+=1.;
                                }
                            }
                        }
                    
                }
            }
                
            if ((aliveNeighbors/deadNeighbors)>(threshold)){
                C.r =  0.98;
            }else {
                C.r = 0.;
                //C.r = C.r - 0.051;
            }
            // implement the input from camera
            if (cameraFeedback == 1){ // This is working, put == 0 you get pacemaker
                if(cc.x<0.01 && 0.4<cc.y && cc.y<0.6){ // make the left side of the texture active
                    C.r = 0.98;//C.r + 0.95;
                }
            }
            
        }else{  
            C.r = C.r - 0.051;
        }
        




        
    }
    
    

    out_txt = C.r ;


    return ;
}
