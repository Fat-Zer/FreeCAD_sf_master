/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2013 Stefan Tröger  <stefantroeger@gmx.net>             *
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


#ifndef Assembly_Item_H
#define Assembly_Item_H

#include <App/PropertyStandard.h>
#include <App/GeoFeature.h>
#include <TopoDS_Shape.hxx>

namespace Assembly
{

class AssemblyExport Item : public App::GeoFeature
{
    PROPERTY_HEADER(Assembly::Item);

public:
    Item();
	~Item() {};

   /** @name base properties of all Assembly Items 
     * This properties corospond mostly to the meta information
     * in the App::Document class
     */
    //@{
    /// Id e.g. Part number
    App::PropertyString  Id;
    /// unique identifier of the Item 
    App::PropertyUUID    Uid;
    /// long description of the Item 
    App::PropertyString  Description  ;
    /// creators name (utf-8)
    App::PropertyString  CreatedBy;
    App::PropertyString  CreationDate;
    /// user last modified the document
    App::PropertyString  LastModifiedBy;
    App::PropertyString  LastModifiedDate;
    /// company name UTF8(optional)
    App::PropertyString  Company;
    /// long comment or description (UTF8 with line breaks)
    App::PropertyString  Comment;
    /** License string
      * Holds the short license string for the Item, e.g. CC-BY
      * for the Creative Commons license suit. 
      */
    App::PropertyString  License;
    /// License descripton/contract URL
    App::PropertyString  LicenseURL;
    /// Meta descriptons
    App::PropertyMap     Meta;
    /// Meta descriptons
    App::PropertyMap     Material;
    //@}

    /** @name Visual properties */
    //@{
    /** Base color of the Item
        If the transparency value is 1.0
        the color or the next hirachy is used
        */
    App::PropertyColor Color;
    /// Visibility
    App::PropertyBool  Visibility;
    //@}
    
    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderItem";
    }
    //@}

    virtual TopoDS_Shape getShape(void)const =0 ;

    PyObject *getPyObject(void);
    
};

} //namespace Assembly


#endif // ASSEMBLY_Item_H
