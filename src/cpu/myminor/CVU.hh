namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

class CVU : public Named
{

protected:
    unsigned int tableSize;
    unsigned int threshold;
    unsigned int maxValue;
    unsigned int inPC;
    unsigned int inAddr;
    unsigned int inIndexLVPT;
    bool constant;
    bool isStore;

    unsigned int outVal;
    bool verifyPrediction;

    unsigned int mispredValue;
    unsigned int mispredPC;

    struct tableEntry
    {
        bool         valid;
        unsigned int addr;
        unsigned int index;
        unsigned int data;
    }

    std::vector<tableEntry> cvuTable (tableSize, 0);

    void storeInvalidate();
    // Invalidate all matching addresses on a store

    /** Table access here for given PC */
    void evaluate();

}
}
}