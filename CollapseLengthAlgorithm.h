#include "CoverAlgorithm.h"

#include <vector>

class CollapseLengthAlgorithm : public CoverAlgorithm {
    private:
        std::vector<unsigned int*> _subIndices;

    public:
        CollapseLengthAlgorithm(unsigned int startIndex, unsigned int endIndex) : CoverAlgorithm(startIndex, endIndex) {

        }

        unsigned int getLedCount() {
            unsigned int sum = 0;

        	for (int i = 0; i < _subIndices.size(); i++) {
        		sum += _subIndices[i][1] - _subIndices[i][0];
        	}

        	return sum;
        };

        void resetCovered() {
            for (int i = 0; i < _subIndices.size(); i++) {
                delete[] _subIndices[i];
                _subIndices[i] = NULL;
            }

            _subIndices.clear();
            _subIndices.push_back(new unsigned int[2] {_startIndex, _endIndex});
        };

        void addCovered(unsigned int startIndex, unsigned int endIndex) {
            for (int i = _subIndices.size() - 1; i >= 0; i--) {
                if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][0] && endIndex < _subIndices[i][1]) {
                    _subIndices[i][0] = endIndex;
                } else if (endIndex >= _subIndices[i][1] && startIndex <= _subIndices[i][1] && startIndex > _subIndices[i][0]) {
                    _subIndices[i][1] = startIndex;
                } else if (startIndex > _subIndices[i][0] && endIndex < _subIndices[i][1]) {
        			unsigned int end = _subIndices[i][1];
        			_subIndices[i][1] = startIndex;

                    unsigned int *b = new unsigned int[2] {endIndex, end};
                    _subIndices.insert(_subIndices.begin() + i + 1, b);
                } else if (startIndex <= _subIndices[i][0] && endIndex >= _subIndices[i][1]) {
                    unsigned int *tmp = _subIndices[i];
        			_subIndices.erase(_subIndices.begin() + i);

        			delete[] tmp;
                }
            }
        };

        virtual bool nextIndex(int currIndex, int *nextIndex, double *nextFrac) {
            unsigned int count = getLedCount();

            int tmpNextIndex = currIndex + 1;
            unsigned int nextIndexCount = 0;

            for (int i = 0; i < _subIndices.size(); i++) {
                if (tmpNextIndex >= _subIndices[i][0] && tmpNextIndex < _subIndices[i][1]) {
                    nextIndexCount += tmpNextIndex - _subIndices[i][0];
                    break;
                }

                if (_subIndices[i][0] >= tmpNextIndex) {
                    tmpNextIndex = _subIndices[i][0];
                    break;
                }

                nextIndexCount += _subIndices[i][1] - _subIndices[i][0];
            }

            if (tmpNextIndex < _endIndex) {
                (*nextIndex) = tmpNextIndex;
                (*nextFrac) = (double) nextIndexCount / count;
                return true;
            }

            return false;
        };
};
