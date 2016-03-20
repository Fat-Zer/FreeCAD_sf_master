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

/**
 * A task for selecting a feature closely related to FeaturePicker and TaskDlgFeaturePick
 */
class TaskFeaturePick : public Gui::TaskView::TaskBox
{
    Q_OBJECT
public:
    /**
     * A constructor
     * @param picker      the feature picker associated with dialog (couldn't be nullptr)
     * @param multiSelect if true than create a task with FeaturePickerDoublePanelWidget as central widget
     *                    otherwise create a FeaturePickerSinglePanelWidget, default is false
     * @param parent      a parent widget passed (as in QWidget())
     */
    TaskFeaturePick(FeaturePicker *picker, bool multiSelect=false, QWidget *parent = 0);

    /// Return true if the task was created for multisection of features
    bool isMultiselect() const
        { return multiSelect; }

    /// Returns the FeaturePicker associated with the task
    FeaturePicker *getPicker() const
        { return picker; }

    void setErrorMessage (const QString & str);
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
//     SoSwitch* featureswitch;
//     std::vector<Gui::ViewProviderOrigin*> origins;
//
//     void updateList();
private:
    QLabel *errorLabel;
    FeaturePicker *picker;
    const bool multiSelect;
};


/**
 * A task dialog for selecting a feature to use for some operation.
 *
 * @note: This dialog should be likely run in sync mode (@see Gui::Control::showDialog) due to it doesn't
 * do anything useful due either accept() or reject() but asserting that the selection is not empty.
 */
class TaskDlgFeaturePick : public Gui::TaskView::TaskDialog
{
    Q_OBJECT
public:
    TaskDlgFeaturePick (FeaturePicker *picker, bool multiSelect=false);

    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

private:
    TaskFeaturePick *taskPick;
};

}

#endif // PARTDESIGNGUI_FeaturePickDialog_H
