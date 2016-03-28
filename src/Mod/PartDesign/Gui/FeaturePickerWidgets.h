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

#ifndef FEATUREPICKERWIDGETS_H_ESFQFHD1
#define FEATUREPICKERWIDGETS_H_ESFQFHD1

#include <QWidget>
#include <QPointer>
#include <Gui/Selection.h>

#include <boost/bimap.hpp>

#include "FeaturePicker.h"


class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace Gui {
    class ActionSelector;
}

namespace PartDesignGui
{

/**
 * A widget to display the content of FeaturePicker
 */
class AbstractFeaturePickerWidget: public QWidget {
    Q_OBJECT
public:
    AbstractFeaturePickerWidget ( FeaturePicker *picker_s, QWidget *parent = nullptr );
    explicit AbstractFeaturePickerWidget ( QWidget *parent = nullptr )
        :AbstractFeaturePickerWidget ( nullptr, parent ) {}

    virtual ~AbstractFeaturePickerWidget () = 0;

    /// Returns the list of features selected by the widget
    virtual std::vector <App::DocumentObject *> getSelectedFeatures () = 0;

    /// Set the assotiated picker
    void setPicker (FeaturePicker *picker_s);

    /**
     * If any bit in a feature's status is not set in the visability set when the feature
     * would be hidden in the tree view.
     * @return true if features with given status are set to be displayed
     */
    bool isVisible (FeaturePicker::FeatureStatus status) const {
        return visability[status];
    }

    /**
     * If any bit in a feature's status is not set in the visability set when the feature
     * would be hidden in the tree view.
     * @return true if features with given status set are ment to be displayed
     */
    bool isVisible (FeaturePicker::StatusSet status) const {
        return ( status & ~visability ).none ();
    }

    /// Returns a picker associated with the widger
    FeaturePicker * getPicker () { return picker; }

    /// Returns a current visability settings of features
    const FeaturePicker::StatusSet & getVisability () { return visability; }

    /// Returns a vector of features sorted in order for display purposes
    const std::vector<App::DocumentObject *> & getSortedFeatures () {
        return sortedFeatures;
    }

Q_SIGNALS:
    /// The signal should be emitted by derived class when feature selection for picker is changed
    void selectionChanged ();

public Q_SLOTS:
    /// Update the associated FeaturePicker according to changes inside widget
    virtual void updatePickedFeatures ();

    /// Update the widget according to changes in featurePicker
    virtual void updateUi();

private Q_SLOTS:
    /// A callback on a button getting clicked
    void onStateButtonClicked ( bool state );

    /// Resort the features
    void updateSortedFeatures ();

private:
    void sortFeatures ();
protected:
    /// Sets up the user interface widget for derived classes
    void setupContent (QWidget *wgt);

private:
    QPointer<FeaturePicker> picker;
    FeaturePicker::StatusSet visability;

    typedef boost::bimap< FeaturePicker::FeatureStatus, QToolButton *> ButtonsBimap;
    ButtonsBimap controlButtons;

    std::vector<App::DocumentObject *> sortedFeatures;
};


/**
 * A common base class for tree widget based widgets
 * @see FeaturePickerSinglePanelWidget FeaturePickerDoublePanelWidget
 */
class TreeWidgetBasedFeaturePickerWidget: public AbstractFeaturePickerWidget, protected Gui::SelectionObserver {
    Q_OBJECT
public:
    TreeWidgetBasedFeaturePickerWidget ( FeaturePicker *picker_s, QWidget *parent = 0 )
        :AbstractFeaturePickerWidget (picker_s, parent) { }
    explicit TreeWidgetBasedFeaturePickerWidget ( QWidget *parent = nullptr )
        :TreeWidgetBasedFeaturePickerWidget ( nullptr, parent ) {}

public Q_SLOTS:
    /// Update the widget according to changes in featurePicker
    virtual void updateUi() {
        update3dSelection ( );
        AbstractFeaturePickerWidget::updateUi ();
    }

    /// Select items in the 3d view according to current selection in a tree widget
    void update3dSelection ( );

protected:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem ( QTreeWidget *treeWidget,
            App::DocumentObject *feat,
            const FeaturePicker::StatusSet & status);

    /// Adjusts the selection according to the changes in the 3d View
    void onSelectionChanged ( const Gui::SelectionChanges& msg);

    typedef boost::bimap < App::DocumentObject *, QTreeWidgetItem *> TreeWidgetItemMap;
    TreeWidgetItemMap treeItems;
};


/**
 * A widget to display the content of FeaturePicker in a single QTreeWidget
 */
class FeaturePickerSinglePanelWidget: public TreeWidgetBasedFeaturePickerWidget {
    Q_OBJECT
public:
    FeaturePickerSinglePanelWidget ( FeaturePicker *picker_s, bool s_multipick, QWidget *parent = 0 );
    explicit FeaturePickerSinglePanelWidget ( FeaturePicker *picker_s, QWidget *parent = 0 )
        : FeaturePickerSinglePanelWidget (picker_s, false, parent ) {}
    explicit FeaturePickerSinglePanelWidget ( QWidget *parent = nullptr )
        :FeaturePickerSinglePanelWidget ( nullptr, false, parent ) {}
    explicit FeaturePickerSinglePanelWidget ( bool s_multipick, QWidget *parent = nullptr )
        :FeaturePickerSinglePanelWidget ( nullptr, s_multipick, parent ) {}

    virtual std::vector <App::DocumentObject *> getSelectedFeatures ();

public Q_SLOTS:
    virtual void updateUi();

private:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat, const FeaturePicker::StatusSet &stat) {
        return TreeWidgetBasedFeaturePickerWidget::createTreeWidgetItem ( treeWidget, feat, stat);
    }

private:
    QTreeWidget *treeWidget;
    bool multipick;
};


/**
 * A widget to display the content of FeaturePicker with two pannels
 */
class FeaturePickerDoublePanelWidget: public TreeWidgetBasedFeaturePickerWidget {
    Q_OBJECT
public:
    FeaturePickerDoublePanelWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );
    explicit FeaturePickerDoublePanelWidget ( QWidget *parent = nullptr )
        :FeaturePickerDoublePanelWidget ( nullptr, parent ) {}

    virtual std::vector <App::DocumentObject *> getSelectedFeatures ();

public Q_SLOTS:
    virtual void updateUi();

private:
    /// Constructs a new tree widget item for the given feature
    inline QTreeWidgetItem * createTreeWidgetItem (
            App::DocumentObject *feat, const FeaturePicker::StatusSet &stat);

private:
    Gui::ActionSelector *actionSelector;
};

} /* PartDesignGui */

#endif /* end of include guard: FEATUREPICKERWIDGETS_H_ESFQFHD1 */
