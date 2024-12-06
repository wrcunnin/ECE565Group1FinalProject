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
        unsigned int value;
    };

public:
    struct lvptData
    {
        bool constant;
        bool predict;
        unsigned int value;
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

    virtual ~LVPT();

    void updateTable(unsigned int data, unsigned int addr, unsigned int pc, bool predict);

    /** Table access here for given PC */
    void read(unsigned int pc, bool& outDataPredict, bool& outDataConstant, unsigned int& outDataValue,
        unsigned int& outDataPC, unsigned int& outDataIndex, unsigned int& outDataAddr);

};

}
}

#endif /* __CPU_MYMINOR_LVPT_HH__ */