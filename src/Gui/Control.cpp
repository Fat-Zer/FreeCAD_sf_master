/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
# include <QAction>
# include <QApplication>
# include <QDebug>
# include <QDockWidget>
# include <QPointer>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Control.h"
#include "TaskView/TaskView.h"

#include <Gui/MainWindow.h>
#include <Gui/CombiView.h>
#include <Gui/DockWindowManager.h>


using namespace Gui;
using namespace std;

/* TRANSLATOR Gui::ControlSingleton */

ControlSingleton* ControlSingleton::_pcSingleton = 0;
static QPointer<Gui::TaskView::TaskView> _taskPanel = 0;

ControlSingleton::ControlSingleton()
  : syncDialogLoop(0)
{

}

ControlSingleton::~ControlSingleton()
{
    if (syncDialogLoop) {
        delete syncDialogLoop;
    }
}

Gui::TaskView::TaskView* ControlSingleton::taskPanel() const
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView)
        return pcCombiView->getTaskPanel();
    // not all workbenches have the combo view enabled
    else
        return _taskPanel; //< either the non combo view task panel or no task panel available at all
}

void ControlSingleton::showTaskView()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcCombiView)
        pcCombiView->showTaskView();
    else if (_taskPanel)
        _taskPanel->raise();
}

void ControlSingleton::showModelView()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    if (pcCombiView)
        pcCombiView->showTreeView();
    else if (_taskPanel)
        _taskPanel->raise();
}

int ControlSingleton::showDialog(Gui::TaskView::TaskDialog *dlg, bool sync)
{
    int rv = 0;
    auto activeDlg = activeDialog ();

    // only one dialog at a time, print a warning instead of raising an assert
    if (activeDlg && activeDlg == dlg) {
        if (dlg) {
            qWarning() << "ControlSingleton::showDialog: Can't show "
                       << dlg->metaObject()->className()
                       << " since there is already an active task dialog";
        }
        else {
            qWarning() << "ControlSingleton::showDialog: Task dialog is null";
        }
        return -1;
    }
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView) {
        pcCombiView->showDialog(dlg);
        // make sure that the combo view is shown
        QDockWidget* dw = qobject_cast<QDockWidget*>(pcCombiView->parentWidget());
        if (dw) {
            dw->setVisible(true);
            dw->toggleViewAction()->setVisible(true);
            dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        }

        if (activeDlg == dlg)
            return -1; // dialog is already defined
        activeDlg = dlg;
        connect ( pcCombiView->getTaskPanel (), SIGNAL(dialogFinished(int)), this, SLOT(closedDialog()) );
    }
    // not all workbenches have the combo view enabled
    else if (!_taskPanel) {
        // TODO Check if this legacy code works (2016-03-04, Fat-Zer)
        QDockWidget* dw = new QDockWidget();
        dw->setWindowTitle(tr("Task panel"));
        dw->setFeatures(QDockWidget::DockWidgetMovable);
        _taskPanel = new Gui::TaskView::TaskView(dw);
        dw->setWidget(_taskPanel);
        _taskPanel->showDialog(dlg);
        getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, dw);
        connect(dlg, SIGNAL(destroyed()), dw, SLOT(deleteLater()));

        // if we have the normal tree view available then just tabify with it
        QWidget* treeView = Gui::DockWindowManager::instance()->getDockWindow("Tree view");
        QDockWidget* par = treeView ? qobject_cast<QDockWidget*>(treeView->parent()) : 0;
        if (par && par->isVisible()) {
            getMainWindow()->tabifyDockWidget(par, dw);
            qApp->processEvents(); // make sure that the task panel is tabified now
            dw->show();
            dw->raise();
        }
    }

    // if sync is specified run the event loop and wait for dialog being finished
    if (sync) {
        QScopedPointer <QEventLoop> localEventLoop (new QEventLoop());
        syncDialogLoop = localEventLoop.data ();

        // We cannot rely on event loop return code due to QEventDialog::exit () may be called 
        // from QCoreApplication::exit (); so introduce yet another variable
        syncDialogRC = -1;
        connect(taskPanel (), SIGNAL(dialogFinished(int)), this, SLOT (finishSyncDialog(int)) );
        syncDialogLoop->exec ();
        rv = syncDialogRC;
        disconnect(taskPanel (), SIGNAL(dialogFinished(int)), this, SLOT (finishSyncDialog(int)) );
    }

    return rv;
}

QTabWidget* ControlSingleton::tabPanel() const
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    if (pcCombiView)
        return pcCombiView->getTabPanel();
    return 0;
}

Gui::TaskView::TaskDialog* ControlSingleton::activeDialog() const
{
    Gui::TaskView::TaskView* taskPnl = taskPanel();
    return taskPnl ? taskPnl->getActiveDialog() : nullptr;
}

void ControlSingleton::accept()
{
    Gui::TaskView::TaskView* taskPnl = taskPanel();
    if (taskPnl) {
        taskPnl->accept();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::reject()
{
    Gui::TaskView::TaskView* taskPnl = taskPanel();
    if (taskPnl) {
        taskPnl->reject();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::closeDialog()
{
    Gui::TaskView::TaskView* taskPnl = taskPanel();
    if (taskPnl) {
        taskPnl->closeDialog();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents |
                            QEventLoop::ExcludeSocketNotifiers);
    }
}

void ControlSingleton::finishSyncDialog(int status)
{
    if (syncDialogLoop) {
        syncDialogRC = status;
        syncDialogLoop->exit ();
    } else {
        Base::Console().Error ( "No syncronous dialog is running\n" );
    }
}

void ControlSingleton::closedDialog()
{
    Gui::DockWnd::CombiView* pcCombiView = qobject_cast<Gui::DockWnd::CombiView*>
        (Gui::DockWindowManager::instance()->getDockWindow("Combo View"));
    // should return the pointer to combo view
    assert(pcCombiView);
    pcCombiView->closedDialog();
    // make sure that the combo view is shown
    QDockWidget* dw = qobject_cast<QDockWidget*>(pcCombiView->parentWidget());
    if (dw)
        dw->setFeatures(QDockWidget::AllDockWidgetFeatures);
}

bool ControlSingleton::isAllowedAlterDocument(void) const
{
    TaskView::TaskDialog* activeDlg = activeDialog ();
    if (activeDlg)
        return activeDlg->isAllowedAlterDocument();
    else
        return true;
}


bool ControlSingleton::isAllowedAlterView(void) const
{
    TaskView::TaskDialog* activeDlg = activeDialog ();
    if (activeDlg)
        return activeDlg->isAllowedAlterView();
    else
        return true;
}

bool ControlSingleton::isAllowedAlterSelection(void) const
{
    TaskView::TaskDialog* activeDlg = activeDialog ();
    if (activeDlg)
        return activeDlg->isAllowedAlterSelection();
    else
        return true;
}

// -------------------------------------------

ControlSingleton& ControlSingleton::instance(void)
{
    if (_pcSingleton == NULL)
        _pcSingleton = new ControlSingleton;
    return *_pcSingleton;
}

void ControlSingleton::destruct (void)
{
    if (_pcSingleton != NULL)
        delete _pcSingleton;
    _pcSingleton = 0;
}


// -------------------------------------------


#include "moc_Control.cpp"

