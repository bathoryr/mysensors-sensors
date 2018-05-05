template <class T, int SIZE = 32>
class buffer {
    T _buf[SIZE];
    int _head;
    int _size;

    public:
    buffer() {
        _head = 0;
        _size = SIZE;
    }

    ~buffer() {
    }

    void add(T item) {
        _buf[_head] = item;
        if (++_head >= _size)
            _head = 0;
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

    T GetAvgVal() {
        T val = 0;
        for(int i = 0; i < _size; i++) {
            val += _buf[i];
        }
        return static_cast<T>(val / _size);
    }
};
