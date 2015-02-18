/*************************************************************************
 *     Copyright (C) 2007 by Romain Campioni                             *
 *     Copyright (C) 2009 by Renaud Guezennec                            *
 *     Copyright (C) 2010 by Berenger Morel                              *
 *     Copyright (C) 2011 by Joseph Boudou                               *
 *                                                                       *
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


#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QTextEdit>
#include <QMenu>
#include <QWorkspace>

#include "NouveauPlanVide.h"
#include "DessinPerso.h"
#include "initialisation.h"
#include "preferencesmanager.h"

#ifndef NULL_PLAYER
#include "LecteurAudio.h"
#endif

class UpdateChecker;
class BarreOutils;
class CarteFenetre;
class Carte;
class ChatListWidget;
class EditeurNotes;
class Image;
class Liaison;
class ListeUtilisateurs;
class PreferencesDialog;
class Player;
class PlayersListWidget;
class WorkspaceAmeliore;
class ClientServeur;
class TextEdit;

/**
 * @brief Fenêtre principale, contenant la palette d'outils, les tableaux de
 * joueurs et de PJ, les cartes et les fenêtres de dialogue.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public :

    ~MainWindow();
    static MainWindow* getInstance();
    void majCouleursPersonnelles();
    void ajouterCarte(CarteFenetre *carteFenetre, QString titre,QSize mapsize=QSize(),QPoint pos=QPoint());
    void ajouterImage(Image *imageFenetre, QString titre);
    void mettreAJourEspaceTravail();
    Carte *trouverCarte(QString idCarte);
    CarteFenetre *trouverCarteFenetre(QString idCarte);
    Image *trouverImage(QString idImage);
    bool estLaFenetreActive(QWidget *widget);
    bool enleverCarteDeLaListe(QString idCarte);
    bool enleverImageDeLaListe(QString idImage);

    void registerSubWindow(QWidget * subWindow);
    bool showConnectionDialog();
    void setupUi();
    NouveauPlanVide::PermissionMode getPermission(int id);
    void readSettings();
    void writeSettings();

    static void notifyUser(QString msg);

    void notifyUser_p(QString msg);


signals:
    void closing();

public slots :
    void creerNouveauPlanVide(QString titre, QString idCarte, QColor couleurFond, quint16 largeur, quint16 hauteur,quint8);
    void aucunNouveauPlanVide();
    void afficherEditeurNotes(bool afficher, bool cocherAction = false);
    void quitterApplication(bool perteConnexion = false);
    void checkUpdate();
    void setNetworkManager(ClientServeur*);
    void updateUi();
    void updateWindowTitle();

protected :
    void closeEvent(QCloseEvent *event);

private :
    MainWindow();
    static MainWindow* m_singleton;
    void creerLogUtilisateur();
    void creerMenu();
    void linkActionToMenu();

    void lireCarteEtPnj(QDataStream &file, bool masquer = false, QString nomFichier = "");
    void lireImage(QDataStream &file);
    void sauvegarderTousLesPlans(QDataStream &file);
    void sauvegarderToutesLesImages(QDataStream &file);



    /**
     * @brief Add a health state to a list
     *
     * @param &color QColor reference to use to initialise the state
     * @param &label QString reference that contain... the label of the state
     * @param &listHealthState reference to the list
     * @return Whether or not the windows was successfully hidden.
     */
    void AddHealthState(const QColor &color, const QString &label, QList<DessinPerso::etatDeSante> &listHealthState);

    /**
     * @brief Initialize a mouse pointer
     *
     * @param *pointer QCursor pointer to the cursor to initialise
     * @param iconFileName Filename of the icon to use
     * @param hotX same as in QCursor's constructor
     * @param hotY same as in QCursor's constructor
     * @return Whether or not the windows was successfully hidden.
     */
    void InitMousePointer(QCursor **pointer, const QString &iconFileName, const int hotX, const int hotY);


    WorkspaceAmeliore *workspace;
    ListeUtilisateurs* m_listeUtilisateurs;
    PlayersListWidget * m_playersList;
    QMenu *menuFenetre;
    BarreOutils *m_toolBar;
    NouveauPlanVide *fenetreNouveauPlan;
    EditeurNotes *editeurNotes;
    QList <CarteFenetre *> listeCarteFenetre;
    QList <Image *> listeImage;
#ifndef NULL_PLAYER   
    LecteurAudio* m_audioPlayer;
#endif

    PreferencesDialog * m_preferencesDialog;

    ChatListWidget * m_chatListWidget;

    QAction* actionNouveauPlan;
    QAction* actionOuvrirImage;
    QAction* actionOuvrirPlan;
    QAction* actionOuvrirEtMasquerPlan;
    QAction* actionOuvrirScenario;
    QAction* actionOuvrirNotes;
    QAction* actionFermerPlan;
    QAction* actionSauvegarderPlan;
    QAction* actionSauvegarderScenario;
    QAction* actionSauvegarderNotes;
    QAction* actionPreferences;
    QAction* actionQuitter;

    QAction* m_reconnectAct;
    QAction* m_disconnectAct;

    QAction *actionAfficherNomsPj;
    QAction *actionAfficherNomsPnj;
    QAction *actionAfficherNumerosPnj;
    QAction *actionSansGrille;
    QAction *actionCarre;
    QAction *actionHexagones;

    QAction *actionCascade;
    QAction *actionTuiles;
    QAction *actionEditeurNotes;

    QAction *actionAideLogiciel;
    QAction *actionAPropos;
    UpdateChecker* m_updateChecker;

    QString m_version;
    QDockWidget* m_dockLogUtil;
    PreferencesManager* m_preferences;

    ClientServeur* m_networkManager;

    QTextEdit* m_notifierDisplay;

    TextEdit* m_noteEditor;
//#ifndef NULL_PLAYER
//    QDockWidget* m_audioDock;
//#endif
private slots :
    void changementNatureUtilisateur();
    void afficherNomsPj(bool afficher);
    void afficherNomsPnj(bool afficher);
    void afficherNumerosPnj(bool afficher);
    void changementFenetreActive(QWidget *widget);
    void nouveauPlan();
    void ouvrirPlan(bool masquer = false);
    void ouvrirImage();
    void ouvrirEtMasquerPlan();
    void ouvrirScenario();
    void ouvrirNotes();
    void fermerPlanOuImage();
    void sauvegarderPlan();
    void updateMayBeNeeded();
    void emettreTousLesPlans(Liaison * link);
    void emettreToutesLesImages(Liaison * link);


    //Network private Slot
    void stopReconnection();
    void closeConnection();
    void startReconnection();

    /**
    * \brief Show the about dialog
    *
    */
    void aPropos();

    /// \brief open the Qt assistant with the rolisteam documentation
    void helpOnLine();
    bool sauvegarderScenario();
    bool sauvegarderNotes();

    void notifyAboutAddedPlayer(Player * player) const;
    void notifyAboutDeletedPlayer(Player * player) const;
};

#endif