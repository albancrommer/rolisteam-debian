/*************************************************************************
 *        Copyright (C) 2007 by Romain Campioni                          *
 *        Copyright (C) 2009 by Renaud Guezennec                         *
 *        Copyright (C) 2010 by Berenger Morel                           *
 *        Copyright (C) 2010 by Joseph Boudou                            *
 *                                                                       *
 *        http://www.rolisteam.org/                                      *
 *                                                                       *
 *   rolisteam is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published   *
 *   by the Free Software Foundation; either version 2 of the License,   *
 *   or (at your option) any later version.                              *
 *                                                                       *
 *   This program is distributed in the hope that it will be useful,     *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 *   GNU General Public License for more details.                        *
 *                                                                       *
 *   You should have received a copy of the GNU General Public License   *
 *   along with this program; if not, write to the                       *
 *   Free Software Foundation, Inc.,                                     *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           *
 *************************************************************************/
#include <QApplication>
#include <QBitmap>
#include <QBuffer>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStringBuilder>
#include <QTime>
#include <QUrl>
#include <QUuid>
#include <QStatusBar>
#include <QCommandLineParser>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "toolsbar.h"
#include "map/mapframe.h"
#include "map/map.h"
#include "chat/chatlistwidget.h"
#include "network/networkmanager.h"
#include "Image.h"
#include "network/networkmessagewriter.h"
#include "charactersheet/charactersheet.h"
#include "data/person.h"
#include "data/player.h"
#include "userlist/playersList.h"
#include "userlist/playersListWidget.h"
#include "preferences/preferencesdialog.h"
#include "services/updatechecker.h"
#include "improvedworkspace.h"
#include "data/mediacontainer.h"
#include "network/receiveevent.h"

//Text editor
#include "textedit.h"
#include "widgets/aboutrolisteam.h"
#include "widgets/gmtoolbox/UnitConvertor/convertor.h"
#include "widgets/gmtoolbox/NameGenerator/namegeneratorwidget.h"
//VMAP
#include "vmap/vmapframe.h"
#include "vmap/vmap.h"
#include "vmap/vmapwizzarddialog.h"
//character sheet
#include "charactersheet/charactersheetwindow.h"
//session
#include "session/sessionmanager.h"
#ifndef NULL_PLAYER
#include "audioPlayer.h"
#endif

// singleton to the mainwindow
MainWindow* MainWindow::m_singleton= nullptr;

MainWindow::MainWindow()
    : QMainWindow(),
      m_networkManager(nullptr),
      m_ui(new Ui::MainWindow),
      m_resetSettings(false),
      m_currentConnectionProfile(nullptr),
      m_profileDefined(false),
      m_currentStory(nullptr),
      m_preferencesDialog(nullptr)
{
    setAcceptDrops(true);
    m_profileDefined = false;

    m_ui->setupUi(this);
    m_shownProgress=false;

    m_preferences = PreferencesManager::getInstance();
    m_downLoadProgressbar = new QProgressBar();
    m_downLoadProgressbar->setRange(0,100);

    m_downLoadProgressbar->setVisible(false);
    m_networkManager = new NetworkManager();
    m_vmapToolBar = new VmapToolBar();
    addToolBar(Qt::TopToolBarArea,m_vmapToolBar);

    connect(m_networkManager,SIGNAL(notifyUser(QString)),this,SLOT(notifyUser(QString)));
    m_ipChecker = new IpChecker(this);
    m_mapAction = new QMap<MediaContainer*,QAction*>();

    m_sessionManager = new SessionManager();

    connect(m_sessionManager,SIGNAL(sessionChanged(bool)),this,SLOT(setWindowModified(bool)));
    connect(m_sessionManager,SIGNAL(openFile(CleverURI*,bool)),this,SLOT(openCleverURI(CleverURI*,bool)));

    /// Create all GM toolbox widget
    m_gmToolBoxList.append(new NameGeneratorWidget());
    m_gmToolBoxList.append(new GMTOOL::Convertor());

    foreach (QWidget* wid, m_gmToolBoxList)
    {
        QDockWidget* widDock = new QDockWidget(this);
        widDock->setAllowedAreas(Qt::AllDockWidgetAreas);
        widDock->setWidget(wid);
        widDock->setWindowTitle(wid->windowTitle());
        widDock->setObjectName(wid->objectName());
        addDockWidget(Qt::RightDockWidgetArea, widDock);

        m_ui->m_gmToolBoxMenu->addAction(widDock->toggleViewAction());
        widDock->setVisible(false);
    }

}
MainWindow::~MainWindow()
{
    // delete m_dockLogUtil;
    if(nullptr != m_currentStory)
    {
        delete m_currentStory;
    }
}
void MainWindow::aboutRolisteam()
{
    AboutRolisteam diag(m_version,this);
    diag.exec();
}
void MainWindow::addMediaToMdiArea(MediaContainer* mediac)
{
    CleverURI* uri = mediac->getCleverUri();
    if(nullptr!=uri)
    {
        setLatestFile(uri);
        m_sessionManager->addRessource(mediac->getCleverUri());
        uri->setDisplayed(true);
    }
    QAction *action = m_ui->m_menuSubWindows->addAction(mediac->getTitle());
    action->setCheckable(true);
    action->setChecked(true);

    mediac->setAction(action);
    m_mdiArea->addContainerMedia(mediac);
    m_mapAction->insert(mediac,action);
    mediac->setVisible(true);
    mediac->setFocus();

}
void  MainWindow::closeConnection()
{
    if(nullptr!=m_networkManager)
    {
        m_networkManager->disconnectAndClose();
        m_ui->m_connectionAction->setEnabled(true);
        m_ui->m_disconnectAction->setEnabled(false);
    }
}
void MainWindow::closeAllImagesAndMaps()
{
    for(MediaContainer* tmp : m_mediaHash.values())
    {
        if(nullptr!=tmp)
        {
            closeMediaContainer(tmp->getMediaId());
        }
    }
}
void MainWindow::closeMediaContainer(QString id)
{
     if(m_mediaHash.contains(id))
     {
         MediaContainer* mediaCon = m_mediaHash.value(id);
         if(nullptr!=mediaCon)
         {
             m_mediaHash.remove(id);
             if(CleverURI::VMAP == mediaCon->getContentType())
             {
                 m_vmapToolBar->setCurrentMap(nullptr);
             }
             else if(CleverURI::MAP == mediaCon->getContentType())
             {
                 m_playersListWidget->model()->changeMap(nullptr);
                 m_toolBar->changeMap(nullptr);
             }

             //setUri as undisplayed
             CleverURI* uri = mediaCon->getCleverUri();
             if(nullptr!=uri)
             {
                 uri->setState(CleverURI::Remain);
                 if(!uri->hasData())
                 {
                    m_sessionManager->resourceClosed(uri);//delete the uri
                 }
             }

             //remove action from data and from memory
             QAction* act = m_mapAction->value(mediaCon);
             m_mapAction->remove(mediaCon);
             delete act;
             delete mediaCon;


             if((nullptr!=m_currentConnectionProfile)&&(m_currentConnectionProfile->isGM()))
             {
                 NetworkMessageWriter msg(NetMsg::MediaCategory,NetMsg::closeMedia);
                 msg.string8(id);
                 msg.sendAll();
             }
         }
     }
}

void MainWindow::closeCurrentSubWindow()
{
    QMdiSubWindow* subactive = m_mdiArea->currentSubWindow();
    MediaContainer* container = dynamic_cast<MediaContainer*>(subactive);
    if(nullptr != container)
    {
        closeMediaContainer(container->getMediaId());
    }
}
void MainWindow::checkUpdate()
{
    if(m_preferences->value("MainWindow_MustBeChecked",true).toBool())
    {
        m_updateChecker = new UpdateChecker();
        m_updateChecker->startChecking();
        connect(m_updateChecker,SIGNAL(checkFinished()),this,SLOT(updateMayBeNeeded()));
    }
}
void MainWindow::activeWindowChanged(QMdiSubWindow *subWindow)
{
    if(nullptr!=m_currentConnectionProfile)
    {
        bool localPlayerIsGM = m_currentConnectionProfile->isGM();
        if(nullptr!=subWindow)
        {
            if (subWindow->objectName() == QString("MapFrame") && (localPlayerIsGM))
            {
                m_toolBarStack->setCurrentWidget(m_toolBar);
                m_ui->m_closeAction->setEnabled(true);
                m_ui->m_saveAction->setEnabled(true);
                m_ui->m_saveAsAction->setEnabled(true);
                m_vmapToolBar->setEnabled(false);
                subWindow->setFocus();
            }
            else if(subWindow->objectName() == QString("Image") && (localPlayerIsGM))
            {
                m_playersListWidget->model()->changeMap(nullptr);
                m_ui->m_closeAction->setEnabled(true);
                m_ui->m_saveAction->setEnabled(true);
                m_ui->m_saveAsAction->setEnabled(true);
                m_vmapToolBar->setEnabled(false);
            }
            else if(subWindow->objectName() == QString("VMapFrame") && (localPlayerIsGM))
            {
                m_playersListWidget->model()->changeMap(nullptr);
                m_toolBarStack->setCurrentWidget(m_vToolBar);
                m_vmapToolBar->setEnabled(true);
                m_ui->m_closeAction->setEnabled(true);
                m_ui->m_saveAction->setEnabled(true);
                m_ui->m_saveAsAction->setEnabled(true);

                VMapFrame* frame = dynamic_cast<VMapFrame*>(subWindow);
                m_vmapToolBar->setCurrentMap(frame->getMap());
                m_vToolBar->updateUi(frame->getMap()->getPermissionMode());
            }
            else if(subWindow->objectName() == QString("CharacterSheetViewer") && (localPlayerIsGM))
            {
                m_playersListWidget->model()->changeMap(nullptr);
                m_ui->m_closeAction->setEnabled(true);
                m_ui->m_saveAction->setEnabled(true);
                m_ui->m_saveAsAction->setEnabled(true);
                m_vmapToolBar->setEnabled(false);
            }
        }
        else
        {
            m_ui->m_closeAction->setEnabled(false);
            m_ui->m_saveAction->setEnabled(false);
        }
    }
    else
    {
        m_ui->m_closeAction->setEnabled(false);
        m_ui->m_saveAction->setEnabled(false);
        m_ui->m_saveAsAction->setEnabled(false);
    }
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(mayBeSaved())
    {
        if(nullptr!=m_playerList)
        {
            m_playerList->sendDelLocalPlayer();
        }
        writeSettings();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
void MainWindow::userNatureChange(bool isGM)
{
    if(nullptr!=m_currentConnectionProfile)
    {
        m_currentConnectionProfile->setGm(isGM);
        updateUi();
        updateWindowTitle();
    }
}
NetworkManager* MainWindow::getNetWorkManager()
{
    return m_networkManager;
}
MainWindow* MainWindow::getInstance()
{
    if(NULL==m_singleton)
    {
        m_singleton = new MainWindow();
    }
    return m_singleton;
}
Map::PermissionMode MainWindow::getPermission(int id)
{
    switch(id)
    {
    case 0:
        return Map::GM_ONLY;
    case 1:
        return Map::PC_MOVE;
    case 2:
        return Map::PC_ALL;
    default:
        return Map::GM_ONLY;
    }

}

void MainWindow::receiveData(quint64 readData,quint64 size)
{
    if(size==0)
    {
        m_downLoadProgressbar->setVisible(false);
        m_shownProgress=false;
    }
    else if(readData!=size)
    {
        m_downLoadProgressbar->setVisible(true);
        quint64 i = (size-readData)*100/size;

        m_downLoadProgressbar->setValue(i);
        m_shownProgress=true;
    }

}

void MainWindow::createNotificationZone()
{
    m_dockLogUtil = new QDockWidget(tr("Notification Zone"), this);
    m_dockLogUtil->setObjectName("dockLogUtil");
    m_dockLogUtil->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_dockLogUtil->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    QWidget* wd = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    wd->setLayout(layout);
    m_notifierDisplay = new QTextEdit(m_dockLogUtil);
    m_notifierDisplay->setReadOnly(true);

    layout->addWidget(m_notifierDisplay);
    layout->addWidget(m_downLoadProgressbar);

    m_dockLogUtil->setWidget(wd);
    m_dockLogUtil->setMinimumWidth(125);
}

void MainWindow::linkActionToMenu()
{
    // file menu
    connect(m_ui->m_newMapAction, SIGNAL(triggered(bool)), this, SLOT(newMap()));
    connect(m_ui->m_addVectorialMap, SIGNAL(triggered(bool)), this, SLOT(newVectorialMap()));
    connect(m_ui->m_newCharacterSheet,SIGNAL(triggered(bool)),this,SLOT(newCharacterSheetWindow()));
    connect(m_ui->m_newChatAction, SIGNAL(triggered(bool)), m_chatListWidget, SLOT(createPrivateChat()));
    connect(m_ui->m_newNoteAction, SIGNAL(triggered(bool)), this, SLOT(newNoteDocument()));

    //open
    connect(m_ui->m_openPictureAction, SIGNAL(triggered(bool)), this, SLOT(openContent()));
    connect(m_ui->m_openOnlinePictureAction, SIGNAL(triggered(bool)), this, SLOT(openContent()));
    connect(m_ui->m_openMapAction, SIGNAL(triggered(bool)), this, SLOT(openContent()));
    connect(m_ui->m_openCharacterSheet,SIGNAL(triggered(bool)),this,SLOT(openContent()));
    connect(m_ui->m_openVectorialMap, SIGNAL(triggered(bool)), this, SLOT(openContent()));
    connect(m_ui->m_openStoryAction, SIGNAL(triggered(bool)), this, SLOT(openStory()));
    connect(m_ui->m_openNoteAction, SIGNAL(triggered(bool)), this, SLOT(openNote()));

    m_ui->m_openPictureAction->setData((int)CleverURI::PICTURE);
    m_ui->m_openOnlinePictureAction->setData((int)CleverURI::ONLINEPICTURE);
    m_ui->m_openMapAction->setData((int)CleverURI::MAP);
    m_ui->m_openCharacterSheet->setData((int)CleverURI::CHARACTERSHEET);
    m_ui->m_openVectorialMap->setData((int)CleverURI::VMAP);
    m_ui->m_openStoryAction->setData((int)CleverURI::SCENARIO);
    m_ui->m_openNoteAction->setData((int)CleverURI::TEXT);

    m_ui->m_recentFileMenu->setVisible(false);
    connect(m_ui->m_closeAction, SIGNAL(triggered(bool)), this, SLOT(closeCurrentSubWindow()));
    connect(m_ui->m_saveAction, SIGNAL(triggered(bool)), this, SLOT(saveCurrentMedia()));
    connect(m_ui->m_saveAsAction, SIGNAL(triggered(bool)), this, SLOT(saveCurrentMedia()));
    connect(m_ui->m_saveScenarioAction, SIGNAL(triggered(bool)), this, SLOT(saveStory()));
    connect(m_ui->m_saveScenarioAsAction, SIGNAL(triggered(bool)), this, SLOT(saveAsStory()));
    connect(m_ui->m_preferencesAction, SIGNAL(triggered(bool)), m_preferencesDialog, SLOT(show()));

    // close
    connect(m_ui->m_quitAction, SIGNAL(triggered(bool)), this, SLOT(close()));

    // network
    connect(m_networkManager,SIGNAL(stopConnectionTry()),this,SLOT(stopReconnection()));
    connect(m_ui->m_disconnectAction,SIGNAL(triggered()),this,SLOT(closeConnection()));
    connect(m_ui->m_connectionAction,SIGNAL(triggered()),this,SLOT(startReconnection()));
    connect(m_ui->m_changeProfileAct,SIGNAL(triggered()),this,SLOT(showConnectionDialog()));

    // Windows managing
    connect(m_ui->m_cascadeViewAction, SIGNAL(triggered(bool)), m_mdiArea, SLOT(cascadeSubWindows()));
    connect(m_ui->m_tabViewAction,SIGNAL(triggered(bool)),m_mdiArea,SLOT(setTabbedMode(bool)));
    connect(m_ui->m_tileViewAction, SIGNAL(triggered(bool)), m_mdiArea, SLOT(tileSubWindows()));

    // Help
    connect(m_ui->m_aboutAction, SIGNAL(triggered()), this, SLOT(aboutRolisteam()));
    connect(m_ui->m_onlineHelpAction, SIGNAL(triggered()), this, SLOT(helpOnLine()));


    //Note Editor
    //connect(m_ui->m_showMinutesEditorAction, SIGNAL(triggered(bool)), this, SLOT(displayMinutesEditor(bool)));

}
void MainWindow::prepareMap(MapFrame* mapFrame)
{
    m_mediaHash.insert(mapFrame->getMediaId(),mapFrame);
    Map* map = mapFrame->getMap();
    if(NULL==map)
        return;
    map->setPointeur(m_toolBar->getCurrentTool());

    if(NULL!=m_currentConnectionProfile)
    {
        map->setLocalIsPlayer(!m_currentConnectionProfile->isGM());
    }

    connect(m_toolBar,SIGNAL(currentToolChanged(ToolsBar::SelectableTool)),map,SLOT(setPointeur(ToolsBar::SelectableTool)));

    connect(m_toolBar,SIGNAL(currentNpcSizeChanged(int)),map,SLOT(setCharacterSize(int)));
    connect(m_toolBar,SIGNAL(currentPenSizeChanged(int)),map,SLOT(setPenSize(int)));
    connect(m_toolBar,SIGNAL(currentTextChanged(QString)),map,SLOT(setCurrentText(QString)));
    connect(m_toolBar,SIGNAL(currentNpcNameChanged(QString)),map,SLOT(setCurrentNpcName(QString)));
    connect(m_toolBar,SIGNAL(currentNpcNumberChanged(int)),map,SLOT(setCurrentNpcNumber(int)));

    connect(map, SIGNAL(changeCurrentColor(QColor)), m_toolBar, SLOT(changeCurrentColor(QColor)));
    connect(map, SIGNAL(incrementeNumeroPnj()), m_toolBar, SLOT(incrementNpcNumber()));
    connect(map, SIGNAL(mettreAJourPnj(int, QString)), m_toolBar, SLOT(updateNpc(int,QString)));

    connect(m_ui->m_showPcNameAction, SIGNAL(triggered(bool)), map, SLOT(setPcNameVisible(bool)));
    connect(m_ui->m_showNpcNameAction, SIGNAL(triggered(bool)), map, SLOT(setNpcNameVisible(bool)));
    connect(m_ui->m_showNpcNumberAction, SIGNAL(triggered(bool)), map, SLOT(setNpcNumberVisible(bool)));

    connect(m_ui->m_showNpcNameAction, SIGNAL(triggered(bool)), map, SLOT(setNpcNameVisible(bool)));
    connect(m_ui->m_showPcNameAction, SIGNAL(triggered(bool)), map, SLOT(setPcNameVisible(bool)));
    connect(m_ui->m_showNpcNumberAction,SIGNAL(triggered(bool)),map,SLOT(setNpcNumberVisible(bool)));

    map->setNpcNameVisible(m_ui->m_showNpcNameAction->isChecked());
    map->setPcNameVisible(m_ui->m_showPcNameAction->isChecked());
    map->setNpcNumberVisible(m_ui->m_showNpcNumberAction->isChecked());
    map->setCurrentNpcNumber(m_toolBar->getCurrentNpcNumber());
    map->setPenSize(m_toolBar->getCurrentPenSize());

    // new PlayersList connection
    connect(mapFrame, SIGNAL(activated(Map *)), m_playersListWidget->model(), SLOT(changeMap(Map *)));
    connect(mapFrame, SIGNAL(activated(Map *)), m_toolBar, SLOT(changeMap(Map *)));
}
void MainWindow::prepareImage(Image* imageFrame)
{
    m_mediaHash.insert(imageFrame->getMediaId(),imageFrame);
    connect(m_toolBar,SIGNAL(currentToolChanged(ToolsBar::SelectableTool)),imageFrame,SLOT(setCurrentTool(ToolsBar::SelectableTool)));
    imageFrame->setCurrentTool(m_toolBar->getCurrentTool());
}

void MainWindow::prepareNote(NoteContainer* note)
{
    m_mediaHash.insert(note->getMediaId(),note);
}
void MainWindow::updateWorkspace()
{
    QMdiSubWindow* active = m_mdiArea->currentSubWindow();
    if (NULL!=active)
    {
        activeWindowChanged(active);
    }
}
void MainWindow::newMap()
{
    MapFrame* mapFrame = new MapFrame(NULL, m_mdiArea);
    if(!mapFrame->createMap())
    {
        delete mapFrame;
    }
    else
    {
        prepareMap(mapFrame);
        addMediaToMdiArea(mapFrame);
        mapFrame->setVisible(true);
    }
}
void MainWindow::newCharacterSheetWindow()
{
    CharacterSheetWindow* window = new CharacterSheetWindow();
    prepareCharacterSheetWindow(window);
    addMediaToMdiArea(window);
}


void MainWindow::newVectorialMap()
{
    MapWizzardDialog mapWizzard(m_mdiArea);
    if(mapWizzard.exec())
    {
        VMap* tempmap = new VMap();
        mapWizzard.setAllMap(tempmap);
        VMapFrame* tmp = new VMapFrame(new CleverURI(tempmap->getMapTitle(),"",CleverURI::VMAP),tempmap);
        prepareVMap(tmp);
        addMediaToMdiArea(tmp);
        //tempmap->setCurrentTool(m_toolbar->getCurrentTool());

        NetworkMessageWriter msg(NetMsg::VMapCategory,NetMsg::addVmap);

        tempmap->fill(msg);
        tmp->fill(msg);
        msg.sendAll();
    }
}
void MainWindow::newNoteDocument()
{
    NoteContainer* note = new NoteContainer();
    if(!m_mediaHash.contains(note->getMediaId()))
    {
        m_mediaHash.insert(note->getMediaId(),note);
    }
    m_sessionManager->addRessource(note->getCleverUri());
    addMediaToMdiArea(note);
    //displayMinutesEditor(true,true);
}
void MainWindow::sendOffAllMaps(NetworkLink * link)
{
    for(auto mediaC : m_mediaHash)
    {
        if(CleverURI::VMAP == mediaC->getContentType())
        {
            //mapi.next();
            VMapFrame* tmp = dynamic_cast<VMapFrame*>(mediaC);
            if(NULL!=tmp)
            {
                VMap* tempmap = tmp->getMap();
                NetworkMessageWriter msg(NetMsg::VMapCategory,NetMsg::addVmap);
                tempmap->fill(msg);
                tempmap->sendAllItems(msg);
                tmp->fill(msg);
                msg.sendTo(link);
            }
        }
        else if(CleverURI::MAP == mediaC->getContentType())
        {
            MapFrame* tmp = dynamic_cast<MapFrame*>(mediaC);
            if(NULL!=tmp)
            {
                tmp->getMap()->setHasPermissionMode(m_playerList->everyPlayerHasFeature("MapPermission"));
                tmp->getMap()->emettreCarte(tmp->windowTitle(), link);
                tmp->getMap()->emettreTousLesPersonnages(link);
            }
        }
    }
}
void MainWindow::sendOffAllImages(NetworkLink * link)
{
    NetworkMessageWriter message = NetworkMessageWriter(NetMsg::PictureCategory, NetMsg::AddPictureAction);

    for(MediaContainer* sub: m_mediaHash.values())
    {
        if(sub->getContentType() == CleverURI::PICTURE)
        {
            Image* img = dynamic_cast<Image*>(sub);
            if(NULL!=sub)
            {
                img->fill(message);
                message.sendTo(link);
            }
        }
    }
}
Map* MainWindow::findMapById(QString idMap)
{
    MediaContainer* media = m_mediaHash.value(idMap);
    if(NULL!=media)
    {
        MapFrame* mapframe = dynamic_cast<MapFrame*>(media);
        if(NULL!=mapframe)
        {
            return mapframe->getMap();
        }
    }
    return NULL;
}
bool MainWindow::mayBeSaved(bool connectionLoss)
{
    QMessageBox msgBox(this);
    QAbstractButton *boutonSauvegarder = msgBox.addButton(QMessageBox::Save);
    QAbstractButton *boutonQuitter = msgBox.addButton(tr("Quit"), QMessageBox::RejectRole);
    Qt::WindowFlags flags = msgBox.windowFlags();
    msgBox.setWindowFlags(flags ^ Qt::WindowSystemMenuHint);

    QString message;
    QString msg = m_preferences->value("Application_Name","rolisteam").toString();
    if (connectionLoss)
    {
        message = tr("Connection has been lost. %1 will be close").arg(msg);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(tr("Connection lost"));
    }
    else
    {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setWindowTitle(tr("Quit %1 ").arg(msg));
    }

    if(NULL!=PlayersList::instance()->getLocalPlayer())
    {
        if(!PlayersList::instance()->getLocalPlayer()->isGM())
        {
            message += tr("Do you want to save your minutes before to quit %1?").arg(msg);
        }
        else
        {
            message += tr("Do you want to save your scenario before to quit %1?").arg(msg);

        }

        msgBox.setText(message);
        msgBox.exec();
        if (msgBox.clickedButton() == boutonQuitter)
        {
            return true;
        }
        else if (msgBox.clickedButton() == boutonSauvegarder) //saving
        {
            bool ok;
            if (!PlayersList::instance()->getLocalPlayer()->isGM())
                ok = saveMinutes();
            else
                ok = saveStory();

            if (ok || connectionLoss)
            {
                return true;
            }
        }
        return false;
    }
    return true;
}
QWidget* MainWindow::registerSubWindow(QWidget * subWindow,QAction* action)
{
    return m_mdiArea->addWindow(subWindow,action);
}

void MainWindow::openNote()
{
    // open file name.
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Minutes"), m_preferences->value("MinutesDirectory",QDir::homePath()).toString(), CleverURI::getFilterForType(CleverURI::TEXT));

    if (!filename.isEmpty())
    {
        QFileInfo fi(filename);
        m_preferences->registerValue("MinutesDirectory",fi.absolutePath() +"/");
        NoteContainer* noteEditor = new NoteContainer();
        noteEditor->setFileName(filename);
        noteEditor->readFileFromUri();
        addMediaToMdiArea(noteEditor);
    }
}

void MainWindow::openStory()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open scenario"), m_preferences->value("SessionDirectory",QDir::homePath()).toString(), tr("Scenarios (*.sce)"));

    readStory(fileName);
}
void MainWindow::readStory(QString fileName)
{
    if (fileName.isNull())
        return;
    QFileInfo info(fileName);
    m_preferences->registerValue("SessionDirectory",info.absolutePath());
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        notifyUser("Cannot be read (openStory - MainWindow.cpp)");
        return;
    }
    m_sessionManager->setSessionName(info.baseName());
    QDataStream in(&file);
    m_sessionManager->loadSession(in);
    file.close();
    m_currentStory = new  CleverURI(getShortNameFromPath(fileName),fileName,CleverURI::SCENARIO);
    updateWindowTitle();
}
QString MainWindow::getShortNameFromPath(QString path)
{
    QFileInfo info(path);
    return info.baseName();
}

void MainWindow::saveAsStory()
{
    // open file
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Scenario as"), m_preferences->value("SessionDirectory",QDir::homePath()).toString(), tr("Scenarios (*.sce)"));

    if (!fileName.isNull())
    {
        if(!fileName.endsWith(".sce"))
        {
            fileName.append(QStringLiteral(".sce"));
        }
        m_currentStory = new  CleverURI(getShortNameFromPath(fileName),fileName,CleverURI::SCENARIO);
        saveStory();
    }
}
bool MainWindow::saveStory()
{
    if(nullptr == m_currentStory)
    {
        saveAsStory();
    }
    QFileInfo info(m_currentStory->getUri());

    m_preferences->registerValue("SessionDirectory",info.absolutePath());

    QFile file(m_currentStory->getUri());
    if (!file.open(QIODevice::WriteOnly))
    {
        notifyUser(tr("%1 cannot be opened (saveStory - MainWindow.cpp)").arg(m_currentStory->getUri()));
        return false;
    }

    saveAllMediaContainer();

    QDataStream out(&file);
    m_sessionManager->saveSession(out);
    file.close();
    m_sessionManager->setSessionName(m_currentStory->getData(ResourcesNode::NAME).toString());
    updateWindowTitle();
    return true;
}
////////////////////////////////////////////////////
// Save data
////////////////////////////////////////////////////


void MainWindow::saveCurrentMedia()
{
    QMdiSubWindow* active = m_mdiArea->currentSubWindow();
    if(NULL != active)
    {
        MediaContainer* currentMedia = dynamic_cast<MediaContainer*>(active);
        if(NULL!=currentMedia)
        {
            bool saveAs = false;
            if(qobject_cast<QAction*>(sender())==m_ui->m_saveAsAction)
            {
                saveAs=true;
            }
            saveMedia(currentMedia,true,saveAs);
        }
    }
}
void MainWindow::saveMedia(MediaContainer* mediaC,bool askPath, bool saveAs)
{
    if(NULL!=mediaC)
    {
        CleverURI* cleverURI = mediaC->getCleverUri();
        if(NULL!=cleverURI)
        {
            QString uri  = cleverURI->getUri();
            QFileInfo info(uri);
            if(!askPath)//save into story
            {
                mediaC->putDataIntoCleverUri();
                /*if(!uri.isEmpty())
                {
                    mediaC->saveMedia();
                }*/

            }
            else
            {
                if((uri.isEmpty())||(!info.exists(uri))||(!info.isWritable())||saveAs)
                {
                    QString key = CleverURI::getPreferenceDirectoryKey(cleverURI->getType());
                    QString filter = CleverURI::getFilterForType(cleverURI->getType());
                    QString media = CleverURI::typeToString(cleverURI->getType());
                    QString fileName= QFileDialog::getSaveFileName(this, tr("Save %1").arg(media), m_preferences->value(key,QDir::homePath()).toString(), filter);
                    if(fileName.isEmpty())
                    {
                        return;
                    }
                    QFileInfo info(fileName);
                    m_preferences->registerValue(key,info.absolutePath());
                    cleverURI->setUri(fileName);
                }
                mediaC->saveMedia();
                setLatestFile(cleverURI);
            }
        }
    }
}

bool MainWindow::saveMinutes()
{
    for(MediaContainer* edit : m_mediaHash.values())
    {
        if(CleverURI::TEXT == edit->getContentType())
        {
            NoteContainer* note = dynamic_cast<NoteContainer*>(edit);
            if(NULL!=note)
            {
                note->saveMedia();
            }
        }

    }
    return true;
}

void MainWindow::saveAllMediaContainer()
{
    //new fashion
    for(auto media : m_mediaHash.values())
    {
        saveMedia(media,false,false);
    }
}
void MainWindow::stopReconnection()
{
    m_ui->m_connectionAction->setEnabled(true);
    m_ui->m_disconnectAction->setEnabled(false);
}
void MainWindow::startReconnection()
{
    if(NULL==m_currentConnectionProfile)
    {
        showConnectionDialog(true);
        return;
    }
    if(!m_currentConnectionProfile->isServer())
    {
        closeAllImagesAndMaps();
    }
    if(m_networkManager->startConnection())
    {
        m_playerList->sendOffLocalPlayerInformations();
        m_playerList->sendOffFeatures(m_currentConnectionProfile->getPlayer());
        m_ui->m_connectionAction->setEnabled(false);
        m_ui->m_disconnectAction->setEnabled(true);
    }
    else
    {
        m_ui->m_connectionAction->setEnabled(true);
        m_ui->m_disconnectAction->setEnabled(false);
    }
}
void MainWindow::showIp(QString ip)
{
    notifyUser(tr("Server Ip Address:%1\nPort:%2").arg(ip).arg(m_networkManager->getPort()));
}
void MainWindow::setUpNetworkConnection()
{
    if((m_currentConnectionProfile!=NULL)&&(!m_currentConnectionProfile->isServer()))
    {
        connect(m_playerList, SIGNAL(localGMRefused(bool)), this, SLOT(userNatureChange(bool)));
        connect(this, SIGNAL(closing()), m_playerList, SLOT(sendDelLocalPlayer()));
    }
    else
    {
        //connect(m_networkManager, SIGNAL(linkAdded(NetworkLink *)), this, SLOT(updateSessionToNewClient(NetworkLink*)));
    }
    connect(m_networkManager, SIGNAL(dataReceived(quint64,quint64)), this, SLOT(receiveData(quint64,quint64)));
}
void MainWindow::readImageFromStream(QDataStream &file)
{
    QString title;
    QByteArray baImage;
    QPoint topleft;
    QSize size;

    file >> title;
    file >>topleft;
    file >> size;
    file >> baImage;

    QImage img;
    if (!img.loadFromData(baImage, "jpg"))
    {
        notifyUser(tr("Image compression error (readImageFromStream - MainWindow.cpp)"));
    }

    // Creation de l'identifiant
    QString idImage = QUuid::createUuid().toString();

    // Creation de la fenetre image
    Image* imgWindow = new Image(m_mdiArea);
    imgWindow->setTitle(title);
    imgWindow->setLocalPlayerId(m_localPlayerId);
    imgWindow->setImage(img);

    prepareImage(imgWindow);
    addMediaToMdiArea(imgWindow);

    connect(m_toolBar,SIGNAL(currentToolChanged(ToolsBar::SelectableTool)),imgWindow,SLOT(setCurrentTool(ToolsBar::SelectableTool)));
    imgWindow->setCurrentTool(m_toolBar->getCurrentTool());

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    if (!img.save(&buffer, "jpg", 70))
    {
        notifyUser(tr("Image compression error (readImageFromStream - MainWindow.cpp)"));
    }

    NetworkMessageWriter msg(NetMsg::PictureCategory,NetMsg::AddPictureAction);
    msg.string16(title);
    msg.string8(idImage);
    msg.string8(m_localPlayerId);
    msg.byteArray32(byteArray);
    msg.sendAll();
}
void MainWindow::helpOnLine()
{
    if (!QDesktopServices::openUrl(QUrl("http://wiki.rolisteam.org/")))
    {
        QMessageBox * msgBox = new QMessageBox(
                    QMessageBox::Information,
                    tr("Help"),
                    tr("Documentation of %1 can be found online at :<br> <a href=\"http://wiki.rolisteam.org\">http://wiki.rolisteam.org/</a>").arg(m_preferences->value("Application_Name","rolisteam").toString()),
                    QMessageBox::Ok
                    );
        msgBox->exec();
    }
}
void MainWindow::updateUi()
{
    if(nullptr==m_currentConnectionProfile)
    {
        return;
    }
    m_toolBar->updateUi(m_currentConnectionProfile->isGM());
#ifndef NULL_PLAYER
    m_audioPlayer->updateUi(m_currentConnectionProfile->isGM());
#endif
    if(nullptr!=m_preferencesDialog)
    {
        m_preferencesDialog->updateUi(m_currentConnectionProfile->isGM());
    }
    if(nullptr!=m_playersListWidget)
    {
        m_playersListWidget->updateUi(m_currentConnectionProfile->isGM());
    }


    bool isGM = m_currentConnectionProfile->isGM();
    m_vToolBar->setGM(isGM);
    m_ui->m_newMapAction->setEnabled(isGM);
    m_ui->m_addVectorialMap->setEnabled(isGM);
    m_ui->m_openMapAction->setEnabled(isGM);
    m_ui->m_openStoryAction->setEnabled(isGM);
    m_ui->m_closeAction->setEnabled(isGM);
    m_ui->m_saveAction->setEnabled(isGM);
    m_ui->m_saveScenarioAction->setEnabled(isGM);
    m_ui->m_saveScenarioAsAction->setEnabled(isGM);
    m_vmapToolBar->setVisible(isGM);
    m_vmapToolBar->toggleViewAction()->setVisible(isGM);

    m_ui->m_connectionAction->setEnabled(false);
    m_ui->m_disconnectAction->setEnabled(true);

    updateRecentFileActions();
}
void MainWindow::updateMayBeNeeded()
{
    if(m_updateChecker->mustBeUpdated())
    {
        QMessageBox::information(this,tr("Update Monitor"),tr("The %1 version has been released. Please take a look at <a href=\"http://www.rolisteam.org/download\">Download page</a> for more information").arg(m_updateChecker->getLatestVersion()));
    }
    m_updateChecker->deleteLater();
}
void MainWindow::networkStateChanged(NetworkManager::ConnectionState state)
{
    switch(state)
    {
    case NetworkManager::LISTENING:
        m_ipChecker->startCheck();
    case NetworkManager::CONNECTED:
        m_ui->m_connectionAction->setEnabled(false);
        m_ui->m_disconnectAction->setEnabled(true);
        m_dialog->accept();
        break;
    case NetworkManager::DISCONNECTED:
        m_ui->m_connectionAction->setEnabled(true);
        m_ui->m_disconnectAction->setEnabled(false);
        m_dialog->open();
        break;
    case NetworkManager::CONNECTING:
        break;
    }
}

void MainWindow::notifyAboutAddedPlayer(Player * player) const
{
    notifyUser(tr("%1 just joins the game.").arg(player->getName()));
    if(player->getUserVersion().compare(m_version)!=0)
    {
        notifyUser(tr("%1 has not the right version: %2.").arg(player->getName()).arg(player->getUserVersion()),Error);
    }
}

void MainWindow::notifyAboutDeletedPlayer(Player * player) const
{
    notifyUser(tr("%1 just leaves the game.").arg(player->getName()));
}

void MainWindow::updateSessionToNewClient(Player* player)
{
    if(NULL!=player)
    {
        sendOffAllMaps(player->link());
        sendOffAllImages(player->link());
        m_preferencesDialog->sendOffAllDiceAlias(player->link());
        m_preferencesDialog->sendOffAllState(player->link());
    }
}

void MainWindow::setNetworkManager(NetworkManager* tmp)
{
    m_networkManager = tmp;
    m_networkManager->setParent(this);
}
void MainWindow::readSettings()
{
    QSettings settings("rolisteam",QString("rolisteam_%1/preferences").arg(m_major));

    if(m_resetSettings)
    {
        settings.clear();
    }

    restoreState(settings.value("windowState").toByteArray());
    bool  maxi = settings.value("Maximized", false).toBool();
    if(!maxi)
    {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    m_preferences->readSettings(settings);

    /**
      * management of recentFileActs
      * */
    m_maxSizeRecentFile = m_preferences->value("recentfilemax",5).toInt();
    for (int i = 0; i < m_maxSizeRecentFile; ++i)
    {
        m_recentFileActs.insert(i,new QAction(m_ui->m_recentFileMenu));
        m_recentFileActs[i]->setVisible(false);
        connect(m_recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));

        m_ui->m_recentFileMenu->addAction(m_recentFileActs[i]);
    }
    updateRecentFileActions();
    m_preferencesDialog->initializePostSettings();
}
void MainWindow::writeSettings()
{
    QSettings settings("rolisteam",QString("rolisteam_%1/preferences").arg(m_major));
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("Maximized", isMaximized());
    m_preferences->writeSettings(settings);
}
void MainWindow::parseCommandLineArguments(QStringList list)
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption port(QStringList() << "p"<< "port", tr("Set rolisteam to use <port> for the connection"),"port");
    QCommandLineOption hostname(QStringList() << "s"<< "server", tr("Set rolisteam to connect to <server>."),"server");
    QCommandLineOption role(QStringList() << "r"<< "role", tr("Define the <role>: gm or pc"),"role");
    QCommandLineOption reset(QStringList() << "reset-settings", tr("Erase the settings and use the default parameters"));
    QCommandLineOption user(QStringList() << "u"<<"user", tr("Define the <username>"),"username");
    QCommandLineOption translation(QStringList() << "t"<<"translation", QObject::tr("path to the translation file: <translationfile>"),"translationfile");

    parser.addOption(port);
    parser.addOption(hostname);
    parser.addOption(role);
    parser.addOption(reset);
    parser.addOption(user);
    parser.addOption(translation);

    parser.process(list);

    bool hasPort = parser.isSet(port);
    bool hasHostname = parser.isSet(hostname);
    bool hasRole = parser.isSet(role);
    bool hasUser = parser.isSet(user);
    m_resetSettings = parser.isSet(reset);

    QString portValue;
    QString hostnameValue;
    QString roleValue;
    QString username;
    if(hasPort)
    {
        portValue = parser.value(port);
    }
    if(hasHostname)
    {
        hostnameValue = parser.value(hostname);
    }
    if(hasRole)
    {
        roleValue = parser.value(role);
    }
    if(hasUser)
    {
        username = parser.value(user);
    }
    if(!(roleValue.isNull()&&hostnameValue.isNull()&&portValue.isNull()&&username.isNull()))
    {
        m_networkManager->setValueConnection(portValue,hostnameValue,username,roleValue);
    }
}
NetWorkReceiver::SendType MainWindow::processMessage(NetworkMessageReader* msg, NetworkLink* link)
{
    if(NULL==msg)
        return NetWorkReceiver::NONE;


    NetWorkReceiver::SendType type;
    switch(msg->category())
    {
    case NetMsg::PictureCategory:
        processImageMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    case NetMsg::MapCategory:
        processMapMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    case NetMsg::NPCCategory:
        processNpcMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    case NetMsg::DrawCategory:
        processPaintingMessage(msg);
        type = NetWorkReceiver::ALL;
        break;
    case NetMsg::CharacterCategory:
        processCharacterMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    case NetMsg::ConnectionCategory:
        processConnectionMessage(msg);
        type = NetWorkReceiver::NONE;
        break;
    case NetMsg::VMapCategory:
        type = processVMapMessage(msg);
        break;
    case NetMsg::CharacterPlayerCategory:
        processCharacterPlayerMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    case NetMsg::MediaCategory:
        processMediaMessage(msg);
        type = NetWorkReceiver::AllExceptSender;
        break;
    }
    return type;//NetWorkReceiver::AllExceptMe;
}
void MainWindow::processMediaMessage(NetworkMessageReader* msg)
{
    if(msg->action() == NetMsg::closeMedia)
    {
        closeMediaContainer(msg->string8());
    }
}

void MainWindow::processConnectionMessage(NetworkMessageReader* msg)
{
    if(msg->action() == NetMsg::EndConnectionAction)
    {
        notifyUser(tr("End of the connection process"));
        updateWorkspace();
    }
}
void MainWindow::notifyUser(QString message, MainWindow::MessageType type) const
{
    static bool alternance = false;
    QColor color;
    alternance = !alternance;

    if (alternance)
        color = Qt::darkBlue;
    else
        color = Qt::darkRed;

    switch (type)
    {
    case Error:
        color = Qt::red;
        message.prepend(tr("Error:"));
        break;
    case Warning:
        color = Qt::darkRed;
        message.prepend(tr("Warning:"));
        break;
    case Information:
    case Notice:
        break;
    }

    QString time = QTime::currentTime().toString("hh:mm:ss") + " - ";
    m_notifierDisplay->moveCursor(QTextCursor::End);
    m_notifierDisplay->setTextColor(Qt::darkGreen);
    m_notifierDisplay->append(time);
    m_notifierDisplay->setTextColor(color);
    m_notifierDisplay->insertPlainText(message);
}
void  MainWindow::showConnectionDialog(bool forced)
{
    if((!m_profileDefined)||(forced))
    {
        m_dialog->open();
    }
}
void MainWindow::startConnection()
{
    m_chatListWidget->cleanChatList();
    QSettings settings("rolisteam","rolisteam");
    if(NULL!=m_dialog)
    {
        m_dialog->writeSettings(settings);
        m_currentConnectionProfile = m_dialog->getSelectedProfile();
        if((NULL!=m_currentConnectionProfile)&&(NULL!=m_networkManager))
        {
            m_networkManager->setConnectionProfile(m_currentConnectionProfile);
            if(NULL!=m_playerList)
            {
                m_playerList->completeListClean();

                m_playerList->setLocalPlayer(m_currentConnectionProfile->getPlayer());

                m_networkManager->startConnection();
                if(!m_currentConnectionProfile->isGM())
                {
                    m_playerList->addLocalCharacter(m_currentConnectionProfile->getCharacter());
                }
                m_playerList->sendOffLocalPlayerInformations();

                //may not be needed anymore
                m_playerList->sendOffFeatures(m_currentConnectionProfile->getPlayer());
            }
            m_localPlayerId = m_currentConnectionProfile->getPlayer()->getUuid();
            m_chatListWidget->addPublicChat();

            if(NULL!=m_preferences)
            {
                m_preferences->registerValue("isClient",!m_currentConnectionProfile->isServer());
            }


            setUpNetworkConnection();
            updateWindowTitle();
            checkUpdate();
            updateUi();
            m_dialog->writeSettings(settings);
        }
    }

}

void MainWindow::setupUi()
{
    // Initialisation de la liste des BipMapWindow, des Image et des Tchat
    m_mediaHash.clear();
    m_version=tr("unknown");
#ifdef VERSION_MINOR
#ifdef VERSION_MAJOR
#ifdef VERSION_MIDDLE
    m_version = QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MIDDLE).arg(VERSION_MINOR);
    m_major = QString("%1.%2.0").arg(VERSION_MAJOR).arg(VERSION_MIDDLE);
#endif
#endif
#endif


    //setAnimated(false);
    m_mdiArea = new ImprovedWorkspace();
    setCentralWidget(m_mdiArea);
    connect(m_mdiArea, SIGNAL(subWindowActivated ( QMdiSubWindow * )), this, SLOT(activeWindowChanged(QMdiSubWindow *)));

    m_toolBar = new ToolsBar();

    m_vToolBar = new VToolsBar();
    m_toolBarStack = new QStackedWidget();
    m_toolBarStack->setMinimumWidth(10);
    m_toolBarStack->addWidget(m_toolBar);
    m_toolBarStack->addWidget(m_vToolBar);

    QDockWidget* dock = new QDockWidget(this);
    dock->setWidget(m_toolBarStack);
    addDockWidget(Qt::LeftDockWidgetArea,dock);
    dock->setWindowTitle(tr("ToolBox"));
    dock->setObjectName("DockToolBar");
    m_ui->m_menuSubWindows->insertAction(m_ui->m_toolBarAct,dock->toggleViewAction());
    QAction* vmapToolBar  = m_vmapToolBar->toggleViewAction();
    vmapToolBar->setShortcut(Qt::Key_F9);
    m_ui->m_menuSubWindows->insertAction(m_ui->m_toolBarAct,m_vmapToolBar->toggleViewAction());
    m_ui->m_menuSubWindows->removeAction(m_ui->m_toolBarAct);

    createNotificationZone();
    m_ui->m_menuSubWindows->insertAction(m_ui->m_notificationAct,m_dockLogUtil->toggleViewAction());
    m_ui->m_menuSubWindows->removeAction(m_ui->m_notificationAct);
    addDockWidget(Qt::RightDockWidgetArea, m_dockLogUtil);


    m_chatListWidget = new ChatListWidget(this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::SharePreferencesCategory,m_chatListWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_chatListWidget);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_ui->m_menuSubWindows->insertAction(m_ui->m_chatListAct,m_chatListWidget->toggleViewAction());


    QDockWidget* dock2 = new QDockWidget(this);
    dock2->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    dock2->setWidget(m_sessionManager);
    dock2->setWindowTitle(tr("Resources Explorer"));
    dock2->setObjectName("sessionManager");
    addDockWidget(Qt::RightDockWidgetArea,dock2);
    m_ui->m_menuSubWindows->insertAction(m_ui->m_chatListAct,dock2->toggleViewAction());
    m_ui->m_menuSubWindows->removeAction(m_ui->m_chatListAct);


    ReceiveEvent::registerNetworkReceiver(NetMsg::PictureCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::MapCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::VMapCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::NPCCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::DrawCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::CharacterCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::ConnectionCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::CharacterPlayerCategory,this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::MediaCategory,this);


    ///////////////////
    //PlayerList
    ///////////////////
    m_playersListWidget = new PlayersListWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_playersListWidget);
    setWindowIcon(QIcon(":/logo.png"));
    m_ui->m_menuSubWindows->insertAction(m_ui->m_characterListAct,m_playersListWidget->toggleViewAction());
    m_ui->m_menuSubWindows->removeAction(m_ui->m_characterListAct);

    ///////////////////
    //Audio Player
    ///////////////////
#ifndef NULL_PLAYER
    m_audioPlayer = AudioPlayer::getInstance(this);
    ReceiveEvent::registerNetworkReceiver(NetMsg::MusicCategory,m_audioPlayer);
    addDockWidget(Qt::RightDockWidgetArea,m_audioPlayer );
    m_ui->m_menuSubWindows->insertAction(m_ui->m_audioPlayerAct,m_audioPlayer->toggleViewAction());
    m_ui->m_menuSubWindows->removeAction(m_ui->m_audioPlayerAct);
#endif

    m_preferencesDialog = new PreferencesDialog(this);
    linkActionToMenu();
    if(nullptr!=m_preferencesDialog->getStateModel())
    {
        ReceiveEvent::registerNetworkReceiver(NetMsg::SharePreferencesCategory,m_preferencesDialog->getStateModel());
    }


    // Initialisation des etats de sante des PJ/PNJ (variable declarees dans DessinPerso.cpp)
    m_playerList = PlayersList::instance();

    connect(m_playerList, SIGNAL(playerAdded(Player *)), this, SLOT(notifyAboutAddedPlayer(Player *)));
    connect(m_playerList, SIGNAL(playerAddedAsClient(Player*)), this, SLOT(updateSessionToNewClient(Player*)));
    connect(m_playerList, SIGNAL(playerDeleted(Player *)), this, SLOT(notifyAboutDeletedPlayer(Player *)));


    m_dialog = new SelectConnectionProfileDialog(m_version,this);
    connect(m_dialog,SIGNAL(tryConnection()),this,SLOT(startConnection()));
    connect(m_networkManager,SIGNAL(errorOccur(QString)),m_dialog,SLOT(errorOccurs(QString)));
    connect(m_networkManager,SIGNAL(connectionStateChanged(NetworkManager::ConnectionState)),this,SLOT(updateWindowTitle()));
    connect(m_networkManager,SIGNAL(connectionStateChanged(NetworkManager::ConnectionState)),this,SLOT(networkStateChanged(NetworkManager::ConnectionState)));
    connect(m_ipChecker,SIGNAL(finished(QString)),this,SLOT(showIp(QString)));
}
void MainWindow::processMapMessage(NetworkMessageReader* msg)
{
    if(msg->action() == NetMsg::CloseMap)
    {
        QString idMap = msg->string8();
        closeMediaContainer(idMap);
    }
    else
    {
        MapFrame* mapFrame = new MapFrame(NULL, m_mdiArea);
      //  mapFrame->setCleverUri(new CleverURI("",CleverURI::MAP));

        if((NULL!=m_currentConnectionProfile)&&(!mapFrame->processMapMessage(msg,!m_currentConnectionProfile->isGM())))
        {
            delete mapFrame;
        }
        else
        {
            prepareMap(mapFrame);
            addMediaToMdiArea(mapFrame);
            mapFrame->setVisible(true);
        }
    }
}
void MainWindow::processImageMessage(NetworkMessageReader* msg)
{
    if(msg->action() == NetMsg::AddPictureAction)
    {
        QString title = msg->string16();
        QString idImage = msg->string8();
        QString idPlayer = msg->string8();
        QByteArray dataImage = msg->byteArray32();

        QImage *img = new QImage;
        if (!img->loadFromData(dataImage, "jpg"))
        {
            notifyUser("Cannot read received image (receptionMessageImage - NetworkLink.cpp)");
        }
        Image* image = new Image();
        image->setTitle(title);
        image->setMediaId(idImage);
        image->setIdOwner(idPlayer);
        image->setImage(*img);

        //addImage(image, title);
        prepareImage(image);
        addMediaToMdiArea(image);
        image->setVisible(true);
    }
    else if(msg->action() == NetMsg::DelPictureAction)
    {
        QString idImage = msg->string8();
        closeMediaContainer(idImage);
    }
}
void MainWindow::processNpcMessage(NetworkMessageReader* msg)
{
    QString idMap = msg->string8();
    if(msg->action() == NetMsg::addNpc)
    {
        Map* map = findMapById(idMap);
        extractCharacter(map,msg);
    }
    else if(msg->action() == NetMsg::delNpc)
    {
        QString idNpc = msg->string8();
        Map* map = findMapById(idMap);
        if(NULL!=map)
        {
            map->eraseCharacter(idNpc);
        }
    }
}
void MainWindow::processPaintingMessage(NetworkMessageReader* msg)
{
    if(msg->action() == NetMsg::penPainting)
    {
        QString idPlayer = msg->string8();
        QString idMap = msg->string8();

        quint32 pointNumber = msg->uint32();

        QList<QPoint> pointList;
        QPoint point;
        for (int i=0; i<pointNumber; i++)
        {
            quint16 pointX = msg->uint16();
            quint16 pointY = msg->uint16();
            point = QPoint(pointX, pointY);
            pointList.append(point);
        }
        quint16 zoneX = msg->uint16();
        quint16 zoneY = msg->uint16();
        quint16 zoneW = msg->uint16();
        quint16 zoneH = msg->uint16();

        QRect zoneToRefresh(zoneX,zoneY,zoneW,zoneH);

        quint8 diameter = msg->uint8();
        quint8 colorType = msg->uint8();
        QColor color(msg->rgb());

        Map* map = findMapById(idMap);

        if(NULL!=map)
        {
            SelectedColor selectedColor;
            selectedColor.color = color;
            selectedColor.type = (ColorKind)colorType;
            map->paintPenLine(&pointList,zoneToRefresh,diameter,selectedColor,idPlayer==m_localPlayerId);
        }
    }
    else if(msg->action() == NetMsg::textPainting)
    {
        QString idMap = msg->string8();
        QString text = msg->string16();
        quint16 posx = msg->uint16();
        quint16 posy = msg->uint16();
        QPoint pos(posx,posy);

        quint16 zoneX = msg->uint16();
        quint16 zoneY = msg->uint16();
        quint16 zoneW = msg->uint16();
        quint16 zoneH = msg->uint16();

        QRect zoneToRefresh(zoneX,zoneY,zoneW,zoneH);
        quint8 colorType = msg->uint8();
        QColor color(msg->rgb());

        Map* map = findMapById(idMap);

        if(NULL!=map)
        {
            SelectedColor selectedColor;
            selectedColor.color = color;
            selectedColor.type = (ColorKind)colorType;

            map->paintText(text,pos,zoneToRefresh,selectedColor);

        }
    }
    else if(msg->action() == NetMsg::handPainting)
    {

    }
    else
    {
        QString idMap = msg->string8();
        quint16 posx = msg->uint16();
        quint16 posy = msg->uint16();
        QPoint startPos(posx,posy);

        quint16 endposx = msg->uint16();
        quint16 endposy = msg->uint16();
        QPoint endPos(endposx,endposy);


        quint16 zoneX = msg->uint16();
        quint16 zoneY = msg->uint16();
        quint16 zoneW = msg->uint16();
        quint16 zoneH = msg->uint16();

        QRect zoneToRefresh(zoneX,zoneY,zoneW,zoneH);

        quint8 diameter = msg->uint8();
        quint8 colorType = msg->uint8();
        QColor color(msg->rgb());
        Map* map = findMapById(idMap);

        if(NULL!=map)
        {
            SelectedColor selectedColor;
            selectedColor.color = color;
            selectedColor.type = (ColorKind)colorType;

            map->paintOther(msg->action(),startPos,endPos,zoneToRefresh,diameter,selectedColor);
        }


    }
}
void MainWindow::extractCharacter(Map* map,NetworkMessageReader* msg)
{
    if(NULL!=map)
    {
        QString npcName = msg->string16();
        QString npcId = msg->string8();
        quint8 npcType = msg->uint8();
        quint8 npcNumber = msg->uint8();
        quint8 npcDiameter = msg->uint8();
        QColor npcColor(msg->rgb());
        quint16 npcXpos = msg->uint16();
        quint16 npcYpos = msg->uint16();

        qint16 npcXorient = msg->int16();
        qint16 npcYorient = msg->int16();

        QColor npcState(msg->rgb());
        QString npcStateName = msg->string16();
        quint16 npcStateNum = msg->uint16();

        quint8 npcVisible = msg->uint8();
        quint8 npcVisibleOrient = msg->uint8();

        QPoint orientation(npcXorient, npcYorient);

        QPoint npcPos(npcXpos, npcYpos);

        bool showNumber=(npcType == CharacterToken::pnj)?m_ui->m_showNpcNumberAction->isChecked():false;
        bool showName=(npcType == CharacterToken::pnj)? m_ui->m_showNpcNameAction->isChecked():m_ui->m_showPcNameAction->isChecked();

        CharacterToken* npc = new CharacterToken(map, npcId, npcName, npcColor, npcDiameter, npcPos, (CharacterToken::typePersonnage)npcType,showNumber,showName, npcNumber);

        if((npcVisible)||(npcType == CharacterToken::pnj && (NULL!=m_currentConnectionProfile) && m_currentConnectionProfile->isGM()))
        {
            npc->showCharacter();
        }
        npc->newOrientation(orientation);
        if(npcVisibleOrient)
        {
            npc->showOrHideOrientation();
        }

        CharacterToken::etatDeSante health;
        health.couleurEtat = npcState;
        health.nomEtat = npcStateName;
        npc->newHealtState(health, npcStateNum);
        map->afficheOuMasquePnj(npc);

    }
}

void MainWindow::processCharacterMessage(NetworkMessageReader* msg)
{
    if(NetMsg::addCharacterList == msg->action())
    {
        QString idMap = msg->string8();
        quint16 characterNumber = msg->uint16();
        Map* map = findMapById(idMap);

        if(NULL!=map)
        {
            for(int i=0;i<characterNumber;++i)
            {
                extractCharacter(map,msg);
            }
        }
    }
    else if(NetMsg::moveCharacter == msg->action())
    {
        QString idMap = msg->string8();
        QString idCharacter = msg->string8();
        quint32 pointNumber = msg->uint32();


        QList<QPoint> moveList;
        QPoint point;
        for (int i=0; i<pointNumber; i++)
        {
            quint16 posX = msg->uint16();
            quint16 posY = msg->uint16();
            point = QPoint(posX, posY);
            moveList.append(point);
        }
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            map->lancerDeplacementPersonnage(idCharacter,moveList);
        }
    }
    else if(NetMsg::changeCharacterState == msg->action())
    {
        QString idMap = msg->string8();
        QString idCharacter = msg->string8();
        quint16 stateNumber = msg->uint16();
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            CharacterToken* character = map->trouverPersonnage(idCharacter);
            if(NULL!=character)
            {
                character->changeHealtStatus(stateNumber);
            }
        }
    }
    else if(NetMsg::changeCharacterOrientation == msg->action())
    {
        QString idMap = msg->string8();
        QString idCharacter = msg->string8();
        qint16 posX = msg->int16();
        qint16 posY = msg->int16();
        QPoint orientation(posX, posY);
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            CharacterToken* character = map->trouverPersonnage(idCharacter);
            if(NULL!=character)
            {
                character->newOrientation(orientation);
            }
        }
    }
    else if(NetMsg::showCharecterOrientation == msg->action())
    {
        QString idMap = msg->string8();
        QString idCharacter = msg->string8();
        quint8 showOrientation = msg->uint8();
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            CharacterToken* character = map->trouverPersonnage(idCharacter);
            if(NULL!=character)
            {
                character->showOrientation(showOrientation);
            }
        }
    }
    else if(NetMsg::addCharacterSheet == msg->action())
    {
        QString idPlayer = msg->string8();
        if(m_localPlayerId == idPlayer)
        {
            CharacterSheetWindow* sheetWindow = new CharacterSheetWindow();
            prepareCharacterSheetWindow(sheetWindow);
            sheetWindow->read(msg);
            addMediaToMdiArea(sheetWindow);
            m_mediaHash.insert(sheetWindow->getMediaId(),sheetWindow);
        }

    }
    else if(NetMsg::updateFieldCharacterSheet == msg->action())
    {

        QString idCharacterSheetW = msg->string8();
        CharacterSheetWindow* sheet = findCharacterSheetWindowById(idCharacterSheetW);
        if(nullptr!=sheet)
        {
            sheet->processUpdateFieldMessage(msg);

        }

    }
    else if(NetMsg::closeCharacterSheet == msg->action())
    {
        QString mediaId = msg->string8();
        QString sheetId = msg->string8();
        CharacterSheetWindow* sheet = findCharacterSheetWindowById(mediaId);

        if(nullptr!=sheet)
        {
            if((sheet->hasCharacterSheet(sheetId))&&(!m_currentConnectionProfile->isGM()))//can't close the media for the GM.
            {
                closeMediaContainer(mediaId);
            }
        }
    }
}
CharacterSheetWindow*  MainWindow::findCharacterSheetWindowById(QString id)
{
    if(m_mediaHash.contains(id))
    {
        MediaContainer* media = m_mediaHash.value(id);
        return dynamic_cast<CharacterSheetWindow*>(media);
    }
    else
    {
        return nullptr;
    }
}

void MainWindow::processCharacterPlayerMessage(NetworkMessageReader* msg)
{
    QString idMap = msg->string8();
    QString idCharacter = msg->string8();
    if(msg->action() == NetMsg::ToggleViewPlayerCharacterAction)
    {
        quint8 display = msg->uint8();
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            map->showPc(idCharacter,display);
        }
    }
    else if(msg->action() == NetMsg::ChangePlayerCharacterSizeAction)
    {
        /// @warning overweird
        quint8 size = msg->uint8();
        Map* map=findMapById(idMap);
        if(NULL!=map)
        {
            map->selectCharacter(idCharacter);
            map->changePjSize(size + 11);
        }
    }
}
void MainWindow::prepareVMap(VMapFrame* tmp)
{
    if(NULL==tmp)
        return;

    VMap* map = tmp->getMap();

    if(NULL==map)
        return;
    if(NULL!=m_currentConnectionProfile)
    {
        map->setOption(VisualItem::LocalIsGM,m_currentConnectionProfile->isGM());
    }
    map->setLocalId(m_localPlayerId);

    //Toolbar to Map
    connect(m_vToolBar,SIGNAL(currentToolChanged(VToolsBar::SelectableTool)),tmp,SLOT(currentToolChanged(VToolsBar::SelectableTool)));
    connect(tmp,SIGNAL(defineCurrentTool(VToolsBar::SelectableTool)),m_vToolBar,SLOT(setCurrentTool(VToolsBar::SelectableTool)));
    connect(map,SIGNAL(colorPipette(QColor)),m_vToolBar,SLOT(setCurrentColor(QColor)));
    connect(m_vToolBar,SIGNAL(currentColorChanged(QColor&)),tmp,SLOT(currentColorChanged(QColor&)));
    connect(m_vToolBar,SIGNAL(currentModeChanged(int)),tmp,SLOT(setEditingMode(int)));
    connect(m_vToolBar,SIGNAL(currentPenSizeChanged(int)),tmp,SLOT(currentPenSizeChanged(int)));
    connect(m_vToolBar,SIGNAL(currentNpcNameChanged(QString)),tmp,SLOT(setCurrentNpcNameChanged(QString)));
    connect(m_vToolBar,SIGNAL(currentNpcNumberChanged(int)),tmp,SLOT(setCurrentNpcNumberChanged(int)));

    connect(m_vToolBar,SIGNAL(opacityChanged(qreal)),map,SLOT(setCurrentItemOpacity(qreal)));
    connect(map,SIGNAL(currentItemOpacity(qreal)),m_vToolBar,SLOT(setCurrentOpacity(qreal)));

    connect(m_vToolBar,SIGNAL(currentEditionModeChanged(VToolsBar::EditionMode)),map,SLOT(setEditionMode(VToolsBar::EditionMode)));


    //map to toolbar
    connect(map,SIGNAL(npcAdded()),m_vToolBar,SLOT(increaseNpcNumber()));

    // menu to Map
    connect(m_ui->m_showNpcNameAction, &QAction::triggered,[=](bool b){
        map->setOption(VisualItem::ShowNpcName,b);
    });
    connect(m_ui->m_showNpcNumberAction, &QAction::triggered,[=](bool b){
        map->setOption(VisualItem::ShowNpcNumber,b);
    });
    connect(m_ui->m_showPcNameAction, &QAction::triggered,[=](bool b){
        map->setOption(VisualItem::ShowPcName,b);
    });
    connect(m_ui->m_showHealtStatusAction, &QAction::triggered,[=](bool b){
        map->setOption(VisualItem::ShowHealtStatus,b);
    });

    map->setOption(VisualItem::ShowNpcName,m_ui->m_showNpcNameAction->isChecked());
    map->setOption(VisualItem::ShowNpcNumber,m_ui->m_showNpcNumberAction->isChecked());
    map->setOption(VisualItem::ShowPcName,m_ui->m_showPcNameAction->isChecked());
    map->setOption(VisualItem::ShowHealtStatus,m_ui->m_showHealtStatusAction->isChecked());

    map->setCurrentNpcNumber(m_toolBar->getCurrentNpcNumber());
    tmp->currentPenSizeChanged(m_vToolBar->getCurrentPenSize());

    m_mediaHash.insert(tmp->getMediaId(),tmp);

    m_vToolBar->setCurrentTool(VToolsBar::HANDLER);
    tmp->currentToolChanged(m_vToolBar->getCurrentTool());

}
NetWorkReceiver::SendType MainWindow::processVMapMessage(NetworkMessageReader* msg)
{
    bool isServer = false;
    if(NULL!=m_currentConnectionProfile)
    {
        isServer = m_currentConnectionProfile->isServer();
    }
    NetWorkReceiver::SendType type = NetWorkReceiver::NONE;
    if(isServer)
    {
        type = NetWorkReceiver::AllExceptSender;
    }
    switch(msg->action())
    {
    case NetMsg::addVmap:
    {
        VMap* map = new VMap();
        map->setOption(VisualItem::LocalIsGM,false);
        map->readMessage(*msg);

        VMapFrame* mapFrame = new VMapFrame(nullptr,map);
        mapFrame->readMessage(*msg);
        prepareVMap(mapFrame);
        addMediaToMdiArea(mapFrame);
    }
        break;
    case NetMsg::loadVmap:
        break;
    case NetMsg::closeVmap:
    {
        QString vmapId = msg->string8();
        closeMediaContainer(vmapId);
    }
        break;
    case NetMsg::DelPoint:
        break;
    case NetMsg::addItem:
    case NetMsg::DelItem:
    case NetMsg::MoveItem:
    case NetMsg::GeometryItemChanged:
    case NetMsg::OpacityItemChanged:
    case NetMsg::LayerItemChanged:
    case NetMsg::MovePoint:
    case NetMsg::vmapChanges:
    case NetMsg::GeometryViewChanged:
    case NetMsg::SetParentItem:
    case NetMsg::RectGeometryItem:
    case NetMsg::RotationItem:
    case NetMsg::characterStateChanged:
    case NetMsg::VisionChanged:
    case NetMsg::ZValueItem:
    {
        QString vmapId = msg->string8();
        MediaContainer* tmp = m_mediaHash.value(vmapId);
        if(NULL!=tmp)
        {
            VMapFrame* mapF = dynamic_cast<VMapFrame*>(tmp);
            if(NULL!=mapF)
            {
                type = mapF->processMessage(msg);
            }
        }
    }
        break;
    }

    return type;
}
CleverURI* MainWindow::contentToPath(CleverURI::ContentType type,bool save)
{
    QString filter = CleverURI::getFilterForType(type);
    QString folder;
    QString title;
    switch(type)
    {
    case CleverURI::PICTURE:
        title = tr("Open Picture");
        folder = m_preferences->value(QString("PicturesDirectory"),".").toString();
        break;
    case CleverURI::MAP:
        title = tr("Open Map");
        folder = m_preferences->value(QString("MapsDirectory"),".").toString();
        break;
    case CleverURI::TEXT:
        title = tr("Open Minutes");
        folder = m_preferences->value(QString("MinutesDirectory"),".").toString();
        break;
    default:
        break;
    }
    if(!filter.isNull())
    {
        QString filepath;
        if(save)
            filepath= QFileDialog ::getSaveFileName(this,title,folder,filter);
        else
            filepath= QFileDialog::getOpenFileName(this,title,folder,filter);

        return new CleverURI(getShortNameFromPath(filepath),filepath,type);
    }
    return NULL;
}
void MainWindow::openContent()
{
    QAction* action=static_cast<QAction*>(sender());
    CleverURI::ContentType type = (CleverURI::ContentType)action->data().toInt();
    openContentFromType(type);
}
void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        CleverURI* uri = new CleverURI();
        *uri = action->data().value<CleverURI>();
        openCleverURI(uri,true);
    }
}
void MainWindow::updateRecentFileActions()
{
    QVariant var = QVariant::fromValue(CleverUriList());

    CleverUriList files = m_preferences->value("recentFileList",var).value<CleverUriList >();

    int numRecentFiles = qMin(files.size(), m_maxSizeRecentFile);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = QStringLiteral("&%1 %2").arg(i + 1).arg(files[i].name());
        m_recentFileActs[i]->setText(text);
        QVariant var;
        var.setValue(files[i]);
        m_recentFileActs[i]->setData(var);
        m_recentFileActs[i]->setIcon(QIcon(files[i].getIcon()));
        m_recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < m_recentFileActs.size() ; ++j)
    {
        m_recentFileActs[j]->setVisible(false);
    }

    m_ui->m_recentFileMenu->setEnabled(numRecentFiles > 0);
}
void MainWindow::setLatestFile(CleverURI* fileName)
{
    // no online picture because they are handled in a really different way.
    if((NULL!=fileName)&&(fileName->getType()!=CleverURI::ONLINEPICTURE))
    {
        QVariant var = QVariant::fromValue(CleverUriList());

        CleverUriList files = m_preferences->value("recentFileList",var).value<CleverUriList>();

        files.removeAll(*fileName);
        files.prepend(*fileName);
        while (files.size() > m_maxSizeRecentFile)
        {
            files.removeLast();
        }
        QVariant var3;
        var3.setValue(files);

        m_preferences->registerValue("recentFileList", var3,true);


        updateRecentFileActions();
    }
}
void MainWindow::showCleverUri(CleverURI* uri)
{
    for(auto i : m_mapAction->keys())
    {
        if(i->getCleverUri() == uri)
        {
            i->setVisible(true);
        }
    }
}
void MainWindow::prepareCharacterSheetWindow(CharacterSheetWindow* window)
{
    if(NULL!=m_currentConnectionProfile)
    {
        window->setLocalIsGM(m_currentConnectionProfile->isGM());
    }
    m_mediaHash.insert(window->getMediaId(),window);
    connect(window,SIGNAL(addWidgetToMdiArea(QWidget*,QString )),m_mdiArea,SLOT(addWidgetToMdi(QWidget*,QString)));
    connect(window,SIGNAL(rollDiceCmd(QString,QString)),m_chatListWidget,SLOT(rollDiceCmd(QString,QString)));
    connect(window,SIGNAL(errorOccurs(QString,MainWindow::MessageType)),this,SLOT(notifyUser(QString,MainWindow::MessageType)));
    connect(m_playerList,SIGNAL(playerDeleted(Player*)),window,SLOT(removeConnection(Player*)));
}

void MainWindow::openCleverURI(CleverURI* uri,bool force)
{
    if(NULL==uri)
    {
        return;
    }
    if((uri->isDisplayed())&&(!force))
    {
        showCleverUri(uri);
        return;
    }

    MediaContainer* tmp=NULL;
    switch(uri->getType())
    {
    case CleverURI::MAP:
        tmp = new MapFrame();
        break;
    case CleverURI::VMAP:
        tmp = new VMapFrame();
        break;
    case CleverURI::PICTURE:
    case CleverURI::ONLINEPICTURE:
        tmp = new Image();
        break;
    case CleverURI::TEXT:
        tmp = new NoteContainer();
        break;
    case CleverURI::SCENARIO:
        break;
    case CleverURI::CHARACTERSHEET:
    {
        CharacterSheetWindow* csW = new CharacterSheetWindow();
        prepareCharacterSheetWindow(csW);
        tmp = csW;
    }
        break;
    case CleverURI::SONGLIST:
    {
        #ifndef NULL_PLAYER
            m_audioPlayer->openSongList(uri->getUri());
        #endif
    }
        break;
    case CleverURI::SONG:
    {
        #ifndef NULL_PLAYER
            m_audioPlayer->openSong(uri->getUri());
        #endif
    }
        break;
    default:
        break;
    }
    if(tmp!=NULL)
    {
        tmp->setLocalPlayerId(m_localPlayerId);
        tmp->setCleverUri(uri);
        if(tmp->readFileFromUri())
        {
            if(uri->getType()==CleverURI::MAP)
            {
                prepareMap(static_cast<MapFrame*>(tmp));
            }
            else if(uri->getType()==CleverURI::VMAP)
            {
                prepareVMap(static_cast<VMapFrame*>(tmp));
            }
            else if((uri->getType()==CleverURI::PICTURE)||((uri->getType()==CleverURI::ONLINEPICTURE)))
            {
                prepareImage(static_cast<Image*>(tmp));
            }
            else if(uri->getType()==CleverURI::TEXT)
            {
                prepareNote(static_cast<NoteContainer*>(tmp));
            }
            addMediaToMdiArea(tmp);
        }
        else
        {
            uri->setDisplayed(false);
        }
    }
}
void MainWindow::openContentFromType(CleverURI::ContentType type)
{
    QString filter = CleverURI::getFilterForType(type);
    if(!filter.isEmpty())
    {
        QString folder = m_preferences->value(CleverURI::getPreferenceDirectoryKey(type),".").toString();
        QString title = tr("Open %1").arg(CleverURI::typeToString(type));
        QStringList filepath = QFileDialog::getOpenFileNames(this,title,folder,filter);
        QStringList list = filepath;
        QStringList::Iterator it = list.begin();
        while(it != list.end())
        {
            openCleverURI(new CleverURI(getShortNameFromPath(*it),*it,type));
            ++it;
        }
    }
    else
    {
        MediaContainer* tmp=NULL;
        switch(type)
        {
        case CleverURI::MAP:
            tmp = new MapFrame();
            break;
        case CleverURI::PICTURE:
        case CleverURI::ONLINEPICTURE:
            tmp = new Image();
            break;
        case CleverURI::VMAP:
            tmp = new VMapFrame();
            break;
        default:
            break;
        }

        if(tmp!=NULL)
        {
            tmp->setCleverUriType(type);
            if(tmp->openMedia())
            {
                if(tmp->readFileFromUri())
                {
                    if(type==CleverURI::MAP)
                    {
                        prepareMap(static_cast<MapFrame*>(tmp));
                    }
                    else if(type == CleverURI::VMAP)
                    {
                        prepareVMap(static_cast<VMapFrame*>(tmp));
                    }
                    else if((type==CleverURI::PICTURE)||(type==CleverURI::ONLINEPICTURE))
                    {
                        prepareImage(static_cast<Image*>(tmp));
                    }
                    addMediaToMdiArea(tmp);
                    tmp->setVisible(true);
                }
            }
        }
    }
}
void MainWindow::updateWindowTitle()
{
    if(NULL != m_currentConnectionProfile)
    {
        setWindowTitle(QStringLiteral("%6[*] - v%2 - %3 - %4 - %5 - %1").arg(m_preferences->value("applicationName","Rolisteam").toString())
                       .arg(m_version)
                       .arg(m_networkManager->isConnected() ? tr("Connected") : tr("Not Connected"))
                       .arg(m_currentConnectionProfile->isServer() ? tr("Server") : tr("Client")).arg(m_currentConnectionProfile->isGM() ? tr("GM") : tr("Player"))
                       .arg(m_sessionManager->getSessionName()));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    QMainWindow::dragEnterEvent(event);
}
CleverURI::ContentType MainWindow::getContentType(QString str)
{
    QImage imag(str);
    if(str.endsWith(".pla"))
    {
        return CleverURI::MAP;
    }
    else if(!imag.isNull())
    {
        return CleverURI::PICTURE;
    }
    else if(str.endsWith(".vmap"))
    {
        return CleverURI::VMAP;
    }
    else if(str.endsWith(".m3u"))
    {
        return CleverURI::SONGLIST;
    }
    else if(str.endsWith(".sce"))
    {
        return CleverURI::SCENARIO;
    }
    else if(str.endsWith(".rcs"))
    {
        return CleverURI::CHARACTERSHEET;
    }
    else
    {
        QStringList list = m_preferences->value("AudioFileFilter","*.wav *.mp2 *.mp3 *.ogg *.flac").toString().split(' ');
        //QStringList list = audioFileFilter.split(' ');
        int i=0;
        while(i<list.size())
        {
            QString filter = list.at(i);
            filter.replace("*","");
            if(str.endsWith(filter))
            {
                return CleverURI::SONG;
            }
            ++i;
        }
    }
    return CleverURI::NONE;
}
void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* data = event->mimeData();
    if(data->hasUrls())
    {
        QList<QUrl> list = data->urls();
        for(int i = 0; i< list.size();++i)
        {
            CleverURI::ContentType type= getContentType(list.at(i).toLocalFile());
            MediaContainer* tmp=NULL;
            CleverURI* uri = new CleverURI(getShortNameFromPath(list.at(i).toLocalFile()),list.at(i).toLocalFile(),type);
            switch(type)
            {
            case CleverURI::MAP:
                tmp = new MapFrame();
                tmp->setCleverUri(uri);
                tmp->readFileFromUri();
                prepareMap(static_cast<MapFrame*>(tmp));
                addMediaToMdiArea(tmp);
                tmp->setVisible(true);
                break;
            case CleverURI::CHARACTERSHEET:
            {
                CharacterSheetWindow* sheet = new CharacterSheetWindow();
                prepareCharacterSheetWindow(sheet);
                tmp = sheet;
                tmp->setCleverUri(uri);
                tmp->readFileFromUri();
                addMediaToMdiArea(tmp);
                tmp->setVisible(true);
            }
                break;
            case CleverURI::PICTURE:
                tmp = new Image();
                tmp->setCleverUri(uri);
                tmp->readFileFromUri();
                prepareImage(static_cast<Image*>(tmp));
                addMediaToMdiArea(tmp);
                tmp->setVisible(true);
                break;
            case CleverURI::VMAP:
                tmp = new VMapFrame();
                tmp->setCleverUri(uri);
                tmp->readFileFromUri();
                prepareVMap(static_cast<VMapFrame*>(tmp));
                addMediaToMdiArea(tmp);
                tmp->setVisible(true);
            case CleverURI::SONG:
#ifndef NULL_PLAYER
                m_audioPlayer->openSong(list.at(i).toLocalFile());
#endif
                m_sessionManager->addRessource(uri);
                break;
            case CleverURI::SONGLIST:
#ifndef NULL_PLAYER
                m_audioPlayer->openSongList(list.at(i).toLocalFile());
#endif
                m_sessionManager->addRessource(uri);
                break;
            case CleverURI::SCENARIO:
                readStory(uri->getUri());
                break;
            }
            //m_sessionManager->addRessource(uri);
        }
        event->acceptProposedAction();
    }
}
