#include "cpu/myminor/CVU.hh"

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

CVU::CVU(const BaseMyMinorCPUParams &params) :
  tableSize(params.tableSize),
  threshold(params.thresholdLCT),
  maxValue(params.maxValueLCT),
{

}

void
CVU::storeInvalidate(unsigned int address)
{
  for (unsigned int hitIndex = 0; hitIndex < tableSize; hitIndex++) {
    tableEntry entry = cvuTable[hitIndex];
    if(entry.addr == address){
        entry.valid = false;
    }
  }
}

bool
CVU::verifyEntryInCVU(unsigned int address, unsigned int index, bool constant)
{
  // declarations
  unsigned int hitIndex = tableSize;

  // go through each frame
  if (constant) {
    for (hitIndex = 0; hitIndex < tableSize; hitIndex++) {
      tableEntry entry = cvuTable[hitIndex];
      if (entry.valid && (entry.addr == address) && (entry.index == index)) {
        break;
      }
    }
  }

  return hitIndex < tableSize;
}

}
}