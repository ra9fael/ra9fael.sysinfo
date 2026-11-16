// 液态玻璃材质：移植自 TaskbarPopup（src/glass.cpp），去掉 GDI+ 依赖。
//
// 流程：截下窗口后面的屏幕 → 缩小一半、模糊、提高饱和度 → 用圆角矩形的有向距离场
// 算出每个像素离边缘多远，近边缘的一圈像凸透镜把背景往里折射（RGB 略有色散），
// 再叠着色、顶部光泽、边缘高光和外阴影。所有像素是预乘 alpha 的 BGRA，
// 可以直接交给 Qt Quick 当纹理，窗口本身透明。
//
// 和面板形状有关的量在 prepare() 里一次算好并缓存；每帧只需要按位移取背景色，
// 所以背景不动时刷新很便宜。
#pragma once

#include <cmath>

#include <cstdint>
#include <vector>

#include <windows.h>

namespace sysinfo {

enum class GlassStyle { Liquid, Win11 };

// Signed distance to a rounded rectangle with outer half-extents hx/hy.
// Coordinates are relative to its center; nx/ny is the outward unit normal.
inline float roundRectDistance(float x, float y, float hx, float hy, float r, float& nx, float& ny)
{
    const float sx = x < 0 ? -1.0f : 1.0f, sy = y < 0 ? -1.0f : 1.0f;
    const float qx = std::fabs(x) - hx + r, qy = std::fabs(y) - hy + r;
    if (qx > 0 && qy > 0) {
        const float len = std::sqrt(qx * qx + qy * qy);
        nx = sx * qx / len;
        ny = sy * qy / len;
        return len - r;
    }
    if (qx > qy) {
        nx = sx;
        ny = 0;
        return qx - r;
    }
    nx = 0;
    ny = sy;
    return qy - r;
}

inline float roundRectDistance(float x, float y, float hx, float hy, float r)
{
    float nx, ny;
    return roundRectDistance(x, y, hx, hy, r, nx, ny);
}

struct GlassRect {
    float x = 0, y = 0, w = 0, h = 0;
};

// 一次截图的结果（截图范围比窗口大一圈，模糊和边缘取样不碰边界）。
struct GlassShot {
    RECT area{};
    int w = 0, h = 0;
    std::vector<uint32_t> raw;  // 原图，用来判断背景有没有变
    std::vector<uint32_t> blur; // 模糊并提高饱和度后的背景
};

class GlassEngine {
public:
    // 截 window 四周再多一圈的屏幕，模糊并提高饱和度。same 不为空时先比对原图：
    // 完全一样返回 false（不重算）。线程安全，不碰成员。
    static bool shoot(const RECT& window, float scale, GlassStyle style, GlassShot& out,
                      const GlassShot* same = nullptr);

    // 截下来的背景交给这个引擎（换背景不改面板形状，prepare 的缓存仍有效）。
    void adopt(GlassShot&& shot, float scale);

    void setLight(bool light) { m_light = light; }
    bool light() const { return m_light; }
    void setStyle(GlassStyle style) { m_style = style; }
    GlassStyle style() const { return m_style; }
    // 液态玻璃的透明度 0–100（默认 60），越大越透。
    void setTransparency(int percent) { m_transparency = percent; }
    void setBlur(int percent) { m_blurAmount = percent; }
    void setShadow(int percent) { m_shadow = percent; }
    // 自己选的底色（r,g,b，0–255）；不调用就用默认的深/浅两套中性色。
    void setTint(int r, int g, int b);

    // 面板形状：窗口像素尺寸、面板在窗口里的位置（阴影外圈之外）、圆角半径（像素）。
    void prepare(int width, int height, const GlassRect& panel, float radiusPx);
    // 把整窗画进 pixels（width×height，预乘 BGRA）；origin = 窗口左上角的屏幕坐标。
    void render(uint32_t* pixels, POINT origin) const;

    const RECT& area() const { return m_area; }
    const std::vector<uint32_t>& raw() const { return m_raw; }
    bool empty() const { return m_blur.empty(); }

private:
    struct Texel {
        uint8_t cover = 0;
        uint8_t light = 0;
        uint8_t shadow = 0;
        bool bevel = false;
        uint8_t gloss = 0;
        float dx = 0, dy = 0;
    };

    struct Tint {
        float rgb[3] = {16, 16, 20};
        float luma = 16;
        float alpha = 0.38f;
        float guard = 1;
        float bound = 76;
    };

    Tint tintNow() const;
    uint32_t fetch(int x, int y) const;
    void sampleBevel(float x, float y, const Texel& t, float rgb[3]) const;
    void renderRows(uint32_t* pixels, POINT origin, int rowBegin, int rowEnd) const;

    RECT m_area{};
    int m_w = 0, m_h = 0;
    std::vector<uint32_t> m_raw;
    std::vector<uint32_t> m_blur;
    float m_scale = 1;

    int m_texW = 0, m_texH = 0;
    float m_texRadius = -1;
    float m_texScale = 1;
    GlassRect m_texPanel{};
    GlassStyle m_texStyle = GlassStyle::Liquid;
    std::vector<Texel> m_texels;

    GlassStyle m_style = GlassStyle::Liquid;
    bool m_light = true;
    bool m_customTint = false;
    int m_tint[3] = {0, 0, 0};
    int m_transparency = 60;
    int m_blurAmount = 55;
    int m_shadow = 0;
};

} // namespace sysinfo
