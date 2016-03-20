/******************************************************************************
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

#ifndef FEATUREPICKER_H_SOTFW5WL
#define FEATUREPICKER_H_SOTFW5WL

#include <QObject>

namespace App {
    class DocumentObject;
}

namespace PartDesign {
    class Body;
}

namespace PartDesignGui {

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
    explicit FeaturePicker ( QObject * parent = 0 );

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
        if (features != pickedFeatures) {
            pickedFeatures = features;
            Q_EMIT pickedFeaturesChanged ();
        }
    }

    /// Returns the set of picked features
    const std::vector <App::DocumentObject *>& getPickedFeatures () const {
        return pickedFeatures;
    }
Q_SIGNALS:
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

    std::vector <App::DocumentObject *> pickedFeatures;
};

} /* PartDesignGui  */

#endif /* end of include guard: FEATUREPICKER_H_SOTFW5WL */
