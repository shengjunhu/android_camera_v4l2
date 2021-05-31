#version 310 es
precision mediump float;
precision mediump sampler2D;
const mat3 convert = mat3(1.0,1.0,1.0,0.0,-0.39465,1.53211,1.13983,-0.58060,0.0);
in vec2 texCoord;
uniform sampler2D texY;
uniform sampler2D texUV;
out vec4 fragColor;

void main() {
   vec3 yuv;
   yuv.x = texture(texY, texCoord).r + 0.0625;
   yuv.y = texture(texUV,texCoord).r - 0.5;
   yuv.z = texture(texUV,texCoord).a - 0.5;
   fragColor = vec4(convert * yuv, 1.0);
}