#include "MainWindow.h"

#include "PluginsList.h"
#include "FavoritesList.h"

#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>
#include <QScopedArrayPointer>
#include <QScrollArea>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QPixmap>
#include <QPolygon>
#include <QColor>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QInputDialog>
#include <QSettings>
#include <QDockWidget>

#include "CustomSettings.h"
#include "GraphDisplay.h"
#include "PipelineIE.h"
#include "SeekSlider.h"

#include "version_info.h"

#include <gst/gst.h>

MainWindow::MainWindow (QWidget *parent, Qt::WindowFlags flags)
: QMainWindow (parent, flags),
m_pGraph (new GraphManager)
{
  QToolBar *ptb = addToolBar ("Menu");

  QAction *pactAdd = ptb->addAction ("Add...");
  pactAdd->setShortcut (QKeySequence ("Ctrl+F"));
  connect (pactAdd, SIGNAL (triggered ()), SLOT (AddPlugin ()));

  QAction *pactOpenFile = ptb->addAction ("Open Media File...");
  connect (pactOpenFile, SIGNAL (triggered ()), SLOT (OpenMediaFile ()));

  ptb->addSeparator ();

  QPixmap pxPlay (24, 24);
  pxPlay.fill (QColor (0, 0, 0, 0));
  QPainter pntrPlay (&pxPlay);
  pntrPlay.setPen (Qt::darkGreen);
  pntrPlay.setBrush (QBrush (Qt::darkGreen));

  QPolygon polygon (3);
  polygon.setPoint (0, 4, 4);
  polygon.setPoint (1, 4, 20);
  polygon.setPoint (2, 20, 12);

  pntrPlay.drawPolygon (polygon, Qt::WindingFill);

  QAction *pactPlay = ptb->addAction (QIcon (pxPlay), "Play");
  connect (pactPlay, SIGNAL (triggered ()), SLOT (Play ()));

  QPixmap pxPause (24, 24);
  pxPause.fill (QColor (0, 0, 0, 0));
  QPainter pntrPause (&pxPause);
  pntrPause.setPen (Qt::darkGray);
  pntrPause.setBrush (QBrush (Qt::darkGray));

  pntrPause.drawRect (8, 4, 3, 16);
  pntrPause.drawRect (13, 4, 3, 16);

  QAction *pactPause = ptb->addAction (QIcon (pxPause), "Pause");
  connect (pactPause, SIGNAL (triggered ()), SLOT (Pause ()));

  QPixmap pxStop (24, 24);
  pxStop.fill (QColor (0, 0, 0, 0));
  QPainter pntrStop (&pxStop);
  pntrStop.setPen (Qt::darkRed);
  pntrStop.setBrush (QBrush (Qt::darkRed));

  pntrStop.drawRect (6, 6, 12, 12);

  QAction *pactStop = ptb->addAction (QIcon (pxStop), "Stop");
  connect (pactStop, SIGNAL (triggered ()), SLOT (Stop ()));

  QPixmap pxFulsh (24, 24);
  pxFulsh.fill (QColor (0, 0, 0, 0));
  QPainter pntrFlush (&pxFulsh);
  pntrFlush.setPen (Qt::darkGreen);
  pntrFlush.setBrush (QBrush (Qt::darkGreen));

  pntrFlush.drawRect (3, 4, 3, 16);

  polygon = QPolygon (3);
  polygon.setPoint (0, 9, 4);
  polygon.setPoint (1, 9, 20);
  polygon.setPoint (2, 21, 12);

  pntrFlush.drawPolygon (polygon, Qt::WindingFill);

  QAction *pactFlush = ptb->addAction (QIcon (pxFulsh), "Flush");
  connect (pactFlush, SIGNAL (triggered ()), SLOT (Flush ()));

  QAction *pactClear = ptb->addAction ("Clear");
  connect (pactClear, SIGNAL (triggered ()), SLOT (ClearGraph ()));
  ptb->addSeparator ();

  m_pslider = new SeekSlider ();
  m_pslider->setOrientation (Qt::Horizontal);
  m_pslider->setRange (0, 10000);
  m_pslider->setTracking (false);

  connect(m_pslider, SIGNAL(valueChanged(int)), SLOT(Seek(int)));
  ptb->addWidget (m_pslider);

  m_menu = menuBar ()->addMenu ("&File");

  QAction *pactOpen = m_menu->addAction ("Open...", this, SLOT (Open ()),
                                        QKeySequence::Open);
  addAction (pactOpen);

  QAction *pactOpenMediaFile = m_menu->addAction ("Open Media File...", this,
                                                 SLOT (OpenMediaFile ()),
                                                 QKeySequence::Open);
  addAction (pactOpenMediaFile);

  QAction *pactSave = m_menu->addAction ("Save", this, SLOT (Save ()),
                                        QKeySequence::Save);
  addAction (pactSave);

  QAction *pactSaveAs = m_menu->addAction ("Save As...", this, SLOT (SaveAs ()),
                                          QKeySequence::SaveAs);
  addAction (pactSaveAs);

  m_menu->addSeparator ();
  m_menu->addAction ("Exit", this, SLOT (close ()));

  m_menu = menuBar ()->addMenu ("&Graph");

  m_menu->addAction (pactAdd);
  m_menu->addAction (pactOpenMediaFile);
  m_menu->addAction ("Open Media Uri...", this, SLOT (OpenMediaUri ()));
  m_menu->addSeparator ();
  m_menu->addAction (pactPlay);
  m_menu->addAction (pactPause);
  m_menu->addAction (pactStop);
  m_menu->addAction (pactFlush);
  m_menu->addSeparator ();
  m_menu->addAction (pactClear);

  m_menu = menuBar ()->addMenu ("&Help");

  m_menu->addAction ("About pipeviz...", this, SLOT (About ()));

  m_pGraphDisplay = new GraphDisplay(this);
  connect(m_pGraphDisplay, SIGNAL(signalAddPlugin()),
                    this, SLOT(AddPlugin()));
  connect(m_pGraphDisplay, SIGNAL(signalClearGraph()),
                    this, SLOT(ClearGraph()));

  QScrollArea *pscroll = new QScrollArea;
  pscroll->setWidget (m_pGraphDisplay);
  pscroll->setWidgetResizable (false);
  m_pGraphDisplay->resize (10000, 10000);
  m_pGraphDisplay->m_pGraph = m_pGraph;
  setCentralWidget (pscroll);
  m_pstatusBar = new QStatusBar;
  setStatusBar (m_pstatusBar);
  m_pluginListDlg = new PluginsListDialog (this);
  m_pluginListDlg->setModal (false);

  connect(m_pluginListDlg, SIGNAL(signalAddPluginToFav(const QString&)),
                    this, SLOT(AddPluginToFavorites(const QString&)));
  connect(m_pluginListDlg, SIGNAL(signalRemPluginToFav(const QString&)),
                    this, SLOT(RemovePluginToFavorites(const QString&)));


  restoreGeometry (CustomSettings::mainWindowGeometry ());
  createDockWindows();

  Logger::instance().start();
  connect(&Logger::instance(), SIGNAL(sendLog(const QString &, int)),
                  this, SLOT(InsertLogLine(const QString &, int)));

  LOG_INFO("Mainwindow is now initialized");

  startTimer (100);
}

void MainWindow::createDockWindows()
{
    /* create the log list window */
    QDockWidget *dock = new QDockWidget(tr("logs"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_logList = new QListWidget(dock);
    dock->setWidget(m_logList);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    m_menu->addAction(dock->toggleViewAction());

    /*create the favorite list window */
    dock = new QDockWidget(tr("favorite list"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_favoriteList = new FavoritesList(dock);
    dock->setWidget(m_favoriteList);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    m_menu->addAction(dock->toggleViewAction());
    connect(m_favoriteList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this, SLOT(onFavoriteListItemDoubleClicked(QListWidgetItem*)));
    m_favoriteList->setContextMenuPolicy(Qt::CustomContextMenu);
     connect(m_favoriteList,SIGNAL(customContextMenuRequested(const QPoint &)),
     this,SLOT(ProvideContextMenu(const QPoint &)));

}
FavoritesList* MainWindow::getFavoritesList()
{
  return m_favoriteList;
}

void MainWindow::AddPluginToFavorites(const QString& plugin_name)
{
    m_favoriteList->addFavorite(plugin_name);
}

void MainWindow::RemovePluginToFavorites(const QString& plugin_name)
{
  m_favoriteList->removeFavorite(plugin_name);
}

void MainWindow::onFavoriteListItemDoubleClicked(QListWidgetItem* pitem)
{
  LOG_INFO("onFavoriteListItemDoubleClicked: %s", pitem->text().toStdString ().c_str ());
  if (!m_pGraph
    || !m_pGraph->AddPlugin (pitem->text ().toStdString ().c_str (), NULL)) {
      QMessageBox::warning (
      this, "Plugin addition problem",
      "Plugin `" + pitem->text () + "` insertion was FAILED");
      LOG_INFO("Plugin '%s' insertion FAILED",  pitem->text ().toStdString ().c_str ());
      return;
    }
}

void MainWindow::ProvideContextMenu(const QPoint &pos)
{
  QPoint item = m_favoriteList->mapToGlobal(pos);
  QMenu submenu;
  submenu.addAction("Delete");
  QAction* rightClickItem = submenu.exec(item);
  if (rightClickItem && rightClickItem->text().contains("Delete") ) {
    LOG_INFO("Delete item: %s", m_favoriteList->currentItem()->text().toStdString ().c_str ());
    RemovePluginToFavorites(m_favoriteList->currentItem()->text());
  }
}

void MainWindow::InsertLogLine(const QString& line, int category)
{
  QListWidgetItem* pItem =new QListWidgetItem(line);
  switch(category) {
    case eLOG_CATEGORY_INTERNAL:
      pItem->setForeground(Qt::blue);
      break;
    case eLOG_CATEGORY_GST:
      pItem->setForeground(Qt::red);
      break;
    default:
      pItem->setForeground(Qt::black);
  }
  m_logList->addItem(pItem);
}

MainWindow& MainWindow::instance()
{
  static MainWindow instance;
  return instance;
}

MainWindow::~MainWindow ()
{
  CustomSettings::saveMainWindowGeometry (saveGeometry ());
  Logger::instance().Quit();
  delete m_pluginListDlg;
}

void
MainWindow::AddPlugin ()
{
  m_pluginListDlg->setGraph (m_pGraph.data ());

  m_pluginListDlg->raise ();
  m_pluginListDlg->show ();
  std::vector<ElementInfo> info = m_pGraph->GetInfo ();
  m_pGraphDisplay->update (info);
}

void
MainWindow::OpenMediaFile ()
{
  QString dir = CustomSettings::lastIODirectory ();

  QString path = QFileDialog::getOpenFileName (this, "Open File...", dir);
  if (!path.isEmpty ()) {
    gchar *uri = gst_filename_to_uri (path.toStdString ().c_str (), NULL);
    if (uri) {
      LOG_INFO("Open Source file: %s", path.toStdString ().c_str ());

      m_pGraph->OpenUri (uri, NULL);
      g_free (uri);

      std::vector<ElementInfo> info = m_pGraph->GetInfo ();
      m_pGraphDisplay->update (info);

      QString dir = QFileInfo (path).absoluteDir ().absolutePath ();
      CustomSettings::saveLastIODirectory (dir);
    }
  }
}

void
MainWindow::OpenMediaUri ()
{
  QString uri = QInputDialog::getText (this, "Open Uri...", "Uri:");

  if (!uri.isEmpty ()) {
    LOG_INFO("Open uri: %s", uri.toStdString ().c_str ());
    m_pGraph->OpenUri (uri.toStdString ().c_str (), NULL);

    std::vector<ElementInfo> info = m_pGraph->GetInfo ();
    m_pGraphDisplay->update (info);
  }
}

void
MainWindow::Play ()
{
  LOG_INFO( "Play");
  m_pGraph->Play ();
}

void
MainWindow::Pause ()
{
  LOG_INFO("Pause");
  m_pGraph->Pause ();
}

void
MainWindow::Stop ()
{
  LOG_INFO("Stop");
  m_pGraph->Stop ();
}

void
MainWindow::Flush ()
{
  LOG_INFO("Flush");

  if (m_pGraph->m_pGraph) {
    gst_element_send_event (GST_ELEMENT (m_pGraph->m_pGraph),
                            gst_event_new_flush_start ());
#if GST_VERSION_MAJOR >= 1
    gst_element_send_event(GST_ELEMENT(m_pGraph -> m_pGraph), gst_event_new_flush_stop(true));
#else
    gst_element_send_event (GST_ELEMENT (m_pGraph->m_pGraph),
                            gst_event_new_flush_stop ());
#endif
  }
}

void
MainWindow::ClearGraph ()
{
  LOG_INFO("ClearGraph");
  PipelineIE::Clear (m_pGraph);
}

void
MainWindow::Seek (int val)
{
  if (m_pGraph->SetPosition ((double) (val) / m_pslider->maximum ()))
    LOG_INFO("Seek to %d", val);
  else
    LOG_INFO("Seek FAILED");
}

void
MainWindow::timerEvent (QTimerEvent *)
{
  GstState state;
  GstStateChangeReturn res = gst_element_get_state (m_pGraph->m_pGraph, &state,
  NULL,
                                                    GST_MSECOND);

  if (res == GST_STATE_CHANGE_SUCCESS) {
    QString str;
    switch (state) {
      case GST_STATE_VOID_PENDING:
        str = "Pending";
        break;
      case GST_STATE_NULL:
        str = "Null";
        break;
      case GST_STATE_READY:
        str = "Ready";
        break;
      case GST_STATE_PAUSED:
        str = "Paused";
        break;
      case GST_STATE_PLAYING:
        str = "Playing";
        break;
    };

    m_pstatusBar->showMessage (str);
  }
  else {
    m_pstatusBar->showMessage (
    QString (gst_element_state_change_return_get_name (res)));
  }

  double pos = m_pGraph->GetPosition ();

  if (m_pslider->value () != (int) (m_pslider->maximum () * pos))
    m_pslider->setSliderPosition (m_pslider->maximum () * pos);

  m_pGraphDisplay->update (m_pGraph->GetInfo ());
}

void
MainWindow::Save ()
{
  if (m_fileName.isEmpty ())
    SaveAs ();
  else {
    QFileInfo fileInfo (m_fileName);
    if (fileInfo.completeSuffix ().isEmpty ()
    || fileInfo.completeSuffix () != "gpi")
      m_fileName = m_fileName + ".gpi";

    PipelineIE::Export (m_pGraph, m_fileName);

  }
}

void
MainWindow::SaveAs ()
{
  QString dir = CustomSettings::lastIODirectory ();

  QString path = QFileDialog::getSaveFileName (this, "Save As...", dir,
                                               tr ("*.gpi"));

  if (!path.isEmpty ()) {
    m_fileName = path;
    Save ();

    QString dir = QFileInfo (path).absoluteDir ().absolutePath ();
    CustomSettings::saveLastIODirectory (dir);
  }
}

void
MainWindow::Open ()
{
  QString dir = CustomSettings::lastIODirectory ();

  QString path = QFileDialog::getOpenFileName (
  this, "Open...", dir, tr ("GPI (*.gpi *.xpm);;All files (*.*)"));

  if (!path.isEmpty ()) {
    if (PipelineIE::Import (m_pGraph, path))
      m_fileName = path;

    QString dir = QFileInfo (path).absoluteDir ().absolutePath ();
    CustomSettings::saveLastIODirectory (dir);
  }
}

void
MainWindow::About ()
{
  QString message;
  message = "<center><b>pipeviz</b></center><br>";
  message = "<center>virinext@gmail.com</center><br>";
  message += QString ("<center>Version: ") + VERSION_STR + "</center><br>";
  message += "<center>GUI Based on Qt</center>";
  message += "<center>using ";
  message += gst_version_string ();
  message += "</center>";

  QMessageBox::about (this, "About", message);
}
