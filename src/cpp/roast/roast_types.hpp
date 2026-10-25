/**
 * @file roast_types.hpp
 * @brief Roast scales (broad five-step and eight Agtron tiles) and their value objects.
 */

#pragma once

namespace cupano {

/** Broad five-step roast scale used for the headline verdict. */
enum class RoastLevel { Unknown = -1, Light = 0, MediumLight = 1, Medium = 2, MediumDark = 3, Dark = 4 };

/**
 * Full eight-tile roast table (SCA/Agtron gourmet tiles #95..#25). Every
 * regional and roastery-specific term maps onto exactly one tile.
 */
enum class RoastStage {
    Unknown = -1,
    VeryLight = 0,       // Agtron #95
    Light = 1,           // #85  Cinnamon, Nordic
    ModeratelyLight = 2, // #75  New England, Half City, Omni
    LightMedium = 3,     // #65  American, High, Blonde
    Medium = 4,          // #55  City
    ModeratelyDark = 5,  // #45  Full City, City+
    Dark = 6,            // #35  Full City+, Vienna, French
    VeryDark = 7         // #25  Italian, Spanish
};

/** Result of mapping a roast term, Agtron value or roaster scale onto a tile. */
struct RoastClassification {
    RoastStage stage = RoastStage::Unknown;
};

/** A position on a roaster's own N-step scale ("3/5", "●●●○○"). */
struct ScalePosition {
    int position = 0;
    int size = 0;
};

}  // namespace cupano
