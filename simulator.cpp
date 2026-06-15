#include <bits/stdc++.h>

using namespace std;

struct Task {
    string id;
    int burst_time;
    vector<string> mem_blocks;
    int remaining_time;

    Task(const string& id, int burst, const vector<string>& mem)
        : id(id), burst_time(burst), mem_blocks(mem), remaining_time(burst) {}
};

class CacheLevel {
private:
    string level_name;
    int capacity;
    int latency;
    queue<string> blocks;  
    unordered_map<string, bool> present;  
    string last_evicted;

public:
    CacheLevel(const string& name, int cap, int lat)
        : level_name(name), capacity(cap), latency(lat), last_evicted("") {}

    bool contains(const string& block) const {
        return present.find(block) != present.end() && present.at(block);
    }

    string insert(const string& block) {
        last_evicted = "";

        if (contains(block)) return "";  

        if ((int)blocks.size() >= capacity) {
            last_evicted = blocks.front();
            blocks.pop();
            present[last_evicted] = false;
        }

        blocks.push(block);
        present[block] = true;
        return last_evicted;
    }

    vector<string> get_blocks() const {
        vector<string> res;
        queue<string> temp = blocks;
        while (!temp.empty()) {
            res.push_back(temp.front());
            temp.pop();
        }
        return res;
    }

    string get_last_evicted() const { return last_evicted; }
    int get_latency() const { return latency; }
    string get_name() const { return level_name; }
    int get_size() const { return blocks.size(); }
};

class MemoryHierarchy {
private:
    CacheLevel L1, L2, L3;
    int ram_accesses = 0;

    void spill_to_next(CacheLevel& level, CacheLevel* next, const string& block, string& log) {
        string evicted = level.insert(block);
        if (evicted.empty()) return; 

        string msg = evicted + " evicted from " + level.get_name();

        if (next != nullptr) {
            msg += " -> spilled to " + next->get_name();
            string l3_evicted = next->insert(evicted);
            
            if (!l3_evicted.empty()) {
                msg += "; " + l3_evicted + " evicted from " + next->get_name()
                       + " -> written back to RAM";
            }
        } else {
            msg += " -> written back to RAM";
        }

        if (!log.empty()) log += "; ";
        log += msg;
    }

public:
    MemoryHierarchy() : L1("L1", 32, 4), L2("L2", 128, 12), L3("L3", 512, 40) {}

    struct AccessResult {
        bool hit;       
        int latency;
        string hit_at;
        string eviction_msg;
    };

    AccessResult access(const string& block) {
        string log;

        if (L1.contains(block)) return {true, L1.get_latency(), "L1", ""};
        cout << ">>MISS at L1  ";

        if (L2.contains(block)) {
            spill_to_next(L1, &L2, block, log);
            return {true, L2.get_latency(), "L2", log};
        }
        cout << " >>MISS at L2  ";

        if (L3.contains(block)) {
            spill_to_next(L1, &L2, block, log);
            return {true, L3.get_latency(), "L3", log};
        }
        cout << " >>MISS at L3  ";

        spill_to_next(L1, &L2, block, log);
        ram_accesses++;
        return {false, 200, "RAM", log};
    }

    void print_state() const {
        auto print_lvl = [](const string& name, const CacheLevel& lvl, int cap) {
            cout << "    " << name << " [" << lvl.get_size() << "/" << cap << "]: [";
            auto blks = lvl.get_blocks();
            for (size_t i = 0; i < blks.size(); i++) {
                cout << blks[i];
                if (i < blks.size() - 1) cout << ", ";
            }
            cout << "]\n";
        };
        
        print_lvl("L1", L1, 32);
        print_lvl("L2", L2, 128);
        print_lvl("L3", L3, 512);
    }

    int get_ram_accesses() const { return ram_accesses; }
};

class Scheduler {
private:
    queue<Task> ready_queue;
    Task* curr = nullptr;
    int quantum;
    int current_slice_time = 0;
    vector<string> completed;

public:
    explicit Scheduler(int q = 3) : quantum(q) {}

    ~Scheduler() { delete curr; }

    void add_task(const Task& task) { ready_queue.push(task); }

    void schedule_next() {
        if (curr != nullptr && curr->remaining_time > 0) {
            ready_queue.push(*curr);  
        }

        delete curr;
        curr = nullptr;
        current_slice_time = 0;

        if (!ready_queue.empty()) {
            curr = new Task(ready_queue.front());
            ready_queue.pop();
        }
    }

    bool has_running_task() const { return curr != nullptr; }
    Task* get_current_task()      { return curr; }

    void execute_cycle() {
        if (curr != nullptr) {
            curr->remaining_time--;
            current_slice_time++;
            if (curr->remaining_time == 0) {
                completed.push_back(curr->id);
            }
        }
    }

    bool should_switch() const {
        if (curr == nullptr) return false;
        if (curr->remaining_time == 0) return true;
        if (current_slice_time >= quantum) return true;
        return false;
    }

    bool all_done() const {
        return ready_queue.empty() && (curr == nullptr || curr->remaining_time <= 0);
    }

    int tasks_completed() const { return completed.size(); }
    string get_algo_name() const { return "Round Robin (quantum = " + to_string(quantum) + ")"; }
};

class CPUSchedulerSimulator {
private:
    Scheduler scheduler;
    MemoryHierarchy memory;
    int cycle = 0;
    int total_cycles = 0;
    unordered_map<string, int> task_mem_index;

public:
    explicit CPUSchedulerSimulator(int rr_quantum = 3) : scheduler(rr_quantum) {}

    void add_task(const Task& task) { scheduler.add_task(task); }

    void run() {
        scheduler.schedule_next();  

        while (!scheduler.all_done()) {
            cycle++;
            total_cycles++;

            cout << string(60, '-') << "\n";
            cout << "Cycle " << cycle << " - ";

            if (scheduler.has_running_task()) {
                Task* curr = scheduler.get_current_task();
                cout << "Running: " << curr->id << " (remaining: " << curr->remaining_time << " cycles)";

                int idx = task_mem_index[curr->id];
                
                if (idx < (int)curr->mem_blocks.size()) {
                    const string& req_block = curr->mem_blocks[idx];
                    cout << " | Request: " << req_block << "\n";

                    auto result = memory.access(req_block);

                    if (result.hit_at == "RAM") {
                        cout << "    Memory Access: Fetched from RAM (latency: " << result.latency << " cycles)\n";
                    } else {
                        string hit_type = result.hit ? ">> HIT" : ">> MISS";
                        cout << "    Memory Access: " << hit_type << " at " << result.hit_at
                             << " (latency: " << result.latency << " cycles)\n";
                    }
                    if (!result.eviction_msg.empty()) {
                        cout << "    Eviction: " << result.eviction_msg << "\n";
                    }

                    task_mem_index[curr->id]++;
                } else {
                    cout << "    IDLE \n";
                }

                memory.print_state();
            } else {
                cout << "IDLE\n";
                memory.print_state();
            }

            scheduler.execute_cycle();
            if (scheduler.should_switch()) {
                scheduler.schedule_next();
            }
        }
    }

    void print_stats() {
        cout << "\n====================================\n";
        cout << "FINAL RESULTS\n";
        cout << "====================================\n";
        cout << "Total Cycles:        " << total_cycles << "\n";
        cout << "Tasks Completed:     " << scheduler.tasks_completed() << "\n";
        cout << "Scheduler:           " << scheduler.get_algo_name() << "\n";
        cout << "Total RAM Accesses:  " << memory.get_ram_accesses() << "\n";
        cout << "====================================\n";
    }
};

vector<Task> parse_file(const string& filename) {
    vector<Task> tasks;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << "\n";
        exit(1);
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream iss(line);
        string token, task_id;
        int burst = 0;
        vector<string> mem_blocks;

        iss >> token >> task_id >> token >> burst >> token;  
        while (iss >> token) {
            mem_blocks.push_back(token);
        }
        tasks.push_back(Task(task_id, burst, mem_blocks));
    }
    return tasks;
}

int main(int argc, char* argv[]) {
    string file_name = "tasks.txt";
    int q = 3;

    if (argc > 1) file_name = argv[1];
    if (argc > 2) q = stoi(argv[2]);

    cout << "=== Simulator Started ===\n";
    cout << "Loading: " << file_name << "\n\n";

    auto tasks = parse_file(file_name);

    if (tasks.empty()) {
        cout << "No tasks found.\n";
        return 1;
    }

    CPUSchedulerSimulator sim(q);
    for (const auto& t : tasks) {
        sim.add_task(t);
    }

    sim.run();
    sim.print_stats();

    return 0;
}