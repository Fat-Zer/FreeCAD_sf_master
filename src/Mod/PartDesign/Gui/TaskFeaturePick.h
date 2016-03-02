/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *   Copyright (c)2016 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>        *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef PARTDESIGNGUI_FeaturePickDialog_H
#define PARTDESIGNGUI_FeaturePickDialog_H

# include <boost/bimap.hpp>

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/Widgets.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/ViewProviderOrigin.h>
#include <App/DocumentObject.h>


class QTreeWidget;
class QTreeWidgetItem;


namespace PartDesignGui {

class SoSwitch;
class Ui_TaskFeaturePick;

class FeaturePickSelectWidget;

/**
 * The class used to control which features are useable and which are not and display them in the right manner
 */
class FeaturePicker : public QObject {
    Q_OBJECT

public:
    /// The status of the picked feature
    enum FeatureStatus {
        validFeature = 0, ///< Valid and ok to be used
        isUsed,           ///< Already used by some other
        isExternal,       ///< External against the current context, @see otherBody @see otherPart
        otherBody,        ///< Doesn't belongs to current body
        otherPart,        ///< Doesn't belongs to current part
        afterTip,         ///< The feature is located after the current Tip
        invalidShape,     ///< Inappropriate shape for the task
        // BasePlane specific
        basePlane,        ///< Is a baseplane
        // Sketch specific
        noWire,           ///< The featuer (likely sketch) has no wire
//        userSelected,     ///< The feature is preapprooved (ok)
        FEATURE_STATUS_MAX
    };


    /// A help type for store set of statuses
    typedef std::bitset<FEATURE_STATUS_MAX> StatusSet;

    /**
     * Constructor
     * @param parent      a Qt parent of the object
     * @param s_multiPick if true configure the picker to select multiple objects
     */
    explicit FeaturePicker ( bool s_multiPick = false, QObject * parent = 0 );

    /**
     * Set a status of a feature and adds it to the picker
     * @note This function may be used to either add a feature and alter already added feature
     *
     *  @param obj     The feature
     *  @param status  The status of this feature
     */
    void setFeatureStatus ( App::DocumentObject *obj, StatusSet status );

    /**
     * This function validates and adds according to their content and relation
     * to active body.
     *
     * @param obj the sketch to determin the status and add
     */
    void addSketch ( App::DocumentObject *obj ) {
        setFeatureStatus (obj, sketchStatus (obj));
    }

    /// this function determines the status of the sketch and returns it
    static StatusSet sketchStatus ( App::DocumentObject *obj );

    /**
     * This function verifies the relation of given feature against the given body.
     */
    static StatusSet bodyRelationStatus ( App::DocumentObject *obj, PartDesign::Body *body );

    /**
     * This overloaded function verifies the position of given feature against the active body.
     */
    static StatusSet bodyRelationStatus ( App::DocumentObject *obj );


    /// @name Status manipulators
    ///@{

    /// Returns a string describing the given status
    static QString getFeatureStatusString ( FeatureStatus status );

    /**
     * Returns the status of given object object.
     * If the object is not registred in the picker empty bitset is returned
     */
    StatusSet getStatus ( App::DocumentObject * obj ) const;

    /// Returns all statuses that added features has
    StatusSet getFeatureStatusMask ( ) const;

    /// Returns all features which has at least the given status set
    std::set < App::DocumentObject * > getFeaturesWithStatus ( FeatureStatus status ) const;

    /**
     * Returns all features which has at least the given status
     * @param status the set of statuses to search
     */
    std::set < App::DocumentObject * > getFeaturesWithStatus ( StatusSet status ) const;


    /**
     * Returns all features which has exact the given status
     * @param status the set of statuses to search
     */
    std::set < App::DocumentObject * > getFeaturesWithExactStatus ( StatusSet status ) const;

    /**
     * Returns added features with associated statuses
     * @param status the set of statuses to search
     */
    const std::map < App::DocumentObject *, StatusSet> & getFeaturesStatusMap ( ) const {
        return statusMap;
    }
    ///@}

    /**
     * If any bit in a feature's status is not set in the visability set when the feature
     * would be hidden in the tree view.
     * @return true if features with given status are set to be displayed
     */
    bool isVisible (FeaturePicker::FeatureStatus status) const {
        return visability[status];
    }

    /**
     * If any bit in a feature's status is not set in the visability set when the feature
     * would be hidden in the tree view.
     * @return true if features with given status set are ment to be displayed
     */
    bool isVisible (FeaturePicker::StatusSet status) const {
        return ( status & ~visability ).none ();
    }

    /// Retruns true if given feature should be visible
    bool isVisible (App::DocumentObject *feat) const;

    /// Sets the visability of features with the given status
    void setVisability (FeaturePicker::FeatureStatus status, bool state);

    /// Set a single features picked by the user
    void setPickedFeature (App::DocumentObject * feature) {
        assert (feature);
        if (pickedFeatures.size() != 1 || pickedFeatures.front () != feature) {
            pickedFeatures.clear();
            pickedFeatures.push_back (feature);
            Q_EMIT pickedFeaturesChanged ();
        }
    }

    /// Resets the picked features
    void resetPickedFeature () {
        pickedFeatures.clear();
    }

    /// Set the features picked by the user
    void setPickedFeatures (const std::vector <App::DocumentObject *> & features) {
        if ( multiPick && !features.empty() ) {
            setPickedFeature (features.front());
        } else if (features != pickedFeatures) {
            pickedFeatures = features;
            Q_EMIT pickedFeaturesChanged ();
        }
    }

    /// Returns the set of picked features
    const std::vector <App::DocumentObject *>& getPickedFeatures () const {
        return pickedFeatures;
    }

    /// Returns whether multiple feature picking is enabled
    bool isMultiPick ()
    { return multiPick; }
Q_SIGNALS:
    /// Emmited when visability for specific status changed
    void visabilityChanged (PartDesignGui::FeaturePicker::FeatureStatus status, bool state);

    /// Emmited after a feature status getting set
    void featureStatusSet (App::DocumentObject *, PartDesignGui::FeaturePicker::StatusSet);

    /// Emmited on picked features are getting set
    void pickedFeaturesChanged ();
private:
    /// Returns set of features for which filter function returns true
    inline std::set < App::DocumentObject * > getFeaturesFilteredByStatus(
            std::function<bool (StatusSet)> filter ) const;

private:
    std::map<App::DocumentObject *, StatusSet> statusMap;

    StatusSet visability;

    std::vector <App::DocumentObject *> pickedFeatures;

    bool multiPick;
};


/**
 * A widget to display the content of FeaturePicker
 */
class FeaturePickSingleSelectWidget: public QTreeWidget {
    Q_OBJECT
public:
    FeaturePickSingleSelectWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );

protected Q_SLOTS:
    void onWidgetSelectionChanged ();
    void onPickedFeaturesChanged ();
    void onFeatureStatusSet (App::DocumentObject *feat, PartDesignGui::FeaturePicker::StatusSet status);
    void onVisabilityChanged ();

private:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat){
        return createTreeWidgetItem ( feat, picker->getStatus ( feat ) );
    }

    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat, const FeaturePicker::StatusSet &);

private:
    FeaturePicker *picker;

    typedef boost::bimap < App::DocumentObject *, QTreeWidgetItem *> TreeWidgetItemMap;
    TreeWidgetItemMap treeItems;
};

/**
 * A widget to display the content of FeaturePicker with two pannels
 */
class FeaturePickTwoPanelWidget: public Gui::ActionSelector {
    Q_OBJECT
public:
    FeaturePickTwoPanelWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );

protected Q_SLOTS:
    void onWidgetSelectionChanged ();
    void onPickedFeaturesChanged ();
    void onFeatureStatusSet (App::DocumentObject *feat, PartDesignGui::FeaturePicker::StatusSet status);
    void onVisabilityChanged ();

private:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat){
        return createTreeWidgetItem ( feat, picker->getStatus ( feat ) );
    }

    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat, const FeaturePicker::StatusSet &);

    /// Returns the features moved to selectedWidget mapped to DocumentObject*
    inline std::vector <App::DocumentObject *> getSelectedFeatures ();
private:
    FeaturePicker *picker;

    typedef boost::bimap < App::DocumentObject *, QTreeWidgetItem *> TreeWidgetItemMap;
    TreeWidgetItemMap treeItems;
};


/**
 * A reusable widget to control the FeaturePicker hide/show settings
 */
class FeaturePickControlWidget: public QWidget {
    Q_OBJECT
public:
     FeaturePickControlWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );
protected Q_SLOTS:
    /// A callback on status getting changed in the FeaturePicker
    void onVisabilityChanged ( PartDesignGui::FeaturePicker::FeatureStatus status, bool state );
    /// A callback on a button getting clicked
    void onStateButtonClicked ( bool state );
    /// Adjust button visability on feature getting added to the picker
    void onFeatureStatusSet ( );

private:
    FeaturePicker *picker;

    typedef boost::bimap< FeaturePicker::FeatureStatus, QToolButton *> ButtonsBimap;
    ButtonsBimap controlButtons;
};


/// A task for use in the associated dialog
class TaskFeaturePick : public Gui::TaskView::TaskBox//, public Gui::SelectionObserver
{
    Q_OBJECT
public:
     TaskFeaturePick(FeaturePicker *picker, QWidget *parent = 0);

// TODO Rewright (2015-12-09, Fat-Zer)
//
//     std::vector<App::DocumentObject*> getFeatures();
//     std::vector<App::DocumentObject*> buildFeatures();
//     void showExternal(bool val);
//
//     static App::DocumentObject* makeCopy(App::DocumentObject* obj, std::string sub, bool independent);
//
// protected Q_SLOTS:
//     void onUpdate(bool);
//     void onSelectionChanged(const Gui::SelectionChanges& msg);
//
// private:
//     Ui_TaskFeaturePick* ui;
//     QWidget* proxy;
//     SoSwitch* featureswitch;
//     std::vector<Gui::ViewProviderOrigin*> origins;
//
// //    std::vector<QString> features;
// //    std::vector<featureStatus> statuses;
//
//     void updateList();
//     const QString getFeatureStatusString(const featureStatus st);
};


/// simulation dialog for the TaskView
class TaskDlgFeaturePick : public Gui::TaskView::TaskDialog
{
    Q_OBJECT
public:
    TaskDlgFeaturePick (FeaturePicker *picker);
    /**
     * Checks wether there is a TaskDialog already running and creates and shows a new one
     * TaskDlgFeaturePick if not.
     *
     * @returns zero if the user accepts the dialog and nonzero value othervice
     */
    static int safeExecute ( FeaturePicker *picker );

// TODO Rewright (2015-12-09, Fat-Zer)
// protected:
//     TaskFeaturePick  *pick;
//     bool accepted;
//     boost::function<bool (std::vector<App::DocumentObject*>)>  acceptFunction;
//     boost::function<void (std::vector<App::DocumentObject*>)>  workFunction;
};

}

#endif // PARTDESIGNGUI_FeaturePickDialog_H
