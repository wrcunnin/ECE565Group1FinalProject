/*
 * Copyright (c) 2013-2014, 2017 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 *  The constructed pipeline.  Kept out of MyMinorCPU to keep the interface
 *  between the CPU and its grubby implementation details clean.
 */

#ifndef __CPU_MYMINOR_PIPELINE_HH__
#define __CPU_MYMINOR_PIPELINE_HH__

#include "cpu/myminor/activity.hh"
#include "cpu/myminor/cpu.hh"
#include "cpu/myminor/decode.hh"
#include "cpu/myminor/executeDummy.hh"
#include "cpu/myminor/execute.hh"
#include "cpu/myminor/fetch1.hh"
#include "cpu/myminor/fetch2.hh"
#include "params/BaseMyMinorCPU.hh"
#include "sim/ticked_object.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

/**
 * @namespace myminor
 *
 * MyMinor contains all the definitions within the MyMinorCPU apart from the CPU
 * class itself
 */

/** The constructed pipeline.  Kept out of MyMinorCPU to keep the interface
 *  between the CPU and its grubby implementation details clean. */
class Pipeline : public Ticked
{
  protected:
    MyMinorCPU &cpu;

    /** Allow cycles to be skipped when the pipeline is idle */
    bool allow_idling;

    Latch<ForwardLineData> f1ToF2;
    Latch<BranchData> f2ToF1;
    Latch<ForwardInstData> f2ToD;
    Latch<ForwardInstData> dToE;
    Latch<ForwardInstData> eToE; // dummy execution stage
    Latch<BranchData> eToF1;

    Execute execute;
    ExecuteDummy executeDummy;
    Decode decode;
    Fetch2 fetch2;
    Fetch1 fetch1;

    /** Activity recording for the pipeline.  This is access through the CPU
     *  by the pipeline stages but belongs to the Pipeline as it is the
     *  cleanest place to initialise it */
    MyMinorActivityRecorder activityRecorder;

  public:
    /** Enumerated ids of the 'stages' for the activity recorder */
    enum StageId
    {
        /* A stage representing wakeup of the whole processor */
        CPUStageId = 0,
        /* Real pipeline stages */
        Fetch1StageId, Fetch2StageId, DecodeStageId, ExecuteDummyStageId, ExecuteStageId,
        Num_StageId /* Stage count */
    };

    /** True after drain is called but draining isn't complete */
    bool needToSignalDrained;

  public:
    Pipeline(MyMinorCPU &cpu_, const BaseMyMinorCPUParams &params);

  public:
    /** Wake up the Fetch unit.  This is needed on thread activation esp.
     *  after quiesce wakeup */
    void wakeupFetch(ThreadID tid);

    /** Try to drain the CPU */
    bool drain();

    void drainResume();

    /** Test to see if the CPU is drained */
    bool isDrained();

    /** A custom evaluate allows report in the right place (between
     *  stages and pipeline advance) */
    void evaluate() override;

    void myminorTrace() const;

    /** Functions below here are BaseCPU operations passed on to pipeline
     *  stages */

    /** Return the IcachePort belonging to Fetch1 for the CPU */
    MyMinorCPU::MyMinorCPUPort &getInstPort();
    /** Return the DcachePort belonging to Execute for the CPU */
    MyMinorCPU::MyMinorCPUPort &getDataPort();

    /** To give the activity recorder to the CPU */
    MyMinorActivityRecorder *getActivityRecorder() { return &activityRecorder; }
};

} // namespace myminor
} // namespace gem5

#endif /* __CPU_MYMINOR_PIPELINE_HH__ */
