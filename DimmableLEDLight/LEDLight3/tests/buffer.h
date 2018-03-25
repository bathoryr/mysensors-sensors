template <class T>
class buffer {
    T* _buf;
    int _head;
    int _size;

    public:
    buffer(int size = 32) {
        _buf = new T[size];
        _head = 0;
        _size = size;
    }

    ~buffer() {
        delete [] _buf;
    }

    void add(T item) {
        _buf[_head] = item;
        if (++_head >= _size)
            _head = 0;
    }

    int GetAvgVal() {
        T val = 0;
        for(int i = 0; i < _size; i++) {
            val += _buf[i];
        }
        return val / _size;
    }

    T get(int pos) {
        return _buf[pos];
    }

    int size() {
        return _size;
    }

    int head() {
        return _head;
    }


};
