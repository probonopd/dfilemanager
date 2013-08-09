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
#include "config.h"
#include "mainwindow.h"
#include "application.h"
#include <QGroupBox>

using namespace DFM;

BehaviourWidget::BehaviourWidget(QWidget *parent) : QWidget(parent)
{
    m_hideTabBar = new QCheckBox(tr("Hide tab bar when only one tab"), this);
    m_hideTabBar->setChecked(Configuration::config.behaviour.hideTabBarWhenOnlyOneTab);

    m_useCustomIcons = new QCheckBox(tr("Use system icons for actions (Unix only)"),this);
    m_useCustomIcons->setChecked(Configuration::config.behaviour.systemIcons);

    m_drawDevUsage = new QCheckBox(tr("Draw usage info on mounted devices"), this);
    m_drawDevUsage->setChecked(Configuration::config.behaviour.devUsage);

    QVBoxLayout *vl = new QVBoxLayout(this);
    vl->addWidget(m_hideTabBar);
    vl->addWidget(m_useCustomIcons);
    vl->addWidget(m_drawDevUsage);
    vl->addStretch();
    setLayout(vl);
}


/////////////////////////////////////////////////////////////////


StartupWidget::StartupWidget(QWidget *parent) : QWidget(parent),
    m_lock(Configuration::config.docks.lock)
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
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(new QLabel(tr("Path to load at startup:"), this));
    hLayout->addStretch();
    hLayout->addWidget(m_startPath);
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addLayout(hLayout);
//    vl->addLayout(defView);
    vl->addWidget(gb);
    vl->addStretch();
    setLayout(vl);
}


/////////////////////////////////////////////////////////////////

ViewsWidget::ViewsWidget(QWidget *parent) : QWidget(parent)
  , m_iconWidth( new QSlider( Qt::Horizontal, this ) ), m_width( new QLabel(this) )
  , m_showThumbs( new QCheckBox(tr("Show thumbnails of supported pictures"), this) )
  , m_smoothScroll( new QCheckBox(tr("Smooth scrolling"), this) )
  , m_rowPadding( new QSpinBox(this) )
  , m_iconSlider( new QSlider( Qt::Horizontal, this ) )
  , m_size( new QLabel(QString::number(Configuration::config.views.iconView.iconSize*16) + " px", this) )
  , m_viewBox(new QComboBox(this))
{
    m_smoothScroll->setChecked(Configuration::config.views.iconView.smoothScroll);
    m_showThumbs->setChecked(Configuration::config.views.showThumbs);
    QGroupBox *gBox = new QGroupBox(tr("IconView"), this);
    QVBoxLayout *gvLayout = new QVBoxLayout();
    gvLayout->addWidget(m_smoothScroll);
    QHBoxLayout *ghLayout = new QHBoxLayout();
    ghLayout->addWidget( new QLabel(tr("Extra width added to text:"), this) );
    ghLayout->addStretch();
    ghLayout->addWidget( m_iconWidth );
    m_iconWidth->setRange(1, 32);
    m_iconWidth->setSingleStep(1);
    m_iconWidth->setPageStep(1);
    m_iconWidth->setValue(Configuration::config.views.iconView.textWidth);
    m_width->setText(QString::number(Configuration::config.views.iconView.textWidth*2) + " px");
    ghLayout->addWidget(m_width);
    connect ( m_iconWidth, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)) );

    gvLayout->addLayout(ghLayout);

    QHBoxLayout *hIconSize = new QHBoxLayout();
    hIconSize->addWidget(new QLabel(tr("Default size for icons:"), this));
    m_iconSlider->setRange(1, 16);
    m_iconSlider->setSingleStep(1);
    m_iconSlider->setPageStep(1);
    m_iconSlider->setValue(Configuration::config.views.iconView.iconSize);
    hIconSize->addStretch();
    hIconSize->addWidget(m_iconSlider);
    hIconSize->addWidget(m_size);
    connect( m_iconSlider, SIGNAL(valueChanged(int)), this, SLOT(sizeChanged(int)) );

    gvLayout->addLayout(hIconSize);

    gBox->setLayout(gvLayout);

    QGroupBox *detailsBox = new QGroupBox(tr("DetailsView"), this);
    QHBoxLayout *detailsLayout = new QHBoxLayout(detailsBox);
    detailsLayout->addWidget(new QLabel(tr("Padding added to rowheight:")));
    detailsLayout->addStretch();
    detailsLayout->addWidget(m_rowPadding);
    detailsBox->setLayout(detailsLayout);
    m_rowPadding->setMinimum(0);
    m_rowPadding->setMaximum(5);
    m_rowPadding->setValue(Configuration::config.views.detailsView.rowPadding);

    m_viewBox->insertItems(0, QStringList() << "Icons" << "Details" << "Columns" << "Flow");
    m_viewBox->setCurrentIndex(Configuration::config.behaviour.view);
    QHBoxLayout *defView = new QHBoxLayout();
    defView->addWidget(new QLabel(tr("Default view:"), this));
    defView->addStretch();
    defView->addWidget(m_viewBox);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->addWidget(m_showThumbs);
    vLayout->addLayout(defView);
    vLayout->addWidget(gBox);
    vLayout->addWidget(detailsBox);
    vLayout->addStretch();
    setLayout(vLayout);
}

/////////////////////////////////////////////////////////////////

static SettingsDialog *inst = 0;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    QTabWidget *tabWidget = new QTabWidget(this);
    m_startupWidget = new StartupWidget(tabWidget);
    m_behWidget = new BehaviourWidget(tabWidget);
    m_viewWidget = new ViewsWidget(tabWidget);
    m_ok = new QPushButton(this);
    m_cancel = new QPushButton(this);

    setWindowTitle(tr("Configure"));

    m_startupWidget->setStartupPath(Configuration::settings()->value("startPath").toString());

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

SettingsDialog
*SettingsDialog::instance()
{
    if ( !inst )
    {
        inst = new SettingsDialog(MAINWINDOW);
        connect( inst, SIGNAL(settingsChanged()), MAINWINDOW, SLOT(updateConfig()) );
    }
    return inst;
}

void
SettingsDialog::accept()
{
    if(QFileInfo(m_startupWidget->startupPath()).isDir())
        Configuration::settings()->setValue("startPath",m_startupWidget->startupPath());
    Configuration::settings()->setValue("hideTabBarWhenOnlyOne",m_behWidget->m_hideTabBar->isChecked());
    Configuration::settings()->setValue("useSystemIcons", m_behWidget->m_useCustomIcons->isChecked());
    Configuration::settings()->setValue("docks.lock", m_startupWidget->locked() );
    Configuration::settings()->setValue("smoothScroll", m_viewWidget->m_smoothScroll->isChecked());
    Configuration::settings()->setValue("showThumbs", m_viewWidget->m_showThumbs->isChecked());
    Configuration::settings()->setValue("drawDevUsage", m_behWidget->m_drawDevUsage->isChecked());
    Configuration::settings()->setValue("textWidth", m_viewWidget->m_iconWidth->value());
    Configuration::settings()->setValue("detailsView.rowPadding", m_viewWidget->m_rowPadding->value());
    Configuration::settings()->setValue("start.view", m_viewWidget->m_viewBox->currentIndex());
    Configuration::settings()->setValue("iconView.iconSize", m_viewWidget->m_iconSlider->value());
    Configuration::readConfig();
    emit settingsChanged();
    QDialog::accept();
}
