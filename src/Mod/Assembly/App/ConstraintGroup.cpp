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


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Placement.h>
#include <Base/Console.h>

#include "ConstraintGroupPy.h"
#include "ConstraintGroup.h"
#include "ItemPart.h"
#include "ItemAssembly.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ConstraintGroup, App::DocumentObject)

ConstraintGroup::ConstraintGroup()
{
    ADD_PROPERTY(Constraints,(0));
}

PyObject *ConstraintGroup::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ConstraintGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void ConstraintGroup::addConstraint(Constraint* c) 
{
    Base::Console().Message("add constraint to group\n");
  
    //add the constraint to our list
    const std::vector< App::DocumentObject * > &vals = this->Constraints.getValues();
    std::vector< App::DocumentObject * > newVals(vals);
    newVals.push_back(c);
    this->Constraints.setValues(newVals); 
    
    //let's retrieve the solver if not already done
    ItemAssembly* assembly = NULL;
    if(!m_solver || !assembly) {
      
	typedef std::vector<App::DocumentObject*>::iterator iter;
	std::vector<App::DocumentObject*> vec =  getInList();
      
	for(iter it = vec.begin(); it!=vec.end(); it++) {
	 
	  if( (*it)->getTypeId() == ItemAssembly::getClassTypeId() ) {
	      assembly = static_cast<ItemAssembly*>(*it);
	      m_solver = assembly->m_solver;
	      break;
	  };
	}
    };
    
    //check if we have been successfull
    if(!m_solver) {
      Base::Console().Message("ConstraintGroup: Unable to retrieve assembly solver\n");
      return;
    };
    if(!assembly) {
      Base::Console().Message("ConstraintGroup: Unable to retrieve assembly\n");
      return;
    };
            
    //init the constraint
    c->init(m_solver);
    
    //solve the system and propagate the change upstream
    m_solver->solve();
    assembly->touch();
}


short ConstraintGroup::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ConstraintGroup::execute(void)
{
 
    Base::Console().Message("Recalculate constraint group\n");
    return App::DocumentObject::StdReturn;
}

}
