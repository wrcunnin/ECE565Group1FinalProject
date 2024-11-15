/*
 * Copyright (c) 2013-2014 ARM Limited
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
 *  Dummy execution stage between decode and execution.
 *
 */

#ifndef __CPU_MYMINOR_EXECUTEDUMMY_HH__
#define __CPU_MYMINOR_EXECUTEDUMMY_HH__

#include <vector>

#include "base/named.hh"
#include "base/types.hh"
#include "cpu/myminor/buffers.hh"
#include "cpu/myminor/cpu.hh"
#include "cpu/myminor/func_unit.hh"
#include "cpu/myminor/lsq.hh"
#include "cpu/myminor/pipe_data.hh"
#include "cpu/myminor/scoreboard.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

/** Execute stage.  Everything apart from fetching and decoding instructions.
 *  The LSQ lives here too. */
class ExecuteDummy : public Named
{
  protected:

    /** Input port carrying instructions from Decode */
    Latch<ForwardInstData>::Output inp;

    /** Input port carrying stream changes to Fetch1 */
    Latch<ForwardInstData>::Input out;

    /** Pointer back to the containing CPU */
    MyMinorCPU &cpu;

    /** Interface to reserve space in the next stage */
    std::vector<InputBuffer<ForwardInstData>> &nextStageReserve;

    /** Width of output of this stage/input of next in instructions */
    unsigned int outputWidth;

    /** If true, more than one input word can be processed each cycle if
     *  there is room in the output to contain its processed data */
    bool processMoreThanOneInput;

  public: /* Public for Pipeline to be able to pass it to Decode */
    std::vector<InputBuffer<ForwardInstData>> inputBuffer;

  protected:
    /** Stage cycle-by-cycle state */

    /** State that drain passes through (in order).  On a drain request,
     *  Execute transitions into either DrainCurrentInst (if between
     *  microops) or DrainHaltFetch.
     *
     *  Note that Execute doesn't actually have *  a 'Drained' state, only
     *  an indication that it's currently draining and isDrained that can't
     *  tell if there are insts still in the pipeline leading up to
     *  Execute */
    enum DrainState
    {
        NotDraining, /* Not draining, possibly running */
        DrainCurrentInst, /* Draining to end of inst/macroop */
        DrainHaltFetch, /* Halting Fetch after completing current inst */
        DrainAllInsts /* Discarding all remaining insts */
    };

    struct ExecuteDummyThreadInfo
    {
        ExecuteDummyThreadInfo() {}

        ExecuteDummyThreadInfo(const ExecuteDummyThreadInfo& other) :
            inputIndex(other.inputIndex),
            streamSeqNum(other.streamSeqNum),
            execSeqNum(other.execSeqNum),
            drainState(other.drainState),
            blocked(other.blocked)
        { }


        /** Index into the inputBuffer's head marking the start of unhandled
         *  instructions */
        unsigned int inputIndex = 0;

        /** Source of sequence number for instuction streams.  Increment this and
         *  pass to fetch whenever an instruction stream needs to be changed.
         *  For any more complicated behaviour (e.g. speculation) there'll need
         *  to be another plan. */
        InstSeqNum streamSeqNum;

        /** Source of execSeqNums to number instructions. */
        InstSeqNum execSeqNum = InstId::firstExecSeqNum;

        /** State progression for draining NotDraining -> ... -> DrainAllInsts */
        DrainState drainState;

        /** Blocked indication for report */
        bool blocked = false;
    };

    std::vector<ExecuteDummyThreadInfo> executeDummyInfo;
    ThreadID threadPriority;

  protected:
    friend std::ostream &operator <<(std::ostream &os, DrainState state);

    /** Get a piece of data to work on from the inputBuffer, or 0 if there
     *  is no data. */
    const ForwardInstData *getInput(ThreadID tid);  

    /** Pop an element off the input buffer, if there are any */
    void popInput(ThreadID tid);

    /** Use the current threading policy to determine the next thread to
     *  decode from. */
    ThreadID getScheduledThread();

    /** Set the drain state (with useful debugging messages) */
    void setDrainState(ThreadID thread_id, DrainState state);

  public:
    ExecuteDummy(const std::string &name,
        MyMinorCPU &cpu_,
        const BaseMyMinorCPUParams &params,
        Latch<ForwardInstData>::Output inp_,
        Latch<ForwardInstData>::Input out_,
        std::vector<InputBuffer<ForwardInstData>> &next_stage_input_buffer);

  public:
    /** Pass on input/buffer data to the output if you can */
    void evaluate();

    void myminorTrace() const;

    /** After thread suspension, has Execute been drained of in-flight
     *  instructions and memory accesses. */
    bool isDrained();

    /** Like the drain interface on SimObject */
    unsigned int drain();
    void drainResume();
};

} // namespace myminor
} // namespace gem5

#endif /* __CPU_MYMINOR_EXECUTEDUMMY_HH__ */