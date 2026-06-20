#include <bits/stdc++.h>
using namespace std;

struct Task {
    string id;
    int burst;
    int remaining;
    vector<string> mem;
    int memIdx;
};

struct Cache {
    int cap;
    int latency;
    deque<string> blocks;

    bool has(string m) {
        for (auto& b : blocks)
            if (b == m) return true;
        return false;
    }

    string insert(string m) {
        if (has(m)) return "";
        string evicted = "";
        if ((int)blocks.size() >= cap) {
            evicted = blocks.front();
            blocks.pop_front();
        }
        blocks.push_back(m);
        return evicted;
    }
};

string fmt(Cache& c) {
    string s = "[";
    for (int i = 0; i < (int)c.blocks.size(); i++) {
        if (i) s += ", ";
        s += c.blocks[i];
    }
    s += "]";
    return s;
}

int main(int argc, char* argv[]) {
    string filename;
    if (argc > 1) {
        filename = argv[1]; 
    } else {
        filename = "input.txt"; 
    }
    ifstream fin(filename);
    if (!fin) {
        cout << "Cannot open file: " << filename << endl;
        return 1;
    }

    
    vector<Task> tasks;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string tok;
        ss >> tok;
        if (tok == "TASK") {
            Task t;
            t.memIdx = 0;
            ss >> t.id;
            string kw;
            ss >> kw;          
            ss >> t.burst;
            t.remaining = t.burst;
            ss >> kw;          
            string m;
            while (ss >> m) t.mem.push_back(m);
            tasks.push_back(t);
        }
    }
    fin.close();

    // caches: 
    Cache L1{32, 4};
    Cache L2{128, 12};
    Cache L3{512, 40};

    int ramAccesses = 0;
    int totalCycles = 0;
    int quantum = 3;

    queue<int> ready; //round robin ready queue
    for (int i = 0; i < (int)tasks.size(); i++) ready.push(i);

    int cycle = 1;
    int completed = 0;

    while (!ready.empty()) {
        int idx = ready.front(); ready.pop();
        Task& t = tasks[idx];

        int q = quantum;
        if (t.remaining < q) q = t.remaining;

        for (int i = 0; i < q; i++) {
            string m = "";
            if (!t.mem.empty()) {
                m = t.mem[t.memIdx % t.mem.size()];
                t.memIdx++;
            }

            cout << "Cycle " << cycle << " - Running: " << t.id;
            if (!m.empty()) cout << " Requesting: " << m;
            cout << endl;

            if (m.empty()) {
                
                totalCycles += 1;
                t.remaining--;
                cycle++;
                continue;
            }

            int latency = 0;
            bool hitL1 = L1.has(m);
            bool hitL2 = !hitL1 && L2.has(m);
            bool hitL3 = !hitL1 && !hitL2 && L3.has(m);

            if (hitL1) {
                latency = L1.latency;
                cout << "  L1: " << fmt(L1) << " -> HIT (" << latency << " cycles)" << endl;
                cout << "  L2: " << fmt(L2) << endl;
                cout << "  L3: " << fmt(L3) << endl;
            } else if (hitL2) {
                latency = L2.latency;
                cout << "  L1: " << fmt(L1) << " >> MISS" << endl;
                cout << "  L2: " << fmt(L2) << " >> HIT (" << latency << " cycles)" << endl;
                cout << "  L3: " << fmt(L3) << endl;
                cout << "  Promoting " << m << " -> L1" << endl;
                string evicted = L1.insert(m);
                cout << "  L1: " << fmt(L1);
                if (!evicted.empty()) cout << " (" << evicted << " evicted)";
                cout << endl;
            } else if (hitL3) {
                latency = L3.latency;
                cout << "  L1: " << fmt(L1) << " >> MISS" << endl;
                cout << "  L2: " << fmt(L2) << " >> MISS" << endl;
                cout << "  L3: " << fmt(L3) << " >> HIT (" << latency << " cycles)" << endl;
                cout << "  Promoting " << m << " -> L1" << endl;
                string evicted = L1.insert(m);
                cout << "  L1: " << fmt(L1);
                if (!evicted.empty()) cout << " (" << evicted << " evicted)";
                cout << endl;
                cout << "  L2: " << fmt(L2) << endl;
                cout << "  L3: " << fmt(L3) << endl;
            } else {
                latency = 200;
                ramAccesses++;
                cout << "  L1: " << fmt(L1) << " >> MISS" << endl;
                cout << "  L2: " << fmt(L2) << " >> MISS" << endl;
                cout << "  L3: " << fmt(L3) << " >> MISS" << endl;
                cout << "  Fetching from RAM (200 cycles)" << endl;
                string evicted = L1.insert(m);
                cout << "  L1: " << fmt(L1);
                if (!evicted.empty()) cout << " (" << evicted << " evicted)";
                cout << endl;
                cout << "  L2: " << fmt(L2) << endl;
                cout << "  L3: " << fmt(L3) << endl;
            }

            totalCycles += latency;
            t.remaining--;
            cycle++;
        }

        if (t.remaining > 0) {
            ready.push(idx);   // back of the queue
        } else {
            completed++;
            cout << "  ** " << t.id << " completed **" << endl;
        }
    }
    cout << "\nFinal Result: " << endl;
    cout << "Total Cycles: " << totalCycles << endl;
    cout << "Tasks Completed: " << completed << endl;

    return 0;
}