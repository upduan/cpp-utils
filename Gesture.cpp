// #include "pch.h"

#include "Gesture.h"

#include <cmath>

#include "base.h"

namespace util::Gesture {
    namespace {
        Touch oldTouch1;
        Touch oldTouch2;
        float minScale = 0.1f;
        float maxScale = 10.0f;
        glm::mat4 transform;

        rainbow::Point2D operator-(rainbow::Point2D a, rainbow::Point2D b) noexcept {
            return {a.x - b.x, a.y - b.y};
        }

        float Distance(rainbow::Point2D a, rainbow::Point2D b) noexcept {
            float diffX = a.x - b.x;
            float diffY = a.y - b.y;
            return std::sqrt /*f*/ (diffX * diffX + diffY * diffY);
        }

        float Dot(rainbow::Point2D a, rainbow::Point2D b) noexcept {
            return a.x * b.x + a.y * b.y;
        }
    } // namespace

    void init(float const matrix[16]) noexcept {
        transform = glm::make_mat4(matrix);
    }

    void action(std::vector<Touch> touchs) noexcept {
        switch (touchs.size()) {
        case 1:
            // rotate
            transform = glm::rotate(transform, -touchs[0].deltaPosition.x, glm::vec3(0, 1, 0));
            // transform.Rotate(0, -touchs[0].deltaPosition.x, 0);
            break;
        case 2:
            if (touchs[1].phase != TouchPhase::Began) {
                // auto dot = glm::dot(touchs[0].position - oldTouch1.position, touchs[1].position - oldTouch2.position);
                float dot = Dot(touchs[0].position - oldTouch1.position, touchs[1].position - oldTouch2.position);
                if (dot > 0) {
                    // move
                    transform = glm::translate(transform, glm::vec3(touchs[0].deltaPosition.x * 0.001f, touchs[0].deltaPosition.y * 0.001f, 0.0f));
                } else {
                    // scale
                    // 计算老的两点距离和新的两点间距离，变大要放大模型，变小要缩放模型
                    // float oldDistance = glm::distance(oldTouch1.position, oldTouch2.position);
                    // float newDistance = glm::distance(touchs[0].position, touchs[1].position);
                    float oldDistance = Distance(oldTouch1.position, oldTouch2.position);
                    float newDistance = Distance(touchs[0].position, touchs[1].position);
                    // 两个距离之差，为正表示放大手势，为负表示缩小手势
                    float offset = newDistance - oldDistance;
                    // 放大因子， 一个像素按 0.01倍来算(100可调整)
                    float scaleFactor = offset / 1000.0f;
                    auto x = transform[0][0] + scaleFactor;
                    auto y = transform[1][1] + scaleFactor;
                    auto z = transform[2][2] + scaleFactor;
                    // 最小缩放到 0.1 倍 ，最大放大到 10 倍
                    if (x > minScale && y > minScale && z > minScale && x < maxScale && y < maxScale && z < maxScale) {
                        transform = glm::scale(transform, glm::vec3(x, y, z));
                    }
                }
            }
            // 记住最新的触摸点，下次使用
            oldTouch1 = touchs[0];
            oldTouch2 = touchs[1];
            break;
        }
    }

    glm::mat4 getMatrix() noexcept {
        return transform;
    }
} // namespace util::Gesture