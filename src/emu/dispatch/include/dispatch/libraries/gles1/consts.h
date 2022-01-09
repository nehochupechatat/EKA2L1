#pragma once

namespace eka2l1::dispatch {
    enum {
        // Most GLES1 Symbian phone seems to only supported up to 2 slots (even iPhone 3G is so).
        // But be three for safety. Always feel 2 is too limited.
        GLES1_EMU_MAX_TEXTURE_SIZE = 4096,
        GLES1_EMU_MAX_TEXTURE_MIP_LEVEL = 10,
        GLES1_EMU_MAX_TEXTURE_COUNT = 3,
        GLES1_EMU_MAX_LIGHT = 8,
        GLES1_EMU_MAX_CLIP_PLANE = 6
    };
}