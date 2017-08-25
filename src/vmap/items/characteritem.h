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
#ifndef CHARACTERITEM_H
#define CHARACTERITEM_H
#include <QAction>



#include "visualitem.h"
#include "data/person.h"
#include "data/charactervision.h"
#include "data/characterstate.h"

/**
    * @brief represents any character on map.
    */
class CharacterItem : public VisualItem
{
    Q_OBJECT
public:
    /**
     * @brief CharacterItem
     */
    CharacterItem();
    /**
     * @brief CharacterItem
     * @param m
     * @param center
     * @param diameter
     */
    CharacterItem(Character* m,QPointF center,int diameter = 40);
    /**
    * @brief serialisation function to write data
    */
    virtual void writeData(QDataStream& out) const;
    /**
    * @brief serialisation function to read data.
    */
    virtual void readData(QDataStream& in);
    /**
     * @brief getType
     * @return
     */
    virtual VisualItem::ItemType getType() const;
    
    /**
    * @brief gives the bounding rect of the ellipse
    */
    virtual QRectF boundingRect() const ;
    /**
     * @brief shape
     * @return
     */
	virtual QPainterPath shape() const;
    /**
    * @brief modifies the ellipse size and shape.
    */
    virtual void setNewEnd(QPointF& nend);
    /**
    * @brief paint the ellipse at the correct position
    */
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

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
    void setGeometryPoint(qreal pointId, QPointF &pos);
    /**
     * @brief initChildPointItem
     */
    virtual void initChildPointItem();
    /**
     * @brief resizeContents
     * @param rect
     * @param keepRatio
     */
    void resizeContents(const QRectF& rect, bool keepRatio);
    /**
     * @brief updateChildPosition
     */
    void updateChildPosition();
    /**
     * @brief itemChange
     * @param change
     * @param value
     * @return
     */
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /**
     * @brief addActionContextMenu
     */
    virtual void addActionContextMenu(QMenu*);
    /**
     * @brief getItemCopy
     * @return
     */
	virtual VisualItem* getItemCopy();
    /**
     * @brief getCharacterId
     * @return
     */
    QString getCharacterId() const;
    /**
     * @brief getParentId
     * @return
     */
    QString getParentId() const;
    /**
     * @brief getCenter
     * @return
     */
    const QPointF& getCenter() const;
    /**
     * @brief getRadius
     * @return
     */
    int getRadius() const;
    /**
     * @brief addChildPoint
     * @param item
     */
    void addChildPoint(ChildPointItem* item);
    /**
     * @brief getChildPointCount
     * @return
     */
    int getChildPointCount() const;
    /**
     * @brief setDefaultVisionParameter
     * @param radius
     * @param angle
     */
    void setDefaultVisionParameter(CharacterVision::SHAPE, qreal radius, qreal angle);
    /**
     * @brief getVision
     * @return
     */
    CharacterVision* getVision() const;
    /**
     * @brief getRadiusChildWidget
     */
    ChildPointItem* getRadiusChildWidget();
    /**
     * @brief isLocal
     * @return
     */
    virtual bool isLocal() const;
    /**
     * @brief isPlayableCharacter
     * @return
     */
    bool isPlayableCharacter();
    /**
     * @brief setSize
     * @param size
     */
    virtual void setSize(QSizeF size);
    /**
     * @brief setCharacterIsMovable if isMovable is true and the character is local then the item is mavable
     * @param isMovable
     */
    void setCharacterIsMovable(bool isMovable);
    /**
     * @brief isNpc
     * @return
     */
    bool isNpc();
    /**
     * @brief setRectSize
     * @param x
     * @param y
     * @param w
     * @param h
     */
    virtual void setRectSize(qreal x, qreal y, qreal w, qreal h);
    /**
     * @brief readCharacterStateChanged
     * @param msg
     */
    void readCharacterStateChanged(NetworkMessageReader &msg);
    /**
     * @brief getColor
     * @return
     */
    virtual QColor getColor();
    /**
     * @brief readVisionMsg
     * @param msg
     */
    void readVisionMsg(NetworkMessageReader *msg);
    /**
     * @brief sendVisionMsg
     */
    void sendVisionMsg();
    /**
     * @brief setChildrenVisible
     * @param b
     */
    void setChildrenVisible(bool b);
    /**
     * @brief setEditableItem
     * @param b
     */
    void setEditableItem(bool b);
signals:
    /**
     * @brief positionChanged
     */
    void positionChanged();
    /**
     * @brief geometryChangeOnUnkownChild
     * @param pointId
     * @param F
     */
    void geometryChangeOnUnkownChild(qreal pointId, QPointF& F);


    /**
     * @brief localItemZValueChange
     */
    void localItemZValueChange(CharacterItem*);
    /**
     * @brief ownerChanged
     * @param old
     */
    void ownerChanged(Character* old,CharacterItem*);

public slots:
    /**
     * @brief characterHasBeenDeleted
     */
    void characterHasBeenDeleted(Character*);
    /**
     * @brief changeVisionShape
     */
	void changeVisionShape();
    /**
     * @brief sizeChanged
     * @param m_size
     */
    void sizeChanged(int m_size);
    /**
     * @brief readPositionMsg
     * @param msg
     */
    virtual void readPositionMsg(NetworkMessageReader* msg);
    /**
     * @brief endOfGeometryChange
     */
    void endOfGeometryChange();
    /**
     * @brief generatedThumbnail
     */
    void generatedThumbnail();
protected:
    /**
     * @brief canBeMoved
     * @return
     */
    virtual bool canBeMoved() const;
private slots:
    /**
     * @brief createActions
     */
    void createActions();
    /**
     * @brief characterStateChange
     */
    void characterStateChange();
    /**
     * @brief changeCharacter
     */
	void changeCharacter();
private:
    /**
     * @brief getSubTitle
     * @return
     */
    QString getSubTitle() const;

private:
    Character* m_character;
    QPointF m_center;
    int m_diameter;
    QPixmap* m_thumnails;
    QRectF m_rectText;
	QString m_title;

    //QAction*
	QAction* m_visionShapeDisk;
	QAction* m_visionShapeAngle;

    //sight
    CharacterVision* m_vision;


    QPointF m_oldPosition;
    QPainterPath m_debugPath;
    QPainterPath m_debugPath2;

    bool m_protectGeometryChange;
    bool m_visionChanged;
    void visionChanged();
};

#endif // CHARACTERITEM_H
