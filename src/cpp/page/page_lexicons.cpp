/**
 * @file page_lexicons.cpp
 * @brief Fixed page-reading lexicons.
 */

#include "page/page_lexicons.hpp"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

namespace cupano {

LabelSet makeLabelSet(const TextNormalizer& normalizer, std::vector<std::string> labels) {
    LabelSet set;
    for (const std::string& label : labels) {
        set.labels.push_back(normalizer.foldCase(TextFragment{label}).text);
    }
    std::stable_sort(set.labels.begin(), set.labels.end(),
                     [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
    return set;
}

std::vector<AliasEntry<std::string>> buildCountryEntries() {
    std::vector<AliasEntry<std::string>> e;
    auto add = [&e](const char* name, std::initializer_list<const char*> aliases) {
        e.push_back(AliasEntry<std::string>{name, name});
        addAliases(e, std::string(name), aliases);
    };
    add("Ethiopia", {"에티오피아", "エチオピア", "衣索比亞", "埃塞俄比亚", "etiopia", "äthiopien"});
    add("Kenya", {"케냐", "ケニア", "肯亞", "肯尼亚", "kenia"});
    add("Colombia", {"콜롬비아", "コロンビア", "哥倫比亞", "哥伦比亚", "kolumbien"});
    add("Panama", {"파나마", "パナマ", "巴拿馬", "巴拿马"});
    add("Guatemala", {"과테말라", "グアテマラ", "瓜地馬拉", "危地马拉"});
    add("Costa Rica", {"코스타리카", "コスタリカ", "哥斯大黎加", "哥斯达黎加"});
    add("Brazil", {"브라질", "ブラジル", "巴西", "brasil", "brasilien"});
    add("Rwanda", {"르완다", "ルワンダ", "盧安達", "卢旺达", "ruanda"});
    add("Burundi", {"부룬디", "ブルンジ", "蒲隆地", "布隆迪"});
    add("Honduras", {"온두라스", "ホンジュラス", "宏都拉斯", "洪都拉斯"});
    add("El Salvador", {"엘살바도르", "エルサルバドル", "薩爾瓦多", "萨尔瓦多"});
    add("Nicaragua", {"니카라과", "ニカラグア", "尼加拉瓜"});
    add("Peru", {"페루", "ペルー", "秘魯", "秘鲁"});
    add("Bolivia", {"볼리비아", "ボリビア", "玻利維亞", "bolivien"});
    add("Ecuador", {"에콰도르", "エクアドル", "厄瓜多"});
    add("Mexico", {"멕시코", "メキシコ", "墨西哥", "mexiko"});
    add("Indonesia", {"인도네시아", "インドネシア", "印尼", "indonesien", "sumatra", "수마트라", "スマトラ"});
    add("Yemen", {"예멘", "イエメン", "葉門", "也门", "jemen"});
    add("Tanzania", {"탄자니아", "タンザニア", "坦尚尼亞", "tansania"});
    add("Uganda", {"우간다", "ウガンダ", "烏干達"});
    add("Taiwan", {"대만", "台湾", "台灣", "阿里山"});
    add("China", {"윈난", "雲南", "云南", "yunnan"});
    add("India", {"인도", "インド", "印度", "indien"});
    add("Papua New Guinea", {"파푸아뉴기니", "パプアニューギニア", "巴布亞紐幾內亞", "papua-neuguinea", "papua ny-guinea"});
    return e;
}

std::vector<AliasEntry<std::string>> buildProcessEntries() {
    std::vector<AliasEntry<std::string>> e;
    addAliases(e, std::string("Anaerobic"),
               {"anaerobic", "무산소", "애너로빅", "アナエロビック", "嫌気性", "厭氧", "厌氧", "anaerob"});
    addAliases(e, std::string("Carbonic Maceration"),
               {"carbonic maceration", "카보닉", "カーボニック", "二氧化碳浸漬", "karbonisk", "kohlensäuremazeration"});
    addAliases(e, std::string("Honey"),
               {"honey", "허니", "ハニー", "蜜處理", "蜜处理", "honning", "honig"});
    addAliases(e, std::string("Natural"),
               {"natural", "내추럴", "내츄럴", "ナチュラル", "日曬", "日晒", "非水洗", "naturlig", "natur",
                "trocken aufbereitet", "naturbelassen"});
    addAliases(e, std::string("Washed"),
               {"washed", "fully washed", "워시드", "수세식", "ウォッシュト", "水洗", "vasket", "gewaschen",
                "nass aufbereitet"});
    addAliases(e, std::string("Wet-hulled"),
               {"wet hulled", "wet-hulled", "giling basah", "웻헐", "スマトラ式", "濕剝", "湿刨"});
    return e;
}

std::vector<AliasEntry<std::string>> buildPromoEntries() {
    std::vector<AliasEntry<std::string>> e;
    const std::string marker = "promo";
    addAliases(e, marker,
               {"free shipping", "best seller", "bestseller", "sale", "limited", "sold out", "in stock",
                "무료배송", "무배", "특가", "할인", "신상", "베스트", "당일발송", "품절", "送料無料", "セール", "限定",
                "新商品", "数量限定", "免運", "免运费", "特價", "特价", "限量", "現貨", "gratis frakt", "tilbud",
                "nyhet", "utsolgt", "kostenloser versand", "versandkostenfrei", "angebot", "ausverkauft"});
    return e;
}

std::vector<AliasEntry<std::string>> buildCoffeeSignalEntries() {
    std::vector<AliasEntry<std::string>> e;
    const std::string marker = "coffee";
    addAliases(e, marker,
               {"coffee", "espresso", "single origin", "roastery", "roasters", "커피", "원두", "로스터리",
                "コーヒー", "珈琲", "焙煎", "咖啡", "咖啡豆", "烘焙", "kaffe", "kaffebønner", "kaffebrenneri",
                "kaffee", "kaffeebohnen", "rösterei", "röstgrad"});
    return e;
}

}  // namespace cupano
