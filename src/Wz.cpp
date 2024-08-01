#include "Wz.hpp"
#include "Property.hpp"

#define HASHING(V, S) ((V >> S##u) & 0xFFu)
#define AUTO_HASH(V) (0xFFu ^ HASHING(V, 24) ^ HASHING(V, 16) ^ HASHING(V, 8) ^ V & 0xFFu)

u8 hashing(u32 value, size_t shift) { return (value >> shift) & 0xFFu; }

u32 auto_hash(u32 value)
{
    return 0xFFu ^ hashing(value, 24) ^ hashing(value, 16) ^ hashing(value, 8) ^ (value & 0xFFu);
}

/**
 * 验证加密的版本号是否与实际版本号匹配，并返回实际的版本哈希值
 * @param encryptedVersion 加密的版本号
 * @param realVersion 实际版本号
 * @return 实际的版本哈希值
 */
u32 wz::get_version_hash(i32 encryptedVersion, i32 realVersion)
{
    i32  versionHash   = 0;
    auto versionString = std::to_string(realVersion);

    auto len = versionString.size();

    for (int i = 0; i < len; ++i)
    {
        versionHash = (32 * versionHash) + static_cast<i32>(versionString[i]) + 1;
    }

    i32 decryptedVersionNumber = AUTO_HASH(static_cast<u32>(versionHash));

    if (encryptedVersion == decryptedVersionNumber) {
        return static_cast<u32>(versionHash);
    }

    return 0;
}
