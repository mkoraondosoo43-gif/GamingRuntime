#include "gaming_runtime/asset_manager.h"

#include <filesystem>
#include <fstream>

namespace gaming_runtime {

AssetManager::AssetManager(std::string assets_root) {
    set_root(std::move(assets_root));
}

bool AssetManager::set_root(std::string assets_root) {
    std::error_code error;
    const std::filesystem::path root =
        std::filesystem::path(std::move(assets_root)).lexically_normal();

    if (!std::filesystem::is_directory(root, error) || error) {
        assets_root_.clear();
        return false;
    }

    assets_root_ = root.string();
    return true;
}

bool AssetManager::resolve_asset(const std::string& relative_path,
                                 std::string& resolved_path) const {
    if (assets_root_.empty() || relative_path.empty()) {
        return false;
    }

    const std::filesystem::path root(assets_root_);
    const std::filesystem::path candidate =
        (root / relative_path).lexically_normal();

    std::error_code error;
    const auto relative = std::filesystem::relative(
        root, candidate, error);
    if (error) {
        return false;
    }

    const std::string text = relative.generic_string();
    if (text == ".." || text.rfind("../", 0) == 0) {
        return false;
    }

    resolved_path = candidate.string();
    return true;
}

bool AssetManager::has_asset(const std::string& relative_path) const {
    std::string resolved;
    if (!resolve_asset(relative_path, resolved)) {
        return false;
    }

    std::error_code error;
    return std::filesystem::is_regular_file(resolved, error) && !error;
}

bool AssetManager::read_asset(const std::string& relative_path,
                              std::vector<std::uint8_t>& output,
                              std::uint64_t max_bytes) const {
    output.clear();

    std::string resolved;
    if (!resolve_asset(relative_path, resolved)) {
        return false;
    }

    std::error_code error;
    if (!std::filesystem::is_regular_file(resolved, error) || error) {
        return false;
    }

    const auto size = std::filesystem::file_size(resolved, error);
    if (error || size > max_bytes) {
        return false;
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        return false;
    }

    output.resize(static_cast<std::size_t>(size));
    if (size > 0) {
        file.read(reinterpret_cast<char*>(output.data()),
                  static_cast<std::streamsize>(size));
        if (!file) {
            output.clear();
            return false;
        }
    }

    return true;
}

std::vector<AssetInfo> AssetManager::list_assets() const {
    std::vector<AssetInfo> assets;
    if (assets_root_.empty()) {
        return assets;
    }

    std::error_code error;
    const std::filesystem::path root(assets_root_);

    for (std::filesystem::recursive_directory_iterator it(root, error), end;
         it != end && !error; it.increment(error)) {
        if (!it->is_regular_file(error) || error) {
            continue;
        }

        const auto relative = std::filesystem::relative(root, it->path(), error);
        if (error) {
            continue;
        }

        const auto size = it->file_size(error);
        if (error) {
            continue;
        }

        assets.push_back({
            relative.generic_string(),
            relative.generic_string(),
            size
        });
    }

    return assets;
}

const std::string& AssetManager::root() const noexcept {
    return assets_root_;
}

} // namespace gaming_runtime
