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
#endif

#include "ui_TaskRevolutionParameters.h"
#include "TaskRevolutionParameters.h"
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <App/Line.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "TaskSketchBasedParameters.h"
#include "Workbench.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskRevolutionParameters */

TaskRevolutionParameters::TaskRevolutionParameters(PartDesignGui::ViewProvider* RevolutionView, QWidget *parent)
    : TaskSketchBasedParameters(RevolutionView, parent, "PartDesign_Revolution",tr("Revolution parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskRevolutionParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->revolveAngle, SIGNAL(valueChanged(double)),
            this, SLOT(onAngleChanged(double)));
    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->checkBoxMidplane, SIGNAL(toggled(bool)),
            this, SLOT(onMidplane(bool)));
    connect(ui->checkBoxReversed, SIGNAL(toggled(bool)),
            this, SLOT(onReversed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->revolveAngle->blockSignals(true);
    ui->axis->blockSignals(true);
    ui->checkBoxMidplane->blockSignals(true);
    ui->checkBoxReversed->blockSignals(true);

    //bind property mirrors
    PartDesign::SketchBased* pcFeat = static_cast<PartDesign::SketchBased*>(vp->getObject());
    if (pcFeat->isDerivedFrom(PartDesign::Revolution::getClassTypeId())) {
        PartDesign::Revolution* rev = static_cast<PartDesign::Revolution*>(vp->getObject());
        this->propAngle = &(rev->Angle);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
    } else {
        assert(pcFeat->isDerivedFrom(PartDesign::Groove::getClassTypeId()));
        PartDesign::Groove* rev = static_cast<PartDesign::Groove*>(vp->getObject());
        this->propAngle = &(rev->Angle);
        this->propMidPlane = &(rev->Midplane);
        this->propReferenceAxis = &(rev->ReferenceAxis);
        this->propReversed = &(rev->Reversed);
    }

    double l = propAngle->getValue();
    bool mirrored = propMidPlane->getValue();
    bool reversed = propReversed->getValue();

    ui->revolveAngle->setValue(l);
    blockUpdate = false;
    updateUI();

    ui->checkBoxMidplane->setChecked(mirrored);
    ui->checkBoxReversed->setChecked(reversed);

    ui->revolveAngle->blockSignals(false);
    ui->axis->blockSignals(false);
    ui->checkBoxMidplane->blockSignals(false);
    ui->checkBoxReversed->blockSignals(false);

    setFocus ();
    
    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(vp->getObject(), false);
    if(part) {        
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(true, Gui::Application::Instance->activeDocument());
            origin->setTemporaryVisibilityAxis(true);
        }            
     } 
}

void TaskRevolutionParameters::fillAxisCombo(bool forceRefill)
{
    bool oldVal_blockUpdate = blockUpdate;
    blockUpdate = true;

    if(axesInList.size() == 0)
        forceRefill = true;//not filled yet, full refill

    if (forceRefill){
        ui->axis->clear();

        for(int i = 0  ;  i < axesInList.size()  ;  i++ ){
            delete axesInList[i];
        }
        this->axesInList.clear();

        //add sketch axes
        PartDesign::SketchBased* pcFeat = static_cast<PartDesign::Revolution*>(vp->getObject());
        Part::Part2DObject* pcSketch = static_cast<Part::Part2DObject*>(pcFeat->Sketch.getValue());
        if (pcSketch){
            addAxisToCombo(pcSketch,"V_Axis",QObject::tr("Vertical sketch axis"));
            addAxisToCombo(pcSketch,"H_Axis",QObject::tr("Horizontal sketch axis"));
            for (int i=0; i < pcSketch->getAxisCount(); i++) {
                QString itemText = QObject::tr("Construction line %1").arg(i+1);
                std::stringstream sub;
                sub << "Axis" << i;
                addAxisToCombo(pcSketch,sub.str(),itemText);
            }
        }

        //add part axes
        App::DocumentObject* line = 0;
        line = getPartLines(App::Part::BaselineTypes[0]);
        if(line)
            addAxisToCombo(line,"",tr("Base X axis"));
        line = getPartLines(App::Part::BaselineTypes[1]);
        if(line)
            addAxisToCombo(line,"",tr("Base Y axis"));
        line = getPartLines(App::Part::BaselineTypes[2]);
        if(line)
            addAxisToCombo(line,"",tr("Base Z axis"));

        //add "Select reference"
        addAxisToCombo(0,std::string(),tr("Select reference..."));
    }//endif forceRefill

    //add current link, if not in list
    //first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string> &subList = propReferenceAxis->getSubValues();
    for(int i = 0  ;  i < axesInList.size()  ;  i++) {
        if(ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues())
            indexOfCurrent = i;
    }
    if ( indexOfCurrent == -1  &&  ax ){
        assert(subList.size() <= 1);
        std::string sub;
        if (subList.size()>0)
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size()-1;
    }

    //highlight current.
    if (indexOfCurrent != -1)
        ui->axis->setCurrentIndex(indexOfCurrent);

    blockUpdate = oldVal_blockUpdate;
}

void TaskRevolutionParameters::addAxisToCombo(App::DocumentObject* linkObj,
                                              std::string linkSubname,
                                              QString itemText)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.push_back(new App::PropertyLinkSub());
    App::PropertyLinkSub &lnk = *(axesInList[axesInList.size()-1]);
    lnk.setValue(linkObj,std::vector<std::string>(1,linkSubname));
}

void TaskRevolutionParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    fillAxisCombo();

    blockUpdate = false;
}

void TaskRevolutionParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        exitSelectionMode();
        std::vector<std::string> axis;
        App::DocumentObject* selObj;
        getReferencedSelection(vp->getObject(), msg, selObj, axis);
        propReferenceAxis->setValue(selObj, axis);

        recomputeFeature();
        updateUI();
    }
}


void TaskRevolutionParameters::onAngleChanged(double len)
{
    propAngle->setValue(len);
    exitSelectionMode();
    recomputeFeature();
}

void TaskRevolutionParameters::onAxisChanged(int num)
{
    if (blockUpdate)
        return;
    PartDesign::SketchBased* pcRevolution = static_cast<PartDesign::SketchBased*>(vp->getObject());

    if(axesInList.size() == 0)
        return;

    App::DocumentObject *oldRefAxis = propReferenceAxis->getValue();
    std::vector<std::string> oldSubRefAxis = propReferenceAxis->getSubValues();

    App::PropertyLinkSub &lnk = *(axesInList[num]);
    if(lnk.getValue() == 0){
        // enter reference selection mode
        TaskSketchBasedParameters::onSelectReference(true, true, false, true);
    } else {
        if (! pcRevolution->getDocument()->isIn(lnk.getValue())){
            Base::Console().Error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);
        exitSelectionMode();
    }

    App::DocumentObject *newRefAxis = propReferenceAxis->getValue();
    const std::vector<std::string> &newSubRefAxis = propReferenceAxis->getSubValues();
    if (oldRefAxis != newRefAxis ||
        oldSubRefAxis.size() != newSubRefAxis.size() ||
        oldSubRefAxis[0] != newSubRefAxis[0]) {
        bool reversed = propReversed->getValue();
        if(pcRevolution->isDerivedFrom(PartDesign::Revolution::getClassTypeId()))
            reversed = static_cast<PartDesign::Revolution*>(pcRevolution)->suggestReversed();
        if(pcRevolution->isDerivedFrom(PartDesign::Groove::getClassTypeId()))
            reversed = static_cast<PartDesign::Groove*>(pcRevolution)->suggestReversed();

        if (reversed != propReversed->getValue()) {
            propReversed->setValue(reversed);
            ui->checkBoxReversed->blockSignals(true);
            ui->checkBoxReversed->setChecked(reversed);
            ui->checkBoxReversed->blockSignals(false);
        }
    }

    recomputeFeature();
}

void TaskRevolutionParameters::onMidplane(bool on)
{
    propMidPlane->setValue(on);
    recomputeFeature();
}

void TaskRevolutionParameters::onReversed(bool on)
{
    propReversed->setValue(on);
    recomputeFeature();
}

double TaskRevolutionParameters::getAngle(void) const
{
    return ui->revolveAngle->value().getValue();
}

void TaskRevolutionParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.size() == 0)
        throw Base::Exception("Not initialized!");

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub &lnk = *(axesInList[num]);
    if(lnk.getValue() == 0){
        throw Base::Exception("Still in reference selection mode; reference wasn't selected yet");
    } else {
        PartDesign::SketchBased* pcRevolution = static_cast<PartDesign::SketchBased*>(vp->getObject());
        if (! pcRevolution->getDocument()->isIn(lnk.getValue())){
            throw Base::Exception("Object was deleted");
            return;
        }
        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

bool   TaskRevolutionParameters::getMidplane(void) const
{
    return ui->checkBoxMidplane->isChecked();
}

bool   TaskRevolutionParameters::getReversed(void) const
{
    return ui->checkBoxReversed->isChecked();
}

TaskRevolutionParameters::~TaskRevolutionParameters()
{
    //hide the parts coordinate system axis for selection
    App::Part* part = getPartFor(vp->getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
    }
    
    delete ui;

    for(int i = 0  ;  i < axesInList.size()  ;  i++ ){
        delete axesInList[i];
    }
}

void TaskRevolutionParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgRevolutionParameters::TaskDlgRevolutionParameters(PartDesignGui::ViewProvider *RevolutionView)
    : TaskDlgSketchBasedParameters(RevolutionView)
{
    assert(RevolutionView);
    parameter  = new TaskRevolutionParameters(RevolutionView);

    Content.push_back(parameter);
}

TaskDlgRevolutionParameters::~TaskDlgRevolutionParameters()
{

}

//==== calls from the TaskView ===============================================================

bool TaskDlgRevolutionParameters::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    //Gui::Command::openCommand("Revolution changed");
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Angle = %f",name.c_str(),parameter->getAngle());
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    parameter->getReferenceAxis(obj, sub);
    std::string axis = getPythonStr(obj, sub);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ReferenceAxis = %s",name.c_str(),axis.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(),parameter->getMidplane()?1:0);
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(),parameter->getReversed()?1:0);

    return TaskDlgSketchBasedParameters::accept();
}


#include "moc_TaskRevolutionParameters.cpp"
