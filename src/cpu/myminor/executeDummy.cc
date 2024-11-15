/*
 * Copyright (c) 2013-2014,2018-2020 ARM Limited
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

#include "cpu/myminor/executeDummy.hh"

#include <functional>

#include "cpu/myminor/cpu.hh"
#include "cpu/myminor/exec_context.hh"
#include "cpu/myminor/fetch1.hh"
#include "cpu/myminor/lsq.hh"
#include "cpu/op_class.hh"
#include "debug/Activity.hh"
#include "debug/Branch.hh"
#include "debug/Drain.hh"
#include "debug/ExecFaulting.hh"
#include "debug/MyMinorExecute.hh"
#include "debug/MyMinorInterrupt.hh"
#include "debug/MyMinorMem.hh"
#include "debug/MyMinorTrace.hh"
#include "debug/PCEvent.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

ExecuteDummy::ExecuteDummy(const std::string &name_,
    MyMinorCPU &cpu_,
    const BaseMyMinorCPUParams &params,
    Latch<ForwardInstData>::Output inp_,
    Latch<ForwardInstData>::Input out_,
    std::vector<InputBuffer<ForwardInstData>> &next_stage_input_buffer) :
    Named(name_),
    inp(inp_),
    out(out_),
    cpu(cpu_),
    nextStageReserve(next_stage_input_buffer),
    outputWidth(params.executeInputWidth),
    processMoreThanOneInput(params.executeDummyCycleInput),
    executeDummyInfo(params.numThreads),
    threadPriority(0)
{

    if (params.executeDummyInputBufferSize < 1) {
        fatal("%s: executeInputBufferSize must be >= 1 (%d)\n", name_,
        params.executeDummyInputBufferSize);
    }

    /* Per-thread structures */
    for (ThreadID tid = 0; tid < params.numThreads; tid++) {
        std::string tid_str = std::to_string(tid);

        /* Input Buffers */
        inputBuffer.push_back(
        InputBuffer<ForwardInstData>(
            name_ + ".inputBuffer" + tid_str, "insts",
            params.executeDummyInputBufferSize));
    }
}

const ForwardInstData *
ExecuteDummy::getInput(ThreadID tid)
{
    /* Get a line from the inputBuffer to work with */
    if (!inputBuffer[tid].empty()) {
        const ForwardInstData &head = inputBuffer[tid].front();

        return (head.isBubble() ? NULL : &(inputBuffer[tid].front()));
    } else {
        return NULL;
  }
}

void
ExecuteDummy::popInput(ThreadID tid)
{
    if (!inputBuffer[tid].empty())
        inputBuffer[tid].pop();

    executeDummyInfo[tid].inputIndex = 0;
}

void
ExecuteDummy::evaluate()
{
    /* Push input onto appropriate input buffer */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].setTail(*inp.outputWire);

    ForwardInstData &insts_out = *out.inputWire;

    assert(insts_out.isBubble());

    for (ThreadID tid = 0; tid < cpu.numThreads; tid++)
        executeDummyInfo[tid].blocked = !nextStageReserve[tid].canReserve();

    ThreadID tid = getScheduledThread();

    if (tid != InvalidThreadID) {
        ExecuteDummyThreadInfo &executeDummy_info = executeDummyInfo[tid];
        const ForwardInstData *insts_in = getInput(tid);

        unsigned int output_index = 0;

        while (insts_in &&
           executeDummy_info.inputIndex < insts_in->width() && /* Still more input */
           output_index < outputWidth /* Still more output to fill */)
        {
            MyMinorDynInstPtr inst = insts_in->insts[executeDummy_info.inputIndex];

            if (inst->isBubble()) {
                /* Skip */
                executeDummy_info.inputIndex++;
            } else {
                StaticInstPtr static_inst = inst->staticInst;
                /* Static inst of a macro-op above the output_inst */
                StaticInstPtr parent_static_inst = NULL;
                MyMinorDynInstPtr output_inst = inst;

                /* Pass on instruction */
                DPRINTF(MyMinorExecute, "Execute dummy: Passing on inst: %s inputIndex:"
                    " %d output_index: %d\n",
                    *output_inst, executeDummy_info.inputIndex, output_index);

                parent_static_inst = static_inst;

                /* Step input */
                executeDummy_info.inputIndex++;

                /* Step to next sequence number */
                executeDummy_info.execSeqNum++;

                /* Set execSeqNum of output_inst */
                output_inst->id.execSeqNum = executeDummy_info.execSeqNum;

                /* Correctly size the output before writing */
                if (output_index == 0) insts_out.resize(outputWidth);

                /* Push into output */
                insts_out.insts[output_index] = output_inst;
                output_index++;
            }

            /* Have we finished with the input? */
            if (executeDummy_info.inputIndex == insts_in->width()) {
                /* If we have just been producing micro-ops, we *must* have
                 * got to the end of that for inputIndex to be pushed past
                 * insts_in->width() */
                popInput(tid);
                insts_in = NULL;

                if (processMoreThanOneInput) {
                    DPRINTF(MyMinorExecute, "Execute dummy: Wrapping\n");
                    insts_in = getInput(tid);
                }
            }
        } // end while
    }

    /* If we generated output, reserve space for the result in the next stage
     *  and mark the stage as being active this cycle */
    if (!insts_out.isBubble()) {
        /* Note activity of following buffer */
        cpu.activityRecorder->activity();
        insts_out.threadId = tid;
        nextStageReserve[tid].reserve();
    }

    /* If we still have input to process and somewhere to put it,
     *  mark stage as active */
    for (ThreadID i = 0; i < cpu.numThreads; i++)
    {
        if (getInput(i) && nextStageReserve[i].canReserve()) {
            cpu.activityRecorder->activateStage(Pipeline::ExecuteDummyStageId);
            break;
        }
    }

    /* Make sure the input (if any left) is pushed */
    if (!inp.outputWire->isBubble())
        inputBuffer[inp.outputWire->threadId].pushTail();
}

void
ExecuteDummy::myminorTrace() const
{
    std::ostringstream data;

    if (executeDummyInfo[0].blocked)
        data << 'B';
    else
        (*out.inputWire).reportData(data);

    myminor::myminorTrace("insts=%s\n", data.str());
    inputBuffer[0].myminorTrace();
}

std::ostream &operator <<(std::ostream &os, ExecuteDummy::DrainState state)
{
    switch (state)
    {
        case ExecuteDummy::NotDraining:
            os << "NotDraining";
            break;
        case ExecuteDummy::DrainCurrentInst:
            os << "DrainCurrentInst";
            break;
        case ExecuteDummy::DrainHaltFetch:
            os << "DrainHaltFetch";
            break;
        case ExecuteDummy::DrainAllInsts:
            os << "DrainAllInsts";
            break;
        default:
            os << "Drain-" << static_cast<int>(state);
            break;
    }

    return os;
}

void
ExecuteDummy::setDrainState(ThreadID thread_id, DrainState state)
{
    DPRINTF(Drain, "setDrainState[%d]: %s\n", thread_id, state);
    executeDummyInfo[thread_id].drainState = state;
}

unsigned int
ExecuteDummy::drain()
{
    // DPRINTF(Drain, "MyMinorExecuteDummy drain\n");

    // for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
    //     if (executeDummyInfo[tid].drainState == NotDraining) {
    //     cpu.wakeupOnEvent(Pipeline::ExecuteDummyStageId);

    //     /* Go to DrainCurrentInst if we're between microops
    //         * or waiting on an unbufferable memory operation.
    //         * Otherwise we can go straight to DrainHaltFetch
    //         */
    //     setDrainState(tid, DrainCurrentInst);
    //     }
    // }
    // return (isDrained() ? 0 : 1);
    return 0;
}

inline ThreadID
ExecuteDummy::getScheduledThread()
{
    /* Select thread via policy. */
    std::vector<ThreadID> priority_list;

    switch (cpu.threadPolicy) {
      case enums::SingleThreaded:
        priority_list.push_back(0);
        break;
      case enums::RoundRobin:
        priority_list = cpu.roundRobinPriority(threadPriority);
        break;
      case enums::Random:
        priority_list = cpu.randomPriority();
        break;
      default:
        panic("Unknown fetch policy");
    }

    for (auto tid : priority_list) {
        if (getInput(tid) && !executeDummyInfo[tid].blocked) {
            threadPriority = tid;
            return tid;
        }
    }

   return InvalidThreadID;
}

bool
ExecuteDummy::isDrained()
{
    for (const auto &buffer : inputBuffer) {
        if (!buffer.empty())
            return false;
    }

    return (*inp.outputWire).isBubble();
}

void
ExecuteDummy::drainResume()
{
//     DPRINTF(Drain, "MyMinorExecuteDummy drainResume\n");

//     for (ThreadID tid = 0; tid < cpu.numThreads; tid++) {
//         setDrainState(tid, NotDraining);
//     }

//     cpu.wakeupOnEvent(Pipeline::ExecuteDummyStageId);
    return;
}

} // namespace myminor
} // namespace gem5