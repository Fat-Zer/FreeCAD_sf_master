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
# include <boost/range/adaptor/map.hpp>
# include <TopExp_Explorer.hxx>
# include <QTreeWidget>
# include <QTreeWidgetItem>
#endif

#include <Base/Console.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
//#include <Gui/MainWindow.h>
//#include <Gui/Document.h>
//#include <Gui/ViewProviderOrigin.h>
//#include <App/Document.h>
//#include <App/Origin.h>
//#include <App/OriginFeature.h>
//#include <Base/Tools.h>
//#include <Base/Reader.h>
//
#include <Mod/PartDesign/App/Body.h>
//#include <Mod/Sketcher/App/SketchObject.h>
//
#include "Utils.h"
//
//#include "ui_TaskFeaturePick.h"
//#include <Mod/PartDesign/App/ShapeBinder.h>
//#include <Mod/PartDesign/App/DatumPoint.h>
//#include <Mod/PartDesign/App/DatumLine.h>
//#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/Part/App/Part2DObject.h>

#include "TaskFeaturePick.h"

using namespace PartDesignGui;

/*********************************************************************
 *                           FeaturePicker                           *
 *********************************************************************/


FeaturePicker::FeaturePicker ( bool s_multiPick, QObject * parent )
    : QObject (parent), multiPick (s_multiPick)
{
    visability [ validFeature ]    = true;
    // Due to external features are additionally constrained enable enable them by default
    visability [ isExternal ]      = true;
    visability [ basePlane    ]    = true;
//    visability [ userSelected ]    = true;
}

void FeaturePicker::setFeatureStatus( App::DocumentObject *obj, StatusSet status ) {

    assert (obj);

    // Remove old values
    statusMap.insert ( std::make_pair(obj, status) );

    Q_EMIT featureStatusSet (obj, status);
}

FeaturePicker::StatusSet FeaturePicker::sketchStatus ( App::DocumentObject *obj ) {
    StatusSet rv;

    rv.set (validFeature); // count the feature valid unless otherwise prooved

    // Set statuses against the active body
    rv |= bodyRelationStatus (obj);

    // Check if the sketch is used
    auto sketchInList = obj->getInList();
    auto partDesignLink = std::find_if ( sketchInList.begin (), sketchInList.end (),
            [] (App::DocumentObject *obj) -> bool {
                return obj->isDerivedFrom (PartDesign::Feature::getClassTypeId () );
            } );

    if ( partDesignLink != sketchInList.end() ) {
        rv.set (isUsed);
    }

    // Check whether the sketch shape is valid
    Part::Part2DObject* sketch = Base::freecad_dynamic_cast< Part::Part2DObject >(obj);
    assert (sketch);
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    if (shape.IsNull()) {
        rv.set (invalidShape);
        rv.reset (validFeature);
    }

    // count free wires
    int ctWires=0;
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        ctWires++;
    }

    if (ctWires == 0) {
        rv.set (noWire);
        rv.reset (validFeature);
    }

    return rv;
}

FeaturePicker::StatusSet FeaturePicker::bodyRelationStatus ( App::DocumentObject *obj) {
    return bodyRelationStatus (obj, PartDesignGui::getBody(false) );
}

FeaturePicker::StatusSet FeaturePicker::bodyRelationStatus ( App::DocumentObject *obj, PartDesign::Body *body ) {
    StatusSet rv;

    App::Part* activePart =
        body ? PartDesignGui::getPartFor(body, false) : PartDesignGui::getActivePart ();

    PartDesign::Body *objBody = PartDesign::Body::findBodyOf (obj);
    App::Part *objPart = App::Part::getPartOfObject ( objBody ? objBody : obj );

    // Check if the sketch belongs to active part
    if (objPart != activePart) {
        rv.set (otherPart);
        rv.set (isExternal);
    } else if (objBody != body) {
    // Check if the sketch belongs to active body;
    // note that the active part check gets advantage over the body check
        rv.set (otherBody);
        rv.set (isExternal);
    }

    // Check if the sketch is after tip
    if (body && objBody->isAfterInsertPoint ( obj )) {
        rv.set (afterTip);
    }

    return rv;
}

QString FeaturePicker::getFeatureStatusString ( FeaturePicker::FeatureStatus status )
{
    switch (status) {
        case validFeature: return tr("Valid");
        case isUsed:       return tr("The feature already used by other");
        case isExternal:   return tr("The feature is external against current context");
        case otherBody:    return tr("The feature belongs to another body");
        case otherPart:    return tr("The feature belongs to another part");
        case afterTip:     return tr("The feature is located after the tip feature");
        case basePlane:    return tr("Base plane");
        case invalidShape: return tr("Invalid shape");
        case noWire:       return tr("The sketch has no wire");
        case FEATURE_STATUS_MAX: ;//< suppress warning
    }

    assert (!"Bad status passed to getFeatureStatusString()");
}

FeaturePicker::StatusSet FeaturePicker::getStatus ( App::DocumentObject * obj ) const {
    FeaturePicker::StatusSet rv;

    auto rvIt = statusMap.find (obj);
    if (rvIt != statusMap.end () ) {
        return rvIt->second;
    } else {
        return 0;
    }
}

FeaturePicker::StatusSet FeaturePicker::getFeatureStatusMask ( ) const {
    StatusSet rv;

    for (auto featStat : statusMap ) {
        rv |= featStat.second;
    }

    return rv;
}


inline std::set < App::DocumentObject * > FeaturePicker::getFeaturesFilteredByStatus (
        std::function<bool(StatusSet)> filter) const
{
    std::set < App::DocumentObject * > rv;


    for (auto featStat : statusMap ) {
        if ( filter(featStat.second) ) {
            rv.insert (featStat.first);
        }
    }

    return rv;

}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithStatus (
        FeaturePicker::FeatureStatus stat) const
{
    return getFeaturesFilteredByStatus ([stat] (StatusSet s) -> bool { return s[stat];} );
}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithStatus (
        FeaturePicker::StatusSet status) const
{
    return getFeaturesFilteredByStatus ([status] (StatusSet s) -> bool { return (s | ~status).all (); } );
}

std::set < App::DocumentObject * > FeaturePicker::getFeaturesWithExactStatus (
        FeaturePicker::StatusSet status) const
{
    return getFeaturesFilteredByStatus ([status] (StatusSet s) -> bool { return (s ^ status).none (); } );
}

void FeaturePicker::setVisability (FeaturePicker::FeatureStatus status, bool state) {
    if (visability [status] != state ) {
        visability.set ( status, state );
        Q_EMIT visabilityChanged (status, state);
    }
}

/// Retruns true if given feature should be visible
bool FeaturePicker::isVisible (App::DocumentObject *feat) const {
    return isVisible ( getStatus (feat) );
}

/**********************************************************************
*                   FeaturePickSingleSelectWidget                    *
**********************************************************************/

FeaturePickSingleSelectWidget::FeaturePickSingleSelectWidget (
        FeaturePicker *picker_s, QWidget *parent )
: QTreeWidget (parent), picker (picker_s)
{
    assert (picker);

    setHeaderHidden ( true );
    sortByColumn ( 0 );
    setSortingEnabled ( 0 );
    setSelectionBehavior ( QAbstractItemView::SelectRows );
    setSelectionMode ( QAbstractItemView::SingleSelection );

    // TODO May be make it possible to select multiple objects through this widget (?) (2016-03-01, Fat-Zer)
    if (picker->isMultiPick ()) {
        Base::Console().Error ( "FeaturePickSingleSelectWidget shouldn't be used for multiple selection\n" );
    }

    for ( auto featStat : picker->getFeaturesStatusMap () ) {
        createTreeWidgetItem (featStat.first, featStat.second);
    }

    connect ( this, SIGNAL ( itemSelectionChanged () ), this, SLOT (onWidgetSelectionChanged () ) );
    connect ( picker, SIGNAL ( pickedFeaturesChanged () ), this, SLOT (onPickedFeaturesChanged () ) );
    connect ( picker, SIGNAL ( visabilityChanged ( PartDesignGui::FeaturePicker::FeatureStatus, bool ) ),
              this, SLOT (onVisabilityChanged () ) );
    connect ( picker, SIGNAL ( featureStatusSet ( App::DocumentObject *,
                                                  PartDesignGui::FeaturePicker::StatusSet ) ),
              this, SLOT ( onFeatureStatusSet ( App::DocumentObject *,
                                                PartDesignGui::FeaturePicker::StatusSet ) ) );
}


void  FeaturePickSingleSelectWidget::onWidgetSelectionChanged () {
    auto sel = selectedItems();
    assert (sel.size () <= 1);
    if (sel.size() != 0) {
        auto treeWgtIt = treeItems.right.find (sel[0]);
        assert ( treeWgtIt != treeItems.right.end() );
        picker->setPickedFeature (treeWgtIt->second);
    } else {
        picker->resetPickedFeature ();
    }

    // TODO Select the feature in the 3DView (2016-02-28, Fat-Zer)
}

void FeaturePickSingleSelectWidget::onPickedFeaturesChanged () {
    // select the picked widget if it differs from currently selected
    auto sel = selectedItems ();
    assert (sel.size () <= 1);
    const auto & picked = picker->getPickedFeatures ();

    if (picked.size() == 0) {
        if (sel.size()!= 0) {
           clearSelection ();
        }
    } else if ( sel.size () !=0 ) {
        auto itemPickedIt = treeItems.left.find (picked.front());
        if ( itemPickedIt != treeItems.left.end() ) {
            auto pickedTreeWgt = itemPickedIt->second;
            if (pickedTreeWgt != sel.front() ) {
                setCurrentItem (pickedTreeWgt);
            }
        } else {
# ifdef FC_DEBUG
            Base::Console().Error(
                    "FeaturePickControlWidget::onPickedFeaturesChanged(): "
                    "bad no tree widget for given feature: %s'\n",
                    picked.front() ? picked.front()->Label.getValue () : "bad address");
# endif
        }
    }
}

void FeaturePickSingleSelectWidget::onFeatureStatusSet (
        App::DocumentObject *feat, PartDesignGui::FeaturePicker::StatusSet status)
{
    auto featIt = treeItems.left.find (feat);
    if (featIt == treeItems.left.end() ) {
        createTreeWidgetItem (feat);
    } else {
        featIt->second->setHidden ( !picker->isVisible (status) );
    }
}

QTreeWidgetItem *FeaturePickSingleSelectWidget::createTreeWidgetItem (
        App::DocumentObject * feat,
        const FeaturePicker::StatusSet & status )
{
    Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider (feat);
    assert (vp);

    QTreeWidgetItem *item =
        new QTreeWidgetItem ( QStringList ( QString::fromUtf8 ( feat->Label.getValue () ) ) );
    assert (item);
    item->setIcon ( 0, vp->getIcon () );

    // construct the tooltip
    QString toolTip;

    for (unsigned i=FeaturePicker::validFeature+1; i<status.size (); ++i) { //< Note skipping the validFeature
        if (status[i]) {
            toolTip.append ( FeaturePicker::getFeatureStatusString ( (FeaturePicker::FeatureStatus) i) ).
                append ( QLatin1String ("\n") );
        }
    }
    if (!toolTip.isEmpty () ) {
        item->setToolTip ( 0, toolTip );
    }

    treeItems.insert ( TreeWidgetItemMap::value_type ( feat, item ) );
    addTopLevelItem (item);


    // Hide the item if it shouldn't be visible
    item->setHidden (!picker->isVisible (status));

    return item;
}

void FeaturePickSingleSelectWidget::onVisabilityChanged () {
    for (auto featTreeItem : treeItems.left ) {
        featTreeItem.second->setHidden ( !picker->isVisible (featTreeItem.first) );
    }
}


/**********************************************************************
*                     FeaturePickTwoPanelWidget                      *
**********************************************************************/

FeaturePickTwoPanelWidget::FeaturePickTwoPanelWidget (
        FeaturePicker *picker_s, QWidget *parent )
: Gui::ActionSelector (parent), picker (picker_s)
{
    assert (picker);

    if (!picker->isMultiPick ()) {
        Base::Console().Error ( "FeaturePickTwoPlaneWidget couldn't be used for single selection\n" );
    }

    for ( auto featStat : picker->getFeaturesStatusMap () ) {
        createTreeWidgetItem (featStat.first, featStat.second);
    }

    // Move picked items to selected widget
    for ( auto feat : picker->getPickedFeatures () ) {
        auto featIt = treeItems.left.find (feat);
        assert (featIt != treeItems.left.end ());
        selectItem ( featIt->second );
        featIt->second->setHidden (false);
    }

    QAbstractItemModel *selectedModel = selectedTreeWidget ()->model ();

    connect ( selectedModel, SIGNAL ( rowsInserted (const QModelIndex &, int, int) ),
              this, SLOT (onWidgetSelectionChanged () ) );
    connect ( selectedModel, SIGNAL ( rowsRemoved (const QModelIndex &, int, int) ),
              this, SLOT (onWidgetSelectionChanged () ) );
    connect ( selectedModel, SIGNAL ( rowsMoved ( const QModelIndex &, int, int,
                                                  const QModelIndex &, int, int) ),
              this, SLOT (onWidgetSelectionChanged () ) );

    connect ( picker, SIGNAL ( pickedFeaturesChanged () ), this, SLOT (onPickedFeaturesChanged () ) );
    connect ( picker, SIGNAL ( visabilityChanged ( PartDesignGui::FeaturePicker::FeatureStatus, bool ) ),
              this, SLOT (onVisabilityChanged () ) );
    connect ( picker, SIGNAL ( featureStatusSet ( App::DocumentObject *,
                                                  PartDesignGui::FeaturePicker::StatusSet ) ),
              this, SLOT ( onFeatureStatusSet ( App::DocumentObject *,
                                                PartDesignGui::FeaturePicker::StatusSet ) ) );
}

inline std::vector <App::DocumentObject *> FeaturePickTwoPanelWidget::getSelectedFeatures () {
    std::vector <App::DocumentObject *> rv;

    // Get all items in the selected widget
    rv.reserve (selectedTreeWidget()->topLevelItemCount());
    for (int i=0; i<selectedTreeWidget()->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = selectedTreeWidget()->topLevelItem( i );

        auto itemIt = treeItems.right.find (item);
        if (itemIt != treeItems.right.end ()) {
            rv.push_back (itemIt->second);
        } else {
            Base::Console().Error ( "FeaturePicker select widget holds unknown QTreeWidgetItem\n" );
        }
    }

    return rv;
}

void  FeaturePickTwoPanelWidget::onWidgetSelectionChanged () {
    picker->setPickedFeatures (getSelectedFeatures ());
    onVisabilityChanged (); //< we don't know what widgets was moved or whatever so update visabilities for all
}

void FeaturePickTwoPanelWidget::onPickedFeaturesChanged () {
    // select the picked widget if it differs from currently selected
    auto sel = getSelectedFeatures ();

    const auto & picked = picker->getPickedFeatures ();

    if (sel != picked) {
        // block all signals from the model
        bool modelBlocked = selectedTreeWidget ()->model ()->blockSignals ( true );

        // remove all items from selectedTreeWidget
        while ( selectedTreeWidget ()->topLevelItemCount () ) {
            unselectItem ( 0 );
        }

        // And select the new set of features again
        for ( auto feat : picked ) {
            auto featIt = treeItems.left.find (feat);
            assert (featIt != treeItems.left.end ());
            selectItem ( featIt->second );
            featIt->second->setHidden (false);
        }

        selectedTreeWidget ()->model ()->blockSignals ( modelBlocked );
    }
}

void FeaturePickTwoPanelWidget::onFeatureStatusSet (
        App::DocumentObject *feat, PartDesignGui::FeaturePicker::StatusSet status)
{
    auto featIt = treeItems.left.find (feat);
    if (featIt == treeItems.left.end() ) {
        createTreeWidgetItem (feat);
    } else if (featIt->second->treeWidget () == availableTreeWidget() ) {
        featIt->second->setHidden ( !picker->isVisible (status) );
    }
}

QTreeWidgetItem *FeaturePickTwoPanelWidget::createTreeWidgetItem (
        App::DocumentObject * feat,
        const FeaturePicker::StatusSet & status )
{
    Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider (feat);
    assert (vp);

    QTreeWidgetItem *item =
        new QTreeWidgetItem ( QStringList ( QString::fromUtf8 ( feat->Label.getValue () ) ) );
    assert (item);
    item->setIcon ( 0, vp->getIcon () );

    // construct the tooltip
    QString toolTip;

    for (unsigned i=FeaturePicker::validFeature+1; i<status.size (); ++i) { //< Note skipping the validFeature
        if (status[i]) {
            toolTip.append ( FeaturePicker::getFeatureStatusString ( (FeaturePicker::FeatureStatus) i) ).
                append ( QLatin1String ("\n") );
        }
    }
    if (!toolTip.isEmpty () ) {
        item->setToolTip ( 0, toolTip );
    }

    treeItems.insert ( TreeWidgetItemMap::value_type ( feat, item ) );
    availableTreeWidget()->addTopLevelItem (item);

    // Hide the item if it shouldn't be visible
    item->setHidden (!picker->isVisible (status));

    return item;
}

void FeaturePickTwoPanelWidget::onVisabilityChanged () {
    for (auto featTreeItem : treeItems.left ) {
        if (featTreeItem.second->treeWidget () == availableTreeWidget()) {
            featTreeItem.second->setHidden ( !picker->isVisible (featTreeItem.first) );
        } else {
            featTreeItem.second->setHidden ( false );
        }
    }
}


/**********************************************************************
 *                      FeaturePickControlWidget                      *
 **********************************************************************/

FeaturePickControlWidget::FeaturePickControlWidget ( FeaturePicker *picker_s, QWidget *parent )
    : QWidget (parent), picker ( picker_s )
{
    assert ( picker );
    QBoxLayout * layout = new QVBoxLayout (this);
    this->setLayout ( layout );

    auto existingStatuses = picker->getFeatureStatusMask ( );

    // A commone code for adding a toolButton
    auto addToolButton = [this, layout, existingStatuses] (
            FeaturePicker::FeatureStatus status, const char * icon, const QString & toolTip) -> QToolButton * {
        QToolButton *btn = new QToolButton (this);

        btn->setIcon (Gui::BitmapFactory().pixmap(icon));
        btn->setToolTip (toolTip);
        btn->setCheckable (true);
        btn->setChecked ( this->picker->isVisible (status) );
        btn->setDisabled ( !existingStatuses[status] );
        this->layout ()->addWidget (btn);
        this->controlButtons.insert ( ButtonsBimap::value_type ( status , btn ) );
        connect (btn, SIGNAL (clicked (bool) ), this, SLOT( onStateButtonClicked (bool) ) );

        return btn;
    };

    // TODO all icons here are temporary and should be replaced later (2015-12-19, Fat-Zer)
    addToolButton (FeaturePicker::isUsed, "PartDesign_Pad", tr ("Show used features") );
    QToolButton * bodyBtn = addToolButton (FeaturePicker::otherBody, "PartDesign_Body_Tree",
            tr ("Show features which doesn't belong to current body") );
    QToolButton * partBtn = addToolButton (FeaturePicker::otherPart, "PartFeature",
            tr ("Show features which doesn't belong to current part") );
    addToolButton (FeaturePicker::afterTip, "PartDesign_MoveTip",
            tr ("Show features located in current body after the tip") );

    // If external features visability is disabled; disable the other body/part buttons also
    if ( !picker->isVisible ( FeaturePicker::isExternal ) ) {
        bodyBtn->setDisabled (true);
        partBtn->setDisabled (true);
    }

    layout->addStretch ();

    // apply right pushbutton states on modifications of the picker
    connect ( picker, SIGNAL ( visabilityChanged ( PartDesignGui::FeaturePicker::FeatureStatus, bool ) ),
              this, SLOT ( onVisabilityChanged ( PartDesignGui::FeaturePicker::FeatureStatus, bool ) ) );
    connect ( picker, SIGNAL ( featureStatusSet ( App::DocumentObject *,
                                                  PartDesignGui::FeaturePicker::StatusSet ) ),
              this, SLOT ( onFeatureStatusSet ( ) ) );
}

void FeaturePickControlWidget::onVisabilityChanged ( FeaturePicker::FeatureStatus status, bool state ) {
    auto tbIt = controlButtons.left.find (status);
    if ( tbIt != controlButtons.left.end () ) {
        tbIt->second->setChecked (state);
    }

    // on external visability changed update visability of the buttons
    if ( status == FeaturePicker::isExternal) {
        onFeatureStatusSet ();
    }
}

void FeaturePickControlWidget::onStateButtonClicked ( bool state ) {
    QToolButton *btn = qobject_cast <QToolButton *> (sender ());
    if ( !btn ) {
# ifdef FC_DEBUG
        Base::Console().Error( "FeaturePickControlWidget::onStateButtonClicked bad signal sender'\n");
# endif
        return;
    }

    auto statusIt = controlButtons.right.find (btn);

    if ( statusIt != controlButtons.right.end () ) {
        picker->setVisability (statusIt->second, state);
    }
}

void FeaturePickControlWidget::onFeatureStatusSet ( ) {
    auto existingStatuses = picker->getFeatureStatusMask ( );

    // Disable all buttons features for which statuses are not present in the picker
    for ( auto btnStat: controlButtons.right ) {
        btnStat.first->setDisabled ( !existingStatuses[btnStat.second] );
    }

    // If external features visability is disabled; disable the other body/part buttons also
    if ( !picker->isVisible ( FeaturePicker::isExternal ) ) {
        auto disableButton = [this] (FeaturePicker::FeatureStatus status) {
            auto tbIt = controlButtons.left.find (status);
            if ( tbIt != controlButtons.left.end () ) {
                tbIt->second->setDisabled ( true ) ;
            }
        };

        disableButton ( FeaturePicker::otherBody );
        disableButton ( FeaturePicker::otherPart );
    }
}


/**********************************************************************
 *                          TaskFeaturePick                           *
 **********************************************************************/

TaskFeaturePick::TaskFeaturePick ( FeaturePicker *picker, QWidget *parent )
    : TaskBox ( Gui::BitmapFactory().pixmap("edit-select-box"),
                QObject::tr("Select feature"), true, parent)
{
    // We need a separate container widget to add all controls to
    QWidget *proxy = new QWidget(this);
    this->addWidget(proxy);

    proxy->setLayout ( new QHBoxLayout (proxy) );

    proxy->layout ()->addWidget (new FeaturePickControlWidget (picker, proxy));

    if ( picker->isMultiPick () ) {
        FeaturePickTwoPanelWidget *pickWgt = new FeaturePickTwoPanelWidget (picker, proxy);
        proxy->layout ()->addWidget (pickWgt );
    } else {
        FeaturePickSingleSelectWidget *treeWidget = new FeaturePickSingleSelectWidget (picker, proxy);
        proxy->layout ()->addWidget (treeWidget );
    }
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
// void TaskFeaturePick::updateList()
// {
//     int index = 0;
//
//     for (std::vector<featureStatus>::const_iterator st = statuses.begin(); st != statuses.end(); st++) {
//         QListWidgetItem* item = ui->listWidget->item(index);
//
//         switch (*st) {
//             case validFeature: item->setHidden(false); break;
//             case invalidShape: item->setHidden(true); break;
//             case isUsed: item->setHidden(!ui->checkUsed->isChecked()); break;
//             case noWire: item->setHidden(true); break;
//             case otherBody: item->setHidden(!ui->checkOtherBody->isChecked()); break;
//             case otherPart: item->setHidden(!ui->checkOtherPart->isChecked()); break;
//             case notInBody: item->setHidden(!ui->checkOtherPart->isChecked()); break;
//             case basePlane: item->setHidden(false); break;
//             case afterTip:  item->setHidden(true); break;
//         }
//
//         index++;
//     }
// }
//
// void TaskFeaturePick::onUpdate(bool)
// {
//     bool enable = false;
//     if(ui->checkOtherBody->isChecked() || ui->checkOtherPart->isChecked())
//         enable = true;
//
//     ui->radioDependent->setEnabled(enable);
//     ui->radioIndependent->setEnabled(enable);
//     ui->radioXRef->setEnabled(enable);
//
//     updateList();
// }
//
// std::vector<App::DocumentObject*> TaskFeaturePick::getFeatures() {
//
//     features.clear();
//     QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
//     while (i.hasNext()) {
//
//         auto item = i.next();
//         if(item->isHidden())
//             continue;
//
//         QString t = item->text();
//         t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
//         features.push_back(t);
//     }
//
//     std::vector<App::DocumentObject*> result;
//
//     for (std::vector<QString>::const_iterator s = features.begin(); s != features.end(); s++)
//         result.push_back(App::GetApplication().getActiveDocument()->getObject(s->toAscii().data()));
//
//     return result;
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
//
// void TaskFeaturePick::onSelectionChanged(const Gui::SelectionChanges& msg)
// {
//     ui->listWidget->clearSelection();
//     for(Gui::SelectionSingleton::SelObj obj :  Gui::Selection().getSelection()) {
//
//         for(int row = 0; row < ui->listWidget->count(); row++) {
//
//             QListWidgetItem *item = ui->listWidget->item(row);
//             QString t = item->text();
//             t = t.left(t.indexOf(QString::fromAscii("(")) - 1);
//             if(t.compare(QString::fromAscii(obj.FeatName))==0) {
//                 ui->listWidget->setItemSelected(item, true);
//             }
//         }
//     }
// }
//
// void TaskFeaturePick::showExternal(bool val) {
//
//     ui->checkOtherBody->setChecked(val);
//     ui->checkOtherPart->setChecked(val);
//     updateList();
// }
//
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
