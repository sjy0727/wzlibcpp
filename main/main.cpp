#include <algorithm>
#include <iostream>
#include <string>

#include <wz/Directory.hpp>
#include <wz/File.hpp>
#include <wz/Node.hpp>
#include <wz/Property.hpp>

#include <fstream>

#define U8 static_cast<u8>
#define IV4(A, B, C, D) \
    { \
        U8(A), U8(B), U8(C), U8(D) \
    }

int main()
{
    const auto iv = IV4(0xb9, 0x7d, 0x63, 0xe9);
    wz::File   file(iv, "/Users/sunjunyi/CLionProjects/sdlMS/cmake-build-debug/bin/Data/Map.wz");

    file.parse(u"Map");
    // auto childs = file.get_root()->find_from_path(u"Map/Map1/100000000.img");
    auto childs = file.get_root()->find_from_path("Map/Map1/100000000.img");
    for (const auto& c : *childs)
    {
        std::printf("%ls\n", c.first.c_str());

        for (auto& n : c.second)
        {
            auto t = n->get_type();
            // print type
            switch (t)
            {
                case wz::Type::Canvas:
                    std::printf("Canvas\n");
                    break;
                case wz::Type::Convex2D:
                    std::printf("Convex2D\n");
                    break;
                case wz::Type::Directory:
                    std::printf("Directory\n");
                    break;
                case wz::Type::Double:
                    std::printf("Double\n");
                    break;
                case wz::Type::Float:
                    std::printf("Float\n");
                    break;
                case wz::Type::Image:
                    std::printf("Image\n");
                    break;
                case wz::Type::Int:
                    std::printf("Int\n");
                    break;
                case wz::Type::NotSet:
                    std::printf("NotSet\n");
                    break;
                case wz::Type::Null:
                    std::printf("Null\n");
                    break;
                case wz::Type::Property:
                    std::printf("Property\n");
                    break;
                case wz::Type::Sound:
                    std::printf("Sound\n");
                    break;
                case wz::Type::String:
                    std::printf("String\n");
                    break;
                case wz::Type::SubProperty:
                    std::printf("SubProperty\n");
                    break;
                case wz::Type::UOL:
                    std::printf("UOL\n");
                    break;
                case wz::Type::UnsignedShort:
                    std::printf("UnsignedShort\n");
                    break;
                case wz::Type::Vector2D:
                    std::printf("Vector2D\n");
                    break;
                default:
                    std::printf("Unknown\n");
            }
        }
    }

    return 0;
}