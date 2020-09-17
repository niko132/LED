#ifndef SYNCTIME_H
#define SYNCTIME_H

#include "SyncAlgorithm.h"

class SyncTime : public SyncAlgorithm {
    public:
        SyncTime(unsigned long timeOffset, Effect *effect, unsigned int length) : SyncAlgorithm(timeOffset, effect, length) {

        };

        void syncTimeOffset(unsigned long timeOffset) {
            setTimeOffset(timeOffset);
        };
};

#endif
