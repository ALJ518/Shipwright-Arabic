#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Include this in soh/soh/resource/importer/TextFactory.cpp
// Call: entry.msg = ShapeArabicUTF8ToSoH(child->Attribute("Message"));
namespace SOHArabic {

struct ArabicForms {
    uint8_t isol, fina, init, medi;
    bool connectsNext() const { return init != 0; }
};

inline const std::unordered_map<uint32_t, ArabicForms>& GetArabicMap() {
    static const std::unordered_map<uint32_t, ArabicForms> m = {
        {0x0621, {0xAC, 0, 0, 0}},
        {0x0627, {0xAD, 0xAE, 0, 0}},
        {0x0623, {0xAF, 0xB0, 0, 0}},
        {0x0625, {0xB1, 0xB2, 0, 0}},
        {0x0622, {0xB3, 0xB4, 0, 0}},
        {0x0649, {0xB5, 0xB6, 0, 0}},
        {0x0629, {0xB7, 0xB8, 0, 0}},
        {0x062F, {0xB9, 0xBA, 0, 0}},
        {0x0630, {0xBB, 0xBC, 0, 0}},
        {0x0631, {0xBD, 0xBE, 0, 0}},
        {0x0632, {0xBF, 0xC0, 0, 0}},
        {0x0648, {0xC1, 0xC2, 0, 0}},
        {0x0624, {0xC3, 0xC4, 0, 0}},
        {0x0628, {0xC5, 0xC8, 0xC6, 0xC7}},
        {0x062A, {0xC9, 0xCC, 0xCA, 0xCB}},
        {0x062B, {0xCD, 0xD0, 0xCE, 0xCF}},
        {0x062C, {0xD1, 0xD4, 0xD2, 0xD3}},
        {0x062D, {0xD5, 0xD8, 0xD6, 0xD7}},
        {0x062E, {0xD9, 0xDC, 0xDA, 0xDB}},
        {0x0633, {0xDD, 0xE0, 0xDE, 0xDF}},
        {0x0634, {0xE1, 0xE4, 0xE2, 0xE3}},
        {0x0635, {0xE5, 0xE8, 0xE6, 0xE7}},
        {0x0636, {0xE9, 0xEC, 0xEA, 0xEB}},
        {0x0637, {0xED, 0xF0, 0xEE, 0xEF}},
        {0x0638, {0xF1, 0xF4, 0xF2, 0xF3}},
        {0x0639, {0xF5, 0xF8, 0xF6, 0xF7}},
        {0x063A, {0xF9, 0xFC, 0xFA, 0xFB}},
        {0x0641, {0xFD, 0x7F, 0xFE, 0xFF}},
        {0x0642, {0x80, 0x83, 0x81, 0x82}},
        {0x0643, {0x84, 0x87, 0x85, 0x86}},
        {0x0644, {0x88, 0x8B, 0x89, 0x8A}},
        {0x0645, {0x8C, 0x8F, 0x8D, 0x8E}},
        {0x0646, {0x90, 0x93, 0x91, 0x92}},
        {0x0647, {0x94, 0x97, 0x95, 0x96}},
        {0x064A, {0x98, 0x9B, 0x99, 0x9A}},
        {0x0626, {0x9C, 0x5B, 0x9D, 0x9E}},
    };
    return m;
}

inline std::vector<uint32_t> DecodeUTF8(const std::string& s) {
    std::vector<uint32_t> out;
    for (size_t i = 0; i < s.size();) {
        uint8_t c = static_cast<uint8_t>(s[i]);
        if (c < 0x80) { out.push_back(c); i += 1; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
            out.push_back(((c & 0x1F) << 6) | (static_cast<uint8_t>(s[i+1]) & 0x3F));
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
            out.push_back(((c & 0x0F) << 12) | ((static_cast<uint8_t>(s[i+1]) & 0x3F) << 6) | (static_cast<uint8_t>(s[i+2]) & 0x3F));
            i += 3;
        } else { i += 1; }
    }
    return out;
}

inline std::string ShapeArabicUTF8ToSoH(const char* utf8) {
    if (!utf8) return "";
    auto cps = DecodeUTF8(utf8);
    const auto& map = GetArabicMap();
    std::string out;
    out.reserve(cps.size());

    auto isTashkeel = [](uint32_t cp) { return (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 || cp == 0x0640; };

    for (size_t i = 0; i < cps.size(); ++i) {
        uint32_t cp = cps[i];
        if (isTashkeel(cp)) continue; // skip diacritics not in font
        if (cp == 0x060C) { out.push_back(static_cast<char>(0x7E)); continue; }
        if (cp == 0x061F) { out.push_back(static_cast<char>(0x3C)); continue; }
        if (cp == 0x061B) { out.push_back(static_cast<char>(0x3E)); continue; }

        auto it = map.find(cp);
        if (it == map.end()) {
            out.push_back(static_cast<char>(cp & 0xFF));
            continue;
        }

        // Check previous connecting letter
        bool prevConn = false;
        if (i > 0) {
            auto pit = map.find(cps[i - 1]);
            if (pit != map.end() && pit->second.connectsNext()) prevConn = true;
        }

        // Handle Lam-Alef ligature
        if (cp == 0x0644 && i + 1 < cps.size()) {
            uint32_t nxt = cps[i + 1];
            uint8_t la_isol = 0, la_fina = 0;
            if (nxt == 0x0627) { la_isol = 0x5C; la_fina = 0x5D; }
            if (nxt == 0x0623) { la_isol = 0x5E; la_fina = 0x5F; }
            if (nxt == 0x0625) { la_isol = 0x60; la_fina = 0x7B; }
            if (nxt == 0x0622) { la_isol = 0x7C; la_fina = 0x7D; }
            if (la_isol != 0) {
                out.push_back(static_cast<char>(prevConn ? la_fina : la_isol));
                ++i;
                continue;
            }
        }

        // Check next Arabic letter
        bool nextConn = false;
        if (i + 1 < cps.size()) {
            auto nit = map.find(cps[i + 1]);
            if (nit != map.end()) nextConn = true;
        }

        const auto& f = it->second;
        uint8_t glyph = f.isol;
        if (prevConn && nextConn && f.medi) glyph = f.medi;
        else if (prevConn && f.fina) glyph = f.fina;
        else if (nextConn && f.init) glyph = f.init;
        out.push_back(static_cast<char>(glyph));
    }
    return out;
}

} // namespace SOHArabic