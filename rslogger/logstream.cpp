#include "logstream.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdio.h>   //snprintf
#include <cinttypes> //PRId64
#include <limits>    //numeric_limits
#include "stringpiece.h"
#include "floatingbuffer.h"
#include "fmt.h"


const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");
const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof(digitsHex) == 17, "wrong number of digitsHex");

// int到string转换函数,value->buf，10进制
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;
    //当value为0时，也会往buf中加入一个字符0
    //如果有while()，则会得到一个空的Buf
    do
    {
        //T的类型可能不是int，只有int能有%运算符
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    //返回得到的字符串的长度
    return p - buf;
}

// 16进制转化，int->char[]
size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

// 把intz转换成字符串再输入
template <typename T>
void RsLogStream::formatInteger(T v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

RsLogStream& RsLogStream::operator<< (bool v)
{
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

// short提升到相应的int
RsLogStream& RsLogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}
// int,long,long long转换成字符串后插入
RsLogStream& RsLogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

RsLogStream& RsLogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

// 输出指针
RsLogStream& RsLogStream::operator<<(const void* p)
{
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}

RsLogStream& RsLogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        // %g 把输出的值按照%e或者%f类型中输出长度较小的方式输出
        // %e 指数形式的浮点数
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

//重载浮点输出
RsLogStream& RsLogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}

//重载字符输出
RsLogStream& RsLogStream::operator<<(char v)
{
    buffer_.append(&v, 1);
    return *this;
}

RsLogStream& RsLogStream::operator<<(const char* str)
{
    if (str)
    {
        buffer_.append(str, strlen(str));
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}

//重载字符串输出
RsLogStream& RsLogStream::operator<<(const string& v)
{
    buffer_.append(v.c_str(), v.size());
    return *this;
}

RsLogStream& RsLogStream::operator<<(const RsStringPiece& v)
{
    buffer_.append(v.data(), v.size());
    return *this;
}

RsLogStream& RsLogStream::operator<<(const Buffer& v)
{
    *this << v.toStringPiece();
    return *this;
}

//往data里加入len长的data
void RsLogStream::append(const char* data, int len)
{ 
    buffer_.append(data, len);
}

const RsFloatingBuffer<kSmallBuffer>& RsLogStream::buffer() const
{ 
    return buffer_;
}

void RsLogStream::resetBuffer()
{ 
    buffer_.reset(); 
}
