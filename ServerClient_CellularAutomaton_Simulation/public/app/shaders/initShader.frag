#version 300 es

precision highp float ;
precision highp int ;

in vec2 cc ;

layout (location =0) out float o_col_0 ; // fragment color set 0
layout (location =1) out float o_col_1 ; // fragment color set 1

void main(){
    float red = 0. ;

    o_col_0 = red ;
    o_col_1 = red ;
    return ;
}
