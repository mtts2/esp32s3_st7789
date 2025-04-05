/////////////////////////////////////////////////////////////////
// ESP32-S3 + ST7789 Amiga/Tesseract Style Megademo with LovyanGFX
/////////////////////////////////////////////////////////////////

// --- ヘッダーファイル ---
#include <LovyanGFX.hpp>

// ユーザー定義のLCD設定ファイル (ST7789用)
#include <st7789.hpp>

#include <vector>
#include <cmath>
#include <stdlib.h> // for random, srand

// --- 定数とユーティリティ ---
#define DEG_TO_RAD (M_PI / 180.0f)

// Arduino関数エミュレーション (ESP-IDF環境用)
inline long random(long howbig) {
    if (howbig == 0) return 0;
    return ::random() % howbig;
}
inline long random(long howsmall, long howbig) {
    if (howsmall >= howbig) return howsmall;
    long diff = howbig - howsmall;
    return random(diff + 1) + howsmall;
}
inline int constrain(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
inline float constrain(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
inline uint32_t millis(void) {
    return lgfx::v1::millis();
}
inline void randomSeed(unsigned long seed) {
    srand(seed);
}

// --- LCDインスタンス ---
static LGFX_ST7789 lcd;

// --- デモ用グローバル変数 & 構造体 ---
uint32_t demo_start_time = 0;
float scroll_pos = 0.0f;
float angle = 0.0f;         // 汎用角度 (テッセラクト YZ回転)
float copper_angle = 0.0f;    // コッパーバー波打ち用角度
float angle_xy = 0.0f;        // テッセラクト XY回転角度
float angle_zw = 0.0f;        // テッセラクト ZW回転角度
float angle_xw = 0.0f;        // テッセラクト XW回転角度

// 星構造体
struct Star {
    float x, y, z;
    uint16_t color;
};
std::vector<Star> stars;
const int NUM_STARS = 600;

// 4Dベクトル構造体
struct Vec4 {
    float x, y, z, w;
};

// 3Dベクトル構造体
struct Vec3 {
    float x, y, z;
};

// テッセラクト頂点データ (16個)
Vec4 tesseract_points[16];
// テッセラクト辺接続情報 (頂点インデックスのペア, 32本)
const int tesseract_edges[32][2] = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}, {8, 9}, {9, 11}, {11, 10}, {10, 8},
    {12, 13}, {13, 15}, {15, 14}, {14, 12}, {8, 12}, {9, 13}, {10, 14}, {11, 15},
    {0, 8}, {1, 9}, {2, 10}, {3, 11}, {4, 12}, {5, 13}, {6, 14}, {7, 15}
};

// --- ヘルパー関数 ---

// HSVからRGB565への変換
uint16_t hsvToRgb565(float h, float s, float v) {
    h = fmod(h, 360.0f); if (h < 0) h += 360.0f;
    s = constrain(s, 0.0f, 1.0f); v = constrain(v, 0.0f, 1.0f);
    float r, g, b;
    int i = floor(h / 60.0f); float f = h / 60.0f - i;
    float p = v * (1.0f - s); float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break; case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break; case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break; default:r=v; g=p; b=q; break;
    }
    return lcd.color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

// 星の初期化
void init_star(Star& star) {
    // 星の位置を初期化
    star.x = (float)random(-lcd.width() * 6, lcd.width() * 6) / 10.0f;
    star.y = (float)random(-lcd.height() * 6, lcd.height() * 6) / 10.0f;
    star.z = (float)random(1, lcd.width());
    
    // 星の色をランダムに（白に近い明るい色）
    float hue = random(0, 360);
    float sat = random(0, 20) / 100.0f; // 彩度を低めに
    float val = random(70, 100) / 100.0f; // 明るさを高めに
    star.color = hsvToRgb565(hue, sat, val);
}

// デモ全体の初期化
void setup_amiga_demo() {
    demo_start_time = millis();
    scroll_pos = lcd.width();
    angle = 0.0f; angle_xy = 0.0f; angle_zw = 0.0f; angle_xw = 15.0f;
    copper_angle = 0.0f;

    // 星の初期化
    stars.resize(NUM_STARS);
    randomSeed(millis());
    for (int i = 0; i < NUM_STARS; ++i) init_star(stars[i]);

    // テッセラクト頂点の初期化
    int idx = 0; float s = 1.0f;
    tesseract_points[idx++] = {-s/2, -s/2, -s/2, -s/2}; // 0 W=-1 Z=-1 Y=-1 X=-1
    tesseract_points[idx++] = { s/2, -s/2, -s/2, -s/2}; // 1 W=-1 Z=-1 Y=-1 X=+1
    tesseract_points[idx++] = {-s/2,  s/2, -s/2, -s/2}; // 2 W=-1 Z=-1 Y=+1 X=-1
    tesseract_points[idx++] = { s/2,  s/2, -s/2, -s/2}; // 3 W=-1 Z=-1 Y=+1 X=+1
    tesseract_points[idx++] = {-s/2, -s/2,  s/2, -s/2}; // 4 W=-1 Z=+1 Y=-1 X=-1
    tesseract_points[idx++] = { s/2, -s/2,  s/2, -s/2}; // 5 W=-1 Z=+1 Y=-1 X=+1
    tesseract_points[idx++] = {-s/2,  s/2,  s/2, -s/2}; // 6 W=-1 Z=+1 Y=+1 X=-1
    tesseract_points[idx++] = { s/2,  s/2,  s/2, -s/2}; // 7 W=-1 Z=+1 Y=+1 X=+1
    tesseract_points[idx++] = {-s/2, -s/2, -s/2,  s/2}; // 8 W=+1 Z=-1 Y=-1 X=-1
    tesseract_points[idx++] = { s/2, -s/2, -s/2,  s/2}; // 9 W=+1 Z=-1 Y=-1 X=+1
    tesseract_points[idx++] = {-s/2,  s/2, -s/2,  s/2}; // 10 W=+1 Z=-1 Y=+1 X=-1
    tesseract_points[idx++] = { s/2,  s/2, -s/2,  s/2}; // 11 W=+1 Z=-1 Y=+1 X=+1
    tesseract_points[idx++] = {-s/2, -s/2,  s/2,  s/2}; // 12 W=+1 Z=+1 Y=-1 X=-1
    tesseract_points[idx++] = { s/2, -s/2,  s/2,  s/2}; // 13 W=+1 Z=+1 Y=-1 X=+1
    tesseract_points[idx++] = {-s/2,  s/2,  s/2,  s/2}; // 14 W=+1 Z=+1 Y=+1 X=-1
    tesseract_points[idx++] = { s/2,  s/2,  s/2,  s/2}; // 15 W=+1 Z=+1 Y=+1 X=+1

    lcd.setTextSize(1); // デフォルト文字サイズ
    printf("Hyper Tesseract Demo Initialized!\n");
}

// --- セットアップ関数 ---
void setup(void)
{
    lcd.init();
    printf("LCD Initialized.\n");

    lcd.setRotation(0); // ★画面の向き (0, 1, 2, 3) 環境に合わせて変更★
    lcd.setBrightness(128);
    lcd.setColorDepth(24); // 24bitカラー

    setup_amiga_demo(); // デモ要素の初期化
    lcd.fillScreen(TFT_BLACK); // 最初の画面クリア
}

// --- ループ関数 (メイン描画処理) ---
void loop(void)
{
    // 時間計算
    uint32_t current_time = millis();
    static uint32_t last_frame_time = 0;
    float frame_delta_sec = (current_time - last_frame_time) / 1000.0f;
    if (frame_delta_sec <= 0.0f || frame_delta_sec > 0.1f) frame_delta_sec = 1.0f / 60.0f;
    last_frame_time = current_time;
    float elapsed_sec = (current_time - demo_start_time) / 1000.0f;

    // 描画バッファリング開始
    lcd.startWrite();

    // 1. 背景: スターフィールド
    lcd.fillScreen(TFT_BLACK);
    int center_x = lcd.width() / 2;
    int center_y = lcd.height() / 2;
    float star_speed = frame_delta_sec * 120.0f;
    for (int i = 0; i < NUM_STARS; ++i) {
        stars[i].z -= star_speed;
        if (stars[i].z <= 0.1f) { init_star(stars[i]); stars[i].z = lcd.width(); }
        float inv_z = 1.0f / stars[i].z;
        int sx = center_x + (int)(stars[i].x * inv_z * center_x * 1.2f);
        int sy = center_y + (int)(stars[i].y * inv_z * center_y * 1.2f);
        if (sx >= 0 && sx < lcd.width() && sy >= 0 && sy < lcd.height()) {
            float size_factor = constrain(2.5f - stars[i].z / (lcd.width() / 4.0f), 1.0f, 3.0f);
            int star_size = (int)size_factor;
            float bright_factor = constrain(1.0f - stars[i].z / (lcd.width() * 1.5f) , 0.2f, 1.0f);
            
            // brightness_adjustがないので、自前で明るさを調整
            uint16_t r = ((stars[i].color >> 11) & 0x1F) * bright_factor;
            uint16_t g = ((stars[i].color >> 5) & 0x3F) * bright_factor;
            uint16_t b = (stars[i].color & 0x1F) * bright_factor;
            uint16_t adjusted_color = ((uint16_t)(r) << 11) | ((uint16_t)(g) << 5) | (uint16_t)(b);
            uint32_t final_color = lcd.color16to24(adjusted_color);
            if (star_size > 1) lcd.fillRect(sx - star_size / 2, sy - star_size / 2, star_size, star_size, final_color);
            else lcd.drawPixel(sx, sy, final_color);
        }
    }

    // 2. 中景: コッパーバー (波打ち) - 暗くする
    int copper_y_start = lcd.height() / 2 - 50; int copper_height = 100;
    copper_angle += frame_delta_sec * 120.0f;
    for (int y = 0; y < copper_height; ++y) {
        float y_ratio = (float)y / copper_height;
        float hue = fmod(elapsed_sec * 80.0f + y * 1.0f, 360.0f);
        // 明るさを下げる（0.6→0.3、0.4→0.2）
        float brightness = 0.3f + 0.2f * sinf(elapsed_sec*5.f+y_ratio*10.f)*cosf(elapsed_sec*2.5f+y_ratio*5.f);
        // 彩度も若干下げて色を薄く
        uint16_t color16 = hsvToRgb565(hue, 0.8f, constrain(brightness, 0.1f, 0.5f));
        float wave_amplitude = 30.f * sinf(elapsed_sec * 3.f);
        int x_offset = (int)(wave_amplitude * sinf(copper_angle * DEG_TO_RAD + y_ratio * 4.f * M_PI));
        int start_x = constrain(x_offset - lcd.width()/2, 0, lcd.width());
        int end_x = constrain(x_offset + lcd.width()/2 + lcd.width(), 0, lcd.width());
        int line_width = end_x - start_x;
        if(line_width > 0) lcd.drawFastHLine(start_x, copper_y_start + y, line_width, color16);
    }

    // 3. 主役: 回転するテッセラクト
    angle += frame_delta_sec * 30.f; angle_xy += frame_delta_sec * 40.f;
    angle_zw += frame_delta_sec * 50.f; angle_xw += frame_delta_sec * 60.f;
    float cos_yz = cosf(angle*DEG_TO_RAD),    sin_yz = sinf(angle*DEG_TO_RAD);
    float cos_xy = cosf(angle_xy*DEG_TO_RAD), sin_xy = sinf(angle_xy*DEG_TO_RAD);
    float cos_zw = cosf(angle_zw*DEG_TO_RAD), sin_zw = sinf(angle_zw*DEG_TO_RAD);
    float cos_xw = cosf(angle_xw*DEG_TO_RAD), sin_xw = sinf(angle_xw*DEG_TO_RAD);
    Vec3 projected_3d[16]; int projected_2d[16][2];
    float tesseract_scale = lcd.height() / 4.0f; float distance_4d = 2.5f;
    for (int i = 0; i < 16; ++i) {
        float x0=tesseract_points[i].x, y0=tesseract_points[i].y, z0=tesseract_points[i].z, w0=tesseract_points[i].w;
        float y1=cos_yz*y0-sin_yz*z0, z1=sin_yz*y0+cos_yz*z0; // YZ rotation
        float x2=cos_xy*x0-sin_xy*y1, y2=sin_xy*x0+cos_xy*y1; // XY rotation
        float z3=cos_zw*z1-sin_zw*w0, w3=sin_zw*z1+cos_zw*w0; // ZW rotation
        float x4=cos_xw*x2-sin_xw*w3, w4=sin_xw*x2+cos_xw*w3; // XW rotation
        Vec4 rotated = {x4, y2, z3, w4};
        float persp_4d = distance_4d / (distance_4d - rotated.w); // 4D->3D Projection
        projected_3d[i] = {rotated.x*persp_4d*tesseract_scale, rotated.y*persp_4d*tesseract_scale, rotated.z*persp_4d*tesseract_scale};
        float distance_3d = 400.f; // 3D->2D Projection
        float persp_3d = distance_3d / (distance_3d + projected_3d[i].z + tesseract_scale);
        projected_2d[i][0] = center_x + (int)(projected_3d[i].x * persp_3d);
        projected_2d[i][1] = center_y + (int)(projected_3d[i].y * persp_3d);
    }
    float tess_hue = fmod(elapsed_sec * 50.f, 360.f); // Color cycling
    // 既存のhsvToRgb565関数を使用してHSVからRGB565に変換
    uint32_t tess_color = hsvToRgb565(tess_hue, 1.0f, 1.0f);
    
    // テッセラクトをより目立たせる
    for (int i = 0; i < 32; ++i) { // Draw edges
        int p1=tesseract_edges[i][0], p2=tesseract_edges[i][1];
        
        // 線を太くして目立たせる（2ピクセル幅）
        lcd.drawLine(projected_2d[p1][0], projected_2d[p1][1], projected_2d[p2][0], projected_2d[p2][1], tess_color);
        
        // 線の周りに同じ色でぼかし効果を追加
        int dx = projected_2d[p2][0] - projected_2d[p1][0];
        int dy = projected_2d[p2][1] - projected_2d[p1][1];
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 0) {
            // 横方向にずらした線
            float nx = -dy / len;
            float ny = dx / len;
            lcd.drawLine(
                projected_2d[p1][0] + nx, projected_2d[p1][1] + ny, 
                projected_2d[p2][0] + nx, projected_2d[p2][1] + ny, 
                tess_color
            );
        }
    }
    
    // 頂点を強調（光る点で表現）
    for (int i = 0; i < 16; ++i) {
        // 中心点を明るく描画
        lcd.fillCircle(projected_2d[i][0], projected_2d[i][1], 2, tess_color);
        
        // グロー効果（より暗い色で外側の円を描画）
        uint16_t glow_color = hsvToRgb565(tess_hue, 0.5f, 0.7f);
        lcd.drawCircle(projected_2d[i][0], projected_2d[i][1], 3, glow_color);
    }

    // 4. 前景: スクロールテキスト (上下動)
    const char* scroll_text=" +++ HYPER DEMO SCENE on ESP32-S3 +++ 4D TESSERACT +++ LOVYANGFX ROCKS +++ ";
    lcd.setTextSize(2);
    static int text_width = 0; if(text_width==0) text_width=lcd.textWidth(scroll_text);
    int font_height = lcd.fontHeight();
    scroll_pos -= frame_delta_sec * 220.f; if (scroll_pos < -text_width) scroll_pos += text_width;
    int text_base_y = lcd.height() - 25;
    int text_y = text_base_y + (int)(8.f * sinf(elapsed_sec * 5.f));
    int clip_y = text_base_y - font_height/2 - 10; int clip_h = lcd.height() - clip_y;
    lcd.setClipRect(0, clip_y, lcd.width(), clip_h); // Clipping
    lcd.fillRect(0, clip_y, lcd.width(), clip_h, TFT_BLACK); // Clear scroll area
    uint8_t text_bright = 180+(uint8_t)(75.f*sinf(elapsed_sec*2.f));
    lcd.setTextColor(lcd.color888(text_bright, text_bright, text_bright));
    int current_x = (int)scroll_pos;
    while(current_x < lcd.width()) { lcd.drawString(scroll_text, current_x, text_y); current_x += text_width; }
    lcd.clearClipRect(); // Remove clipping

    // 5. タイトルテキスト
    float title_scale = 1.f + 0.1f * sinf(elapsed_sec * 3.f);
    lcd.setTextSize(2 * title_scale); lcd.setTextColor(TFT_MAGENTA);
    
    // 非推奨のdrawCentreStringを避ける
    int demo_text_y = 10 + (int)(3.f * cosf(elapsed_sec * 4.f));
    // テキストの幅を計算して中央揃えに
    int demo_text_width = lcd.textWidth("HYPER DEMO!");
    int demo_text_x = lcd.width() / 2 - (demo_text_width / 2);
    lcd.drawString("HYPER DEMO!", demo_text_x, demo_text_y);

    // 6. 画面フラッシュ効果
    static uint32_t last_flash_time = 0; static bool flashing = false;
    if (!flashing && current_time - last_flash_time > 5000 && random(100) < 5) {
        flashing = true; last_flash_time = current_time;
        lcd.fillScreen(TFT_WHITE); // Flash white
    } else if (flashing && current_time - last_flash_time > 50) {
        flashing = false; // Stop flashing after 50ms (next frame will redraw bg)
    }

    // 描画バッファをLCDに転送
    lcd.endWrite();
}

// --- ESP-IDF メインエントリーポイント ---
extern "C" void app_main(void)
{
    // 初期化処理
    setup();

    // 無限ループで描画を続ける
    while (1)
    {
        loop(); // メインの描画処理を実行

        // 他のタスクへCPU時間を譲る (1ms待機)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
