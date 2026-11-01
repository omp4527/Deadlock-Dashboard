#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <iostream>

namespace SimpleJson {

enum class JsonType { Null, Boolean, Number, String, Array, Object };

class JsonValue {
public:
    JsonType type = JsonType::Null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val = "";
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Boolean), bool_val(b) {}
    JsonValue(int n) : type(JsonType::Number), num_val(n) {}
    JsonValue(double n) : type(JsonType::Number), num_val(n) {}
    JsonValue(const char* s) : type(JsonType::String), str_val(s) {}
    JsonValue(const std::string& s) : type(JsonType::String), str_val(s) {}
    JsonValue(const std::vector<JsonValue>& a) : type(JsonType::Array), arr_val(a) {}
    JsonValue(const std::map<std::string, JsonValue>& o) : type(JsonType::Object), obj_val(o) {}

    bool is_null() const { return type == JsonType::Null; }
    bool is_bool() const { return type == JsonType::Boolean; }
    bool is_number() const { return type == JsonType::Number; }
    bool is_string() const { return type == JsonType::String; }
    bool is_array() const { return type == JsonType::Array; }
    bool is_object() const { return type == JsonType::Object; }

    int as_int(int default_val = 0) const {
        if (type == JsonType::Number) return static_cast<int>(num_val);
        return default_val;
    }

    double as_double(double default_val = 0.0) const {
        if (type == JsonType::Number) return num_val;
        return default_val;
    }

    bool as_bool(bool default_val = false) const {
        if (type == JsonType::Boolean) return bool_val;
        return default_val;
    }

    std::string as_string(const std::string& default_val = "") const {
        if (type == JsonType::String) return str_val;
        return default_val;
    }

    bool has_key(const std::string& key) const {
        if (type != JsonType::Object) return false;
        return obj_val.find(key) != obj_val.end();
    }

    JsonValue get(const std::string& key) const {
        if (type == JsonType::Object) {
            auto it = obj_val.find(key);
            if (it != obj_val.end()) return it->second;
        }
        return JsonValue();
    }

    std::vector<int> as_vector_int() const {
        std::vector<int> res;
        if (type == JsonType::Array) {
            for (const auto& item : arr_val) {
                res.push_back(item.as_int());
            }
        }
        return res;
    }

    std::vector<std::string> as_vector_string() const {
        std::vector<std::string> res;
        if (type == JsonType::Array) {
            for (const auto& item : arr_val) {
                res.push_back(item.as_string());
            }
        }
        return res;
    }

    std::vector<std::vector<int>> as_matrix_int() const {
        std::vector<std::vector<int>> res;
        if (type == JsonType::Array) {
            for (const auto& row : arr_val) {
                res.push_back(row.as_vector_int());
            }
        }
        return res;
    }

    std::string serialize() const {
        switch (type) {
            case JsonType::Null: return "null";
            case JsonType::Boolean: return bool_val ? "true" : "false";
            case JsonType::Number: {
                if (num_val == static_cast<int>(num_val)) {
                    return std::to_string(static_cast<int>(num_val));
                }
                return std::to_string(num_val);
            }
            case JsonType::String: {
                std::string s = "\"";
                for (char c : str_val) {
                    if (c == '"') s += "\\\"";
                    else if (c == '\\') s += "\\\\";
                    else if (c == '\n') s += "\\n";
                    else if (c == '\r') s += "\\r";
                    else if (c == '\t') s += "\\t";
                    else s += c;
                }
                s += "\"";
                return s;
            }
            case JsonType::Array: {
                std::string s = "[";
                for (size_t i = 0; i < arr_val.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += arr_val[i].serialize();
                }
                s += "]";
                return s;
            }
            case JsonType::Object: {
                std::string s = "{";
                size_t count = 0;
                for (const auto& kv : obj_val) {
                    if (count > 0) s += ", ";
                    s += "\"" + kv.first + "\": " + kv.second.serialize();
                    count++;
                }
                s += "}";
                return s;
            }
        }
        return "null";
    }
};

class JsonParser {
private:
    std::string src;
    size_t pos = 0;

    void skip_whitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r')) {
            pos++;
        }
    }

    char peek() {
        skip_whitespace();
        if (pos >= src.size()) return '\0';
        return src[pos];
    }

    char get() {
        skip_whitespace();
        if (pos >= src.size()) return '\0';
        return src[pos++];
    }

    std::string parse_string() {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        std::string s = "";
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return s;
            if (c == '\\') {
                if (pos >= src.size()) break;
                char esc = src[pos++];
                if (esc == '"') s += '"';
                else if (esc == '\\') s += '\\';
                else if (esc == 'n') s += '\n';
                else if (esc == 'r') s += '\r';
                else if (esc == 't') s += '\t';
                else s += esc;
            } else {
                s += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    JsonValue parse_number() {
        size_t start = pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) pos++;
        while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E')) {
            pos++;
        }
        std::string num_str = src.substr(start, pos - start);
        double val = std::stod(num_str);
        return JsonValue(val);
    }

    JsonValue parse_array() {
        if (get() != '[') throw std::runtime_error("Expected '['");
        std::vector<JsonValue> arr;
        skip_whitespace();
        if (peek() == ']') {
            get();
            return JsonValue(arr);
        }
        while (true) {
            arr.push_back(parse_value());
            char c = peek();
            if (c == ']') {
                get();
                break;
            } else if (c == ',') {
                get();
            } else {
                throw std::runtime_error("Expected ',' or ']' in array");
            }
        }
        return JsonValue(arr);
    }

    JsonValue parse_object() {
        if (get() != '{') throw std::runtime_error("Expected '{'");
        std::map<std::string, JsonValue> obj;
        skip_whitespace();
        if (peek() == '}') {
            get();
            return JsonValue(obj);
        }
        while (true) {
            skip_whitespace();
            if (peek() != '"') throw std::runtime_error("Expected string key in object");
            std::string key = parse_string();
            skip_whitespace();
            if (get() != ':') throw std::runtime_error("Expected ':' after key");
            JsonValue val = parse_value();
            obj[key] = val;

            char c = peek();
            if (c == '}') {
                get();
                break;
            } else if (c == ',') {
                get();
            } else {
                throw std::runtime_error("Expected ',' or '}' in object");
            }
        }
        return JsonValue(obj);
    }

public:
    JsonParser(const std::string& input) : src(input), pos(0) {}

    JsonValue parse_value() {
        skip_whitespace();
        char c = peek();
        if (c == '\0') return JsonValue();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return JsonValue(parse_string());
        if (c == '-' || std::isdigit(c)) return parse_number();
        if (c == 't' || c == 'f') {
            if (src.compare(pos, 4, "true") == 0) { pos += 4; return JsonValue(true); }
            if (src.compare(pos, 5, "false") == 0) { pos += 5; return JsonValue(false); }
        }
        if (c == 'n') {
            if (src.compare(pos, 4, "null") == 0) { pos += 4; return JsonValue(); }
        }
        throw std::runtime_error("Unexpected token in JSON: " + std::string(1, c));
    }
};

inline JsonValue parse(const std::string& json_str) {
    JsonParser parser(json_str);
    return parser.parse_value();
}

// Helper builder functions
inline JsonValue make_array(const std::vector<int>& vec) {
    std::vector<JsonValue> arr;
    for (int v : vec) arr.push_back(JsonValue(v));
    return JsonValue(arr);
}

inline JsonValue make_matrix(const std::vector<std::vector<int>>& mat) {
    std::vector<JsonValue> arr;
    for (const auto& row : mat) arr.push_back(make_array(row));
    return JsonValue(arr);
}

} // namespace SimpleJson

#endif // JSON_HELPER_H
