attribute vec4 vPosition;
attribute vec2 vTexCoord;
varying vec2 texCoord;
varying vec2 blurCoord1s[14];
const highp float mWidth=800.0;
const highp float mHeight=1280.0;

void main() {
    gl_Position = vPosition;
    texCoord = vTexCoord;

    highp float mul_x = 2.0 / mWidth;
    highp float mul_y = 2.0 / mHeight;

    // 14个采样点
    blurCoord1s[0] = vTexCoord + vec2(0.0 * mul_x, -10.0 * mul_y);
    blurCoord1s[1] = vTexCoord + vec2(8.0 * mul_x, -5.0 * mul_y);
    blurCoord1s[2] = vTexCoord + vec2(8.0 * mul_x, 5.0 * mul_y);
    blurCoord1s[3] = vTexCoord + vec2(0.0 * mul_x, 10.0 * mul_y);
    blurCoord1s[4] = vTexCoord + vec2(-8.0 * mul_x, 5.0 * mul_y);
    blurCoord1s[5] = vTexCoord + vec2(-8.0 * mul_x, -5.0 * mul_y);
    blurCoord1s[6] = vTexCoord + vec2(0.0 * mul_x, -6.0 * mul_y);
    blurCoord1s[7] = vTexCoord + vec2(-4.0 * mul_x, -4.0 * mul_y);
    blurCoord1s[8] = vTexCoord + vec2(-6.0 * mul_x, 0.0 * mul_y);
    blurCoord1s[9] = vTexCoord + vec2(-4.0 * mul_x, 4.0 * mul_y);
    blurCoord1s[10] = vTexCoord + vec2(0.0 * mul_x, 6.0 * mul_y);
    blurCoord1s[11] = vTexCoord + vec2(4.0 * mul_x, 4.0 * mul_y);
    blurCoord1s[12] = vTexCoord + vec2(6.0 * mul_x, 0.0 * mul_y);
    blurCoord1s[13] = vTexCoord + vec2(4.0 * mul_x, -4.0 * mul_y);
}