#version 300 es
precision highp float ;
precision highp int ;



precision highp  usampler2D ; /* lowp : 8bit, mediump : 16 bit, highp : 32bit */


in vec2 cc, pixPos ;  // input from the vertex shader
uniform usampler2D txtCamera ;

out vec4 outcolor ; 
// Main body of the shader
void main() {
    uint red = texture( txtCamera, cc ).r ;
    outcolor= vec4( float(red)/30. , 0., 0.,1.) ; 

    return ;
}