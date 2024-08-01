#include "Reader.hpp"
#include "Keys.hpp"

#include <cassert>
#include <vector>

namespace wz
{
    Reader::Reader(MutableKey& new_key, const char* file_path) : key(new_key), cursor(0)
    {
        std::error_code error_code;
        mmap = mio::make_mmap_source<decltype(file_path)>(file_path, error_code);
    }

    u8 Reader::read_byte() { return mmap[cursor++]; }

    [[maybe_unused]] std::vector<u8> Reader::read_bytes(const size_t& len)
    {
        std::vector<u8> result(len);

        for (size_t i = 0; i < len; ++i)
        {
            result.emplace_back(read_byte());
            cursor++;
        }

        return result;
    }

    wzstring Reader::read_string()
    {
        wzstring result {};

        while (true)
        {
            const char c = static_cast<char>(read_byte());
            if (c == '\0')
                break;
            result.push_back(c);
        }

        return result;
    }

    wzstring Reader::read_string(const size_t& len)
    {
        wzstring result {};

        for (int i = 0; i < len; ++i)
        {
            result.push_back(read_byte());
        }

        return result;
    }

    void Reader::set_position(const size_t& size) { cursor = size; }

    size_t Reader::get_position() const { return cursor; }

    void Reader::skip(const size_t& size) { cursor += size; }

    i32 Reader::read_compressed_int()
    {
        const i32 result = static_cast<i32>(read<i8>());
        if (result == INT8_MIN)
            return read<i32>();
        return result;
    }

    i16 Reader::read_i16() { return read<i16>(); }

    wzstring Reader::read_wz_string()
    {
        // 读取一个8位有符号整数len8
        const auto len8 = read<i8>();

        // 判断len8是否为0，如果是，则返回一个空字符串
        if (len8 == 0)
            return {};

        i32 len;

        // 如果len8大于0，则判断len8是否为127，如果是，则读取一个32位有符号整数，否则len=len8
        if (len8 > 0)
        {
            u16 mask = 0xAAAA;

            len = len8 == 127 ? read<i32>() : len8;

            // 如果len小于等于0，则返回一个空字符串
            if (len <= 0)
            {
                return {};
            }

            wzstring result {};

            // 循环读取len个16位无符号整数，并进行异或操作，然后将其添加到result字符串中
            for (int i = 0; i < len; ++i)
            {
                auto encryptedChar = read<u16>();
                encryptedChar ^= mask;
                encryptedChar ^= *reinterpret_cast<u16*>(&key[2 * i]);
                result.push_back(encryptedChar);
                mask++;
            }

            return result;
        }

        // 如果len8小于0，则判断len8是否为-128，如果是，则读取一个32位有符号整数，否则len=-len8
        u8 mask = 0xAA;

        if (len8 == -128)
        {
            len = read<i32>();
        }
        else
        {
            len = -len8;
        }

        // 如果len小于等于0，则返回一个空字符串
        if (len <= 0)
        {
            return {};
        }

        wzstring result {};

        // 循环读取len个8位无符号整数，并进行异或操作，然后将其添加到result字符串中
        for (int n = 0; n < len; ++n)
        {
            u8 encryptedChar = read_byte();
            encryptedChar ^= mask;
            encryptedChar ^= key[n];
            result.push_back(static_cast<u16>(encryptedChar));
            mask++;
        }

        return result;
    }

    mio::mmap_source::size_type Reader::size() const { return mmap.size(); }

    bool Reader::is_wz_image()
    {
        // 要同时满足先读取到的8位无符号整数（read<u8>()）为0x73，
        // 再读取到的wz字符串（read_wz_string()）为"Property"，
        // 再读取到的16位无符号整数（read<u16>()）为0才可以判断读取的数据是一张wz图片
        if (read<u8>() != 0x73)
            return false;
        if (read_wz_string() != u"Property")
            return false;
        if (read<u16>() != 0)
            return false;
        return true;
    }

    wzstring Reader::read_string_block(const size_t& offset)
    {
        switch (read<u8>())
        {
            case 0:
            case 0x73:
                return read_wz_string();
            case 1:
            case 0x1B:
                return read_wz_string_from_offset(offset + read<u32>());
            default: {
                assert(0);
                return {};
            }
        }
    }

    wzstring Reader::read_wz_string_from_offset(const size_t &offset)
    {
        // 获取当前位置
        const auto prev = get_position();
        // 设置当前位置为offset
        set_position(offset);
        // 从当前位置读取一个wzstring
        auto result = read_wz_string();
        // 把当前位置重置为prev
        set_position(prev);
        // 返回读取的wzstring
        return result;
    }

    void Reader::set_key(const MutableKey &new_key)
    {
        this->key = new_key;
    }
}