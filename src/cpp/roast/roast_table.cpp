/**
 * @file roast_table.cpp
 * @brief The fixed multilingual roast table.
 */

#include "roast/roast_table.hpp"

#include <vector>

namespace cupano {

std::vector<AliasEntry<RoastStage>> buildRoastEntries() {
    std::vector<AliasEntry<RoastStage>> e;
    // Aliases are matched longest-first, so compound terms ("full city+",
    // "중강배전", "中深煎り", "mitteldunkel") always win over their substrings.
    // Each tile lists: canonical tile names, classic US/JP stage names, then
    // KO / JA / ZH-TW(+zh-CN) / NO / DE vocabulary and roastery dialects.

    // Tile #95
    addAliases(e, RoastStage::VeryLight,
               {"very light", "extra light", "ultra light", "ultra-light", "extremely light", "white roast",
                "극약배전", "초약배전", "익스트림 라이트", "울트라 라이트",
                "極浅煎り", "超浅煎り", "ごく浅煎り",
                "極淺焙", "極淺烘焙", "极浅焙", "極淺",
                "ekstra lys", "ekstra lysbrent", "veldig lys", "veldig lysbrent",
                "sehr hell", "extra hell", "ultrahell"});
    // Tile #85
    addAliases(e, RoastStage::Light,
               {"light", "light roast", "lightly roasted", "cinnamon", "cinnamon roast", "nordic", "nordic roast",
                "nordic style", "scandinavian roast", "filter roast", "filter", "for filter",
                "약배전", "라이트", "라이트 로스팅", "라이트 로스트", "시나몬", "시나몬 로스트", "노르딕",
                "노르딕 로스팅", "필터 로스팅", "필터용", "필터용 로스팅",
                "浅煎り", "浅煎", "浅め", "ライト", "ライトロースト", "シナモン", "シナモンロースト", "北欧焙煎",
                "北欧風浅煎り", "フィルターロースト",
                "淺焙", "淺烘焙", "浅焙", "浅烘焙", "肉桂烘焙", "北歐烘焙", "北欧烘焙", "北歐式烘焙",
                "lys", "lysbrent", "lys brenning", "lysristet", "filterbrent", "filterbrenning", "nordisk brenning",
                "hell", "helle röstung", "hell geröstet", "filterröstung", "filter-röstung",
                "skandinavische röstung", "nordische röstung", "zimtröstung"});
    // Tile #75
    addAliases(e, RoastStage::ModeratelyLight,
               {"moderately light", "medium light", "medium-light", "half city", "new england", "new england roast",
                "omni", "omni roast", "omni-roast", "filter and espresso", "filter & espresso", "filter/espresso",
                "약중배전", "중약배전", "미디엄 라이트", "미디엄라이트", "하프시티", "하프 시티", "뉴잉글랜드",
                "옴니", "옴니 로스팅", "옴니 로스트",
                "中浅煎り", "やや浅煎り", "浅めの中煎り", "ミディアム", "ミディアムロースト", "ハーフシティ",
                "オムニロースト",
                "淺中焙", "中淺焙", "淺中烘焙", "中淺烘焙", "浅中焙", "中浅焙", "新英格蘭烘焙", "新英格兰烘焙",
                "middels lys", "middels lysbrent", "omnibrent", "omni-brent", "omnibrenning",
                "mittelhell", "hell bis mittel", "omniröstung", "omni-röstung"});
    // Tile #65
    addAliases(e, RoastStage::LightMedium,
               {"light medium", "light-medium", "american", "american roast", "regular roast", "breakfast roast",
                "blonde", "blonde roast", "high", "high roast", "espresso roast", "espresso", "for espresso",
                "하이", "하이 로스트", "하이 로스팅", "블론드", "블론드 로스트", "아메리칸 로스트",
                "에스프레소 로스팅", "에스프레소용", "에스프레소용 로스팅",
                "ハイ", "ハイロースト", "アメリカンロースト", "エスプレッソロースト",
                "美式烘焙", "金黃烘焙", "高度烘焙", "義式濃縮烘焙",
                "espressobrent", "espressobrenning",
                "espressoröstung", "espresso-röstung", "amerikanische röstung"});
    // Tile #55
    addAliases(e, RoastStage::Medium,
               {"medium", "medium roast", "city", "city roast",
                "중배전", "미디엄", "미디엄 로스팅", "미디엄 로스트", "시티", "시티 로스트", "시티 로스팅",
                "中煎り", "中煎", "シティ", "シティロースト",
                "中焙", "中烘焙", "城市烘焙", "城市烘培",
                "mellombrent", "middels", "medium brent", "mellomristet", "middels brent",
                "mittel", "mittlere röstung", "mittelstark", "medium-röstung"});
    // Tile #45
    addAliases(e, RoastStage::ModeratelyDark,
               {"moderately dark", "medium dark", "medium-dark", "full city", "full-city", "city+", "city plus",
                "중강배전", "미디엄 다크", "미디엄다크", "풀시티", "풀 시티", "시티 플러스", "시티플러스",
                "中深煎り", "やや深煎り", "フルシティ", "フルシティロースト", "シティプラス",
                "中深焙", "中深烘焙", "深城市烘焙", "全城市烘焙", "中深",
                "middels mørk", "middels mørkbrent",
                "mitteldunkel", "mittel-dunkel", "kräftig", "kräftige röstung"});
    // Tile #35
    addAliases(e, RoastStage::Dark,
               {"dark", "dark roast", "full city+", "full city plus", "full-city+", "vienna", "vienna roast",
                "viennese", "continental", "continental roast", "after dinner", "french", "french roast",
                "강배전", "다크", "다크 로스팅", "다크 로스트", "풀시티 플러스", "풀시티+", "비엔나", "비엔나 로스트",
                "프렌치", "프렌치 로스트",
                "深煎り", "深煎", "フルシティプラス", "ウィーン", "ウィーンロースト", "フレンチ", "フレンチロースト",
                "深焙", "深烘焙", "維也納烘焙", "维也纳烘焙", "法式烘焙", "法式",
                "mørk", "mørkbrent", "mørkristet", "fransk brenning",
                "dunkel", "dunkle röstung", "dunkel geröstet", "wiener röstung", "französische röstung", "stark"});
    // Tile #25
    addAliases(e, RoastStage::VeryDark,
               {"very dark", "extra dark", "italian", "italian roast", "spanish", "spanish roast", "new orleans",
                "neapolitan", "neapolitan roast",
                "극강배전", "이탈리안", "이탈리안 로스트", "스패니시", "스페니쉬",
                "極深煎り", "超深煎り", "イタリアン", "イタリアンロースト", "スパニッシュ", "スパニッシュロースト",
                "極深焙", "極深烘焙", "极深焙", "義式烘焙", "意式烘焙", "西班牙烘焙",
                "ekstra mørk", "veldig mørk", "italiensk brenning",
                "sehr dunkel", "extra dunkel", "italienische röstung", "neapolitanische röstung"});
    return e;
}

}  // namespace cupano
