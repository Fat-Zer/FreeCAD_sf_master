/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
# include <QMessageBox>
#endif

#include "ui_TaskBooleanParameters.h"
#include "TaskBooleanParameters.h"
#include "Workbench.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/Gui/ReferenceSelection.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskBooleanParameters */

TaskBooleanParameters::TaskBooleanParameters(ViewProviderBoolean *BooleanView,QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("PartDesign_Boolean"),tr("Boolean parameters"),true, parent),BooleanView(BooleanView)
{
    selectionMode = none;

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskBooleanParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonBodyAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonBodyAdd(bool)));
    connect(ui->buttonBodyRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonBodyRemove(bool)));
    connect(ui->comboType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTypeChanged(int)));

    this->groupLayout()->addWidget(proxy);

    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    std::vector<App::DocumentObject*> bodies = pcBoolean->Bodies.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++)
    {
        ui->listWidgetBodies->insertItem(0, QString::fromAscii((*b)->getNameInDocument()));
    }
    // Create context menu
    QAction* action = new QAction(tr("Remove"), this);
    ui->listWidgetBodies->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(onBodyDeleted()));
    ui->listWidgetBodies->setContextMenuPolicy(Qt::ActionsContextMenu);

    int index = pcBoolean->Type.getValue();
    ui->comboType->setCurrentIndex(index);
}

void TaskBooleanParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (strcmp(msg.pDocName, BooleanView->getObject()->getDocument()->getName()) != 0)
            return;

        PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
        std::string body(msg.pObjectName);
        if (body.empty())
            return;
        App::DocumentObject* pcBody = pcBoolean->getDocument()->getObject(body.c_str());
        if (pcBody == NULL)
            return;
        if (!pcBody->getTypeId().isDerivedFrom(PartDesign::Body::getClassTypeId())) {
            pcBody = PartDesign::Body::findBodyOf(pcBody);
            if (pcBody == NULL)
                return;
        }

        std::vector<App::DocumentObject*> bodies = pcBoolean->Bodies.getValues();

        if (selectionMode == bodyAdd) {
            if (std::find(bodies.begin(), bodies.end(), pcBody) == bodies.end()) {
                bodies.push_back(pcBody);
                pcBoolean->Bodies.setValues(bodies);
                ui->listWidgetBodies->insertItem(ui->listWidgetBodies->count(),
                                                 QString::fromStdString(pcBody->getNameInDocument()));

                pcBoolean->getDocument()->recomputeFeature(pcBoolean);
                ui->buttonBodyAdd->setChecked(false);
                exitSelectionMode();

                // Hide the bodies if there are more than two
                if (bodies.size() == 2) {
                    // Hide both bodies
                    Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                                Gui::Application::Instance->getViewProvider(bodies.front()));
                    if (vp != NULL)
                        vp->hide();
                    vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                                                    Gui::Application::Instance->getViewProvider(bodies.back()));
                    if (vp != NULL)
                        vp->hide();
                    BooleanView->show();
                } else {
                    // Hide newly added body
                    Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                                Gui::Application::Instance->getViewProvider(bodies.back()));
                    if (vp != NULL)
                        vp->hide();
                }
            }
        } else if (selectionMode == bodyRemove) {
            std::vector<App::DocumentObject*>::iterator b = std::find(bodies.begin(), bodies.end(), pcBody);
            if (b != bodies.end()) {
                bodies.erase(b);
                pcBoolean->Bodies.setValues(bodies);
                QList<QListWidgetItem*> items = ui->listWidgetBodies->findItems(QString::fromStdString(body), Qt::MatchExactly);
                if (!items.empty()) {
                    for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
                        QListWidgetItem* it = ui->listWidgetBodies->takeItem(ui->listWidgetBodies->row(*i));
                        delete it;
                    }
                }
                pcBoolean->getDocument()->recomputeFeature(pcBoolean);
                ui->buttonBodyRemove->setChecked(false);
                exitSelectionMode();

                // Make bodies visible again
                Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                            Gui::Application::Instance->getViewProvider(*b));
                if (vp != NULL)
                    vp->show();
                if (bodies.size() == 1) {
                    Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                                Gui::Application::Instance->getViewProvider(bodies.front()));
                    if (vp != NULL)
                        vp->show();
                    BooleanView->hide();
                }
            }
        }
    }
}

void TaskBooleanParameters::onButtonBodyAdd(bool checked)
{
    if (checked) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc != NULL)
            doc->setHide(BooleanView->getObject()->getNameInDocument());
        selectionMode = bodyAdd;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
}

void TaskBooleanParameters::onButtonBodyRemove(bool checked)
{
    if (checked) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc != NULL)
            doc->setHide(BooleanView->getObject()->getNameInDocument());
        selectionMode = bodyRemove;
        Gui::Selection().clearSelection();
    } else {
        exitSelectionMode();
    }
}

void TaskBooleanParameters::onTypeChanged(int index)
{
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());

    switch (index) {
        case 0: pcBoolean->Type.setValue("Fuse"); break;
        case 1: pcBoolean->Type.setValue("Cut"); break;
        case 2: pcBoolean->Type.setValue("Common"); break;
        case 3: pcBoolean->Type.setValue("Section"); break;
        default: pcBoolean->Type.setValue("Fuse");
    }

    pcBoolean->getDocument()->recomputeFeature(pcBoolean);
}

const std::vector<std::string> TaskBooleanParameters::getBodies(void) const
{
    std::vector<std::string> result;
    for (int i = 0; i < ui->listWidgetBodies->count(); i++)
        result.push_back(ui->listWidgetBodies->item(i)->text().toStdString());
    return result;
}

int TaskBooleanParameters::getType(void) const
{
    return ui->comboType->currentIndex();
}

void TaskBooleanParameters::onBodyDeleted(void)
{
    PartDesign::Boolean* pcBoolean = static_cast<PartDesign::Boolean*>(BooleanView->getObject());
    std::vector<App::DocumentObject*> bodies = pcBoolean->Bodies.getValues();
    int index = ui->listWidgetBodies->currentRow();
    if (index > bodies.size())
        return;
    App::DocumentObject* body = bodies[index];
    bodies.erase(bodies.begin() + ui->listWidgetBodies->currentRow());
    pcBoolean->Bodies.setValues(bodies);
    ui->listWidgetBodies->model()->removeRow(ui->listWidgetBodies->currentRow());
    pcBoolean->getDocument()->recomputeFeature(pcBoolean);

    // Make bodies visible again
    Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                Gui::Application::Instance->getViewProvider(body));
    if (vp != NULL)
        vp->show();
    if (bodies.size() == 1) {
        Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                    Gui::Application::Instance->getViewProvider(bodies.front()));
        if (vp != NULL)
            vp->show();
        BooleanView->hide();
    }
}

TaskBooleanParameters::~TaskBooleanParameters()
{
    delete ui;
}

void TaskBooleanParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->comboType->blockSignals(true);
        int index = ui->comboType->currentIndex();
        ui->retranslateUi(proxy);
        ui->comboType->setCurrentIndex(index);
    }
}

void TaskBooleanParameters::exitSelectionMode()
{
    selectionMode = none;
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc != NULL)
        doc->setShow(BooleanView->getObject()->getNameInDocument());
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgBooleanParameters::TaskDlgBooleanParameters(ViewProviderBoolean *BooleanView)
    : TaskDialog(),BooleanView(BooleanView)
{
    assert(BooleanView);
    parameter  = new TaskBooleanParameters(BooleanView);

    Content.push_back(parameter);
}

TaskDlgBooleanParameters::~TaskDlgBooleanParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgBooleanParameters::open()
{

}

void TaskDlgBooleanParameters::clicked(int)
{

}

bool TaskDlgBooleanParameters::accept()
{
    std::string name = BooleanView->getObject()->getNameInDocument();
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc != NULL)
        doc->setShow(name.c_str());

    try {
        std::vector<std::string> bodies = parameter->getBodies();
        std::stringstream str;
        str << "App.ActiveDocument." << name.c_str() << ".Bodies = [";
        for (std::vector<std::string>::const_iterator it = bodies.begin(); it != bodies.end(); ++it)
            str << "App.ActiveDocument." << *it << ",";
        str << "]";
        Gui::Command::doCommand(Gui::Command::Doc,str.str().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Boolean: Accept: Input error"), QString::fromAscii(e.what()));
        return false;
    }

    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getType());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::commitCommand();

    return true;
}

bool TaskDlgBooleanParameters::reject()
{
    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    return true;
}



#include "moc_TaskBooleanParameters.cpp"
