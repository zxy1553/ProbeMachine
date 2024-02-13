#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

struct Kripke {
    std::vector<std::string> states;
    std::vector<std::pair<std::string, std::string>> transitions;
    std::map<std::string, std::map<std::string, bool>> propLabels;
    std::vector<std::string> initialStates;
};

// 添加 nlohmann/json 的反序列化函数
void from_json(const nlohmann::json& j, Kripke& kripke) {
    j.at("states").get_to(kripke.states);
    for (const auto& transition : j.at("transitions")) {
    kripke.transitions.push_back({
        transition["from"].get<std::string>(),
        transition["to"].get<std::string>()
    });
}
    j.at("propLabels").get_to(kripke.propLabels);
    j.at("initialStates").get_to(kripke.initialStates);
} 


// 从 JSON 对象解析 Kripke 结构
/* Kripke from_json(const nlohmann::json& jsonKripke) {
    Kripke kripke;
    kripke.states = jsonKripke["states"].get<std::vector<std::string>>();

    for (const auto& transition : jsonKripke["transitions"]) {
        kripke.transitions.push_back({
            transition["from"].get<std::string>(),
            transition["to"].get<std::string>()
        });
    }

    for (const auto& state : kripke.states) {
        kripke.propLabels[state] = jsonKripke["propLabels"][state].get<std::map<std::string, bool>>();
    }

    kripke.initialStates = jsonKripke["initialStates"].get<std::vector<std::string>>();

    return kripke;
}
 */

int main() {
    // 从文件中读取 JSON 数据
    std::ifstream inputFile("kripke.json");
    nlohmann::json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    // 使用反序列化函数还原数据
    Kripke kripke;
    from_json(jsonData, kripke);
/*     Kripke kripke = from_json(jsonData); */

    // 打印读取到的 Kripke 结构
    std::cout << "States:\n";
    for (const auto& state : kripke.states) {
        std::cout << state << '\n';
    }

    std::cout << "\nTransitions:\n";
    for (const auto& transition : kripke.transitions) {
        std::cout << "From: " << transition.first << ", To: " << transition.second << '\n';
    }

    std::cout << "\nPropLabels:\n";
    for (const auto& entry : kripke.propLabels) {
        const std::string& state = entry.first;
        const auto& labels = entry.second;
        std::cout << state << ":\n";
        for (const auto& label : labels) {
            std::cout << "  " << label.first << ": " << label.second << '\n';
        }
    }

    std::cout << "\nInitialStates:\n";
    for (const auto& initialState : kripke.initialStates) {
        std::cout << initialState << '\n';
    }

    return 0;
}
