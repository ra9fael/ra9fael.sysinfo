#include "native_overlay.h"
#include <liquid_glass/window.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <algorithm>
#include <unordered_map>

namespace sysinfo {
namespace {
template<class T> void release(T*& p) { if (p) { p->Release(); p = nullptr; } }
liquid_glass::Edge edge(const QString& s) {
    if (s == QStringLiteral("left")) return liquid_glass::Edge::Left;
    if (s == QStringLiteral("top")) return liquid_glass::Edge::Top;
    if (s == QStringLiteral("bottom")) return liquid_glass::Edge::Bottom;
    return liquid_glass::Edge::Right;
}
QImage image(const liquid_glass::Image& v) {
    if (v.empty()) return {};
    return QImage(v.pixels.data(), v.width, v.height, v.width * 4,
                  QImage::Format_ARGB32_Premultiplied).copy();
}
}

struct NativeOverlay::Impl {
    liquid_glass::Window window;
    NativeOverlayModel model;
    IDWriteFactory* factory = nullptr;
    std::unordered_map<std::wstring, float> rateRanges;
    Impl() { DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&factory)); }
    ~Impl() { release(factory); }
    void paint(const liquid_glass::ContentFrame& frame) {
        if (!factory) return;
        auto* dc = frame.context;
        const float scale = std::max(.1f, model.scale);
        dc->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale));
        const float width = frame.width / scale, height = frame.height / scale;
        const float ink = frame.backdropLuminance > .56f ? .06f : .97f;
        ID2D1SolidColorBrush* text = nullptr;
        ID2D1SolidColorBrush* wave = nullptr;
        dc->CreateSolidColorBrush(D2D1::ColorF(ink, ink, ink, 1), &text);
        dc->CreateSolidColorBrush(D2D1::ColorF(.10f, .62f, .80f, .90f), &wave);
        IDWriteTextFormat* format = nullptr;
        const auto family = (model.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : model.fontFamily).toStdWString();
        factory->CreateTextFormat(family.c_str(), nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            (model.tiny ? 13.f : 14.f) * model.fontPercent / 100.f, L"", &format);
        if (!format || !text || !wave) { release(format); release(text); release(wave); dc->SetTransform(D2D1::Matrix3x2F::Identity()); return; }
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        auto drawText = [&](const QString& value, float x, float y, float w, float h) {
            dc->DrawTextW(reinterpret_cast<const wchar_t*>(value.utf16()), static_cast<UINT32>(value.size()), format,
                D2D1::RectF(x,y,x+w,y+h), text, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        };
        const float pad = model.tiny ? 10.f : model.padding;
        const float cellW = model.cellWidth;
        const float cellH = model.cellHeight;
        float tinyX = pad;
        for (const auto& metric : model.metrics) {
            if (metric.empty) continue;
            QString value = model.showValues ? metric.value : QString();
            if (model.showUnits && !metric.unit.isEmpty() && (model.showValues || model.tiny)) value += QStringLiteral(" ") + metric.unit;
            if (model.tiny) {
                const QString line = (model.showLabels ? metric.label + QStringLiteral(" ") : QString()) + value;
                const float available = std::max(1.f,width-pad-tinyX);
                drawText(line,tinyX,std::max(0.f,height*.5f-11),available,24);
                IDWriteTextLayout* layout = nullptr;
                if (SUCCEEDED(factory->CreateTextLayout(reinterpret_cast<const wchar_t*>(line.utf16()), static_cast<UINT32>(line.size()), format, available,24,&layout))) {
                    DWRITE_TEXT_METRICS metrics{}; layout->GetMetrics(&metrics);
                    tinyX += metrics.widthIncludingTrailingWhitespace+15; release(layout);
                }
                if (tinyX >= width-pad) break;
                continue;
            }
            const float x=pad+metric.column*cellW,y=pad+metric.row*cellH;
            const float chartX = model.wash ? x : x + cellW*.56f;
            const float chartY = model.wash ? y+7 : y+cellH*.48f;
            const float chartW = model.wash ? cellW-8 : cellW*.44f-8;
            const float chartH = std::max(8.f, y+cellH-7-chartY);
            if (model.sparklines && metric.history.size()>1) {
                double ceiling=100;
                if (metric.rate) {
                    double peak=1;
                    for (double v:metric.history) peak=std::max(peak,v*1.15);
                    float& range=rateRanges[metric.label.toStdWString()];
                    range=std::max(static_cast<float>(peak),range*.985f); ceiling=range;
                }
                ID2D1Factory* d2dFactory=nullptr; dc->GetFactory(&d2dFactory);
                ID2D1PathGeometry* path=nullptr;
                if (SUCCEEDED(d2dFactory->CreatePathGeometry(&path))) {
                    ID2D1GeometrySink* sink=nullptr;
                    if (SUCCEEDED(path->Open(&sink))) {
                        sink->BeginFigure(D2D1::Point2F(chartX,chartY+chartH),D2D1_FIGURE_BEGIN_FILLED);
                        for (size_t i=0;i<metric.history.size();++i) {
                            const float px=chartX+chartW*static_cast<float>(i)/static_cast<float>(metric.history.size()-1);
                            const float py=chartY+chartH*(1-static_cast<float>(std::clamp(metric.history[i]/ceiling,0.,1.)));
                            sink->AddLine(D2D1::Point2F(px,py));
                        }
                        sink->AddLine(D2D1::Point2F(chartX+chartW,chartY+chartH));
                        sink->EndFigure(D2D1_FIGURE_END_CLOSED); sink->Close(); release(sink);
                        wave->SetOpacity(.32f); dc->FillGeometry(path,wave);
                        wave->SetOpacity(1); dc->DrawGeometry(path,wave,1.2f);
                    }
                    release(path);
                }
                release(d2dFactory);
            }
            const float lineHeight=(model.tiny?13.f:14.f)*model.fontPercent/100.f*1.35f;
            const bool stacked=cellH>=lineHeight*2+4;
            if (model.showLabels) drawText(metric.label,x,y,cellW-8,lineHeight);
            if (model.showValues) {
                const float valueX=model.showLabels&&!stacked?x+cellW*.5f:x;
                const float valueY=model.showLabels&&stacked?y+lineHeight+3:y;
                const float valueW=model.showLabels&&!stacked?cellW*.5f-8:cellW-8;
                drawText(value,valueX,valueY,valueW,lineHeight);
            }
        }
        release(format); release(text); release(wave);
        dc->SetTransform(D2D1::Matrix3x2F::Identity());
    }
};

NativeOverlay::NativeOverlay():m(std::make_unique<Impl>()) {
    m->window.moved=[this](liquid_glass::Point p){if(moved)moved(QPoint(p.x,p.y));};
    m->window.edgeCollapsedChanged=[this](bool v){if(edgeCollapsedChanged)edgeCollapsedChanged(v);};
    m->window.paintContent=[this](const liquid_glass::ContentFrame& f){m->paint(f);};
}
NativeOverlay::~NativeOverlay()=default;
bool NativeOverlay::create(){return m->window.create();}
void NativeOverlay::destroy(){m->window.destroy();}
void NativeOverlay::show(const QRect& r){m->window.show({r.x(),r.y(),r.width(),r.height()});}
void NativeOverlay::hide(){m->window.hide();}
bool NativeOverlay::visible() const{return m->window.visible();}
HWND NativeOverlay::hwnd() const{return m->window.hwnd();}
QRect NativeOverlay::geometry() const{auto r=m->window.geometry();return {r.x,r.y,r.width,r.height};}
void NativeOverlay::setGeometry(const QRect& r){m->window.setGeometry({r.x(),r.y(),r.width(),r.height()});}
void NativeOverlay::setClickThrough(bool v){m->window.setClickThrough(v);}
void NativeOverlay::setCaptureVisible(bool v){m->window.setCaptureVisible(v);}
void NativeOverlay::setLocked(bool v){m->window.setLocked(v);}
void NativeOverlay::setModel(NativeOverlayModel model){
    m->model=std::move(model); const auto& v=m->model;
    liquid_glass::Material material;
    material.dark=v.dark; material.edgeHide=v.edgeHide; material.debugPattern=v.debugPattern;
    material.animations=v.animations;
    material.radius=v.radius; material.blur=v.blur; material.glass=v.glass; material.refraction=v.refraction;
    material.dispersion=v.dispersion; material.rimLight=v.rimLight; material.surfaceMotion=v.surfaceMotion; material.scale=v.scale;
    m->window.setMaterial(material);
}
void NativeOverlay::setEdgeCollapsed(bool v,const QString& side){m->window.setEdgeCollapsed(v,edge(side));}
bool NativeOverlay::edgeCollapsed() const{return m->window.edgeCollapsed();}
void NativeOverlay::render(){m->window.render();}
QImage NativeOverlay::snapshot() const{return image(m->window.snapshot());}
QImage NativeOverlay::backdropSnapshot() const{return image(m->window.backdropSnapshot());}
QImage NativeOverlay::capsuleSnapshot() const{return image(m->window.capsuleSnapshot());}
}
