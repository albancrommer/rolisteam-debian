/***************************************************************************
*	Copyright (C) 2009 by Renaud Guezennec                             *
*   http://www.rolisteam.org/contact                   *
*                                                                         *
*   Rolisteam is free software; you can redistribute it and/or modify     *
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
#include "cleveruri.h"
#include <QString>
#include <QDebug>


#include <QDataStream>
#include <QFileInfo>


#include "preferences/preferencesmanager.h"

/////////////////
// CleverUri
/////////////////
QHash<CleverURI::ContentType,QString> CleverURI::m_iconPathHash={
    {CleverURI::NONE,""},
    {CleverURI::MAP,":/map.png"},
    {CleverURI::VMAP,":/vmap.png"},
    {CleverURI::CHAT,":/resources/icons/scenario.png"},
    {CleverURI::PICTURE,":/resources/icons/photo.png"},
    {CleverURI::ONLINEPICTURE,":/resources/icons/photo.png"},
    {CleverURI::TEXT,":/notes.png"},
    {CleverURI::CHARACTERSHEET,":/resources/icons/treeview.png"},
    {CleverURI::SCENARIO,":/story.png"},
    {CleverURI::SONG,":/resources/icons/audiofile.svg"},
    #ifdef WITH_PDF
    {CleverURI::PDF,":/iconpdf"},
    #endif
    {CleverURI::SONGLIST,":/resources/icons/playlist.svg"}
};

//enum ContentType {NONE,MAP,VMAP,CHAT,PICTURE,ONLINEPICTURE,TEXT,CHARACTERSHEET,SCENARIO,SONG,SONGLIST
QStringList CleverURI::m_typeNameList = QStringList() <<    QObject::tr("None") <<QObject::tr("Map") <<QObject::tr("Vectorial Map") <<QObject::tr("Chat")
                                                      <<    QObject::tr("Picture") <<QObject::tr("Online Picture") <<QObject::tr("Text") <<QObject::tr("Text") <<
                                                            QObject::tr("Charecter Sheet") <<QObject::tr("Scenario") <<QObject::tr("Song") <<QObject::tr("Song List");

QStringList CleverURI::m_typeToPreferenceDirectory = QStringList() <<   QString("SessionDirectory") <<QString("MapDirectory")       <<QString("MapDirectory")           <<QString("ChatDirectory")
                                                                   <<   QString("ImageDirectory")   <<QString("ImageDirectory")     <<QString("MinutesDirectory")       <<QString("CharacterSheetDirectory") <<
                                                                        QString("SessionDirectory") <<QString("MusicDirectoryPlayer")   <<QString("MusicDirectoryPlayer");
CleverURIListener* CleverURI::s_listener = NULL;

CleverURI::CleverURI()
    : m_type(NONE),m_state(Remain)
{
    init();
}

CleverURI::CleverURI(const CleverURI & mp)
{
    m_type=mp.getType();
    m_uri=mp.getUri();
    m_currentMode = mp.getCurrentMode();
    m_name = mp.name();
    m_state = mp.getState();
    setUpListener();

}
QIcon CleverURI::getIcon()
{
    return QIcon(m_iconPathHash[m_type]);
}

CleverURI::CleverURI(QString name,QString uri,ContentType type)
    : m_uri(uri),m_type(type),m_state(Remain)
{
    m_name = name;
    setUpListener();
    init();
}

CleverURI::~CleverURI()
{
    // qDebug() << "Destructor of CleverURI";
}
bool CleverURI::operator==(const CleverURI& uri) const
{
    if((uri.getUri()==getUri())&&(uri.getType()==getType()))
        return true;
    return false;
}

void CleverURI::setUri(QString& uri)
{
    m_uri=uri;
    setUpListener();
}

void CleverURI::setType(CleverURI::ContentType type)
{
    m_type=type;
}

const QString CleverURI::getUri() const
{
    return m_uri;
}

CleverURI::ContentType CleverURI::getType() const
{
    return m_type;
}
bool CleverURI::hasChildren() const
{
    return false;
}
void CleverURI::setUpListener()
{
    if(NULL!=s_listener)
    {
        s_listener->cleverURIHasChanged(this);
    }
}

void CleverURI::init()
{
     PreferencesManager* preferences=PreferencesManager::getInstance();
     m_currentMode = (LoadingMode)preferences->value(QStringLiteral("DefaultLoadingMode"),(int)Linked).toInt();
}

CleverURI::State CleverURI::getState() const
{
    return m_state;
}

void CleverURI::setState(const State &state)
{
    m_state = state;
}

bool CleverURI::hasData() const
{
    return !m_data.isEmpty();
}


void CleverURI::setListener(CleverURIListener *value)
{
    s_listener = value;
}


QByteArray CleverURI::getData() const
{
    return m_data;
}

void CleverURI::setData(const QByteArray &data)
{
    m_data = data;
}

bool CleverURI::isDisplayed() const
{
    return (m_state == Displayed);
}

void CleverURI::setDisplayed(bool displayed)
{
    if(displayed)
    {
        m_state = Displayed;
    }
    else
    {
        m_state = Opened;
    }

    if(NULL!=s_listener)
    {
        s_listener->cleverURIHasChanged(this);
    }

}

CleverURI::LoadingMode CleverURI::getCurrentMode() const
{
    return m_currentMode;
}

void CleverURI::setCurrentMode(const LoadingMode &currentMode)
{
    m_currentMode = currentMode;
}
const QString CleverURI::getAbsolueDir() const
{
    QFileInfo info(m_uri);
    
    return info.absolutePath();
}

void CleverURI::write(QDataStream &out) const
{
    out << QStringLiteral("CleverUri");
    QByteArray data;
    if(m_data.isEmpty())
    {
        loadFileFromUri(data);
    }
    else
    {
        data = m_data;
    }
    out << (int)m_type << m_uri << m_name << (int)m_currentMode << data << (int)m_state;
}

void CleverURI::read(QDataStream &in)
{
    int type;
    int mode;
    int state;
    in >> type >> m_uri >> m_name >> mode >> m_data >> state;
    m_type = (CleverURI::ContentType)type;
    m_currentMode = (CleverURI::LoadingMode)mode;
    m_state = (CleverURI::State)state;
    setUpListener();
   /* if(QFile::exists(m_uri))
    {
        m_data.clear();
    }
    else
    {
        m_uri.clear();
    }*/
}

QString CleverURI::getFilterForType(CleverURI::ContentType type) //static
{
    PreferencesManager* preferences=PreferencesManager::getInstance();
    switch(type)
    {
    case CleverURI::CHARACTERSHEET:
       return  QObject::tr("Character Sheets files  (%1)").arg(preferences->value("CharacterSheetFileFilter","*.rcs").toString());
        break;
    case CleverURI::PICTURE:
        return QObject::tr("Supported Image formats (%1)").arg(preferences->value("ImageFileFilter","*.jpg *.jpeg *.png *.bmp *.svg").toString());
        break;
    case CleverURI::MAP:
        return QString();
        //return QObject::tr("Supported Map formats (%1)").arg(preferences->value("MapFileFilter","*.pla").toString());
        break;
    case CleverURI::CHAT:
        return QString();
        break;
    case CleverURI::TEXT:
        return QObject::tr("Supported Text Files (%1)").arg(preferences->value("TextFileFilter","*.odt *.html *.txt").toString());
        break;
    case CleverURI::SCENARIO:
        return QObject::tr("Supported Story Files (%1)").arg(preferences->value("StoryFileFilter","*.sce").toString());
        break;
    case CleverURI::SONG:
        return QObject::tr("Supported Audio formats (%1)").arg(preferences->value("AudioFileFilter","*.wav *.mp2 *.mp3 *.ogg *.flac").toString());
        break;
    case CleverURI::ONLINEPICTURE:
        return QString();
        break;
    case CleverURI::VMAP:
        return QString();
        //return QObject::tr("Supported Vmap formats (%1)").arg(preferences->value("VMapFileFilter","*.vmap").toString());
        break;
#ifdef WITH_PDF
    case CleverURI::PDF:
        return QObject::tr("Pdf File (%1)").arg(preferences->value("PdfFileFilter","*.pdf").toString());;
        break;
#endif
    default:
        return QString();
        break;
    }
}
QString CleverURI::typeToString(CleverURI::ContentType type)
{
    if(m_typeNameList.size()>(int)type)
    {
        return m_typeNameList.at((int)type);
    }
    return QString();
}

QString CleverURI::getPreferenceDirectoryKey(CleverURI::ContentType type)
{
    if(m_typeToPreferenceDirectory.size()>(int)type)
    {
        return m_typeToPreferenceDirectory.at((int)type);
    }
    return QString();
}
void CleverURI::loadFileFromUri(QByteArray& array) const
{
    QFile file(m_uri);
    if(file.open(QIODevice::ReadOnly))
    {
        array = file.readAll();
    }
}

void CleverURI::clearData()
{
    m_data.clear();
}
bool CleverURI::seekNode(QList<ResourcesNode*>& path,ResourcesNode* node)
{
    return false;
}

QVariant CleverURI::getData(ResourcesNode::DataValue i)
{

    switch(i)
    {
    case NAME:
        return m_name;
    case MODE:
        return m_currentMode==Internal ? QObject::tr("Internal") : QObject::tr("Linked");
    case DISPLAYED:
        return m_state == Displayed ? QObject::tr("true"):QObject::tr("false");
    case URI:
        return m_uri;
    }
}

QDataStream& operator<<(QDataStream& out, const CleverURI& con)
{
    out << con.getUri();
    out << (int)con.getType();
    return out;
}

QDataStream& operator>>(QDataStream& is,CleverURI& con)
{
    QString str;
    is >> str;

    int type;
    is >> type;


    con.setType((CleverURI::ContentType)type);
    con.setUri(str);
    return is;
}

QDataStream& operator<<(QDataStream& out, const CleverUriList& con)
{
    out << con.size();
    foreach(CleverURI uri, con)
    {
        out << uri;
    }
    return out;
}

QDataStream& operator>>(QDataStream& is,CleverUriList& con)
{
    int count;
    is >> count;
    for(int i = 0;i<count;++i)
    {
        CleverURI* uri = new CleverURI();
        is >> *uri;
        con.append(*uri);
    }
    return is;
}
