#ifndef __CPU_MYMINOR_LVPT_HH__
#define __CPU_MYMINOR_LVPT_HH__

#include <vector>

#include "arch/generic/mmu.hh"
#include "base/named.hh"
#include "cpu/base.hh"
#include "params/BaseMyMinorCPU.hh"


namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

/** Execute stage.  Everything apart from fetching and decoding instructions.
 *  The LSQ lives here too. */
class LVPT
{
public:
    unsigned int tableSize;
    unsigned int threshold;
    unsigned int maxValue;

    struct tableEntry
    {
        bool         valid;
        unsigned int tag;
        unsigned int addr;
        unsigned long value;
    };

public:
    struct lvptData
    {
        bool constant;
        bool predict;
        unsigned long value;
        unsigned int pc;
        unsigned int addr;
        unsigned int index;
    };

    // std::vector<tableEntry> valueTable (tableSize, 0);
    std::vector<tableEntry> valueTable;

    // std::vector<unsigned int> predictTable (tableSize / 4, 0);
    std::vector<unsigned int> predictTable;

public:
    LVPT(const BaseMyMinorCPUParams &params);

    // virtual ~LVPT();

    void updateTable(unsigned long data, unsigned int addr, unsigned int pc, bool predict, bool constant);

    /** Table access here for given PC */
    void read(unsigned int pc, bool& outDataPredict, bool& outDataConstant, unsigned long& outDataValue,
        unsigned int& outDataPC, unsigned int& outDataIndex, unsigned int& outDataAddr, unsigned int& outDataCounter);

};

}
}

#endif /* __CPU_MYMINOR_LVPT_HH__ */