#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cstdlib> // for strtod and strtol

// 定义 JSON 类型枚举
enum class JSONType { Null, Bool, Int, Double, String, List, Dict };

// 定义 JSON 对象
struct JSONObject {
    JSONType type; // JSON 类型标签

    // 使用 union 存储不同类型的值
    union {
        bool boolVal;
        int intVal;
        double doubleVal;
    };
    std::string strVal;                          // 字符串值
    std::vector<JSONObject> listVal;             // 列表值
    std::unordered_map<std::string, JSONObject> dictVal; // 字典值

    // 构造函数
    JSONObject() : type(JSONType::Null) {}
    JSONObject(bool b) : type(JSONType::Bool), boolVal(b) {}
    JSONObject(int i) : type(JSONType::Int), intVal(i) {}
    JSONObject(double d) : type(JSONType::Double), doubleVal(d) {}
    JSONObject(const std::string& s) : type(JSONType::String), strVal(s) {}
    JSONObject(std::vector<JSONObject> l) : type(JSONType::List), listVal(std::move(l)) {}
    JSONObject(std::unordered_map<std::string, JSONObject> d) : type(JSONType::Dict), dictVal(std::move(d)) {}

    // 打印 JSON 对象（简化版）
    void print() const {
        switch (type) {
            case JSONType::Null:
                std::cout << "null";
                break;
            case JSONType::Bool:
                std::cout << (boolVal ? "true" : "false");
                break;
            case JSONType::Int:
                std::cout << intVal;
                break;
            case JSONType::Double:
                std::cout << doubleVal;
                break;
            case JSONType::String:
                std::cout << "\"" << strVal << "\"";
                break;
            case JSONType::List:
                std::cout << "[";
                for (size_t i = 0; i < listVal.size(); i++) {
                    listVal[i].print();
                    if (i < listVal.size() - 1) std::cout << ", ";
                }
                std::cout << "]";
                break;
            case JSONType::Dict:
                std::cout << "{";
                for (auto it = dictVal.begin(); it != dictVal.end(); ++it) {
                    std::cout << "\"" << it->first << "\": ";
                    it->second.print();
                    if (std::next(it) != dictVal.end()) std::cout << ", ";
                }
                std::cout << "}";
                break;
        }
    }
};

// 数字解析函数
bool try_parse_int(const std::string& str, int& value) {
    char* end;
    value = strtol(str.c_str(), &end, 10);
    return *end == '\0';
}

bool try_parse_double(const std::string& str, double& value) {
    char* end;
    value = strtod(str.c_str(), &end);
    return *end == '\0';
}

// 字符转义
char unescaped_char(char c) {
    switch (c) {
        case 'n': return '\n';
        case 'r': return '\r';
        case '0': return '\0';
        case 't': return '\t';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'b': return '\b';
        case 'a': return '\a';
        default: return c;
    }
}

// JSON 解析主函数
std::pair<JSONObject, size_t> parse(const std::string& json, size_t start = 0) {
    // 去除空白字符
    while (start < json.size() && isspace(json[start])) {
        start++;
    }

    // 空 JSON 返回 null
    if (start >= json.size()) {
        return {JSONObject(), start};
    }

    char first = json[start];

    // 解析数字
    if (isdigit(first) || first == '-' || first == '+') {
        std::regex num_re("[+-]?[0-9]+(\\.[0-9]*)?([eE][+-]?[0-9]+)?");
        std::smatch match;
        if (std::regex_search(json.begin() + start, json.end(), match, num_re)) {
            std::string str = match.str();
            int intValue;
            double doubleValue;
            if (try_parse_int(str, intValue)) {
                return {JSONObject(intValue), start + str.size()};
            }
            if (try_parse_double(str, doubleValue)) {
                return {JSONObject(doubleValue), start + str.size()};
            }
        }
    }

    // 解析字符串
    if (first == '"') {
        std::string str;
        size_t i = start + 1;
        while (i < json.size()) {
            if (json[i] == '\\') {
                i++;
                if (i < json.size()) str += unescaped_char(json[i]);
            } else if (json[i] == '"') {
                return {JSONObject(str), i + 1};
            } else {
                str += json[i];
            }
            i++;
        }
    }

    // 解析数组
    if (first == '[') {
        std::vector<JSONObject> res;
        size_t i = start + 1;
        while (i < json.size()) {
            if (json[i] == ']') {
                return {JSONObject(res), i + 1};
            }
            auto [obj, next] = parse(json, i);
            res.push_back(std::move(obj));
            i = next;
            if (json[i] == ',') {
                i++;
            }
        }
    }

    // 解析对象
    if (first == '{') {
        std::unordered_map<std::string, JSONObject> res;
        size_t i = start + 1;
        while (i < json.size()) {
            if (json[i] == '}') {
                return {JSONObject(res), i + 1};
            }
            auto [keyobj, keyeaten] = parse(json, i);
            i = keyeaten;
            if (json[i] == ':') {
                i++;
            }
            auto [valobj, valeaten] = parse(json, i);
            i = valeaten;
            if (keyobj.type == JSONType::String) {
                res[keyobj.strVal] = std::move(valobj);
            }
            if (json[i] == ',') {
                i++;
            }
        }
    }

    // 其他返回 null
    return {JSONObject(), start};
}

// 主函数测试
int main() {
    std::string json = R"JSON({"key": 42, "array": [1, 2, 3], "message": "hello world"})JSON";

    auto [obj, pos] = parse(json);
    std::cout << "Parsed JSON: ";
    obj.print();
    std::cout << std::endl;

    return 0;
}
