#include "glass_engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <thread>

namespace sysinfo {
namespace {

inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

// 横向盒式模糊一行，越界取边缘像素。
void boxBlurRow(const uint32_t* src, uint32_t* dst, int n, int r)
{
    const int window = 2 * r + 1;
    const int inv = (65536 + window / 2) / window; // 除法换成乘法（四舍五入）
    int sb = 0, sg = 0, sr = 0;
    for (int i = -r; i <= r; ++i) {
        const uint32_t p = src[std::clamp(i, 0, n - 1)];
        sb += p & 0xFF;
        sg += (p >> 8) & 0xFF;
        sr += (p >> 16) & 0xFF;
    }
    for (int i = 0; i < n; ++i) {
        dst[i] = 0xFF000000 | (((sr * inv + 0x8000) >> 16) << 16) | (((sg * inv + 0x8000) >> 16) << 8) |
                 ((sb * inv + 0x8000) >> 16);
        const uint32_t add = src[std::min(i + r + 1, n - 1)], sub = src[std::max(i - r, 0)];
        sb += static_cast<int>(add & 0xFF) - static_cast<int>(sub & 0xFF);
        sg += static_cast<int>((add >> 8) & 0xFF) - static_cast<int>((sub >> 8) & 0xFF);
        sr += static_cast<int>((add >> 16) & 0xFF) - static_cast<int>((sub >> 16) & 0xFF);
    }
}

// 纵向盒式模糊整张图：每列一组累加和，一行一行往下推。
void boxBlurColumns(const uint32_t* src, uint32_t* dst, int w, int h, int r, std::vector<int>& sums)
{
    const int window = 2 * r + 1;
    const int inv = (65536 + window / 2) / window;
    sums.assign(static_cast<size_t>(w) * 3, 0);
    int* s = sums.data();
    auto row = [&](int y) { return src + static_cast<size_t>(std::clamp(y, 0, h - 1)) * w; };
    for (int i = -r; i <= r; ++i) {
        const uint32_t* p = row(i);
        for (int x = 0; x < w; ++x) {
            s[3 * x] += p[x] & 0xFF;
            s[3 * x + 1] += (p[x] >> 8) & 0xFF;
            s[3 * x + 2] += (p[x] >> 16) & 0xFF;
        }
    }
    for (int y = 0; y < h; ++y) {
        uint32_t* out = dst + static_cast<size_t>(y) * w;
        const uint32_t* add = row(y + r + 1);
        const uint32_t* sub = row(y - r);
        for (int x = 0; x < w; ++x) {
            int* c = s + 3 * x;
            out[x] = 0xFF000000 | (((c[2] * inv + 0x8000) >> 16) << 16) | (((c[1] * inv + 0x8000) >> 16) << 8) |
                     ((c[0] * inv + 0x8000) >> 16);
            c[0] += static_cast<int>(add[x] & 0xFF) - static_cast<int>(sub[x] & 0xFF);
            c[1] += static_cast<int>((add[x] >> 8) & 0xFF) - static_cast<int>((sub[x] >> 8) & 0xFF);
            c[2] += static_cast<int>((add[x] >> 16) & 0xFF) - static_cast<int>((sub[x] >> 16) & 0xFF);
        }
    }
}

// 横竖各做三遍盒式模糊，效果接近高斯模糊。
void blur(std::vector<uint32_t>& px, int w, int h, int r)
{
    std::vector<uint32_t> tmp(px.size());
    std::vector<int> sums;
    for (int pass = 0; pass < 3; ++pass) {
        for (int y = 0; y < h; ++y)
            boxBlurRow(&px[static_cast<size_t>(y) * w], &tmp[static_cast<size_t>(y) * w], w, r);
        boxBlurColumns(tmp.data(), px.data(), w, h, r, sums);
    }
}

// 缩小一半：每 2×2 个像素取平均，奇数边上的取边缘像素。
std::vector<uint32_t> halfSize(const std::vector<uint32_t>& px, int w, int h, int hw, int hh)
{
    std::vector<uint32_t> out(static_cast<size_t>(hw) * hh);
    for (int y = 0; y < hh; ++y) {
        const uint32_t* r0 = &px[static_cast<size_t>(2 * y) * w];
        const uint32_t* r1 = &px[static_cast<size_t>(std::min(2 * y + 1, h - 1)) * w];
        uint32_t* o = &out[static_cast<size_t>(y) * hw];
        for (int x = 0; x < hw; ++x) {
            const int x0 = 2 * x, x1 = std::min(2 * x + 1, w - 1);
            const uint32_t a = r0[x0], b = r0[x1], c = r1[x0], d = r1[x1];
            const uint32_t lo = ((a & 0xFF00FF) + (b & 0xFF00FF) + (c & 0xFF00FF) + (d & 0xFF00FF) + 0x020002) >> 2;
            const uint32_t mid = ((a & 0xFF00) + (b & 0xFF00) + (c & 0xFF00) + (d & 0xFF00) + 0x200) >> 2;
            o[x] = 0xFF000000 | (lo & 0xFF00FF) | (mid & 0xFF00);
        }
    }
    return out;
}

// 放大回 w×h：双线性插值，半尺寸图的像素中心在原图的 2x+0.5 处。
void doubleSize(const std::vector<uint32_t>& half, int hw, int hh, std::vector<uint32_t>& out, int w, int h)
{
    out.resize(static_cast<size_t>(w) * h);
    auto taps = [](int n, int hn, std::vector<int>& i0, std::vector<int>& i1) {
        i0.resize(n);
        i1.resize(n);
        for (int i = 0; i < n; ++i) {
            const int k = std::min(i / 2, hn - 1);
            i1[i] = k;
            i0[i] = i % 2 == 0 ? std::max(k - 1, 0) : std::min(k + 1, hn - 1);
        }
    };
    std::vector<int> x0, x1, y0, y1;
    taps(w, hw, x0, x1);
    taps(h, hh, y0, y1);
    for (int y = 0; y < h; ++y) {
        const uint32_t* ra = &half[static_cast<size_t>(y0[y]) * hw];
        const uint32_t* rb = &half[static_cast<size_t>(y1[y]) * hw];
        uint32_t* o = &out[static_cast<size_t>(y) * w];
        for (int x = 0; x < w; ++x) {
            const uint32_t p00 = rb[x1[x]], p01 = rb[x0[x]], p10 = ra[x1[x]], p11 = ra[x0[x]];
            const uint32_t lo =
                (9 * (p00 & 0xFF00FF) + 3 * (p01 & 0xFF00FF) + 3 * (p10 & 0xFF00FF) + (p11 & 0xFF00FF) + 0x080008) >> 4;
            const uint32_t mid = (9 * (p00 & 0xFF00) + 3 * (p01 & 0xFF00) + 3 * (p10 & 0xFF00) + (p11 & 0xFF00) + 0x800) >> 4;
            o[x] = 0xFF000000 | (lo & 0xFF00FF) | (mid & 0xFF00);
        }
    }
}

// 提高饱和度，背景透过玻璃显得更鲜亮。
void saturate(std::vector<uint32_t>& px, float amount)
{
    const int k = static_cast<int>(std::lround(amount * 256));
    for (uint32_t& p : px) {
        const int b = p & 0xFF, g = (p >> 8) & 0xFF, r = (p >> 16) & 0xFF;
        const int l = (77 * r + 150 * g + 29 * b) >> 8;
        auto adj = [&](int c) { return static_cast<uint32_t>(std::clamp(l + (((c - l) * k) >> 8), 0, 255)); };
        p = 0xFF000000 | (adj(r) << 16) | (adj(g) << 8) | adj(b);
    }
}

// 在 w×h 的图上双线性取一个通道；x、y 是连续坐标（像素中心在 .5），越界取边缘。
float sampleChannel(const std::vector<uint32_t>& px, int w, int h, float x, float y, int shift)
{
    const float fx = std::clamp(x - 0.5f, 0.0f, static_cast<float>(w - 1));
    const float fy = std::clamp(y - 0.5f, 0.0f, static_cast<float>(h - 1));
    const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
    const int x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const float tx = fx - x0, ty = fy - y0;
    const uint32_t* r0 = &px[static_cast<size_t>(y0) * w];
    const uint32_t* r1 = &px[static_cast<size_t>(y1) * w];
    const float a = static_cast<float>((r0[x0] >> shift) & 0xFF), b = static_cast<float>((r0[x1] >> shift) & 0xFF);
    const float c = static_cast<float>((r1[x0] >> shift) & 0xFF), d = static_cast<float>((r1[x1] >> shift) & 0xFF);
    const float top = a + (b - a) * tx, bottom = c + (d - c) * tx;
    return top + (bottom - top) * ty;
}

// 截图的范围：多截一圈，模糊和边缘折射取样都不会碰到截图的边；不超出窗口所在的屏幕。
RECT shotArea(const RECT& window, float scale)
{
    const int pad = static_cast<int>(std::lround(28 * scale));
    RECT area = {window.left - pad, window.top - pad, window.right + pad, window.bottom + pad};
    MONITORINFO mi = {sizeof(mi)};
    if (GetMonitorInfoW(MonitorFromRect(&window, MONITOR_DEFAULTTONEAREST), &mi))
        IntersectRect(&area, &area, &mi.rcMonitor);
    return area;
}

// 原图 → 背景：缩小一半再模糊、提高饱和度，再放大回来（模糊得很厉害，看不出区别，快好几倍）。
void process(GlassShot& out, float scale, GlassStyle style)
{
    const int hw = (out.w + 1) / 2, hh = (out.h + 1) / 2;
    std::vector<uint32_t> half = halfSize(out.raw, out.w, out.h, hw, hh);
    const float radius = style == GlassStyle::Win11 ? 9.0f : 3.5f;
    blur(half, hw, hh, std::max(1, static_cast<int>(std::lround(radius * scale))));
    saturate(half, style == GlassStyle::Win11 ? 1.2f : 1.45f);
    doubleSize(half, hw, hh, out.blur, out.w, out.h);
}

// 抓屏：CAPTUREBLT 让分层窗口也截进来；BitBlt 不填 alpha，补成不透明。
bool captureScreen(const RECT& area, std::vector<uint32_t>& pixels)
{
    const int w = area.right - area.left, h = area.bottom - area.top;
    if (w <= 0 || h <= 0)
        return false;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool ok = false;
    if (dib) {
        HGDIOBJ old = SelectObject(mem, dib);
        ok = BitBlt(mem, 0, 0, w, h, screen, area.left, area.top, SRCCOPY | CAPTUREBLT) != FALSE;
        GdiFlush();
        if (ok) {
            const uint32_t* src = static_cast<const uint32_t*>(bits);
            pixels.assign(src, src + static_cast<size_t>(w) * h);
            for (uint32_t& p : pixels)
                p |= 0xFF000000;
        }
        SelectObject(mem, old);
        DeleteObject(dib);
    }
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    return ok;
}

} // namespace

bool GlassEngine::shoot(const RECT& window, float scale, GlassStyle style, GlassShot& out, const GlassShot* same)
{
    RECT area = shotArea(window, scale);
    out.area = area;
    out.w = std::max(0L, area.right - area.left);
    out.h = std::max(0L, area.bottom - area.top);
    out.raw.clear();
    out.blur.clear();
    if (out.w <= 0 || out.h <= 0) {
        out.w = out.h = 0;
        return true;
    }
    if (!captureScreen(area, out.raw)) {
        if (same)
            return false;
        out.raw.assign(static_cast<size_t>(out.w) * out.h, 0xFF202024);
    }
    if (same && EqualRect(&same->area, &area) && same->raw.size() == out.raw.size() &&
        std::memcmp(same->raw.data(), out.raw.data(), out.raw.size() * sizeof(uint32_t)) == 0)
        return false;
    process(out, scale, style);
    return true;
}

void GlassEngine::adopt(GlassShot&& shot, float scale)
{
    m_scale = scale;
    m_area = shot.area;
    m_w = shot.w;
    m_h = shot.h;
    m_raw.swap(shot.raw);
    m_blur.swap(shot.blur);
}

void GlassEngine::setTint(int r, int g, int b)
{
    m_customTint = true;
    m_tint[0] = std::clamp(r, 0, 255);
    m_tint[1] = std::clamp(g, 0, 255);
    m_tint[2] = std::clamp(b, 0, 255);
}

void GlassEngine::prepare(int width, int height, const GlassRect& panel, float radiusPx)
{
    if (!m_texels.empty() && width == m_texW && height == m_texH && radiusPx == m_texRadius && m_scale == m_texScale &&
        m_style == m_texStyle && panel.x == m_texPanel.x && panel.y == m_texPanel.y && panel.w == m_texPanel.w &&
        panel.h == m_texPanel.h)
        return;
    m_texScale = m_scale;
    m_texStyle = m_style;
    m_texW = std::max(0, width);
    m_texH = std::max(0, height);
    m_texPanel = panel;
    m_texRadius = radiusPx;
    m_texels.assign(static_cast<size_t>(m_texW) * m_texH, Texel{});

    const float s = m_scale;
    const float bevel = 18 * s;       // 边缘弯曲的宽度
    const float refraction = 14 * s;  // 最外缘的折射位移
    const float rimWidth = 1.3f * s;  // 高光细边的宽度
    const float border = std::max(1.0f, std::round(s)); // Win11 原版的边框宽度
    const float shadowSigma = 11 * s, shadowDy = 6 * s;
    const float lx = -0.5f, ly = -0.866f; // 光从左上方照过来

    const float cx = panel.x + panel.w / 2, cy = panel.y + panel.h / 2;
    const float hx = panel.w / 2, hy = panel.h / 2;
    const float shadowReach = 3 * shadowSigma + shadowDy;
    const float twoSigma2 = 2 * shadowSigma * shadowSigma;

    for (int y = 0; y < m_texH; ++y) {
        Texel* row = &m_texels[static_cast<size_t>(y) * m_texW];
        const float py = y + 0.5f;
        for (int x = 0; x < m_texW; ++x) {
            const float px = x + 0.5f;
            float nx, ny;
            const float d = roundRectDistance(px - cx, py - cy, hx, hy, radiusPx, nx, ny);
            const float cover = clamp01(0.5f - d);

            float shadow = 0;
            if (cover < 1) {
                const float ds = roundRectDistance(px - cx, py - cy - shadowDy, hx, hy, radiusPx);
                if (ds < shadowReach) {
                    const float q = std::max(0.0f, ds + 2 * s);
                    shadow = std::exp(-q * q / twoSigma2);
                    if (shadow < 0.008f)
                        shadow = 0;
                }
            }
            Texel& t = row[x];
            t.cover = static_cast<uint8_t>(std::lround(cover * 255));
            t.shadow = static_cast<uint8_t>(std::lround(shadow * 255));
            if (!t.cover)
                continue;

            const float depth = std::max(0.0f, -d);
            if (m_style == GlassStyle::Win11) { // 平的，最外面一圈细边框
                t.light = static_cast<uint8_t>(std::lround(clamp01(border + 0.5f - depth) * 255));
                continue;
            }
            if (depth < bevel) {
                // 沿法线往里取样：边缘像凸透镜一样把背景放大
                const float k = 1 - depth / bevel;
                const float disp = refraction * k * k;
                t.dx = -nx * disp;
                t.dy = -ny * disp;
                t.bevel = true;
            }

            // 高光：顶部一层光泽；朝光一侧的亮边、对侧的弱反光、斜面上的柔光
            const float v = clamp01((py - panel.y) / panel.h);
            const float ndl = nx * lx + ny * ly;
            const float facing = std::max(0.0f, ndl), away = std::max(0.0f, -ndl);
            const float spec = facing * facing * facing + 0.5f * away * away * away;
            const float rim = depth < 8 * rimWidth ? std::exp(-depth / rimWidth) : 0;
            const float edge = depth < bevel ? (1 - depth / bevel) * (1 - depth / bevel) : 0;
            const float light = clamp01(spec * (0.85f * rim + 0.25f * edge) + 0.16f * rim);
            t.light = static_cast<uint8_t>(std::lround(light * 255));
            t.gloss = static_cast<uint8_t>(std::lround((1 - v) * (1 - v) * 255));
        }
    }
}

uint32_t GlassEngine::fetch(int x, int y) const
{
    if (m_blur.empty())
        return 0xFF202024;
    x = std::clamp(x, 0, m_w - 1);
    y = std::clamp(y, 0, m_h - 1);
    const size_t index = static_cast<size_t>(y) * m_w + x;
    const uint32_t raw = m_raw[index];
    const uint32_t blurred = m_blur[index];
    const int amount = std::clamp(m_blurAmount, 0, 100);
    if (amount == 0)
        return raw;
    if (amount == 100)
        return blurred;
    const auto mix = [amount](uint32_t a, uint32_t b, int shift) {
        const int av = static_cast<int>((a >> shift) & 0xFF);
        const int bv = static_cast<int>((b >> shift) & 0xFF);
        return static_cast<uint32_t>(av + (bv - av) * amount / 100);
    };
    return 0xFF000000 | (mix(raw, blurred, 16) << 16) | (mix(raw, blurred, 8) << 8) | mix(raw, blurred, 0);
}

// 边缘一圈：三个通道按略有不同的位移各取一次，带出色散。
void GlassEngine::sampleBevel(float x, float y, const Texel& t, float rgb[3]) const
{
    if (m_blur.empty()) {
        rgb[0] = rgb[1] = rgb[2] = 32;
        return;
    }
    rgb[0] = sampleChannel(m_blur, m_w, m_h, x + t.dx * 0.9f, y + t.dy * 0.9f, 16);
    rgb[1] = sampleChannel(m_blur, m_w, m_h, x + t.dx, y + t.dy, 8);
    rgb[2] = sampleChannel(m_blur, m_w, m_h, x + t.dx * 1.1f, y + t.dy * 1.1f, 0);
}

void GlassEngine::render(uint32_t* pixels, POINT origin) const
{
    // 一行行互不相干，分几段交给几个线程一起画。
    const int h = m_texH;
    int threads = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, 4);
    if (static_cast<long long>(m_texW) * h < 200000)
        threads = 1;
    std::vector<std::thread> pool;
    for (int i = 1; i < threads; ++i)
        pool.emplace_back([=] { renderRows(pixels, origin, h * i / threads, h * (i + 1) / threads); });
    renderRows(pixels, origin, 0, h / threads);
    for (std::thread& t : pool)
        t.join();
}

GlassEngine::Tint GlassEngine::tintNow() const
{
    const bool win11 = m_style == GlassStyle::Win11;
    Tint t;
    // Win11 原版用系统亚克力的灰：深色 #202020，浅色 #F3F3F3，着色浓，背景只透出一点。
    t.luma = win11 ? (m_light ? 243.0f : 32.0f) : (m_light ? 252.0f : 16.0f);
    t.rgb[0] = t.rgb[1] = t.luma;
    t.rgb[2] = win11 ? t.luma : (m_light ? 255.0f : 20.0f);
    if (m_customTint) { // 自己选的底色（m_light 已经按它的深浅定了）
        t.rgb[0] = static_cast<float>(m_tint[0]);
        t.rgb[1] = static_cast<float>(m_tint[1]);
        t.rgb[2] = static_cast<float>(m_tint[2]);
        t.luma = 0.299f * t.rgb[0] + 0.587f * t.rgb[1] + 0.114f * t.rgb[2];
    }
    t.alpha = win11 ? (m_light ? 0.9f : 0.93f) : (m_light ? 0.36f : 0.38f);
    // 液态玻璃的透明度：默认（60%）时 k = 1；更透明时着色变淡，保底也跟着放松。
    t.guard = 1;
    if (m_style == GlassStyle::Liquid) {
        const float k = (100 - std::clamp(m_transparency, 0, 100)) / 40.0f;
        t.alpha = std::min(0.94f, t.alpha * k);
        t.guard = std::min(1.0f, k);
    }
    // 深浅和背景不搭时着色加浓，保证上面的文字看得清。Win11 原版要求更严。
    t.bound = win11 ? (m_light ? 0.82f * 255 : 0.2f * 255) : (m_light ? 0.72f * 255 : 0.30f * 255);
    if (m_customTint)
        t.bound = m_light ? std::min(t.bound, t.luma - 20) : std::max(t.bound, t.luma + 20);
    return t;
}

void GlassEngine::renderRows(uint32_t* pixels, POINT origin, int rowBegin, int rowEnd) const
{
    const int w = m_texW, h = m_texH;
    const bool win11 = m_style == GlassStyle::Win11;
    const Tint tn = tintNow();
    const float* tint = tn.rgb;
    const float tintL = tn.luma, tintAlpha = tn.alpha, guard = tn.guard, bound = tn.bound;
    const float shadowAlpha = (win11 ? (m_light ? 0.16f : 0.3f) : (m_light ? 0.22f : 0.32f))
                              * std::clamp(m_shadow, 0, 100) / 100.0f;
    // Win11 原版的边框：深色时叠白，浅色时叠黑。
    const float borderTo = m_light ? 0.0f : 255.0f, borderAlpha = (m_light ? 0.1f : 0.12f) / 255.0f;
    const float sheen = (m_light ? 0.12f : 0.07f) / 255.0f;
    // 窗口像素 → 背景截图里的坐标
    const int offX = origin.x - m_area.left, offY = origin.y - m_area.top;

    for (int y = rowBegin; y < rowEnd; ++y) {
        uint32_t* row = pixels + static_cast<size_t>(y) * w;
        if (y >= h) {
            std::fill(row, row + w, 0u);
            continue;
        }
        const Texel* tex = &m_texels[static_cast<size_t>(y) * w];
        for (int x = 0; x < w; ++x) {
            const Texel& t = tex[x];
            if (!t.cover) {
                row[x] = static_cast<uint32_t>(t.shadow * shadowAlpha + 0.5f) << 24;
                continue;
            }
            float c[3];
            if (t.bevel) {
                sampleBevel(static_cast<float>(x + offX) + 0.5f, static_cast<float>(y + offY) + 0.5f, t, c);
            } else {
                const uint32_t p = fetch(x + offX, y + offY);
                c[0] = static_cast<float>((p >> 16) & 0xFF);
                c[1] = static_cast<float>((p >> 8) & 0xFF);
                c[2] = static_cast<float>(p & 0xFF);
            }
            const float lum = 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2];
            float ta = tintAlpha;
            if (m_light ? lum < bound : lum > bound)
                ta = std::min(0.94f, std::max(ta, (bound - lum) / (tintL - lum) * guard));
            const float light = std::min(1.0f, t.light / 255.0f + t.gloss * sheen);
            const float cover = t.cover / 255.0f;
            const float a = cover + t.shadow * shadowAlpha / 255.0f * (1 - cover);
            uint32_t out = static_cast<uint32_t>(a * 255 + 0.5f) << 24;
            float grain = 0;
            if (win11) { // 亚克力上一点点颗粒
                unsigned n = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u;
                n = (n ^ (n >> 13)) * 1274126177u;
                grain = static_cast<float>(static_cast<int>((n ^ (n >> 16)) & 255u) - 128) * (2.5f / 128);
            }
            for (int k = 0; k < 3; ++k) {
                float col = c[k] + (tint[k] - c[k]) * ta;
                col += (255 - col) * light;
                if (win11)
                    col += (borderTo - col) * (t.light * borderAlpha) + grain;
                out |= static_cast<uint32_t>(std::clamp(col * cover, 0.0f, 255.0f) + 0.5f) << (16 - 8 * k);
            }
            row[x] = out;
        }
    }
}

} // namespace sysinfo
