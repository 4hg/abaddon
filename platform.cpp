#include "platform.hpp"
#include <string>
#include <fstream>
#include <filesystem>
#include <config.h>

using namespace std::literals::string_literals;

bool IsFolder(std::string_view path) {
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    return status.type() == std::filesystem::file_type::directory;
}

bool IsFile(std::string_view path) {
    std::error_code ec;
    const auto status = std::filesystem::status(path, ec);
    if (ec) return false;
    return status.type() == std::filesystem::file_type::regular;
}

#if defined(_WIN32) && defined(_MSC_VER)
    #include <Windows.h>
    #include <Shlwapi.h>
    #include <ShlObj_core.h>
    #include <pango/pangocairo.h>
    #include <pango/pangofc-fontmap.h>
    #pragma comment(lib, "Shlwapi.lib")
bool Platform::SetupFonts() {
    using namespace std::string_literals;

    char buf[MAX_PATH] { 0 };
    GetCurrentDirectoryA(MAX_PATH, buf);
    {
        // thanks @WorkingRobot for da help :^))

        std::ifstream template_stream(buf + "\\fonts\\fonts.template.conf"s);
        std::ofstream conf_stream(buf + "\\fonts\\fonts.conf"s);
        if (!template_stream.good()) {
            printf("can't open fonts/fonts.template.conf\n");
            return false;
        }
        if (!conf_stream.good()) {
            printf("can't open write to fonts.conf\n");
            return false;
        }

        std::string line;
        while (std::getline(template_stream, line)) {
            if (line == "<!--(CONFD)-->")
                conf_stream << "<include ignore_missing=\"no\">" << (buf + "\\fonts\\conf.d"s) << "</include>";
            else
                conf_stream << line;
            conf_stream << '\n';
        }
    }

    auto fc = FcConfigCreate();
    FcConfigSetCurrent(fc);
    FcConfigParseAndLoad(fc, const_cast<FcChar8 *>(reinterpret_cast<const FcChar8 *>((buf + "\\fonts\\fonts.conf"s).c_str())), true);
    FcConfigAppFontAddDir(fc, const_cast<FcChar8 *>(reinterpret_cast<const FcChar8 *>((buf + "\\fonts"s).c_str())));

    char fonts_path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, fonts_path) == S_OK) {
        FcConfigAppFontAddDir(fc, reinterpret_cast<FcChar8 *>(fonts_path));
    }

    auto map = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    pango_fc_font_map_set_config(reinterpret_cast<PangoFcFontMap *>(map), fc);
    pango_cairo_font_map_set_default(reinterpret_cast<PangoCairoFontMap *>(map));

    return true;
}
#else
bool Platform::SetupFonts() {
    return true;
}
#endif

#if defined(_WIN32)
std::string Platform::FindResourceFolder() {
    return ".";
}

std::string Platform::FindConfigFile() {
    const auto x = std::getenv("ABADDON_CONFIG");
    if (x != nullptr)
        return x;
    return "./abaddon.ini";
}

#elif defined(__linux__)
std::string Platform::FindResourceFolder() {
    static std::string found_path;
    static bool found = false;
    if (found) return found_path;

    const static std::string home_path = std::getenv("HOME") + "/.local/share/abaddon"s;

    for (const auto &path : { "."s, home_path, std::string(ABADDON_DEFAULT_RESOURCE_DIR) }) {
        if (IsFolder(path + "/res") && IsFolder(path + "/css")) {
            found_path = path;
            found = true;
            return found_path;
        }
    }

    puts("cant find a resources folder, will try to load from cwd");
    found_path = ".";
    found = true;
    return found_path;
}

std::string Platform::FindConfigFile() {
    const auto x = std::getenv("ABADDON_CONFIG");
    if (x != nullptr)
        return x;

    const auto home_path = std::string(std::getenv("HOME")) + "/.config/abaddon/abaddon.ini";
    for (const auto path : { "./abaddon.ini"s, home_path }) {
        if (IsFile(path)) return path;
    }
    puts("can't find configuration file!");
    return "./abaddon.ini";
}
#else
std::string Platform::FindResourceFolder() {
    puts("unknown OS, trying to load resources from cwd");
    return ".";
}

std::string Platform::FindConfigFile() {
    const auto x = std::getenv("ABADDON_CONFIG");
    if (x != nullptr)
        return x;
    puts("unknown OS, trying to load config from cwd");
    return "./abaddon.ini";
}
#endif
