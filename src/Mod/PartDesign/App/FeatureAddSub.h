/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_FeatureAdditive_H
#define PARTDESIGN_FeatureAdditive_H

#include <Mod/Part/App/PropertyTopoShape.h>

#include "Feature.h"

/// Base class of all additive features in PartDesign
namespace PartDesign
{

class PartDesignExport FeatureAddSub : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::FeatureAddSub);

public:
    /// The type of the addsub feature
    enum Type {
        Additive = 0,
        Subtractive
    };

    FeatureAddSub();

    /// The added or substracted shape for the feature
    Part::PropertyPartShape   AddSubShape;

    /// Returns addSubType value
    Type getAddSubType()
        { return addSubType; } 
    
    /**
     * A general execute logic for addictive and substractive features that actually
     * performs the addition and substraction. Should be called in the end of the execute
     * in derived classes after setting the AddSubShape property to correct value
     */
    // TODO Make all derived classes use it (2015-12-07, Fat-Zer)
    virtual App::DocumentObjectExecReturn *execute (void);

protected:
    /// Determines whether the feature addictive or substractive
    Type addSubType;
};

} //namespace PartDesign


#endif // PARTDESIGN_FeatureAdditive_H
