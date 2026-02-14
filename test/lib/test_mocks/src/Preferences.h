#ifndef MOCK_PREFERENCES_H
#define MOCK_PREFERENCES_H

#include <cstdint>
#include <cstring>

// Simple in-memory NVS mock with fixed-size storage
class Preferences {
public:
    Preferences() : _count(0) {}

    bool begin(const char* name, bool readOnly = false) {
        (void)name; (void)readOnly;
        return true;
    }
    void end() {}

    bool putFloat(const char* key, float value) {
        for (int i = 0; i < _count; i++) {
            if (strcmp(_keys[i], key) == 0) {
                _values[i] = value;
                return true;
            }
        }
        if (_count < MAX_ENTRIES) {
            strncpy(_keys[_count], key, KEY_LEN - 1);
            _keys[_count][KEY_LEN - 1] = '\0';
            _values[_count] = value;
            _count++;
            return true;
        }
        return false;
    }

    float getFloat(const char* key, float defaultValue = 0.0f) {
        for (int i = 0; i < _count; i++) {
            if (strcmp(_keys[i], key) == 0) {
                return _values[i];
            }
        }
        return defaultValue;
    }

    void clear() { _count = 0; }

private:
    static const int MAX_ENTRIES = 16;
    static const int KEY_LEN = 32;
    char _keys[MAX_ENTRIES][KEY_LEN];
    float _values[MAX_ENTRIES];
    int _count;
};

#endif // MOCK_PREFERENCES_H
