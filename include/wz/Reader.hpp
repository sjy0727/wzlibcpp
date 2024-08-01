#pragma once

#include <mio/mmap.hpp>
#include "NumTypes.hpp"
#include "Keys.hpp"

namespace wz
{

    using wzstring = std::u16string;

    class Reader final
    {
    public:
        explicit Reader() = delete;
        explicit Reader(wz::MutableKey &new_key, const char *file_path);

        // 使用模板方法 read() 来读取不同类型的数据。
        // 例如read<u8>() 用于读取一个字节，read<i32>() 用于读取一个整数
        template <typename T>
        [[nodiscard]] T read()
        {
            T result = *reinterpret_cast<T *>(&mmap[cursor]);
            cursor += sizeof(decltype(result));
            return result;
        }

        // 跳过size个字节
        void skip(const size_t &size);

        // 读取一个字节
        [[nodiscard]] u8 read_byte();

        // 读取len个字节，并将返回值存入std::vector<u8>
        [[maybe_unused]] [[nodiscard]] std::vector<u8> read_bytes(const size_t &len);

        // 从输入中读取一个以空字符结尾的字符串，并将其作为wzstring类型的对象返回
        [[nodiscard]] wzstring read_string();

        // 从输入中读取len个字节，并将其作为wzstring类型的对象返回
        [[nodiscard]] wzstring read_string(const size_t &len);

        // 首先读取一个8位的有符号整数（i8类型），然后判断该整数是否等于INT8_MIN（即-128），
        // 如果是，则再读取一个32位的有符号整数（i32类型），否则直接返回读取到的8位整数。
        [[nodiscard]] i32 read_compressed_int();

        // 读取一个16位的有符号整数
        i16 read_i16();

        // 读取一个经过加密的字符串，并返回解密后的字符串。
        [[nodiscard]] wzstring read_wz_string();

        /**
         * 使用read<u8>()读取一个8位无符号整数，然后根据读取到的值进行switch语句判断。
         * 如果读取到的值等于0或者0x73，则调用read_wz_string()函数读取一个WZ字符串并返回。
         * 如果读取到的值等于1或者0x1B，则调用read_wz_string_from_offset()函数，从给定的偏移量开始读取一个WZ字符串并返回。
         * 如果读取到的值不等于上述情况，则执行assert(0)，抛出一个异常。
         * @param offset 要读取的字符串块的偏移量
         * @return WZ字符串
         */
        wzstring read_string_block(const size_t &offset);

        /**
         * 从偏移量offset处读取一个T类型的值，并将读取到的wzstring类型的值存入out中
         * @tparam T 要读取的值的类型
         * @param offset 要读取的值的偏移量
         * @param out 存储读取到的值和WZ字符串的引用
         * @return 读取到的值
         */
        template <typename T>
        [[nodiscard]] T read_wz_string_from_offset(const size_t &offset, wzstring &out)
        {
            // 保存当前光标位置
            const auto prev = cursor;
            // 将光标移动到偏移量offset处
            set_position(offset);
            // 读取T类型的值
            auto result = read<T>();
            // 读取wzstring类型的值
            out = read_wz_string();
            // 将光标移动回原来的位置
            set_position(prev);
            // 返回T类型的值
            return result;
        }

        // 从偏移量offset处读取一个wzstring
        wzstring read_wz_string_from_offset(const size_t &offset);

        // 获取文件指针游标
        [[nodiscard]] size_t get_position() const;

        // 设置文件指针游标
        void set_position(const size_t &size);

        // 获取文件大小
        [[nodiscard]] mio::mmap_source::size_type size() const;

        // 判断是否是wz图片
        [[nodiscard]] bool is_wz_image();

        // 设置key
        void set_key(const MutableKey &new_key);

    private:
        MutableKey &key;

        // 文件指针游标
        size_t cursor = 0;

        mio::mmap_source mmap;

        friend class Node;
    };
}
