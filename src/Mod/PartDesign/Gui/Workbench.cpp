/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <qobject.h>
# include <boost/bind.hpp>
# include <Precision.hxx>
#endif

#include "Workbench.h"
#include <App/Plane.h>
#include <App/Placement.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Control.h>

#include <Mod/Sketcher/Gui/Workbench.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace PartDesignGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Part Design");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Face tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Sketch tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Create Geometry");
#endif

namespace PartDesignGui {
//===========================================================================
// Helper for Body
//===========================================================================

PartDesign::Body *getBody(void)
{
    if(!PartDesignGui::ActivePartObject){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Body"),
            QObject::tr("In order to use PartDesign you need an active Body object in the document. "
                        "Please make one active or create one. If you have a legacy document "
                        "with PartDesign objects without Body, use the transfer function in "
                        "PartDesign to put them into a Body."
                        ));
    }
    return PartDesignGui::ActivePartObject;

}

}

/// @namespace PartDesignGui @class Workbench
TYPESYSTEM_SOURCE(PartDesignGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void switchToDocument(const App::Document* doc)
{
    if (doc == NULL) return;

    PartDesign::Body* activeBody = NULL;
    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    // Is there a body feature in this document?
    if (bodies.empty()) {
        // Get the objects now, before adding the Body and the base planes
        std::vector<App::DocumentObject*> features = doc->getObjects();

        // This adds both the base planes and the body
        // Note: In the following code we rely on the first body always having the name "Body"!
        Gui::Command::runCommand(Gui::Command::Doc, "FreeCADGui.runCommand('PartDesign_Body')");

        // Assign all document features to the new body
        std::string modelString = "";

        for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
            if ((*f)->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ||
                (*f)->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
                modelString += std::string(modelString.empty() ? "" : ",") + "App.ActiveDocument." + (*f)->getNameInDocument();
            }
        }

        if (!modelString.empty()) {
            modelString = std::string("[") + modelString + "]";
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().Body.Model = %s", modelString.c_str());
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().Body.Tip = App.activeDocument().%s", features.back()->getNameInDocument());
        }

        // Initialize the BaseFeature property of all PartDesign solid features
        App::DocumentObject* baseFeature = NULL;
        for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
            if (PartDesign::Body::isSolidFeature(*f)) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.BaseFeature = %s",
                                        (*f)->getNameInDocument(),
                                        baseFeature == NULL ?
                                            "None" :
                                            (std::string("App.activeDocument().") + baseFeature->getNameInDocument()).c_str());

                baseFeature = *f;
            }
        }

        // Re-route all sketches without support to the base planes
        for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
            if ((*f)->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
                Sketcher::SketchObject* sketch = static_cast<Sketcher::SketchObject*>(*f);
                App::DocumentObject* support = sketch->Support.getValue();
                if (support != NULL)
                    continue; // Sketch is on a face of a solid
                Base::Placement plm = sketch->Placement.getValue();
                Base::Vector3d pnt = plm.getPosition();
                // Currently we only handle positions that correspond to the base planes
                if (pnt.Length() > Precision::Confusion()) {
                    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Sketch plane cannot be migrated"),
                        QObject::tr("Please edit ") + QString::fromAscii(sketch->getNameInDocument()) +
                        QObject::tr("and redefine it to use a Base or Datum plane as the sketch plane."));
                    break; // avoid repeating this message for every sketch
                }
                Base::Rotation rot = plm.getRotation();
                Base::Vector3d SketchVector(0,0,1);
                rot.multVec(SketchVector, SketchVector);
                std::string  side = (SketchVector.x + SketchVector.y + SketchVector.z) < 0.0 ? "back" : "front";
                if (side == "back") SketchVector *= -1.0;

                if (SketchVector == Base::Vector3d(0,0,1)) {
                    Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,['%s'])",
                                            sketch->getNameInDocument(), BaseplaneNames[0], side.c_str());
                } else if (SketchVector == Base::Vector3d(0,1,0)) {
                    Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,['%s'])",
                                            sketch->getNameInDocument(), BaseplaneNames[1], side.c_str());
                } else if (SketchVector == Base::Vector3d(1,0,0)) {
                    Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.Support = (App.activeDocument().%s,['%s'])",
                                            sketch->getNameInDocument(), BaseplaneNames[2], side.c_str());
                } else {
                    QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Sketch plane cannot be migrated"),
                        QObject::tr("Please edit") + QString::fromAscii(sketch->getNameInDocument()) +
                        QObject::tr("and redefine it to use a Base or Datum plane as the sketch plane."));
                    break; // avoid repeating this message for every sketch
                }
            }
        }

        activeBody = static_cast<PartDesign::Body*>(doc->getObject("Body"));
    } else {
        // Find active body
        for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++) {
            PartDesign::Body* body = static_cast<PartDesign::Body*>(*b);
            if (body->IsActive.getValue()) {
                activeBody = body;
                break;
            }
        }

        // Do the base planes exist in this document?
        bool found = false;
        std::vector<App::DocumentObject*> planes = doc->getObjectsOfType(App::Plane::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator p = planes.begin(); p != planes.end(); p++) {
            for (unsigned i = 0; i < 3; i++) {
                if (strcmp(PartDesignGui::BaseplaneNames[i], (*p)->getNameInDocument()) == 0) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        if (!found) {
            // Add the planes ...
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", PartDesignGui::BaseplaneNames[0]);
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XY-Plane").toStdString().c_str());
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", PartDesignGui::BaseplaneNames[1]);
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),90))");
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XZ-Plane").toStdString().c_str());
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", PartDesignGui::BaseplaneNames[2]);
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("YZ-Plane").toStdString().c_str());
            // ... and put them in the 'Origin' group
            Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::DocumentObjectGroup','%s')", QObject::tr("Origin").toStdString().c_str());
            for (unsigned i = 0; i < 3; i++)
                Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().Origin.addObject(App.activeDocument().getObject('%s'))", PartDesignGui::BaseplaneNames[i]);
            // TODO: Fold the group (is that possible through the Python interface?)
        }
    }

    // If there is only one body, make it active
    if ((activeBody == NULL) && (bodies.size() == 1))
        activeBody = static_cast<PartDesign::Body*>(bodies.front());

    if (activeBody != NULL) {
        Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");
        Gui::Command::doCommand(Gui::Command::Gui,"PartDesignGui.setActivePart(App.activeDocument().%s)", activeBody->getNameInDocument());
    } else {
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Could not create body"),
            QObject::tr("No body was found in this document, and none could be created. Please report this bug."
                        "We recommend you do not use this document with the PartDesign workbench until the bug has been fixed."
                        ));
    }
}

void Workbench::slotActiveDocument(const Gui::Document& Doc)
{
    switchToDocument(Doc.getDocument());
}

void Workbench::slotNewDocument(const App::Document& Doc)
{
    switchToDocument(&Doc);
}

void Workbench::slotFinishRestoreDocument(const App::Document& Doc)
{    
    switchToDocument(&Doc);
}

void Workbench::slotDeleteDocument(const App::Document&)
{
    ActivePartObject = 0;
    ActiveGuiDoc = 0;
    ActiveAppDoc = 0;
    ActiveVp = 0;
}

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(PartDesign::Feature::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Datum::getClassTypeId()) +
            Gui::Selection().countObjectsOfType(Part::Part2DObject::getClassTypeId()) > 0 )
            *item << "PartDesign_MoveTip";
    }
}

void Workbench::activated()
{
    Gui::Workbench::activated();


    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Vertex[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Vertex COUNT 1..",
        Vertex,
        "Vertex tools",
        "Part_Box"
    ));
 
    const char* Edge[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Edge COUNT 1..",
        Edge,
        "Edge tools",
        "Part_Box"
    ));

    const char* Face[] = {
        "PartDesign_NewSketch",        
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 1",
        Face,
        "Face tools",
        "Part_Box"
    ));

    const char* Body[] = {
        "PartDesign_NewSketch",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1",
        Body,
        "Start Body",
        "Part_Box"
    ));

    const char* Plane1[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT App::Plane COUNT 1",
        Plane1,
        "Start Part",
        "Part_Box"
    ));
    const char* Plane2[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Plane COUNT 1",
        Plane2,
        "Start Part",
        "Part_Box"
    ));

    const char* Line[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Line COUNT 1",
        Line,
        "Start Part",
        "Part_Box"
    ));

    const char* Point[] = {
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Point COUNT 1",
        Point,
        "Start Part",
        "Part_Box"
    ));

    const char* NoSel[] = {
        "PartDesign_Body",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommandsEmptySelection(
        NoSel,
        "Start Part",
        "Part_Box"
    ));

    const char* Faces[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 2..",
        Faces,
        "Face tools",
        "Part_Box"
    ));

    const char* Sketch[] = {
        "PartDesign_NewSketch",
        "PartDesign_Pad",
        "PartDesign_Pocket",
        "PartDesign_Revolution",
        "PartDesign_Groove",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Sketcher::SketchObject COUNT 1",
        Sketch,
        "Sketch tools",
        "Part_Box"
    ));

    const char* Transformed[] = {
        "PartDesign_Mirrored",
        "PartDesign_LinearPattern",
        "PartDesign_PolarPattern",
//        "PartDesign_Scaled",
        "PartDesign_MultiTransform",
        0};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::SketchBased",
        Transformed,
        "Transformation tools",
        "PartDesign_MultiTransform"
    ));

    // make the previously used active Body active again
    PartDesignGui::ActivePartObject = NULL;
    switchToDocument(App::GetApplication().getActiveDocument());

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();

    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.connect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.connect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.connect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.connect(boost::bind(&Workbench::slotDeleteDocument, this, _1));
}

void Workbench::deactivated()
{
    // Let us be notified when a document is activated, so that we can update the ActivePartObject
    Gui::Application::Instance->signalActiveDocument.disconnect(boost::bind(&Workbench::slotActiveDocument, this, _1));
    App::GetApplication().signalNewDocument.disconnect(boost::bind(&Workbench::slotNewDocument, this, _1));
    App::GetApplication().signalFinishRestoreDocument.disconnect(boost::bind(&Workbench::slotFinishRestoreDocument, this, _1));
    App::GetApplication().signalDeleteDocument.disconnect(boost::bind(&Workbench::slotDeleteDocument, this, _1));

    removeTaskWatcher();
    // reset the active Body
    Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");
    Gui::Command::doCommand(Gui::Command::Doc,"PartDesignGui.setActivePart(None)");

    Gui::Workbench::deactivated();

}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* geom = new Gui::MenuItem();
    geom->setCommand("Sketcher geometries");
    SketcherGui::addSketcherWorkbenchGeometries( *geom );

    Gui::MenuItem* cons = new Gui::MenuItem();
    cons->setCommand("Sketcher constraints");
    SketcherGui::addSketcherWorkbenchConstraints( *cons );
    
    Gui::MenuItem* consaccel = new Gui::MenuItem();
    consaccel->setCommand("Sketcher tools");
    SketcherGui::addSketcherWorkbenchTools(*consaccel);

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("&Part Design");
    SketcherGui::addSketcherWorkbenchSketchActions( *part );
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_LeaveSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_ReorientSketch"
          << geom
          << cons
          << "Separator"
          << "PartDesign_Plane"
          << "PartDesign_Line"
          << "PartDesign_Point"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform";
    // For 0.13 a couple of python packages like numpy, matplotlib and others
    // are not deployed with the installer on Windows. Thus, the WizardShaft is
    // not deployed either hence the check for the existence of the command.
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_InvoluteGear")) {
        *part << "PartDesign_InvoluteGear";
    }
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_WizardShaft")) {
        *part << "Separator" << "PartDesign_WizardShaft";
    }

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design");
//    SketcherGui::addSketcherWorkbenchSketchActions( *part );
    *part << "PartDesign_Body"
          << "PartDesign_NewSketch"
          << "Sketcher_ViewSketch"
          << "Sketcher_MapSketch"
          << "Sketcher_LeaveSketch"
          << "Separator"
          << "PartDesign_Plane"
          << "PartDesign_Line"
          << "PartDesign_Point"
          << "Separator"
          << "PartDesign_Pad"
          << "PartDesign_Pocket"
          << "PartDesign_Revolution"
          << "PartDesign_Groove"
          << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
//          << "PartDesign_Scaled"
          << "PartDesign_MultiTransform";

    Gui::ToolBarItem* geom = new Gui::ToolBarItem(root);
    geom->setCommand("Sketcher geometries");
    SketcherGui::addSketcherWorkbenchGeometries( *geom );

    Gui::ToolBarItem* cons = new Gui::ToolBarItem(root);
    cons->setCommand("Sketcher constraints");
    SketcherGui::addSketcherWorkbenchConstraints( *cons );

    Gui::ToolBarItem* consaccel = new Gui::ToolBarItem(root);
    consaccel->setCommand("Sketcher tools");
    SketcherGui::addSketcherWorkbenchTools( *consaccel );
    
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

