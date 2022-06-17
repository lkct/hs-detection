#include "SpikeSaver.h"

using namespace std;

namespace HSDetection
{
    SpikerSaver::SpikerSaver(vector<char> *pResult) : pResult(pResult) {}

    SpikerSaver::~SpikerSaver() {}

    void SpikerSaver::operator()(Spike *pSpike)
    {
        const int bufSize = 3 * sizeof(int) + 2 * sizeof(float);
        char buf[bufSize];
        *(int *)(buf) = pSpike->channel;
        *(int *)(buf + sizeof(int)) = pSpike->frame;
        *(int *)(buf + sizeof(int) * 2) = pSpike->amplitude;
        *(float *)(buf + sizeof(int) * 3) = pSpike->position.x;
        *(float *)(buf + sizeof(int) * 3 + sizeof(float)) = pSpike->position.y;

        pResult->insert(pResult->end(), buf, buf + bufSize);
    }

} // namespace HSDetection
