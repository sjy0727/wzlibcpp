#include "Property.hpp"
#include "Types.hpp"


// 直接获取纹理格式数据，可以直接导入到游戏引擎使用
template <>
std::vector<u8> wz::Property<wz::WzCanvas>::get_raw_data(std::array<u8, 4> iv)
{
    WzCanvas canvas = get();

    std::vector<u8> data_stream;
    reader->set_position(canvas.offset);
    size_t end_offset = reader->get_position() + canvas.size;
    unsigned long uncompressed_len = canvas.uncompressed_size;
    u8 *uncompressed = new u8[uncompressed_len];
    if (!canvas.is_encrypted)
    {
        for (size_t i = 0; i < canvas.size; ++i)
        {
            data_stream.push_back(reader->read_byte());
        }
        int len = data_stream.size();
        uncompress(uncompressed, &uncompressed_len, data_stream.data(), data_stream.size());
    }
    else
    {
        auto wz_key = get_key();

        while (reader->get_position() < end_offset)
        {
            auto block_size = reader->read<i32>();
            for (size_t i = 0; i < block_size; ++i)
            {
                auto n = wz_key[i];
                data_stream.push_back(
                    static_cast<u8>(reader->read_byte() ^ n));
            }
        }
        uncompress(uncompressed, (unsigned long *)&uncompressed_len, data_stream.data(), data_stream.size());
    }

    std::vector<u8> pixel_stream(uncompressed, uncompressed + uncompressed_len);
    return pixel_stream;
}

//

template <>
std::vector<u8> wz::Property<wz::WzSound>::get_raw_data(std::array<u8, 4> iv)
{
    WzSound sound = get();
    std::vector<u8> data_stream;

    reader->set_position(sound.offset);
    size_t end_offset = reader->get_position() + sound.size;

    while (reader->get_position() < end_offset)
    {
        data_stream.push_back(static_cast<u8>(reader->read_byte()));
    }
    return data_stream;
}

template <>
wz::Node *wz::Property<wz::WzUOL>::get_uol()
{
    auto path = get().uol;
    auto uol_node = parent->find_from_path(path);
    while (uol_node->type == wz::Type::UOL)
    {
        path = dynamic_cast<wz::Property<wz::WzUOL> *>(uol_node)->get().uol;
        uol_node = uol_node->parent->find_from_path(path);
    }

    return uol_node;
}
