/*===========================================================================*\
 *                                                                           *
 *                              OpenFlipper                                  *
 *      Copyright (C) 2001-2010 by Computer Graphics Group, RWTH Aachen      *
 *                           www.openflipper.org                             *
 *                                                                           *
 *---------------------------------------------------------------------------*
 *  This file is part of OpenFlipper.                                        *
 *                                                                           *
 *  OpenFlipper is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU Lesser General Public License as           *
 *  published by the Free Software Foundation, either version 3 of           *
 *  the License, or (at your option) any later version with the              *
 *  following exceptions:                                                    *
 *                                                                           *
 *  If other files instantiate templates or use macros                       *
 *  or inline functions from this file, or you compile this file and         *
 *  link it with other files to produce an executable, this file does        *
 *  not by itself cause the resulting executable to be covered by the        *
 *  GNU Lesser General Public License. This exception does not however       *
 *  invalidate any other reasons why the executable file might be            *
 *  covered by the GNU Lesser General Public License.                        *
 *                                                                           *
 *  OpenFlipper is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU Lesser General Public License for more details.                      *
 *                                                                           *
 *  You should have received a copy of the GNU LesserGeneral Public          *
 *  License along with OpenFlipper. If not,                                  *
 *  see <http://www.gnu.org/licenses/>.                                      *
 *                                                                           *
\*===========================================================================*/

/*===========================================================================*\
 *                                                                           *
 *   $Revision: 9595 $                                                       *
 *   $Author: moebius $                                                      *
 *   $Date: 2010-06-17 12:48:23 +0200 (Do, 17. Jun 2010) $                   *
 *                                                                           *
\*===========================================================================*/

#include <QtGui>

#include "MovePlugin.hh"

#include <iostream>
#include <ACG/GL/GLState.hh>
#include <QStringList>
#include <ACG/QtScenegraph/QtTranslationManipulatorNode.hh>

#include <OpenFlipper/BasePlugin/PluginFunctions.hh>
#include <OpenFlipper/common/GlobalOptions.hh>

#include <MeshTools/MeshFunctions.hh>
#include <MeshTools/MeshInfoT.hh>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_POLYLINE_SUPPORT
#include <ObjectTypes/PolyLine/PolyLine.hh>
#endif

#ifdef ENABLE_TSPLINEMESH_SUPPORT
#include <ObjectTypes/TSplineMesh/TSplineMesh.hh>
#endif

/** \brief Default Constructor
 *
 */
MovePlugin::MovePlugin() :
    placeAndSnapAction_(0),
    manMode_(QtTranslationManipulatorNode::TranslationRotation),
    contextAction_(0),
    toAllTargets_(0),
    placeMode_(false),
    recursiveJointTransformation_(true),
    transformRefPose_(true),
    transformCurrentPose_(false)
{
    manip_size_          = 1.0;
    manip_size_modifier_ = 1.0;

    selectionType_ = VERTEX;
    selectionConnected_ = false;

    axisA_ = 0;
    axisB_ = 1;

    hide_ = true;
    allTargets_ = false;
}

/** \brief Deconstructor
 *
 */
MovePlugin::~MovePlugin() {

	if(contextAction_) {
		delete contextAction_;
	}

	if(toAllTargets_) {
		delete toAllTargets_;
	}

	for(QList<movePropsWidget*>::iterator it = propsWindows_.begin();
		it != propsWindows_.end(); ++it) {

		if(*it) {
			delete *it;
		}
	}
}


/*******************************************************************************
        BaseInterface implementation
 *******************************************************************************/

/** \brief Initialization of the plugin when it is loaded by the core
 *
 */
void MovePlugin::pluginsInitialized() {

  //PICKMODES
  emit addPickMode("Separator");
  emit addHiddenPickMode("Move");
  emit setPickModeMouseTracking ("Move", true);
  emit addHiddenPickMode("MoveSelection");
  emit setPickModeMouseTracking ("MoveSelection", true);
  
#ifdef ENABLE_SKELETON_SUPPORT
  emit addHiddenPickMode("MoveSkeleton");
  emit setPickModeMouseTracking ("MoveSkeleton", true);
#endif

  //KEYS
  emit registerKey (Qt::Key_Shift, Qt::ShiftModifier, tr("Manipulator rotation"), true);
  emit registerKey (Qt::Key_Shift, Qt::NoModifier, tr("Manipulator rotation"), true);
  emit registerKey (Qt::Key_Control, Qt::ControlModifier, tr("Resize"), true);
  emit registerKey (Qt::Key_Control, Qt::NoModifier, tr("Resize"), true);

  //SCRIPTING SLOT DESCRIPTIONS
  setDescriptions();

  // CONTEXT MENU
  toAllTargets_ = new QAction(tr("Apply to all targets"), this);
  toAllTargets_->setCheckable(true);
  toAllTargets_->setToolTip(tr("Apply transformation to all target objects"));
  toAllTargets_->setStatusTip( toAllTargets_->toolTip() );

  contextAction_ = new QAction(tr("Set properties"), this);
  contextAction_->setToolTip(tr("Set properties"));
  contextAction_->setStatusTip( contextAction_->toolTip() );
  contextAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-properties.png") );
  
  contextActionHide_ = new QAction(tr("Hide Manipulator"), this);
  contextActionHide_->setToolTip(tr("Hide Manipulator"));
  contextActionHide_->setStatusTip( contextActionHide_->toolTip() );
  contextActionHide_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-hide.png") );

  emit addContextMenuItem(toAllTargets_      , CONTEXTNODEMENU );
  emit addContextMenuItem(contextAction_     , CONTEXTNODEMENU );
  emit addContextMenuItem(contextActionHide_ , CONTEXTNODEMENU );

  connect( toAllTargets_  ,     SIGNAL(toggled(bool) ), this, SLOT(setAllTargets(bool)));
  connect( contextAction_ ,     SIGNAL( triggered() ),  this, SLOT(showProps()) );
  connect( contextActionHide_ , SIGNAL( triggered() ),  this, SLOT(hideManipulator()) );

  //TOOLBAR
  toolbar_ = new QToolBar(tr("Transform and Move"));

  toolBarActions_ = new QActionGroup(toolbar_);

  moveAction_ = new QAction(tr("<B>Move Object</B><br>Move an object in 3D"), toolBarActions_);
  moveAction_->setStatusTip(tr("Move object in 3D."));
  moveAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-objects.png") );
  moveAction_->setCheckable(true);
  toolbar_->addAction(moveAction_);

  moveSelectionAction_ = new QAction(tr("<B>Move Selection</B><br>Move a selection on an object"), toolBarActions_);
  moveSelectionAction_->setStatusTip(tr("Move selections in 3D."));
  moveSelectionAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-selections.png") );
  moveSelectionAction_->setCheckable(true);
  toolbar_->addAction(moveSelectionAction_);

  
#ifdef ENABLE_SKELETON_SUPPORT
  moveSkeletonAction_ = new QAction(tr("<B>Move Skeleton</B><br>Move joints of a skeleton"), toolBarActions_);
  moveSkeletonAction_->setStatusTip(tr("Move joints of a skeleton."));
  moveSkeletonAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-skeleton.png") );
  moveSkeletonAction_->setCheckable(true);
  toolbar_->addAction(moveSkeletonAction_);
#endif
  
  connect(toolBarActions_, SIGNAL(triggered(QAction*)), this, SLOT(slotSetMoveMode(QAction*)) );

  emit addToolbar(toolbar_);

  pickToolbar_ = new QToolBar(tr("Transform and Move"));
  pickToolbar_->setAttribute(Qt::WA_AlwaysShowToolTips, true);
  pickToolBarActions_ = new QActionGroup(pickToolbar_);
  pickToolBarActions_->setExclusive (false);

  placeAction_ = new QAction(tr("Place manipulator"), pickToolBarActions_);
  placeAction_->setStatusTip(tr("Place manipulator on object. <Doubleclick>"));
  placeAction_->setToolTip(tr("Place manipulator on object. <Doubleclick>"));
  placeAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-place.png") );
  placeAction_->setCheckable(true);
  pickToolbar_->addAction(placeAction_);

  pickToolbar_->addSeparator ();

  rotateTranslateAction_ = new QAction(tr("Rotate/Translate object"), pickToolBarActions_);
  rotateTranslateAction_->setStatusTip(tr("Rotate/Translate object."));
  rotateTranslateAction_->setToolTip(tr("Rotate/Translate object."));
  rotateTranslateAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-translaterotate.png") );
  rotateTranslateAction_->setCheckable(true);
  rotateTranslateAction_->setChecked(true);
  pickToolbar_->addAction(rotateTranslateAction_);

  resizeAction_ = new QAction(tr("Resize object"), pickToolBarActions_);
  resizeAction_->setStatusTip(tr("Resize object. <Control>"));
  resizeAction_->setToolTip(tr("Resize object. <Control>"));
  resizeAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-resize.png") );
  resizeAction_->setCheckable(true);
  pickToolbar_->addAction(resizeAction_);

  pickToolbar_->addSeparator ();

  rotateManipAction_ = new QAction(tr("Rotate manipulator"), pickToolBarActions_);
  rotateManipAction_->setStatusTip(tr("Rotate manipulator. <Shift>"));
  rotateManipAction_->setToolTip(tr("Rotate manipulator. <Shift>"));
  rotateManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-maniprotate.png") );
  rotateManipAction_->setCheckable(true);
  pickToolbar_->addAction(rotateManipAction_);

  placeAndSnapAction_ = new QAction(tr("Locally translate manipulator"), pickToolBarActions_);
  placeAndSnapAction_->setStatusTip(tr("Locally translate manipulator. Press and hold <Alt> for snapping."));
  placeAndSnapAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-placeandsnap.png") );
  placeAndSnapAction_->setCheckable(true);
  pickToolbar_->addAction(placeAndSnapAction_);

  smallerManipAction_ = new QAction(tr("Decrease size of manipulator"), pickToolBarActions_);
  smallerManipAction_->setStatusTip(tr("Make manipulator smaller. <Mouse wheel up>"));
  smallerManipAction_->setToolTip(tr("Make manipulator smaller. <Mouse wheel up>"));
  smallerManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-manipsmall.png") );
  smallerManipAction_->setCheckable(false);
  pickToolbar_->addAction(smallerManipAction_);

  biggerManipAction_ = new QAction(tr("Increase size of manipulator"), pickToolBarActions_);
  biggerManipAction_->setStatusTip(tr("Make manipulator bigger. <Mouse wheel down>"));
  biggerManipAction_->setToolTip(tr("Make manipulator bigger. <Mouse wheel down>"));
  biggerManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-manipbig.png") );
  biggerManipAction_->setCheckable(false);
  pickToolbar_->addAction(biggerManipAction_);

#ifdef ENABLE_SKELETON_SUPPORT
  pickToolbar_->addSeparator();
  fixChildManipAction_ = new QAction(tr("Keep child joints fixed"), pickToolBarActions_);
  fixChildManipAction_->setStatusTip(tr("Do not apply transformations to all child joints"));
  fixChildManipAction_->setToolTip(tr("Do not apply transformations to all child joints"));
  fixChildManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-manipfix.png") );
  fixChildManipAction_->setCheckable(true);
  fixChildManipAction_->setChecked(recursiveJointTransformation_);
  pickToolbar_->addAction(fixChildManipAction_);

  transformRefPoseManipAction_ = new QAction(tr("Apply to Reference Pose"), pickToolBarActions_);
  transformRefPoseManipAction_->setStatusTip(tr("Apply the transformation to the reference pose"));
  transformRefPoseManipAction_->setToolTip(tr("Apply the transformation to the reference pose"));
  transformRefPoseManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-manipRefPose.png") );
  transformRefPoseManipAction_->setCheckable(true);
  transformRefPoseManipAction_->setChecked(transformRefPose_);
  pickToolbar_->addAction(transformRefPoseManipAction_);
  
  currentPoseManipAction_ = new QAction(tr("Transform only current pose"), pickToolBarActions_);
  currentPoseManipAction_->setStatusTip(tr("Apply the transformation only to the current pose"));
  currentPoseManipAction_->setToolTip(tr("Apply the transformation only to the current pose"));
  currentPoseManipAction_->setIcon(QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"move-manipCurrentPose.png") );
  currentPoseManipAction_->setCheckable(true);
  currentPoseManipAction_->setChecked(transformCurrentPose_);
  pickToolbar_->addAction(currentPoseManipAction_);
#endif
  
  connect(pickToolBarActions_, SIGNAL(triggered(QAction*)), this, SLOT(slotPickToolbarAction(QAction*)) );

  emit setPickModeToolbar ("Move", pickToolbar_);
  emit setPickModeToolbar ("MoveSelection", pickToolbar_);
  
#ifdef ENABLE_SKELETON_SUPPORT
  emit setPickModeToolbar ("MoveSkeleton", pickToolbar_);
#endif
  
}


/*******************************************************************************
        ToolBoxInterface implementation
 *******************************************************************************/

void MovePlugin::initializePlugin()
{
   toolboxActive_ = false;
   tool_ = new moveToolbarWidget();

   connect(tool_->moveToOrigin,SIGNAL(clicked() ),this,SLOT(slotMoveToOrigin()));

   tool_->moveToOrigin->setIcon( QIcon(OpenFlipper::Options::iconDirStr() + OpenFlipper::Options::dirSeparator() + "moveToCOG.png") );
   tool_->moveToOrigin->setIconSize(QSize(48,48));

   connect(tool_->unifyBoundingBoxDiagonal,SIGNAL(clicked() ),this,SLOT(slotUnifyBoundingBoxDiagonal()));
   tool_->unifyBoundingBoxDiagonal->setIcon( QIcon(OpenFlipper::Options::iconDirStr() + OpenFlipper::Options::dirSeparator() + "unifyBB.png") );
   tool_->unifyBoundingBoxDiagonal->setIconSize(QSize(48,48));

   lastActiveManipulator_ = -1;

   emit addToolbox( tr("Move") , tool_ );
}


/*******************************************************************************
        MouseInterface implementation
 *******************************************************************************/

/** \brief MouseWheel event occured
 *
 * @param _event the event that occured
 */
void MovePlugin::slotMouseWheelEvent(QWheelEvent * _event, const std::string & /*_mode*/)
{
  // Skip execution if this is not our pick mode
  if( ( (PluginFunctions::pickMode() != "Move")
     && (PluginFunctions::pickMode() != "MoveSelection")
     && (PluginFunctions::pickMode() != "MoveSkeleton") )
    || PluginFunctions::actionMode() != Viewer::PickingMode)
    return;
  
  // compute the manipulator size modifier based on the mouse wheel change
  manip_size_modifier_ = manip_size_modifier_ - (float)_event->delta() / 120.0 * 0.1;
  
  // Resize all manipulators based on the modifier on all objects
  for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)
      o_it->manipulatorNode()->set_size(manip_size_ * manip_size_modifier_);

  // Redraw scene with updated manipulators
  emit updateView();
}


//------------------------------------------------------------------------------

/** \brief MousePress event occured
 *
 * @param _event the event that occured
 */
void MovePlugin::slotMouseEvent(QMouseEvent* _event) {
    if (((PluginFunctions::pickMode() == ("Move"))
      || (PluginFunctions::pickMode() == ("MoveSelection"))
      || (PluginFunctions::pickMode() == ("MoveSkeleton")))
            && PluginFunctions::actionMode() == Viewer::PickingMode) {

        if (_event->type() == QEvent::MouseButtonDblClick || (_event->type() == QEvent::MouseButtonPress
                && _event->button() == Qt::LeftButton && (placeAction_->isChecked() || placeMode_))) {
          
            bool snap = (placeMode_ && (PluginFunctions::pickMode() == ("MoveSelection")))
                      || (PluginFunctions::pickMode() == ("MoveSkeleton"));

            placeManip(_event, snap);
            placeAction_->setChecked(false);
            updateManipulatorDialog();

            if (placeMode_) {
                manMode_ = QtTranslationManipulatorNode::TranslationRotation;

                for (PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS); o_it
                        != PluginFunctions::objectsEnd(); ++o_it)
                    if (o_it->manipPlaced())
                        o_it->manipulatorNode()->setMode(manMode_);

                resizeAction_->setChecked(false);
                rotateManipAction_->setChecked(false);
                rotateTranslateAction_->setChecked(true);
                placeAndSnapAction_->setChecked(false);
            }

            placeMode_ = false;
            return;

        } else if (placeMode_) {

            /*
             * Move manipulator along with cursor if placeAndSnap mode
             * is active. Snap to nearest geometry element (vertex, line, face center)
             * depending on which selection type is active.
             *
             * For skeletons always snap to nearest joint
             *
             */

            placeManip(_event, (PluginFunctions::pickMode() != ("Move")));
            updateManipulatorDialog();
            return;
        }

        // interaction
        ACG::SceneGraph::MouseEventAction action(_event,PluginFunctions::viewerProperties().glState());
        PluginFunctions::traverse(action);

        if (_event->buttons() == Qt::LeftButton)
          emit nodeVisibilityChanged(-1);

    }
}

/*******************************************************************************
        KeyInterface implementation
 *******************************************************************************/

void MovePlugin::slotKeyEvent (QKeyEvent* _event)
{
  if (_event->key() == Qt::Key_Control)
    setManipMode (QtTranslationManipulatorNode::Resize);
  else if (_event->key () == Qt::Key_Shift)
    setManipMode (QtTranslationManipulatorNode::LocalRotation);
}

//------------------------------------------------------------------------------

void MovePlugin::slotKeyReleaseEvent (QKeyEvent* _event)
{
  if ((_event->key() == Qt::Key_Control && manMode_ == QtTranslationManipulatorNode::Resize) ||
      (_event->key() == Qt::Key_Shift && manMode_ == QtTranslationManipulatorNode::LocalRotation))
    setManipMode (QtTranslationManipulatorNode::TranslationRotation);
}

/*******************************************************************************
        PickingInterface implementation
 *******************************************************************************/

/** \brief slot is called when the pickMode changed
 *
 * @param _mode new pickMode
 */
void MovePlugin::slotPickModeChanged( const std::string& _mode)
{
  moveAction_->setChecked(_mode == "Move");
  moveSelectionAction_->setChecked(_mode == "MoveSelection");

#ifdef ENABLE_SKELETON_SUPPORT
  moveSkeletonAction_->setChecked(_mode == "MoveSkeleton");
  fixChildManipAction_->setVisible(_mode == "MoveSkeleton");
  transformRefPoseManipAction_->setVisible(_mode == "MoveSkeleton");
  currentPoseManipAction_->setVisible(_mode == "MoveSkeleton");
#endif

  hide_ = !(_mode == "Move" || _mode == "MoveSelection" || _mode == "MoveSkeleton");

  showManipulators();

  if (!hide_)
  {
    switch (manMode_)
    {
      case QtTranslationManipulatorNode::Resize:
        PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
        break;
      case QtTranslationManipulatorNode::LocalRotation:
        PluginFunctions::setViewObjectMarker (&objectMarker_);
        break;
      case QtTranslationManipulatorNode::Place:
        PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
        break;
      case QtTranslationManipulatorNode::TranslationRotation:
        PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
        break;
    }
  }
  else
    PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
}


/*******************************************************************************
        MovePlugin implementation
 *******************************************************************************/

/** \brief Move object with given transformation matrix
 *
 * @param mat transformation matrix
 * @param _id id of the object
 */
void MovePlugin::moveObject(ACG::Matrix4x4d mat, int _id) {
  BaseObjectData* object;
  if ( ! PluginFunctions::getObject(_id,object) )
    return;

  if  ( object->dataType()  == DATA_TRIANGLE_MESH ) {
    transformMesh(mat , *PluginFunctions::triMesh(object) );
  } else  if  ( object->dataType()  == DATA_POLY_MESH ) {
    transformMesh(mat , *PluginFunctions::polyMesh(object) );
#ifdef ENABLE_TSPLINEMESH_SUPPORT
  } else  if  ( object->dataType()  == DATA_TSPLINE_MESH ) {
    transformMesh(mat , *PluginFunctions::tsplineMesh(object) );
#endif
#ifdef ENABLE_POLYLINE_SUPPORT
  } else  if  ( object->dataType()  == DATA_POLY_LINE ) {
    transformPolyLine(mat , *PluginFunctions::polyLine(object) );
#endif
#ifdef ENABLE_SKELETON_SUPPORT
  } else  if  ( object->dataType()  == DATA_SKELETON ) {
    transformSkeleton(mat , PluginFunctions::skeletonObject(object) );
#endif
  } else  if  ( object->dataType()  == DATA_PLANE ) {
    PluginFunctions::planeNode(object)->transform(mat);
  } else {

    emit log(LOGERR,tr("moveObject called for unsupported Object Type"));
    return;
  }

  emit updatedObject(_id, UPDATE_GEOMETRY);
  emit createBackup(_id,"Move");
}


//------------------------------------------------------------------------------

/** \brief Move selection on an object with given transformation matrix
 *
 * Which Selection (Vertex, Edge, Face) is used is determined by the
 * current SelectionMode in SelectionPlugin.
 * If the Plugin is unable to communicate with SelectionPlugin, Vertex Selection is used.
 *
 * @param mat
 * @param _id
 */
void MovePlugin::moveSelection(ACG::Matrix4x4d mat, int _id) {

  if (selectionType_ == VERTEX)
    transformVertexSelection( _id , mat );
  else if (selectionType_ == FACE)
    transformFaceSelection( _id , mat );
  else if (selectionType_ == EDGE)
    transformEdgeSelection( _id , mat );

  emit updatedObject(_id, UPDATE_GEOMETRY);
//   emit createBackup(_id,"MoveSelection");
}

//------------------------------------------------------------------------------

#ifdef ENABLE_SKELETON_SUPPORT
/** \brief Move joint of a skeleton with given transformation matrix
 *
 * @param mat the transformation matrix
 * @param _id id of the skeleton
 */
void MovePlugin::moveSkeletonJoint(ACG::Matrix4x4d mat, int _id) {

  transformSkeletonJoint( _id , mat );
  
//   emit createBackup(_id,"MoveSkeleton");
}
#endif

//------------------------------------------------------------------------------

/** \brief Set the manipulator manipulation mode
 *
 * @param _mode Mode
 */

void MovePlugin::setManipMode (QtTranslationManipulatorNode::ManipulatorMode _mode)
{
  if (_mode != manMode_)
  {
    manMode_ = _mode;
    if ((PluginFunctions::pickMode() == "Move" ) || (PluginFunctions::pickMode() == "MoveSelection" )
                                                 || (PluginFunctions::pickMode() == "MoveSkeleton" )) {
        for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ;
              o_it != PluginFunctions::objectsEnd(); ++o_it)
           if ( o_it->manipPlaced() )
                o_it->manipulatorNode()->setMode (_mode);
      if (!hide_)
        switch (manMode_)
        {
          case QtTranslationManipulatorNode::Resize:
            PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
            placeMode_ = false;
            break;
          case QtTranslationManipulatorNode::LocalRotation:
            PluginFunctions::setViewObjectMarker (&objectMarker_);
            placeMode_ = false;
            break;
          case QtTranslationManipulatorNode::Place:
            PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
            placeMode_ = true;
            break;
          case QtTranslationManipulatorNode::TranslationRotation:
            PluginFunctions::setViewObjectMarker (PluginFunctions::defaultViewObjectMarker ());
            placeMode_ = false;
            break;
        }
    }
    switch (manMode_)
    {
      case QtTranslationManipulatorNode::Resize:
        resizeAction_->setChecked (true);
        rotateManipAction_->setChecked (false);
        rotateTranslateAction_->setChecked (false);
        placeAndSnapAction_->setChecked (false);
        break;
      case QtTranslationManipulatorNode::LocalRotation:
        resizeAction_->setChecked (false);
        rotateManipAction_->setChecked (true);
        rotateTranslateAction_->setChecked (false);
        placeAndSnapAction_->setChecked (false);
        break;
      case QtTranslationManipulatorNode::TranslationRotation:
        resizeAction_->setChecked (false);
        rotateManipAction_->setChecked (false);
        rotateTranslateAction_->setChecked (true);
        placeAndSnapAction_->setChecked (false);
        break;
      case QtTranslationManipulatorNode::Place:
        resizeAction_->setChecked (false);
        rotateManipAction_->setChecked (false);
        rotateTranslateAction_->setChecked (false);
        placeAndSnapAction_->setChecked (true);
        break;
    }
  }
}

//------------------------------------------------------------------------------

/** \brief Hide context menu entry when right clicking on node other than manipulator node
 *
 * @param _nodeId Identifier of node that has been clicked
 */
void MovePlugin::slotUpdateContextMenuNode(int _nodeId) {

    ACG::SceneGraph::BaseNode* node = ACG::SceneGraph::find_node(PluginFunctions::getSceneGraphRootNode(), _nodeId);

    if (node == 0)
        return;

    if (node->className() != "QtTranslationManipulatorNode") {
        contextAction_->setVisible(false);
    } else {
        contextAction_->setVisible(true);
    }
}

//------------------------------------------------------------------------------

/** \brief move the object when its manipulator moves
 *
 * @param _node the manipulator node
 * @param _event the mouse event
 */
void MovePlugin::manipulatorMoved( QtTranslationManipulatorNode* _node , QMouseEvent* _event) {

  // React on event only in move mode
  if ( PluginFunctions::pickMode() != "Move"
    && PluginFunctions::pickMode() != "MoveSelection"
    && PluginFunctions::pickMode() != "MoveSkeleton" )
    return;

  OpenFlipper::Options::redrawDisabled( true );

  // Apply changes only on Release for moveMode and after every movement in MoveSelection/MoveSkeleton Mode
  if ( ((_event->type() == QEvent::MouseButtonRelease) || (PluginFunctions::pickMode() != "Move")) && !placeMode_) {

    int objectId = _node->getIdentifier();

    ACG::Matrix4x4d mat;
    mat.identity();
    mat = _node->matrix();

    // Reset Node
    _node->loadIdentity();
    _node->set_center(mat.transform_point(_node->center()));

    // move the object which corresponds to the manipulator
    if (PluginFunctions::pickMode() == "Move")
      moveObject( mat, objectId );
    else if (PluginFunctions::pickMode() == "MoveSelection")
      moveSelection( mat, objectId );
    
    #ifdef ENABLE_SKELETON_SUPPORT
    else if (PluginFunctions::pickMode() == "MoveSkeleton"){
      moveSkeletonJoint( mat, objectId );
    
      if (_event->type() == QEvent::MouseButtonRelease)
        emit updatedObject(objectId, UPDATE_GEOMETRY);
    }
    #endif

    // move all other targets without manipulator
    if(allTargets_) {
      for (PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS); o_it
          != PluginFunctions::objectsEnd(); ++o_it) {
        if ((o_it->id() != objectId) && !o_it->manipulatorNode()->visible()) { // If it has its own manipulator active, dont move it
          
          // move the object which corresponds to the manipulator
          if (PluginFunctions::pickMode() == "Move")
            moveObject( mat, o_it->id() );
          else if (PluginFunctions::pickMode() == "MoveSelection")
            moveSelection( mat, o_it->id() );
          
          #ifdef ENABLE_SKELETON_SUPPORT
          else if (PluginFunctions::pickMode() == "MoveSkeleton"){
            moveSkeletonJoint( mat, o_it->id() );

            if (_event->type() == QEvent::MouseButtonRelease)
              emit updatedObject(objectId, UPDATE_GEOMETRY);
          }
          #endif
        }
      }
    }

    lastActiveManipulator_ = objectId;
    updateManipulatorDialog();
  }

  OpenFlipper::Options::redrawDisabled( false );

}


//------------------------------------------------------------------------------

/** \brief update object when its manipulator changes position
 *
 * @param _node the manipulator node
 */
void MovePlugin::ManipulatorPositionChanged(QtTranslationManipulatorNode* _node ) {

  // Position has been changed of the manipulator by a direct function
  int objectId = _node->getIdentifier();

  if ( objectId > 0 ){

    BaseObjectData* object;
    PluginFunctions::getObject(objectId,object);

    // Assume that it has a good position now
    object->manipPlaced( true );
  }

  // Show manipulator only if in Move mode
  if ( PluginFunctions::pickMode() == "Move" )
    _node->show();

  lastActiveManipulator_ = objectId;
  updateManipulatorDialog();

}


//------------------------------------------------------------------------------

/** \brief Place and show the Manipulator
 *
 * @param _event  mouseEvent that occured
 */
void MovePlugin::placeManip(QMouseEvent * _event, bool _snap) {
    unsigned int node_idx, target_idx;
    OpenMesh::Vec3d hitPoint;
    BaseObjectData* object;

    bool successfullyPicked = false;

    int data = -1;
    
    /*
     * Snap manipulator to nearest geometry
     * element depending on which selection type is
     * active.
     */
    if (_snap) {
      
        #ifdef ENABLE_SKELETON_SUPPORT
        
        if ( PluginFunctions::pickMode() == "MoveSkeleton" ){
        
          //disable picking for anything but skeletons
          for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)
            o_it->enablePicking( o_it->dataType(DATA_SKELETON) );
          
          //perform picking
          successfullyPicked = PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_ANYTHING, _event->pos(), node_idx,
                  target_idx, &hitPoint) && PluginFunctions::getPickedObject(node_idx, object);
          
          //reenable picking for anything
          for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)
            o_it->enablePicking( true );

          if ( successfullyPicked && object->dataType(DATA_SKELETON) ) {

            hitPoint = getNearestJoint(PluginFunctions::skeletonObject(object), hitPoint, data);
            
            PluginFunctions::setDrawMode(ACG::SceneGraph::DrawModes::WIREFRAME);
            
          } else {
            successfullyPicked = false;
          }
          
        } else
        #endif
      
        if (!successfullyPicked){
 
          successfullyPicked = PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_FACE, _event->pos(), node_idx,
                               target_idx, &hitPoint) && PluginFunctions::getPickedObject(node_idx, object);

          if(!successfullyPicked) {
              //emit log(LOGWARN, tr("Picking failed"));
              return;
          }

          if (selectionType_ == VERTEX) {
              if ( object->dataType(DATA_TRIANGLE_MESH) ) {
                  hitPoint = getNearestVertex(PluginFunctions::triMesh(object), target_idx, hitPoint);
              } else if ( object->dataType(DATA_POLY_MESH) ) {
                  hitPoint = getNearestVertex(PluginFunctions::polyMesh(object), target_idx, hitPoint);
#ifdef ENABLE_TSPLINEMESH_SUPPORT
              } else if ( object->dataType(DATA_TSPLINE_MESH) ) {
                  hitPoint = getNearestVertex(PluginFunctions::tsplineMesh(object), target_idx, hitPoint);
#endif
              }
          } else if (selectionType_ == EDGE) {
              if ( object->dataType(DATA_TRIANGLE_MESH) ) {
                  hitPoint = getNearestEdge(PluginFunctions::triMesh(object), target_idx, hitPoint);
              } else if ( object->dataType(DATA_POLY_MESH) ) {
                  hitPoint = getNearestEdge(PluginFunctions::polyMesh(object), target_idx, hitPoint);
#ifdef ENABLE_TSPLINEMESH_SUPPORT
              } else if ( object->dataType(DATA_TSPLINE_MESH) ) {
                  hitPoint = getNearestEdge(PluginFunctions::tsplineMesh(object), target_idx, hitPoint);
#endif
              }
          } else if (selectionType_ == FACE) {
              if ( object->dataType(DATA_TRIANGLE_MESH) ) {
                  hitPoint = getNearestFace(PluginFunctions::triMesh(object), target_idx, hitPoint);
              } else if ( object->dataType(DATA_POLY_MESH) ) {
                  hitPoint = getNearestFace(PluginFunctions::polyMesh(object), target_idx, hitPoint);
#ifdef ENABLE_TSPLINEMESH_SUPPORT
              } else if ( object->dataType(DATA_TSPLINE_MESH) ) {
                  hitPoint = getNearestFace(PluginFunctions::tsplineMesh(object), target_idx, hitPoint);
#endif
              }
          }
        }

    } else {
        successfullyPicked = PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_ANYTHING, _event->pos(), node_idx,
                target_idx, &hitPoint) && PluginFunctions::getPickedObject(node_idx, object);
    }

    if (successfullyPicked) {

        object->manipPlaced(true);

        if (data != -1)
          object->manipulatorNode()->setData( data );

        object->manipulatorNode()->loadIdentity();
        object->manipulatorNode()->set_center(hitPoint);
        object->manipulatorNode()->set_draw_cylinder(true);
        object->manipulatorNode()->set_autosize(QtTranslationManipulatorNode::Once);
        object->manipulatorNode()->set_size(manip_size_ * manip_size_modifier_);
        object->manipulatorNode()->setMode(manMode_);
        object->manipulatorNode()->show();

        object->manipulatorNode()->apply_transformation(PluginFunctions::pickMode() == "Move");

        // Disconnect a previously connected Signal
        disconnect(object->manipulatorNode() , SIGNAL(manipulatorMoved(QtTranslationManipulatorNode*,QMouseEvent*)),
                   this , SLOT( manipulatorMoved(QtTranslationManipulatorNode*,QMouseEvent*)));
                
        disconnect(object->manipulatorNode() , SIGNAL(positionChanged(QtTranslationManipulatorNode*)),
                  this , SLOT( ManipulatorPositionChanged(QtTranslationManipulatorNode*)));
        
        // Reconnect the signals.
        connect(object->manipulatorNode() , SIGNAL(manipulatorMoved(QtTranslationManipulatorNode*,QMouseEvent*)),
                this , SLOT( manipulatorMoved(QtTranslationManipulatorNode*,QMouseEvent*)));

        connect(object->manipulatorNode() , SIGNAL(positionChanged(QtTranslationManipulatorNode*)),
                this , SLOT( ManipulatorPositionChanged(QtTranslationManipulatorNode*)));

        lastActiveManipulator_ = object->id();

        emit updateView();
       
        bool found = false;
        
        for (uint i=0; i < activeManipulators_.size(); i++)
          if ( activeManipulators_[i] == object->id() ){
            found = true;
            break;
          }
        
        if ( !found )
          activeManipulators_.push_back( object->id() );

    } else {
        //emit log(LOGWARN, tr("Picking failed"));
    }
}


//------------------------------------------------------------------------------

/** \brief Checks if the manipulators should be visible or not
 *
 */
void MovePlugin::showManipulators( )
{

  if (!hide_ && (toolboxActive_ || (PluginFunctions::pickMode() == "Move")
                                || (PluginFunctions::pickMode() == "MoveSelection")
                                || (PluginFunctions::pickMode() == "MoveSkeleton"))) {
    
    for (uint i=0; i < activeManipulators_.size(); i++){
      
      BaseObjectData* obj = 0;
      
      PluginFunctions::getObject( activeManipulators_[i], obj );
      
      if (obj != 0 && obj->manipPlaced()) {
        
        #ifdef ENABLE_SKELETON_SUPPORT
        if ( obj->dataType(DATA_SKELETON) )
          moveManipulatorOnSkeleton(obj);
        #endif
        
        obj->manipulatorNode()->show();
        obj->manipulatorNode()->apply_transformation( PluginFunctions::pickMode() == "Move" );
        emit nodeVisibilityChanged(obj->id());
      }
    }

  } else {
    
    for (uint i=0; i < activeManipulators_.size(); i++){
      
      BaseObjectData* obj = 0;
      
      PluginFunctions::getObject( activeManipulators_[i], obj );
      
      if ( obj != 0 ) {
        obj->manipulatorNode()->hide();
        emit nodeVisibilityChanged(obj->id());
      }
    }
  }

  emit updateView();

}

//------------------------------------------------------------------------------

/** \brief Get Dialog Widget that contains the button
 *
 * @param _but Reference to QPushButton object that has been pressed
 */
movePropsWidget* MovePlugin::getDialogFromButton(QPushButton* _but) {

    if(_but == 0) return 0;
    return dynamic_cast<movePropsWidget*>((((_but->parentWidget())->parentWidget())->parentWidget()));
}
//------------------------------------------------------------------------------

/** \brief Position of manipulator in tab changed
 *
 */
void MovePlugin::slotSetPosition() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    TriMesh::Point newpos;

    bool ok = false;
    newpos[0] =  (pW->nposx->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for X Coordinate")); return; }
    newpos[1] =  (pW->nposy->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Y Coordinate")); return; }
    newpos[2] =  (pW->nposz->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Z Coordinate")); return; }

    BaseObjectData* object;
    if ( PluginFunctions::getObject(lastActiveManipulator_ , object) ) {
	if (  object->manipulatorNode()->visible() )
	    object->manipulatorNode()->set_center( newpos );
	updateManipulatorDialog();
	emit updateView();
    }
}


//------------------------------------------------------------------------------

/** \brief Toggle the first axis for changing direction in tab
 *
 */
void MovePlugin::slotToggleAxisA() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    axisA_ = (axisA_ + 1) % 3;

    if (axisA_ == axisB_)
	axisA_ = (axisA_ + 1) % 3;

    switch(axisA_){
	case 0: pW->axisAButton->setText(tr("X Direction")); break;
	case 1: pW->axisAButton->setText(tr("Y Direction")); break;
	case 2: pW->axisAButton->setText(tr("Z Direction")); break;
	default: break;
    }
}


//------------------------------------------------------------------------------

/** \brief Toggle the second axis for changing direction in tab
 *
 */
void MovePlugin::slotToggleAxisB() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    axisB_ = (axisB_ + 1) % 3;

    if (axisA_ == axisB_)
	axisB_ = (axisB_ + 1) % 3;

    switch(axisB_){
	case 0: pW->axisBButton->setText(tr("X Direction")); break;
	case 1: pW->axisBButton->setText(tr("Y Direction")); break;
	case 2: pW->axisBButton->setText(tr("Z Direction")); break;
	default: break;
    }
}


//------------------------------------------------------------------------------

/** \brief Set Direction of manipulator in tab changed
 *
 */
void MovePlugin::slotSetDirection() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    ACG::Vec3d newdirA,newdirB;
    ACG::Vec3d dirX,dirY;
    ACG::Vec3d dirZ(0.0,0.0,0.0);

    bool ok = false;
    newdirA[0] =  (pW->ndirAx->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for X Coordinate"));  return; }
    newdirA[1] =  (pW->ndirAy->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Y Coordinate")); return; }
    newdirA[2] =  (pW->ndirAz->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Z Coordinate")); return; }

    newdirB[0] =  (pW->ndirBx->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for X Coordinate"));  return; }
    newdirB[1] =  (pW->ndirBy->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Y Coordinate")); return; }
    newdirB[2] =  (pW->ndirBz->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Z Coordinate")); return; }

    bool xAxis = false;
    bool yAxis = false;

    switch(axisA_){
	case  0: dirX = newdirA; xAxis = true; break;
	case  1: dirY = newdirA; yAxis = true; break;
	default: dirZ = newdirA; break;
    }

    switch(axisB_){
	case  0: dirX = newdirB; xAxis = true; break;
	case  1: dirY = newdirB; yAxis = true; break;
	default: dirZ = newdirB; break;
    }

    if (!xAxis)
	dirX = dirY % dirZ;

    if (!yAxis)
	dirY = dirX % dirZ;


    if ( (dirX | dirY) != 0.0){
	emit log(LOGERR,tr("The axes of the new direction have to be orthogonal"));
	return;
    }

//    // Apply to All Target Objects
//     if ( pW->targetObjects->isChecked() ) {
// 	for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ;
//              o_it PluginFunctions::objectsEnd(); ++o_it){
//
// 	    o_it->manipulatorNode()->set_direction( dirX, dirY );
// 	}
//     }

    
    BaseObjectData* object = 0;
    PluginFunctions::getObject(pW->getBaseObjectDataId(),object);
    if ( object != 0 ) {
        if (  object->manipulatorNode()->visible() ){

            object->manipulatorNode()->set_direction( dirX, dirY );
	}
    } else return;

    updateManipulatorDialog();
    emit updateView();
}


//------------------------------------------------------------------------------

/** \brief perform a translation for Manipulator in tab
 *
 */
void MovePlugin::slotTranslation() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    ACG::Vec3d translation;

    bool ok = false;
    translation[0] =  (pW->translationX->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for X Coordinate"));  return; }
    translation[1] =  (pW->translationY->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Y Coordinate")); return; }
    translation[2] =  (pW->translationZ->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Z Coordinate")); return; }

//	// Apply to All Target Objects
//	if ( pW->targetObjects->isChecked() ) {
//
//		int manipcount = 0; // Check how many of the target meshes have an visible manipulator
//		int targets = 0; // Count the number of target meshes
//		for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS); o_it != PluginFunctions::objectsEnd(); ++o_it) {
//			++targets;
//			if ( ! o_it->manipulatorNode()->hidden() ) {
//				++ manipcount;
//			}
//		}
//
//		if (manipcount == 0 ) // No manipulator -> no translation
//		return;
//
//		for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS,DataType( DATA_TRIANGLE_MESH | DATA_POLY_MESH ));
//				o_it != PluginFunctions::objectsEnd(); ++o_it) {
//			if ( manipcount > 1 ) { // Use manipulator direction for each target mesh
//				if ( o_it->manipulatorNode()->hidden() )
//				continue;
//			}
//
//			translate( o_it->id() , translation );
//
//			o_it->manipulatorNode()->set_center( o_it->manipulatorNode()->center() + translation );
//			emit createBackup(o_it->id(),"Translation");
//			emit updatedObject(o_it->id());
//		}
//
//	}

        BaseObjectData* object = 0;
        PluginFunctions::getObject(pW->getBaseObjectDataId(),object);

	if (object != 0) {
		if (object->manipulatorNode()->visible()) {

			translate(object->id(), translation);

			object->manipulatorNode()->set_center(
					object->manipulatorNode()->center() + translation);
			emit createBackup(object->id(), "Translation");
			emit updatedObject(object->id(), UPDATE_GEOMETRY);
		}
	} else {
		return;
	}


    updateManipulatorDialog();
    emit scriptInfo(QString("slotTranslation()"));
    emit updateView();
}


//------------------------------------------------------------------------------

/** \brief Project the current manipulator onto the tangent plane of the object
 *
 */
void MovePlugin::slotProjectToTangentPlane() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    if ( allTargets_ ) {
    	emit log(LOGWARN,tr("TODO Project for multiple targets"));
    	return;
    } else {
    	emit log(LOGWARN,tr("TODO Project for one target"));
    	return;
    }

}


//------------------------------------------------------------------------------

/** \brief Move the current manipulator to the cog of the object
 *
 */
void MovePlugin::slotMoveManipToCOG() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;


//     if ( pW->targetObjects->isChecked() ) {
// 	for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
// 		if (  o_it->manipulatorNode()->hidden() )
// 		continue;
//
// 		if ( o_it->dataType( DATA_TRIANGLE_MESH ) )
// 		o_it->manipulatorNode()->set_center( MeshInfo::cog(*PluginFunctions::triMesh(*o_it) ) );
// 		else if ( o_it->dataType( DATA_POLY_MESH ) )
// 		o_it->manipulatorNode()->set_center( MeshInfo::cog(*PluginFunctions::polyMesh(*o_it)) );
//
// 		updateManipulatorDialog();
// 		o_it->manipulatorNode()->loadIdentity();
// 	}
//     } else {

    BaseObjectData* object = 0;
    PluginFunctions::getObject(pW->getBaseObjectDataId(),object);
    if ( object != 0 ) {
	if (  object->manipulatorNode()->visible() ){

	    if ( object->dataType( DATA_TRIANGLE_MESH ) )
	    object->manipulatorNode()->set_center( MeshInfo::cog(*PluginFunctions::triMesh(object)) );
	    else if ( object->dataType( DATA_POLY_MESH ) )
	    object->manipulatorNode()->set_center( MeshInfo::cog(*PluginFunctions::polyMesh(object)) );
#ifdef ENABLE_TSPLINEMESH_SUPPORT
	    else if ( object->dataType( DATA_TSPLINE_MESH ) )
	    object->manipulatorNode()->set_center( MeshInfo::cog(*PluginFunctions::tsplineMesh(object)) );
#endif

	    updateManipulatorDialog();
	    object->manipulatorNode()->loadIdentity();
	}
    }

    emit updateView();
}


//------------------------------------------------------------------------------

/** \brief Rotate Manipulator (with values from Tab)
 *
 */
void MovePlugin::slotRotate() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;

    TriMesh::Point axis;
    double angle;

    bool ok = false;
    axis[0] =  (pW->rotx->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for X Coordinate"));  return; }
    axis[1] =  (pW->roty->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Y Coordinate")); return; }
    axis[2] =  (pW->rotz->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for Z Coordinate")); return; }

    angle =  (pW->rotAngle->text()).toDouble(&ok);
    if ( !ok ) {  emit log(LOGERR,tr("Wrong Format for Angle")); return; }

//     if ( pW->targetObjects->isChecked() ) {
// 	for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
// 		if (  o_it->manipulatorNode()->hidden() )
// 		continue;
// 		o_it->manipulatorNode()->rotate(  angle,axis);
//
// 		if (o_it->dataType( DATA_TRIANGLE_MESH ) )
// 		transformMesh(  o_it->manipulatorNode()->matrix() , (*PluginFunctions::triMesh(*o_it)) );
//
// 		if (o_it->dataType( DATA_POLY_MESH ) )
// 		transformMesh(  o_it->manipulatorNode()->matrix() , (*PluginFunctions::polyMesh(*o_it)) );
//
// 		o_it->manipulatorNode()->loadIdentity();
// 		updateManipulatorDialog();
//
// 		emit createBackup(o_it->id(),"Rotation");
// 		emit updatedObject(o_it->id());
// 	}
//     } else {


        BaseObjectData* object = 0;
        PluginFunctions::getObject(pW->getBaseObjectDataId(),object);
	if (object != 0) {
		if (object->manipulatorNode()->visible() && (object->target()
				|| !allTargets_)) {

			object->manipulatorNode()->rotate(angle, axis);

			if (object->dataType(DATA_TRIANGLE_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::triMesh(object)));

			if (object->dataType(DATA_POLY_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::polyMesh(object)));
#ifdef ENABLE_TSPLINEMESH_SUPPORT
			if (object->dataType(DATA_TSPLINE_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::tsplineMesh(object)));
#endif

			updateManipulatorDialog();

			emit createBackup(object->id(), "Rotation");
			emit updatedObject(object->id(), UPDATE_GEOMETRY);
		}
	}

	emit scriptInfo(QString("slotRotate()"));
	emit updateView();
}


//------------------------------------------------------------------------------

/** \brief Scale Manipulator (with values from Tab)
 *
 */
void MovePlugin::slotScale() {

    QPushButton* but = dynamic_cast<QPushButton*>(QObject::sender());
    movePropsWidget* pW = getDialogFromButton(but);
    if(pW == 0) return;


    TriMesh::Point scale;

    bool ok = false;
    scale[0] =  (pW->scalex->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for factor 1"));  return; }
    scale[1] =  (pW->scaley->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for factor 2")); return; }
    scale[2] =  (pW->scalez->text()).toDouble(&ok);
    if ( !ok ) { emit log(LOGERR,tr("Wrong Format for factor 3")); return; }

//    if ( pW->targetObjects->isChecked() ) {
// 	for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
// 		if (  o_it->manipulatorNode()->hidden() )
// 		continue;
// 		o_it->manipulatorNode()->scale(  scale);
//
// 		if (o_it->dataType( DATA_TRIANGLE_MESH ) )
// 		transformMesh(  o_it->manipulatorNode()->matrix() , (*PluginFunctions::triMesh(*o_it)) );
//
// 		if (o_it->dataType( DATA_POLY_MESH ) )
// 		transformMesh(  o_it->manipulatorNode()->matrix() , (*PluginFunctions::polyMesh(*o_it)) );
//
// 		o_it->manipulatorNode()->loadIdentity();
// 		updateManipulatorDialog();
//
// 		emit createBackup(o_it->id(),"Scaling");
// 		emit updatedObject(o_it->id());
// 	}
//     } else {


        BaseObjectData* object = 0;
        PluginFunctions::getObject(pW->getBaseObjectDataId(),object);
	if (object != 0) {
		if (object->manipulatorNode()->visible() && (object->target()
				|| !allTargets_)) {

			object->manipulatorNode()->scale(scale);

			if (object->dataType(DATA_TRIANGLE_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::triMesh(object)));

			if (object->dataType(DATA_POLY_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::polyMesh(object)));
#ifdef ENABLE_TSPLINEMESH_SUPPORT
			if (object->dataType(DATA_TSPLINE_MESH))
				transformMesh(getLastManipulatorMatrix(true),
						(*PluginFunctions::tsplineMesh(object)));
#endif

			updateManipulatorDialog();

			emit createBackup(object->id(), "Scaling");
			emit updatedObject(object->id(), UPDATE_GEOMETRY);
		}
	}

    emit scriptInfo(QString("slotScale()"));
    emit updateView();
}


//------------------------------------------------------------------------------


/** \brief Move target Meshes cog to the origin
 *
 */
void MovePlugin::slotMoveToOrigin() {

  bool useCommonCOG = false;
  ACG::Vec3d cog = ACG::Vec3d(0.0,0.0,0.0);

  if ( PluginFunctions::targetCount() > 1 ) {
    QMessageBox::StandardButton button = QMessageBox::question( 0, tr("Use common COG?"), tr("Should the targets be moved depending on their common cog?"),QMessageBox::Yes|QMessageBox::No);


    useCommonCOG =  ( button == QMessageBox::Yes );
    double vertexCount = 0.0;

    if ( useCommonCOG ) {

      // Compute cog for all objects
      for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
        if ( o_it->dataType( DATA_TRIANGLE_MESH )) {
          TriMesh& mesh = *PluginFunctions::triMesh(*o_it);
          cog += MeshInfo::cog(mesh) * double(mesh.n_vertices());
          vertexCount += double(mesh.n_vertices());
        }

        if ( o_it->dataType( DATA_POLY_MESH )) {
          PolyMesh& mesh = *PluginFunctions::polyMesh(*o_it);
          cog += MeshInfo::cog(mesh) * double(mesh.n_vertices());
          vertexCount += double(mesh.n_vertices());
        }

#ifdef ENABLE_TSPLINEMESH_SUPPORT
        if ( o_it->dataType( DATA_TSPLINE_MESH )) {
          TSplineMesh& mesh = *PluginFunctions::tsplineMesh(*o_it);
          cog += MeshInfo::cog(mesh) * double(mesh.n_vertices());
          vertexCount += double(mesh.n_vertices());
        }
#endif
      }

      cog = cog / vertexCount;
    }

  }

  for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
    if ( o_it->dataType( DATA_TRIANGLE_MESH )) {
        TriMesh& mesh = *PluginFunctions::triMesh(*o_it);

        if ( !useCommonCOG )
          cog = MeshInfo::cog(mesh);

        for ( TriMesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end() ; ++v_it)
          mesh.set_point(v_it , ( mesh.point(v_it) ) - cog );

        o_it->manipulatorNode()->set_center( o_it->manipulatorNode()->center() - cog );
    }

    if ( o_it->dataType( DATA_POLY_MESH )) {
        PolyMesh& mesh = *PluginFunctions::polyMesh(*o_it);

        if ( !useCommonCOG )
          cog = MeshInfo::cog(mesh);

        for ( PolyMesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end() ; ++v_it)
          mesh.set_point(v_it , ( mesh.point(v_it) ) - cog );

        o_it->manipulatorNode()->set_center( o_it->manipulatorNode()->center() - cog );

    }

#ifdef ENABLE_TSPLINEMESH_SUPPORT
    if ( o_it->dataType( DATA_TSPLINE_MESH )) {
        TSplineMesh& mesh = *PluginFunctions::tsplineMesh(*o_it);

        if ( !useCommonCOG )
          cog = MeshInfo::cog(mesh);

        for ( TSplineMesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end() ; ++v_it)
          mesh.set_point(v_it , ( mesh.point(v_it) ) - cog );

        o_it->manipulatorNode()->set_center( o_it->manipulatorNode()->center() - cog );

    }
#endif
    emit updatedObject( o_it->id(), UPDATE_GEOMETRY );

    updateManipulatorDialog();
    o_it->manipulatorNode()->loadIdentity();

    emit createBackup(o_it->id(),"Move to origin");
  }

  emit updateView();
}


//------------------------------------------------------------------------------

/** \brief Scale Boundingbox Diagonal to unit size
 *
 */
void MovePlugin::slotUnifyBoundingBoxDiagonal()
{
  bool useCommonBB = false;
  ACG::Vec3d bb_min = ACG::Vec3d(FLT_MAX,FLT_MAX,FLT_MAX);
  ACG::Vec3d bb_max = ACG::Vec3d(FLT_MIN,FLT_MIN,FLT_MIN);

  if ( PluginFunctions::targetCount() > 1 ) {
    QMessageBox::StandardButton button = QMessageBox::question( 0, tr("Use common BB?"), tr("Should the targets be scaled depending on their common Bounding Box?"),QMessageBox::Yes|QMessageBox::No);


    useCommonBB =  ( button == QMessageBox::Yes );
    double count = 0.0;

    if ( useCommonBB ) {

      // Compute cog for all objects
      for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
        ACG::Vec3d bb_min_tmp(0.0,0.0,0.0);
        ACG::Vec3d bb_max_tmp(0.0,0.0,0.0);

        if ( o_it->dataType( DATA_TRIANGLE_MESH )) {
          TriMesh& mesh = *PluginFunctions::triMesh(*o_it);
          getBB(mesh,bb_min_tmp,bb_max_tmp);
          bb_min.minimize(bb_min_tmp);
          bb_max.maximize(bb_max_tmp);
        }

        if ( o_it->dataType( DATA_POLY_MESH )) {
          PolyMesh& mesh = *PluginFunctions::polyMesh(*o_it);
          getBB(mesh,bb_min_tmp,bb_max_tmp);
          bb_min.minimize(bb_min_tmp);
          bb_max.maximize(bb_max_tmp);
        }
#ifdef ENABLE_TSPLINEMESH_SUPPORT
        if ( o_it->dataType( DATA_TSPLINE_MESH )) {
          TSplineMesh& mesh = *PluginFunctions::tsplineMesh(*o_it);
          getBB(mesh,bb_min_tmp,bb_max_tmp);
          bb_min.minimize(bb_min_tmp);
          bb_max.maximize(bb_max_tmp);
        }
#endif
      }
    }

  }

  for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)  {
    if ( useCommonBB ) {
      if ( o_it->dataType( DATA_TRIANGLE_MESH ) )
        unifyBBDiag(*PluginFunctions::triMesh(*o_it),bb_min,bb_max);
      else if ( o_it->dataType( DATA_POLY_MESH ) )
        unifyBBDiag(*PluginFunctions::polyMesh(*o_it),bb_min,bb_max);
#ifdef ENABLE_TSPLINEMESH_SUPPORT
      else if ( o_it->dataType( DATA_TSPLINE_MESH ) )
        unifyBBDiag(*PluginFunctions::tsplineMesh(*o_it),bb_min,bb_max);
#endif
    } else {
      if ( o_it->dataType( DATA_TRIANGLE_MESH ) )
        unifyBBDiag(*PluginFunctions::triMesh(*o_it));
      else if ( o_it->dataType( DATA_POLY_MESH ) )
        unifyBBDiag(*PluginFunctions::polyMesh(*o_it));
#ifdef ENABLE_TSPLINEMESH_SUPPORT
      else if ( o_it->dataType( DATA_TSPLINE_MESH ) )
        unifyBBDiag(*PluginFunctions::tsplineMesh(*o_it));
#endif
    }

    emit updatedObject( o_it->id(), UPDATE_GEOMETRY );

  }

  emit updateView();
}


//------------------------------------------------------------------------------

/** \brief Update the Dialog with the last clicked manipulator
 *
 */
void MovePlugin::updateManipulatorDialog() {

    BaseObjectData* object;
    if ( !PluginFunctions::getObject(lastActiveManipulator_ , object) ) {
	return;
    }

    if ( object->manipulatorNode()->visible() ) {

	// Get properties widget that corresponds to
        // to the last manipulated object
        movePropsWidget* pW = getDialogWidget(object);

        // If there's no properties dialog yet...
        if(pW == 0) return;

	const TriMesh::Point pos = object->manipulatorNode()->center();

	QString num;

	num = QString::number(pos[0]); pW->posx->setText(num);
	num = QString::number(pos[1]); pW->posy->setText(num);
	num = QString::number(pos[2]); pW->posz->setText(num);

	TriMesh::Point direction = object->manipulatorNode()->directionX();
	num = QString::number(direction[0]); pW->dirxx->setText(num);
	num = QString::number(direction[1]); pW->dirxy->setText(num);
	num = QString::number(direction[2]); pW->dirxz->setText(num);

	direction = object->manipulatorNode()->directionY();
	num = QString::number(direction[0]); pW->diryx->setText(num);
	num = QString::number(direction[1]); pW->diryy->setText(num);
	num = QString::number(direction[2]); pW->diryz->setText(num);

	direction = object->manipulatorNode()->directionZ();
	num = QString::number(direction[0]); pW->dirzx->setText(num);
	num = QString::number(direction[1]); pW->dirzy->setText(num);
	num = QString::number(direction[2]); pW->dirzz->setText(num);

    }
}

//------------------------------------------------------------------------------

/** \brief Get pointer to corresponding dialog widget
 *
 * @param _obj the object to which the dialog is attached
 */
movePropsWidget* MovePlugin::getDialogWidget(BaseObjectData* _obj) {

    for(QList<movePropsWidget*>::iterator it = propsWindows_.begin();
       it != propsWindows_.end(); it++) {
	   if ( (*it)->getBaseObjectDataId() == _obj->id() )
	       return *it;
       }
    return 0;
}
//------------------------------------------------------------------------------

/** \brief Called by Toolbar to enable move mode
 *
 * @param _action the action that was triggered
 */
void MovePlugin::slotSetMoveMode(QAction* _action) {

  if (_action == moveAction_){
    PluginFunctions::actionMode(Viewer::PickingMode);
    PluginFunctions::pickMode("Move");

    moveAction_->setChecked( true );
  }

  if (_action == moveSelectionAction_){

    connectSelectionActions();

    PluginFunctions::actionMode(Viewer::PickingMode);
    PluginFunctions::pickMode("MoveSelection");

    moveSelectionAction_->setChecked( true );
  }
  
#ifdef ENABLE_SKELETON_SUPPORT
  if (_action == moveSkeletonAction_){

    PluginFunctions::actionMode(Viewer::PickingMode);
    PluginFunctions::pickMode("MoveSkeleton");

    moveSkeletonAction_->setChecked( true );
  }
#endif
}

//------------------------------------------------------------------------------

/** \brief Called by pick mode Toolbar
 *
 * @param _action the action that was triggered
 */

void MovePlugin::slotPickToolbarAction(QAction* _action)
{

  if (_action == rotateTranslateAction_)
  {
    setManipMode (QtTranslationManipulatorNode::TranslationRotation);
  }

  if (_action == rotateManipAction_)
  {
    setManipMode (QtTranslationManipulatorNode::LocalRotation);
  }

  if (_action == placeAndSnapAction_)
  {
    setManipMode(QtTranslationManipulatorNode::Place);
  }

  if (_action == resizeAction_)
  {
    setManipMode(QtTranslationManipulatorNode::Resize);
  }

  if (_action == biggerManipAction_)
  {
    manip_size_modifier_ = manip_size_modifier_ + 0.1;
    for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)
         o_it->manipulatorNode()->set_size(manip_size_ * manip_size_modifier_);
    emit nodeVisibilityChanged (-1);
  }

  if (_action == smallerManipAction_)
  {
    manip_size_modifier_ = manip_size_modifier_ - 0.1;
    for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::ALL_OBJECTS) ; o_it != PluginFunctions::objectsEnd(); ++o_it)
         o_it->manipulatorNode()->set_size(manip_size_ * manip_size_modifier_);
    emit nodeVisibilityChanged (-1);
  }
#ifdef ENABLE_SKELETON_SUPPORT
  if (_action == fixChildManipAction_)
  {
    recursiveJointTransformation_ = fixChildManipAction_->isChecked();
  }
  
  if (_action == transformRefPoseManipAction_)
  {
    transformRefPose_ = transformRefPoseManipAction_->isChecked();
  }
  
  if (_action == currentPoseManipAction_)
  {
    transformCurrentPose_ = currentPoseManipAction_->isChecked();
  }
#endif

}


//------------------------------------------------------------------------------

/** \brief Get the Matrix of the last active Manipulator ( Identity if not found or hidden Manipulator )
  *
  * @param _reset reset the transformation matrix of the manipulator to identity)
  * @return current transformation matrix of the manipulator
  */
ACG::Matrix4x4d MovePlugin::getLastManipulatorMatrix(bool _reset) {
   ACG::Matrix4x4d matrix;
   matrix.identity();

   BaseObjectData* object;
   if ( PluginFunctions::getObject(lastActiveManipulator_ , object) )
         if (  object->manipulatorNode()->visible() ) {
            matrix = object->manipulatorNode()->matrix();
            if (_reset)
               object->manipulatorNode()->loadIdentity();
         }
   return matrix;
}


//------------------------------------------------------------------------------

/** \brief Transform a mesh with the given transformation matrix
 *
 * @param _mat transformation matrix
 * @param _mesh the mesh
 */
template< typename MeshT >
void MovePlugin::transformMesh(ACG::Matrix4x4d _mat , MeshT& _mesh ) {
  // Get the inverse matrix of the transformation for the normals
  ACG::Matrix4x4d invMat = _mat;
  invMat.invert ();

  typename MeshT::VertexIter v_it     = _mesh.vertices_begin();
  typename MeshT::VertexIter v_end = _mesh.vertices_end();
  for (; v_it!=v_end; ++v_it) {
    
    // transform the mesh vertex
    _mesh.set_point(v_it,_mat.transform_point(_mesh.point(v_it)));
    
    // transform the vertex normal
    typename MeshT::Point n = invMat.transform_vector(_mesh.normal(v_it));
    
    //re-normalize the vertex normal
    if (n.length () != 0.0)
        n *= (1.0/n.length ());
    
    _mesh.set_normal(v_it,n);
  }

  typename MeshT::FaceIter f_it     = _mesh.faces_begin();
  typename MeshT::FaceIter f_end = _mesh.faces_end();
  for (; f_it != f_end; ++f_it) {
    
    // transform the face normal
    typename MeshT::Point n = invMat.transform_vector(_mesh.normal(f_it));
    
    //re-normalize the face normal
    if (n.length () != 0.0)
        n *= (1.0/n.length ());
    
    _mesh.set_normal(f_it,n);
  }
}


//------------------------------------------------------------------------------

#ifdef ENABLE_POLYLINE_SUPPORT

/** \brief Transform a polyline with the given transformation matrix
 *
 * @param _mat transformation matrix
 * @param _polyLine the polyline
 */
template< class PolyLineT >
void MovePlugin::transformPolyLine( ACG::Matrix4x4d _mat , PolyLineT& _polyLine  ) {
  #ifdef USE_OPENMP
  #pragma omp parallel for
  #endif
  for ( int i = 0 ; i < (int)_polyLine.n_vertices(); ++i )
    _polyLine.point(i) = _mat.transform_point( _polyLine.point(i) );
}

#endif

//------------------------------------------------------------------------------

#ifdef ENABLE_SKELETON_SUPPORT

/** \brief Transform a skeleton with the given transformation matrix
 *
 * @param _mat transformation matrix
 * @param _skeleton the skeleton
 */
void MovePlugin::transformSkeleton(ACG::Matrix4x4d _mat , SkeletonObject* _skeletonObj ) {

  Skeleton* skeleton = PluginFunctions::skeleton(_skeletonObj);
  
  Skeleton::Joint* root = skeleton->getRoot();

  AnimationHandle handle = _skeletonObj->skeletonNode()->getActivePose();
  
  if ( !handle.isValid() ){
    
    //no current animation found -> transform the reference Pose
    Skeleton::Pose* pose = skeleton->getReferencePose();
    
    //the transformation only has to be applied to the root joint
    //it is automatically passed down to the children
    ACG::Matrix4x4d transform = pose->getGlobal(root->getID()) * _mat;
    
    pose->setGlobal( root->getID(), transform, false);
    
  } else {
  
    Skeleton::Animation* animation = skeleton->getAnimation(handle);
    
    //transform every Pose of the animation
    for(unsigned long iFrame = 0; iFrame < animation->getFrameCount(); iFrame++){
    
      Skeleton::Pose* pose = animation->getPose(iFrame);
      
      //the transformation only has to be applied to the root joint
      //it is automatically passed down to the children
      ACG::Matrix4x4d transform = _mat * pose->getGlobal(root->getID());
      
      pose->setGlobal( root->getID(), transform, false);
    }
  }
}

#endif

//------------------------------------------------------------------------------

/** \brief scale mesh to have a boundingboxdiagonal of one
 *
 * @param _mesh the mesh
 */
template< typename MeshT >
void MovePlugin::unifyBBDiag(MeshT& _mesh )
{
  typename MeshT::VertexIter v_it  = _mesh.vertices_begin();
  typename MeshT::VertexIter v_end = _mesh.vertices_end();

  // no vertices?
  if( v_it == v_end) return;

  typename MeshT::Point bb_min = _mesh.point(v_it);
  typename MeshT::Point bb_max = _mesh.point(v_it);

  for(; v_it!=v_end; ++v_it)
  {
    bb_min.minimize( _mesh.point(v_it));
    bb_max.maximize( _mesh.point(v_it));
  }

  typename MeshT::Point bb_center =  0.5 * (bb_min + bb_max) ;

  typename MeshT::Scalar scale = 1.0/(bb_max-bb_min).norm();

  for( v_it = _mesh.vertices_begin(); v_it!=v_end; ++v_it)
  {

     _mesh.point(v_it) = (_mesh.point(v_it) - bb_center) * scale + bb_center;
  }

  _mesh.update_normals();

}

/** \brief Get the bounding box of a mesh
 *
 * @param _mesh the mesh
 * @param _bb_min Lower left corner of bounding box
 * @param _bb_max Upper right corner of bounding box
 */
template< typename MeshT >
void MovePlugin::getBB( MeshT& _mesh, ACG::Vec3d& _bb_min, ACG::Vec3d& _bb_max  )
{
  typename MeshT::VertexIter v_it  = _mesh.vertices_begin();
  typename MeshT::VertexIter v_end = _mesh.vertices_end();

  // no vertices?
  if( v_it == v_end) return;

  _bb_min = _mesh.point(v_it);
  _bb_max = _mesh.point(v_it);

  for(; v_it!=v_end; ++v_it)
  {
    _bb_min.minimize( _mesh.point(v_it));
    _bb_max.maximize( _mesh.point(v_it));
  }


}

/** \brief Scales object such that bounding box diagonal has unit length
 *
 * @param _mesh the mesh
 * @param _bb_min Lower left corner of bounding box
 * @param _bb_max Upper right corner of bounding box
 */
template< typename MeshT >
void MovePlugin::unifyBBDiag( MeshT& _mesh, ACG::Vec3d& _bb_min, ACG::Vec3d& _bb_max  )
{

  typename MeshT::VertexIter v_it  = _mesh.vertices_begin();
  typename MeshT::VertexIter v_end = _mesh.vertices_end();

  typename MeshT::Point bb_center =  0.5 * (_bb_min + _bb_max) ;

  typename MeshT::Scalar scale = 1.0/(_bb_max-_bb_min).norm();

  for( v_it ; v_it!=v_end; ++v_it)
    _mesh.point(v_it) = (_mesh.point(v_it) - bb_center) * scale + bb_center;

  _mesh.update_normals();

}


//------------------------------------------------------------------------------

/** \brief Connect to SelectionPlugin
 *
 */
void MovePlugin::connectSelectionActions(){

  if (!selectionConnected_){

    selectionConnected_ = true;

    bool connected = false;

    QToolBar* selToolBar = 0;
    emit getToolBar( "Selection", selToolBar );

    if (selToolBar != 0){

      QActionGroup* actionGroup = 0;

      for (int i=0; i < selToolBar->actions().count(); i++){
        if( selToolBar->actions().at(i)->text() == tr("Enable Vertex Selection") ){

          actionGroup = selToolBar->actions().at(i)->actionGroup();

          if ( selToolBar->actions().at(i)->isChecked() )
            selectionType_ = VERTEX;

        } else if ( (selToolBar->actions().at(i)->text() == tr("Enable Edge Selection") )
                && (selToolBar->actions().at(i)->isChecked()) )
            selectionType_ = EDGE;
        else if ( (selToolBar->actions().at(i)->text() == tr("Enable Face Selection") )
                && (selToolBar->actions().at(i)->isChecked()) )
            selectionType_ = FACE;
      }

      if (actionGroup != 0){
        connect( actionGroup, SIGNAL( triggered(QAction*) ), this, SLOT(slotSelectionModeChanged(QAction*)) );
        connected = true;
      }
    }

    if (!connected)
      emit log(LOGWARN, tr("Unable to connect to Selection-Plugin. MoveSelection will work on vertices only."));
  }
}


//------------------------------------------------------------------------------

/** \brief The SelectionMode in SelectionPlugin Changed
 *
 * @param _action the action on the Selection-Toolbar that was hit
 */
void MovePlugin::slotSelectionModeChanged(QAction* _action){

  if (_action->text() == tr("Enable Vertex Selection"))
    selectionType_ = VERTEX;
  if (_action->text() == tr("Enable Edge Selection"))
    selectionType_ = EDGE;
  if (_action->text() == tr("Enable Face Selection"))
    selectionType_ = FACE;
}

//------------------------------------------------------------------------------

/** \brief Sets whether all targets should be affected or not
 *
 * @param _state Qt::CheckState of checkbox
 */

void MovePlugin::setAllTargets(bool _state) {
	allTargets_ = _state;
}

//--------------------------------------------------------------------------------

/**
 * \brief Get nearest vertex to hitPoint (use for snapping)
 */
template< typename MeshType >
OpenMesh::Vec3d MovePlugin::getNearestVertex(MeshType* _mesh, uint _fh, OpenMesh::Vec3d &_hitPoint) {

    typename MeshType::FaceHandle fh = _mesh->face_handle(_fh);

    if ( !fh.is_valid() )
        return OpenMesh::Vec3d(0.0, 0.0, 0.0);

    typename MeshType::FaceVertexIter fv_it(*_mesh, fh);
    typename MeshType::Point hitPointP = (typename MeshType::Point) _hitPoint;
    typename MeshType::Scalar shortest_distance = (_mesh->point(fv_it.handle()) - hitPointP).sqrnorm();
    typename MeshType::VertexHandle vh = fv_it.handle();

    for (; fv_it; ++fv_it) {

        typename MeshType::Scalar tmpdist =
            (_mesh->point(fv_it.handle()) - hitPointP).sqrnorm();

        if(tmpdist < shortest_distance) {
            shortest_distance = tmpdist;
            vh = fv_it.handle();
        }
    }

    return (OpenMesh::Vec3d)_mesh->point(vh);
}

//--------------------------------------------------------------------------------

/**
 * \brief Get nearest edge to hitPoint (use for snapping)
 */
template< typename MeshType >
OpenMesh::Vec3d MovePlugin::getNearestEdge(MeshType* _mesh, uint _fh, OpenMesh::Vec3d &_hitPoint) {

    typename MeshType::FaceHandle fh = _mesh->face_handle(_fh);

    if (!fh.is_valid())
        return OpenMesh::Vec3d(0.0, 0.0, 0.0);

    typename MeshType::FaceEdgeIter fe_it(*_mesh, fh);
    typename MeshType::Point hitPointP = (typename MeshType::Point) _hitPoint;

    typename MeshType::Point center;
    typename MeshType::Scalar closest_dist(-1);

    for (; fe_it; ++fe_it) {

        typename MeshType::HalfedgeHandle heh0 = _mesh->halfedge_handle(fe_it.handle(), 0);
        typename MeshType::HalfedgeHandle heh1 = _mesh->halfedge_handle(fe_it.handle(), 1);

        typename MeshType::Point lp0 = _mesh->point(_mesh->to_vertex_handle(heh0));
        typename MeshType::Point lp1 = _mesh->point(_mesh->to_vertex_handle(heh1));

        double dist_new = ACG::Geometry::distPointLineSquared( hitPointP, lp0, lp1);

        /*typename MeshType::Point b(hitPointP - lp0), a(((lp1 - lp0).sqrnorm()));
        typename MeshType::Scalar d = b|a;
        typename MeshType::Point x = lp0 + a * d;
        double dist_new = (hitPointP - x).length();*/

        if (dist_new < closest_dist || closest_dist == -1) {
            // save closest Edge
            closest_dist = dist_new;
            center = lp0 + (lp1 - lp0) * .5;

        }
    }

    return (OpenMesh::Vec3d)center;
}

//--------------------------------------------------------------------------------

/**
 * \brief Get nearest face center to hitPoint (use for snapping)
 */
template< typename MeshType >
OpenMesh::Vec3d MovePlugin::getNearestFace(MeshType* _mesh, uint _fh, OpenMesh::Vec3d &_hitPoint) {

    typename MeshType::FaceHandle fh = _mesh->face_handle(_fh);

    if ( !fh.is_valid() )
        return OpenMesh::Vec3d(0.0, 0.0, 0.0);

    typename MeshType::FaceVertexIter fv_it(*_mesh, fh);
    typename MeshType::Point hitPointP = (typename MeshType::Point) _hitPoint;

    typename MeshType::Point cog;
    uint count = 0;

    for (; fv_it; ++fv_it) {

        cog += _mesh->point(fv_it.handle());
        ++count;
    }

    return (OpenMesh::Vec3d)cog/count;
}

//--------------------------------------------------------------------------------

#ifdef ENABLE_SKELETON_SUPPORT
/**
 * \brief Get nearest joint to hitPoint (used for snapping)
 */
OpenMesh::Vec3d MovePlugin::getNearestJoint(SkeletonObject* _skeletonObj, OpenMesh::Vec3d &_hitPoint, int& _bestJointID) {

  ACG::Vec3d    bestJoint;
  double        bestDistance = DBL_MAX;
  
  _bestJointID = -1;
  
  Skeleton* skeleton = PluginFunctions::skeleton(_skeletonObj);

  AnimationHandle handle = _skeletonObj->skeletonNode()->getActivePose();

  Skeleton::Pose* pose = 0;
  
  if ( !handle.isValid() ){
    
    //no current animation found -> transform the reference Pose
    pose = skeleton->getReferencePose();
    
  } else {
  
    Skeleton::Animation* animation = skeleton->getAnimation(handle);

    pose = animation->getPose( handle.getFrame() );
  }
  
  //find the nearest joint
  for (unsigned long joint = 0; joint < skeleton->getJointCount(); joint++){
    
    double dist = (_hitPoint - pose->getGlobalTranslation(joint)).sqrnorm();
    
    if (dist < bestDistance){
      bestJoint     = pose->getGlobalTranslation(joint);
      _bestJointID  = joint;
      bestDistance  = dist;
    }
  }

  return (OpenMesh::Vec3d) bestJoint;
}


/** \brief make sure the manipulator is positioned on a joint
 *
 */
void MovePlugin::moveManipulatorOnSkeleton(BaseObjectData* _skeletonObj){

  int bestJoint;
  
  OpenMesh::Vec3d pos    = _skeletonObj->manipulatorNode()->center();
  OpenMesh::Vec3d newPos = getNearestJoint(PluginFunctions::skeletonObject(_skeletonObj), pos, bestJoint );
  
  
  _skeletonObj->manipulatorNode()->set_center(newPos);
  _skeletonObj->manipulatorNode()->setData( QVariant(bestJoint) );
}

#endif

//--------------------------------------------------------------------------------

void MovePlugin::slotAllCleared(){
  activeManipulators_.clear();
}

//--------------------------------------------------------------------------------

void MovePlugin::objectDeleted( int _id ){
  
  for (uint i=0; i < activeManipulators_.size(); i++)
    if ( activeManipulators_[i] == _id ){
      activeManipulators_.erase( activeManipulators_.begin() + i  );
      return;
    }
}

//--------------------------------------------------------------------------------

Q_EXPORT_PLUGIN2( moveplugin , MovePlugin );

