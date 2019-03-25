// -*- mode:C++ { } tab-width:4 { } c-basic-offset:4 { } indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2015 iCub Facility
 * Authors: Marco Randazzo
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#ifndef _TORQUECONTORLCONSISTENCY_H_
#define _TORQUECONTORLCONSISTENCY_H_

#include <string>
#include <yarp/rtf/TestCase.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/PolyDriver.h>

class TorqueControlConsistency : public yarp::rtf::TestCase {
public:
    TorqueControlConsistency();
    virtual ~TorqueControlConsistency();

    virtual bool setup(yarp::os::Property& property);

    virtual void tearDown();

    virtual void run();

    void goHome();
    void executeCmd();
    void setMode(int desired_control_mode, yarp::dev::InteractionModeEnum desired_interaction_mode);
    void verifyMode(int desired_control_mode, yarp::dev::InteractionModeEnum desired_interaction_mode, std::string title);
    void setRefTorque(double value);
    void verifyRefTorque(double value, std::string title);

    void zeroCurrentLimits();
    void getOriginalCurrentLimits();
    void resetOriginalCurrentLimits();

private:
    std::string robotName;
    std::string partName;
    int* jointsList;

    double zero;
    int    n_part_joints;
    int    n_cmd_joints;
    enum cmd_mode_t
    { 
      single_joint = 0,
      all_joints = 1,
      some_joints =2
    } cmd_mode;

    yarp::dev::PolyDriver        *dd;
    yarp::dev::IPositionControl *ipos;
    yarp::dev::IAmplifierControl *iamp;
    yarp::dev::IControlMode     *icmd;
    yarp::dev::IInteractionMode  *iimd;
    yarp::dev::IEncoders         *ienc;
    yarp::dev::ITorqueControl    *itrq;

    double  cmd_single;
    double* cmd_tot;
    double* cmd_some;

    double  prevcurr_single;
    double* prevcurr_tot;
    double* prevcurr_some;

    double* pos_tot;
};

#endif //_TORQUECONTORLCONSISTENCY_H
