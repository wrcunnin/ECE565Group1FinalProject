/*
 * Copyright (c) 2012-2014, 2017 ARM Limited
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

#include "cpu/myminor/cpu.hh"

#include "cpu/myminor/dyn_inst.hh"
#include "cpu/myminor/fetch1.hh"
#include "cpu/myminor/pipeline.hh"
#include "debug/Drain.hh"
#include "debug/MyMinorCPU.hh"
#include "debug/Quiesce.hh"

namespace gem5
{

MyMinorCPU::MyMinorCPU(const BaseMyMinorCPUParams &params) :
    BaseCPU(params),
    threadPolicy(params.threadPolicy),
    stats(this)
{
    /* This is only written for one thread at the moment */
    myminor::MyMinorThread *thread;

    for (ThreadID i = 0; i < numThreads; i++) {
        if (FullSystem) {
            thread = new myminor::MyMinorThread(this, i, params.system,
                    params.mmu, params.isa[i], params.decoder[i]);
            thread->setStatus(ThreadContext::Halted);
        } else {
            thread = new myminor::MyMinorThread(this, i, params.system,
                    params.workload[i], params.mmu,
                    params.isa[i], params.decoder[i]);
        }

        threads.push_back(thread);
        ThreadContext *tc = thread->getTC();
        threadContexts.push_back(tc);
    }


    if (params.checker) {
        fatal("The MyMinor model doesn't support checking (yet)\n");
    }

    pipeline = new myminor::Pipeline(*this, params);
    activityRecorder = pipeline->getActivityRecorder();

    fetchEventWrapper = NULL;
}

MyMinorCPU::~MyMinorCPU()
{
    delete pipeline;

    if (fetchEventWrapper != NULL)
        delete fetchEventWrapper;

    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        delete threads[thread_id];
    }
}

void
MyMinorCPU::init()
{
    BaseCPU::init();

    if (!params().switched_out && system->getMemoryMode() != enums::timing) {
        fatal("The MyMinor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }
}

/** Stats interface from SimObject (by way of BaseCPU) */
void
MyMinorCPU::regStats()
{
    BaseCPU::regStats();
    pipeline->regStats();
}

void
MyMinorCPU::serializeThread(CheckpointOut &cp, ThreadID thread_id) const
{
    threads[thread_id]->serialize(cp);
}

void
MyMinorCPU::unserializeThread(CheckpointIn &cp, ThreadID thread_id)
{
    threads[thread_id]->unserialize(cp);
}

void
MyMinorCPU::serialize(CheckpointOut &cp) const
{
    pipeline->serialize(cp);
    BaseCPU::serialize(cp);
}

void
MyMinorCPU::unserialize(CheckpointIn &cp)
{
    pipeline->unserialize(cp);
    BaseCPU::unserialize(cp);
}

void
MyMinorCPU::wakeup(ThreadID tid)
{
    DPRINTF(Drain, "[tid:%d] MyMinorCPU wakeup\n", tid);
    assert(tid < numThreads);

    if (threads[tid]->status() == ThreadContext::Suspended) {
        threads[tid]->activate();
    }
}

void
MyMinorCPU::startup()
{
    DPRINTF(MyMinorCPU, "MyMinorCPU startup\n");

    BaseCPU::startup();

    for (ThreadID tid = 0; tid < numThreads; tid++)
        pipeline->wakeupFetch(tid);
}

DrainState
MyMinorCPU::drain()
{
    // Deschedule any power gating event (if any)
    deschedulePowerGatingEvent();

    if (switchedOut()) {
        DPRINTF(Drain, "MyMinor CPU switched out, draining not needed.\n");
        return DrainState::Drained;
    }

    DPRINTF(Drain, "MyMinorCPU drain\n");

    /* Need to suspend all threads and wait for Execute to idle.
     * Tell Fetch1 not to fetch */
    if (pipeline->drain()) {
        DPRINTF(Drain, "MyMinorCPU drained\n");
        return DrainState::Drained;
    } else {
        DPRINTF(Drain, "MyMinorCPU not finished draining\n");
        return DrainState::Draining;
    }
}

void
MyMinorCPU::signalDrainDone()
{
    DPRINTF(Drain, "MyMinorCPU drain done\n");
    Drainable::signalDrainDone();
}

void
MyMinorCPU::drainResume()
{
    /* When taking over from another cpu make sure lastStopped
     * is reset since it might have not been defined previously
     * and might lead to a stats corruption */
    pipeline->resetLastStopped();

    if (switchedOut()) {
        DPRINTF(Drain, "drainResume while switched out.  Ignoring\n");
        return;
    }

    DPRINTF(Drain, "MyMinorCPU drainResume\n");

    if (!system->isTimingMode()) {
        fatal("The MyMinor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    for (ThreadID tid = 0; tid < numThreads; tid++){
        wakeup(tid);
    }

    pipeline->drainResume();

    // Reschedule any power gating event (if any)
    schedulePowerGatingEvent();
}

void
MyMinorCPU::memWriteback()
{
    DPRINTF(Drain, "MyMinorCPU memWriteback\n");
}

void
MyMinorCPU::switchOut()
{
    DPRINTF(MyMinorCPU, "MyMinorCPU switchOut\n");

    assert(!switchedOut());
    BaseCPU::switchOut();

    /* Check that the CPU is drained? */
    activityRecorder->reset();
}

void
MyMinorCPU::takeOverFrom(BaseCPU *old_cpu)
{
    DPRINTF(MyMinorCPU, "MyMinorCPU takeOverFrom\n");

    BaseCPU::takeOverFrom(old_cpu);
}

void
MyMinorCPU::activateContext(ThreadID thread_id)
{
    DPRINTF(MyMinorCPU, "ActivateContext thread: %d\n", thread_id);

    /* Do some cycle accounting.  lastStopped is reset to stop the
     *  wakeup call on the pipeline from adding the quiesce period
     *  to BaseCPU::numCycles */
    stats.quiesceCycles += pipeline->cyclesSinceLastStopped();
    pipeline->resetLastStopped();

    /* Wake up the thread, wakeup the pipeline tick */
    threads[thread_id]->activate();
    wakeupOnEvent(myminor::Pipeline::CPUStageId);

    if (!threads[thread_id]->getUseForClone())//the thread is not cloned
    {
        pipeline->wakeupFetch(thread_id);
    } else { //the thread from clone
        if (fetchEventWrapper != NULL)
            delete fetchEventWrapper;
        fetchEventWrapper = new EventFunctionWrapper([this, thread_id]
                  { pipeline->wakeupFetch(thread_id); }, "wakeupFetch");
        schedule(*fetchEventWrapper, clockEdge(Cycles(0)));
    }

    BaseCPU::activateContext(thread_id);
}

void
MyMinorCPU::suspendContext(ThreadID thread_id)
{
    DPRINTF(MyMinorCPU, "SuspendContext %d\n", thread_id);

    threads[thread_id]->suspend();

    BaseCPU::suspendContext(thread_id);
}

void
MyMinorCPU::wakeupOnEvent(unsigned int stage_id)
{
    DPRINTF(Quiesce, "Event wakeup from stage %d\n", stage_id);

    /* Mark that some activity has taken place and start the pipeline */
    activityRecorder->activateStage(stage_id);
    pipeline->start();
}

Port &
MyMinorCPU::getInstPort()
{
    return pipeline->getInstPort();
}

Port &
MyMinorCPU::getDataPort()
{
    return pipeline->getDataPort();
}

Counter
MyMinorCPU::totalInsts() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numInst;

    return ret;
}

Counter
MyMinorCPU::totalOps() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numOp;

    return ret;
}

} // namespace gem5
