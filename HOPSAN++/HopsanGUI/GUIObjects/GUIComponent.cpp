/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011 
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   GUIComponent.cpp
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2010-01-01
//!
//! @brief Contains the GUI class representing Components
//!
//$Id$

#include <QDrag>

#include "Configuration.h"
#include "GraphicsView.h"
#include "GUIPort.h"
#include "PlotWindow.h"
#include "Dialogs/ComponentPropertiesDialog.h"
#include "GUIObjects/GUIComponent.h"
#include "GUIObjects/GUIContainerObject.h"
#include "Widgets/ProjectTabWidget.h"


Component::Component(QPointF position, qreal rotation, ModelObjectAppearance* pAppearanceData, ContainerObject *pParentContainer, selectionStatus startSelected, graphicsType gfxType)
    : ModelObject(position, rotation, pAppearanceData, startSelected, gfxType, pParentContainer, pParentContainer)
{
    //Set the hmf save tag name
    mHmfTagName = HMF_COMPONENTTAG;

    //Create the object in core, and get its default core name
    mModelObjectAppearance.setName(mpParentContainerObject->getCoreSystemAccessPtr()->createComponent(mModelObjectAppearance.getTypeName(), mModelObjectAppearance.getName()));

    //Sets the ports
    createPorts();

    refreshDisplayName(); //Make sure name window is correct size for center positioning

    //Component shall be hidden when toggle signals is deactivated, if it is of signal type and has no power ports (= is a sensor)
    if(this->getTypeCQS() == "S" && !this->hasPowerPorts())
    {
        connect(mpParentContainerObject, SIGNAL(showOrHideSignals(bool)), this, SLOT(setVisible(bool)));
    }

    //! @todo maybe set default param values for ALL ModelObjects
    QStringList defaultParameterNames = getParameterNames();
    for(int i=0; i<defaultParameterNames.size(); ++i)
    {
        mDefaultParameterValues.insert(defaultParameterNames.at(i), getParameterValue(defaultParameterNames.at(i)));
    }
}

Component::~Component()
{
    //Remove in core
    //! @todo maybe change to delte instead of remove with dodelete yes
    mpParentContainerObject->getCoreSystemAccessPtr()->removeSubComponent(this->getName(), true);
}


//! @brief Returns whether or not the component has at least one power port
bool Component::hasPowerPorts()
{
    bool retval = false;
    for(int i=0; i<mPortListPtrs.size(); ++i)
    {
        if(mPortListPtrs.at(i)->getNodeType() != "NodeSignal")
        {
            retval = true;
        }
    }
    return retval;
}


//! Event when double clicking on component icon.
//! @todo Fix the sink component so it works with this
void Component::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if(!mpParentContainerObject->mpParentProjectTab->isEditingEnabled())
        return;

    QGraphicsWidget::mouseDoubleClickEvent(event);
    //std::cout << "GUIComponent.cpp: " << "mouseDoubleClickEvent " << std::endl;

    //If this is a sink component that has plot data, plot it instead of showing the dialog
    if(this->getTypeName() == "SignalSink" && this->getPort("in")->isConnected() && this->mpParentContainerObject->getAllPlotData().size() > 0 && !mpParentContainerObject->isCreatingConnector())   //Not very nice code, but a nice feature...
    {
        PlotWindow *pPlotWindow = getPort("in")->getConnectedPorts().first()->plot("Value");
        for(int i=1; (i<getPort("in")->getConnectedPorts().size() && pPlotWindow != 0); ++i)
        {
            if(!pPlotWindow)
            {
                pPlotWindow = getPort("in")->getConnectedPorts().at(i)->plot("Value");
            }
            else
            {
                getPort("in")->getConnectedPorts().at(i)->plotToPlotWindow(pPlotWindow, "Value");
            }
        }
        if(this->getPort("in_right")->isConnected() && pPlotWindow)
        {
            for(int i=0; (i<getPort("in_right")->getConnectedPorts().size() && pPlotWindow != 0); ++i)
            {
                getPort("in_right")->getConnectedPorts().at(i)->plotToPlotWindow(pPlotWindow, "Value", QString(), 1);
            }
        }
        if(this->getPort("in_bottom")->isConnected() && pPlotWindow)
        {
            QString componentName = getPort("in_bottom")->getConnectedPorts().at(0)->mpParentGuiModelObject->getName();
            QString portName = getPort("in_bottom")->getConnectedPorts().at(0)->getPortName();
            QString dataName = "Value";
            pPlotWindow->changeXVector(mpParentContainerObject->getPlotData(mpParentContainerObject->getNumberOfPlotGenerations()-1, componentName, portName, dataName), componentName, portName, dataName, gConfig.getDefaultUnit(dataName));
        }

        //No plot window was opened, so it is a non-connected sink - open properties instead
        if(!pPlotWindow)
        {
            openPropertiesDialog();
        }
    }
    else
        openPropertiesDialog();
}


//! Returns a string with the component type.
QString Component::getTypeName()
{
    return mModelObjectAppearance.getTypeName();
}

QString Component::getTypeCQS()
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->getSubComponentTypeCQS(this->getName());
}


void Component::getParameters(QVector<QString> &qParameterNames, QVector<QString> &qParameterValues, QVector<QString> &qDescriptions, QVector<QString> &qUnits, QVector<QString> &qTypes)
{
    mpParentContainerObject->getCoreSystemAccessPtr()->getParameters(this->getName(), qParameterNames, qParameterValues, qDescriptions, qUnits, qTypes);
}


//! @brief Get a vector with the names of the available parameters
QStringList Component::getParameterNames()
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->getParameterNames(this->getName());
}

QString Component::getParameterUnit(QString name)
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->getParameterUnit(this->getName(), name);
}

QString Component::getParameterDescription(QString name)
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->getParameterDescription(this->getName(), name);
}

QString Component::getParameterValue(QString name)
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->getParameterValue(this->getName(), name);
}

//! @brief Set a parameter value to be mapped to a System parameter
bool Component::setParameterValue(QString name, QString value, bool force)
{
    return mpParentContainerObject->getCoreSystemAccessPtr()->setParameterValue(this->getName(), name, value, force);
}

//! @brief Set a start value to be mapped to a System parameter
//! @todo What does the "variable" parameter do?
bool Component::setStartValue(QString portName, QString variable, QString sysParName)
{
//    QVector<QString> vVariable;
//    QVector<QString> vSysParName;
//    vVariable.append(variable);
//    vSysParName.append(sysParName);
//    return this->getPort(portName)->setStartValueDataByNames(vVariable, vSysParName);
    QString dataName;
    dataName = portName + QString("::Value");
    return mpParentContainerObject->getCoreSystemAccessPtr()->setParameterValue(this->getName(), dataName, sysParName);
}


//! @brief Set a start value to be mapped to a System parameter
QString Component::getStartValueTxt(QString portName, QString variable)
{
    QVector<QString> vVariable, vSysParName, vUnit;
    this->getPort(portName)->getStartValueDataNamesValuesAndUnits(vVariable, vSysParName, vUnit);
    int idx = vVariable.indexOf(variable);
    if(idx < 0)
        return "";
    else
        return vSysParName[idx];
}


//! @brief Slot that opens the parameter dialog for the component
void Component::openPropertiesDialog()
{
    ComponentPropertiesDialog *pDialog = new ComponentPropertiesDialog(this, gpMainWindow);
    pDialog->exec();
    delete pDialog;
}


//! @brief Help function to create ports in the component when it is created
void Component::createPorts()
{
    //! @todo make sure that all old ports and connections are cleared, (not really necessary in guicomponents)
    QString cqsType = mpParentContainerObject->getCoreSystemAccessPtr()->getSubComponentTypeCQS(getName());
    PortAppearanceMapT::iterator i;
    for (i = mModelObjectAppearance.getPortAppearanceMap().begin(); i != mModelObjectAppearance.getPortAppearanceMap().end(); ++i)
    {
        QString nodeType = mpParentContainerObject->getCoreSystemAccessPtr()->getNodeType(this->getName(), i.key());
        QString portType = mpParentContainerObject->getCoreSystemAccessPtr()->getPortType(this->getName(), i.key());
        i.value().selectPortIcon(cqsType, portType, nodeType);

        qreal x = i.value().x * this->boundingRect().width();
        qreal y = i.value().y * this->boundingRect().height();

        Port *pNewPort = new Port(i.key(), x, y, &(i.value()), this);
        mPortListPtrs.append(pNewPort);
    }
}



int Component::type() const
{
    return Type;
}



void Component::setVisible(bool visible)
{
    this->mpIcon->setVisible(visible);
    for(int i=0; i<mPortListPtrs.size(); ++i)
    {
        mPortListPtrs.at(i)->showIfNotConnected(!mpParentContainerObject->areSubComponentPortsHidden());
    }
}


//! @brief Save component coredata to XML Dom Element
//! @param[in] rDomElement The dom element to save to
void Component::saveCoreDataToDomElement(QDomElement &rDomElement)
{
    ModelObject::saveCoreDataToDomElement(rDomElement);

    //Save parameters (also core related)
    QDomElement xmlParameters = appendDomElement(rDomElement, HMF_PARAMETERS);
    QVector<QString> parameterNames, parameterValues, descriptions, units, types;
    getParameters(parameterNames, parameterValues, descriptions, units, types);
    for(int pit=0; pit<parameterNames.size(); ++pit)
    {
        QDomElement xmlParam = appendDomElement(xmlParameters, HMF_PARAMETERTAG);
        xmlParam.setAttribute(HMF_NAMETAG, parameterNames[pit]);
        xmlParam.setAttribute(HMF_VALUETAG, parameterValues[pit]);
        xmlParam.setAttribute(HMF_TYPE, types[pit]);

        /*if(this->isParameterMappedToSystemParameter(*pit))
        {
            xmlParam.setAttribute(HMF_SYSTEMPARAMETERTAG, this->getSystemParameterKey(*pit));
        }*/
    }

    //Save start values //Is not needed, start values are saved as ordinary parameters! This code snippet can probably be removed.
//    QDomElement xmlStartValues = appendDomElement(rDomElement, HMF_STARTVALUES);
//    QVector<QString> startValueNames;
//    QVector<QString> startValueValuesTxt;
//    QVector<QString> dummy;
//    QList<GUIPort*>::iterator portIt;
//    for(portIt = mPortListPtrs.begin(); portIt != mPortListPtrs.end(); ++portIt)
//    {
//        mpParentContainerObject->getCoreSystemAccessPtr()->getStartValueDataNamesValuesAndUnits(this->getName(), (*portIt)->getPortName(), startValueNames, startValueValuesTxt, dummy);
//        if((!startValueNames.empty()))
//        {
//            for(int i = 0; i < startValueNames.size(); ++i)
//            {
//                QDomElement xmlStartValue = appendDomElement(xmlStartValues, "startvalue");
//                xmlStartValue.setAttribute("portname", (*portIt)->getPortName());
//                xmlStartValue.setAttribute("variable", startValueNames[i]);
//                xmlStartValue.setAttribute("value", startValueValuesTxt[i]);
//            }
//        }
//    }
}
