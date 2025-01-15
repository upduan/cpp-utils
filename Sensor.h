#pragma once

#include <vector> // for std::vector<>

namespace util::Sensor {
    struct SensorData {
        float x;
        float y;
        float z;
    };

    struct ComplexSensorData {
        SensorData accelerator;
        SensorData gyroscope;
    };

    struct Combination {
        double timestamp;
        ComplexSensorData data;
        // SensorData accelerator;
        // SensorData gyroscope;
    };

    void init(int frameFreq, int sensorFreq) noexcept;

    std::vector<Combination> getData(int start, int stop) noexcept;

    void uninit() noexcept;
} // namespace util::Sensor
