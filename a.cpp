#include <iostream>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <tuple>
#include <random>
#include <algorithm>
#include <chrono>
#include <sstream>

using namespace std;

struct PairHash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const
    {
        // 将两个元素的哈希值合并成一个哈希值
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

struct Kripke
{
    std::vector<std::string> states;
    std::vector<std::pair<std::string, std::string>> transitions;
    std::map<std::string, std::map<std::string, bool>> propLabels;
    std::vector<std::string> initialStates;
};

// 定义Voter
struct Voter
{
    int No;                       // 从1开始编号
    std::vector<std::string> Act; // local actions
};

// 定义 model of MAS 结构
struct MoM
{
    std::vector<Voter> voters;                                                        // voter
    std::vector<std::string> L;                                                       // global_states
    std::vector<std::string> l;                                                       // initial global state
    std::unordered_map<std::pair<std::string, std::string>, std::string, PairHash> T; // global transition function
    std::unordered_map<std::string, std::string> V;                                   // valuation function
};

std::map<std::string, std::vector<string>> pre;
std::map<std::string, std::vector<string>> post;
std::vector<std::tuple<string, string, string>> dataLib;
std::vector<std::pair<string, string>> EFLib;
std::vector<std::pair<string, string>> EGLib;
std::vector<std::pair<string, string>> EULib;
// 计算平台
std::vector<tuple<string, string, string>> pf;
std::vector<tuple<string, string, string>> pft;

// 投票方式
string way[] = {"m", "i", "p"};

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

// 函数声明
void combineHelper(const std::vector<std::string> &elements, int start, int n, std::vector<std::string> &current, std::vector<std::vector<std::string>> &result);

// 主函数，用于生成组合
std::vector<std::vector<std::string>> combine(const std::vector<std::string> &elements, int n)
{
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> current;
    combineHelper(elements, 0, n, current, result);
    return result;
}

// 辅助递归函数
void combineHelper(const std::vector<std::string> &elements, int start, int n, std::vector<std::string> &current, std::vector<std::vector<std::string>> &result)
{
    // 如果当前组合已达到所需长度，将其添加到结果集
    if (n == 0)
    {
        result.push_back(current);
        return;
    }

    // 遍历剩余的元素
    for (int i = start; i <= elements.size() - n; ++i)
    {
        current.push_back(elements[i]);                         // 包含当前元素
        combineHelper(elements, i + 1, n - 1, current, result); // 递归调用
        current.pop_back();                                     // 不包含当前元素，回溯
    }
}

// 生成Voter模型
Voter genVoter(int canNum, int No)
{
    // 三种注册方式
    string reg[3];
    string pack[3];
    for (size_t i = 0; i < 3; i++)
    {
        std::vector<std::string> Act;
        reg[i] = ("reg" + way[i]);
        pack[i] = ("pack" + way[i]);
    }

    // 当前voter的所有投票结果集合
    std::vector<std::string> leaf;
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t k = 1; k <= canNum; k++)
        {
            leaf.push_back("v" + way[i] + to_string(k));
        }
    }

    // 三种Act集合
    std::vector<std::vector<std::string>> Act_1 = combine(leaf, 1);
    std::vector<std::vector<std::string>> Act_2 = combine(leaf, 2);
    std::vector<std::vector<std::string>> Act_3 = combine(leaf, 3);
    std::vector<std::vector<std::vector<std::string>>> Act{Act_1, Act_2, Act_3};

    // 添加reg和pack
    for (size_t m = 0; m < 3; m++)
    {
        int size = Act[m].size();
        // 遍历act_i的动作向量
        for (size_t i = 0; i < size; i++)
        {
            std::vector<std::string> path;
            std::unordered_map<std::string, bool> wayIn;
            wayIn[way[0]] = 0;
            wayIn[way[1]] = 0;
            wayIn[way[2]] = 0;
            // 遍历动作向量中的所有动作
            for (auto a : Act[m][i])
            {
                if (a[1] == 'm' && wayIn[way[0]] == 0)
                {
                    wayIn[way[0]] = 1;
                    path.push_back(reg[0]);
                    path.push_back(pack[0]);
                }
                if (a[1] == 'i' && wayIn[way[1]] == 0)
                {
                    wayIn[way[1]] = 1;
                    path.push_back(reg[1]);
                    path.push_back(pack[1]);
                }
                if (a[1] == 'p' && wayIn[way[2]] == 0)
                {
                    wayIn[way[2]] = 1;
                    path.push_back(reg[2]);
                    path.push_back(pack[2]);
                }
            }
            // 将动作添加到动作向量中
            Act[m][i].insert(Act[m][i].begin(), path.begin(), path.end());
            path.clear();
        }
    }
    // 生成随机索引
    std::random_device rd;
    std::mt19937 gen(rd());
    // 从三个动作向量中随机选择一个
    std::uniform_int_distribution<> dis(0, Act.size() - 1);
    int index = dis(gen);
    std::vector<std::vector<std::string>> selectedVecVec = Act[index];

    // 随机选择一个向量
    std::uniform_int_distribution<> dis2(0, selectedVecVec.size() - 1);
    int vecIndex = dis2(gen);
    std::vector<std::string> selectedVec = selectedVecVec[vecIndex];
    return Voter{No, selectedVec};
}

// 生成MoM模型
MoM genMoM(int voterNmu, int canNum)
{
    // 添加voters
    std::vector<Voter> voters;
    for (size_t i = 1; i < voterNmu + 1; i++)
    {
        voters.push_back(genVoter(canNum, i));
    }

    // 添加动作协议和转移关系
    std::vector<std::string> leaves;
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t k = 1; k <= canNum; k++)
        {
            leaves.push_back("v" + way[i] + to_string(k));
        }
    }
    std::unordered_map<std::string, std::vector<std::string>> P;
    P["s0"] = {"regm", "regi", "regp"};
    P["s1"] = {"packm"};
    P["s2"] = {"packi"};
    P["s3"] = {"packp"};
    std::unordered_map<std::pair<std::string, std::string>, std::string, PairHash> T;
    T[make_pair("s0", "regm")] = "s1";
    T[make_pair("s0", "regi")] = "s2";
    T[make_pair("s0", "regp")] = "s3";
    T[make_pair("s1", "packm")] = "s4";
    T[make_pair("s0", "packi")] = "s5";
    T[make_pair("s0", "packp")] = "s6";
    for (auto l : leaves)
    {
        int candidate;
        istringstream ss(l.substr(2));
        ss >> candidate;
        if (l[1] == 'm')
        {
            P["s4"].push_back(l);
            T[make_pair("s4", l)] = "s" + to_string(6 + candidate);
        }
        if (l[1] == 'i')
        {
            P["s5"].push_back(l);
            T[make_pair("s5", l)] = "s" + to_string(6 + candidate);
        }
        if (l[1] == 'p')
        {
            T[make_pair("s6", l)] = "s" + to_string(6 + candidate);
            P["s6"].push_back(l);
        }
    }

    // global states
    vector<string> L;
    for (int i = 0; i < 3 * canNum + 7; i++)
    {
        L.push_back("s" + to_string(i));
    }

    // initial global states
    std::vector<std::string> l = {"s0"};

    // propositions
    std::vector<std::string> PV;
    for (size_t i = 1; i < canNum + 1; i++)
    {
        PV.push_back("v" + to_string(i));
    }

    // valuation function
    std::unordered_map<std::string, std::string> V;
    int j = 0;
    for (size_t i = 7; i < L.size(); i++)
    {
        V[L[i]] = PV[j];
        if (++j == canNum)
        {
            j = 0;
        }
    }
    return MoM{voters, L, l, T, V};
}

// 生成kripke结构
Kripke genKripke(MoM &mom, int canNum)
{

    std::map<std::string, std::map<std::string, bool>> propLabels;
    std::vector<std::pair<std::string, std::string>> transitions;
    // 添加reg和pack的转移关系
    for (size_t i = 0; i < 3; i++)
    {
        transitions.push_back((std::pair<std::string, std::string>)make_pair("s0", "s" + to_string(i + 1)));
        transitions.push_back((std::pair<std::string, std::string>)make_pair("s" + to_string(i + 1), "s" + to_string(i + 4)));
    }
    // 添加投票转移关系、命题真值
    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < canNum; j++)
        {
            transitions.push_back(make_pair("s" + to_string(4 + i), "s" + to_string(7 + i * canNum + j)));
            transitions.push_back(make_pair("s" + to_string(7 + i * canNum + j), "s" + to_string(7 + i * canNum + j)));
            propLabels["s" + to_string(7 + i * canNum + j)]["v" + to_string(j + 1)] = true;
        }
    }

    return Kripke{mom.L, transitions, propLabels, mom.l};
}

// 重构kripke结构
Kripke reorderKripke(Kripke &k)
{
    unordered_map<std::string, std::string> m;
    std::vector<std::string> ss;
    std::vector<std::pair<std::string, std::string>> tr;
    std::map<std::string, std::map<std::string, bool>> pL;
    std::vector<std::string> iS;
    for (size_t i = 0; i < k.states.size(); i++)
    {
        m[k.states[i]] = "s" + to_string(i);
        ss.push_back(m[k.states[i]]);
    }

    for (size_t i = 0; i < k.initialStates.size(); i++)
    {
        iS.push_back(m[k.initialStates[i]]);
    }

    for (size_t i = 0; i < k.transitions.size(); i++)
    {
        tr.push_back(make_pair(m[k.transitions[i].first], m[k.transitions[i].second]));
    }

    for (auto &out : k.propLabels)
    {
        if (m.find(out.first) != m.end())
        {
            std::string s = m[out.first];
            for (auto &in : out.second)
            {
                std::string p = in.first;
                pL[s][p] = true;
            }
        }
    }
    return Kripke{ss, tr, pL, iS};
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

void genEFLib(Kripke &kripke, string f)
{
    for (const auto &x1 : dataLib)
    {
        for (const auto &x2 : dataLib)
        {
            // x1是x2的前序，
            if (get<0>(x1) == get<1>(x2) && get<2>(x1) == get<0>(x2))
            {
                if (kripke.propLabels[get<1>(x1)][f] == false)
                {
                    // 到了
                    if (kripke.propLabels[get<0>(x2)][f] == true)
                    {
                        EFLib.push_back(make_pair(get<0>(x1), get<0>(x2)));
                        break;
                    }
                    // 没到
                    EFLib.push_back(make_pair(get<0>(x1), get<0>(x2)));
                }
            }
        }
    }
}

void computeEF(Kripke &kripke, int n)
{
    // 数据元放入计算平台
    for (const auto &d : dataLib)
    {
        pf.push_back(d);
    }

    // 生成聚合体
    int i = 0;
    while (i < n) // 没有新增聚合体，说明探针过程完成
    {
        i++;
        for (int i = 0; i < pf.size(); i++)
        {
            for (int j = i + 1; j < pf.size(); j++)
            {
                // 如果是数据元是前后关系且存在对应的探针
                if (get<0>(pf[i]).substr(get<0>(pf[i]).length() - 2, 2) == get<1>(pf[j]) && get<2>(pf[i]) == get<0>(pf[j]).substr(0, 2) && isContain(EFLib, get<1>(pf[j]), get<2>(pf[i])))
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

bool EFModelChecking(MoM &mom, Kripke &kripke, int canNum, string f, int agNum)
{
    int voteNum = mom.voters.size();

    // 随机选择 agNum 个 Voters
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, voteNum - 1);

    // 存储选中的 Voters
    std::vector<Voter> agents;
    while (agents.size() < agNum)
    {
        int index = dis(gen);
        agents.push_back(mom.voters[index]);
    }
    // 随机选择 agNum 个不重复的 Voters
    while (agents.size() < agNum)
    {
        int index = dis(gen);
        agents.push_back(mom.voters[index]);
    }

    // 初始化标记小组
    vector<vector<bool>> vote(canNum, vector<bool>(3));
    bool reg[] = {0, 0, 0};
    bool pack[] = {0, 0, 0};
    unordered_map<char, int> wayMap;
    wayMap['m'] = 1;
    wayMap['i'] = 2;
    wayMap['p'] = 3;
    // 对存在的策略进行标记
    for (auto agent : agents)
    {
        for (auto act : agent.Act)
        {
            if (act[0] == 'v')
            {
                int candidate;
                std::istringstream ss(act.substr(2));
                ss >> candidate;
                vote[candidate - 1][wayMap[act[1]] - 1] = 1;
                reg[wayMap[act[1]] - 1] = 1;
                pack[wayMap[act[1]] - 1] = 1;
            }
        }
    }

    // prune
    for (size_t i = 0; i < canNum; i++)
    {
        for (size_t k = 0; k < 3; k++)
        {
            if (vote[i][k] == 0)
            {
                // 删去边
                std::string from, to;
                k == 0 ? from = "s4" : k == 1 ? from = "s5"
                                              : from = "s6";
                to = "s" + to_string(7 + k * canNum + i);
                std::remove(kripke.transitions.begin(), kripke.transitions.end(), (std::pair<std::string, std::string>)make_pair(from, to));
                // 删去点
                std::remove(kripke.states.begin(), kripke.states.end(), to);
            }
        }
    }
    for (size_t i = 0; i < 3; i++)
    {

        if (reg[i] == 0)
        {
            // 删去边
            std::remove(kripke.transitions.begin(), kripke.transitions.end(), (std::pair<string, string>)make_pair("s0", "s" + to_string(i + 1)));
            std::remove(kripke.transitions.begin(), kripke.transitions.end(), (std::pair<string, string>)make_pair("s" + to_string(i + 1), "s" + to_string(i + 4)));
            // 删去点
            std::remove(kripke.states.begin(), kripke.states.end(), "s" + to_string(i + 1));
            std::remove(kripke.states.begin(), kripke.states.end(), "s" + to_string(i + 4));
        }
    }
    kripke.transitions.erase(std::find(kripke.transitions.begin(), kripke.transitions.end(), (std::pair<string, string>)make_pair("", "")), kripke.transitions.end());
    kripke.states.erase(std::find(kripke.states.begin(), kripke.states.end(), ""), kripke.states.end());

    kripke = reorderKripke(kripke);
    // probe
    genP(kripke);
    genDataLib(kripke);
    genEFLib(kripke, f);
    computeEF(kripke, 3);
    // 根据状态序号判断是由那种途径投票的
    std::string s = get<2>(pf[0]);
    int sn;
    istringstream ss(s.substr(1));
    ss >> sn;
    if (sn > 6 + 2 * canNum)
    {
        return false;
    }
    // 判断是否投给了1
    return kripke.propLabels[get<2>(pf[0])][f] == true;
}

int main()
{

    int canNum, voteNum, agNum;
    canNum = 4;
    voteNum = 15;
    agNum = 1;
    for (size_t i = 1; i < 1 + agNum; i++)
    {
        for (size_t j = 1; j < voteNum + 1; j++)
        {
            for (size_t k = 1; k < canNum + 1; k++)
            {
                // 初始化
                pre.clear();
                post.clear();
                pf.clear();
                EFLib.clear();
                EGLib.clear();
                EULib.clear();
                dataLib.clear();

                // case
                auto start = std::chrono::high_resolution_clock::now();
                MoM mom = genMoM(j, k);
                Kripke kripke = genKripke(mom, k);
                bool f = EFModelChecking(mom, kripke, k, "v1", i);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                double seconds = duration.count() / 1000000.0;

                // 输出程序运行时间，显示到小数点后三位
                std::cout << "************************************************************************" << endl;
                std::cout << "voters:    " << j << endl
                          << "candidates:" << k << endl
                          << "agents:    " << i << endl;
                std::cout << "************************************" << endl;
                std::cout << (f ? "True" : "False") << endl;
                cout << std::get<0>(pf[0]) << "--" << get<1>(pf[0]) << "--" << get<2>(pf[0]) << endl;
                std::cout << "Time taken: " << std::fixed << std::setprecision(3) << seconds << " seconds" << std::endl;
            }
        }
    }
}