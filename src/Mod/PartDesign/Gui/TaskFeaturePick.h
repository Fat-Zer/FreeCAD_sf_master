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


#ifndef PARTDESIGNGUI_FeaturePickDialog_H
#define PARTDESIGNGUI_FeaturePickDialog_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

#include "FeaturePicker.h"


namespace PartDesignGui {

class FeaturePicker;

/// A task for use in the associated dialog
class TaskFeaturePick : public Gui::TaskView::TaskBox
{
    Q_OBJECT
public:
     TaskFeaturePick(FeaturePicker *picker, bool multiSelect=false, QWidget *parent = 0);

// TODO Rewright (2015-12-09, Fat-Zer)
//     std::vector<App::DocumentObject*> buildFeatures();
//     void showExternal(bool val);
//
//     static App::DocumentObject* makeCopy(App::DocumentObject* obj, std::string sub, bool independent);
//
// protected Q_SLOTS:
//     void onUpdate(bool);
//
// private:
//     Ui_TaskFeaturePick* ui;
//     QWidget* proxy;
//     SoSwitch* featureswitch;
//     std::vector<Gui::ViewProviderOrigin*> origins;
//
//     void updateList();
};


/// simulation dialog for the TaskView
class TaskDlgFeaturePick : public Gui::TaskView::TaskDialog
{
    Q_OBJECT
public:
    TaskDlgFeaturePick (FeaturePicker *picker, bool multiSelect=false);
};

}

#endif // PARTDESIGNGUI_FeaturePickDialog_H
