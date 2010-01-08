#ifndef EXTERNALSTEP_HPP_INCLUDED
#define EXTERNALSTEP_HPP_INCLUDED

#include "HopsanCore.h"

class ComponentExternalStep : public ComponentSignal
{

private:
    double mStartValue;
    double mStepSize;
    double mStepTime;
    enum {out};

public:
    static Component *Creator()
    {
        std::cout << "running Step creator" << std::endl;
        return new ComponentExternalStep("DefaultStepName");
    }


    ComponentExternalStep(const string name,
                          const double startvalue = 0.0,
                          const double stepsize = 1.0,
                          const double steptime = 1.0,
                          const double timestep = 0.001)
	: ComponentSignal(name, timestep)
    {
        mStartValue = startvalue;
        mStepSize = stepsize;
        mStepTime = steptime;

        addPort("out", "NodeSignal", out);

        registerParameter("StartValue", "Start Value", "-", mStartValue);
        registerParameter("StepSize", "Final Value", "-", mStepSize);
        registerParameter("StepTime", "Step Time", "-", mStepTime);
    }


	void initialize()
	{
        //Nothing to initilize
	}


    void simulateOneTimestep()
    {
        //read fron nodes
   		Node* p1_ptr = mPorts[out].getNodePtr();

        //Step Equations
        double outputSignal;
        if (mTime < mStepTime)
        {
            outputSignal = mStartValue;
        }
        else
        {
            outputSignal = mStartValue + mStepSize;
        }

        //Write new values to nodes
        p1_ptr->setData(NodeSignal::VALUE, outputSignal);
    }
};

#endif // EXTERNALSTEP_HPP_INCLUDED
