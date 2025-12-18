//
// Created by Pulkit Garg on 2025-12-17.
//

#ifndef RASTERIZER_VEC3_H
#define RASTERIZER_VEC3_H

struct vec3 {
    double x, y, z;

    // Constructor to initialize the vector
    vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    // Index operator for accessing elements
    double& operator[](int i) {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        throw std::out_of_range("Index out of range");
    }

    double operator[](int i) const {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        throw std::out_of_range("Index out of range");
    }

    // To print the vector
    friend std::ostream& operator<<(std::ostream& out, const vec3& v) {
        out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return out;
    }
};



#endif //RASTERIZER_VEC3_H