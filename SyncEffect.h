#ifndef SYNCEFFECT_H
#define SYNCEFFECT_H

#include "SyncTime.h"

class SyncEffect : public SyncTime {
    public:
        Effect* syncEffect(Effect *effect) {
            return setEffect(effect);
        };
};

#endif
