#include "sprite.h"

#include <QPropertyAnimation>
#include <QPainter>

void EffectAnimation::emphasize(QGraphicsItem *const item)
{
    //void QGraphicsItem::setGraphicsEffect(QGraphicsEffect * effect)
    //Sets effect as the item's effect. If there already is an effect installed on this item,
    //QGraphicsItem will delete the existing effect before installing the new effect.
    //If effect is the installed on a different item, setGraphicsEffect() will remove the effect from the item
    //and install it on this item.
    //QGraphicsItem takes ownership of effect.
    //由上述Qt的帮助信息可知，即使上一次的动画没播放完毕，
    //我们也不需要显式的删除原来的动画特效对象，Qt会自已删除它的。
    //因此可以直接new一个新的动画特效对象并设置进item中，而不会出现内存泄露
    EmphasizeEffect *const emphasize = new EmphasizeEffect(this);
    item->setGraphicsEffect(emphasize);
}

void EffectAnimation::effectOut(QGraphicsItem *const item)
{
    QAnimatedEffect *const effect = static_cast<QAnimatedEffect *const>(item->graphicsEffect());
    if (effect) {
        effect->setStay(false);
    }
}

void EffectAnimation::sendBack(QGraphicsItem *const item)
{
    SentbackEffect *const sendBack = new SentbackEffect(true, this);
    item->setGraphicsEffect(sendBack);
}

//EXTRA_EMPHASIZE_FACTOR的值越大，放大倍数也越大。
//但值太大的话，会有拖影的现象，所以为保险起见，
//不建议使用额外的放大因子，故本程序中设置为0
const int EXTRA_EMPHASIZE_FACTOR = 0;
const int INDEX_MAX_VALUE = 40;
const int DURATION_FACTOR = 5;

EmphasizeEffect::EmphasizeEffect(QObject *const parent)
    : QAnimatedEffect(parent)
{
    setObjectName("emphasizer");

    //Emphasize动画播放完毕后，并没有删除EmphasizeEffect对象本身，
    //而是交由EffectAnimation::effectOut函数重新获取到该对象，
    //在执行完动画回退效果后(参见AnimatedEffect::setStay函数)，再删除本对象。
    QPropertyAnimation *const anim = new QPropertyAnimation(this, "index", this);
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));

    anim->setEndValue(INDEX_MAX_VALUE);
    anim->setDuration(INDEX_MAX_VALUE * DURATION_FACTOR);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void EmphasizeEffect::draw(QPainter *painter)
{
    QSizeF srcRect = sourceBoundingRect().size();
    qreal srcRectWidth = srcRect.width();
    qreal srcRectHeight = srcRect.height();

    qreal scale = (-qAbs(index - 50) + 50) / 1000.0;
    scale = 0.1 - scale;

    QRectF target = boundingRect().adjusted(srcRectWidth * scale - EXTRA_EMPHASIZE_FACTOR,
        srcRectHeight * scale - EXTRA_EMPHASIZE_FACTOR, -srcRectWidth * scale, -srcRectHeight * scale);

    QRectF source(srcRectWidth * 0.1, srcRectHeight * 0.1,
        srcRectWidth, srcRectHeight);

    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates);

    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(target, pixmap, source);
}

QRectF EmphasizeEffect::boundingRectFor(const QRectF &sourceRect) const
{
    qreal scale = 0.1;
    QRectF rect(sourceRect);
    rect.adjust(-sourceRect.width() * scale,
                -sourceRect.height() * scale,
                sourceRect.width() * scale,
                sourceRect.height() * scale);
    return rect;
}

void QAnimatedEffect::setStay(bool stay)
{
    this->stay = stay;

    if (!stay) {
        QPropertyAnimation *const anim = new QPropertyAnimation(this, "index", this);
        anim->setEndValue(0);
        //要求子类在设置Duration时采用INDEX_MAX_VALUE * DURATION_FACTOR的形式，
        //这样可以确保在播放回退动画时的持续时间与之前播放其他特效动画的持续时间相当
        anim->setDuration(index * DURATION_FACTOR);

        //回退动画播放完毕后，会删除特效对象自己
        connect(anim, SIGNAL(finished()), this, SLOT(deleteLater()));
        connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

SentbackEffect::SentbackEffect(bool stay, QObject *parent)
    : QAnimatedEffect(parent)
{
    this->setObjectName("backsender");
    index = 0;
    this->stay = stay;

    QPropertyAnimation *anim = new QPropertyAnimation(this, "index", this);
    connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
    anim->setEndValue(40);
    anim->setDuration((40 - index) * 5);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QRectF SentbackEffect::boundingRectFor(const QRectF &sourceRect) const
{
    qreal scale = 0.05;
    QRectF rect(sourceRect);
    rect.adjust(-sourceRect.width() * scale,
                -sourceRect.height() * scale,
                sourceRect.width() * scale,
                sourceRect.height() * scale);
    return rect;
}

void SentbackEffect::draw(QPainter *painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset);
    QImage grayedImage(pixmap.size(), QImage::Format_ARGB32);

    QImage image = pixmap.toImage();
    int width = image.width();
    int height = image.height();
    int gray;

    QRgb col;

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            col = image.pixel(i, j);
            gray = qGray(col) >> 1;
            grayedImage.setPixel(i, j, qRgba(gray, gray, gray, qAlpha(col)));
        }
    }

    painter->drawPixmap(offset, pixmap);
    painter->setOpacity((40 - qAbs(index - 40)) / 80.0);
    painter->drawImage(offset, grayedImage);
}
