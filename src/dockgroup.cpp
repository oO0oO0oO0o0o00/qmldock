#include "dockgroup.h"

#include "debugrect.h"
#include "dockgroup_p.h"
#include "dockgroupresizehandler.h"
#include "docktabbar.h"
#include "dockwidget.h"

#include <QCursor>
#include <QDebug>
#include <QPainter>

DockGroupPrivate::DockGroupPrivate(DockGroup *parent)
    : q_ptr(parent), mousepRessed{false},
      area(Dock::Float), enableResizing{true}
      , tabBar{nullptr}, displayType{Dock::SplitView}
{
}

DockGroupResizeHandler *DockGroupPrivate::createHandlers()
{
    Q_Q(DockGroup);

    DockGroupResizeHandler *h{nullptr};
    switch (area) {
    case Dock::Left:
    case Dock::Right:
        h = new DockGroupResizeHandler(Qt::Horizontal, q);
        h->setX(0);
        h->setWidth(q->width());
        break;

    case Dock::Top:
    case Dock::Bottom:
        h = new DockGroupResizeHandler(Qt::Vertical, q);
        h->setY(0);
        h->setHeight(q->height());
        break;

    case Dock::Float:
    case Dock::Center:
        return nullptr;
    }
    if (!h)
        return nullptr;

    h->setIndex(handlers.count());
    QObject::connect(h,
                     &DockGroupResizeHandler::moving,
                     q,
                     &DockGroup::handler_moving);

    QObject::connect(h,
                     &DockGroupResizeHandler::moved,
                     q,
                     &DockGroup::handler_moved);
    return h;
}

bool DockGroupPrivate::isHorizontal() const
{
    return area == Dock::Top || area == Dock::Bottom;
}

bool DockGroupPrivate::isVertical() const
{
    return area == Dock::Right || area == Dock::Left;
}

bool DockGroupPrivate::acceptResizeEvent(const QPointF &point)
{
    Q_Q(DockGroup);

    if (!enableResizing)
        return false;

    switch (area) {
    case Dock::Right:
        return point.x() < Dock::resizeHandleSize;
        break;

    case Dock::Left:
        return point.x() > q->width() - Dock::resizeHandleSize;
        break;

    case Dock::Bottom:
        return point.y() < Dock::resizeHandleSize;

    case Dock::Top:
        return point.y() > q->height() - Dock::resizeHandleSize;

    case Dock::Float:
    case Dock::Center:
        return false;
    }
}

void DockGroupPrivate::fitItem(QQuickItem *item)
{
    Q_Q(DockGroup);

    switch (area) {
    case Dock::Right:
        item->setX(q->x() + (enableResizing ? Dock::resizeHandleSize : 1.));
        item->setWidth(q->width() - 2
                       - (enableResizing ? Dock::resizeHandleSize : 0.));
        break;

    case Dock::Left:
        item->setX(q->x() + 1);
        item->setWidth(q->width() - 2
                       - (enableResizing ? Dock::resizeHandleSize : 0.) - 2);
        break;

    case Dock::Top:
        item->setY(q->y() + 1);
        item->setHeight(q->height() - 2
                        - (enableResizing ? Dock::resizeHandleSize : 0.));
        break;
    case Dock::Bottom:
        item->setY(q->y() + (enableResizing ? Dock::resizeHandleSize : 1.));
        item->setHeight(q->height() - 2
                        - (enableResizing ? Dock::resizeHandleSize : 0.));
        break;

    case Dock::Center:
        item->setPosition(QPointF(q->x(), q->y() + 30));
        item->setSize(QSizeF(q->width(), q->height() - 30));
        break;

    case Dock::Float:
        break;
    }
}

void DockGroupPrivate::reorderItems()
{
    Q_Q(DockGroup);
    int ss;

    //    for (auto h : _handlers) {
    //        _dockWidgets.at(h->index())->setY(y() + ss);
    //        _dockWidgets.at(h->index())->setHeight(h->y() - ss);
    //        ss += h->y() + h->height();
    //    }
    //    _dockWidgets.last()->setY(y() + ss);
    //    _dockWidgets.last()->setHeight(height() - ss);

    qreal freeSize;

    if (isVertical()) {
        ss = q->y();
        freeSize = (q->height()
                    - (Dock::resizeHandleSize * (dockWidgets.count() - 1)));
    }

    if (isHorizontal()) {
        ss = q->x();
        freeSize = (q->width()
                    - (Dock::resizeHandleSize * (dockWidgets.count() - 1)));
    }
    QList<qreal> sl;
    for (int i = 0; i < dockWidgets.count(); i++) {
        auto d = dockWidgets.at(i);

        if (isVertical()) {
            d->setHeight(itemSizes.at(i) * freeSize);
            d->setY(ss);
            ss += d->height() + Dock::resizeHandleSize;
            sl.append(d->height());
        }
        if (isHorizontal()) {
            d->setWidth(itemSizes.at(i) * freeSize);
            d->setX(ss);
            ss += d->width() + Dock::resizeHandleSize;
            sl.append(d->width());
        }

        if (i < dockWidgets.count() - 1) {
            if (isVertical()) {
                fitItem(handlers.at(i));
                handlers.at(i)->setY(ss - q->y() - Dock::resizeHandleSize);
                qDebug() << "handler pos" << i << area << handlers.at(i)->y()<<d->height();
            }

            if (isHorizontal())
                handlers.at(i)->setX(ss - q->x() - Dock::resizeHandleSize);
        }
    }
}

void DockGroupPrivate::reorderHandles()
{
    Q_Q(DockGroup);

    int index{0};
    for (auto h : handlers) {
        h->setIndex(index++);
//        if (isVertical()) {
//            h->setY(itemSizes.at(h->index()) * q->height());
//            qDebug() << "handle pos" << h->y() << q->height();
//        }
//        if (isHorizontal())
//            h->setX(itemSizes.at(h->index()) * q->width());
    }
}

void DockGroupPrivate::normalizeItemSizes()
{
    qreal sum{0};
    qDebug() << "====";
    qDebug() << itemSizes;

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    itemSizes.resize(dockWidgets.count());
#else
    {
        auto d = dockWidgets.count() - itemSizes.count();
        if (d > 0)
            for (auto i = 0; i < d; i++)
                itemSizes.append(0);

        if (d < 0)
            for (auto i = 0; i < -d; i++)
                itemSizes.removeLast();
    }
#endif

    for (auto i = 0; i != itemSizes.count(); ++i) {
        if (qFuzzyCompare(0, itemSizes.at(i)))
            itemSizes[i] = 1. / itemSizes.count();
        sum += itemSizes.at(i);
    }
    qDebug() << itemSizes << "dockWidgets="<<dockWidgets.count();
    for (auto i = 0; i != itemSizes.count(); ++i) {
        if (i == itemSizes.count())
            itemSizes[i] = 1 - sum;
        else
            itemSizes[i] = itemSizes.at(i) / sum;
    }

    qDebug() << itemSizes << "sum="<<sum;
}

QRectF DockGroupPrivate::updateUsableArea()
{
    Q_Q(DockGroup);
    if (area == Dock::Center) {
        usableArea = {q->position(), q->size()};
    } else {
        usableArea = q->childrenRect();
    }
}

DockGroup::DockGroup(QQuickItem *parent)
    : QQuickPaintedItem(parent), d_ptr(new DockGroupPrivate(this))
{
    Q_D(DockGroup);
    d->area = Dock::Float;
    setClip(true);
    setAcceptHoverEvents(true);
    //    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    d->tabBar = new DockTabBar(this);
}

DockGroup::DockGroup(Dock::Area area, QQuickItem *parent)
    : QQuickPaintedItem(parent), d_ptr(new DockGroupPrivate(this))
{
    Q_D(DockGroup);
    d->area = area;

//    setClip(true);
    setAcceptHoverEvents(true);
    //    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    d->tabBar = new DockTabBar(this);
    connect(d->tabBar,
            &DockTabBar::currentIndexChanged,
            this,
            &DockGroup::tabBar_currentIndexChanged);
}

void DockGroup::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(DockGroup);

    if (d->mousepRessed)
        return;

    switch (d->area) {
    case Dock::Right:
    case Dock::Left:
        setCursor(d->acceptResizeEvent(event->pos()) ? Qt::SizeHorCursor
                                                     : Qt::ArrowCursor);
        break;

    case Dock::Bottom:
    case Dock::Top:
        setCursor(d->acceptResizeEvent(event->pos()) ? Qt::SizeVerCursor
                                                     : Qt::ArrowCursor);
        break;

    case Dock::Float:
    case Dock::Center:
        break;
    }
}

bool DockGroup::childMouseEventFilter(QQuickItem *, QEvent *e)
{
    Q_D(DockGroup);

    static QPointF _lastMousePos;
    static QPointF _lastChildPos;

    auto me = static_cast<QMouseEvent *>(e);
    switch (e->type()) {
    case QEvent::MouseButtonPress:
        _lastMousePos = me->windowPos();
        _lastChildPos = position();
        e->accept();
        break;

    case QEvent::MouseMove: {
        auto pt = _lastChildPos + (me->windowPos() - _lastMousePos);
        switch (d->area) {
        case Dock::Right:
        case Dock::Left:
            setWidth(pt.x());
            break;

        case Dock::Bottom:
        case Dock::Top:
            break;

        case Dock::Float:
        case Dock::Center:
            break;
        }

        break;
    }

    default:
        break;
    }

    return false;
}

void DockGroup::mousePressEvent(QMouseEvent *event)
{
    Q_D(DockGroup);

    d->mousepRessed = true;
    switch (d->area) {
    case Dock::Right:
    case Dock::Left:
        d->lastMousePos = event->windowPos().x();
        d->lastGroupSize = width();
        setKeepMouseGrab(true);
        break;

    case Dock::Bottom:
    case Dock::Top:
        d->lastMousePos = event->windowPos().y();
        d->lastGroupSize = height();
        setKeepMouseGrab(true);
        break;

    case Dock::Float:
    case Dock::Center:
        break;
    }
}

void DockGroup::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(DockGroup);

    switch (d->area) {
    case Dock::Left:
        setPanelSize(d->lastGroupSize
                     + (event->windowPos().x() - d->lastMousePos));
        break;

    case Dock::Right:
        setPanelSize(d->lastGroupSize
                     + (d->lastMousePos - event->windowPos().x()));
        break;

    case Dock::Top:
        setPanelSize(d->lastGroupSize
                     + (event->windowPos().y() - d->lastMousePos));
        break;

    case Dock::Bottom:
        setPanelSize(d->lastGroupSize
                     + (d->lastMousePos - event->windowPos().y()));
        break;

    case Dock::Float:
    case Dock::Center:
        break;
    }
}

void DockGroup::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    Q_D(DockGroup);
    d->mousepRessed = false;
    setKeepMouseGrab(false);
}

void DockGroup::setColor(const QColor &color)
{
    Q_D(DockGroup);
    //    setBackground(new DebugRect(_color, this));
}

void DockGroup::geometryChanged(const QRectF &newGeometry,
                                const QRectF &oldGeometry)
{
    Q_D(DockGroup);

    if (!d->dockWidgets.count())
        return;

    if (d->tabBar) {
        if (d->area == Dock::Center) {
            d->tabBar->setWidth(width());
            d->tabBar->setHeight(30);
            d->tabBar->setVisible(true);
        } else {
            d->tabBar->setVisible(false);
        }
    }
    for (auto dw : d->dockWidgets)
        d->fitItem(dw);

    if (d->displayType == Dock::SplitView)
        d->reorderHandles();
    d->reorderItems();
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
}

void DockGroup::tabBar_currentIndexChanged(int index)
{
    Q_D(DockGroup);
    if (d->displayType != Dock::TabbedView)
        return;

    qDebug() << "change cu=" << index << d->dockWidgets.count();
    for (int i = 0; i < d->dockWidgets.count(); ++i) {
        d->dockWidgets.at(i)->setVisible(i == index);
    }
}

void DockGroup::handler_moving(qreal pos, bool *ok)
{
    Q_D(DockGroup);

    auto handler = qobject_cast<DockGroupResizeHandler *>(sender());
    if (!handler)
        return;

    auto ps = handler->index() ? pos - d->handlers.at(handler->index() - 1)->pos()
                               - Dock::resizeHandleSize
                         : pos;

    auto ns = handler->index() == d->handlers.count() - 1
                  ? (d->isVertical() ? height() : width()) - pos
                        - Dock::resizeHandleSize
                  : d->handlers.at(handler->index() + 1)->pos() - pos
                        - Dock::resizeHandleSize;

    auto prevDockWidget = d->dockWidgets.at(handler->index());
    auto nextDockWidget = d->dockWidgets.at(handler->index() + 1);

    if (ns > 30 && ps > 30) {
        if (d->isVertical()) {
            nextDockWidget->setY(pos + Dock::resizeHandleSize);
            nextDockWidget->setHeight(ns);
            prevDockWidget->setHeight(ps);
            qDebug() << d->itemSizes << "<<<";
//            d->itemSizes[handler->index()] = height() / ps;
//            d->itemSizes[handler->index() + 1] = height() / ns;
            qDebug() << d->itemSizes << ">>>";
        }
        if (d->isHorizontal()) {
            nextDockWidget->setX(pos + Dock::resizeHandleSize);
            nextDockWidget->setWidth(ns);
            prevDockWidget->setWidth(ps);
//            d->itemSizes[handler->index()] = width() / ps;
//            d->itemSizes[handler->index() + 1] = width() / ns;
        }
        *ok = true;
    }
}

void DockGroup::handler_moved()
{
    Q_D(DockGroup);
    qreal freeSize;
    qreal totalSpace{0};

    if (d->isVertical()) {
        foreach (auto dw, d->dockWidgets)
            totalSpace += dw->height();
        freeSize = (height()
                    - (Dock::resizeHandleSize * (d->dockWidgets.count() - 1)));
    }

    if (d->isHorizontal()) {
        foreach (auto dw, d->dockWidgets)
            totalSpace += dw->width();
        freeSize = (width()
                    - (Dock::resizeHandleSize * (d->dockWidgets.count() - 1)));
    }

    int index{0};
    foreach (auto dw, d->dockWidgets) {
        if (d->isVertical())
            d->itemSizes[index++] = (dw->height() / totalSpace);
        //*freeSize;
        if (d->isHorizontal())
            d->itemSizes[index++] = (dw->width() / totalSpace);
        //*freeSize;
    }
    d->reorderItems();
}

bool DockGroup::isOpen() const
{
    Q_D(const DockGroup);
    return d->dockWidgets.count();
    //isOpen;
}

qreal DockGroup::panelSize() const
{
    Q_D(const DockGroup);
    return d->panelSize;
}

Dock::Area DockGroup::area() const
{
    Q_D(const DockGroup);
    return d->area;
}

bool DockGroup::enableResizing() const
{
    Q_D(const DockGroup);
    return d->enableResizing;
}

Dock::DockWidgetDisplayType DockGroup::displayType() const
{
       Q_D(const DockGroup);
    return d->displayType;
}

void DockGroup::addDockWidget(DockWidget *item)
{
    Q_D(DockGroup);
    item->setDockGroup(this);
    //    addItem(item);
    d->dockWidgets.append(item);
    d->normalizeItemSizes();

    if (d->dockWidgets.count() > 1)
        d->handlers.append(d->createHandlers());

    if (d->tabBar) {
        d->tabBar->addTab(item->title());
        d->tabBar->setCurrentIndex(d->dockWidgets.count() - 1);
    }

//    if (isComponentComplete())
//        geometryChanged(QRectF(), QRectF());
    d->fitItem(item);

    if (d->displayType == Dock::SplitView)
        d->reorderHandles();
    d->reorderItems();
}

void DockGroup::removeDockWidget(DockWidget *item)
{
    Q_D(DockGroup);
    auto index = d->dockWidgets.indexOf(item);
    if (index == -1)
        return;

    d->dockWidgets.removeOne(item);
    item->setDockGroup(nullptr);

    if (d->tabBar) {
        d->tabBar->removeTab(index);
        d->tabBar->setCurrentIndex(d->tabBar->currentIndex());
    }

    if (d->handlers.count()) {
        auto h = d->handlers.takeAt(d->handlers.count() - 1);

        if (h) {
            h->setParentItem(nullptr);
            h->deleteLater();
        }
    }

    d->normalizeItemSizes();
    d->reorderHandles();
    d->reorderItems();
}

void DockGroup::setIsOpen(bool isOpen)
{
    Q_D(DockGroup);
    if (d->isOpen == isOpen)
        return;

    d->isOpen = isOpen;
    emit isOpenChanged(isOpen);
}

void DockGroup::setPanelSize(qreal panelSize)
{
    Q_D(DockGroup);
    if (qFuzzyCompare(d->panelSize, panelSize))
        return;

    d->panelSize = panelSize;
    emit panelSizeChanged(panelSize);
}

void DockGroup::setArea(Dock::Area area)
{
    Q_D(DockGroup);
    if (d->area == area)
        return;

    d->area = area;
    emit areaChanged(area);
}

void DockGroup::setEnableResizing(bool enableResizing)
{
    Q_D(DockGroup);
    if (d->enableResizing == enableResizing)
        return;

    d->enableResizing = enableResizing;
    emit enableResizingChanged(enableResizing);
}

void DockGroup::setDisplayType(Dock::DockWidgetDisplayType displayType)
{
    Q_D(DockGroup);

    if (d->displayType == displayType)
        return;

    d->displayType = displayType;
    emit displayTypeChanged(displayType);
}

void DockGroup::paint(QPainter *painter)
{
    //    painter->setOpacity(1);
    //    painter->setPen(Qt::gray);
    //    painter->setBrush(Qt::white);
    //    painter->drawRect(0, 0, width() - 1, height() - 1);
    //    painter->fillRect(clipRect(), _color);
    //    painter->setOpacity(0.3);
}
