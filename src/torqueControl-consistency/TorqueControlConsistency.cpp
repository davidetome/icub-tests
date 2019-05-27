/*
 * iCub Robot Unit Tests (Robot Testing Framework)
 *
 * Copyright (C) 2015-2019 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <math.h>
#include <robottestingframework/TestAssert.h>
#include <robottestingframework/dll/Plugin.h>
#include <yarp/os/Time.h>
#include <yarp/os/Property.h>

#include "TorqueControlConsistency.h"

//example1    -v -t TorqueControlConsistency.dll -p "--robot icub --part head --joints ""(0)"" --zero 0"
//example2    -v -t TorqueControlConsistency.dll -p "--robot icub --part head --joints ""(0 1 2)"" --zero 0 "
//example3    -v -t TorqueControlConsistency.dll -p "--robot icub --part head --joints ""(0 1 2 3 4 5)"" --zero 0"

using namespace robottestingframework;
using namespace yarp::os;
using namespace yarp::dev;

// prepare the plugin
ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(TorqueControlConsistency)

TorqueControlConsistency::TorqueControlConsistency() : yarp::robottestingframework::TestCase("TorqueControlConsistency") {
    jointsList=0;
    pos_tot=0;
    dd=0;
    ipos=0;
    iamp=0;
    icmd=0;
    iimd=0;
    ienc=0;
    itrq=0;
    cmd_some=0;
    cmd_tot=0;
    prevcurr_some=0;
    prevcurr_tot=0;
}

TorqueControlConsistency::~TorqueControlConsistency() { }

bool TorqueControlConsistency::setup(yarp::os::Property& property) {

    //updating the test name
    if(property.check("name"))
        setName(property.find("name").asString());

    // updating parameters
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("robot"), "The robot name must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("part"), "The part name must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("joints"), "The joints list must be given as the test parameter!");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(property.check("zero"),    "The zero position must be given as the test parameter!");

    robotName = property.find("robot").asString();
    partName = property.find("part").asString();

    zero = property.find("zero").asDouble();

    Bottle* jointsBottle = property.find("joints").asList();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(jointsBottle!=0,"unable to parse joints parameter");
    n_cmd_joints = jointsBottle->size();
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(n_cmd_joints>0,"invalid number of joints, it must be >0");

    Property options;
    options.put("device", "remote_controlboard");
    options.put("remote", "/"+robotName+"/"+partName);
    options.put("local", "/TorqueControlConsistencyTest/"+robotName+"/"+partName);

    dd = new PolyDriver(options);
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->isValid(),"Unable to open device driver");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(itrq),"Unable to open torque control interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(ienc),"Unable to open encoders interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(iamp),"Unable to open ampliefier interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(ipos),"Unable to open position interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(icmd),"Unable to open control mode interface");
    ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(dd->view(iimd),"Unable to open interaction mode interface");

    if (!ienc->getAxes(&n_part_joints))
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("unable to get the number of joints of the part");
    }

    if      (n_part_joints<=0)
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Error this part has in invalid (<=0) number of jonits");
    else if (jointsBottle->size() == 1)
        cmd_mode=single_joint;
    else if (jointsBottle->size() < n_part_joints)
        cmd_mode=some_joints;
    else if (jointsBottle->size() == n_part_joints)
        cmd_mode=all_joints;
    else
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("invalid joint selection?");

    cmd_tot = new double[n_part_joints];
    pos_tot=new double[n_part_joints];
    jointsList=new int[n_cmd_joints];
    cmd_some=new double[n_cmd_joints];
    prevcurr_tot=new double[n_part_joints];
    prevcurr_some=new double[n_cmd_joints];
    for (int i=0; i <n_cmd_joints; i++) jointsList[i]=jointsBottle->get(i).asInt();

    return true;
}

void TorqueControlConsistency::tearDown()
{
    if (jointsList) {delete jointsList; jointsList =0;}
    if (dd) {delete dd; dd =0;}
}

void TorqueControlConsistency::setMode(int desired_control_mode, yarp::dev::InteractionModeEnum desired_interaction_mode)
{
    for (int i=0; i<n_cmd_joints; i++)
    {
        icmd->setControlMode(jointsList[i],desired_control_mode);
        iimd->setInteractionMode(jointsList[i],desired_interaction_mode);
        yarp::os::Time::delay(0.010);
    }
}

void TorqueControlConsistency::verifyMode(int desired_control_mode, yarp::dev::InteractionModeEnum desired_interaction_mode, std::string title)
{
    int cmode;
    yarp::dev::InteractionModeEnum imode;
    int timeout = 0;

    while (1)
    {
        int ok=0;
        for (int i=0; i<n_cmd_joints; i++)
        {
            icmd->getControlMode (jointsList[i],&cmode);
            iimd->getInteractionMode(jointsList[i],&imode);
            if (cmode==desired_control_mode && imode==desired_interaction_mode) ok++;
        }
        if (ok==n_cmd_joints) break;
        if (timeout>100)
        {
            char sbuf[500];
            sprintf(sbuf,"Test (%s) failed: current mode is (%d,%d), it should be (%d,%d)",title.c_str(), desired_control_mode,desired_interaction_mode,cmode,imode);
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(sbuf);
        }
        yarp::os::Time::delay(0.2);
        timeout++;
    }
    char sbuf[500];
    sprintf(sbuf,"Test (%s) passed: current mode is (%d,%d)",title.c_str(), desired_control_mode,desired_interaction_mode);
    ROBOTTESTINGFRAMEWORK_TEST_REPORT(sbuf);
}

void TorqueControlConsistency::goHome()
{
    for (int i=0; i<n_cmd_joints; i++)
    {
        ipos->setRefSpeed(jointsList[i],20.0);
        ipos->positionMove(jointsList[i],zero);
    }

    int timeout = 0;
    while (1)
    {
        int in_position=0;
        for (int i=0; i<n_cmd_joints; i++)
        {
            ienc->getEncoder(jointsList[i],&pos_tot[jointsList[i]]);
            if (fabs(pos_tot[jointsList[i]]-zero)<0.5) in_position++;
        }
        if (in_position==n_cmd_joints) break;
        if (timeout>100)
        {
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Timeout while reaching zero position");
        }
        yarp::os::Time::delay(0.2);
        timeout++;
    }
}

void TorqueControlConsistency::setRefTorque(double value)
{
    cmd_single=value;
    if (cmd_mode==single_joint)
    {
        for (int i=0; i<n_cmd_joints; i++)
        {
            itrq->setRefTorque(jointsList[i],cmd_single);
        }
    }
    else if (cmd_mode==some_joints)
    {
        //same of single_joint, since multiple joint is not currently supported
        for (int i=0; i<n_cmd_joints; i++)
        {
            itrq->setRefTorque(jointsList[i],cmd_single);
        }
    }
    else if (cmd_mode==all_joints)
    {
        for (int i=0; i<n_part_joints; i++)
        {
            cmd_tot[i]=cmd_single;
        }
        itrq->setRefTorques(cmd_tot);
    }
    else
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid cmd_mode");
    }
    yarp::os::Time::delay(0.010);
}

void TorqueControlConsistency::verifyRefTorque(double verify_val, std::string title)
{
    double value;
    char sbuf[500];
    if (cmd_mode==single_joint)
    {
        for (int i=0; i<n_cmd_joints; i++)
        {
            itrq->getRefTorque(jointsList[i],&value);
            if (value==verify_val)
            {
                sprintf(sbuf,"Test (%s) passed, j%d current reference is (%f)",title.c_str(),i, verify_val);
                ROBOTTESTINGFRAMEWORK_TEST_REPORT(sbuf);
            }
            else
            {
                sprintf(sbuf,"Test (%s) failed: current reference is (%f), it should be (%f)",title.c_str(), value, verify_val);
                ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(sbuf);
            }
        }
    }
    else if (cmd_mode==some_joints)
    {
        //same of single_joint, since multiple joint is not currently supported
        for (int i=0; i<n_cmd_joints; i++)
        {
            itrq->getRefTorque(jointsList[i],&value);
            if (value==verify_val)
            {
                sprintf(sbuf,"Test (%s) passed j%d current reference is (%f)",title.c_str(),i, verify_val);
                ROBOTTESTINGFRAMEWORK_TEST_REPORT(sbuf);
            }
            else
            {
                sprintf(sbuf,"Test (%s) failed: current reference is (%f), it should be (%f)",title.c_str(), value, verify_val);
                ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(sbuf);
            }
        }
    }
    else if (cmd_mode==all_joints)
    {
        int ok=0;
        itrq->getRefTorques(cmd_tot);
        for (int i=0; i<n_part_joints; i++)
        {
            if (verify_val==cmd_tot[i]) ok++;
        }
        if (ok==n_part_joints)
        {
            sprintf(sbuf,"Test (%s) passed, current reference is (%f)",title.c_str(), verify_val);
            ROBOTTESTINGFRAMEWORK_TEST_REPORT(sbuf);
        }
        else
        {
            sprintf(sbuf,"Test (%s) failed: only %d joints (of %d) are ok",title.c_str(),ok,n_part_joints);
            ROBOTTESTINGFRAMEWORK_ASSERT_ERROR(sbuf);
        }
    }
    else
    {
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR("Invalid cmd_mode");
    }
    yarp::os::Time::delay(0.010);
}

void TorqueControlConsistency::run()
{
    setMode(VOCAB_CM_POSITION,VOCAB_IM_STIFF);
    verifyMode(VOCAB_CM_POSITION,VOCAB_IM_STIFF,"test0");
    goHome();

    setMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF);
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test1");
    setRefTorque(0.1);
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test2");
    verifyRefTorque(0.1,"test2a");
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test2b");

    setRefTorque(0);
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test3");
    verifyRefTorque(0,"test3a");
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test3b");
    setRefTorque(-0.1);
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test4");
    verifyRefTorque(-0.1,"test4a");
    verifyMode(VOCAB_CM_TORQUE,VOCAB_IM_STIFF,"test4b");

    setMode(VOCAB_CM_POSITION,VOCAB_IM_STIFF);
    verifyMode(VOCAB_CM_POSITION,VOCAB_IM_STIFF,"test5");
    goHome();

}
