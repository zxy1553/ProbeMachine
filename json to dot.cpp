#include <iostream>
#include <graphviz/gvc.h>
#include <fstream>
// #include <string>
#include <nlohmann/json.hpp>
using namespace std;

struct Kripke
{
    std::vector<std::string> states;
    std::vector<std::pair<std::string, std::string>> transitions;
    std::map<std::string, std::map<std::string, bool>> propLabels;
    std::vector<std::string> initialStates;
};

// 添加 nlohmann/json 的反序列化函数
void from_json(const nlohmann::json &j, Kripke &kripke)
{
    j.at("states").get_to(kripke.states);
    for (const auto &transition : j.at("transitions"))
    {
        kripke.transitions.push_back({transition["from"].get<std::string>(),
                                      transition["to"].get<std::string>()});
    }
    j.at("propLabels").get_to(kripke.propLabels);
    j.at("initialStates").get_to(kripke.initialStates);
}

// 绘制
void draw(Kripke& kripke)
{
    std::ofstream outputFile("graph.dot",ios::out);
    outputFile << "digraph G{" << endl;
    //转移关系
    for(auto tr: kripke.transitions){
        outputFile<<tr.first<<"->"<<tr.second<<";"<<endl;
    }
    outputFile<<endl;

    //标签函数
    for(auto s:kripke.propLabels){
        outputFile<<s.first<<"[label=\"";
        for(auto lbs:s.second){
            outputFile<<lbs.first<<" ";    
        }
        outputFile<<"\"]"<<endl;
    }
    outputFile<<endl;

    //初始状态
    for (auto s:kripke.initialStates)
    {
        outputFile<<s<<"[style=\"filled\"]"<<endl;
    }
    outputFile << "}" << endl;
    
}


int main()
{
    // 从json中读取数据
    std::ifstream inputFile("kripke.json");
    nlohmann::json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    // 使用反序列化函数
    Kripke kripke;
    from_json(jsonData, kripke);

    //输出dot文件
    draw(kripke);

}