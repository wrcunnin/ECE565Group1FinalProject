namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(MyMinor, myminor);
namespace myminor
{

/** Execute stage.  Everything apart from fetching and decoding instructions.
 *  The LSQ lives here too. */
class LVPT : public Named
{


unsigned int tableSize;

struct tableEntry
{
    int32_t PC;
    int32_t value;
}

tableEntry table[tablesize];

}

}
}