#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>
#include <map>
#include <set>
#include <cmath>
#include <limits> // Required for std::numeric_limits
#include <algorithm> // Required for std::reverse and std::max
#include <fstream> // Required for file I/O
#include <string>  // Required for filenames
#include <list>    // Useful for Prim's algorithm

// Windows-specific header and settings for UTF-8
#ifdef _WIN32
    // Prevent definition of min and max macros
#define NOMINMAX
#include <windows.h>
#endif

// --- Color Constants (ANSI Escape Codes) ---
const std::string RESET = "\033[0m";
const std::string BLACK = "\033[30m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string BOLD = "\033[1m";
const std::string BG_BLACK = "\033[40m";
const std::string BG_RED = "\033[41m";
const std::string BG_GREEN = "\033[42m";
const std::string BG_YELLOW = "\033[43m";
const std::string BG_BLUE = "\033[44m";
const std::string BG_MAGENTA = "\033[45m";
const std::string BG_CYAN = "\033[46m";
const std::string BG_WHITE = "\033[47m";


// --- Cell Types ---
enum CellType {
    WALL = 0, PATH = 1, START = 2, END = 3,
    VISITED = 4, EXPLORED = 5, SOLUTION = 6
};

// --- Position Struct ---
struct Position {
    int r, c;
    bool operator==(const Position& other) const { return r == other.r && c == other.c; }
    bool operator!=(const Position& other) const { return !(*this == other); }
    bool operator<(const Position& other) const { if (r != other.r) return r < other.r; return c < other.c; }
};
// Hash specialization for Position to be used in maps/sets
namespace std { template <> struct hash<Position> { size_t operator()(const Position& p) const { auto h1 = hash<int>{}(p.r); auto h2 = hash<int>{}(p.c); return h1 ^ (h2 << 1); } }; }


// --- A* Node Struct ---
struct AStarNode { Position pos;int gCost;int hCost;int fCost()const { return gCost + hCost; }bool operator>(const AStarNode& o)const { if (fCost() != o.fCost()) { return fCost() > o.fCost(); }return hCost > o.hCost; } };


class Maze {
private:
    int width = 0, height = 0;
    std::vector<std::vector<CellType>> grid;
    std::vector<std::vector<CellType>> originalGrid; // Stores the original generated/loaded state
    Position start = { -1,-1 }, end = { -1,-1 };
    std::mt19937 rng; // Random number generator
    int visualizationDelayMs = 10; // Visualization delay in milliseconds

    // --- Core Helpers ---
    void initializeGrid(bool fillWalls = true) {
        if (width <= 0 || height <= 0) return;
        grid.assign(height, std::vector<CellType>(width, fillWalls ? WALL : PATH));
    }

    bool isValid(int r, int c, bool allowWalls = false) const {
        if (r >= 0 && r < height && c >= 0 && c < width) {
            return allowWalls || grid[r][c] != WALL;
        }
        return false;
    }

    // --- Generation Algorithms ---
    void generateRecursiveBacktracker(int r, int c) {
        grid[r][c] = PATH;
        std::vector<int> directions = { 0, 1, 2, 3 }; // N, E, S, W
        std::shuffle(directions.begin(), directions.end(), rng);
        int dr[] = { -1, 0, 1, 0 }; int dc[] = { 0, 1, 0, -1 };
        for (int dir : directions) {
            int nr = r + dr[dir] * 2; int nc = c + dc[dir] * 2; // Neighbor cell
            int wr = r + dr[dir]; int wc = c + dc[dir];       // Wall between
            if (isValid(nr, nc, true) && grid[nr][nc] == WALL) {
                grid[wr][wc] = PATH; // Carve wall
                generateRecursiveBacktracker(nr, nc); // Recurse
            }
        }
    }

    void generatePrims() {
        if (width <= 0 || height <= 0) return;
        initializeGrid(true); // Start with all walls
        // Start cell (must be odd coordinates)
        int startR = (rng() % (height / 2)) * 2 + 1;
        int startC = (rng() % (width / 2)) * 2 + 1;
        grid[startR][startC] = PATH;

        std::list<Position> frontier; // Use list for efficient random access erase
        int dr[] = { -1, 0, 1, 0 }; int dc[] = { 0, 1, 0, -1 };

        // Add initial frontiers
        for (int i = 0; i < 4; ++i) {
            int nr = startR + dr[i]; int nc = startC + dc[i];
            if (isValid(nr, nc, true) && grid[nr][nc] == WALL) {
                frontier.push_back({ nr, nc });
                grid[nr][nc] = EXPLORED; // Temporarily mark wall as frontier
            }
        }

        while (!frontier.empty()) {
            // Randomly select a frontier wall
            auto it = frontier.begin();
            std::advance(it, rng() % frontier.size());
            Position wall = *it;
            frontier.erase(it); // Remove from frontier

            // Find the path cell neighbor and the potential new path cell
            Position pathNeighbor = { -1, -1 };
            Position nextCell = { -1, -1 };

            // Check both sides of the wall
            for (int i = 0; i < 4; ++i) {
                int r1 = wall.r + dr[i]; int c1 = wall.c + dc[i]; // One side
                int r2 = wall.r - dr[i]; int c2 = wall.c - dc[i]; // Opposite side

                // Check if r1,c1 is path and r2,c2 is wall
                if (isValid(r1, c1) && grid[r1][c1] == PATH && isValid(r2, c2, true) && grid[r2][c2] == WALL) {
                    pathNeighbor = { r1, c1 }; nextCell = { r2, c2 }; break;
                }
                // Check if r2,c2 is path and r1,c1 is wall
                if (isValid(r2, c2) && grid[r2][c2] == PATH && isValid(r1, c1, true) && grid[r1][c1] == WALL) {
                    pathNeighbor = { r2, c2 }; nextCell = { r1, c1 }; break;
                }
            }

            if (nextCell.r != -1) { // Found a valid wall to carve
                grid[wall.r][wall.c] = PATH;     // Carve wall
                grid[nextCell.r][nextCell.c] = PATH; // Carve next cell

                // Add new frontiers from the newly carved cell
                for (int i = 0; i < 4; ++i) {
                    int nr = nextCell.r + dr[i]; int nc = nextCell.c + dc[i];
                    if (isValid(nr, nc, true) && grid[nr][nc] == WALL) {
                        frontier.push_back({ nr, nc });
                        grid[nr][nc] = EXPLORED; // Mark as frontier
                    }
                }
            }
            else {
                // Optional: Revert wall status if it didn't lead anywhere new
                grid[wall.r][wall.c] = WALL;
            }
        }
        // Clean up any remaining EXPLORED markers
        for (int r = 0; r < height; ++r) for (int c = 0; c < width; ++c) if (grid[r][c] == EXPLORED) grid[r][c] = WALL;
    }


    void placeStartEnd() {
        if (width <= 0 || height <= 0) return;
        // Place Start near top-left path
        start = { 1, 1 };
        // If (1,1) is a wall, find the first available path cell
        if (grid[start.r][start.c] == WALL) { for (int r = 1; r < height; r += 2) for (int c = 1; c < width; c += 2) if (grid[r][c] == PATH) { start = { r,c }; goto found_start; } }
    found_start: grid[start.r][start.c] = START;

        // Place End near bottom-right path
        end = { height - 2, width - 2 };
        // If corner is wall, find last available path cell (different from start)
        if (grid[end.r][end.c] == WALL) {
            for (int r = height - 2; r > 0; r -= 2) for (int c = width - 2; c > 0; c -= 2) if (grid[r][c] == PATH && Position{ r,c } != start) { end = { r,c }; goto found_end; }
            // Fallback if no other path cell found
            if (isValid(height - 2, width - 3) && grid[height - 2][width - 3] == PATH) end = { height - 2,width - 3 };
            else if (isValid(height - 3, width - 2) && grid[height - 3][width - 2] == PATH) end = { height - 3,width - 2 };
            else end = start; // Worst case: end is same as start (no path possible)
        }
    found_end:
        // Try slightly offset if end defaulted to start
        if (end == start && height > 3 && width > 3) {
            if (isValid(start.r + 1, start.c) && grid[start.r + 1][start.c] == PATH) end = { start.r + 1,start.c };
            else if (isValid(start.r, start.c + 1) && grid[start.r][start.c + 1] == PATH) end = { start.r,start.c + 1 };
        }
        grid[end.r][end.c] = END;
    }

    // --- Pathfinding Helpers ---
    std::vector<Position> reconstructPath(const std::map<Position, Position>& parentMap, Position current) {
        std::vector<Position> path;
        while (current != start && parentMap.count(current)) {
            path.push_back(current);
            current = parentMap.at(current);
        }
        if (current == start) { path.push_back(start); }
        std::reverse(path.begin(), path.end());
        // Validate path integrity
        if (path.empty() || path.front() != start || path.back() != end) return {};
        return path;
    }

    int heuristic(Position a, Position b) {
        // Manhattan distance heuristic
        return std::abs(a.r - b.r) + std::abs(a.c - b.c);
    }

    // --- Display Helpers ---
    void colorCell(CellType type) {
        switch (type) {
        case WALL:     std::cout << BG_WHITE << BLACK << BOLD << "##" << RESET; break;
        case PATH:     std::cout << BG_BLACK << WHITE << "  " << RESET; break;
        case START:    std::cout << BG_GREEN << BLACK << BOLD << "ST" << RESET; break; // Start
        case END:      std::cout << BG_RED << WHITE << BOLD << "EN" << RESET;   break; // End
        case VISITED:  std::cout << BG_BLUE << WHITE << ".." << RESET; break; // Visited during search
        case EXPLORED: std::cout << BG_CYAN << BLACK << "xx" << RESET; break; // Explored but not path
        case SOLUTION: std::cout << BG_YELLOW << BLACK << BOLD << "::" << RESET; break; // Final solution path
        default:       std::cout << "  "; break;
        }
    }

public:
    Maze() { std::random_device rd; rng.seed(rd()); } // Constructor

    // Set dimensions, ensuring they are odd and >= 5
    bool setDimensions(int w, int h) {
        width = (w % 2 == 0 ? w + 1 : w);
        height = (h % 2 == 0 ? h + 1 : h);
        if (width < 5) width = 5;
        if (height < 5) height = 5;
        return true;
    }

    // Generate the maze using the chosen algorithm
    void generate(int algorithm = 0) { // 0 = Backtracker, 1 = Prim's
        if (width <= 0 || height <= 0) {
            std::cerr << RED << "Error: Dimensions not set before generating." << RESET << std::endl;
            return;
        }
        initializeGrid(true); // Start with a grid full of walls
        if (algorithm == 1) {
            generatePrims();
        }
        else { // Default to Recursive Backtracker
            // Start carving from a random odd position
            int startR = (rng() % (height / 2)) * 2 + 1;
            int startC = (rng() % (width / 2)) * 2 + 1;
            generateRecursiveBacktracker(startR, startC);
        }
        placeStartEnd(); // Place start/end points after carving
        originalGrid = grid; // Save the generated state
    }

    // Display the maze grid in the console
    void display(const std::vector<Position>& solutionPath = {}, bool clear = true) {
        if (width <= 0 || height <= 0) return; // Don't display if not initialized
        if (clear) { std::cout << "\033[2J\033[1;1H" << std::flush; } // Clear screen (ANSI)
        std::map<Position, bool> isSolutionPath;
        for (const auto& p : solutionPath) { isSolutionPath[p] = true; }
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                Position curr = { r,c };
                // If the cell is part of the solution path (and not Start/End), color it as SOLUTION
                if (isSolutionPath[curr] && grid[r][c] != START && grid[r][c] != END) {
                    colorCell(SOLUTION);
                }
                else {
                    colorCell(grid[r][c]); // Otherwise, use its normal cell type color
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    // Restore the grid to its original state (after generation/loading)
    void restoreGrid() {
        if (!originalGrid.empty()) grid = originalGrid;
    }

    // Set the delay between visualization steps
    void setVisualizationDelay(int delayMs) {
        // Use std::max for clarity and to ensure non-negative delay
        visualizationDelayMs = std::max(0, delayMs);
    }

    // --- SOLVING ALGORITHMS (BFS, DFS, A*) ---
    // The internal logic of these remains the same as the previous versions.

    std::vector<Position> solveBFS() {
        restoreGrid(); // Reset grid before solving
        std::queue<Position> q;
        std::map<Position, Position> parentMap;
        std::map<Position, bool> visited;
        q.push(start); visited[start] = true; parentMap[start] = start;
        int dr[] = { -1,0,1,0 }; int dc[] = { 0,1,0,-1 };
        std::vector<Position> path;
        while (!q.empty()) {
            Position current = q.front(); q.pop();
            // Mark as visited (for visualization) unless it's start/end
            if (grid[current.r][current.c] != START && grid[current.r][current.c] != END) grid[current.r][current.c] = VISITED;
            if (current == end) { path = reconstructPath(parentMap, end); break; } // Goal found
            // Visualization step
            if (visualizationDelayMs > 0) { display(std::vector<Position>(), true); std::this_thread::sleep_for(std::chrono::milliseconds(visualizationDelayMs)); }
            // Explore neighbors
            for (int i = 0;i < 4;++i) {
                int nr = current.r + dr[i]; int nc = current.c + dc[i]; Position next = { nr,nc };
                if (isValid(nr, nc) && !visited[next]) { visited[next] = true; parentMap[next] = current; q.push(next); }
            }
        }
        restoreGrid(); // Clean up VISITED markers
        return path;
    }

    std::vector<Position> solveDFS() {
        restoreGrid();
        std::stack<Position> s;
        std::map<Position, Position> parentMap;
        std::map<Position, bool> visited;
        s.push(start); visited[start] = true; parentMap[start] = start;
        int dr[] = { -1,0,1,0 }; int dc[] = { 0,1,0,-1 };
        std::vector<Position> path;
        while (!s.empty()) {
            Position current = s.top(); // Peek
            if (current == end) { path = reconstructPath(parentMap, end); break; } // Goal found
            // Mark as visited (for visualization)
            if (grid[current.r][current.c] != START && grid[current.r][current.c] != END) grid[current.r][current.c] = VISITED;
            // Visualization step
            if (visualizationDelayMs > 0) { display(std::vector<Position>(), true); std::this_thread::sleep_for(std::chrono::milliseconds(visualizationDelayMs)); }
            bool found_neighbor = false;
            // Explore neighbors
            for (int i = 0;i < 4;++i) {
                int nr = current.r + dr[i]; int nc = current.c + dc[i]; Position next = { nr,nc };
                if (isValid(nr, nc) && !visited[next]) { visited[next] = true; parentMap[next] = current; s.push(next); found_neighbor = true; break; } // Push and break to go deep
            }
            // If no unvisited neighbor found, backtrack
            if (!found_neighbor) { s.pop(); }
        }
        restoreGrid();
        return path;
    }

    std::vector<Position> solveAStar() {
        restoreGrid();
        // Min-priority queue for open set
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
        std::map<Position, Position> parentMap; // To reconstruct path
        std::map<Position, int> gCost; // Cost from start to node

        // Initialize gCost for all nodes to infinity
        // Using parentheses around max() to avoid macro issues
        for (int r = 0;r < height;++r) for (int c = 0;c < width;++c) gCost[{r, c}] = (std::numeric_limits<int>::max)();

        gCost[start] = 0; parentMap[start] = start;
        openSet.push({ start, 0, heuristic(start,end) }); // Add start node

        int dr[] = { -1,0,1,0 }; int dc[] = { 0,1,0,-1 }; // Directions
        std::vector<Position> path;

        while (!openSet.empty()) {
            AStarNode current_node = openSet.top(); // Get node with lowest fCost
            Position current_pos = current_node.pos;
            openSet.pop();

            // Optional optimization: If we found a shorter path already, skip
            // if (current_node.gCost > gCost[current_pos]) continue;

            // Mark as visited (for visualization)
            if (grid[current_pos.r][current_pos.c] != START && grid[current_pos.r][current_pos.c] != END) grid[current_pos.r][current_pos.c] = VISITED;

            if (current_pos == end) { path = reconstructPath(parentMap, end); break; } // Goal found

            // Visualization step
            if (visualizationDelayMs > 0) { display(std::vector<Position>(), true); std::this_thread::sleep_for(std::chrono::milliseconds(visualizationDelayMs)); }

            // Explore neighbors
            for (int i = 0;i < 4;++i) {
                int nr = current_pos.r + dr[i]; int nc = current_pos.c + dc[i]; Position neighbor_pos = { nr,nc };

                if (isValid(nr, nc)) { // If neighbor is valid (within bounds and not a wall)
                    int tentative_gCost = gCost[current_pos] + 1; // Cost to reach neighbor through current

                    // If this path to neighbor is better than any previous one found
                    if (tentative_gCost < gCost[neighbor_pos]) {
                        parentMap[neighbor_pos] = current_pos; // Update parent
                        gCost[neighbor_pos] = tentative_gCost; // Update cost
                        int hCost = heuristic(neighbor_pos, end); // Calculate heuristic
                        openSet.push({ neighbor_pos, tentative_gCost, hCost }); // Add/Update neighbor in open set
                    }
                }
            }
        }
        restoreGrid();
        return path;
    }

    // --- File I/O ---
    bool saveToFile(const std::string& filename) const {
        if (width <= 0 || height <= 0 || originalGrid.empty()) {
            std::cerr << RED << "Error: Cannot save an uninitialized or empty maze." << RESET << std::endl;
            return false;
        }
        std::ofstream outFile(filename);
        if (!outFile) {
            std::cerr << RED << "Error: Could not open file '" << filename << "' for writing." << RESET << std::endl;
            return false;
        }
        outFile << width << " " << height << "\n"; // Write dimensions
        // Write the integer value of the CellType from the *original* grid
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                outFile << static_cast<int>(originalGrid[r][c]) << (c == width - 1 ? "" : " ");
            }
            outFile << "\n";
        }
        outFile.close();
        return !outFile.fail(); // Check if closing succeeded
    }

    bool loadFromFile(const std::string& filename) {
        std::ifstream inFile(filename);
        if (!inFile) {
            std::cerr << RED << "Error: Could not open file '" << filename << "' for reading." << RESET << std::endl;
            return false;
        }
        int w, h;
        inFile >> w >> h; // Read dimensions
        if (inFile.fail() || w <= 0 || h <= 0) {
            std::cerr << RED << "Error: Invalid dimensions found in file '" << filename << "'." << RESET << std::endl;
            inFile.close();
            return false;
        }
        setDimensions(w, h); // Use setter to ensure odd sizing if needed
        initializeGrid(false); // Initialize grid (content will be overwritten)

        start = { -1, -1 }; end = { -1, -1 };
        int cellValue;
        // Read cell data
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                if (!(inFile >> cellValue)) {
                    std::cerr << RED << "Error: Failed to read cell data at (" << r << "," << c << ") from file '" << filename << "'." << RESET << std::endl;
                    inFile.close(); width = 0; height = 0; grid.clear(); // Invalidate maze state
                    return false;
                }
                CellType type = static_cast<CellType>(cellValue);
                grid[r][c] = type;
                // Identify start and end points during load
                if (type == START) start = { r, c };
                else if (type == END) end = { r, c };
            }
        }
        inFile.close();
        // Check if start and end points were found
        if (start.r == -1 || end.r == -1) {
            std::cerr << RED << "Error: Start or End point not found in loaded maze file '" << filename << "'." << RESET << std::endl;
            width = 0; height = 0; grid.clear(); // Invalidate maze state
            return false;
        }
        originalGrid = grid; // Store the loaded state as the 'original'
        return true;
    }

    // --- Getters ---
    Position getStart() const { return start; }
    Position getEnd() const { return end; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isGenerated() const { return !originalGrid.empty(); } // Check if maze exists
};

// --- Utility Functions ---
void displayAZD() {
    // Display "AZD" using ASCII art
    std::cout << "\n\n" << BOLD << MAGENTA
        << "      AAAAA      ZZZZZZZZZZZ     DDDDDDDD    \n"
        << "     AAAAAAA     ZZZZZZZZZZZ     DDDDDDDDDD  \n"
        << "    AAA   AAA          ZZZ       DDD     DDD \n"
        << "   AAA     AAA        ZZZ        DDD     DDD \n"
        << "  AAAAAAAAAAAAA      ZZZ         DDD     DDD \n"
        << " AAAAAAAAAAAAAAA    ZZZ          DDD     DDD \n"
        << "AAA         AAA    ZZZ           DDD    DDD  \n"
        << "AAA         AAA   ZZZZZZZZZZZ    DDDDDDDDDD  \n"
        << "AAA         AAA  ZZZZZZZZZZZ     DDDDDDDD    \n"
        << RESET << std::endl;
}
// Struct to hold solving results
struct SolveResult { std::string algoName;std::vector<Position> path;double timeMs = 0.0; };


// --- Main Application Logic ---
int main() {
    // Set console output to UTF-8 on Windows
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 65001
    // May also need for input: SetConsoleCP(CP_UTF8);
#endif

    Maze maze;
    int delay = 10; // Default visualization delay
    bool mazeLoadedOrGenerated = false; // Track if a maze is ready

    while (true) {
        // Display Menu
        std::cout << BLUE << "\n--- Maze Solver Menu ---\n" << RESET;
        std::cout << "1. Generate New Maze (Set Dimensions)\n";
        std::cout << "2. Load Maze from File\n";
        std::cout << "3. Solve Current Maze\n";
        std::cout << "4. Save Current Maze to File\n";
        std::cout << "5. Set Visualization Delay (Current: " << delay << "ms)\n";
        std::cout << "6. Display Current Maze\n";
        std::cout << "0. Exit\n";
        std::cout << BLUE << "Enter your choice: " << RESET;

        int choice;
        std::cin >> choice;

        // Input validation
        if (std::cin.fail()) {
            std::cout << RED << "Invalid input. Please enter a number." << RESET << std::endl;
            std::cin.clear();
            // Using parentheses around max() to avoid potential macro issues
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            continue;
        }
        // Clear the rest of the input buffer (including newline)
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        switch (choice) {
        case 1: { // Generate New Maze
            int w = 0, h = 0, algoChoice = 0;
            // Get width with validation
            while (w < 5) {
                std::cout << BLUE << "Enter maze width (minimum 5): " << RESET;
                std::cin >> w;
                if (std::cin.fail() || w < 5) { std::cout << RED << "Invalid input.\n" << RESET; std::cin.clear(); std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); w = 0; }
            }
            // Get height with validation
            while (h < 5) {
                std::cout << BLUE << "Enter maze height (minimum 5): " << RESET;
                std::cin >> h;
                if (std::cin.fail() || h < 5) { std::cout << RED << "Invalid input.\n" << RESET; std::cin.clear(); std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); h = 0; }
            }
            // Get generation algorithm choice
            while (algoChoice < 1 || algoChoice > 2) {
                std::cout << BLUE << "Generation Algorithm (1: Backtracker, 2: Prim's): " << RESET;
                std::cin >> algoChoice;
                if (std::cin.fail() || algoChoice < 1 || algoChoice > 2) { std::cout << RED << "Invalid choice.\n" << RESET; std::cin.clear(); std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); algoChoice = 0; }
            }
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // Clear newline

            maze.setDimensions(w, h);
            std::cout << YELLOW << "Generating maze using " << (algoChoice == 1 ? "Recursive Backtracker" : "Prim's Algorithm") << "..." << RESET << std::endl;
            maze.generate(algoChoice - 1); // Pass 0 for Backtracker, 1 for Prim's
            mazeLoadedOrGenerated = true;
            std::cout << GREEN << "Maze generated. Displaying:" << RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            maze.display();
            break;
        }
        case 2: { // Load Maze
            std::string filename;
            std::cout << BLUE << "Enter filename to load: " << RESET;
            std::getline(std::cin, filename); // Use getline for names with spaces
            if (maze.loadFromFile(filename)) {
                std::cout << GREEN << "Maze loaded successfully from '" << filename << "'." << RESET << std::endl;
                mazeLoadedOrGenerated = true;
                maze.display();
            }
            else {
                // Error message is printed within loadFromFile
                mazeLoadedOrGenerated = false;
            }
            break;
        }
        case 3: { // Solve Maze
            if (!mazeLoadedOrGenerated || !maze.isGenerated()) {
                std::cout << RED << "No maze loaded or generated yet. Please generate (1) or load (2) first." << RESET << std::endl;
                break;
            }
            // Get solving algorithm choice
            int solveChoice = 0;
            while (solveChoice < 1 || solveChoice > 4) {
                std::cout << BLUE << "\nChoose solving algorithm:\n";
                std::cout << "1. Breadth-First Search (BFS - Shortest Path)\n";
                std::cout << "2. Depth-First Search (DFS)\n";
                std::cout << "3. A* Search (Heuristic - Shortest Path)\n";
                std::cout << "4. Run and Compare All\n";
                std::cout << "Enter your choice (1-4): " << RESET;
                std::cin >> solveChoice;
                if (std::cin.fail() || solveChoice < 1 || solveChoice > 4) { std::cout << RED << "Invalid choice.\n" << RESET; std::cin.clear(); std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); solveChoice = 0; }
            }
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // Clear newline

            std::vector<SolveResult> results; // Store results for comparison
            std::chrono::high_resolution_clock::time_point t1, t2;
            std::chrono::duration<double, std::milli> ms_double;

            // Run selected algorithm(s)
            if (solveChoice == 1 || solveChoice == 4) {
                std::cout << YELLOW << "\nSolving using BFS..." << RESET << std::endl;
                if (delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Short pause before visualization
                t1 = std::chrono::high_resolution_clock::now(); auto p = maze.solveBFS(); t2 = std::chrono::high_resolution_clock::now();
                ms_double = t2 - t1; results.push_back({ "BFS", p, ms_double.count() });
                maze.display(p, true); // Display final path
                if (solveChoice != 4) std::cout << GREEN << "BFS Finished." << RESET << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Pause
            }
            if (solveChoice == 2 || solveChoice == 4) {
                std::cout << YELLOW << "\nSolving using DFS..." << RESET << std::endl;
                if (delay > 0 && solveChoice == 4) std::this_thread::sleep_for(std::chrono::milliseconds(200));
                t1 = std::chrono::high_resolution_clock::now(); auto p = maze.solveDFS(); t2 = std::chrono::high_resolution_clock::now();
                ms_double = t2 - t1; results.push_back({ "DFS", p, ms_double.count() });
                maze.display(p, true);
                if (solveChoice != 4) std::cout << GREEN << "DFS Finished." << RESET << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            if (solveChoice == 3 || solveChoice == 4) {
                std::cout << YELLOW << "\nSolving using A*..." << RESET << std::endl;
                if (delay > 0 && solveChoice == 4) std::this_thread::sleep_for(std::chrono::milliseconds(200));
                t1 = std::chrono::high_resolution_clock::now(); auto p = maze.solveAStar(); t2 = std::chrono::high_resolution_clock::now();
                ms_double = t2 - t1; results.push_back({ "A*", p, ms_double.count() });
                maze.display(p, true);
                if (solveChoice != 4) std::cout << GREEN << "A* Finished." << RESET << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            // Display Summary of Results
            std::cout << "\n--- Solver Results ---" << std::endl;
            for (const auto& r : results) {
                std::cout << CYAN << BOLD << r.algoName << ":" << RESET << std::endl;
                if (!r.path.empty()) {
                    std::cout << GREEN << "  Path found! " << RESET
                        << "Length: " << r.path.size() << " steps. Time: " << std::fixed << std::setprecision(3) << r.timeMs << " ms." << std::endl;
                }
                else {
                    std::cout << RED << "  Path not found. " << RESET
                        << "Time: " << std::fixed << std::setprecision(3) << r.timeMs << " ms." << std::endl;
                }
                std::cout << "----------------------" << std::endl;
            }
            // Display the last computed path again for context
            if (!results.empty()) maze.display(results.back().path, true);

            break;
        }
        case 4: { // Save Maze
            if (!mazeLoadedOrGenerated || !maze.isGenerated()) {
                std::cout << RED << "No maze loaded or generated yet to save." << RESET << std::endl;
                break;
            }
            std::string filename;
            std::cout << BLUE << "Enter filename to save: " << RESET;
            std::getline(std::cin, filename);
            if (maze.saveToFile(filename)) {
                std::cout << GREEN << "Maze saved successfully to '" << filename << "'." << RESET << std::endl;
            }
            else {
                // Error message is printed within saveToFile
            }
            break;
        }
        case 5: { // Set Delay
            std::cout << BLUE << "Enter new visualization delay in milliseconds (0 for none): " << RESET;
            int newDelay;
            std::cin >> newDelay;
            if (std::cin.fail()) {
                std::cout << RED << "Invalid input.\n" << RESET;
                std::cin.clear();
            }
            else {
                // Use std::max to set delay
                delay = std::max(0, newDelay);
                maze.setVisualizationDelay(delay);
                std::cout << GREEN << "Delay set to " << delay << "ms." << RESET << std::endl;
            }
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // Clear newline
            break;
        }
        case 6: { // Display Maze
            if (!mazeLoadedOrGenerated || !maze.isGenerated()) {
                std::cout << RED << "No maze loaded or generated yet to display." << RESET << std::endl;
            }
            else {
                std::cout << YELLOW << "Displaying current maze state:" << RESET << std::endl;
                maze.display(); // Display the maze without solution path
            }
            break;
        }
        case 0: { // Exit
            std::cout << YELLOW << "Exiting program." << RESET << std::endl;
            displayAZD(); // Display final signature
            return 0;
        }
        default:
            std::cout << RED << "Invalid choice. Please try again." << RESET << std::endl;
            break;
        }
        std::cout << YELLOW << "\nPress Enter to return to menu..." << RESET;
        // Wait for user to press Enter before showing menu again
        // Clear potential leftover newline first, then wait for Enter
        // std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n'); // Already done after main input
        std::cin.get();

    }

    return 0; // Should not be reached
}