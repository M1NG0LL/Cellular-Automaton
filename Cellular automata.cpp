#include <bits/stdc++.h>
#include <fstream>
#include <unordered_set>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <windows.h>

using namespace std;

/// @brief Class to represent cellular automaton rules like "B3/S23"
class Rules
{
private:
    /// @brief Sets to hold birth conditions
    unordered_set<int> birth_conditions;
    /// @brief Sets to hold survival conditions
    unordered_set<int> survival_conditions;

public:
    Rules()
    {
        // Default: Conway
        setFromString("B3/S23");
    }

    /// @brief function to set rules from string
    /// @details it takes the rule string format and make it Upper case then divide it into birth and survival parts
    /// @param ruleStr
    void setFromString(const string &ruleStr)
    {
        birth_conditions.clear();
        survival_conditions.clear();

        string s = ruleStr;
        for (char &c : s)
            c = toupper(c);

        size_t slash = s.find('/');
        if (slash == string::npos)
            throw runtime_error("Bad rule format");

        string b = s.substr(0, slash);
        string surv = s.substr(slash + 1);

        auto parse = [&](const string &part, unordered_set<int> &set)
        {
            for (char c : part)
                if (isdigit(c))
                    set.insert(c - '0');
        };

        if (b.find('B') != string::npos)
        {
            parse(b, birth_conditions);
            parse(surv, survival_conditions);
        }
        else
        {
            parse(b, birth_conditions);
            parse(surv, survival_conditions);
        }
    }

    /// @brief function to check if a cell should be born
    /// @param neighbors
    /// @return
    bool shouldBeBorn(int neighbors) const
    {
        return birth_conditions.count(neighbors);
    }

    /// @brief function to check if a cell should survive
    /// @param neighbors
    /// @return
    bool shouldSurvive(int neighbors) const
    {
        return survival_conditions.count(neighbors);
    }

    string toString() const
    {
        string b = "B";
        string s = "S";
        for (int i : birth_conditions)
            b += to_string(i);
        for (int i : survival_conditions)
            s += to_string(i);
        return b + "/" + s;
    }
};

/// @brief Configuration class for cellular automaton
class Config
{
public:
    Config(int width_ = 60, int height_ = 30, bool torus_ = true, bool moore_ = true)
        : width(width_), height(height_), torus(torus_), moore(moore_)
    {
    }

    int width;
    int height;

    bool torus;
    bool moore;

    Rules rules;
};

/// @brief Class to represent the grid of cells
class Grid
{
private:
    int w, h;
    vector<int> cells;

public:
    Grid(int width = 60, int height = 30) : w(width), h(height), cells(width * height, 0) {}

    int getWidth() const { return w; }
    int getHeight() const { return h; }

    void clear()
    {
        std::fill(cells.begin(), cells.end(), 0);
    }
    void randomize(double p)
    {
        for (auto &c : cells)
            c = ((double)rand() / RAND_MAX) < p ? 1 : 0;
    }

#pragma region Setters and Getters
    /// @brief Get the value of a cell at (x, y)
    /// @param x
    /// @param y
    /// @param torus
    /// @return
    int get(int x, int y, bool torus) const
    {
        if (torus)
        {
            x = (x + w) % w;
            y = (y + h) % h;
            return cells[y * w + x];
        }

        if (x < 0 || x >= w || y < 0 || y >= h)
            return 0;
        return cells[y * w + x];
    }

    void resize(int newW, int newH)
    {
        vector<int> newCells(newW * newH, 0);
        for (int y = 0; y < min(h, newH); y++)
            for (int x = 0; x < min(w, newW); x++)
                newCells[y * newW + x] = cells[y * w + x];

        w = newW;
        h = newH;
        cells.swap(newCells);
    }

    void set(int x, int y, int v)
    {
        if (x >= 0 && x < w && y >= 0 && y < h)
            cells[y * w + x] = v;
    }
#pragma endregion

    void print() const
    {
        cout << "+" << string(w, '-') << "+\n";
        for (int y = 0; y < h; y++)
        {
            cout << "|";
            for (int x = 0; x < w; x++)
                cout << (cells[y * w + x] ? 'O' : ' ');
            cout << "|\n";
        }
        cout << "+" << string(w, '-') << "+\n";
    }

    void save(const string &file) const
    {
        ofstream f(file);
        f << w << " " << h << "\n";
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
                f << cells[y * w + x];
            f << "\n";
        }
    }

    void load(const string &file)
    {
        ifstream f(file);
        int newW, newH;
        f >> newW >> newH;
        resize(newW, newH);

        string line;
        getline(f, line); // flush

        for (int y = 0; y < newH; y++)
        {
            getline(f, line);
            for (int x = 0; x < newW; x++)
                set(x, y, line[x] == '1');
        }
    }
};

/// @brief Class to represent the cellular automaton engine
class CAEngine
{
private:
    Config &config;
    Grid grid;

public:
    CAEngine(Config &cfg) : config(cfg), grid(cfg.width, cfg.height) {}

    /// @brief Get the Grid object
    /// @return
    Grid &getGrid()
    {
        return grid;
    }

    /// @brief  Count the number of alive neighbors for a cell whatever it was using Moore or Von Neumann neighborhood
    /// @param x
    /// @param y
    /// @return
    int neighborCount(int x, int y) const
    {
        static const int mooreX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        static const int mooreY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

        static const int vonX[4] = {0, -1, 1, 0};
        static const int vonY[4] = {-1, 0, 0, 1};

        int cnt = 0;

        if (config.moore)
        {
            for (int i = 0; i < 8; i++)
                cnt += grid.get(x + mooreX[i], y + mooreY[i], config.torus);
        }
        else
        {
            for (int i = 0; i < 4; i++)
                cnt += grid.get(x + vonX[i], y + vonY[i], config.torus);
        }

        return cnt;
    }

    /// @brief Perform a single step of the cellular automaton
    void step()
    {
        Grid next = grid;
        int Height = grid.getHeight();
        int Width = grid.getWidth();

        for (int y = 0; y < Height; y++)
        {
            for (int x = 0; x < Width; x++)
            {
                int alive = grid.get(x, y, config.torus);
                int n = neighborCount(x, y);

                if (alive)
                    next.set(x, y, config.rules.shouldSurvive(n));
                else
                    next.set(x, y, config.rules.shouldBeBorn(n));
            }
        }

        grid = next;
    }
};

class CAConsole
{
private:
    Config &config;
    CAEngine engine;

    void CommandHandler(const string &input)
    {
        istringstream iss(input);
        string cmd;
        iss >> cmd;

        if (cmd == "print")
            engine.getGrid().print();
        else if (cmd == "step")
        {
            int steps = 1;
            iss >> steps;
            for (int i = 0; i < steps; i++)
            {
                engine.step();
            }
        }
        else if (cmd == "clear")
        {
            engine.getGrid().clear();
        }
        else if (cmd == "random")
        {
            double p = 0.5;
            iss >> p;
            engine.getGrid().randomize(p);
        }
        else if (cmd == "resize")
        {
            int w, h;
            iss >> w >> h;
            engine.getGrid().resize(w, h);
        }
        else if (cmd == "config")
        {
            cout << "Width: " << config.width << "\n";
            cout << "Height: " << config.height << "\n";
            cout << "Torus: " << (config.torus ? "Yes" : "No") << "\n";
            cout << "Moore: " << (config.moore ? "Yes" : "No") << "\n";
            cout << "Rule: " << config.rules.toString() << "\n";
        }
        else if (cmd == "setconfig")
        {
            string param;
            iss >> param;
            if (param == "width")
            {
                int w;
                iss >> w;
                config.width = w;
                engine.getGrid().resize(config.width, config.height);
            }
            else if (param == "height")
            {
                int h;
                iss >> h;
                config.height = h;
                engine.getGrid().resize(config.width, config.height);
            }
            else if (param == "torus")
            {
                string val;
                iss >> val;
                config.torus = (val == "true" || val == "1");
            }
            else if (param == "moore")
            {
                string val;
                iss >> val;
                config.moore = (val == "true" || val == "1");
            }
            else if (param == "setrule")
            {
                string ruleStr;
                iss >> ruleStr;
                try
                {
                    config.rules.setFromString(ruleStr);
                    cout << "Rule set to " << config.rules.toString() << "\n";
                }
                catch (const exception &e)
                {
                    cout << "Error: " << e.what() << "\n";
                }
            }
            else
            {
                cout << "Unknown config parameter: " << param << "\n";
            }
        }
        else if (cmd == "save")
        {
            string filename;
            iss >> filename;
            engine.getGrid().save(filename);
            cout << "Saved to " << filename << "\n";
        }
        else if (cmd == "load")
        {
            string filename;
            iss >> filename;
            try
            {
                engine.getGrid().load(filename);
                cout << "Loaded from " << filename << "\n";
                engine.getGrid().print();
            }
            catch (const exception &e)
            {
                cout << "Error: " << e.what() << "\n";
            }
        }
        else
        {
            cout << "Unknown command: " << cmd << "\n";
        }
    }

public:
    CAConsole(Config &cfg) : config(cfg), engine(cfg) {}

    void run()
    {
        cout << "Cellular Automata Explorer\n";
        cout << "Rule: " << config.rules.toString() << "\n";

        string input;
        while (true)
        {
            cout << ">";
            getline(cin, input);

            for (char c : input)
                c = tolower(c);

            if (input == "exit")
                break;
            CommandHandler(input);
        }
    }
};

int main()
{
    Config config;
    CAConsole console(config);
    console.run();
    return 0;
}