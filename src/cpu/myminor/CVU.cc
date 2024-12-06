#include "cpu/myminor/CVU.hh"

#include <functional>
// #include <stdlib>
#include <ctime>

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

CVU::CVU(unsigned int cvu_table_size,
        unsigned int lct_threshold,
        unsigned int lct_maxvalue) :
  tableSize(cvu_table_size),
  threshold(lct_threshold),
  maxValue(lct_maxvalue),
  InvalidEntries(),
  cvuTable()
{
  cvuTable.resize(cvu_table_size);

  for (unsigned int i = 0; i < tableSize; i++) {
    InvalidEntries.push_back(i);
  }
}

void
CVU::storeInvalidate(unsigned int address)
{
  for (unsigned int hitIndex = 0; hitIndex < tableSize; hitIndex++) {
    tableEntry entry = cvuTable[hitIndex];
    if(entry.addr == address){
        entry.valid = false;
        AddToInvalidateList(hitIndex);
        DPRINTF(LVP, "Adding value to Invalidate list %u", hitIndex);
    }
  }
}

void
CVU::AddToInvalidateList(unsigned int TableIndex){
  InvalidEntries.push_back(TableIndex);
}

void
CVU::AddEntryToCVU(unsigned long data, unsigned int LVPT_Index, unsigned int Translated_Data_Address){

  tableEntry newEntry = {true, Translated_Data_Address, LVPT_Index, data};
  unsigned int new_entry_index;

  bool random = false;

  if(InvalidEntries.empty()){ // If there are no invalid entries
    // Pick random entry to remove from the CVU and fill slot with new data
    std::srand(std::time(0));
    new_entry_index = std::rand() % (tableSize + 1);
    random = true;
  } else { //If invalid entries then pick first one 
    new_entry_index = InvalidEntries.front();
    InvalidEntries.pop_front();
  }

  cvuTable[new_entry_index] = newEntry;
  DPRINTF(LVP, "\nAdding new entry to CVU at index: %u using random %d", new_entry_index, random);
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