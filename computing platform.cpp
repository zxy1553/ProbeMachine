#include <iostream>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>
#include <tuple>
#include <algorithm>
using namespace std;

struct Kripke
{
    std::vector<std::string> states;
    std::vector<std::pair<std::string, std::string>> transitions;
    std::map<std::string, std::map<std::string, bool>> propLabels;
    std::vector<std::string> initialStates;
};

std::map<std::string, std::vector<string>> pre;
std::map<std::string, std::vector<string>> post;
std::vector<std::tuple<string, string, string>> dataLib;
std::vector<std::pair<string, string>> EGLib;
std::vector<std::pair<string, string>> EULib;
// 计算平台
std::vector<tuple<string, string, string>> pf;
std::vector<tuple<string, string, string>> pft;

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

void genP(Kripke &kripke)
{
    // 通过trans求出每个s的前序,后序集合
    for (auto &tr : kripke.transitions)
    {
        pre[tr.second].push_back(tr.first);
        post[tr.first].push_back(tr.second);
    }
    for (auto s : kripke.initialStates)
    {
        pre[s].push_back("ω");
    }
}

void genDataLib(Kripke &kripke)
{
    for (auto &s : kripke.states)
    {
        for (auto &p0 : pre[s])
        {
            for (auto &p1 : post[s])
            {
                // 二长路径，自身+前序+后序
                dataLib.push_back(std::make_tuple(s, p0, p1));
            }
        }
    }
}

// 用前后序集合优化?
void genEGLib(Kripke &kripke, string f)
{
    for (auto &x1 : dataLib)
    {
        for (auto &x2 : dataLib)
        {
            // x1是x2的前序，
            if (get<0>(x1) == get<1>(x2) && get<2>(x1) == get<0>(x2))
            {
                if (kripke.propLabels[get<0>(x1)][f] == true && kripke.propLabels[get<0>(x2)][f] == true)
                {
                    EGLib.push_back(make_pair(get<0>(x1), get<0>(x2)));
                }
            }
        }
    }
}

void genEULib(Kripke &kripke, string a, string b)
{
    for (const auto &x1 : dataLib)
    {
        for (const auto &x2 : dataLib)
        {
            // x1是x2的前序，
            if (get<0>(x1) == get<1>(x2) && get<2>(x1) == get<0>(x2))
            {
                if (kripke.propLabels[get<0>(x1)][a] == true && kripke.propLabels[get<1>(x1)][b] == false)
                {
                    if (kripke.propLabels[get<0>(x2)][b] == true)
                    {
                        EULib.push_back(make_pair(get<0>(x2), get<0>(x2)));
                    }
                    if (kripke.propLabels[get<0>(x2)][a] == true)
                    {
                        EULib.push_back(make_pair(get<0>(x1), get<0>(x2)));
                    }
                }
            }
        }
    }
}

// 判断是否存在对应的探针
bool isContain(const vector<pair<string, string>> &vec, const string &s1, const string &s2)
{
    auto it = find_if(vec.begin(), vec.end(), [&](const pair<string, string> &p)
                      { return p.first == s1 && p.second == s2; });
    return it != vec.end();
}

bool cmp(const tuple<string, string, string> &a, const tuple<string, string, string> &b)
{
    int countA = count(get<0>(a).begin(), get<0>(a).end(), 's');
    int countB = count(get<0>(b).begin(), get<0>(b).end(), 's');
    return countA > countB;
}

void computeEG(Kripke &kripke,int n)
{
    // 数据元放入计算平台
    pf = dataLib;

    // 生成聚合体
    int i=0;
    while (i<n) // 没有新增聚合体，说明探针过程完成(停止探针条件依情况而定)
    {
        i++;
        for (int i=0;i<pf.size();i++)
        {
            for (int j=i+1;j<pf.size();j++)
            {
                // 如果是数据元是前后关系且存在对应的探针
                if (get<0>(pf[i]).substr(get<0>(pf[i]).length()-2,2) == get<1>(pf[j]) && get<2>(pf[i]) == get<0>(pf[j]).substr(0,2) && isContain(EGLib, get<1>(pf[j]), get<2>(pf[i])))
                {
                    pft.push_back(make_tuple(get<0>(pf[i]) + get<0>(pf[j]), get<1>(pf[i]), get<2>(pf[j])));
                }
            }
        }
        // 大的聚合体放前面
        pf.insert(pf.end(), pft.begin(), pft.end());
        pft.clear();
        sort(pf.begin(), pf.end(), cmp);
    }
}

void computeEU(Kripke &kripke,int n)
{
    // 数据元放入计算平台
    for (const auto &d : dataLib)
    {
        pf.push_back(d);
    }

    // 生成聚合体
    int i=0;
    while (i <n) // 没有新增聚合体，说明探针过程完成
    {
        i++;
        for (int i=0;i<pf.size();i++)
        {
            for (int j=i+1;j<pf.size();j++)
            {
                // 如果是数据元是前后关系且存在对应的探针
                if (get<0>(pf[i]).substr(get<0>(pf[i]).length()-2,2) == get<1>(pf[j]) && get<2>(pf[i]) == get<0>(pf[j]).substr(0,2) && isContain(EULib, get<1>(pf[j]), get<2>(pf[i])))
                {
                    pft.push_back(make_tuple(get<0>(pf[i]) + get<0>(pf[j]), get<1>(pf[i]), get<2>(pf[j])));
                }
            }
        }
        // 大的聚合体放前面
        pf.insert(pf.end(), pft.begin(), pft.end());
        pft.clear();
        sort(pf.begin(), pf.end(), cmp);
    }
}

int main()
{
    // 从文件中读取 JSON 数据
    std::ifstream inputFile("kripke.json");
    nlohmann::json jsonData;
    inputFile >> jsonData;
    inputFile.close();
    // 使用反序列化函数还原数据
    Kripke kripke;
    from_json(jsonData, kripke);
    genP(kripke);
    genDataLib(kripke);
    genEULib(kripke, "p", "q");
    genEGLib(kripke, "p");
    computeEG(kripke,3);
    for (auto p : pf)
    {
        cout << std::get<0>(p) << "--" << get<1>(p) << "--" << get<2>(p) << endl;
    }
}