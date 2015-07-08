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
#include "interfaces.h"
#include <QGroupBox>
#include <QFileDialog>
#include <QToolButton>

using namespace DFM;

BehaviourWidget::BehaviourWidget(QWidget *parent)
    : QWidget(parent)
    , m_hideTabBar(new QCheckBox(tr("Hide tab bar when only one tab"), this))
    , m_useCustomIcons(new QCheckBox(tr("Use system icons for actions (Unix only)"),this))
    , m_drawDevUsage(new QCheckBox(tr("Draw usage info on mounted devices"), this))
    , m_tabsBox(new QGroupBox(tr("Tabs On Top"), this))
    , m_tabShape(new QComboBox(m_tabsBox))
    , m_tabRndns(new QSpinBox(m_tabsBox))
    , m_tabHeight(new QSpinBox(m_tabsBox))
    , m_tabWidth(new QSpinBox(m_tabsBox))
    , m_newTabButton(new QCheckBox(tr("A 'newtab' button next to last tab"), m_tabsBox))
    , m_overlap(new QSpinBox(m_tabsBox))
    , m_layOrder(new QComboBox(m_tabsBox))
    , m_capsConts(new QCheckBox(tr("Use CAPITAL LETTERS on containers in bookmarks")))
    , m_startUpWidget(new StartupWidget(this))
    , m_invActBookm(new QCheckBox(tr("Icon on active bookmark follows textrole"), this))
    , m_invAllBookm(new QCheckBox(tr("All icons in bookmarks follow textrole"), this))
    , m_pathBarPlace(new QComboBox(this))
    , m_useIOQueue(new QCheckBox(tr("Queue IO operations (copy/move/delete)"), this))
    , m_showCloseTabButton(new QCheckBox(tr("Show closebutton for tabs"), this))
{
    m_hideTabBar->setChecked(Store::config.behaviour.hideTabBarWhenOnlyOneTab);
    m_useCustomIcons->setChecked(Store::config.behaviour.systemIcons);
    m_drawDevUsage->setChecked(Store::config.behaviour.devUsage);
    m_capsConts->setChecked(Store::config.behaviour.capsContainers);
    m_invActBookm->setChecked(Store::config.behaviour.invActBookmark);
    m_invAllBookm->setChecked(Store::config.behaviour.invAllBookmarks);
    m_useIOQueue->setChecked(Store::config.behaviour.useIOQueue);
    m_showCloseTabButton->setChecked(Store::config.behaviour.showCloseTabButton);
    m_startUpWidget->setStartupPath(Store::settings()->value("startPath").toString());

    m_tabsBox->setCheckable(true);
    m_tabsBox->setChecked(Store::config.behaviour.gayWindow);
    const QString &toolTip = tr("A custom titlebar that shows window control buttons "
                                "(min|max|close), tabs and a menubutton on top of the window. "
                                "This shapes the corners of the window to be slightly rounded "
                                "and removes any window decoration that usually shows these controls. "
                                "Changes here will <b>only take effect</b> after restarting dfm. "
                                "It is recommended to restart dfm after changing anything here. ");

    m_tabsBox->setToolTip(toolTip);

//    config.behaviour.frame = settings()->value("behaviour.gayWindow.frame", false).toBool();

//    config.behaviour.invertedColors = settings()->value("behaviour.gayWindow.invertedColors", false).toBool();

    const Qt::Alignment left = Qt::AlignLeft|Qt::AlignVCenter,
            right = Qt::AlignRight|Qt::AlignVCenter;

#define LABEL(_TEXT_) new QLabel(tr(_TEXT_), m_tabsBox)
    QGridLayout *tabsLay = new QGridLayout(m_tabsBox);
    m_tabShape->addItems(QStringList() << "Standard" << "Chrome");
    m_tabShape->setCurrentIndex(Store::config.behaviour.tabShape);
    int row = -1;
    tabsLay->addWidget(LABEL("Shape of tabs:"), ++row, 0, right);
    tabsLay->addWidget(m_tabShape, row, 1, left);

    tabsLay->addWidget(LABEL("TabRoundness:"), ++row, 0, right);
    m_tabRndns->setRange(0, 12);
    m_tabRndns->setValue(qBound(0, Store::config.behaviour.tabRoundness, 12));
    tabsLay->addWidget(m_tabRndns, row, 1, left);

    tabsLay->addWidget(LABEL("TabHeight:"), ++row, 0, right);
    m_tabHeight->setRange(0, 36);
    m_tabHeight->setValue(qBound(0, Store::config.behaviour.tabHeight, 36));
    tabsLay->addWidget(m_tabHeight, row, 1, left);

    tabsLay->addWidget(LABEL("TabWidth:"), ++row, 0, right);
    m_tabWidth->setRange(50, 300);
    m_tabWidth->setValue(qBound(50, Store::config.behaviour.tabWidth, 300));
    tabsLay->addWidget(m_tabWidth, row, 1, left);

    m_newTabButton->setCheckable(true);
    m_newTabButton->setChecked(Store::config.behaviour.newTabButton);
    tabsLay->addWidget(m_newTabButton, ++row, 0, 1, 2, Qt::AlignCenter);

    tabsLay->addWidget(LABEL("Visual Overlapping for tabs:"), ++row, 0, right);
    m_overlap->setRange(6, 12);
    m_overlap->setValue(qBound(6, Store::config.behaviour.tabOverlap, 12));
    tabsLay->addWidget(m_overlap, row, 1, left);

    tabsLay->addWidget(LABEL("Layout order:"), ++row, 0, right);
    m_layOrder->addItems(QStringList() << "MacLike" << "WindowsLike");
    m_layOrder->setCurrentIndex(Store::config.behaviour.windowsStyle);
    tabsLay->addWidget(m_layOrder, row, 1, left);


#undef LABEL

    m_pathBarPlace->addItems(QStringList() << "Above views" << "Below views");
    m_pathBarPlace->setCurrentIndex(Store::config.behaviour.pathBarPlace);

    QGridLayout *gl = new QGridLayout(this);
    row = -1;
    gl->addWidget(m_startUpWidget, ++row, 0, 1, 2);
    gl->addWidget(m_hideTabBar, ++row, 0, 1, 2);
    gl->addWidget(m_useCustomIcons, ++row, 0, 1, 2);
    gl->addWidget(m_drawDevUsage, ++row, 0, 1, 2);
    gl->addWidget(m_invActBookm, ++row, 0, 1, 2);
    gl->addWidget(m_invAllBookm, ++row, 0, 1, 2);
    gl->addWidget(m_capsConts, ++row, 0, 1, 2);
    gl->addWidget(m_useIOQueue, ++row, 0, 1, 2);
    gl->addWidget(m_showCloseTabButton, ++row, 0, 1, 2);
    gl->addWidget(new QLabel(tr("PathBar position:")), ++row, 0, 1, 1);
    gl->addWidget(m_pathBarPlace, row, 1, 1, 1);
    gl->addWidget(m_tabsBox, ++row, 0, 1, 2);
    gl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), ++row, 0);
    setLayout(gl);
}


/////////////////////////////////////////////////////////////////


StartupWidget::StartupWidget(QWidget *parent)
    : QGroupBox(parent)
    , m_sheetEdit(new QLineEdit(Store::config.styleSheet, this))
    , m_startPath(new QLineEdit(this))
{
    setTitle(tr("Startup"));

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(new QLabel(tr("Path to load at startup:"), this));
    hLayout->addStretch();
    hLayout->addWidget(m_startPath);

    QToolButton *startBtn = new QToolButton(this);
    startBtn->setText("<-");
    connect (startBtn, SIGNAL(clicked()), this, SLOT(getStartPath()));

    hLayout->addWidget(startBtn);

    QHBoxLayout *sheetLo = new QHBoxLayout();
    sheetLo->addWidget(new QLabel(tr("Path to stylesheet to apply:")));
    sheetLo->addStretch();
    sheetLo->addWidget(m_sheetEdit);

    QToolButton *sheetBtn = new QToolButton(this);
    sheetBtn->setText("<-");
    connect (sheetBtn, SIGNAL(clicked()), this, SLOT(getShitPath()));

    sheetLo->addWidget(sheetBtn);

    QVBoxLayout *vl = new QVBoxLayout();
    vl->addLayout(hLayout);
    vl->addLayout(sheetLo);
//    vl->addLayout(defView);
    vl->addStretch();
    setLayout(vl);
}

void
StartupWidget::getShitPath()
{
    QString styleSheet = QFileDialog::getOpenFileName(window(), tr("select stylesheet"), QDir::homePath(), tr("stylesheets (*.css *.qss);"));
    if (!styleSheet.isEmpty())
        m_sheetEdit->setText(styleSheet);
}

void
StartupWidget::getStartPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty())
        m_startPath->setText(dir);
}


/////////////////////////////////////////////////////////////////

ViewsWidget::ViewsWidget(QWidget *parent) : QWidget(parent)
  , m_iconWidth(new QSlider(Qt::Horizontal, this))
  , m_width(new QLabel(this))
  , m_showThumbs(new QGroupBox(tr("Show thumbnails"), this))
  , m_rowPadding(new QSpinBox(this))
  , m_iconSlider(new QSlider(Qt::Horizontal, this))
  , m_size(new QLabel(QString::number(Store::config.views.iconView.iconSize*16) + " px", this))
  , m_viewBox(new QComboBox(this))
  , m_singleClick(new QCheckBox(tr("Open folders and files with one click"), this))
  , m_lineCount(new QSpinBox(this))
  , m_dirSettings(new QCheckBox(tr("Store view for each dir separately (unix only)"), this))
  , m_categorized(new QCheckBox(tr("Show categorized"), this))
  , m_colWidth(new QSpinBox(this))
  , m_altRows(new QCheckBox(tr("Render rows with alternating colors"), this))
{
    m_categorized->setChecked(Store::config.views.iconView.categorized);
    m_showThumbs->setChecked(Store::config.views.showThumbs);
    m_singleClick->setChecked(Store::config.views.singleClick);
    m_dirSettings->setChecked(Store::config.views.dirSettings);
    m_showThumbs->setEnabled(dApp->hasThumbIfaces());
    m_showThumbs->setCheckable(true);

    QVBoxLayout *tL = new QVBoxLayout(m_showThumbs);
    if (dApp->hasThumbIfaces())
    {
        foreach (ThumbInterface *plugin, dApp->thumbIfaces())
        {
            QCheckBox *box = new QCheckBox(plugin->name(), m_showThumbs);
            box->setProperty("pluginName", plugin->name());
            box->setChecked(dApp->isActive(plugin));
            box->setToolTip(plugin->description());
            tL->addWidget(box);
        }
    }

    //IconView
    m_iconSlider->setRange(1, 16);
    m_iconSlider->setSingleStep(1);
    m_iconSlider->setPageStep(1);
    m_iconSlider->setValue(Store::config.views.iconView.iconSize);
    connect(m_iconSlider, SIGNAL(valueChanged(int)), this, SLOT(sizeChanged(int)));

    m_iconWidth->setRange(1, 32);
    m_iconWidth->setSingleStep(1);
    m_iconWidth->setPageStep(1);
    m_iconWidth->setValue(Store::config.views.iconView.textWidth);
    m_width->setText(QString::number(Store::config.views.iconView.textWidth*2) + " px");
    connect (m_iconWidth, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));

    m_lineCount->setRange(1, 8);
    m_lineCount->setValue(qBound(1, Store::config.views.iconView.lineCount, 8));

    QGroupBox *gBox = new QGroupBox(tr("IconView"), this);
    QGridLayout *iconLay = new QGridLayout(gBox);

    int row = -1;
    const Qt::Alignment left = Qt::AlignLeft|Qt::AlignVCenter,
            right = Qt::AlignRight|Qt::AlignVCenter;

    iconLay->setColumnStretch(0, 50);
    iconLay->setColumnStretch(1, 50);
//    iconLay->setColumnStretch(2, 50);

    iconLay->addWidget(m_categorized, ++row, 0, 1, 3, left);

    iconLay->addWidget(new QLabel(tr("Extra width added to text:"), this), ++row, 0, right);
    iconLay->addWidget(m_iconWidth, row, 1, right);
    iconLay->addWidget(m_width, row, 2, right);

    iconLay->addWidget(new QLabel(tr("Text lines:"), gBox), ++row, 0, right);
    iconLay->addWidget(m_lineCount, row, 1, 1, 2, right);


    //DetailsView
    m_rowPadding->setMinimum(0);
    m_rowPadding->setMaximum(5);
    m_rowPadding->setValue(Store::config.views.detailsView.rowPadding);

    m_altRows->setChecked(Store::config.views.detailsView.altRows);

    QGroupBox *detailsBox = new QGroupBox(tr("DetailsView"), this);
    QGridLayout *detailLay = new QGridLayout(detailsBox);
    row = -1;
    detailLay->addWidget(m_altRows, ++row, 0, right);
    detailLay->addWidget(new QLabel(tr("Padding added to rowheight:"), detailsBox), ++row, 0, right);
    detailLay->addWidget(m_rowPadding, row, 1, right);

    //ColumnsView
    m_colWidth->setMinimum(64);
    m_colWidth->setMaximum(1024);
    m_colWidth->setValue(Store::config.views.columnsView.colWidth);

    QGroupBox *colsBox = new QGroupBox(tr("ColumnsView"), this);
    QGridLayout *colLay = new QGridLayout(colsBox);
    row = -1;
    colLay->addWidget(new QLabel(tr("Default size of columns:"), colsBox), ++row, 0, right);
    colLay->addWidget(m_colWidth, row, 1, right);

    m_viewBox->insertItems(0, QStringList() << "Icons" << "Details" << "Columns" << "Flow");
    m_viewBox->setCurrentIndex(Store::config.behaviour.view);
    QHBoxLayout *defView = new QHBoxLayout();
    defView->addWidget(new QLabel(tr("Default view:"), this));
    defView->addStretch();
    defView->addWidget(m_viewBox);

    QHBoxLayout *iconSize = new QHBoxLayout();
    iconSize->addWidget(new QLabel(tr("Default size for icons:"), this));
    iconSize->addStretch();
    iconSize->addWidget(m_iconSlider);
    iconSize->addWidget(m_size);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->addWidget(m_showThumbs);
    vLayout->addWidget(m_singleClick);
    vLayout->addWidget(m_dirSettings);

    vLayout->addLayout(defView);
    vLayout->addLayout(iconSize);
    vLayout->addWidget(gBox);
    vLayout->addWidget(detailsBox);
    vLayout->addWidget(colsBox);
    vLayout->addStretch();
    setLayout(vLayout);
}

/////////////////////////////////////////////////////////////////

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    QTabWidget *tabWidget = new QTabWidget(this);
    m_behWidget = new BehaviourWidget(tabWidget);
    m_viewWidget = new ViewsWidget(tabWidget);
    m_ok = new QPushButton(this);
    m_cancel = new QPushButton(this);

    setWindowTitle(tr("Configure DFM"));

    tabWidget->addTab(m_behWidget, tr("Behaviour"));
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

    connect(m_ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void
SettingsDialog::accept()
{
    if (QFileInfo(m_behWidget->m_startUpWidget->startupPath()).isDir())
        Store::config.startPath = m_behWidget->m_startUpWidget->startupPath();
    if (QFileInfo(m_behWidget->m_startUpWidget->m_sheetEdit->text()).isReadable()
            &&  (QFileInfo(m_behWidget->m_startUpWidget->m_sheetEdit->text()).suffix() == "css"
                  || QFileInfo(m_behWidget->m_startUpWidget->m_sheetEdit->text()).suffix() == "qss"))
    {
        Store::config.styleSheet = m_behWidget->m_startUpWidget->m_sheetEdit->text();
        QFile file(DFM::Store::config.styleSheet);
        file.open(QFile::ReadOnly);
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
    else
    {
        Store::config.styleSheet = QString();
        qApp->setStyleSheet(QString());
    }

    foreach (QCheckBox *box, m_viewWidget->m_showThumbs->findChildren<QCheckBox *>())
    {
        if (box->isChecked())
            dApp->activateThumbInterface(box->property("pluginName").toString());
        else
            dApp->deActivateThumbInterface(box->property("pluginName").toString());
    }

    Store::config.behaviour.hideTabBarWhenOnlyOneTab = m_behWidget->m_hideTabBar->isChecked();
    Store::config.behaviour.systemIcons = m_behWidget->m_useCustomIcons->isChecked();
    Store::config.views.showThumbs = m_viewWidget->m_showThumbs->isChecked();
    Store::config.behaviour.devUsage = m_behWidget->m_drawDevUsage->isChecked();
    Store::config.views.iconView.textWidth = m_viewWidget->m_iconWidth->value();
    Store::config.views.detailsView.rowPadding = m_viewWidget->m_rowPadding->value();
    Store::config.views.columnsView.colWidth = m_viewWidget->m_colWidth->value();
    Store::config.behaviour.view = m_viewWidget->m_viewBox->currentIndex();
    Store::config.views.iconView.iconSize = m_viewWidget->m_iconSlider->value();
    Store::config.views.singleClick = m_viewWidget->m_singleClick->isChecked();
    Store::config.views.iconView.lineCount = m_viewWidget->m_lineCount->value();
    Store::config.behaviour.capsContainers = m_behWidget->m_capsConts->isChecked();
    Store::config.views.dirSettings = m_viewWidget->m_dirSettings->isChecked();
    Store::config.behaviour.invActBookmark = m_behWidget->m_invActBookm->isChecked();
    Store::config.behaviour.invAllBookmarks = m_behWidget->m_invAllBookm->isChecked();
    Store::config.views.iconView.categorized = m_viewWidget->m_categorized->isChecked();
    Store::config.views.detailsView.altRows = m_viewWidget->m_altRows->isChecked();
    Store::config.behaviour.pathBarPlace = m_behWidget->m_pathBarPlace->currentIndex();
    Store::config.behaviour.useIOQueue = m_behWidget->m_useIOQueue->isChecked();
    Store::config.behaviour.showCloseTabButton = m_behWidget->m_showCloseTabButton->isChecked();

    Store::settings()->setValue("behaviour.useIOQueue", Store::config.behaviour.useIOQueue);
    Store::settings()->setValue("behaviour.gayWindow", m_behWidget->m_tabsBox->isChecked());
    Store::settings()->setValue("behaviour.gayWindow.tabShape", m_behWidget->m_tabShape->currentIndex());
    Store::settings()->setValue("behaviour.gayWindow.tabRoundness", m_behWidget->m_tabRndns->value());
    Store::settings()->setValue("behaviour.gayWindow.tabHeight", m_behWidget->m_tabHeight->value());
    Store::settings()->setValue("behaviour.gayWindow.tabWidth", m_behWidget->m_tabWidth->value());
//    Store::settings()->setValue("behaviour.gayWindow.frame", ???);
    Store::settings()->setValue("behaviour.gayWindow.newTabButton", m_behWidget->m_newTabButton->isChecked());
    Store::settings()->setValue("behaviour.gayWindow.tabOverlap", m_behWidget->m_overlap->value());
    Store::settings()->setValue("behaviour.gayWindow.windowsStyle", (bool)m_behWidget->m_layOrder->currentIndex());
//    Store::settings()->setValue("behaviour.gayWindow.invertedColors", ???);

    if (MainWindow *mw = MainWindow::currentWindow())
        mw->updateConfig();

    QDialog::accept();
}
