#pragma once

#include <tuple>
#include <algorithm>

template <typename T> inline T minthree(T x, T y, T z) {
    return std::min(std::min(x, y), z);
}

std::tuple<double, double, double> rgb2lab(int R, int G, int B) {
    double x = 0.5164 * R + 0.2789 * G + 0.1792 * B;
    x = x / 95.045;

    double y = 0.2963 * R + 0.6192 * G + 0.0845 * B;
    y = y / 100.000;

    double z = 0.0339 * R + 0.1426 * G + 1.0166 * B;
    z = z / 108.255;

    double L, a, b;
    if (minthree(x, y, z) < 0.008856) {
        L = 116 * pow(x, 1 / 3) - 16;
        a = 500 * (pow(x, 1 / 3) - pow(y, 1 / 3));
        b = 200 * (pow(y, 1 / 3) - pow(z, 1 / 3));
    } else {
        L = 903.0 * y;
        a = 3893.5 * (x - y);
        b = 1557.4 * (y - z);
    }
    return {L, a, b};
}

double MedValue(double buf[], int n, int m) {
    std::sort(buf, buf + n);
    // // double k;
    // for (int i = 1; i < n; i++) { // 冒泡排序
    //     int f = 0; // 交换标志清零
    //     for (int j = n - 1; j >= i; j--) {
    //         if (buf[j] > buf[j + 1]) { // 比较
    //             std::swap(buf[j], buf[j + 1]);
    //             // k = buf[j];
    //             // buf[j] = buf[j + 1]; // 两数交换，置标志
    //             // buf[j + 1] = k;
    //             f = 1;
    //         }
    //     }
    //     if (f == 0)
    //         break; // 无交换，退出
    // }
    return buf[m]; // 取中值返回
}
