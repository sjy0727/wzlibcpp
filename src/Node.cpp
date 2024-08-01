#include "Node.hpp"
#include "Directory.hpp"
#include "File.hpp"
#include "Property.hpp"

#include <cassert>
#include <ranges>

namespace wz
{
    Node::Node() : type(Type::NotSet), parent(nullptr), file(nullptr) {}

    Node::Node(const Type& new_type, File* root_file) : type(new_type), parent(nullptr), file(root_file)
    {
        reader = &file->reader;
    }

    // 释放Node对象占用的内存，遍历其子节点并删除它们
    Node::~Node()
    {
        for (auto& [_, nodes] : this->children)
        {
            for (const auto* node : nodes)
            {
                delete node;
            }
        }
    }

    // 向当前节点添加一个子节点。
    void Node::appendChild(const wzstring& name, Node* node)
    {
        assert(node);
        this->children[name].push_back(node);
        node->parent = this;
        node->path   = this->path + u"/" + name;
    }

    // 获取当前节点的所有子节点。
    const WzMap& Node::get_children() const { return this->children; }

    // 获取当前节点的父节点
    Node* Node::get_parent() const { return this->parent; }

    // 获取当前节点的第一个子节点
    WzMap::iterator Node::begin() { return this->children.begin(); }

    // 获取当前节点的最后一个子节点
    WzMap::iterator Node::end() { return this->children.end(); }

    // 获取当前节点的子节点数量
    size_t Node::children_count() const { return this->children.size(); }

    /**
     * 解析属性列表。
     * @param target 要附加属性的目标节点。
     * @param offset 读取字符串块时的偏移量。
     * @return 总是返回true，表示解析过程成功执行。
     */
    bool Node::parse_property_list(Node* target, size_t offset)
    {
        // 读取属性条目的数量。
        auto entryCount = this->reader->read_compressed_int();

        // 遍历每个属性条目。
        for (i32 i = 0; i < entryCount; i++)
        {
            // 读取属性名称。
            auto name = this->reader->read_string_block(offset);

            // 读取属性类型。
            auto prop_type = this->reader->read<u8>();
            switch (prop_type)
            {
                case 0: {
                    // 创建并添加空属性。
                    auto* prop = new Property<WzNull>(Type::Null, file);
                    prop->path = target->path + u"/" + name;

                    target->appendChild(name, prop);
                }
                break;
                case 0x0B:
                case 2: {
                    // 创建并添加无符号短整型属性。
                    auto* prop = new Property<u16>(Type::UnsignedShort, file, reader->read<u16>());
                    prop->path = target->path + u"/" + name;

                    target->appendChild(name, prop);
                }
                break;
                case 3: {
                    // 创建并添加有符号整型属性。
                    auto* prop = new Property<i32>(Type::Int, file, reader->read_compressed_int());
                    prop->path = target->path + u"/" + name;

                    target->appendChild(name, prop);
                }
                break;
                case 4: {
                    // 根据浮点类型创建并添加浮点型属性。
                    auto float_type = reader->read<u8>();
                    if (float_type == 0x80)
                    {
                        auto* prop = new Property<f32>(Type::Float, file, reader->read<f32>());
                        prop->path = target->path + u"/" + name;

                        target->appendChild(name, prop);
                    }
                    else if (float_type == 0)
                    {
                        auto* pProp = new Property<f32>(Type::Float, file, 0.f);
                        pProp->path = target->path + u"/" + name;

                        target->appendChild(name, pProp);
                    }
                }
                break;
                case 5: {
                    // 创建并添加双精度浮点型属性。
                    auto* prop = new Property<f64>(Type::Double, file, reader->read<f64>());
                    prop->path = target->path + u"/" + name;

                    target->appendChild(name, prop);
                }
                break;
                case 8: {
                    // 创建并添加字符串属性。
                    auto* prop = new Property<wzstring>(Type::String, file);
                    prop->path = target->path + u"/" + name;

                    auto str = this->reader->read_string_block(offset);
                    prop->set(str);
                    target->appendChild(name, prop);
                }
                break;
                case 9: {
                    // 解析扩展属性，并根据需要调整读取位置。
                    auto ofs = this->reader->read<u32>();
                    auto eob = this->reader->get_position() + ofs;
                    parse_extended_prop(name, target, offset);
                    if (this->reader->get_position() != eob)
                        this->reader->set_position(eob);
                }
                break;
                default: {
                    // 遇到未知属性类型，断言失败。
                    assert(0);
                }
            }
        }

        // 解析成功，返回true。
        return true;
    }

    /**
     * 解析扩展属性
     * @param name 属性名称
     * @param target 目标节点
     * @param offset 字节偏移量
     * 根据属性名称的不同，解析并创建不同的属性类型，然后将其添加到目标节点中。
     */
    void Node::parse_extended_prop(const wzstring& name, Node* target, const size_t& offset)
    {
        // 根据偏移量读取属性名称的字符串块
        auto strPropName = this->reader->read_string_block(offset);

        // 根据属性名称的值，选择性地解析并处理不同类型的属性
        if (strPropName == u"Property")
        {
            // 处理子属性
            auto* prop = new Property<WzSubProp>(Type::SubProperty, file);
            prop->path = target->path + u"/" + name;
            this->reader->skip(sizeof(u16));   // 跳过特定字节
            parse_property_list(prop, offset); // 解析属性列表
            target->appendChild(name, prop);   // 将属性添加到目标节点
        }
        else if (strPropName == u"Canvas")
        {
            // 处理画布属性
            auto* prop = new Property<WzCanvas>(Type::Canvas, file);
            prop->path = target->path + u"/" + name;
            this->reader->skip(sizeof(u8)); // 跳过特定字节
            if (this->reader->read<u8>() == 1)
            {
                this->reader->skip(sizeof(u16));   // 跳过特定字节
                parse_property_list(prop, offset); // 解析属性列表
            }
            prop->set(parse_canvas_property()); // 设置画布属性
            target->appendChild(name, prop);    // 将属性添加到目标节点
        }
        else if (strPropName == u"Shape2D#Vector2D")
        {
            // 处理2D向量属性
            auto* prop = new Property<WzVec2D>(Type::Vector2D, file);
            prop->path = target->path + u"/" + name;
            auto x     = this->reader->read_compressed_int(); // 读取压缩整数x
            auto y     = this->reader->read_compressed_int(); // 读取压缩整数y
            prop->set({x, y});                                // 设置向量属性
            target->appendChild(name, prop);                  // 将属性添加到目标节点
        }
        else if (strPropName == u"Shape2D#Convex2D")
        {
            // 处理2D凸包属性
            auto* prop           = new Property<WzConvex>(Type::Convex2D, file);
            prop->path           = target->path + u"/" + name;
            int convexEntryCount = this->reader->read_compressed_int(); // 读取凸包项数
            for (int i = 0; i < convexEntryCount; i++)
            {
                // 递归解析每个凸包项的扩展属性
                parse_extended_prop(name, prop, offset);
            }
            target->appendChild(name, prop); // 将属性添加到目标节点
        }
        else if (strPropName == u"Sound_DX8")
        {
            // 处理声音属性
            auto* prop = new Property<WzSound>(Type::Sound, file);
            prop->path = target->path + u"/" + name;
            prop->set(parse_sound_property()); // 设置声音属性
            target->appendChild(name, prop);   // 将属性添加到目标节点
        }
        else if (strPropName == u"UOL")
        {
            // 跳过特定字节
            this->reader->skip(sizeof(u8));
            auto* prop = new Property<WzUOL>(Type::UOL, file);
            prop->path = target->path + u"/" + name;
            prop->set({this->reader->read_string_block(offset)}); // 设置UOL属性
            target->appendChild(name, prop);                      // 将属性添加到目标节点
        }
        else
        {
            // 如果属性名称不匹配任何已知类型，断言失败
            assert(0);
        }
    }

    /**
     * 解析节点的画布属性
     *
     * 该函数从一个阅读器中解析出画布的宽度、高度、格式等属性，并判断画布是否被加密。
     * 根据画布的格式和格式2的组合，计算画布的未压缩大小。
     *
     * @return WzCanvas 一个包含解析后画布属性的结构体。
     */
    WzCanvas Node::parse_canvas_property()
    {
        WzCanvas canvas; // 初始化一个空的画布结构体

        // 从阅读器中读取并赋值画布的宽度、高度、格式和格式2
        canvas.width   = reader->read_compressed_int();
        canvas.height  = reader->read_compressed_int();
        canvas.format  = reader->read_compressed_int();
        canvas.format2 = reader->read<u8>();

        // 跳过一些未使用的数据
        reader->skip(sizeof(u32));
        canvas.size = reader->read<i32>() - 1;
        reader->skip(sizeof(u8));

        // 记录当前读取位置，作为画布的偏移量
        canvas.offset = reader->get_position();

        // 读取一个短整型header，用于判断画布是否被加密
        auto header = reader->read<u16>();

        // 如果header值不为0x9C78或0xDA78，则认为画布被加密
        if (header != 0x9C78 && header != 0xDA78)
        {
            canvas.is_encrypted = true;
        }

        // 根据画布的格式和格式2计算画布的未压缩大小
        switch (canvas.format + canvas.format2)
        {
            case 1: {
                // Format8bpp
                canvas.uncompressed_size = canvas.width * canvas.height * 2;
            }
            break;
            case 2: {
                // Format32bppArgb
                canvas.uncompressed_size = canvas.width * canvas.height * 4;
            }
            break;
            case 513: // Format16bppRgb565
            {
                canvas.uncompressed_size = canvas.width * canvas.height * 2;
            }
            break;
            case 517: {
                // 特殊格式，大小计算方式不同
                canvas.uncompressed_size = canvas.width * canvas.height / 128;
            }
            break;
        }

        // 将读取位置重置到画布的偏移量加上画布的大小，为后续读取准备
        reader->set_position(canvas.offset + canvas.size);

        return canvas; // 返回解析后的画布属性
    }

    /**
     * 解析声音属性
     *
     * 该函数从一个特定的数据读取器中解析出声音对象的属性。
     * 它不返回任何值，但通过声音对象的成员变量来存储解析的结果。
     *
     * @return WzSound 一个包含声音属性的对象。
     */
    WzSound Node::parse_sound_property()
    {
        WzSound sound; // 初始化一个声音对象

        // 跳过一个未使用的字节
        // reader->ReadUInt8();
        reader->skip(sizeof(u8));

        // 读取声音的大小和长度
        sound.size   = reader->read_compressed_int();
        sound.length = reader->read_compressed_int();

        // 快进到声音数据的起始位置
        reader->set_position(reader->get_position() + 56);

        // 读取声音的频率
        sound.frequency = reader->read<i32>();

        // 再次快进以忽略一些未使用的数据
        reader->set_position(reader->get_position() + 22);

        // 记录声音数据在文件中的偏移量
        sound.offset = reader->get_position();

        // 移动读取器到声音数据的结束位置
        reader->set_position(sound.offset + sound.size);

        return sound; // 返回包含声音属性的对象
    }

    Type Node::get_type() const { return type; }

    bool Node::is_property() const { return (bit(type) & bit(Type::Property)) == bit(Type::Property); }

    MutableKey& Node::get_key() const { return file->key; }

    u8* Node::get_iv() const { return file->iv; }

    Node* Node::get_child(const wzstring& name)
    {
        if (auto it = children.find(name); it != children.end())
        {
            return it->second[0];
        }
        return nullptr;
    }

    Node* Node::get_child(const std::string& name) { return get_child(std::u16string {name.begin(), name.end()}); }

    // Node& Node::operator[](const wzstring& name) { return *get_child(name); }

    Node& Node::operator[](const wzstring& path)
    {
        // 将路径分割成多个部分
        std::vector<std::u16string> parts;
        std::u16string              delimiter = u"/";
        size_t                      start     = 0;
        size_t                      end       = path.find(delimiter);
        while (end != std::u16string::npos)
        {
            parts.push_back(path.substr(start, end - start));
            start = end + delimiter.length();
            end   = path.find(delimiter, start);
        }
        parts.push_back(path.substr(start, end));

        // 从当前节点开始搜索
        Node* node = this;

        for (const auto& part : parts)
        {
            if (part == u"..")
            {
                // 返回上一级节点
                node = node->parent;
            }
            else
            {
                // 获取当前节点的子节点，如果存在
                node = node->get_child(part);
                if (node != nullptr)
                {
                    // 处理UOL节点，通过动态转换获取UOL对象
                    if (node->type == Type::UOL)
                    {
                        node = dynamic_cast<Property<WzUOL>*>(node)->get_uol();
                    }

                    // 处理Image节点，缓存image节点以提高后续查找效率
                    if (node->type == Type::Image)
                    {
                        static std::map<std::u16string, Node*> img_map;
                        auto                                   it = img_map.find(node->path);
                        if (it != img_map.end())
                        {
                            node = it->second;
                        }
                        else
                        {
                            auto* image = new Node();
                            auto* dir   = dynamic_cast<Directory*>(node);
                            dir->parse_image(image);
                            node                = image;
                            img_map[node->path] = node;
                        }
                    }
                }
                else
                {
                    // 如果子节点不存在，立即返回nullptr，表示未找到路径所指节点
                    throw std::runtime_error("Node not found");
                }
            }
        }
        // 返回最终找到的节点，或者nullptr如果路径无法完全解析
        return *node;
    }

    /**
     * 根据路径查找节点。
     *
     * 该函数通过解析给定的路径字符串，逐级查找节点。路径可以包含相对路径元素（如".."
     * 用于返回上一级节点）和常规节点名，用于深入节点树。
     *
     * @param path 要查找的路径，可以包含相对路径元素和节点名。
     * @return 找到的节点指针，如果未找到则返回nullptr。
     */
    // Node *Node::find_from_path(const std::u16string &path)
    // {
    //     // 使用标准库视图功能分割路径并去除空字符串
    //     auto next = std::views::split(path, u'/') | std::views::common;
    //
    //     // 从当前节点开始搜索
    //     Node *node = this;
    //
    //     for (const auto &s : next)
    //     {
    //         // 将视图分割得到的字符串范围转换为标准字符串
    //         auto str = std::u16string{s.begin(), s.end()};
    //
    //         // 处理相对路径元素".."，返回上一级节点
    //         if (str == u"..")
    //         {
    //             node = node->parent;
    //             continue;
    //         }
    //         else
    //         {
    //             // 获取当前节点的子节点，如果存在
    //             node = node->get_child(str);
    //             if (node != nullptr)
    //             {
    //                 // 处理UOL节点，通过动态转换获取UOL对象
    //                 // 处理UOL
    //                 if (node->type == Type::UOL)
    //                 {
    //                     node = dynamic_cast<Property<WzUOL> *>(node)->get_uol();
    //                 }
    //
    //                 // 处理Image节点，缓存image节点以提高后续查找效率
    //                 if (node->type == Type::Image)
    //                 {
    //                     static std::map<std::u16string, Node *> img_map;
    //                     if (img_map.contains(node->path))
    //                     {
    //                         node = img_map[node->path];
    //                     }
    //                     else
    //                     {
    //                         auto *image = new Node();
    //                         auto *dir = dynamic_cast<Directory *>(node);
    //                         dir->parse_image(image);
    //                         node = image;
    //                         img_map[node->path] = node;
    //                     }
    //                     continue;
    //                 }
    //             }
    //             else
    //             {
    //                 // 如果子节点不存在，立即返回nullptr，表示未找到路径所指节点
    //                 return nullptr;
    //             }
    //         }
    //     }
    //     // 返回最终找到的节点，或者nullptr如果路径无法完全解析
    //     return node;
    // }

    Node* Node::find_from_path(const std::u16string& path)
    {
        // 将路径分割成多个部分
        std::vector<std::u16string> parts;
        std::u16string delimiter = u"/";
        size_t start = 0;
        size_t end = path.find(delimiter);
        while (end != std::u16string::npos) {
            parts.push_back(path.substr(start, end - start));
            start = end + delimiter.length();
            end = path.find(delimiter, start);
        }
        parts.push_back(path.substr(start, end));

        // 从当前节点开始搜索
        Node* node = this;

        for (const auto& part : parts) {
            if (part == u"..") {
                // 返回上一级节点
                node = node->parent;
            } else {
                // 获取当前节点的子节点，如果存在
                node = node->get_child(part);
                if (node != nullptr) {
                    // 处理UOL节点，通过动态转换获取UOL对象
                    if (node->type == Type::UOL) {
                        node = dynamic_cast<Property<WzUOL>*>(node)->get_uol();
                    }

                    // 处理Image节点，缓存image节点以提高后续查找效率
                    if (node->type == Type::Image) {
                        static std::map<std::u16string, Node*> img_map;
                        auto it = img_map.find(node->path);
                        if (it != img_map.end()) {
                            node = it->second;
                        } else {
                            auto* image = new Node();
                            auto* dir = dynamic_cast<Directory*>(node);
                            dir->parse_image(image);
                            node = image;
                            img_map[node->path] = node;
                        }
                    }
                } else {
                    // 如果子节点不存在，立即返回nullptr，表示未找到路径所指节点
                    throw std::runtime_error("Node not found");
                }
            }
        }
        // 返回最终找到的节点，或者nullptr如果路径无法完全解析
        return node;
    }

    Node *Node::find_from_path(const std::string &path)
    {
        return find_from_path(std::u16string{path.begin(), path.end()});
    }
}