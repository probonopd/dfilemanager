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


#ifndef FLOWVIEW_H
#define FLOWVIEW_H

#include <QAbstractItemView>

class QModelIndex;
class QSplitter;
class QItemSelectionModel;
class QItemSelection;
namespace DFM
{
class Flow;
class DetailsView;
namespace FS{class Model;}
class FlowView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit FlowView(QWidget *parent = 0);
    ~FlowView();

    QModelIndex currentIndex();
    void addActions(QList<QAction *> actions);
    inline DetailsView *detailsView() { return m_dView; }
    inline Flow *flow() { return m_flow; }
    inline QSplitter *splitter() { return m_splitter; }
    void scrollToTop();
    void scrollToBottom();

    void setModel(QAbstractItemModel *model);
    void setSelectionModel(QItemSelectionModel *selectionModel);

    /*pure virtuals*/
    QModelIndex indexAt(const QPoint &point) const;
    void scrollTo(const QModelIndex & index, ScrollHint hint = EnsureVisible);
    QRect visualRect(const QModelIndex &index) const;

public slots:
    void setRootIndex(const QModelIndex &rootIndex);

signals:
    void newTabRequest(const QModelIndex &index);
    void opened(const QModelIndex &index);

protected:
    void resizeEvent(QResizeEvent *e);
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    /*pure virtuals*/
    int horizontalOffset() const;
    int verticalOffset() const;
    bool isIndexHidden(const QModelIndex &index) const;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags flags);
    QRegion visualRegionForSelection(const QItemSelection &selection) const;

private slots:
    void flowCurrentIndexChanged(const QModelIndex &index);
    void treeCurrentIndexChanged(QItemSelection, QItemSelection);
    void saveSplitter();

private:
    DetailsView *m_dView;
    QSplitter *m_splitter;
    Flow *m_flow;
};
}

#endif // FLOWVIEW_H
