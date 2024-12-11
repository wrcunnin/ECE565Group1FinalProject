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
#include "debug/LVP.hh"

namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

LVPT::LVPT(const BaseMyMinorCPUParams &params, MyMinorCPU &cpu_) :
  tableSize(params.tableSizeLVPT),
  threshold(params.thresholdLCT),
  maxValue(params.maxValueLCT),
  cpu(cpu_),
  valueTable(),
  predictTable()
{
  if (threshold > maxValue)
    fatal("LVPT: thresholdLCT (%d) must be <= maxValueLCT (%d)", threshold, maxValue);

  valueTable.resize(tableSize);
  predictTable.resize(tableSize / 4);
  
}

void
LVPT::updateTable(unsigned long data, unsigned long addr, unsigned long pc, bool predict, bool constant)
{
  // value table
  unsigned long numIndexBits = log2(tableSize);
  unsigned long numLCTBits = numIndexBits - 2;
  unsigned long indexMask = ((0x1 << numIndexBits) - 1);
  unsigned long index = (pc) & indexMask;
  // unsigned long indexLCT = pc >> (32 - numLCTBits);
  unsigned long indexLCT = ((pc) & indexMask) >> 2;
  unsigned long tag = (pc) & ~indexMask;

  tableEntry entry = valueTable[index];
  unsigned int prediction = predictTable[indexLCT];

  // if the predict counter should change
  if (constant)
  {
    // on a mispredict
    if (!predict)
    {
      // update LCT
      DPRINTF(LVP, "\nDecreasing LCT value to %u ", prediction == 0 ? 0 : prediction - 1);
      cpu.stats.lvptInCorrectPred++;
      predictTable[indexLCT] = prediction == 0 ? 0 : prediction - 1;

      // update LVPT
      entry.valid = true;
      entry.value = data;
      entry.addr  = addr;
      entry.tag   = tag;
      if(valueTable[index].tag != entry.tag || valueTable[index].valid == false){
        cpu.stats.lvptTableAdditions++;
      }
      valueTable[index] = entry;

    }
    else
    {
      // update LCT
      predictTable[indexLCT] = prediction >= maxValue ? prediction : prediction + 1;
      DPRINTF(LVP, "\nIncreasing LCT value to %u ", prediction >= maxValue ? prediction : prediction + 1);
      cpu.stats.lvptCorrectPred++;
    }
  }
}

void
LVPT::read(unsigned long pc, bool& outDataPredict, bool& outDataConstant, unsigned long& outDataValue,
           unsigned long& outDataPC, unsigned long& outDataIndex, unsigned long& outDataAddr, unsigned int& outDataCounter)
{
  // value table
  unsigned long numIndexBits = log2(tableSize);
  unsigned long numLCTBits = numIndexBits - 2;
  unsigned long indexMask = ((0x1 << numIndexBits) - 1);
  unsigned long index = (pc) & indexMask;
  // unsigned long indexLCT = pc >> (32 - numLCTBits);
  unsigned long indexLCT = ((pc) & indexMask) >> 2;
  unsigned long tag = (pc) & ~indexMask;

  tableEntry entry = valueTable[index];
  unsigned int predict = predictTable[indexLCT];

  // check if valid and right PC
  // lvptData outData = {0};
  // if entry in table is valid and matches
  // if prediction is above the threshold
  outDataPredict = false;
  outDataValue   = 0;
  outDataPC      = 0;
  outDataIndex   = 0;
  outDataAddr    = 0;
  outDataConstant = false;
  outDataCounter = 0;

  if (entry.valid && entry.tag == tag)
  {
    // if prediction is above the threshold
    outDataPredict = true;
    outDataValue   = entry.value;
    outDataPC      = pc;
    outDataIndex   = index;
    outDataAddr    = entry.addr;
    outDataCounter = predict;
    if (predict >= threshold) {
      outDataConstant = true;
    }
  }
}

}
}