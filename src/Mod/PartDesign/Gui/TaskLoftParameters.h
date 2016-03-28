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


#ifndef GUI_TASKVIEW_TaskLoftParameters_H
#define GUI_TASKVIEW_TaskLoftParameters_H

#include <Gui/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderLoft.h"

class Ui_TaskLoftParameters;
class QListWidget;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskLoftParameters : public TaskFeatureParameters
{
    Q_OBJECT

public:
    TaskLoftParameters(ViewProviderLoft *LoftView,bool newObj=false,QWidget *parent = 0);
    ~TaskLoftParameters();

    /// apply changes made in the parameters input to the model via commands
    virtual void apply();

    virtual bool canUpdate () const;

    bool getRuled () const;
    bool getClosed () const;
    const std::vector <App::DocumentObject *> & getSections () const ;

private Q_SLOTS:
    void onClosed(bool);
    void onRuled(bool);
    void onPickedFeaturesChanged();

private:
    QWidget* proxy;
    Ui_TaskLoftParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgLoftParameters : public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj=false);
    ~TaskDlgLoftParameters();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
