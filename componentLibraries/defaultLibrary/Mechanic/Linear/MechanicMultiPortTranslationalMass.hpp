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

//$Id$

#ifndef MECHANICMULTIPORTTRANSLATIONALMASS_HPP_INCLUDED
#define MECHANICMULTIPORTTRANSLATIONALMASS_HPP_INCLUDED

#include <sstream>
#include <math.h>

#include "ComponentEssentials.h"
#include "ComponentUtilities.h"

namespace hopsan {

    //!
    //! @brief
    //! @ingroup MechanicalComponents
    //!
    class MechanicMultiPortTranslationalMass : public ComponentQ
    {

    private:
        double *mpM, *B, *k, *xMin, *xMax;
        double f1, v1, c1, Zx1, f2, v2, c2, Zx2;                                                    //Node data variables
        double mNum[3];
        double mDen[3];
        DoubleIntegratorWithDamping mIntegrator;
        std::vector<double*> mvpN_f1, mvpN_x1, mvpN_v1, mvpN_me1, mvpN_c1, mvpN_Zx1, mvpN_f2, mvpN_x2, mvpN_v2, mvpN_me2, mvpN_c2, mvpN_Zx2;
        std::vector<double> x1, x2, mvpStartX1, mvpStartX2;
        size_t mNumPorts1, mNumPorts2;
        Port *mpP1, *mpP2;

    public:
        static Component *Creator()
        {
            return new MechanicMultiPortTranslationalMass();
        }

        void configure()
        {
            //Add ports to the component
            mpP1 = addPowerMultiPort("P1", "NodeMechanic");
            mpP2 = addPowerMultiPort("P2", "NodeMechanic");

            //Register changable parameters to the HOPSAN++ core
            addInputVariable("m", "Mass", "kg",                100.0, &mpM);
            addInputVariable("B", "Viscous Friction", "Ns/m",  10.0,  &B);
            //addInputVariable("k", "Spring Coefficient", "N/m", 0.0,   &k);
            //! @todo what about k
            addInputVariable("x_min", "Minimum Position of Port P2", "m", 0.0,   &xMin);
            addInputVariable("x_max", "Maximum Position of Port P2", "m", 1.0,   &xMax);
        }


        void initialize()
        {
            mNumPorts1 = mpP1->getNumPorts();
            mNumPorts2 = mpP2->getNumPorts();

            mvpN_f1.resize(mNumPorts1);
            mvpN_x1.resize(mNumPorts1);
            mvpN_v1.resize(mNumPorts1);
            mvpN_me1.resize(mNumPorts1);
            mvpN_c1.resize(mNumPorts1);
            mvpN_Zx1.resize(mNumPorts1);

            mvpN_f2.resize(mNumPorts2);
            mvpN_x2.resize(mNumPorts2);
            mvpN_v2.resize(mNumPorts2);
            mvpN_me2.resize(mNumPorts2);
            mvpN_c2.resize(mNumPorts2);
            mvpN_Zx2.resize(mNumPorts2);

            x1.resize(mNumPorts1);
            x2.resize(mNumPorts2);
            mvpStartX1.resize(mNumPorts1);
            mvpStartX2.resize(mNumPorts2);

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                mvpN_f1[i]  = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::FORCE);
                mvpN_x1[i]  = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::Position);
                mvpN_v1[i]  = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::Velocity);
                mvpN_me1[i]  = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::EquivalentMass);
                mvpN_c1[i]  = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::WaveVariable);
                mvpN_Zx1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::CharImpedance);
            }

            for (size_t i=0; i<mNumPorts2; ++i)
            {
                mvpN_f2[i]  = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::Force);
                mvpN_x2[i]  = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::Position);
                mvpN_v2[i]  = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::Velocity);
                mvpN_me2[i]  = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::EquivalentMass);
                mvpN_c2[i]  = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::WaveVariable);
                mvpN_Zx2[i] = getSafeMultiPortNodeDataPtr(mpP2, i, NodeMechanic::CharImpedance);
            }



            //Initialization
            f1 = 0;
            for (size_t i=0; i<mNumPorts1; ++i)
            {
                f1 += (*mvpN_f1[i]);
            }

            f2 = 0;
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                f2 += (*mvpN_f2[i]);
            }

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                x1[i] = (*mvpN_x1[i]);
                mvpStartX1[i]= (*mvpN_x1[i]);
            }

            for (size_t i=0; i<mNumPorts2; ++i)
            {
                x2[i] = (*mvpN_x2[i]);
                mvpStartX2[i] = (*mvpN_x2[i]);
            }

            v1 = (*mvpN_v1[0]);
            for(size_t i=0; i<mNumPorts1; ++i)
            {
                if(v1 != (*mvpN_v1[i]))
                {
                    addErrorMessage("Velocities in multiport does not match, {"+getName()+"::"+mpP1->getName());
                    stopSimulation();
                }
            }

            v2 = (*mvpN_v2[0]);
            for(size_t i=0; i<mNumPorts2; ++i)
            {
                if(v2 != (*mvpN_v2[i]))
                {
                    addWarningMessage("Velocities in multiport does not match, {"+getName()+"::"+mpP2->getName());
                    stopSimulation();
                }
            }

            mIntegrator.initialize(mTimestep, 0, 0, x2[0], v2);

            //Print debug message if velocities do not match
            if(v1 != -v2)
            {
                addWarningMessage("Start velocities does not match, {"+getName()+"::"+mpP1->getName()+
                                  "} and {"+getName()+"::"+mpP2->getName()+"}.");
                stopSimulation();
            }

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpN_me1[i]) = (*mpM);
            }
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                (*mvpN_me2[i]) = (*mpM);
            }
        }


        void simulateOneTimestep()
        {
            //Get variable values from nodes
            c1 = 0;
            for (size_t i=0; i<mNumPorts1; ++i)
            {
                c1 += (*mvpN_c1[i]);
            }
            Zx1 = 0;
            for (size_t i=0; i<mNumPorts1; ++i)
            {
                Zx1 += (*mvpN_Zx1[i]);
            }
            c2 = 0;
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                c2 += (*mvpN_c2[i]);
            }
            Zx2 = 0;
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                Zx2 += (*mvpN_Zx2[i]);
            }

            const double m = (*mpM);

            mIntegrator.setDamping(((*B)+Zx1+Zx2) / m * mTimestep);
            mIntegrator.integrateWithUndo((c1-c2)/m);
            v2 = mIntegrator.valueFirst();
            double x_nom = mIntegrator.valueSecond();

            if(x_nom<(*xMin))
            {
                x_nom=(*xMin);
                v2=std::max(0.0, v2);
                mIntegrator.initializeValues(0.0, x_nom, v2);
            }
            if(x_nom>(*xMax))
            {
                x_nom=(*xMax);
                v2=std::min(0.0, v2);
                mIntegrator.initializeValues(0.0, x_nom, v2);
            }

            v1 = -v2;

            //Write new values to nodes

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpN_f1[i]) = (*mvpN_c1[i]) + (*mvpN_Zx1[i])*v1;
            }
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                (*mvpN_f2[i]) = (*mvpN_c2[i]) + (*mvpN_Zx2[i])*v2;
            }

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpN_x1[i]) = mvpStartX1[i]+mvpStartX2[0]-x_nom;
            }
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                (*mvpN_x2[i]) = mvpStartX2[i]-mvpStartX2[0]+x_nom;
            }

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpN_v1[i]) = v1;
            }
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                (*mvpN_v2[i]) = v2;
            }
            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpN_me1[i]) = m;
            }
            for (size_t i=0; i<mNumPorts2; ++i)
            {
                (*mvpN_me2[i]) = m;
            }
        }
    };
}

#endif // MECHANICMULTIPORTTRANSLATIONALMASS_HPP_INCLUDED

