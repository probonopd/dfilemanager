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


#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QSettings>
#include <QFileInfo>
#include <QCheckBox>
#include <QDebug>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QWidget>

namespace DFM
{

class StartupWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit StartupWidget(QWidget *parent = 0);
    inline QString startupPath() { return m_startPath->text(); }
    inline void setStartupPath(QString path) { m_startPath->setText(path); }
    QLineEdit *m_sheetEdit;
private slots:
    void getShitPath();
    void getStartPath();

private:
    friend class SettingsDialog;
    QLineEdit *m_startPath;
};

class BehaviourWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BehaviourWidget(QWidget *parent = 0);

private:
    friend class SettingsDialog;
    QGroupBox *m_tabsBox;
    QComboBox *m_tabShape, *m_layOrder, *m_pathBarPlace;
    QSpinBox *m_tabRndns, *m_tabHeight, *m_tabWidth, *m_overlap;
    QCheckBox *m_hideTabBar, *m_useCustomIcons, *m_drawDevUsage, *m_newTabButton, *m_capsConts, *m_invActBookm, *m_invAllBookm, *m_useIOQueue, *m_showCloseTabButton;
    StartupWidget *m_startUpWidget;
};

class ViewsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ViewsWidget(QWidget *parent = 0);

private slots:
    inline void sliderChanged(const int value) { m_width->setText(QString::number(value*2) + " px"); }
    inline void sizeChanged(const int value) { m_size->setText(QString::number(value*16) + " px"); }

private:
    friend class SettingsDialog;
    QCheckBox *m_singleClick, *m_dirSettings, *m_categorized, *m_altRows;
    QSlider *m_iconWidth, *m_iconSlider;
    QString m_iconWidthStr;
    QLabel *m_width, *m_size;
    QSpinBox *m_rowPadding, *m_lineCount, *m_colWidth;
    QComboBox *m_viewBox;
    QGroupBox *m_showThumbs;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = 0);
    
public slots:
    void accept();

private:
    QPushButton *m_ok,*m_cancel;
    BehaviourWidget *m_behWidget;
    ViewsWidget *m_viewWidget;
};

}

#endif // SETTINGSDIALOG_H
