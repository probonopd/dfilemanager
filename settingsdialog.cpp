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


#include "settingsdialog.h"
#include "mainwindow.h"
#include <QGroupBox>

using namespace DFM;

BehaviourWidget::BehaviourWidget(QWidget *parent) : QWidget(parent)
{
    m_hideTabBar = new QCheckBox(tr("Hide tab bar when only one tab"), this);
    m_hideTabBar->setChecked(MainWindow::config.behaviour.hideTabBarWhenOnlyOneTab);

    m_useCustomIcons = new QCheckBox(tr("Use system icons for actions (Unix only)"),this);
    m_useCustomIcons->setChecked(MainWindow::config.behaviour.systemIcons);   

    m_drawDevUsage = new QCheckBox(tr("Draw usage info on mounted devices"), this);
    m_drawDevUsage->setChecked(MainWindow::config.behaviour.devUsage);

    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->addWidget(m_hideTabBar);
    vl->addWidget(m_useCustomIcons);
    vl->addWidget(m_drawDevUsage);
    vl->addStretch();
    setLayout(vl);
}


/////////////////////////////////////////////////////////////////


StartupWidget::StartupWidget(QWidget *parent) : QWidget(parent), m_lock(MainWindow::config.docks.lock)
{
    QGroupBox *gb = new QGroupBox("Lock Docks", this);
    QVBoxLayout *docks = new QVBoxLayout(gb);
    m_left = new QCheckBox(tr("Left"));
    m_left->setChecked( bool( m_lock & Docks::Left ) );
    m_left->setProperty( "dock", (int)Docks::Left );
    connect( m_left, SIGNAL(toggled(bool)), this, SLOT(lockChanged()) );
    docks->addWidget(m_left);
    m_right = new QCheckBox(tr("Right"));
    m_right->setChecked( bool( m_lock & Docks::Right ) );
    m_right->setProperty( "dock", (int)Docks::Right );
    connect( m_right, SIGNAL(toggled(bool)), this, SLOT(lockChanged()) );
    docks->addWidget(m_right);
    m_bottom = new QCheckBox(tr("Bottom"));
    m_bottom->setChecked( bool( m_lock & Docks::Bottom ) );
    m_bottom->setProperty( "dock", (int)Docks::Bottom );
    connect( m_bottom, SIGNAL(toggled(bool)), this, SLOT(lockChanged()) );
    docks->addWidget(m_bottom);
    gb->setLayout(docks);

    m_startPath = new QLineEdit(this);
    QLabel *startup = new QLabel(this);
    QHBoxLayout *hLayout = new QHBoxLayout();
    startup->setText("Path to load at startup:");
    hLayout->addWidget(startup);
    hLayout->addWidget(m_startPath);
    QVBoxLayout *vl = new QVBoxLayout;
    vl->addLayout(hLayout);
    vl->addWidget(gb);
    vl->addStretch();
    setLayout(vl);
}


/////////////////////////////////////////////////////////////////

ViewsWidget::ViewsWidget(QWidget *parent) : QWidget(parent)
  , m_iconWidth( new QSlider( Qt::Horizontal, this ) ), m_width( new QLabel(this) )
  , m_showThumbs( new QCheckBox(tr("Show thumbnails of supported pictures (requires restart)"), this) )
  , m_smoothScroll( new QCheckBox(tr("Smooth scrolling"), this) )
  , m_rowPadding( new QSpinBox(this) )
{
    m_smoothScroll->setChecked(MainWindow::config.views.iconView.smoothScroll);
    m_showThumbs->setChecked(MainWindow::config.views.showThumbs);
    QGroupBox *gBox = new QGroupBox(tr("IconView"), this);
    QVBoxLayout *gvLayout = new QVBoxLayout();
    gvLayout->addWidget(m_smoothScroll);
    QHBoxLayout *ghLayout = new QHBoxLayout();
    ghLayout->addWidget( new QLabel(tr("Extra width added to text:"), this) );
    ghLayout->addWidget( m_iconWidth );
    m_iconWidth->setRange(1, 32);
    m_iconWidth->setSingleStep(1);
    m_iconWidth->setPageStep(1);
    m_iconWidth->setValue(MainWindow::config.views.iconView.textWidth);
    m_width->setText(QString::number(MainWindow::config.views.iconView.textWidth*2) + " px");
    ghLayout->addWidget(m_width);
    connect ( m_iconWidth, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)) );

    gvLayout->addLayout(ghLayout);
    gBox->setLayout(gvLayout);

    QGroupBox *detailsBox = new QGroupBox(tr("detailsView"), this);
    QHBoxLayout *detailsLayout = new QHBoxLayout(detailsBox);
    detailsLayout->addWidget(new QLabel(tr("Padding added to rowheight")));
    detailsLayout->addWidget(m_rowPadding);
    detailsBox->setLayout(detailsLayout);
    m_rowPadding->setMinimum(0);
    m_rowPadding->setMaximum(5);
    m_rowPadding->setValue(MainWindow::config.views.detailsView.rowPadding);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->addWidget(m_showThumbs);
    vLayout->addWidget(gBox);
    vLayout->addWidget(detailsBox);
    vLayout->addStretch();
    setLayout(vLayout);
}

/////////////////////////////////////////////////////////////////

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    m_settings = new QSettings("dfm","dfm");
    QTabWidget *tabWidget = new QTabWidget(this);
    m_startupWidget = new StartupWidget(tabWidget);
    m_behWidget = new BehaviourWidget(tabWidget);
    m_viewWidget = new ViewsWidget(tabWidget);
    m_ok = new QPushButton(this);
    m_cancel = new QPushButton(this);

    setWindowTitle(tr("Configure"));

    m_startupWidget->setStartupPath(m_settings->value("startPath").toString());

    tabWidget->addTab(m_startupWidget, "Startup");
    tabWidget->addTab(m_behWidget, "Behaviour");
    tabWidget->addTab(m_viewWidget, tr("Views"));

    QVBoxLayout *vLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    vLayout->addWidget(tabWidget);
    hLayout->addStretch();
    hLayout->addWidget(m_ok);
    hLayout->addWidget(m_cancel);
    hLayout->addStretch();
    vLayout->addLayout(hLayout);

    setLayout(vLayout);

    m_ok->setText("OK");
    m_cancel->setText("Cancel");

    connect( m_ok, SIGNAL(clicked()),this,SLOT(accept()));
    connect( m_cancel, SIGNAL(clicked()),this,SLOT(reject()));
}

void
SettingsDialog::accept()
{
    if(QFileInfo(m_startupWidget->startupPath()).isDir())
        m_settings->setValue("startPath",m_startupWidget->startupPath());
    m_settings->setValue("hideTabBarWhenOnlyOne",m_behWidget->m_hideTabBar->isChecked());
    m_settings->setValue("useSystemIcons", m_behWidget->m_useCustomIcons->isChecked());
    m_settings->setValue("docks.lock", m_startupWidget->locked() );
    m_settings->setValue("smoothScroll", m_viewWidget->m_smoothScroll->isChecked());
    m_settings->setValue("showThumbs", m_viewWidget->m_showThumbs->isChecked());
    m_settings->setValue("drawDevUsage", m_behWidget->m_drawDevUsage->isChecked());
    m_settings->setValue("textWidth", m_viewWidget->m_iconWidth->value());
    m_settings->setValue("detailsView.rowPadding", m_viewWidget->m_rowPadding->value());
    emit settingsChanged();
    QDialog::accept();
}
