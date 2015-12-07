/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include "FeatureSketchBased.h"

#include "FeatureLoft.h"


using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::FeatureAddSub)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections,(0),"Loft",App::Prop_None,"List of sections");
    Sections.setSize(0);
    ADD_PROPERTY_TYPE(Ruled,(false),"Loft",App::Prop_None,"Create ruled surface");
    ADD_PROPERTY_TYPE(Closed,(false),"Loft",App::Prop_None,"Close Last to First Profile");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Ruled.isTouched())
        return 1;
    if (Closed.isTouched())
        return 1;

    return FeatureAddSub::mustExecute();
}

inline TopoDS_Shape Loft::makeFace(const std::vector<TopoDS_Wire>& w) {
    return SketchBased::makeFace ( w );
}

App::DocumentObjectExecReturn *Loft::execute(void)
{

    auto multisections = Sections.getValues();

    if (multisections.size() < 2 ) {
        return new App::DocumentObjectExecReturn("Loft: At least two section are needed");
    }

    try {
        //setup the location
        this->positionByBase();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        std::vector < std::vector<TopoDS_Wire> > wiresections;

        // Process the first section separately to calculate the number of wires eac section should has
        std::vector<TopoDS_Wire> front_wiresection;

        front_wiresection = SketchBased::getSketchWires (
                SketchBased::getVerifiedSketch ( multisections[0] ) );

        for(TopoDS_Wire& wire : front_wiresection)
            wiresections.push_back ( std::vector<TopoDS_Wire> (1, wire) );

        // Process other wiresections
        for(auto objIt = std::next ( multisections.cbegin () ); objIt != multisections.cend (); ++objIt ) {
            std::vector<TopoDS_Wire> wires = SketchBased::getSketchWires (
                SketchBased::getVerifiedSketch ( *objIt ) );
            if ( wires.size ()!=wiresections.size() ) {
                return new App::DocumentObjectExecReturn(
                        "Loft: Sections need to have the same amount of inner wires as the base section" );
            }

            for (size_t i=0; i<wires.size (); i++) {
                wiresections[i].push_back(TopoDS::Wire ( wires[i] ) );
            }
        }

        //build all shells
        std::vector<TopoDS_Shape> shells;
        for(std::vector<TopoDS_Wire>& wires : wiresections) {

            BRepOffsetAPI_ThruSections mkTS(false, Ruled.getValue(), Precision::Confusion());

            for(TopoDS_Wire& wire : wires)   {
                 wire.Move(invObjLoc);
                 mkTS.AddWire(wire);
            }

            mkTS.Build();
            if (!mkTS.IsDone())
                return new App::DocumentObjectExecReturn("Loft could not be build");

            //build the shell use simulate to get the top and bottom wires in an easy way
            shells.push_back(mkTS.Shape());
        }

        //build the top and bottom face, sew the shell and build the final solid
        TopoDS_Shape front = makeFace(front_wiresection);
        front.Move(invObjLoc);

        std::vector<TopoDS_Wire> backwires;
        for(std::vector<TopoDS_Wire>& wires : wiresections)
            backwires.push_back(wires.back());

        TopoDS_Shape back = makeFace(backwires);

        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        sewer.Add(front);
        sewer.Add(back);
        for(TopoDS_Shape& s : shells)
            sewer.Add(s);

        sewer.Perform();

        //build the solid
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        if(!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn("Loft: Result is not a solid");

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if ( SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        AddSubShape.setValue(result);

        return FeatureAddSub::execute();
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("Loft: A fatal error occurred when making the loft");
    }
}


PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft() {
    addSubType = Subtractive;
}
