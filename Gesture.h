#pragma once

#include <vector> // for std::vector<>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "base.h"

namespace util::Gesture {
    enum class TouchPhase { Began, Move, End };

    struct Touch {
        rainbow::Point2D position;
        rainbow::Point2D deltaPosition;
        TouchPhase phase;
    };

    void init(float const matrix[16]) noexcept;

    void action(std::vector<Touch> touchs) noexcept;

    glm::mat4 getMatrix() noexcept;
} // namespace util::Gesture
