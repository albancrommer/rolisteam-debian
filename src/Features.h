/*************************************************************************
 *     Copyright (C) 2010 by Joseph Boudou                               *
 *     Copyright (C) 2014 by Renaud Guezennec                            *
 *     http://www.rolisteam.org/                                         *
 *                                                                       *
 *   Rolisteam is free software; you can redistribute it and/or modify   *
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


#ifndef FEATURES_H
#define FEATURES_H

#include <QMapIterator>
#include <QString>

#include "network/networkmessagewriter.h"

class ReceiveEvent;
class Player;


extern void setLocalFeatures(Player & player);
extern void addFeature(ReceiveEvent & data);
/**
 * @brief The SendFeaturesIterator class is use to check if all players connected to the same server have the same features.
 */
class SendFeaturesIterator : public QMapIterator<QString, quint8>
{
public:
    SendFeaturesIterator();
    SendFeaturesIterator(const Player & player);
    ~SendFeaturesIterator();

    NetworkMessageWriter & message();

    SendFeaturesIterator & operator=(const Player * player);

private:
    const Player * m_player;
    NetworkMessageWriter m_message;
};

#endif
