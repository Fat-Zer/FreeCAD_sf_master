/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
#endif

#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>

#include <Mod/PartDesign/App/Body.h>
//#include <Mod/Sketcher/App/SketchObject.h>
//
//
//#include "ui_TaskFeaturePick.h"
//#include <Mod/PartDesign/App/ShapeBinder.h>
//#include <Mod/PartDesign/App/DatumPoint.h>
//#include <Mod/PartDesign/App/DatumLine.h>
//#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/Part/App/Part2DObject.h>

#include "FeaturePickerWidgets.h"

#include "TaskFeaturePick.h"


using namespace PartDesignGui;

/**********************************************************************
 *                          TaskFeaturePick                           *
 **********************************************************************/

TaskFeaturePick::TaskFeaturePick ( FeaturePicker *picker, QWidget *parent )
    : TaskBox ( Gui::BitmapFactory().pixmap("edit-select-box"),
                QObject::tr("Select feature"), true, parent)
{
    // We need a separate container widget to add all controls to
    AbstractFeaturePickerWidget *pickWgt;
    if ( picker->isMultiPick () ) {
        pickWgt = new FeaturePickerDoublePanelWidget (picker, this);
    } else {
        pickWgt = new FeaturePickerSinglePanelWidget (picker, this);
    }
    this->addWidget(pickWgt);
}

/*********************************************************************
 *                        TaskDlgFeaturePick                         *
 *********************************************************************/

TaskDlgFeaturePick::TaskDlgFeaturePick ( FeaturePicker * picker )
    : TaskDialog()
{
    Content.push_back ( new TaskFeaturePick ( picker) );
}

int TaskDlgFeaturePick::safeExecute ( FeaturePicker * picker ) {

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        if ( qobject_cast<PartDesignGui::TaskDlgFeaturePick *>(dlg) || dlg->canClose() ) {
            // If we have another picker dialog we may safely close it.
            // And ask the user if it is some other dialog.
            Gui::Control().closeDialog();
        } else {
            return -1;
        }
    }

    Gui::Selection().clearSelection();

    return Gui::Control().showDialog(new PartDesignGui::TaskDlgFeaturePick ( picker ), /* sync = */ true);
}

// TODO Rewright (2015-12-09, Fat-Zer)
// TaskFeaturePick::TaskFeaturePick(std::vector<App::DocumentObject*>& objects,
//                                      const std::vector<featureStatus>& status,
//                                      QWidget* parent)
//   : TaskBox(Gui::BitmapFactory().pixmap("edit-select-box"),
//             QString::fromAscii("Select feature"), true, parent), ui(new Ui_TaskFeaturePick)
// {
//
//     proxy = new QWidget(this);
//     ui->setupUi(proxy);
//
//     connect(ui->checkUsed, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//     connect(ui->checkOtherBody, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//     connect(ui->checkOtherPart, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//     connect(ui->radioIndependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//     connect(ui->radioDependent, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//     connect(ui->radioXRef, SIGNAL(toggled(bool)), this, SLOT(onUpdate(bool)));
//
//     enum { axisBit=0, planeBit = 1};
//
//     // NOTE: generally there shouldn't be more then one origin
//     std::map <App::Origin*, std::bitset<2> > originVisStatus;
//
//     auto statusIt = status.cbegin();
//     auto objIt = objects.begin();
//     assert(status.size() == objects.size());
//     for (; statusIt != status.end(); ++statusIt, ++objIt) {
//         QListWidgetItem* item = new QListWidgetItem(
//                 QString::fromAscii((*objIt)->getNameInDocument()) +
//                 QString::fromAscii(" (") + getFeatureStatusString(*statusIt) + QString::fromAscii(")") );
//         ui->listWidget->addItem(item);
//
//         //check if we need to set any origin in temporary visibility mode
//         if (*statusIt != invalidShape && (*objIt)->isDerivedFrom ( App::OriginFeature::getClassTypeId () )) {
//             App::Origin *origin = static_cast<App::OriginFeature*> (*objIt)->getOrigin ();
//             if (origin) {
//                 if ((*objIt)->isDerivedFrom ( App::Plane::getClassTypeId () )) {
//                     originVisStatus[ origin ].set (planeBit, true);
//                 } else if ( (*objIt)->isDerivedFrom ( App::Line::getClassTypeId () ) ) {
//                     originVisStatus[ origin ].set (axisBit, true);
//                 }
//             }
//         }
//     }
//
//     // Setup the origin's temporary visability
//     for ( const auto & originPair: originVisStatus ) {
//         const auto &origin = originPair.first;
//
//         Gui::ViewProviderOrigin* vpo = static_cast<Gui::ViewProviderOrigin*> (
//                 Gui::Application::Instance->getViewProvider ( origin ) );
//         if (vpo) {
//             vpo->setTemporaryVisibility( originVisStatus[origin][axisBit],
//                     originVisStatus[origin][planeBit]);
//             origins.push_back(vpo);
//         }
//     }
//
//     // TODO may be update origin API to show only some objects (2015-08-31, Fat-Zer)
//
//     groupLayout()->addWidget(proxy);
//     statuses = status;
//     updateList();
// }
//
// TaskFeaturePick::~TaskFeaturePick()
// {
//     for(Gui::ViewProviderOrigin* vpo : origins)
//         vpo->resetTemporaryVisibility();
//
// }
//
// std::vector<App::DocumentObject*> TaskFeaturePick::buildFeatures() {
//
//     int index = 0;
//     std::vector<App::DocumentObject*> result;
//     auto activeBody = PartDesignGui::getBody(false);
//     auto activePart = PartDesignGui::getPartFor(activeBody, false);
//
//     for (std::vector<featureStatus>::const_iterator st = statuses.begin(); st != statuses.end(); st++) {
//         QListWidgetItem* item = ui->listWidget->item(index);
//
//         if(item->isSelected() && !item->isHidden()) {
//
//             QString t = item->text();
//             t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
//             auto obj = App::GetApplication().getActiveDocument()->getObject(t.toAscii().data());
//
//             //build the dependend copy or reference if wanted by the user
//             if(*st == otherBody  ||
//                *st == otherPart  ||
//                *st == notInBody ) {
//
//                 if(!ui->radioXRef->isChecked()) {
//                     auto copy = makeCopy(obj, "", ui->radioIndependent->isChecked());
//
//                     if(*st == otherBody)
//                         activeBody->addFeature(copy);
//                     else if(*st == otherPart) {
//                         auto oBody = PartDesignGui::getBodyFor(obj, false);
//                         if(!oBody)
//                             activePart->addObject(copy);
//                         else
//                             activeBody->addFeature(copy);
//                     }
//                     else if(*st == notInBody) {
//                         activeBody->addFeature(copy);
//                         // doesn't supposed to get here anything but sketch but to be on the safe side better to check
//                         if (copy->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
//                             Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(copy);
//                             PartDesignGui::fixSketchSupport(sketch);
//                         }
//                     }
//                     result.push_back(copy);
//                 }
//                 else
//                     result.push_back(obj);
//             }
//             else
//                 result.push_back(obj);
//
//             break;
//         }
//
//         index++;
//     }
//
//     return result;
// }
//
// App::DocumentObject* TaskFeaturePick::makeCopy(App::DocumentObject* obj, std::string sub, bool independent) {
//     App::DocumentObject* copy = nullptr;
//     if( independent &&
//         (obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId()) ||
//         obj->isDerivedFrom(PartDesign::FeaturePrimitive::getClassTypeId()))) {
//
//         //we do know that the created instance is a document object, as obj is one. But we do not know which
//         //exact type
//         auto name =  std::string("Copy") + std::string(obj->getNameInDocument());
//         copy = App::GetApplication().getActiveDocument()->addObject(obj->getTypeId().getName(), name.c_str());
//
//         //copy over all properties
//         std::vector<App::Property*> props;
//         std::vector<App::Property*> cprops;
//         obj->getPropertyList(props);
//         copy->getPropertyList(cprops);
//
//         auto it = cprops.begin();
//         for( App::Property* prop : props ) {
//
//             //independent copys dont have links and are not attached
//             if(independent && (
//                 prop->getTypeId() == App::PropertyLink::getClassTypeId() ||
//                 prop->getTypeId() == App::PropertyLinkList::getClassTypeId() ||
//                 prop->getTypeId() == App::PropertyLinkSub::getClassTypeId() ||
//                 prop->getTypeId() == App::PropertyLinkSubList::getClassTypeId()||
//                 ( prop->getGroup() && strcmp(prop->getGroup(),"Attachment")==0) ))    {
//
//                 ++it;
//                 continue;
//             }
//
//             App::Property* cprop = *it++;
//
//             if( strcmp(prop->getName(), "Label") == 0 ) {
//                 static_cast<App::PropertyString*>(cprop)->setValue(name.c_str());
//                 continue;
//             }
//
//             cprop->Paste(*prop);
//
//             //we are a independent copy, therefore no external geometry was copied. WE therefore can delete all
//             //contraints
//             if(obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId()))
//                 static_cast<Sketcher::SketchObject*>(copy)->delConstraintsToExternal();
//         }
//     }
//     else {
//
//         std::string name;
//         if(!independent)
//             name = std::string("Reference");
//         else
//             name = std::string("Copy");
//         name += std::string(obj->getNameInDocument());
//
//         std::string entity;
//         if(!sub.empty())
//             entity = sub;
//
//         Part::PropertyPartShape* shapeProp = nullptr;
//
//         // TODO Replace it with commands (2015-09-11, Fat-Zer)
//         if(obj->isDerivedFrom(Part::Datum::getClassTypeId())) {
//             copy = App::GetApplication().getActiveDocument()->addObject(
//                     obj->getClassTypeId().getName(), name.c_str() );
//
//             //we need to reference the individual datums and make again datums. This is important as
//             //datum adjust their size dependend on the part size, hence simply copying the shape is
//             //not enough
//             long int mode = mmDeactivated;
//             Part::Datum *datumCopy = static_cast<Part::Datum*>(copy);
//
//             if(obj->getTypeId() == PartDesign::Point::getClassTypeId()) {
//                 mode = mm0Vertex;
//             }
//             else if(obj->getTypeId() == PartDesign::Line::getClassTypeId()) {
//                 mode = mm1TwoPoints;
//             }
//             else if(obj->getTypeId() == PartDesign::Plane::getClassTypeId()) {
//                 mode = mmFlatFace;
//             }
//             else
//                 return copy;
//
//             // TODO Recheck this. This looks strange in case of independent copy (2015-10-31, Fat-Zer)
//             if(!independent) {
//                 datumCopy->Support.setValue(obj, entity.c_str());
//                 datumCopy->MapMode.setValue(mode);
//             }
//             else if(!entity.empty()) {
//                 datumCopy->Shape.setValue(static_cast<Part::Datum*>(obj)->Shape.getShape().getSubShape(entity.c_str()));
//             } else {
//                 datumCopy->Shape.setValue(static_cast<Part::Datum*>(obj)->Shape.getValue());
//             }
//         }
//         else if(obj->isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
//                 obj->getTypeId() == PartDesign::ShapeBinder2D::getClassTypeId()) {
//             copy = App::GetApplication().getActiveDocument()->addObject("PartDesign::ShapeBinder2D", name.c_str());
//
//             if(!independent)
//                 static_cast<PartDesign::ShapeBinder2D*>(copy)->Support.setValue(obj, entity.c_str());
//             else
//                 shapeProp = &static_cast<PartDesign::ShapeBinder*>(copy)->Shape;
//         }
//         else if(obj->getTypeId() == PartDesign::ShapeBinder::getClassTypeId() ||
//                 obj->isDerivedFrom(Part::Feature::getClassTypeId())) {
//
//             copy = App::GetApplication().getActiveDocument()->addObject("PartDesign::ShapeBinder", name.c_str());
//
//             if(!independent)
//                 static_cast<PartDesign::ShapeBinder*>(copy)->Support.setValue(obj, entity.c_str());
//             else
//                 shapeProp = &static_cast<PartDesign::ShapeBinder*>(copy)->Shape;
//         }
//
//         if(independent && shapeProp) {
//             if(entity.empty())
//                 shapeProp->setValue(static_cast<Part::Feature*>(obj)->Shape.getValue());
//             else
//                 shapeProp->setValue(static_cast<Part::Feature*>(obj)->Shape.getShape().getSubShape(entity.c_str()));
//         }
//     }
//
//     return copy;
// }
//
// //**************************************************************************
// //**************************************************************************
// // TaskDialog
// //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// TaskDlgFeaturePick::TaskDlgFeaturePick(std::vector<App::DocumentObject*> &objects,
//                                         const std::vector<TaskFeaturePick::featureStatus> &status,
//                                         boost::function<bool (std::vector<App::DocumentObject*>)> afunc,
//                                         boost::function<void (std::vector<App::DocumentObject*>)> wfunc)
//     : TaskDialog(), accepted(false)
// {
//     pick  = new TaskFeaturePick(objects, status);
//     Content.push_back(pick);
//
//     acceptFunction = afunc;
//     workFunction = wfunc;
// }
//
// TaskDlgFeaturePick::~TaskDlgFeaturePick()
// {
//     //do the work now as before in accept() the dialog is still open, hence the work
//     //function could not open annother dialog
//     if(accepted)
//         workFunction(pick->buildFeatures());
// }

#include "moc_TaskFeaturePick.cpp"
