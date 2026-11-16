// 一块整窗大小的纹理：放着玻璃引擎烘出来的那张图（含阴影和圆角外的透明）。
#pragma once

#include <QImage>
#include <QQuickItem>

class GlassImageItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
public:
    explicit GlassImageItem(QQuickItem* parent = nullptr);

    QImage image() const { return m_image; }
    void setImage(const QImage& image);

signals:
    void imageChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;

private:
    QImage m_image;
    bool m_dirty = false;
};
