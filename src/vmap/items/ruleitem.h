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

#ifndef RULEITEM_H
#define RULEITEM_H

#include "visualitem.h"

#include "vmap/vmap.h"

#include <QPen>
/**
 * @brief The RuleItem class ephemeral item to display rule and measure the distance between two points.
 */
class RuleItem : public VisualItem
{
public:
    RuleItem();
    RuleItem(QPointF& p);

    ~RuleItem();

    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    /**
    * @brief gives bounding rect of the line
    */
    virtual QRectF boundingRect() const ;

    /**
    * @brief defines new position of the end line.
    */
    virtual void setNewEnd(QPointF& nend);
    /**
    * @brief serialisation writing
    */
    virtual void writeData(QDataStream& out) const;
    /**
    * @brief serialisation reading
    */
    virtual void readData(QDataStream& in);

    /**
     * @brief getType
     * @return
     */
    virtual VisualItem::ItemType getType() const;
    /**
     * @brief fillMessage
     * @param msg
     */
    virtual void fillMessage(NetworkMessageWriter* msg);
    /**
     * @brief readItem
     * @param msg
     */
    virtual void readItem(NetworkMessageReader* msg);
    /**
     * @brief setGeometryPoint
     * @param pointId
     * @param pos
     */
    virtual void setGeometryPoint(qreal pointId, QPointF &pos);
    /**
     * @brief initChildPointItem
     */
    virtual void initChildPointItem();
    /**
     * @brief setUnit
     * @param unit
     */
    void setUnit(VMap::SCALE_UNIT unit);

	/**
	 * @brief setPixelToUnit
	 * @param pixels
	 */
    void setPixelToUnit(qreal pixels);
	/**
	 * @brief setModifiers
	 * @param mod
	 */
    virtual  void setModifiers(Qt::KeyboardModifiers mod);
	/**
	 * @brief getItemCopy
	 * @return
	 */
	virtual VisualItem* getItemCopy();

    static void setZoomLevel(qreal );

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    /**
    * @brief bounding rect copy (no need to compute it each time
    */
    QRectF m_rect;
    /**
    * @brief starting point, does not move except when the whole line is moved.
    */
    QPointF m_startPoint;
    /**
    * @brief ending point, should moved
    */
    QPointF m_endPoint;
    /**
    * @brief pen
    */
    QPen m_pen;

    qreal m_pixelToUnit; //how many pixels make one unit ?

    QString m_unitText;
    Qt::KeyboardModifiers m_mod;

    static qreal m_zoomFactor;


};

#endif // RULEITEM_H
