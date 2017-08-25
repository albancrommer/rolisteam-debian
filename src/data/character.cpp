/***************************************************************************
    *   Copyright (C) 2015 by Renaud Guezennec                                *
    *   http://www.rolisteam.org/contact                   *
    *                                                                         *
    *   rolisteam is free software; you can redistribute it and/or modify     *
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
#include <QColor>
#include <QUuid>
#include <QDebug>
#include <QPixmap>
#include <QBuffer>

#include "character.h"
#include "data/player.h"
#include "network/networkmessagereader.h"
#include "network/networkmessagewriter.h"
//#include "userlist/playersList.h"

QList<CharacterState*>* Character::m_stateList = NULL;

Character::Character()
    :Person(), m_currentState(NULL),m_sheet(NULL)
{
	init();
}

Character::Character(const QString & nom, const QColor & color,bool npc,int number)
    : Person(nom, color), m_isNpc(npc),m_number(number),m_currentState(NULL),m_sheet(NULL)
{
	init();
}

Character::Character(const QString & uuid, const QString & nom, const QColor & color,bool npc,int number)
    : Person(uuid, nom, color),m_isNpc(npc),m_number(number),m_currentState(NULL),m_sheet(NULL)
{
	init();
}

Character::Character(NetworkMessageReader & data)
    : Person(), m_currentState(NULL),m_sheet(NULL)
{
    read(data);

    init();
}

Character::~Character()
{
}
void Character::init()
{
	if((NULL != m_stateList)&&(NULL == m_currentState)&&(!m_stateList->isEmpty()))
	{
		m_currentState = m_stateList->first();
	}
}

QString Character::getParentId() const
{
    if(NULL!=m_parent)
    {
        return m_parent->getUuid();
    }
    return QString();
}

QHash<QString, QString> Character::getVariableDictionnary()
{
    if(NULL==m_sheet)
    {
        return QHash<QString, QString>();
    }
    else
    {
        #ifndef UNIT_TEST
            return m_sheet->getVariableDictionnary();
        #endif
    }
}

CharacterSheet *Character::getSheet() const
{
    return m_sheet;
}

void Character::setSheet(CharacterSheet *sheet)
{
    m_sheet = sheet;
}
void Character::setListOfCharacterState(QList<CharacterState*>* list)
{
    m_stateList = list;
}
int Character::indexOf(CharacterState* state)
{
    return m_stateList->indexOf(state);
}

CharacterState* Character::getStateFromLabel(QString label)
{
    foreach(CharacterState* state, *m_stateList)
    {
        if(state->getLabel() == label)
        {
            return state;
        }
    }
    return NULL;
}

void Character::fill(NetworkMessageWriter & message,bool addAvatar)
{
    if(NULL!=m_parent)
    {
        message.string8(m_parent->getUuid());
    }
    else
    {
        message.string8("NULL");
    }
    message.string8(m_uuid);
    message.string16(m_name);
    message.int8(indexOf(m_currentState));
    message.uint8((int)m_isNpc);
    message.int32(m_number);
    message.rgb(m_color);

    if(addAvatar)
    {
        message.uint8((quint8)!m_avatar.isNull());
        if(!m_avatar.isNull())
        {
            QByteArray baImage;
            QBuffer bufImage(&baImage);
            if(m_avatar.save(&bufImage, "PNG", 70))
            {
                message.byteArray32(baImage);
            }
        }
    }
    else
    {
        message.uint8((quint8)false);
    }

}
QString Character::read(NetworkMessageReader& msg)
{
    QString parentId = msg.string8();
    m_uuid = msg.string8();
    m_name = msg.string16();
    int currentStateIndex = msg.int8();
    if(currentStateIndex>=0)
    {
        m_currentState = getStateFromIndex(currentStateIndex);
    }
    m_isNpc = (bool)msg.uint8();
    m_number = msg.int32();
    m_color = QColor(msg.rgb());

    bool hasAvatar = (bool) msg.uint8();

    if(hasAvatar)
    {
        m_avatar = QImage::fromData(msg.byteArray32());
    }

    return parentId;
}
CharacterState* Character::getStateFromIndex(int i)
{
    if(m_stateList->empty())
        return NULL;
    if(m_stateList->size()>i)
    {
        return m_stateList->at(i);
    }
}

Player* Character::getParentPlayer() const
{
    return dynamic_cast<Player*>(m_parent);
}

int Character::number() const
{
    return m_number;
}

bool Character::isNpc() const
{
    return m_isNpc;
}
void  Character::setState(CharacterState*  h)
{
  m_currentState = h;
}
CharacterState* Character::getState() const
{
    return m_currentState;
}
void Character::writeData(QDataStream& out) const
{
    out << m_uuid;
    out << m_name;
    if(NULL!=m_currentState)
    {
        out << true;
        out << m_currentState->getLabel();
    }
    else
    {
        out << false;
    }
    out << m_isNpc;
    out << m_number;
    out << m_color;
    out << m_avatar;
}
void Character::readData(QDataStream& in)
{
    in >> m_uuid;
    in >> m_name;
    bool hasState;
    in >> hasState;
    if(hasState)
    {
        QString value;
        in >> value;
        m_currentState= getStateFromLabel(value);
    }
    else
    {
        m_currentState = NULL;
    }
    in >> m_isNpc;
    in >> m_number;
    in >> m_color;
    in >> m_avatar;
}
QList<CharacterState*>* Character::getCharacterStateList()
{
	return m_stateList;
}
void Character::setNpc(bool b)
{
    m_isNpc = b;
}
