#ifndef SYNCEFFECT_H
#define SYNCEFFECT_H

#include "SyncTime.h"

class SyncEffect : public SyncTime {
    public:
        SyncEffect(unsigned long timeOffset, Effect *effect, unsigned int length) : SyncTime(timeOffset, effect, length) { };

        Effect* syncEffect(Effect *effect) {
            return setEffect(effect);
        };
};

#endif
