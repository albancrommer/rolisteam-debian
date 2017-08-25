/***************************************************************************
 *	Copyright (C) 2007 by Romain Campioni   			   *
 *	Copyright (C) 2009 by Renaud Guezennec                             *
 *   http://www.rolisteam.org/contact                   *
 *                                                                         *
 *   rolisteam is free software; you can redistribute it and/or modify  *
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

#ifndef IMAGE_H
#define IMAGE_H

#include <QWidget>
#include <QImage>
#include <QScrollArea>
#include <QAction>
#include <QFile>
#include <QString>
#include <QLabel>

#include "improvedworkspace.h"
#include "mainwindow.h"
#include "toolsbar.h"
#include "preferences/preferencesmanager.h"

#include "data/mediacontainer.h"

class NetworkLink;
class NetworkMessageWriter;
class QShortcut;
/**
 * @brief The Image class displays image to the screen. It also manages image from Internet and the zooming.
 */
class Image : public MediaContainer
{
Q_OBJECT

public :
    //Image(QString title, QString identImage, QString identJoueur, QImage *image, QAction *action = 0, ImprovedWorkspace *parent = 0);
    Image(ImprovedWorkspace* parent = NULL);
    /**
     * @brief ~Image destructor.
     */
    virtual ~Image();
    /**
     * @brief setInternalAction may not be useful anymore.
     * @param action
     */
    void setInternalAction(QAction *action);
    QAction* getAssociatedAction() const;

    void fill(NetworkMessageWriter & message) const;
    void saveImageToFile(QFile &file);
    void saveImageToFile(QDataStream& out);
    bool isImageOwner(QString id);
    void setParent(ImprovedWorkspace *parent);
    void setImage(QImage& img);
    void setIdOwner(QString);

	virtual bool readFileFromUri();
    virtual bool openMedia();
    virtual void saveMedia();

    virtual void putDataIntoCleverUri();

public slots :
    void pointeurMain();
    void pointeurNormal();
    void setCurrentTool(ToolsBar::SelectableTool tool);

protected:
    /**
    * @brief called when users roll the wheel of their mouse
    * @param event : define few parameters about the action (way,..)
    */
    void wheelEvent(QWheelEvent *event);

    /**
     * @brief resizeEvent make sure the window keep the right ratio.
     */
    void resizeEvent(QResizeEvent *event);

   // void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void contextMenuEvent ( QContextMenuEvent * event );
    void paintEvent ( QPaintEvent * event );
private slots:
    /**
    * @brief generic function to set the zoom level and refresh the picture
    * @param zoomlevel : new zoomlevel value
    */
    void setZoomLevel(double zoomlevel);
    /**
    * @brief default function to zoomIn, Add 0.2 to the zoom level and then refresh the picture
    */
    void zoomIn();
    /**
    * @brief default function to zoomOut, substract 0.2 to the zoom level and then refresh the picture
    */
    void zoomOut();
    /**
    * @brief refresh the size of the label (which embeds the picture)
    */
    void resizeLabel();
    /**
    * @brief sets the zoomlevel to 0.2
    */
    void zoomLittle();
    /**
    * @brief sets the zoom level to 1
    */
    void zoomNormal();
    /**
    * @brief sets the zoom level to 4
    */
    void zoomBig();
    /**
    * @brief resize the window and sets current size as zoomlevel 1.
    */
    void onFitWorkSpace();

    /**
     * @brief fitWindow set the size of the picture at the best size of the window.
     */
    void fitWindow();

private :
    /**
    * @brief adapt the size window to fit the MdiArea size and no scrollbar (if possible)
    */
    void fitWorkSpace();
    void createActions();
    void initImage();
private :
    QString m_idPlayer;
    QLabel* m_imageLabel;
    QPoint m_startingPoint;
    int m_horizontalStart;
    int m_verticalStart;
    bool m_allowedMove;
    double m_zoomLevel;
    ImprovedWorkspace* m_parent;

    QSize m_NormalSize;
    QSize m_windowSize;
    QPixmap  m_pixMap;

    QAction* m_actionZoomIn;
    QShortcut* m_zoomInShort;

    QAction* m_actionZoomOut;
    QShortcut* m_zoomOutShort;


    QAction* m_actionfitWorkspace;
    QShortcut* m_fitShort;

    QAction* m_actionNormalZoom; // *1
    QShortcut* m_normalShort;

    QAction* m_actionBigZoom;// * 4
    QShortcut* m_bigShort;

    QAction* m_actionlittleZoom;// * 0.2
    QShortcut* m_littleShort;

    //fit window keeping ratio
    QAction* m_fitWindowAct;// * 0.2
    QShortcut* m_fitWindowShort;

    double m_ratioImage;
    double m_ratioImageBis;

    ToolsBar::SelectableTool m_currentTool;

    QScrollArea* m_widgetArea;

};

#endif
