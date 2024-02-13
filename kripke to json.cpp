#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 定义 Kripke 结构
struct Kripke {
    // M={S,I,R,L}
    std::vector<std::string> states;    //S
    std::vector<std::pair<std::string, std::string>> transitions;   //R
    std::map<std::string, std::map<std::string, bool>> propLabels;  // L
    std::vector<std::string> initialStates;  // I
};

// 从键盘录入 Kripke 结构
Kripke inputKripke() {
    Kripke kripke;

    // 输入状态
    std::cout << "Enter states (separated by spaces): ";
    std::string statesInput;
    std::getline(std::cin, statesInput);
    // 将输入字符串拆分为状态
    size_t pos = 0;
    while ((pos = statesInput.find(' ')) != std::string::npos) {
        kripke.states.push_back(statesInput.substr(0, pos));
        statesInput.erase(0, pos + 1);
    }
    kripke.states.push_back(statesInput);

    // 输入转移关系
    std::string from, to;
    std::cout << "Enter transitions (from to, enter 'done' when finished):\n";
    while (true) {
        std::cout << "Transition: ";
        std::cin >> from;
        if (from == "done") {
            break;
        }
        std::cin >> to;
        kripke.transitions.push_back({from, to});
    }

    // 输入标签函数
    for (const auto& state : kripke.states) {
        std::map<std::string, bool> propLabel;
        std::cout << "Enter proposition labels for state " << state << " (prop value, enter 'done' when finished):\n";
        std::string prop;
        while (true) {
            std::cout << "Proposition label: ";
            std::cin >> prop;
            if (prop == "done") {
                break;
            }
            propLabel[prop] = true;
        }
        kripke.propLabels[state] = propLabel;
    }

    // 输入初始状态
    getchar();
    std::cout << "Enter initial states (separated by spaces): ";
    std::string initialStatesInput;
    std::getline(std::cin, initialStatesInput);
    // 将输入字符串拆分为初始状态
    pos = 0;
    while ((pos = initialStatesInput.find(' ')) != std::string::npos) {
        kripke.initialStates.push_back(initialStatesInput.substr(0, pos));
        initialStatesInput.erase(0, pos + 1);
    }
    kripke.initialStates.push_back(initialStatesInput);

    return kripke;
}

// 将 Kripke 结构转换为 JSON 对象
json to_json(const Kripke& kripke) {
    json jsonKripke;
    jsonKripke["states"] = kripke.states;

    // 将转移关系添加为 JSON 数组
    for (const auto& transition : kripke.transitions) {
        jsonKripke["transitions"].push_back({
            {"from", transition.first},
            {"to", transition.second},
        });
    }

    // 将标签函数添加为 JSON 对象
    for (const auto& state : kripke.states) {
        jsonKripke["propLabels"][state] = kripke.propLabels.at(state);
    }

    // 添加初始状态集合
    jsonKripke["initialStates"] = kripke.initialStates;

    return jsonKripke;
}

int main() {
    // 创建一个包括标签函数和初始状态集合的 Kripke 结构
    Kripke myKripke;
    myKripke.states = {"s0", "s1", "s2"};
    myKripke.transitions = {{"s0", "s1"}, {"s1", "s2"}, {"s2", "s0"}};

    // 为每个状态添加真值标签
    myKripke.propLabels["s0"] = {{"p", 1}};
    myKripke.propLabels["s1"] = {{"q", 1}};
    myKripke.propLabels["s2"] = {{"p", 1}, {"q", 1}};

    // 添加初始状态集合
    myKripke.initialStates = {"s0"};


/*     // 从键盘录入 Kripke 结构
    Kripke myKripke = inputKripke();
 */
    // 将 Kripke 结构转换为 JSON 对象
    json jsonKripke = to_json(myKripke);

    // 将 JSON 对象写入文件
    std::ofstream outputFile("kripke.json");
    outputFile << std::setw(4) << jsonKripke << std::endl;

    std::cout << "Kripke structure saved to kripke.json." << std::endl;

    return 0;
}
