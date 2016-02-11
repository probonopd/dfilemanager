/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#include "placesview.h"
#include "iojob.h"
#include "icondialog.h"
#include "viewanimator.h"
#include "mainwindow.h"
#include "config.h"
#include "fsmodel.h"

#include <QInputDialog>
#include <QMenu>
#include <QMetaObject>
#include <QMetaProperty>
#include <QApplication>
#include <QScrollBar>
#include <QMimeData>

using namespace DFM;

#define TEXT index.data().toString()
#define RECT option.rect
#define FM option.fontMetrics
#define PAL option.palette
#define DECOSIZE option.decorationSize

void
PlacesViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid() || !RECT.isValid() || !RECT.intersects(m_placesView->viewport()->rect()))
        return;
    painter->save(); //must save... have to restore at end
    painter->setRenderHint(QPainter::Antialiasing);

    const bool selected = option.state & QStyle::State_Selected;

    const QPalette::ColorRole fgr = m_placesView->foregroundRole(), bgr = m_placesView->backgroundRole();

    int step = selected ? Steps : ViewAnimator::hoverLevel(m_placesView, index);

    int textFlags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine;
    int indent = DECOSIZE.height(), textMargin = indent + (isHeader(index) ? 0 : DECOSIZE.width());
    QRect textRect(RECT.adjusted(textMargin, 0, 0, 0));

    QColor mid = Ops::colorMid(PAL.color(bgr), PAL.color(fgr));
    if (Store::config.behaviour.sideBarStyle)
        mid = Ops::colorMid(PAL.color(QPalette::Highlight), mid, 1, 5);
    QColor fg = isHeader(index) ? mid : PAL.color(fgr);
    QPalette pal(PAL);
    pal.setColor(fgr, fg);

    QFont font(index.data(Qt::FontRole).value<QFont>());
    if (selected || isHeader(index))
        font.setBold(true);
    painter->setFont(font);

    if (step && !isHeader(index))
    {
        QStyleOptionViewItem copy(option);
        if (!selected)
            copy.state |= QStyle::State_MouseOver;
        QPixmap pix(option.rect.size());
        pix.fill(Qt::transparent);
        copy.rect = pix.rect();
        QPainter p(&pix);
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &copy, &p, m_placesView);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(pix.rect(), QColor(0, 0, 0, ((255.0f/(float)Steps)*step)));
        p.end();
        painter->drawPixmap(option.rect, pix);
    }

    DeviceItem *d = m_placesView->itemFromIndex<DeviceItem *>(index);
    if (Store::config.behaviour.devUsage && d && d->isMounted() && d->totalBytes())
    {
        QPixmap pix(RECT.size());
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        QStyleOptionProgressBarV2 opt;
        opt.rect = pix.rect();
        opt.maximum = 100;
        opt.minimum = 0;
        opt.progress = d->used();
        opt.palette = option.palette;
        opt.palette.setColor(QPalette::Highlight, Ops::colorMid(Qt::green, Qt::red, 100-d->used(), d->used()));
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, &p);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(pix.rect(), QColor(0, 0, 0, 127));
        p.end();
        painter->drawPixmap(option.rect, pix);
    }
    if (d && d->isHidden())
        painter->setOpacity(0.5f);

    if (selected)
        fg = PAL.color(QPalette::HighlightedText);
    const QColor &bg = selected?PAL.color(QPalette::Highlight):PAL.color(bgr);
    int y = bg.value()>fg.value()?1:-1;
    const QColor &emb = Ops::colorMid(bg, y==1?Qt::white:Qt::black);
    QLinearGradient lgf(textRect.topLeft(), textRect.topRight());
    lgf.setColorAt(0.0f, fg);
    lgf.setColorAt(0.8f, fg);
    lgf.setColorAt(1.0f, Qt::transparent);
    QLinearGradient lgb(textRect.topLeft(), textRect.topRight());
    lgb.setColorAt(0.0f, emb);
    lgb.setColorAt(0.8f, emb);
    lgb.setColorAt(1.0f, Qt::transparent);
    painter->setPen(QPen(lgb, 1.0f));
    painter->drawText(textRect.translated(0, 1), textFlags, TEXT);
    painter->setPen(QPen(lgf, 1.0f));
    painter->drawText(textRect, textFlags, TEXT);

    if (painter->opacity() < 1.0f)
        painter->setOpacity(1.0f);
    if (isHeader(index))
    {
        QRect arrowRect(0, 0, 10, 10);
        arrowRect.moveCenter(QPoint(RECT.x()+8, RECT.center().y()));
        QStyleOption copy;
        copy.rect = arrowRect;
        //this should trick most styles to draw the arrow w/ the right color...
        copy.palette = fg;
        copy.palette.setColor(QPalette::Text, fg);
        copy.palette.setColor(QPalette::WindowText, fg);
        painter->setPen(fg);
        painter->setBrush(fg);
        QApplication::style()->drawPrimitive(bool(option.state & QStyle::State_Open)?QStyle::PE_IndicatorArrowDown:QStyle::PE_IndicatorArrowRight, &copy, painter);
    }
    QRect iconRect(QPoint(RECT.x()+(indent*0.75f), RECT.y() + (RECT.height() - DECOSIZE.height())/2), DECOSIZE);
    QPixmap iconPix = qvariant_cast<QIcon>(index.data(Qt::DecorationRole)).pixmap(DECOSIZE.height());

    if ((selected && Store::config.behaviour.invActBookmark) || Store::config.behaviour.invAllBookmarks)
    {
        QImage img = iconPix.toImage();
        img = img.convertToFormat(QImage::Format_ARGB32);
        int size = img.width() * img.height();
        QRgb *pixel = reinterpret_cast<QRgb *>(img.bits());
        int r = 0, g = 0, b = 0;
        PAL.color(selected?QPalette::HighlightedText:QPalette::Text).getRgb(&r, &g, &b); //foregroundcolor
        for (int i = 0; i < size; ++i)
            pixel[i] = qRgba(r, g, b, qBound(0, qAlpha(pixel[i]) - qGray(pixel[i]), 255));

        iconPix = QPixmap::fromImage(img);
    }

    if (qMax(iconPix.width(), iconPix.height()) > 16)
        iconPix = iconPix.scaled(QSize(16, 16), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QApplication::style()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, iconPix);
    if ((selected && Store::config.behaviour.invActBookmark) || Store::config.behaviour.invAllBookmarks)
        QApplication::style()->drawItemPixmap(painter, iconRect, Qt::AlignCenter, iconPix);
    painter->restore();
}

void
PlacesViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    const QString &newName = editor->metaObject()->userProperty().read(editor).toString();
    if (!newName.isEmpty())
        if (Place *p = m_placesView->itemFromIndex<Place *>(index))
            p->setName(newName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline QPixmap mountIcon(bool mounted, int size, QColor col)
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    QRect rect(pix.rect());
    const int d = rect.height()/3;
    rect.adjust(d, d, -d, -d);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(col, 2));
    if (mounted)
        p.setBrush(col);
    p.drawEllipse(rect);
    p.end();
    return pix;
}

DeviceItem::DeviceItem(DeviceManager *parentItem, PlacesView *view, Device *dev)
    : Place(QStringList())
    , m_device(dev)
    , m_view(view)
    , m_manager(parentItem)
    , m_button(new Button(m_view->viewport()))
    , m_timer(new QTimer(this))
    , m_isVisible(true)
    , m_isHidden(false)
{
//    QColor mid = Operations::colorMid(m_view->palette().color(QPalette::Base), m_view->palette().color(QPalette::Text));
//    QColor fg = Operations::colorMid(m_view->palette().color(QPalette::Highlight), mid, 1, 5);
    m_button->setVisible(true);
    m_button->setToolTip(isMounted() ? "Unmount" : "Mount");
    connect(m_button, SIGNAL(clicked()), this, SLOT(toggleMount()));
    connect(m_button, SIGNAL(destroyed()), this, SLOT(buttonDestroyed()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateSpace()));
    m_timer->start(1000);
    connect(m_device, SIGNAL(accessibilityChanged(bool, const QString &)), this, SLOT(changeState()));
    updateSpace();
    setDragEnabled(false);
    connect(m_view, SIGNAL(expanded(QModelIndex)), this, SLOT(updateTb()));
    connect(m_view, SIGNAL(changed()), this, SLOT(updateTb()));
    connect(m_view, SIGNAL(collapsed(QModelIndex)), this, SLOT(updateTb()));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateTb()));
    QTimer::singleShot(0, this, SLOT(updateTb()));
    setEditable(false);

    QAction *a = new QAction(tr("hidden"), this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(setHidden()));

    m_actions << a;
    QTimer::singleShot(0, this, SLOT(updateIcon()));
}

DeviceItem::~DeviceItem()
{
    if (m_button)
        m_button->hide();
    m_button = 0; //parented by the viewport so no need to delete...
}

void
DeviceItem::updateTb()
{
    const QRect &rect = m_view->visualRect(index());
    if (isVisible())
        m_button->setVisible(m_view->isExpanded(m_manager->index()));

    m_button->move(12, rect.y());
    m_button->resize(16, rect.height());
}

void
DeviceItem::setVisible(bool v)
{
    m_isVisible = v;
    m_view->setRowHidden(row(), m_manager->index(), !v);
    m_button->setVisible(v);
    foreach (DeviceItem *d, m_view->deviceManager()->deviceItems())
        d->updateTb();
}

void DeviceItem::hide()
{
    setVisible(false);
    if (isHidden())
    {
        m_actions[0]->setChecked(true);
        m_view->deviceManager()->actions().at(0)->setEnabled(true);
    }
}

void DeviceItem::show() { setVisible(true); }

void
DeviceItem::setHidden()
{
    if (QAction *a = static_cast<QAction *>(sender()))
    {
        if (a->isChecked())
        {
            m_view->addHiddenDevice(devPath());
            hide();
        }
        else
            m_view->removeHiddenDevice(devPath());
    }
    if (m_view->hiddenDevices().isEmpty())
    {
        m_view->deviceManager()->actions().at(0)->setChecked(false);
        m_view->deviceManager()->actions().at(0)->setEnabled(false);
    }
}

void DeviceItem::updateIcon() { m_button->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole()))); }

bool DeviceItem::isHidden() const { return m_view->hiddenDevices().contains(devPath()); }

void
DeviceItem::updateSpace()
{
    if (isMounted())
    {
        static quint64 s_fb(0);
        const quint64 fb(freeBytes());
        if (fb == s_fb)
            return;
        s_fb = fb;
        if (Store::config.behaviour.devUsage)
            m_view->update(index());
//        int t = 0;
//        QString free(QString::number(realSize((float)freeBytes(), &t)));
        setToolTip(Ops::prettySize(fb) + " Free");
    }
}

void
DeviceItem::changeState()
{
    if (isMounted())
        setToolTip(Ops::prettySize(freeBytes()) + " Free");
    else
        setToolTip(QString());
    m_button->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole())));
    m_button->setToolTip(isMounted() ? "Unmount" : "Mount");
}

DeviceManager::DeviceManager(const QStringList &texts, QObject *parent)
    : QObject(parent)
    , Container(QString("Devices"))
    , m_view(static_cast<PlacesView *>(parent))
{
    setDragEnabled(false);
    setDropEnabled(false);
    setEditable(false);
    QAction *as = new QAction(tr("show hidden devices"), this);
    as->setCheckable(true);
    connect(as, SIGNAL(triggered()), this, SLOT(showHiddenDevices()));
    connect(Devices::instance(), SIGNAL(deviceAdded(Device*)), this, SLOT(devAdded(Device*)));
    connect(Devices::instance(), SIGNAL(deviceRemoved(Device*)), this, SLOT(devRemoved(Device*)));
    m_actions << as;
    addDevs();
}

DeviceManager::~DeviceManager()
{

}

void
DeviceManager::showHiddenDevices()
{
    QAction *a = static_cast<QAction *>(sender());
    if (a->isChecked())
        m_view->showHiddenDevices();
    else
        m_view->hideHiddenDevices();
}

void
DeviceManager::devAdded(Device *dev)
{
    DeviceItem *d = new DeviceItem(this, m_view, dev);
    appendRow(d);
    m_items.insert(dev, d);
}

void
DeviceManager::devRemoved(Device *dev)
{
    if (m_items.contains(dev))
    {
        DeviceItem *d = m_items.take(dev);
        removeRow(d->row());
    }
}

void
DeviceManager::addDevs()
{
    foreach (Device *dev, Devices::instance()->devices())
    {
        DeviceItem *d = new DeviceItem(this, m_view, dev);
        appendRow(d);
        m_items.insert(dev, d);
    }
}

DeviceItem
*DeviceManager::deviceItemForFile(const QString &file)
{
    const QFileInfo f(file);
    QDir dir = f.isDir()?f.filePath():f.path();
    do {
        foreach (DeviceItem *item, m_items.values())
            if (dir.path() == item->mountPath())
                return item;
    } while (dir.cdUp());
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QVariant
PlacesModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::DecorationRole || role == Qt::FontRole)
    {
        if (Container *c = m_places->itemFromIndex<Container *>(index))
        {
            if (role == Qt::DisplayRole)
                return Store::config.behaviour.capsContainers ? c->name().toUpper() : c->name();
            if (role == Qt::FontRole)
            {
                QFont font = m_places->font();
                font.setPointSize(qMax<int>(Store::config.behaviour.minFontSize, m_places->font().pointSize()*0.8));
                return font;
            }
            return QVariant();
        }
        if (Place *p = m_places->itemFromIndex<Place *>(index))
            if (role == Qt::DisplayRole)
                return p->name();
            else if (role == Qt::DecorationRole && !dynamic_cast<DeviceItem *>(p))
            {
                QIcon icon = QIcon::fromTheme(p->iconName());
                if (icon.isNull())
                    icon = FS::FileIconProvider::fileIcon(QFileInfo(p->path()));
                return icon;
            }
    }
    if (role == Qt::ToolTipRole)
        if (Place *p = m_places->itemFromIndex<Place *>(index))
            if (!dynamic_cast<DeviceItem *>(p))
                return p->path();
    return QStandardItemModel::data(index, role);
}

PlacesModel::~PlacesModel()
{
    m_places = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PlacesView::PlacesView(QWidget *parent)
    : QTreeView(parent)
    , m_timer(new QTimer(this))
    , m_devManager(0L)
    , m_lastClicked(0L)
{
    ScrollAnimator::manage(this);
    setModel(m_model = new PlacesModel(this));
    ViewAnimator::manage(this);
    m_timer->setInterval(1000);

    setUniformRowHeights(false);
    setAllColumnsShowFocus(true);
    setItemDelegate(new PlacesViewDelegate(this));
    setRootIsDecorated(true);
    setHeaderHidden(true);
    setItemsExpandable(true);
    setIndentation(0);
    setIconSize(QSize(16, 16));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragDropMode(QAbstractItemView::DragDrop);
    setAcceptDrops(true);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setEditTriggers(NoEditTriggers);
    setMouseTracking(true);
    setExpandsOnDoubleClick(false);
    setAnimated(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(this, SIGNAL(changed()), this, SLOT(store()));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(emitPath(QModelIndex)));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateAllWindows()));
    connect(MainWindow::currentWindow(), SIGNAL(settingsChanged()), viewport(), SLOT(update()));
    QTimer::singleShot(0, this, SLOT(paletteOps()));
}

PlacesView::~PlacesView()
{
    m_lastClicked = 0;
}

void
PlacesView::paletteOps()
{
    QPalette::ColorRole bg = viewport()->backgroundRole(), fg = viewport()->foregroundRole();
    if (Store::config.behaviour.sideBarStyle == -1)
    {
        viewport()->setAutoFillBackground(false);
        setFrameStyle(0);
    }
    if (!viewport()->autoFillBackground())
    {
        bg = backgroundRole();
        fg = foregroundRole();
    }
    setBackgroundRole(bg);
    setForegroundRole(fg);
    viewport()->setBackgroundRole(bg);
    viewport()->setForegroundRole(fg);
    QPalette pal = palette();
    const QColor fgc = pal.color(fg), bgc = pal.color(bg);
    if (Store::config.behaviour.sideBarStyle == 1)
    {
        viewport()->setAutoFillBackground(true);
        //base color... slight hihglight tint
        QColor midC = Ops::colorMid(pal.color(QPalette::Base), qApp->palette().color(QPalette::Highlight), 10, 1);
        pal.setColor(bg, Ops::colorMid(Qt::black, midC, 255-pal.color(QPalette::Window).value(), 255));
        pal.setColor(fg, qApp->palette().color(fg));
    }
    else if (Store::config.behaviour.sideBarStyle == 2)
    {
        pal.setColor(bg, Ops::colorMid(fgc, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, bgc);
    }
    else if (Store::config.behaviour.sideBarStyle == 3)
    {
        const QColor &wtext = pal.color(QPalette::WindowText), w = pal.color(QPalette::Window);
        pal.setColor(bg, Ops::colorMid(wtext, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, w);
    }
    setPalette(pal);
}

void
PlacesView::paletteChange(const QPalette &pal)
{
    QPalette p(pal);
    p.setColor(QPalette::Highlight, qApp->palette().color(QPalette::Highlight));
    setPalette(p);
}

#define RETURN \
    event->setDropAction(Qt::IgnoreAction); \
    event->accept(); \
    QTreeView::dropEvent(event); \
    return

void
PlacesView::dropEvent(QDropEvent *event)
{
    Container *cont = itemAt<Container *>(event->pos());
    Place *place = itemAt<Place *>(event->pos());

    QStringList files;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        const QString &file = url.path();
        if (QFileInfo(file).isReadable())
            files << file;
    }

    if (dropIndicatorPosition() == OnViewport)
    {
        if (event->source() == this) { RETURN; }

        Container *places = 0L;
        foreach (Container *c, containers())
            if (c->name() == "Places")
                places = c;
        if (!places)
        {
            places = new Container("Places");
            m_model->insertRow(0, places);
        }
        foreach (const QString &file, files)
        {
            const QFileInfo f(file);
            if (!f.isDir())
                continue;
            QSettings settings(QDir(file).absoluteFilePath(".directory"), QSettings::IniFormat);
            QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
            if (icon.isNull()||icon.name().isNull())
                icon = FS::FileIconProvider::fileIcon(f);
            places->appendRow(new Place(f.fileName(), f.filePath(), icon.name(), icon));
        }
        expand(indexFromItem(places));
        emit changed();
        RETURN;
    }

    if (itemAt<DeviceItem *>(event->pos())
         ||  (m_lastClicked && dynamic_cast<DeviceItem *>(m_lastClicked)
               && dropIndicatorPosition() != OnItem
               && event->source() == this)
         || m_lastClicked && dynamic_cast<DeviceManager *>(m_lastClicked)
         || (itemAt<DeviceManager *>(event->pos())
              && dropIndicatorPosition() != AboveItem))
    {
        RETURN;
    }

    const bool below = bool(dropIndicatorPosition() == BelowItem);

    if (event->source() != this)
    {
        if (files.isEmpty()) { RETURN; }

        if (dropIndicatorPosition() == OnItem)
        {

            if (cont) //dropping files on a container
            {
                foreach (const QString &file, files)
                {
                    const QFileInfo f(file);
                    if (!f.isDir())
                        continue;
                    QSettings settings(QDir(file).absoluteFilePath(".directory"), QSettings::IniFormat);
                    QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
                    if (icon.isNull()||icon.name().isNull())
                        icon = FS::FileIconProvider::fileIcon(f);
                    addPlace(f.fileName(), f.filePath(), icon, cont);
                }
                emit changed();
                RETURN;
            }

            if (place) //dropping files on a place
            {
                QApplication::restoreOverrideCursor();
                QMenu m;
                QAction *move = m.addAction(QString("move to %1").arg(place->path()));
                m.addAction(QString("copy to %1").arg(place->path()));
                if (QAction *a = m.exec(viewport()->mapToGlobal(event->pos())))
                {
                    const bool cut(a == move);
                    IO::Manager::copy(files, place->path(), cut, cut);
                }
                RETURN;
            }

        }
        else if (dropIndicatorPosition() != QAbstractItemView::OnViewport)
        {
            foreach (const QString &file, files)
            {
                const QFileInfo f(file);
                if (!f.isDir())
                    continue;
                QSettings settings(QDir(file).absoluteFilePath(".directory"), QSettings::IniFormat);
                const QIcon &icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString(), QIcon::fromTheme("inode-directory", QIcon::fromTheme("folder")));
                Place *item = new Place(f.fileName(), f.filePath(), icon.name());
                Container *c = container(indexAt(event->pos()).parent().row());
                if (c)
                    c->insertRow(indexAt(event->pos()).row()+below, item);
                else
                    cont->insertRow(0, item);
                emit changed();
            }
            RETURN;
        }
    }
    else
    {
        if (cont)
            if (Container *movedCont = dynamic_cast<Container *>(m_lastClicked)) //moving container
            {
                if (dropIndicatorPosition() == AboveItem)
                {
                    Container *newCont = new Container(movedCont);
                    m_model->insertRow(cont->row(), newCont);
                    expand(indexFromItem(newCont));
                    m_model->removeRow(movedCont->row());
                    emit changed();
                }
                RETURN;
            }
            else if (Place *movedPlace = dynamic_cast<Place *>(m_lastClicked))
            {
                if (dropIndicatorPosition() == OnItem)
                    cont->appendRow(new Place(movedPlace));
                else if (dropIndicatorPosition() == BelowItem)
                    cont->insertRow(0, new Place(movedPlace));
                movedPlace->parent()->removeRow(movedPlace->row());
                emit changed();
                RETURN;
            }

        if (place)
            if (Place *movedPlace = dynamic_cast<Place *>(m_lastClicked))
            {
                if (Container *c = dynamic_cast<Container *>(place->parent()))
                {
                    c->insertRow(place->row() + below, new Place(movedPlace));
                    movedPlace->parent()->removeRow(movedPlace->row());
                    emit changed();
                }
                RETURN;
            }
    }
    RETURN;
}

void
PlacesView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        event->accept();
    else
        QTreeView::mouseMoveEvent(event);
}

void
PlacesView::mouseReleaseEvent(QMouseEvent *event)
{
//    m_lastClicked = itemAt(event->pos());
    if (event->button() == Qt::MiddleButton)
    {
        event->accept();
        return;
    }
    if (!indexAt(event->pos()).parent().isValid() && indexAt(event->pos()).isValid())
        setExpanded(indexAt(event->pos()), !isExpanded(indexAt(event->pos())));
    QTreeView::mouseReleaseEvent(event);
}

void
PlacesView::mousePressEvent(QMouseEvent *event)
{
    m_lastClicked = itemAt(event->pos());
    if (event->button() == Qt::MiddleButton)
    {
        event->accept();
        if (indexAt(event->pos()).isValid() && indexAt(event->pos()).parent().isValid())
            emit newTabRequest(QUrl::fromLocalFile(static_cast<Place *>(itemAt(event->pos()))->path()));
        return;
    }
    else
        QTreeView::mousePressEvent(event);
}

void
PlacesView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
        removePlace();
    QTreeView::keyPressEvent(event);
}

void
PlacesView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F2)
        renPlace();
    QTreeView::keyReleaseEvent(event);
}

void
PlacesView::drawItemsRecursive(QPainter *painter, const QRect &clip, const QModelIndex &parent)
{
    for (int i = 0; i < model()->rowCount(parent); ++i)
    {
        const QModelIndex &index = model()->index(i, 0, parent);
        const QRect vr(visualRect(index));
        if (vr.intersects(clip))
        {
            QStyleOptionViewItemV4 option(viewOptions());
            option.rect=vr;
            if (ViewAnimator::hoverLevel(this, index))
                option.state |= QStyle::State_MouseOver;
            if (selectionModel()->isSelected(index))
                option.state |= QStyle::State_Selected;
            if (isExpanded(index))
                option.state |= QStyle::State_Open;

            itemDelegate()->paint(painter, option, index);
        }
        drawItemsRecursive(painter, clip, index);
    }
}

void
PlacesView::paintEvent(QPaintEvent *event)
{
//    QTreeView::paintEvent(event);
    QPainter p(viewport());
    drawItemsRecursive(&p, event->rect());
    const QModelIndex &index = indexAt(mapFromGlobal(QCursor::pos()));
    if (showDropIndicator() && state() == QAbstractItemView::DraggingState
            && viewport()->cursor().shape() != Qt::ForbiddenCursor && m_model->flags(index) & Qt::ItemIsDropEnabled)
    {
        QStyleOption opt = viewOptions();
//        opt.init(this);
        QRect rect(visualRect(index));
        if (dropIndicatorPosition() == AboveItem)
            rect.setBottom(rect.top()-1);
        else if (dropIndicatorPosition() == BelowItem)
            rect.setTop(rect.bottom()+1);
        opt.rect = rect;
        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, &p, this);
    }
    p.end();
}

bool
PlacesView::hasPlace(const QString &path) const
{
    for (int c = 0; c < containers().count(); ++c)
    {
        Container *cont = container(c);
        for (int p = 0; p < cont->rowCount(); ++p)
        {
            if (cont->place(p)->path() == path)
                return true;
        }
    }
    return false;
}

void
PlacesView::removePlace()
{
    if (currentItem())
        if (currentItem() != m_devManager
                && !m_devManager->isDevice(currentItem()))
        {
            delete currentItem();
            m_lastClicked = 0;
            if (currentItem()->parent())
                currentItem()->parent()->removeRow(currentIndex().row());
            else
                m_model->removeRow(currentIndex().row());
            emit changed();
        }
}

void
PlacesView::renPlace()
{
    if (Place *place = dynamic_cast<Place *>(currentItem()))
    {
        m_model->setData(place->index(), place->name());
        edit(place->index());
        emit changed();
    }
}

void
PlacesView::addPlace(QString name, QString path, QIcon icon, QStandardItem *parent, const bool storeSettings)
{
    Container *cont = dynamic_cast<Container *>(parent);
    if (!cont)
        cont = dynamic_cast<Container *>(currentItem());
    if (!cont)
        return;
    Place *place = new Place(name, path, icon.name(), icon);
    place->setFlags(place->flags() | Qt::ItemIsEditable);
    cont->appendRow(place);
    if (storeSettings)
    {
        store();
        emit changed();
    }
}

void
PlacesView::addPlaceCont()
{
    Container *item = new Container("New_Container");
    m_model->insertRow(0, item);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    edit(item->index());
    expand(indexFromItem(item));
    emit changed();
}

void
PlacesView::setPlaceIcon()
{
    if (!m_lastClicked)
        return;
    const QString &i = IconDialog::iconName();
    if (i.isEmpty() || !m_lastClicked->parent())
        return;
    if (Place *place = dynamic_cast<Place *>(m_lastClicked))
    {
        place->setIconName(i);
        emit changed();
    }
}

void
PlacesView::activateAppropriatePlace(const QString &path)
{
    if (path.isNull() || path.isEmpty())
        return;
    for (int i = 0; i < containerCount(); i++)
        for(int a = 0; a < container(i)->rowCount(); a++)
        {
            Place *place = container(i)->place(a);
            Place *current = static_cast<Place *>(currentItem());
            if (place->path() == path)
            {
                setCurrentItem(place);
                return;
            }
            else if (place && current && path.contains(place->path()))
                if (current->path().length() < place->path().length())
                    setCurrentItem(place);
        }
}

void
PlacesView::showHiddenDevices()
{
    m_hiddenDevices.removeDuplicates();
    foreach (DeviceItem *d, deviceManager()->deviceItems())
        if (d->isHidden())
            d->show();
}

void
PlacesView::hideHiddenDevices()
{
    m_hiddenDevices.removeDuplicates();
    foreach (DeviceItem *d, deviceManager()->deviceItems())
        if (d->isHidden())
            d->hide();
}

void
PlacesView::contextMenuEvent(QContextMenuEvent *event)
{
    if (indexAt(event->pos()).isValid() && indexAt(event->pos()).parent().isValid())
        m_lastClicked = itemAt(event->pos());

    QMenu popupMenu;

    if (DeviceItem *d = itemAt<DeviceItem *>(event->pos()))
        popupMenu.addActions(d->actions());
    else if (DeviceManager *dm = itemAt<DeviceManager *>(event->pos()))
        popupMenu.addActions(dm->actions());
    else
        popupMenu.addActions(actions());
    popupMenu.exec(event->globalPos());
}

QMenu
*PlacesView::containerAsMenu(const int cont)
{
    QMenu *menu = new QMenu(container(cont)->name());
    for (int i = 0; i < container(cont)->rowCount(); ++i)
    {
        QAction *action = new QAction(container(cont)->place(i)->name(), qApp);
        action->setData(container(cont)->place(i)->path());
        connect (action, SIGNAL(triggered()), Ops::instance(), SLOT(setRootPath()));
        menu->addAction(action);
    }
    return menu;
}

void
PlacesView::updateAllWindows()
{
    if (!m_devManager)
        return;

    m_timer->stop();

    if (MainWindow::openWindows().count() == 1)
        return;

    foreach (MainWindow *mw, MainWindow::openWindows())
        if (mw != MainWindow::currentWindow())
            mw->placesView()->populate();
}

void
PlacesView::populate()
{
    if (m_model)
        m_model->clear();
    m_devManager = 0L;
#if defined(ISUNIX)
    QSettings s("dfm", "bookmarks");
#else
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "dfm", "bookmarks");
#endif
    foreach (const QString &container, s.value("Containers").toStringList())
    {
        Container *cont = new Container(container);
        m_model->appendRow(cont);
        s.beginGroup(container);
        foreach (const QString &itemString, s.childKeys())
        {
            const QStringList &values = s.value(itemString).toStringList();
            if (values.count() == 3)
                cont->appendRow(new Place(values));
        }
        s.beginGroup("Options");
        setExpanded(cont->index(), s.value("Expanded", false).toBool());
        s.endGroup();
        s.endGroup();
    }

    m_model->appendRow(m_devManager = new DeviceManager(QStringList() << tr("Devices"), this));
    expand(m_devManager->index());

    QStringList hiddenDevices = s.value("hiddenDevices", QStringList()).toStringList();
    if (hiddenDevices.count())
    {
        m_hiddenDevices = hiddenDevices;
        hideHiddenDevices();
    }
}

void
PlacesView::store()
{
#if defined(ISUNIX)
    QSettings s("dfm", "bookmarks");
#else
    QSettings s(QSettings::IniFormat, QSettings::UserScope, "dfm", "bookmarks");
#endif
    s.clear();

    QStringList conts;
    foreach (const Container *c, containers())
        conts << c->name();
    s.setValue("Containers", conts);

    foreach (Container *c, containers())
    {
        s.beginGroup(c->name());
        foreach (Place *p, c->places())
            s.setValue(QString::number(p->row()), p->values());
        if (c->rowCount())
        {
            s.beginGroup("Options");
            s.setValue("Expanded", isExpanded(indexFromItem(c)));
            s.endGroup();
        }
        s.endGroup();
    }
    s.setValue("hiddenDevices", m_hiddenDevices);
    m_timer->start();
}

void
PlacesView::emitPath(const QModelIndex &index)
{
    if (Place *p = itemFromIndex<Place *>(index))
        if (!dynamic_cast<Container *>(p))
            emit placeActivated(QUrl::fromLocalFile(p->path()));
}

void
PlacesView::drawBranches(QPainter *painter,
                              const QRect &rect,
                              const QModelIndex &index) const
{
#if 0 //we want this to be empty.... we dont want any branch.. at least not atm
    if (!index.parent().isValid())
    {
        painter->save();
        QRect r/*(rect)*/;
        r.setSize(QSize(8, 8));
        r.moveCenter(QPoint(rect.right()+iconSize().height()/2, rect.center().y()));
        QPainterPath triangle;
        if (itemFromIndex(index)->isExpanded())
        {
            r.translate(0, 1);
            triangle.moveTo(r.topLeft());
            triangle.lineTo(r.topRight());
            triangle.lineTo(r.adjusted(0, 0, -r.width()/2, 0).bottomRight());
        }
        else
        {
            triangle.moveTo(r.topLeft());
            triangle.lineTo(r.adjusted(0, r.height()/2, 0, 0).topRight());
            triangle.lineTo(r.bottomLeft());
        }
        triangle.closeSubpath();

        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(palette().color(QPalette::WindowText));
        painter->drawPath(triangle);
        painter->restore();
    }
#endif
}

bool
PlacesView::getKdePlaces()
{
    QFile *file = new QFile(QString("%1/.kde4/share/apps/kfileplaces/bookmarks.xml").arg(QDir::homePath()));
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        file->setFileName(QString("%1/.kde/share/apps/kfileplaces/bookmarks.xml").arg(QDir::homePath()));
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
    }

    QDomDocument doc("places");
    if (!doc.setContent(file))
    {
        file->close();
        return false;
    }

    foreach (Container *c, containers())
        if (c->name() == "KDEPlaces")
        {
            file->close();
            return false;
        }

    Container *kdePlaces = new Container("KDEPlaces");
    m_model->insertRow(qMax(0, m_model->rowCount()-1), kdePlaces);
    QDomNodeList nodeList = doc.documentElement().childNodes();
    for(int i = 0;i < nodeList.count(); i++)
    {
        QDomElement el = nodeList.at(i).toElement();
        if (el.nodeName() == "bookmark")
        {
            QStringList values;
            QDomAttr a = el.attributeNode("href");
            values << a.value();
            QDomElement title = el.elementsByTagName("title").at(0).toElement();
            values << title.text();
            QDomElement info = el.elementsByTagName("info").at(0).toElement();
            QDomElement metadata = info.elementsByTagName("metadata").at(0).toElement();
            QDomElement eleIcon = metadata.elementsByTagName("bookmark:icon").at(0).toElement();
            values << eleIcon.attributeNode("name").value();

            const QString &path = QUrl(values.at(0)).toLocalFile();
            if (path.isEmpty() || !QFileInfo(path).exists())
                continue;

            const QString &icon = QIcon::hasThemeIcon(values[2])?values[2]:"inode-directory";
            kdePlaces->appendRow(new Place(values[1], path, icon));
        }
    }
    expand(kdePlaces->index());
    file->close();
    return true;
}
