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
# include <QMessageBox>
#endif

#include "ViewProviderPipe.h"
//#include "TaskPipeParameters.h"
#include "Workbench.h"
#include "TaskPipeParameters.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPipe,PartDesignGui::ViewProvider)

ViewProviderPipe::ViewProviderPipe()
{
    sPixmap = "PartDesign_Additive_Pipe.svg"; 
}

ViewProviderPipe::~ViewProviderPipe()
{
}

std::vector<App::DocumentObject*> ViewProviderPipe::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    App::DocumentObject* sketch = static_cast<PartDesign::Pipe*>(getObject())->Sketch.getValue();
    if (sketch != NULL)
        temp.push_back(sketch);

    return temp;
}

void ViewProviderPipe::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{/*
    QAction* act;
    act = menu->addAction(QObject::tr("Edit pad"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);*/
}

bool ViewProviderPipe::doubleClicked(void)
{
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    return true;
}

bool ViewProviderPipe::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default || ModNum == 1 ) {
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgPipeParameters *padDlg = qobject_cast<TaskDlgPipeParameters *>(dlg);
        if (padDlg && padDlg->getPipeView() != this)
            padDlg = 0; // another pad left open its task panel
        if (dlg && !padDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (padDlg)
            Gui::Control().showDialog(padDlg);
        else
            Gui::Control().showDialog(new TaskDlgPipeParameters(this,ModNum == 1));

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

bool ViewProviderPipe::onDelete(const std::vector<std::string> &s)
{/*
    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = 0;
    if (pcPipe->Sketch.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcPipe->Sketch.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);*/
}

