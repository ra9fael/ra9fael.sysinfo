#include "glass_item.h"

#include <QQuickItem>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QDebug>

GlassImageItem::GlassImageItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void GlassImageItem::setImage(const QImage& image)
{
    if (m_image.cacheKey() == image.cacheKey())
        return;
    m_image = image;
    m_dirty = true;
    emit imageChanged();
    update();
}

QSGNode* GlassImageItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData*)
{
    if (m_image.isNull()) {
        // 还没有玻璃图：不挂节点，图到了再 update()。
        delete node;
        m_dirty = false;
        return nullptr;
    }
    auto* textureNode = static_cast<QSGSimpleTextureNode*>(node);
    if (!textureNode)
        textureNode = new QSGSimpleTextureNode();
    textureNode->setRect(boundingRect());
    textureNode->setFiltering(QSGTexture::Linear);
    if (m_dirty || !textureNode->texture()) {
        QSGTexture* texture = window()->createTextureFromImage(m_image, QQuickWindow::TextureHasAlphaChannel);
        if (texture) {
            textureNode->setTexture(texture);
            textureNode->setOwnsTexture(true);
        }
        m_dirty = false;
    }
    return textureNode;
}
