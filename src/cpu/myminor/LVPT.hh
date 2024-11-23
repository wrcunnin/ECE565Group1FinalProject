namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

/** Execute stage.  Everything apart from fetching and decoding instructions.
 *  The LSQ lives here too. */
class LVPT : public Named
{

protected:
    unsigned int tableSize;
    unsigned int inPC;
    unsigned int outVal;
    unsigned int tableHit;

    struct tableEntry
    {
        unsigned int PC;
        unsigned int value;
    }

    tableEntry table[tablesize];

    /** Table access here for given PC */
    void evaluate();

}

}
}