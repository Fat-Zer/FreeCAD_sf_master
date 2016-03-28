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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopExp_Explorer.hxx>
#endif

#include <App/Part.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "Utils.h"
#include "FeaturePicker.h"

using namespace PartDesignGui;

/*********************************************************************
 *                           FeaturePicker                           *
 *********************************************************************/


FeaturePicker::FeaturePicker ( QObject * parent )
    : QObject (parent)
{ }

void FeaturePicker::setFeatureStatus( App::DocumentObject *obj, StatusSet status ) {
    assert (obj);

    auto objIt = statusMap.lower_bound(obj);

    if (objIt != statusMap.end() && objIt->first == obj) {
        objIt->second = status;
    } else {
        statusMap.insert ( objIt, std::make_pair (obj, status) );
    }

    Q_EMIT featureStatusSet (obj, status);
}

FeaturePicker::StatusSet FeaturePicker::sketchStatus ( App::DocumentObject *obj ) {
    StatusSet rv;

    rv.set (validFeature); // count the feature valid unless otherwise prooved

    // Set statuses against the active body
    rv |= bodyRelationStatus (obj);

    // Check if the sketch is used
    auto sketchInList = obj->getInList();
    auto partDesignLink = std::find_if ( sketchInList.begin (), sketchInList.end (),
            [] (App::DocumentObject *obj) -> bool {
                return obj->isDerivedFrom (PartDesign::Feature::getClassTypeId () );
            } );

    if ( partDesignLink != sketchInList.end() ) {
        rv.set (isUsed);
    }

    // Check whether the sketch shape is valid
    Part::Part2DObject* sketch = Base::freecad_dynamic_cast< Part::Part2DObject >(obj);
    assert (sketch);
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    if (shape.IsNull()) {
        rv.set (invalidShape);
        rv.reset (validFeature);
    }

    // count free wires
    int ctWires=0;
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        ctWires++;
    }

    if (ctWires == 0) {
        rv.set (noWire);
        rv.reset (validFeature);
    }

    return rv;
}

FeaturePicker::StatusSet FeaturePicker::bodyRelationStatus ( App::DocumentObject *obj ) {
    return bodyRelationStatus (obj, PartDesignGui::getBody(false) );
}

FeaturePicker::StatusSet FeaturePicker::bodyRelationStatus (
        App::DocumentObject *obj, PartDesign::Body *body )
{
    StatusSet rv;

    App::Part* activePart =
        body ? PartDesignGui::getPartFor(body, false) : PartDesignGui::getActivePart ();

    PartDesign::Body *objBody = PartDesign::Body::findBodyOf (obj);
    App::Part *objPart = App::Part::getPartOfObject ( objBody ? objBody : obj );

    // Check if the sketch belongs to active part
    if (objPart != activePart) {
        rv.set (otherPart);
        rv.set (isExternal);
    } else if (objBody != body) {
    // Check if the sketch belongs to active body;
    // note that the active part check gets advantage over the body check
        rv.set (otherBody);
        rv.set (isExternal);
    }

    // Check if the sketch is after tip
    if (objBody && objBody->isAfterInsertPoint ( obj )) {
        rv.set (afterTip);
    }

    return rv;
}

QString FeaturePicker::getFeatureStatusString ( FeaturePicker::FeatureStatus status ) {
    switch (status) {
        case validFeature: return tr("Valid");
        case isUsed:       return tr("The feature already used by other");
        case isExternal:   return tr("The feature is external against current context");
        case otherBody:    return tr("The feature belongs to another body");
        case otherPart:    return tr("The feature belongs to another part");
        case afterTip:     return tr("The feature is located after the tip feature");
        case userSelected: return tr("The feature was preselected by user");
        case basePlane:    return tr("Base plane");
        case invalidShape: return tr("Invalid shape");
        case noWire:       return tr("The sketch has no wire");
        case FEATURE_STATUS_MAX: ;//< suppress warning
    }

    assert (!"Bad status passed to getFeatureStatusString()");
}

FeaturePicker::StatusSet FeaturePicker::getStatus ( App::DocumentObject * obj ) const {
    FeaturePicker::StatusSet rv;

    auto rvIt = statusMap.find (obj);
    if (rvIt != statusMap.end () ) {
        return rvIt->second;
    } else {
        return 0;
    }
}

FeaturePicker::StatusSet FeaturePicker::getFeatureStatusMask ( ) const {
    StatusSet rv;

    for (auto featStat : statusMap ) {
        rv |= featStat.second;
    }

    return rv;
}


inline std::set < App::DocumentObject * > FeaturePicker::getFeaturesFilteredByStatus (
        std::function<bool(StatusSet)> filter) const
{
    std::set < App::DocumentObject * > rv;

    for (auto featStat : statusMap ) {
        if ( filter(featStat.second) ) {
            rv.insert (featStat.first);
        }
    }

    return rv;

}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithStatus (
        FeaturePicker::FeatureStatus stat) const
{
    return getFeaturesFilteredByStatus ([stat] (StatusSet s) -> bool { return s[stat];} );
}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithStatus (
        FeaturePicker::StatusSet status) const
{
    return getFeaturesFilteredByStatus ([status] (StatusSet s) -> bool { return (s | ~status).all (); } );
}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithExactStatus (
        FeaturePicker::StatusSet status) const
{
    return getFeaturesFilteredByStatus ([status] (StatusSet s) -> bool { return (s ^ status).none (); } );
}

#include "moc_FeaturePicker.cpp"
