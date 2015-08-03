/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
#include <vector>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <QMessageBox>
#endif

#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include "WorkflowManager.h"


using namespace PartDesignGui;


WorkflowManager * WorkflowManager::_instance = nullptr;


WorkflowManager::WorkflowManager() {
    connectNewDocument = App::GetApplication().signalNewDocument.connect(
            boost::bind( &WorkflowManager::slotNewDocument, this, _1 ) );
    connectFinishRestoreDocument = App::GetApplication().signalFinishRestoreDocument.connect(
            boost::bind( &WorkflowManager::slotFinishRestoreDocument, this, _1 ) );
    connectDeleteDocument = App::GetApplication().signalDeleteDocument.connect(
            boost::bind( &WorkflowManager::slotDeleteDocument, this, _1 ) );
}

WorkflowManager::~WorkflowManager() {
    // won't they will be disconnected on destruction?
    connectNewDocument.disconnect ();
    connectFinishRestoreDocument.disconnect ();
}


// Those destruction/construction is not really needed and could be done in the instance()
// but to make things a bit more cleare better to keep them around.
void WorkflowManager::init() {
    if (_instance)
        throw Base::Exception( "Trying to init the workflow manager second time." );
    _instance = new WorkflowManager();
}

WorkflowManager *WorkflowManager::instance() {
    if (!_instance)
        throw Base::Exception( "Trying to instance the WorkflowManager manager before init() was called." );
    return _instance;
}

void WorkflowManager::destruct() {
    if (!_instance)
        throw Base::Exception( "Workflow manager wasn't inited properly" );
    delete _instance;
    _instance = nullptr;
}

void WorkflowManager::slotNewDocument( const App::Document &doc ) {
    // new document always uses new workflow
    dwMap[&doc] = Workflow::Modern;
}


void WorkflowManager::slotFinishRestoreDocument( const App::Document &doc ) {
    // mark new document as undetermined
    Workflow wf = guessWorkflow (&doc);
    if( wf != Workflow::Modern ) {
        wf = Workflow::Undetermined;
    }
    dwMap[&doc] = wf;
}

void WorkflowManager::slotDeleteDocument( const App::Document &doc ) {
    dwMap.erase(&doc);
}

Workflow WorkflowManager::getWorkflowForDocument( App::Document *doc) {
    assert (doc);

    auto it = dwMap.find(doc);

    if ( it!=dwMap.end() ) {
        return it->second;
    } else {
        // We haven't yet checked the file workflow
        // May happen if e.g. file not compleatly loaded yet
        return Workflow::Undetermined;
    }
}

Workflow WorkflowManager::determinWorkflow( App::Document *doc) {
    Workflow rv = getWorkflowForDocument (doc);

    if (rv != Workflow::Undetermined) {
        // return if workflow is known
        return rv;
    }

    // guess the workflow again
    rv = guessWorkflow (doc);

    if (rv != Workflow::Modern) {
        QMessageBox msgBox;
        msgBox.setText( QObject::tr( "The document \"%1\" you are editing was design with old version of "
                        "PartDesign workbench." ).arg( QString::fromStdString ( doc->getName()) ) );
        msgBox.setInformativeText (
                QObject::tr( "Do you want to migrate in order to use modern PartDesign features?" ) );
        msgBox.setDetailedText( QObject::tr( "Note If you choose to migrate you won't be able to edit"
                    " the file wtih old FreeCAD versions.\n"
                    "If you refuse to migrate ypu won't be able to use new PartDesign features"
                    " like Bodies and Parts. As a result you also won't be able to use your parts"
                    " in the assembly workbench.\n"
                    "Although you will be able to migrate any moment later with 'Part Design->Migrate'." ) );
        msgBox.setIcon( QMessageBox::Question );
        msgBox.setStandardButtons ( QMessageBox::Yes | QMessageBox::No );
        msgBox.setDefaultButton( QMessageBox::Yes );
        // TODO Add prompt for expert mode (2015-08-02, Fat-Zer)

        int rc = msgBox.exec();
        if (rc == QMessageBox::Yes) {
            // TODO Assure thet this will actually work (2015-08-02, Fat-Zer)
            Gui::Application::Instance->commandManager().runCommandByName("PartDesign_Migrate");
            // TODO make sure that migration was success and full
            rv = Workflow::Modern;
        } else {
            if ( rv == Workflow::Mixed ) {
                QMessageBox::warning( Gui::getMainWindow(), QObject::tr("Expert mode"),
                        QObject::tr( "You are editing the document in expert mode which allows"
                            " mixing of old and new PartDesign workflows. It's provided in intention"
                            " to be used to migrate complex files. Use at your own risk." ) );
            }
        }
    }

    dwMap[ doc ] = rv;
    return rv;
}

void WorkflowManager::forceWorkflow( const App::Document *doc, Workflow wf) {
    dwMap[ doc ] = wf;
}

void WorkflowManager::migrate (const App::Document *doc) {
//    Gui::Command::openCommand("Migrate part to Body feature");
//
//    std::set<PartDesign::Feature*> features;
//
//    // retrive all PartDesign Features objects and filter out features already belongs to some body
//    for (auto feat: doc->getObjectsOfType( PartDesign::Feature::getClassTypeId() )) {
//         if( !PartDesign::Body::findBodyOf( feat ) ) {
//             assert( feat->isDerivedFrom( PartDesign::Feature::getClassTypeId() ) );
//            features.insert ( static_cast <PartDesign::Feature *>( feat ) );
//         }
//    }
//
//    if (features.empty()) {
//        // Huh? nothing to migrate?
//        // TODO Some msgbox/error/throw/assert is needed here (2015-08-03, Fat-Zer)
//        return;
//    }
//
//    // Put features into chains. Each chain should become a separate body.
//    std::list<std::list<App::DocumentObject*>> featureChains;
//
//    std::list<App::DocumentObject*> chain;
//
//    std::list<App::DocumentObject*>::iterator baseFeatIt;
//
//    for (auto featIt = features.begin(); !features.empty(); ) {
//
//        features.erase(featIt);
//
//        Part::Feature *base = (*featIt)->getBaseObject( /*silent =*/ true);
//
//        if( !base || !base->isDerivedFrom( PartDesign::Feature::getClassTypeId() ) ) {
//            // a feature based on nothing as well as based on non-partdesign solid starts a new chain
//            new_chain.push_front( *featIt );
//            featureChains.push_back( new_chain );
//        } else {
//
//            PartDesign::Feature *baseFeat = static_cast <PartDesign::Feature *>( *featIt );
//            // Find if already showd up in some chain.
//            auto chainIt = std::find_if( featureChains.begin(), featureChains.end(),
//                    [baseFeat, &baseFeatIt] ( std::list<App::DocumentObject*> &chain) mutable -> bool {
//                        baseFeatIt = std::find( chain.begin(), chain.end(), baseFeat );
//                        return baseFeatIt !=  chain.end();
//                    } );
//
//            if( chainIt != featureChains.end() ) {
//                // the base of the chain is already in some other chain
//                assert( baseFeatIt != chainIt->end() );
//                auto next = baseFeatIt++;
//                if( next != chainIt->end() ) {
//                    // TODO some message or a special treat is required to split up's  (2015-08-03, Fat-Zer)
//                }
//                chainIt->insert (next, *featIt);
//                continue;
//            }
//        }
//
//        chain.push_back (*featIt);
//        featIt = features.begin()
//    }
//
}

Workflow WorkflowManager::guessWorkflow(const App::Document *doc) {
    // Retrive bodies of the document
    auto features = doc->getObjectsOfType( PartDesign::Feature::getClassTypeId() );

    if( features.empty() ) {
        // a new file should be done in the new workflow
        return Workflow::Modern;
    } else {
        auto bodies = doc->getObjectsOfType( PartDesign::Body::getClassTypeId() );
        if (bodies.empty()) {
            // If there are no bodies workflow is legacy
            return Workflow::Legacy;
        } else {
            bool features_without_bodies = false;

            for( auto feat: features ) {
                if( !PartDesign::Body::findBodyOf( feat ) ) {
                    features_without_bodies = true;
                    break;
                }
            }
            // if there are features not belonging to any body it's mixed mode, otherwice it's Modern
            return features_without_bodies ? Workflow::Mixed : Workflow::Modern;
        }
    }
}
