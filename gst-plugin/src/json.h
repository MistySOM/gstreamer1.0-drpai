//
// Created by matin on 22/12/23.
//

#ifndef JSON_H
#define JSON_H

#include <iomanip>
#include <sstream>

class json_base {

public:
    virtual ~json_base() = default;
    void concatenate(const json_base& j) { if(!j.s.empty()) { add_comma(); s += j.s; } }

    [[nodiscard]] virtual std::string to_string() const = 0;

protected:
    std::string s;

    virtual void add(const float value, const int precision) {
        switch (precision) {
            case -1: s += std::to_string(value); return;
            case 0: add(static_cast<int>(value)); return;
            default: {
                std::ostringstream out;
                out << std::fixed << std::setprecision(precision) << value;
                s += out.str();
            }
        }
    }
    virtual void add(const int value) { s += std::to_string(value); }
    virtual void add(const unsigned int value) { s += std::to_string(value); }
    virtual void add(const unsigned long value) { s += std::to_string(value); }
    virtual void add(const std::string& value) { s += format_string(value); }
    virtual void add(const json_base& value) { s += value.to_string(); }
    void add_comma() { if (!s.empty()) s += ", "; }

    [[nodiscard]] std::string static format_string(const std::string& str) { return "\"" + str + "\""; }
};

class json_object final: public json_base {
public:
    void add(const std::string& key, const float value, const int precision=-1) { add_key(key); json_base::add(value, precision); }
    void add(const std::string& key, const int value) { add_key(key); json_base::add(value); }
    void add(const std::string& key, const unsigned int value) { add_key(key); json_base::add(value); }
    void add(const std::string& key, const unsigned long value) { add_key(key); json_base::add(value); }
    void add(const std::string& key, const std::string& value) { add_key(key); json_base::add(value); }
    void add(const std::string& key, const json_base& value) { add_key(key); json_base::add(value); }

    [[nodiscard]] std::string to_string() const override { return "{" + s + "}"; }

private:
    using json_base::add;
    void add_key(const std::string& key) { add_comma(); s += format_string(key) + ": "; }
};

class json_array final: public json_base {
public:
    void add(const float value, const int precision) override { add_comma(); json_base::add(value, precision); }
    void add(const int value) override { add_comma(); json_base::add(value); }
    void add(const unsigned int value) override { add_comma(); json_base::add(value); }
    void add(const unsigned long value) override { add_comma(); json_base::add(value); }
    void add(const std::string& value) override { add_comma(); json_base::add(value); }
    void add(const json_base& value) override { add_comma(); json_base::add(value); }

    [[nodiscard]] std::string to_string() const override { return "[" + s + "]"; }
};

#endif //JSON_H
