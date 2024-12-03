#include "cpu/myminor/LVPT.hh"

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

LVPT::LVPT(const BaseMyMinorCPUParams &params,
  unsigned int inPC_,
  unsigned int outValue_,
  unsigned int outIndexLVPT_,
  bool valuePredict_,
  bool constant_) :
  tableSize(params.tableSize),
  threshold(params.thresholdLCT),
  maxValue(params.maxValueLCT),
  inPC(inPC_),
  outValue(outValue_),
  outIndexLVPT(outIndexLVPT_),
  valuePredict(valuePredict_),
  constant(constant_)
{
  if (threshold > maxValue)
    fatal("LVPT: thresholdLCT (%d) must be <= maxValueLCT (%d)", threshold, maxValue);
}

void
LVPT::updateTable(unsigned int data, unsigned int addr, unsigned int pc, bool mispredict)
{
  // value table
  unsigned int numIndexBits = log2(tableSize);
  unsigned int numLCTBits = numIndexBits - 2;
  unsigned int indexMask = ((0x1 << numIndexBits) - 1);
  unsigned int index = (pc) & indexMask;
  unsigned int indexLCT = pc >> (32 - numLCTBits);
  unsigned int tag = (pc) & ~indexMask;

  tableEntry entry = valueTable[index];
  unsigned int predict = predictTable[indexLCT];

  // on store
  if (store)
  {
    // drop for now
  }
  else if (mispredict)
  {
    // update LCT
    predictTable[indexLCT] = predict == 0 ? 0 : predict - 1;

    // update LVPT
    entry.valid = true;
    entry.value = data;
    entry.addr  = addr;
    valueTable[index] = entry;

  }
  else
  {
    // update LCT
    predictTable[indexLCT] = predict >= maxValue ? predict : predict + 1;
  }
}

void
LVPT::evaluate()
{
  // value table
  unsigned int numIndexBits = log2(tableSize);
  unsigned int numLCTBits = numIndexBits - 2;
  unsigned int indexMask = ((0x1 << numIndexBits) - 1);
  unsigned int index = (inPC) & indexMask;
  unsigned int indexLCT = inPC >> (32 - numLCTBits);
  unsigned int tag = (inPC) & ~indexMask;

  tableEntry entry = valueTable[index];
  unsigned int predict = predictTable[indexLCT];

  // check if valid and right PC
  outVal = 0;
  outIndexLVPT = 0;
  valuePredict = false;
  constant = false;
  // if entry in table is valid and matches
  if (entry.valid && entry.tag == tag)
  {
    // if prediction is above the threshold
    outVal = entry.value;
    outIndexLVPT = index;
    valuePredict = true;
    if (predict >= threshhold) {
      constant = true;
    }
  }
}

}
}