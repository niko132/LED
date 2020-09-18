#ifndef MAGIC_H
#define MAGIC_H

const unsigned char MAGIC[4] = { 0x92, 0xB3, 0x1F, 0x12 };
const unsigned char MAGIC_LENGTH = 4;

class MagicReader {
    private:
        unsigned char *_data = NULL;
        unsigned int _length = 0;

        unsigned int _currentPos = 0;

        bool checkMagicIfNeeded() {
            if (_length < MAGIC_LENGTH) {
                return false;
            }

            if (_currentPos == 0) {
                if (memcmp(&_data[_currentPos], &MAGIC, MAGIC_LENGTH) == 0) {
                    _currentPos += MAGIC_LENGTH;
                    return true;
                } else {
                    return false;
                }
            }

            return true;
        };

    public:
        MagicReader(unsigned char *data, unsigned int length) {
            _data = data;
            _length = length;

            _currentPos = 0;
        };

        bool read(unsigned char *buf, unsigned int length) {
            if (checkMagicIfNeeded() && _currentPos + length <= _length) {
                memcpy(buf, &_data[_currentPos], length);
                _currentPos += length;

                return true;
            }

            return false;
        };

        template<typename T>
        bool read(T *buf) {
            return read((unsigned char*) buf, sizeof(T));
        };

        // template<>
        bool read(String *buf) {
            (*buf) = "";
            char c = 1; // use non 0 value

            while (read<char>(&c) && c != 0) {
                (*buf) += c;
            }

            return c == 0;
        };

        unsigned char* getRemainingData(unsigned int *length) {
            *length = _length - _currentPos;
            return &_data[_currentPos];
        };
};

class MagicWriter {
    private:
        unsigned char *_buf = NULL;
        unsigned int _length = 0;

        unsigned int _currentPos = 0;

        bool reallocate(unsigned int length) {
            if (_length < length) {
                unsigned char *tmpBuf = new unsigned char[length];

                if (tmpBuf == NULL) {
                    return false;
                }

                if (_buf) {
                    memcpy(tmpBuf, _buf, _length);
                    delete[] _buf;
                }

                _buf = tmpBuf;
                _length = length;
            }

            return true;
        };

        bool reallocateIfNeeded(unsigned int dataLength) {
            if (_currentPos + dataLength > _length) {
                return reallocate(_length * 2);
            }

            return true;
        };

        bool writeMagicIfNeeded() {
            if (_currentPos == 0) {
                if (!reallocateIfNeeded(MAGIC_LENGTH)) {
                    return false;
                }

                memcpy(&_buf[_currentPos], &MAGIC, MAGIC_LENGTH);
                _currentPos += MAGIC_LENGTH;
            }

            return true;
        };

    public:
        MagicWriter(unsigned int length = 32) {
            _currentPos = 0;
            reallocate(length);
        };

        ~MagicWriter() {
            if (_buf) {
                delete[] _buf;
                _buf = NULL;
            }
        };

        bool write(unsigned char *data, unsigned int length) {
            if (writeMagicIfNeeded() && reallocateIfNeeded(length)) {
                memcpy(&_buf[_currentPos], data, length);
                _currentPos += length;

                return true;
            }

            return false;
        };

        template<typename T>
        bool write(T data) {
            return write((unsigned char*) &data, sizeof(T));
        };

        // template<>
        bool write(String buf) {
            return write((unsigned char*) buf.c_str(), buf.length() + 1); // write c-string with \0 terminated end
        };

        unsigned char* getData(unsigned int *length) {
            *length = _currentPos;
            return _buf;
        };
};

#endif
