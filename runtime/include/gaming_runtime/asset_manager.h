#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gaming_runtime {

struct AssetInfo {
    std::string id;
    std::string relative_path;
    std::uint64_t size_bytes = 0;
};

class AssetManager {
public:
    AssetManager() = default;
    explicit AssetManager(std::string assets_root);

    bool set_root(std::string assets_root);
    bool has_asset(const std::string& relative_path) const;
    bool read_asset(const std::string& relative_path,
                    std::vector<std::uint8_t>& output,
                    std::uint64_t max_bytes = 16ULL * 1024ULL * 1024ULL) const;
    std::vector<AssetInfo> list_assets() const;
    const std::string& root() const noexcept;

private:
    bool resolve_asset(const std::string& relative_path,
                       std::string& resolved_path) const;

    std::string assets_root_;
};

} // namespace gaming_runtime
