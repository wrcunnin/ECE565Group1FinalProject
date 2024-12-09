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
CVU::storeInvalidate(unsigned long vaddr, unsigned long paddr)
{
  for (unsigned int hitIndex = 0; hitIndex < tableSize; hitIndex++) {
    tableEntry entry = cvuTable[hitIndex];
    if(entry.vaddr == vaddr || entry.paddr == paddr){
        entry.valid = false;
        cvuTable[hitIndex].valid = false; // I am so stupid!!!
        printCVUEntries();
        AddToInvalidateList(hitIndex);
        DPRINTF(LVP, "Adding value to Invalidate list %u\t vaddr: 0x%x\t paddr: 0x%x", hitIndex, vaddr, paddr);
    }
  }
}

void
CVU::AddToInvalidateList(unsigned int TableIndex){
  InvalidEntries.push_back(TableIndex);
}

void
CVU::AddEntryToCVU(unsigned long data, unsigned long LVPT_Index,  unsigned long vaddr, unsigned long paddr){

  // tableEntry newEntry = {.valid=true, Translated_Data_Address, LVPT_Index, data};
  tableEntry newEntry;
  newEntry.valid = true;
  newEntry.vaddr  = vaddr;
  newEntry.paddr  = paddr;
  newEntry.index = LVPT_Index;
  newEntry.data  = data;
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
  DPRINTF(LVP, "\nAdding new entry to CVU at index: %u using random %d, InvalidEntries Size: %d", new_entry_index, random, InvalidEntries.size());
}

bool
CVU::verifyEntryInCVU(unsigned long address, unsigned long index, unsigned long data, bool constant, unsigned long &paddr)
{
  // declarations
  unsigned int hitIndex = tableSize;

  // go through each frame
  if (constant) {
    for (hitIndex = 0; hitIndex < tableSize; hitIndex++) {
      tableEntry entry = cvuTable[hitIndex];
      if (entry.valid)
        DPRINTF(LVP, "\n----verifyEntryInCVU::LOOP----\nhitIndex: %d\nentry.valid: %d\nentry.vaddr: 0x%x\nentry.index: 0x%x\n", hitIndex, entry.valid ? 1 : 0, entry.vaddr, entry.index);
      if (entry.valid && (entry.vaddr == address) && (entry.index == index) && (entry.data == data)) {
        paddr = entry.paddr;
        break;
      }
    }
  }

  DPRINTF(LVP, "\n----verifyEntryInCVU----\nhitIndex: %d\naddress: 0x%x\nindex: 0x%x\nconstant: %d\n", hitIndex, address, index, constant ? 1 : 0);

  return hitIndex < tableSize;
}

void
CVU::printCVUEntries(){
  unsigned int hitIndex = tableSize;

  for (hitIndex = 0; hitIndex < tableSize; hitIndex++) {
    tableEntry entry = cvuTable[hitIndex];
    DPRINTF(LVP, "\nIndex: %u\tvalid: %u\tvaddr: 0x%x\tpaddr: 0x%x\tindex: 0x%x\tdata: 0x%x ", hitIndex, entry.valid, entry.vaddr, entry.paddr, entry.index, entry.data);
  }
}

}
}