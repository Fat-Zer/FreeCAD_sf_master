/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <QMessageBox>
# include <QTimer>
#endif

#include <Base/Console.h>
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
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderOrigin.h>

#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"
#include "Utils.h"

#include "ui_TaskLinearPatternParameters.h"
#include "TaskLinearPatternParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLinearPatternParameters */

TaskLinearPatternParameters::TaskLinearPatternParameters(ViewProviderTransformed *TransformedView,QWidget *parent)
        : TaskTransformedParameters(TransformedView, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLinearPatternParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->buttonOK->hide();
    ui->checkBoxUpdateView->setEnabled(true);

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

TaskLinearPatternParameters::TaskLinearPatternParameters(TaskMultiTransformParameters *parentTask, QLayout *layout)
        : TaskTransformedParameters(parentTask)
{
    proxy = new QWidget(parentTask);
    ui = new Ui_TaskLinearPatternParameters();
    ui->setupUi(proxy);
    connect(ui->buttonOK, SIGNAL(pressed()),
            parentTask, SLOT(onSubTaskButtonOK()));
    QMetaObject::connectSlotsByName(this);

    layout->addWidget(proxy);

    ui->buttonOK->setEnabled(true);
    ui->buttonAddFeature->hide();
    ui->buttonRemoveFeature->hide();
    ui->listWidgetFeatures->hide();
    ui->checkBoxUpdateView->hide();

    selectionMode = none;

    blockUpdate = false; // Hack, sometimes it is NOT false although set to false in Transformed::Transformed()!!
    setupUI();
}

void TaskLinearPatternParameters::setupUI()
{
    connect(ui->buttonAddFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonAddFeature(bool)));
    connect(ui->buttonRemoveFeature, SIGNAL(toggled(bool)), this, SLOT(onButtonRemoveFeature(bool)));
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetFeatures->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onFeatureDeleted()));
    ui->listWidgetFeatures->setContextMenuPolicy(Qt::ActionsContextMenu);

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());

    connect(updateViewTimer, SIGNAL(timeout()),
            this, SLOT(onUpdateViewTimer()));
    connect(ui->comboDirection, SIGNAL(activated(int)),
            this, SLOT(onDirectionChanged(int)));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));
    connect(ui->spinLength, SIGNAL(valueChanged(double)),
            this, SLOT(onLength(double)));
    connect(ui->spinOccurrences, SIGNAL(valueChanged(int)),
            this, SLOT(onOccurrences(int)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    // Get the feature data
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    std::vector<App::DocumentObject*> originals = pcLinearPattern->Originals.getValues();

    // Fill data into dialog elements
    for (std::vector<App::DocumentObject*>::const_iterator i = originals.begin(); i != originals.end(); i++)
    {
        if ((*i) != NULL)
            ui->listWidgetFeatures->addItem(QString::fromAscii((*i)->getNameInDocument()));
    }
    // ---------------------

    ui->comboDirection->setEnabled(true);
    ui->checkReverse->setEnabled(true);
    ui->spinLength->blockSignals(true);
    ui->spinLength->setEnabled(true);
    ui->spinLength->setUnit(Base::Unit::Length);
    ui->spinLength->blockSignals(false);
    ui->spinOccurrences->setEnabled(true);
    
    dirLinks.setCombo(*(ui->comboDirection));
    App::DocumentObject* sketch = getSketchObject();
    if (!sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        sketch = 0;
    this->fillAxisCombo(dirLinks,static_cast<Part::Part2DObject*>(sketch));

    updateUI();

    //show the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
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

void TaskLinearPatternParameters::updateUI()
{
    if (blockUpdate)
        return;
    blockUpdate = true;

    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());

    bool reverse = pcLinearPattern->Reversed.getValue();
    double length = pcLinearPattern->Length.getValue();
    unsigned occurrences = pcLinearPattern->Occurrences.getValue();

    if (dirLinks.setCurrentLink(pcLinearPattern->Direction) == -1){
        //failed to set current, because the link isnt in the list yet
        dirLinks.addLink(pcLinearPattern->Direction, getRefStr(pcLinearPattern->Direction.getValue(),pcLinearPattern->Direction.getSubValues()));
        dirLinks.setCurrentLink(pcLinearPattern->Direction);
    }

    // Note: These three lines would trigger onLength(), on Occurrences() and another updateUI() if we
    // didn't check for blockUpdate
    ui->checkReverse->setChecked(reverse);
    ui->spinLength->setValue(length);
    ui->spinOccurrences->setValue(occurrences);

    blockUpdate = false;
}

void TaskLinearPatternParameters::onUpdateViewTimer()
{
    recomputeFeature();
}

void TaskLinearPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskLinearPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (originalSelected(msg)) {
            if (selectionMode == addFeature)
                ui->listWidgetFeatures->addItem(QString::fromAscii(msg.pObjectName));
            else
                removeItemFromListWidget(ui->listWidgetFeatures, msg.pObjectName);

            exitSelectionMode();
        } else if (selectionMode == reference) {
            // Note: ReferenceSelection has already checked the selection for validity
            exitSelectionMode();
            std::vector<std::string> directions;
            App::DocumentObject* selObj;
            PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
            getReferencedSelection(pcLinearPattern, msg, selObj, directions);
            pcLinearPattern->Direction.setValue(selObj, directions);

            recomputeFeature();
            updateUI();
        } else if( strstr(msg.pObjectName, App::Part::BaselineTypes[0]) == nullptr || 
                   strstr(msg.pObjectName, App::Part::BaselineTypes[1]) == nullptr ||
                   strstr(msg.pObjectName, App::Part::BaselineTypes[2]) == nullptr) {
           
            std::vector<std::string> directions;
            App::DocumentObject* selObj;
            PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
            getReferencedSelection(pcLinearPattern, msg, selObj, directions);
            pcLinearPattern->Direction.setValue(selObj, directions);

            recomputeFeature();
            updateUI();
        }
    }
}

void TaskLinearPatternParameters::clearButtons()
{
    ui->buttonAddFeature->setChecked(false);
    ui->buttonRemoveFeature->setChecked(false);
}

void TaskLinearPatternParameters::onCheckReverse(const bool on) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Reversed.setValue(on);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onLength(const double l) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Length.setValue(l);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onOccurrences(const int n) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    pcLinearPattern->Occurrences.setValue(n);

    exitSelectionMode();
    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onDirectionChanged(int num) {
    if (blockUpdate)
        return;
    PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
    try{
        if(dirLinks.getCurrentLink().getValue() == 0){
            // enter reference selection mode
            hideObject();
            showBase();
            selectionMode = reference;
            Gui::Selection().clearSelection();
            addReferenceSelectionGate(true, true);
        } else {
            exitSelectionMode();
            pcLinearPattern->Direction.Paste(dirLinks.getCurrentLink());
        }
    } catch (Base::Exception &e) {
        QMessageBox::warning(0,tr("Error"),QString::fromAscii(e.what()));
    }

    kickUpdateViewTimer();
}

void TaskLinearPatternParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    if (on) {
        // Do the same like in TaskDlgLinearPatternParameters::accept() but without doCommand
        PartDesign::LinearPattern* pcLinearPattern = static_cast<PartDesign::LinearPattern*>(getObject());
        std::vector<std::string> directions;
        App::DocumentObject* obj;

        getDirection(obj, directions);
        pcLinearPattern->Direction.setValue(obj,directions);
        pcLinearPattern->Reversed.setValue(getReverse());
        pcLinearPattern->Length.setValue(getLength());
        pcLinearPattern->Occurrences.setValue(getOccurrences());

        recomputeFeature();
    }
}

void TaskLinearPatternParameters::onFeatureDeleted(void)
{
    PartDesign::Transformed* pcTransformed = getObject();
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();
    originals.erase(originals.begin() + ui->listWidgetFeatures->currentRow());
    pcTransformed->Originals.setValues(originals);
    ui->listWidgetFeatures->model()->removeRow(ui->listWidgetFeatures->currentRow());
    recomputeFeature();
}

void TaskLinearPatternParameters::getDirection(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    const App::PropertyLinkSub &lnk = dirLinks.getCurrentLink();
    obj = lnk.getValue();
    sub = lnk.getSubValues();
}

const bool TaskLinearPatternParameters::getReverse(void) const
{
    return ui->checkReverse->isChecked();
}

const double TaskLinearPatternParameters::getLength(void) const
{
    return ui->spinLength->value().getValue();
}

const unsigned TaskLinearPatternParameters::getOccurrences(void) const
{
    return ui->spinOccurrences->value();
}


TaskLinearPatternParameters::~TaskLinearPatternParameters()
{
    //hide the parts coordinate system axis for selection
    App::Part* part = getPartFor(getObject(), false);
    if(part) {
        auto app_origin = part->getObjectsOfType(App::Origin::getClassTypeId());
        if(!app_origin.empty()) {
            ViewProviderOrigin* origin;
            origin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->activeDocument()->getViewProvider(app_origin[0]));
            origin->setTemporaryVisibilityMode(false);
        }            
    }
    
    delete ui;
    if (proxy)
        delete proxy;
}

void TaskLinearPatternParameters::changeEvent(QEvent *e)
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

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(ViewProviderLinearPattern *LinearPatternView)
    : TaskDlgTransformedParameters(LinearPatternView)
{
    parameter = new TaskLinearPatternParameters(LinearPatternView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgLinearPatternParameters::accept()
{
    std::string name = vp->getObject()->getNameInDocument();

    TaskLinearPatternParameters* linearpatternParameter = static_cast<TaskLinearPatternParameters*>(parameter);
    std::vector<std::string> directions;
    App::DocumentObject* obj;
    linearpatternParameter->getDirection(obj, directions);
    std::string direction = buildLinkSingleSubPythonStr(obj, directions);

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), direction.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %u",name.c_str(),linearpatternParameter->getReverse());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),linearpatternParameter->getLength());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Occurrences = %u",name.c_str(),linearpatternParameter->getOccurrences());

    return TaskDlgTransformedParameters::accept();
}

#include "moc_TaskLinearPatternParameters.cpp"
