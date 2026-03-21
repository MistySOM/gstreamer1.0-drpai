//
// Created by matin on 22/12/23.
//

#pragma once

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

/**
 * @brief Abstract base class for JSON serialization.
 */
class json_base
{

public:
    /**
     * @brief Virtual destructor for json_base.
     */
    virtual ~json_base() = default;

    /**
     * @brief Concatenates another json_base object to this one.
     * @param j The json_base object to concatenate.
     */
    void concatenate(const json_base &j)
    {
        if (!j.s.empty()) {
            add_comma();
            s += j.s;
        }
    }

    /**
     * @brief Converts the JSON object to a string.
     * @return JSON string representation.
     */
    [[nodiscard]] virtual std::string to_string() const = 0;

protected:
    std::string s; /**< Internal string representation. */

    /**
     * @brief Adds a float value with specified precision.
     * @param value The float value.
     * @param precision Number of decimal places.
     */
    virtual void add(const float value, const int precision)
    {
        switch (precision) {
            case -1:
                s += std::to_string(value);
                return;
            case 0:
                add(static_cast<int>(value));
                return;
            default: {
                std::ostringstream out{};
                out << std::fixed << std::setprecision(precision) << value;
                s += out.str();
            }
        }
    }

    /**
     * @brief Adds a boolean value.
     * @param value The boolean value.
     */
    virtual void add(const bool value) { s += value ? "true" : "false"; }

    /**
     * @brief Adds an integer value.
     * @param value The integer value.
     */
    virtual void add(const int value) { s += std::to_string(value); }

    /**
     * @brief Adds an unsigned integer value.
     * @param value The unsigned integer value.
     */
    virtual void add(const unsigned int value) { s += std::to_string(value); }

    /**
     * @brief Adds an unsigned long value.
     * @param value The unsigned long value.
     */
    virtual void add(const unsigned long value) { s += std::to_string(value); }

    /**
     * @brief Adds a string value.
     * @param value The string value.
     */
    virtual void add(const std::string &value) { s += format_string(value); }

    /**
     * @brief Adds another json_base object.
     * @param value The json_base object.
     */
    virtual void add(const json_base &value) { s += value.to_string(); }

    /**
     * @brief Adds a comma if the internal string is not empty.
     */
    void add_comma()
    {
        if (!s.empty()) {
            s += ", ";
        }
    }

    /**
     * @brief Formats a string for JSON output.
     * @param str The string to format.
     * @return Formatted string.
     */
    [[nodiscard]] std::string static format_string(const std::string &str) { return "\"" + str + "\""; }
};

/**
 * @brief Represents a JSON object with key-value pairs.
 */
class json_object final : public json_base
{
public:
    /**
     * @brief Adds a key-value pair to the JSON object.
     * @tparam T Type of the value.
     * @param key The key.
     * @param value The value.
     */
    template<typename T>
    void add(const std::string &key, const T &value)
    {
        add_key(key);
        json_base::add(value);
    }

    /**
     * @brief Adds a float key-value pair with precision.
     * @param key The key.
     * @param value The float value.
     * @param precision Number of decimal places.
     */
    void add(const std::string &key, const float value, const int precision = -1)
    {
        add_key(key);
        json_base::add(value, precision);
    }

    /**
     * @brief Converts the JSON object to a string.
     * @return JSON string representation.
     */
    [[nodiscard]] std::string to_string() const override { return "{" + s + "}"; }

private:
    using json_base::add;

    /**
     * @brief Adds a formatted key to the JSON object.
     * @param key The key to add.
     */
    void add_key(const std::string &key)
    {
        add_comma();
        s += format_string(format_key(key)) + ": ";
    }

    /**
     * @brief Formats a key by replacing spaces with underscores.
     * @param str The key string.
     * @return Formatted key.
     */
    [[nodiscard]] std::string static format_key(const std::string &str)
    {
        auto k = str;
        std::replace(k.begin(), k.end(), ' ', '_');
        return k;
    }
};

/**
 * @brief Represents a JSON array.
 */
class json_array final : public json_base
{
public:
    /**
     * @brief Default constructor for an empty JSON array.
     */
    json_array() = default;

    /**
     * @brief Constructs a JSON array from a vector of strings.
     * @param array Vector of strings to add.
     */
    explicit json_array(const std::vector<std::string> &array)
    {
        for (const auto &a: array) {
            add_comma();
            json_base::add(a);
        }
    }

    /**
     * @brief Adds a value to the JSON array.
     * @tparam T Type of the value.
     * @param value The value to add.
     */
    template<typename T>
    void add(const T &value)
    {
        add_comma();
        json_base::add(value);
    }

    /**
     * @brief Adds a float value with precision to the JSON array.
     * @param value The float value.
     * @param precision Number of decimal places.
     */
    void add(const float value, const int precision) override
    {
        add_comma();
        json_base::add(value, precision);
    }

    /**
     * @brief Converts the JSON array to a string.
     * @return JSON string representation.
     */
    [[nodiscard]] std::string to_string() const override { return "[" + s + "]"; }

private:
    using json_base::add;
};
