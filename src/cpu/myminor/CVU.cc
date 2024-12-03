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

CVU::CVU(const BaseMyMinorCPUParams &params,
  unsigned int inPC_,
  unsigned int inAddr_,
  unsigned int inIndexLVPT_,,
  bool constant_,
  bool isStore_,
  unsigned int outValue_,
  bool verifyPrediction_) :
  tableSize(params.tableSize),
  threshold(params.thresholdLCT),
  maxValue(params.maxValueLCT),
  inPC(inPC_),
  inAddr(inAddr_),
  inIndexLVPT(inIndexLVPT_),
  constant(constant_),
  isStore(isStore_),
  outValue(outValue_),
  verifyPrediction(verifyPrediction_)
{

}


void
CVU::storeInvalidate(unsigned int address)
{
  for (hitIndex = 0; hitIndex < tableSize; hitIndex++) {
      tableEntry entry = cvuTable[hitIndex];
      if(entry.addr == address){
          entry.valid = false;
      }
}



void
CVU::verifyEntryInCVU(unsigned int address, unsigned int index)
{
  // declarations
  unsigned int hitIndex = tableSize;

  // go through each frame
  if (constant) {
    for (hitIndex = 0; hitIndex < tableSize; hitIndex++) {
      tableEntry entry = cvuTable[hitIndex];
      if (entry.valid && (entry.addr == inAddr) && (entry.index == inIndexLVPT)) {
        break;
      }
    }
  }

  verifyPrediction = hitIndex < tableSize;
}

}
}