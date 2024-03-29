/***********************************************************************
SixAxisNavigationTool - Class to convert an input device with six
valuators into a navigation tool.
Copyright (c) 2010-2011 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Vrui/Tools/SixAxisNavigationTool.h>

#include <vector>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

using GLTransformationWrappers::glMultMatrix; // PO'Leary

namespace Vrui {

/*********************************************
Methods of class SixAxisNavigationToolFactory:
*********************************************/

SixAxisNavigationToolFactory::SixAxisNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("SixAxisNavigationTool",toolManager),
	 zoomFactor(Scalar(1)),
	 followDisplayCenter(false),
	 navigationCenter(getDisplayCenter()),
	 invertNavigation(false),
	 showNavigationCenter(true)
	{
	/* Initialize tool layout: */
	layout.setNumValuators(7);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	
	typedef std::vector<Vector> VectorList;
	
	/* Initialize translation vectors: */
	Scalar translateFactor=cfs.retrieveValue<Scalar>("./translateFactor",getDisplaySize()/Scalar(3));
	VectorList translationVectors;
	for(int i=0;i<3;++i)
		{
		Vector t=Vector::zero;
		t[i]=Scalar(1);
		translationVectors.push_back(t);
		}
	translationVectors=cfs.retrieveValue<VectorList>("./translationVectors",translationVectors);
	if(translationVectors.size()!=3)
		Misc::throwStdErr("SixAxisNavigationToolFactory: wrong number of translation vectors; got %u, needed 3",(unsigned int)translationVectors.size());
	
	for(int i=0;i<3;++i)
		translations[i]=translationVectors[i]*translateFactor;
	
	/* Initialize rotation axes: */
	Scalar rotateFactor=Math::rad(cfs.retrieveValue<Scalar>("./rotateFactor",Scalar(180)));
	VectorList scaledRotationAxes;
	for(int i=0;i<3;++i)
		{
		Vector r=Vector::zero;
		r[i]=Scalar(1);
		scaledRotationAxes.push_back(r);
		}
	scaledRotationAxes=cfs.retrieveValue<VectorList>("./scaledRotationAxes",scaledRotationAxes);
	if(scaledRotationAxes.size()!=3)
		Misc::throwStdErr("SixAxisNavigationToolFactory: wrong number of rotation axes; got %u, needed 3",(unsigned int)scaledRotationAxes.size());
	
	for(int i=0;i<3;++i)
		rotations[i]=scaledRotationAxes[i]*rotateFactor;
	
	/* Get other parameters: */
	zoomFactor=cfs.retrieveValue<Scalar>("./zoomFactor",zoomFactor);
	if(cfs.hasTag("./navigationCenter"))
		navigationCenter=cfs.retrieveValue<Point>("./navigationCenter",navigationCenter);
	else
		followDisplayCenter=true;
	invertNavigation=cfs.retrieveValue<bool>("./invertNavigation",invertNavigation);
	showNavigationCenter=cfs.retrieveValue<bool>("./showNavigationCenter",showNavigationCenter);
	
	/* Set tool class' factory pointer: */
	SixAxisNavigationTool::factory=this;
	}

SixAxisNavigationToolFactory::~SixAxisNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	SixAxisNavigationTool::factory=0;
	}

const char* SixAxisNavigationToolFactory::getName(void) const
	{
	return "Six-Axis";
	}

const char* SixAxisNavigationToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Translate Vector 0";
		
		case 1:
			return "Translate Vector 1";
		
		case 2:
			return "Translate Vector 2";
		
		case 3:
			return "Rotate Axis 0";
		
		case 4:
			return "Rotate Axis 1";
		
		case 5:
			return "Rotate Axis 2";
		
		case 6:
			return "Zoom";
		
		default:
			return 0;
		}
	}

Tool* SixAxisNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new SixAxisNavigationTool(this,inputAssignment);
	}

void SixAxisNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveSixAxisNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createSixAxisNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	SixAxisNavigationToolFactory* sixAxisNavigationToolFactory=new SixAxisNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return sixAxisNavigationToolFactory;
	}

extern "C" void destroySixAxisNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**********************************************
Static elements of class SixAxisNavigationTool:
**********************************************/

SixAxisNavigationToolFactory* SixAxisNavigationTool::factory=0;

/**************************************
Methods of class SixAxisNavigationTool:
**************************************/

SixAxisNavigationTool::SixAxisNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(sFactory,inputAssignment),
	 numActiveAxes(0)
	{
	}

SixAxisNavigationTool::~SixAxisNavigationTool(void)
	{
	}

const ToolFactory* SixAxisNavigationTool::getFactory(void) const
	{
	return factory;
	}

void SixAxisNavigationTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	/* Check if the valuator became active or inactive: */
	if(cbData->oldValuatorValue==0.0&&cbData->newValuatorValue!=0.0)
		{
		/* Activate the axis: */
		++numActiveAxes;
		
		/* Try activating navigation if it is not already active: */
		if(!isActive()&&activate())
			{
			/* Initialize transient navigation state: */
			navTransform=getNavigationTransformation();
			}
		}
	if(cbData->oldValuatorValue!=0.0&&cbData->newValuatorValue==0.0)
		{
		/* Deactivate the axis: */
		--numActiveAxes;
		if(numActiveAxes==0)
			{
			/* Deactivate navigation: */
			deactivate();
			}
		}
	}

void SixAxisNavigationTool::frame(void)
	{
	if(isActive())
		{
		/* Assemble translation from translation vectors and current valuator values: */
		Vector translation=Vector::zero;
		for(int i=0;i<3;++i)
			translation+=factory->translations[i]*Scalar(getValuatorState(i));
		translation*=getCurrentFrameTime();
		
		/* Assemble rotation from scaled rotation axes and current valuator values: */
		Vector rotation=Vector::zero;
		for(int i=0;i<3;++i)
			rotation+=factory->rotations[i]*Scalar(getValuatorState(3+i));
		rotation*=getCurrentFrameTime();
		
		/* Calculate incremental zoom factor: */
		Scalar zoom=factory->zoomFactor*Scalar(getValuatorState(6))*getCurrentFrameTime();
		
		/* Apply proper navigation mode: */
		if(factory->invertNavigation)
			{
			translation=-translation;
			rotation=-rotation;
			zoom=-zoom;
			}
		
		/* Calculate an incremental transformation based on the translation and rotation: */
		NavTrackerState deltaT=NavTrackerState::translateFromOriginTo(factory->followDisplayCenter?getDisplayCenter():factory->navigationCenter);
		deltaT*=NavTrackerState::translate(translation);
		deltaT*=NavTrackerState::rotate(NavTrackerState::Rotation::rotateScaledAxis(rotation));
		deltaT*=NavTrackerState::scale(Math::exp(-zoom));
		deltaT*=NavTrackerState::translateToOriginFrom(factory->followDisplayCenter?getDisplayCenter():factory->navigationCenter);
		
		/* Update the accumulated transformation: */
		navTransform.leftMultiply(deltaT);
		navTransform.renormalize();
		
		/* Update the navigation transformation: */
		setNavigationTransformation(navTransform);
		
		/* Request another frame: */
		scheduleUpdate(getApplicationTime()+1.0/125.0);
		}
	}

void SixAxisNavigationTool::display(GLContextData& contextData) const
	{
	if(factory->showNavigationCenter&&isActive())
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glDepthFunc(GL_LEQUAL);
		
		/* Calculate colors to draw the crosshairs: */
		Color bgColor=getBackgroundColor();
		Color fgColor;
		for(int i=0;i<3;++i)
			fgColor[i]=1.0f-bgColor[i];
		fgColor[3]=bgColor[3];
		
		/* Go to crosshair space: */
		glPushMatrix();
		ONTransform trans=calcHUDTransform(factory->followDisplayCenter?getDisplayCenter():factory->navigationCenter);
		glMultMatrix(trans);
		
		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glColor(bgColor);
		glVertex2d(-getDisplaySize(),0.0);
		glVertex2d(getDisplaySize(),0.0);
		glVertex2d(0.0,-getDisplaySize());
		glVertex2d(0.0,getDisplaySize());
		glEnd();
		
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glColor(fgColor);
		glVertex2d(-getDisplaySize(),0.0);
		glVertex2d(getDisplaySize(),0.0);
		glVertex2d(0.0,-getDisplaySize());
		glVertex2d(0.0,getDisplaySize());
		glEnd();
		
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
