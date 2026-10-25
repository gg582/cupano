/**
 * @file flavor_wheel_table.cpp
 * @brief Segment names of the SCA / WCR Coffee Taster's Flavor Wheel (2016).
 *
 * Source: Specialty Coffee Association & World Coffee Research, "Coffee
 * Taster's Flavor Wheel" (2016), built on the WCR Sensory Lexicon. Only the
 * taxonomy (segment names and nesting) is encoded here.
 */

#include "wheel/flavor_wheel_table.hpp"

namespace cupano {

std::vector<WheelCategorySpec> buildFlavorWheelSpec() {
    return {
        {"Floral", {
            {"Black Tea", {}},
            {"Floral", {"Chamomile", "Rose", "Jasmine"}},
        }},
        {"Fruity", {
            {"Berry", {"Blackberry", "Raspberry", "Blueberry", "Strawberry"}},
            {"Dried Fruit", {"Raisin", "Prune"}},
            {"Other Fruit", {"Coconut", "Cherry", "Pomegranate", "Pineapple", "Grape", "Apple", "Peach", "Pear"}},
            {"Citrus Fruit", {"Grapefruit", "Orange", "Lemon", "Lime"}},
        }},
        {"Sour/Fermented", {
            {"Sour", {"Sour Aromatics", "Acetic Acid", "Butyric Acid", "Isovaleric Acid", "Citric Acid", "Malic Acid"}},
            {"Alcohol/Fermented", {"Winey", "Whiskey", "Fermented", "Overripe"}},
        }},
        {"Green/Vegetative", {
            {"Olive Oil", {}},
            {"Raw", {}},
            {"Green/Vegetative", {"Under-ripe", "Peapod", "Fresh", "Dark Green", "Vegetative", "Hay-like", "Herb-like"}},
            {"Beany", {}},
        }},
        {"Other", {
            {"Papery/Musty", {"Stale", "Cardboard", "Papery", "Woody", "Moldy/Damp", "Musty/Dusty", "Musty/Earthy",
                              "Animalic", "Meaty Brothy", "Phenolic"}},
            {"Chemical", {"Bitter", "Salty", "Medicinal", "Petroleum", "Skunky", "Rubber"}},
        }},
        {"Roasted", {
            {"Pipe Tobacco", {}},
            {"Tobacco", {}},
            {"Burnt", {"Acrid", "Ashy", "Smoky", "Brown, Roast"}},
            {"Cereal", {"Grain", "Malt"}},
        }},
        {"Spices", {
            {"Pungent", {}},
            {"Pepper", {}},
            {"Brown Spice", {"Anise", "Nutmeg", "Cinnamon", "Clove"}},
        }},
        {"Nutty/Cocoa", {
            {"Nutty", {"Peanuts", "Hazelnut", "Almond"}},
            {"Cocoa", {"Chocolate", "Dark Chocolate"}},
        }},
        {"Sweet", {
            {"Brown Sugar", {"Molasses", "Maple Syrup", "Caramelized", "Honey"}},
            {"Vanilla", {}},
            {"Vanillin", {}},
            {"Overall Sweet", {}},
            {"Sweet Aromatics", {}},
        }},
    };
}

}  // namespace cupano
