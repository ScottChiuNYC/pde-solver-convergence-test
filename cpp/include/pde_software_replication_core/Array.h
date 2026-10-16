#ifndef PDE_SOFTWARE_REPLICATION_CORE_ARRAY_H
#define PDE_SOFTWARE_REPLICATION_CORE_ARRAY_H
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace PDE {
class Array1D {
public:
    using iterator = std::vector<double>::iterator;
    using const_iterator = std::vector<double>::const_iterator;
    Array1D() = default;
    explicit Array1D(std::size_t size, double value = 0.0) : m_data(size, value) {}
    Array1D(Array1D const&) = default;
    Array1D(Array1D&& other) { m_data.swap(other.m_data); }
    Array1D& operator=(Array1D const& other) {
        if (this != &other) { Array1D copy(other); swap(copy); }
        return *this;
    }
    Array1D& operator=(Array1D&& other) {
        if (this != &other) { Array1D moved(std::move(other)); swap(moved); }
        return *this;
    }
    std::size_t size() const { return m_data.size(); }
    double& operator[](std::size_t i) { return at(i); }
    double const& operator[](std::size_t i) const { return at(i); }
    double& at(std::size_t i) {
        if (i >= size()) throw std::runtime_error("array index out of bounds: " + std::to_string(i));
        return m_data[i];
    }
    double const& at(std::size_t i) const {
        if (i >= size()) throw std::runtime_error("array index out of bounds: " + std::to_string(i));
        return m_data[i];
    }
    void fill(double value) { std::fill(m_data.begin(), m_data.end(), value); }
    void swap(Array1D& other) { m_data.swap(other.m_data); }
    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
    double* data() { return m_data.empty() ? nullptr : m_data.data(); }
    double const* data() const { return m_data.empty() ? nullptr : m_data.data(); }
private:
    std::vector<double> m_data;
};
} // namespace PDE
#endif
