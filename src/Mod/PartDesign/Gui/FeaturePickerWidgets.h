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
    AbstractFeaturePickerWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );
    virtual ~AbstractFeaturePickerWidget () = 0;

    /// Returns the list of features selected by the widget
    virtual std::vector <App::DocumentObject *> getSelectedFeatures () = 0;

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

Q_SIGNALS:
    /// The signal should be emitted by derived class when feature selection for picker is changed
    void selectionChanged ();

    /// Emmited when visability for specific status changed
    void visabilityChanged (PartDesignGui::FeaturePicker::FeatureStatus status, bool state);

public Q_SLOTS:
    /// Update the associated FeaturePicker according to changes inside widget
    virtual void updatePickedFeatures ();

    /// Update the widget according to changes in featurePicker
    virtual void updateUi();

private Q_SLOTS:
    /// A callback on a button getting clicked
    void onStateButtonClicked ( bool state );

protected:
    /// Sets up the user interface widget for derived classes
    void setupContent (QWidget *wgt);

private:
    FeaturePicker *picker;
    FeaturePicker::StatusSet visability;

    typedef boost::bimap< FeaturePicker::FeatureStatus, QToolButton *> ButtonsBimap;
    ButtonsBimap controlButtons;
};


/**
 * A common base class for tree widget based widgets
 * @see FeaturePickerSinglePanelWidget FeaturePickerDoublePanelWidget
 */
class TreeWidgetBasedFeaturePickerWidget: public AbstractFeaturePickerWidget {
    Q_OBJECT
public:
    TreeWidgetBasedFeaturePickerWidget ( FeaturePicker *picker_s, QWidget *parent = 0 )
        :AbstractFeaturePickerWidget (picker_s, parent) { }

protected:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem ( QTreeWidget *treeWidget,
            App::DocumentObject *feat,
            const FeaturePicker::StatusSet & status);

    typedef boost::bimap < App::DocumentObject *, QTreeWidgetItem *> TreeWidgetItemMap;
    TreeWidgetItemMap treeItems;
};


/**
 * A widget to display the content of FeaturePicker in a single QTreeWidget
 */
class FeaturePickerSinglePanelWidget: public TreeWidgetBasedFeaturePickerWidget {
    Q_OBJECT
public:
    FeaturePickerSinglePanelWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );

    virtual std::vector <App::DocumentObject *> getSelectedFeatures ();

public Q_SLOTS:
    virtual void updateUi();

private:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat) {
        return createTreeWidgetItem ( feat, getPicker ()->getStatus ( feat ) );
    }

    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat, const FeaturePicker::StatusSet &stat) {
        return TreeWidgetBasedFeaturePickerWidget::createTreeWidgetItem ( treeWidget, feat, stat);
    }

private:
    QTreeWidget *treeWidget;
};


/**
 * A widget to display the content of FeaturePicker with two pannels
 */
class FeaturePickerDoublePanelWidget: public TreeWidgetBasedFeaturePickerWidget {
    Q_OBJECT
public:
    FeaturePickerDoublePanelWidget ( FeaturePicker *picker_s, QWidget *parent = 0 );

    virtual std::vector <App::DocumentObject *> getSelectedFeatures ();

public Q_SLOTS:
    virtual void updateUi();

private:
    /// Constructs a new tree widget item for the given feature
    QTreeWidgetItem * createTreeWidgetItem (App::DocumentObject *feat) {
        return createTreeWidgetItem ( feat, getPicker ()->getStatus ( feat ) );
    }

    /// Constructs a new tree widget item for the given feature
    inline QTreeWidgetItem * createTreeWidgetItem (
            App::DocumentObject *feat, const FeaturePicker::StatusSet &stat);

private:
    Gui::ActionSelector *actionSelector;
};

} /* PartDesignGui */

#endif /* end of include guard: FEATUREPICKERWIDGETS_H_ESFQFHD1 */
