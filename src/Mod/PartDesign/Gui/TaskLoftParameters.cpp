/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *   Copyright (C) 2016 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <Base/Console.h>

#include <Gui/Command.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/PartDesign/App/Body.h>

#include "ReferenceSelection.h"

#include "ui_TaskLoftParameters.h"

#include "TaskLoftParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView,bool newObj, QWidget *parent)
    : TaskFeatureParameters(LoftView, parent, "PartDesign_Additive_Loft",tr("Loft parameters"))
{
    PartDesign::Loft * loft = Base::freecad_dynamic_cast<PartDesign::Loft>(vp->getObject());
    assert (loft);

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLoftParameters();
    ui->setupUi(proxy);

    this->addWidget(proxy);

    // Fill the picker with all sketches in current doc and some other
    FeaturePicker *picker = new FeaturePicker (this);
    for ( auto sketchObj: loft->getDocument ()->getObjectsOfType ( Part::Part2DObject::getClassTypeId() ) ) {
        picker->addSketch ( sketchObj );
    }
    for ( auto sketchObj: loft->Sections.getValues () ) {
        FeaturePicker::StatusSet status;
        status.set (FeaturePicker::validFeature);
        status.set (FeaturePicker::userSelected);
        status |= FeaturePicker::bodyRelationStatus (sketchObj);
        picker->setFeatureStatus ( sketchObj, status );
    }

    picker->setPickedFeatures ( loft->Sections.getValues () );

    ui->pickerWidget->setPicker (picker);

    ui->checkBoxRuled->setChecked  (loft->Ruled.getValue () );
    ui->checkBoxClosed->setChecked (loft->Closed.getValue () );

    QMetaObject::connectSlotsByName(this);

    connect(ui->checkBoxRuled, SIGNAL(toggled(bool)), this, SLOT(onRuled(bool)));
    connect(ui->checkBoxClosed, SIGNAL(toggled(bool)), this, SLOT(onClosed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)), this, SLOT(onUpdateView(bool)));
    connect(picker, SIGNAL(pickedFeaturesChanged()), this, SLOT(onPickedFeaturesChanged()) );
}

TaskLoftParameters::~TaskLoftParameters()
{
    delete ui;
}

void TaskLoftParameters::onClosed(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Closed.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onRuled(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Ruled.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onPickedFeaturesChanged () {
    const auto & sections = getSections ();

    if (sections.size()>=2) {
        static_cast<PartDesign::Loft*>(vp->getObject())->Sections.setValues (sections);
    }

    recomputeFeature();
}

bool TaskLoftParameters::canUpdate () const {
    return ui->pickerWidget->getPicker ()->getPickedFeatures ().size () >= 2
        && TaskFeatureParameters::canUpdate ();
}

bool TaskLoftParameters::getClosed () const {
    return ui->checkBoxClosed->isChecked ();
}

bool TaskLoftParameters::getRuled () const {
    return ui->checkBoxRuled->isChecked ();
}

const std::vector <App::DocumentObject *> & TaskLoftParameters::getSections () const {
    return  ui->pickerWidget->getPicker ()->getPickedFeatures ();
}

void TaskLoftParameters::apply () {
    std::string name = vp->getObject()->getNameInDocument();
    const char * cname = name.c_str();

    const auto & sections = getSections ();

    if (sections.size()<2) {
        throw Base::Exception ( tr("To few sections to construct a loft.").toUtf8 () );
    }

    std::string sectionsStr = buildLinkListPythonStr ( sections );

    // TODO copy features if needed (2016-03-28, Fat-Zer)
    Gui::Command::doCommand ( Gui::Command::Doc, "App.ActiveDocument.%s.Ruled = %i",
            cname, getRuled() ? 1: 0);
    Gui::Command::doCommand ( Gui::Command::Doc, "App.ActiveDocument.%s.Closed = %i",
            cname, getClosed() ? 1: 0);
    Gui::Command::doCommand ( Gui::Command::Doc, "App.ActiveDocument.%s.Sections = %s",
            cname, sectionsStr.c_str () );
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj)
   : TaskDlgFeatureParameters(LoftView)
{
    assert(LoftView);
    Content.push_back(new TaskLoftParameters(LoftView,newObj));
}

TaskDlgLoftParameters::~TaskDlgLoftParameters()
{ }

// TODO add external link handling (2016-03-20, Fat-Zer)

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
