/***************************************************************************
    *      Copyright (C) 2010 by Renaud Guezennec                             *
    *                                                                         *
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
#ifndef RECTITEM_H
#define RECTITEM_H

#include "visualitem.h"
/**
    * @brief displays and manages rectangle on map, part of QGraphicsScene/view.
    */
class RectItem : public VisualItem
{
public:
    RectItem();
    /**
    * @brief Constructor with parameters.
    * @param topleft corner of the rectangle.
    * @param buttom right corner of the rectangle
    * @param filled either we paint filled rectange or just uts border
    * @param color
    */
	RectItem(QPointF& topleft,QPointF& buttomright,bool filled,quint16 penSize,QColor& penColor,QGraphicsItem * parent = 0);
    /**
    * @brief paint the current rectangle into the scene.
    * @see Qt documentation
    */
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    
    /**
    * @brief gives bounding rect. Return rect geometry into the QRectF
    */
    virtual QRectF boundingRect() const ;
    
    /**
    * @brief defines new end point.
    */
    virtual void setNewEnd(QPointF& nend);
    /**
    * @brief serialization function to write data.
    */
    virtual void writeData(QDataStream& out) const;
    /**
    * @brief serialization function to read data from stream.
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
     * @brief shape
     * @return
     */
    virtual QPainterPath shape() const;
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
	 * @brief getItemCopy
	 * @return
	 */
    virtual VisualItem* getItemCopy();

    /**
     * @brief resizeContents
     */
    virtual void resizeContents(const QRectF&,bool);


protected:
    /**
     * @brief updateChildPosition
     */
    virtual void updateChildPosition();


private:

	quint16 m_penWidth;
    bool m_initialized;
    /**
    * @brief stores the filled state.
    */
    bool m_filled;
};

#endif // RECTITEM_H
