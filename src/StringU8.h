#ifndef StringU8_h
#define StringU8_h

#include <Arduino.h>

class StringU8 
{
public:
    StringU8();
    StringU8(const uint8_t* str);
    StringU8(const char* str);
    StringU8(const StringU8& other);
    StringU8(const String& str);
    ~StringU8();


    StringU8& operator=(const StringU8& other);
    StringU8& operator=(const String& str);
    StringU8& operator=(const char* str);
    StringU8& operator=(const uint8_t* str);
    
    uint8_t operator[](size_t index) const;
    uint8_t& operator[](size_t index);

    size_t length() const;
    const String c_str() const;

    bool operator==(const StringU8& other) const;
    bool operator==(const String& str) const;
    bool operator==(const uint8_t* str) const;
    bool operator==(const char* str) const;


    StringU8 operator+(const StringU8& other) const;
    StringU8 operator+(const String& other) const;
    friend StringU8 operator+(uint8_t ch, const StringU8& str);
    StringU8 operator+(uint8_t other) const;

    StringU8& operator+=(const StringU8& other);
    StringU8& operator+=(const String& other);
    StringU8& operator+=(uint8_t other);


    void push_start(uint8_t ch);
    void push_back(uint8_t ch);
    void clear();


    StringU8 substring(size_t start) const;
    StringU8 substring(size_t start, size_t end) const;

    static StringU8 ConvertArray(const uint8_t* array, size_t length);

private:
    uint8_t* _buffer;
    size_t _length;
    size_t _capacity;

    void _init();
    void _copy(const uint8_t* src, size_t length);
    void _ensureCapacity(size_t newSize);
};

#endif // StringU8_h