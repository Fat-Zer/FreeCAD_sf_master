/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <memory>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
#endif


#include "FeatureAddSub.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::Feature)

FeatureAddSub::FeatureAddSub()
  :  addSubType(Additive)
{
    ADD_PROPERTY(AddSubShape,(TopoDS_Shape()));
}

App::DocumentObjectExecReturn *FeatureAddSub::execute () {

    TopoDS_Shape addSub = AddSubShape.getValue ();

    if ( addSub.IsNull () ) {
        return new App::DocumentObjectExecReturn("AddSubFeature: AddSub is not a solid");
    }

    TopoDS_Shape base;

    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    if ( base.IsNull () ) {
        // No base feature so set the resulting shape equal to the base one for addictive
        // or returm an error for sibstractive
        switch (getAddSubType()) {
            case FeatureAddSub::Additive    : Shape.setValue (addSub); return App::DocumentObject::StdReturn;
            case FeatureAddSub::Subtractive : return new App::DocumentObjectExecReturn (
                                                      "Nothing to cut substractive feature from" );
            default: assert ( ! "Bad feature type passed to addSubFeature" ); break;
        }
    } else {
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        base.Move ( invObjLoc );
    }

    // if the Base property has a valid shape, fuse/cut the addSub into it
    std::unique_ptr<BRepAlgoAPI_BooleanOperation> addSubOp;

    switch (getAddSubType()) {
        case FeatureAddSub::Additive    : addSubOp.reset ( new BRepAlgoAPI_Fuse (base, addSub) ); break;
        case FeatureAddSub::Subtractive : addSubOp.reset ( new BRepAlgoAPI_Cut  (base, addSub) ); break;
        default: assert ( ! "Bad feature type passed to addSubFeature" ); break;
    }

    if (!addSubOp->IsDone()) {
        return new App::DocumentObjectExecReturn("FeatureAddSub: Adding the feature failed");
    }
    // we have to get the solids (fuse sometimes creates compounds)
    TopoDS_Shape boolOp = getSolid(addSubOp->Shape());

    // lets check if the result is a solid
    if (boolOp.IsNull()) {
        return new App::DocumentObjectExecReturn("FeatureAddSub: Resulting shape is null");
    }

    boolOp = refineShapeIfActive(boolOp);

    Shape.setValue(boolOp);

    return App::DocumentObject::StdReturn;
}
