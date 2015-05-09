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


#ifndef SEARCHBOX_H
#define SEARCHBOX_H

#include <QLineEdit>
#include "widgets.h"
#include "globals.h"

class QTimer;
namespace DFM
{

class SearchIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit SearchIndicator(QWidget *parent = 0);

public slots:
    void start();
    void stop();

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void animate();

private:
    QPixmap m_pixmap;
    bool m_in;
    int m_step;
    QTimer *m_timer;
};

class SearchBox;
class SearchTypeSelector : public Button
{
    Q_OBJECT
public:
    explicit SearchTypeSelector(SearchBox *parent = 0);

public slots:
    void updateIcon();

signals:
    void modeChanged(const SearchMode mode);

private slots:
    void filter();
    inline void search() { emit modeChanged(Search); }
    void cancel();
    void closeSearch();

private:
    SearchBox *m_searchBox;
};

class ClearSearch : public Button
{
    Q_OBJECT
public:
    explicit ClearSearch(QWidget *parent = 0);

public slots:
    void updateIcon();
};

class SearchBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchBox(QWidget *parent = 0);
    ~SearchBox(){}
    QSize sizeHint() const { return QSize(320, QLineEdit::sizeHint().height()); }
//    QSize minimumSizeHint() const { return QLineEdit::minimumSizeHint()+QSize(64, 0); }
    inline SearchMode mode() { return m_mode; }

public slots:
    void setMode(const SearchMode mode);

protected:
    void resizeEvent(QResizeEvent *);
    QPoint posFor(bool selector = true) const;

private slots:
    void setClearButtonEnabled(QString filter) { m_clearSearch->setVisible(!filter.isEmpty()); }
    void search();
    void correctSelectorPos();


private:
    SearchTypeSelector *m_selector;
    ClearSearch *m_clearSearch;
    int m_margin;
    SearchMode m_mode;
};

}

#endif // SEARCHBOX_H
