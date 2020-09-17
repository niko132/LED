#ifndef COVERALGORITHM_H
#define COVERALGORITHM_H

class CoverAlgorithm {
    protected:
        unsigned int _startIndex = 0;
        unsigned int _endIndex = 0;

    public:
        CoverAlgorithm(unsigned int startIndex, unsigned int endIndex) {
            _startIndex = startIndex;
            _endIndex = endIndex;
        };

        unsigned int getStartIndex() {
            return _startIndex;
        };

        void setStartIndex(unsigned int startIndex) {
            _startIndex = startIndex;
        };

        unsigned int getEndIndex() {
            return _endIndex;
        };

        void setEndIndex(unsigned int endIndex) {
            _endIndex = endIndex;
        };

        virtual unsigned int getLedCount() {
            return _endIndex - _startIndex;
        };

        virtual void resetCovered() = 0;
        virtual void addCovered(unsigned int startIndex, unsigned int endIndex) = 0;

        virtual bool nextIndex(int currIndex, int *nextIndex, double *nextFrac) {
            unsigned int count = getLedCount();

            int tmpNextIndex = currIndex + 1;

            if (tmpNextIndex < _startIndex) {
                tmpNextIndex = _startIndex;
            }

            if (tmpNextIndex < _endIndex) {
                (*nextIndex) = tmpNextIndex;
                (*nextFrac) = (double) (tmpNextIndex - _startIndex) / count;
                return true;
            }

            return false;
        };
};

#endif
