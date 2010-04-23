#include "displaycell.h"
#include <QtGui>

DisplayCell::DisplayCell(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_tooltip(new ToolTip("My Tooltip"))
{
    setAcceptHoverEvents(true);
    //m_tooltip->setZValue(1);
    m_tooltip->hide();
}

DisplayCell::~DisplayCell() {
    if (m_tooltip)
        m_tooltip->deleteLater();
}

QRectF DisplayCell::boundingRect() const {
    return QRectF(0, 0, box_w, box_h);
}

void DisplayCell::paint(QPainter *p, const QStyleOptionGraphicsItem *opt,
                        QWidget *widget) {
    p->save();
    p->setPen(QPen(Qt::black, 0.5));
    p->setBrush(Qt::gray);
    p->drawRect(boundingRect().adjusted(0.5f, 0.5f, -0.5f, -0.5f));
    p->restore();
}

void DisplayCell::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    //prepareGeometryChange();
    //qDebug() << "Hover";
    //m_tooltip->setPos(mapFromScene(event->pos()));
    //m_tooltip->setPos(boundingRect().bottomRight() + QPoint(4, 4));
    //m_tooltip->setPos(mapToScene(boundingRect().bottomRight()));
    m_tooltip->setPos(0, 0);
    m_tooltip->show();
    scene()->update(m_tooltip->boundingRect());
}

void DisplayCell::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    //qDebug() << "Leave";
    prepareGeometryChange();
    m_tooltip->hide();
}

void DisplayCell::mousePressEvent(QGraphicsSceneMouseEvent *event) {

}

void DisplayCell::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {

}

void DisplayCell::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

}
