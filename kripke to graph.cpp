#include <iostream>
#include <map>
#include <vector>
#include <string>

// 结构体表示状态
struct State {
    std::string name;
    std::map<std::string, bool> labelFunctions;
    friend operator < (State a,State b){
    return a.name<b.name;
    }
};

// 使用 unordered_map 表示邻接表
std::map<State, std::vector<State>> adjacencyList;

// 函数用于从键盘输入 Kripke 结构
void inputKripkeStructure() {
    int numStates;

    std::cout << "Enter the number of states: ";
    std::cin >> numStates;

    for (int i = 0; i < numStates; ++i) {
        State state;
        std::cout << "Enter the name of State " << i + 1 << ": ";
        std::cin >> state.name;

        int numLabels;
        std::cout << "Enter the number of label functions for State " << state.name << ": ";
        std::cin >> numLabels;

        for (int j = 0; j < numLabels; ++j) {
            std::string labelName;
            bool labelValue;

            std::cout << "Enter label function " << j + 1 << " name for State " << state.name << ": ";
            std::cin >> labelName;

            std::cout << "Enter label function " << j + 1 << " value (0 or 1) for State " << state.name << ": ";
            std::cin >> labelValue;

            state.labelFunctions[labelName] = labelValue;
        }

        // 添加状态到邻接表
        adjacencyList[state] = std::vector<State>();  // 初始化为空的向量

    }

    // 输入转移关系
    int numTransitions;
    std::cout << "Enter the number of transitions: ";
    std::cin >> numTransitions;

    for (int i = 0; i < numTransitions; ++i) {
        State from, to;
        std::cout << "Enter the name of source state for Transition " << i + 1 << ": ";
        std::cin >> from.name;

        std::cout << "Enter the name of target state for Transition " << i + 1 << ": ";
        std::cin >> to.name;

        // 添加转移关系到邻接表
        adjacencyList[from].push_back(to);
    }
}

int main() {
    inputKripkeStructure();

    // 遍历邻接表并输出 Kripke 结构
    for (const auto& entry : adjacencyList) {
        const State& currentState = entry.first;
        const std::vector<State>& nextStates = entry.second;

        std::cout << "Current State: " << currentState.name << std::endl;
        std::cout << "  Label Functions:\n";
        for (const auto& labelFunction : currentState.labelFunctions) {
            std::cout << "    " << labelFunction.first << ": " << labelFunction.second << std::endl;
        }

        std::cout << "  Next States:\n";
        for (const auto& nextState : nextStates) {
            std::cout << "    " << nextState.name << std::endl;
        }
    }

 


    return 0;
}