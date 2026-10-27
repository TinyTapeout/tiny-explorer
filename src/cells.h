#pragma once

#include <vector>
#include <map>
#include <gdstk/gdstk.hpp>
#include "geom.h"
#include "fetsim.h"


enum LayerID {
    L_ERROR = 0,
    L_NWELL,
    L_DIFF,
    L_CHANNEL,  // created from L_DIFF & L_POLY
    L_N_TERM, L_P_TERM, // created from L_DIFF - L_POLY (using L_NWELL)
    L_POLY,     // FET Gates
    L_LICON,    // connects L_LI1 to both L_POLY and L_N_TERM/L_P_TERM
    L_LI1,  L_MCON,
    L_MET1, L_VIA1,
    L_MET2, L_VIA2,
    L_MET3, L_VIA3,
    L_MET4, L_VIA4,
    L_MET5,
    L_CELLS,
    L_COUNT
};

struct QuadLayer {
    uint32_t rectStart = 0;
    uint32_t rectCount = 0;
    std::vector<BVHNode> bvh;
};

struct Cell {
    std::string name;
    std::vector<Rect> rects;
    std::array<QuadLayer, L_COUNT> layers; // Indexed by LayerID
    Rect bbox;
    DSU wireDSU;
    std::vector<int> rect2wire; // Mapping from rect index to dense wire ID
    uint32_t wireCount = 0;
    struct Label { int32_t rectIdx; LayerID layerId; };
    std::map<std::string, Label> labels;
    std::vector<FET> fets;
    std::vector<int> outputWires;
    
    // Special Nets
    int32_t groundRect = -1;
    int32_t powerRect = -1;
    bool isFiller = false;
};

inline bool isFillerCell(const std::string& name) {
    // IHP sg13g2 / sg13cmos5l:
    if (name.starts_with("sg13g2_") || name.starts_with("sg13cmos5l_")) {
        if (name.find("_fill_") != std::string::npos || name.find("_decap_") != std::string::npos) {
            return true;
        }
    }
    // Skywater 130 and GF180MCU:
    return (
        name.find("__fill") != std::string::npos || 
        name.find("__decap") != std::string::npos || 
        name.find("__tap") != std::string::npos ||
        name.find("__endcap") != std::string::npos
    );
}

inline bool isStandardCell(const std::string& name) {
    return name.starts_with("sg13g2_") | name.starts_with("sg13cmos5l_") | name.starts_with("sky130_") | name.starts_with("gf180mcu_");
}

inline bool isGroundLabel(const std::string& label) {
    static const std::vector<std::string> gndLabels = {"VGND", "VSS", "vssd1"};
    for (const auto& l : gndLabels) if (label == l) return true;
    return false;
}

inline bool isPowerLabel(const std::string& label) {
    static const std::vector<std::string> pwrLabels = {"VPWR", "VDPWR", "VDD", "vccd1"};
    for (const auto& l : pwrLabels) if (label == l) return true;
    return false;
}

using LayerMap = std::map<std::pair<int16_t, int16_t>, LayerID>;
using PdkMap = std::map<std::string, LayerMap>;
inline const PdkMap& getPdkMaps() {
    static const PdkMap PdkMaps = {
        {"sky130A", {
            {{64,20}, L_NWELL}, {{65,20}, L_DIFF},
            {{66,20}, L_POLY}, {{66,44}, L_LICON},
            {{67,20}, L_LI1 }, {{67,44}, L_MCON}, {{67, 5}, L_LI1}, // labels
            {{68,20}, L_MET1}, {{68,44}, L_VIA1}, {{68, 5}, L_MET1}, // labels
            {{69,20}, L_MET2}, {{69,44}, L_VIA2}, {{69, 5}, L_MET2}, // labels
            {{70,20}, L_MET3}, {{70,44}, L_VIA3}, {{70, 5}, L_MET3}, // labels
            {{71,20}, L_MET4}, {{71,44}, L_VIA4}, {{71, 5}, L_MET4}, // labels
            {{72,20}, L_MET5}
        }},
        {"ihp-sg13g2", {
            {{31, 0}, L_NWELL}, {{ 1, 0}, L_DIFF},
            {{ 5, 0}, L_POLY}, {{ 6, 0}, L_LICON},
            {{ 8, 0}, L_LI1},  {{19, 0}, L_MCON},
            {{ 8, 1}, L_LI1},  {{ 8, 2}, L_LI1}, {{ 8,25}, L_LI1}, // labels
            {{10, 0}, L_MET1}, {{29, 0}, L_VIA1}, {{10,25}, L_MET1}, // labels
            {{30, 0}, L_MET2}, {{30, 25}, L_MET2}, {{49, 0}, L_VIA2},
            {{50, 0}, L_MET3}, {{50, 25}, L_MET3}, {{66, 0}, L_VIA3},
            {{67, 0}, L_MET4}, {{67,25}, L_MET4}, // labels
            {{125, 0}, L_VIA4},
            {{126, 0}, L_MET5}, {{126, 2}, L_MET5}, {{126, 25}, L_MET5} // labels
        }},
        // Same layers as ihp-sg13g2, but without Metal5 / Via4: TopVia1 sits directly on Metal4
        {"ihp-sg13cmos5l", {
            {{31, 0}, L_NWELL}, {{ 1, 0}, L_DIFF},
            {{ 5, 0}, L_POLY}, {{ 6, 0}, L_LICON},
            {{ 8, 0}, L_LI1},  {{19, 0}, L_MCON},
            {{ 8, 1}, L_LI1},  {{ 8, 2}, L_LI1}, {{ 8,25}, L_LI1}, // labels
            {{10, 0}, L_MET1}, {{29, 0}, L_VIA1}, {{10,25}, L_MET1}, // labels
            {{30, 0}, L_MET2}, {{30, 25}, L_MET2}, {{49, 0}, L_VIA2},
            {{50, 0}, L_MET3}, {{50, 25}, L_MET3}, {{125, 0}, L_VIA3},
            {{126, 0}, L_MET4}, {{126, 2}, L_MET4}, {{126, 25}, L_MET4} // labels
        }},
        {"gf180mcuD", {
            {{21, 0}, L_NWELL}, {{22, 0}, L_DIFF},
            {{30, 0}, L_POLY}, {{33, 0}, L_LICON},
            // gf180mcuD calls it met1, but I call it LI1 to match other PDKs
            {{34, 0}, L_LI1},  {{35, 0}, L_MCON}, {{34,10}, L_LI1}, // labels
            {{36, 0}, L_MET1}, {{38, 0}, L_VIA1},
            {{42, 0}, L_MET2}, {{40, 0}, L_VIA2},
            {{46, 0}, L_MET3}, {{41, 0}, L_VIA3}, {{46,10}, L_MET3}, // labels
            {{81, 0}, L_MET4}
        }}
    };
    return PdkMaps;
}

inline LayerID tag2id(gdstk::Tag tag, const std::string& pdk) {
    const auto& PdkMaps = getPdkMaps();

    auto pdkIt = PdkMaps.find(pdk);
    if (pdkIt == PdkMaps.end()) return L_COUNT;

    auto& map = pdkIt->second;
    auto tagIt = map.find({gdstk::get_layer(tag), gdstk::get_type(tag)});
    return (tagIt != map.end()) ? tagIt->second : L_COUNT;
}

inline const char* getLayerName(LayerID id) {
    static const char* Names[] = {
        "ERROR", "NWELL", "DIFF", "CHANNEL", 
        "N_TERM", "P_TERM", "POLY", "LICON", "LI1", "MCON",
        "MET1", "VIA1", "MET2", "VIA2", "MET3", "VIA3",
        "MET4", "VIA4", "MET5", "CELLS"
    };
    if (id >= 0 && id < L_COUNT) return Names[id];
    return "UNKNOWN";
}
