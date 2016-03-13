/******************************************************************************
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
# include <QToolButton>
# include <QTreeWidget>
# include <QTreeWidgetItem>
#endif

#include <Base/Console.h>

#include <App/Part.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/Widgets.h>

#include "FeaturePickerWidgets.h"


using namespace PartDesignGui;

/**********************************************************************
 *                    AbstractFeaturePickerWidget                     *
 **********************************************************************/

AbstractFeaturePickerWidget::AbstractFeaturePickerWidget (
        FeaturePicker *picker_s, QWidget *parent )
: QWidget(parent), picker (picker_s)
{
    assert (picker);

    visability [ FeaturePicker::validFeature ]    = true;
    // Due to external features are additionally constrained, enable them by default
    visability [ FeaturePicker::isExternal   ]    = true;
    visability [ FeaturePicker::basePlane    ]    = true;
//    visability [ userSelected ]    = true;

    QBoxLayout *mainLayout = new QHBoxLayout (this);

    QBoxLayout * pbLayout = new QVBoxLayout;

    auto existingStatuses = picker->getFeatureStatusMask ( );

    // A commone code for adding a toolButton
    auto addToolButton = [this, pbLayout, existingStatuses] (
            FeaturePicker::FeatureStatus status, const char * icon, const QString & toolTip ) -> QToolButton *
    {
        QToolButton *btn = new QToolButton (this);

        btn->setIcon (Gui::BitmapFactory().pixmap(icon));
        btn->setToolTip (toolTip);
        btn->setCheckable (true);
        btn->setChecked ( this->isVisible (status) );
        btn->setDisabled ( !existingStatuses[status] );
        pbLayout->addWidget (btn);
        this->controlButtons.insert ( ButtonsBimap::value_type ( status , btn ) );
        connect (btn, SIGNAL (clicked (bool) ), this, SLOT( onStateButtonClicked (bool) ) );

        return btn;
    };

    // TODO all icons here are temporary and should be replaced later (2015-12-19, Fat-Zer)
    addToolButton (FeaturePicker::isUsed, "PartDesign_Pad", tr ("Show used features") );

    addToolButton (FeaturePicker::otherBody, "PartDesign_Body_Tree",
            tr ("Show features which doesn't belong to current body") );
    addToolButton (FeaturePicker::otherPart, "PartFeature",
            tr ("Show features which doesn't belong to current part") );
    addToolButton (FeaturePicker::afterTip, "PartDesign_MoveTip",
            tr ("Show features located in current body after the tip") );

    pbLayout->addStretch ();

    mainLayout->addLayout (pbLayout);

    // TODO Save/load last visability states (2016-03-09, Fat-Zer)
    connect ( this, SIGNAL ( selectionChanged () ), this, SLOT ( updatePickedFeatures () ) );
    connect ( picker, SIGNAL ( pickedFeaturesChanged () ), this, SLOT (updateUi () ) );
    connect ( picker, SIGNAL ( featureStatusSet ( App::DocumentObject *,
                                                  PartDesignGui::FeaturePicker::StatusSet ) ),
              this, SLOT ( updateUi () ) );
}

AbstractFeaturePickerWidget::~AbstractFeaturePickerWidget () {

}

void  AbstractFeaturePickerWidget::updatePickedFeatures () {
    if (picker) { // note that picker may be destroyed before the widget
        auto selFeatures = getSelectedFeatures ();
        picker->setPickedFeatures (getSelectedFeatures ());
    }
}

void AbstractFeaturePickerWidget::setupContent (QWidget *cont) {
    QBoxLayout *layout = qobject_cast <QBoxLayout *> ( this->layout () );
    assert (layout);
    layout->addWidget (cont);
}

void AbstractFeaturePickerWidget::updateUi () {
    if (!picker) {
        return;
    }

    auto existingStatuses = picker->getFeatureStatusMask ( );

    for ( auto btnStat: controlButtons.left ) {
        // Disable the button features for which statuses are not present in the picker
        btnStat.second->setDisabled ( !existingStatuses[btnStat.first] );
        // update the checked status of buttons
        btnStat.second->setChecked ( visability[btnStat.first] );
    }
}

void AbstractFeaturePickerWidget::onStateButtonClicked ( bool state ) {
    QToolButton *btn = qobject_cast <QToolButton *> (sender ());
    if ( !btn ) {
# ifdef FC_DEBUG
        Base::Console().Error( "AbstractFeaturePickerWidget::onStateButtonClicked bad signal sender'\n");
# endif
        return;
    }

    auto statusIt = controlButtons.right.find (btn);

    if ( statusIt != controlButtons.right.end () ) {
        visability.set (statusIt->second, state);
    }

    updateUi ();
}

/**********************************************************************
 *                    TreeWidgetBasedFeaturePicker                    *
 **********************************************************************/

QTreeWidgetItem *TreeWidgetBasedFeaturePickerWidget::createTreeWidgetItem (
        QTreeWidget *treeWidget,
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
    treeWidget->addTopLevelItem (item);

    // Hide the item if it shouldn't be visible
    item->setHidden (!isVisible (status));

    return item;
}

void TreeWidgetBasedFeaturePickerWidget::update3dSelection () {
    bool blocked = Gui::SelectionObserver::blockConnection ( true );
    for ( auto featItem : treeItems.left ) {
        App::DocumentObject *feat = featItem.first;
        QTreeWidgetItem *item = featItem.second;
        if (item->isSelected ()) {
            if ( ! Gui::Selection ().isSelected (feat) ) {
                Gui::Selection().addSelection ( feat->getDocument ()->getName (), feat->getNameInDocument () );
            }
        } else {
            if ( Gui::Selection ().isSelected (feat) ) {
                Gui::Selection().rmvSelection ( feat->getDocument ()->getName (), feat->getNameInDocument () );
            }
        }
    }

    Gui::SelectionObserver::blockConnection ( blocked );
}

void TreeWidgetBasedFeaturePickerWidget::onSelectionChanged ( const Gui::SelectionChanges& /*msg*/) {
    std::set <App::DocumentObject *> selectionSet;

    for(Gui::SelectionSingleton::SelObj selObj :  Gui::Selection().getSelection()) {
        selectionSet.insert (selObj.pObject);
    }

    for ( auto featItem : treeItems.left ) {
        App::DocumentObject *feat = featItem.first;
        QTreeWidgetItem *item = featItem.second;

        if (selectionSet.find (feat) != selectionSet.end ()) {
            if ( ! item->treeWidget ()->isItemHidden ( item ) ) {
                item->setSelected (true);
            }
        } else {
            item->setSelected (false);
        }
    }
}


/**********************************************************************
 *                   FeaturePickSingleSelectWidget                    *
 **********************************************************************/

FeaturePickerSinglePanelWidget::FeaturePickerSinglePanelWidget (
        FeaturePicker *picker_s, QWidget *parent )
:  TreeWidgetBasedFeaturePickerWidget ( picker_s, parent )
{
    treeWidget = new QTreeWidget (this);
    treeWidget->setHeaderHidden ( true );
    treeWidget->sortByColumn ( 0 );
    treeWidget->setSortingEnabled ( 0 );
    treeWidget->setSelectionBehavior ( QAbstractItemView::SelectRows );

    setupContent(treeWidget);

    if (getPicker ()->isMultiPick ()) {
        treeWidget->setSelectionMode ( QAbstractItemView::MultiSelection );
    } else {
        treeWidget->setSelectionMode ( QAbstractItemView::SingleSelection );
    }

    updateUi ();

    connect ( treeWidget, SIGNAL ( itemSelectionChanged () ), this, SIGNAL ( selectionChanged () ) );
    connect ( treeWidget, SIGNAL ( itemSelectionChanged () ), this, SLOT ( update3dSelection () ) );
}


std::vector <App::DocumentObject *> FeaturePickerSinglePanelWidget::getSelectedFeatures () {
    std::vector <App::DocumentObject *> rv;

    auto sel = treeWidget->selectedItems();

    if (getPicker ()->isMultiPick ()) {
        rv.reserve ( sel.size() );

        for (auto item: sel) {
            auto itemIt = treeItems.right.find (item);
            if (itemIt != treeItems.right.end ()) {
                rv.push_back (itemIt->second);
            } else {
                Base::Console().Error ( "FeaturePickerSinglePanelWidget holds unknown QTreeWidgetItem\n" );
            }
        }
    } else {
        if (sel.size() != 0) {
            auto treeWgtIt = treeItems.right.find ( sel.front() );
            assert ( treeWgtIt != treeItems.right.end() );
            rv.push_back (treeWgtIt->second);
        } else {
            // Do nothing; rv already empty
        }
    }

    return rv;
}

void FeaturePickerSinglePanelWidget::updateUi() {
    const auto & picked = getPicker ()->getPickedFeatures ();
    std::set<App::DocumentObject *> pickedSet (picked.begin(), picked.end());

    bool wasBlocked = this->blockSignals ( true );
    bool selected = false;

    for ( auto featStat : getPicker ()->getFeaturesStatusMap () ) {
        App::DocumentObject *feat = featStat.first;
        auto featIt = treeItems.left.find (feat);

        QTreeWidgetItem *twItem;
        if (featIt != treeItems.left.end ()) {
            twItem = featIt->second;
        } else {
            twItem = createTreeWidgetItem (featStat.first, featStat.second);
        }

        // Select the feature if it is picked
        if (getPicker ()->isMultiPick () || !selected) {
            if (pickedSet.find (feat) != pickedSet.end()) {
                twItem->setSelected (true);
                selected=true;
            }
        } else {
            twItem->setSelected (false);
        }

        twItem->setHidden ( !isVisible (featStat.second) );
    }

    this->blockSignals (wasBlocked);

    TreeWidgetBasedFeaturePickerWidget::updateUi ();
}


/**********************************************************************
 *                     FeaturePickTwoPanelWidget                      *
 **********************************************************************/
FeaturePickerDoublePanelWidget::FeaturePickerDoublePanelWidget (
        FeaturePicker *picker_s, QWidget *parent )
: TreeWidgetBasedFeaturePickerWidget ( picker_s, parent )
{
    if (!getPicker ()->isMultiPick ()) {
        throw Base::Exception ( "FeaturePickerDoublePlaneWidget couldn't be used for single selection\n" );
    }

    actionSelector = new Gui::ActionSelector (this);

    setupContent(actionSelector);

    updateUi ();

    // Due to QTreeWidgets don't have signals on features added/removed, attach to the model
    QAbstractItemModel *selectedModel = actionSelector->selectedTreeWidget ()->model ();

    connect ( selectedModel, SIGNAL ( rowsInserted (const QModelIndex &, int, int) ),
              this, SIGNAL ( selectionChanged () ) );
    connect ( selectedModel, SIGNAL ( rowsRemoved (const QModelIndex &, int, int) ),
              this, SIGNAL ( selectionChanged () ) );
    connect ( selectedModel, SIGNAL ( rowsMoved ( const QModelIndex &, int, int,
                                                  const QModelIndex &, int ) ),
              this, SIGNAL ( selectionChanged () ) );

    connect ( actionSelector->availableTreeWidget(), SIGNAL ( itemSelectionChanged () ),
            this, SLOT ( update3dSelection () ) );
    connect ( actionSelector->selectedTreeWidget(), SIGNAL ( itemSelectionChanged () ),
            this, SLOT ( update3dSelection () ) );
}


std::vector <App::DocumentObject *> FeaturePickerDoublePanelWidget::getSelectedFeatures () {
    std::vector <App::DocumentObject *> rv;

    // Get all items in the selected widget
    rv.reserve (actionSelector->selectedTreeWidget()->topLevelItemCount());
    for (int i=0; i<actionSelector->selectedTreeWidget()->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = actionSelector->selectedTreeWidget()->topLevelItem( i );

        auto itemIt = treeItems.right.find (item);
        if (itemIt != treeItems.right.end ()) {
            rv.push_back (itemIt->second);
        } else {
            Base::Console().Error ( "FeaturePicker select widget holds unknown QTreeWidgetItem\n" );
        }
    }

    return rv;
}

void FeaturePickerDoublePanelWidget::updateUi() {
    const auto & picked = getPicker ()->getPickedFeatures ();
    std::set<App::DocumentObject *> pickedSet (picked.begin(), picked.end());

    bool wasBlocked = this->blockSignals ( true );

    for ( auto featStat : getPicker ()->getFeaturesStatusMap () ) {
        App::DocumentObject *feat = featStat.first;
        auto featIt = treeItems.left.find (feat);

        QTreeWidgetItem *twItem;
        if (featIt != treeItems.left.end ()) {
            twItem = featIt->second;
        } else {
            twItem = createTreeWidgetItem (featStat.first, featStat.second);
        }

        // move the feature to selected if it is picked
        if (pickedSet.find (feat) != pickedSet.end()) {
            actionSelector->selectItem (twItem);
            twItem->setHidden (false);
        } else {
            actionSelector->unselectItem (twItem);
            twItem->setHidden ( !isVisible (featStat.second) );
        }
    }

    this->blockSignals (wasBlocked);

    TreeWidgetBasedFeaturePickerWidget::updateUi ();
}

inline QTreeWidgetItem * FeaturePickerDoublePanelWidget::createTreeWidgetItem (
        App::DocumentObject *feat, const FeaturePicker::StatusSet &stat)
{
    return TreeWidgetBasedFeaturePickerWidget::createTreeWidgetItem (
            actionSelector->availableTreeWidget(), feat, stat);
}

#include "moc_FeaturePickerWidgets.cpp"
