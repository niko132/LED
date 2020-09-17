#ifndef SYNCALGORITHM_H
#define SYNCALGORITHM_H

class SyncAlgorithm {
    protected:
        unsigned long _timeOffset = 0;
        Effect *_effect = NULL;
        unsigned int _length = 0;

    public:
        SyncAlgorithm(unsigned long timeOffset, Effect *effect, unsigned int length) {
            _timeOffset = timeOffset;
            _effect = effect;
            _length = length;
        };

        unsigned long getTimeOffset() {
            return _timeOffset;
        };

        void setTimeOffset(unsigned long timeOffset) {
            _timeOffset = timeOffset;
        };

        Effect* getEffect() {
            return _effect;
        };

        Effect* setEffect(Effect *effect) {
            Effect *tmp = _effect;
            _effect = effect;

            return tmp;
        };

        virtual void setLength(unsigned int length) {
            _length = length;
        };

        virtual void syncTimeOffset(unsigned long timeOffset) { };
        virtual Effect* syncEffect(Effect *effect) {
            return effect; // returned value gets deleted
        };
        virtual void syncPixelData(unsigned char *pixelData, unsigned int length) { };

        virtual CRGB updatePixel(double timeValue, double posValue, unsigned int relIndex, Effect *effect) {
            if (_effect) {
                return _effect->update(timeValue, posValue);
            }

            return CRGB(0, 0, 0);
        };
};

#endif
