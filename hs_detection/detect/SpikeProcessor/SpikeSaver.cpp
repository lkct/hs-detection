#include "SpikeSaver.h"

using namespace std;

namespace HSDetection
{
    SpikerSaver::SpikerSaver(vector<char> *resultBuffer) : resultBuffer(resultBuffer) {}

    SpikerSaver::~SpikerSaver() {}

    void SpikerSaver::operator()(Spike *pSpike)
    {
        const int bufSize = sizeof(int) * 3 + sizeof(float) * 2;
        char buf[bufSize];
        *(int *)(buf) = pSpike->frame;
        *(int *)(buf + sizeof(int)) = pSpike->channel;
        *(int *)(buf + sizeof(int) * 2) = pSpike->amplitude;
        *(float *)(buf + sizeof(int) * 3) = pSpike->position.x;
        *(float *)(buf + sizeof(int) * 3 + sizeof(float)) = pSpike->position.y;

        resultBuffer->insert(resultBuffer->end(), buf, buf + bufSize);
    }

} // namespace HSDetection
