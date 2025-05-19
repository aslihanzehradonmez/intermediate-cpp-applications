#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <string>
#include <cmath>
#include <limits> // Required for numeric_limits
#include <cctype> // Required for isdigit, tolower

// ANSI Color Codes (same as before)
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string BG_BLACK = "\033[40m";
const std::string BG_BLUE = "\033[44m";
const std::string BG_CYAN = "\033[46m";
const std::string BG_WHITE = "\033[47m";

const int GRID_SIZE = 9;
const int SUBGRID_SIZE = 3;
const int EMPTY_CELL = 0;

// Difficulty levels mapping (approximate number of empty cells)
const int EASY_DIFFICULTY = 35;
const int MEDIUM_DIFFICULTY = 45;
const int HARD_DIFFICULTY = 55;
const int EXPERT_DIFFICULTY = 60; // Adjusted slightly for solvability variance


class Sudoku {
private:
    std::vector<std::vector<int>> grid;
    std::vector<std::vector<int>> initialGrid; // Stores the puzzle state before solving
    std::mt19937 rng;

    bool findEmpty(int& row, int& col) {
        for (row = 0; row < GRID_SIZE; ++row) {
            for (col = 0; col < GRID_SIZE; ++col) {
                if (grid[row][col] == EMPTY_CELL) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isSafe(int row, int col, int num) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            if (grid[row][x] == num || grid[x][col] == num) {
                return false;
            }
        }
        int startRow = row - row % SUBGRID_SIZE;
        int startCol = col - col % SUBGRID_SIZE;
        for (int i = 0; i < SUBGRID_SIZE; ++i) {
            for (int j = 0; j < SUBGRID_SIZE; ++j) {
                if (grid[i + startRow][j + startCol] == num) {
                    return false;
                }
            }
        }
        return true;
    }

    bool isValidInitialGrid() {
        std::vector<std::vector<int>> tempGrid = grid; // Use a copy for validation
        for (int r = 0; r < GRID_SIZE; ++r) {
            for (int c = 0; c < GRID_SIZE; ++c) {
                int num = tempGrid[r][c];
                if (num != EMPTY_CELL) {
                    tempGrid[r][c] = EMPTY_CELL; // Temporarily empty cell for check
                    if (!isSafeForGrid(tempGrid, r, c, num)) { // Use helper check
                        // Conflict found, no need to restore tempGrid[r][c] here
                        return false;
                    }
                    tempGrid[r][c] = num; // Restore number for subsequent checks
                }
            }
        }
        return true; // No conflicts found
    }

    // Helper for isValidInitialGrid to check safety on a given grid state
    bool isSafeForGrid(const std::vector<std::vector<int>>& checkGrid, int row, int col, int num) const {
        for (int x = 0; x < GRID_SIZE; ++x) {
            if (checkGrid[row][x] == num || checkGrid[x][col] == num) {
                return false;
            }
        }
        int startRow = row - row % SUBGRID_SIZE;
        int startCol = col - col % SUBGRID_SIZE;
        for (int i = 0; i < SUBGRID_SIZE; ++i) {
            for (int j = 0; j < SUBGRID_SIZE; ++j) {
                if (checkGrid[i + startRow][j + startCol] == num) {
                    return false;
                }
            }
        }
        return true;
    }


    bool solveInternal() {
        int row, col;
        if (!findEmpty(row, col)) {
            return true;
        }
        for (int num = 1; num <= GRID_SIZE; ++num) {
            if (isSafe(row, col, num)) {
                grid[row][col] = num;
                if (solveInternal()) {
                    return true;
                }
                grid[row][col] = EMPTY_CELL;
            }
        }
        return false;
    }

    void fillDiagonalSubgrids() {
        for (int i = 0; i < GRID_SIZE; i += SUBGRID_SIZE) {
            fillSubgrid(i, i);
        }
    }

    void fillSubgrid(int row, int col) {
        std::vector<int> nums = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        std::shuffle(nums.begin(), nums.end(), rng);
        int k = 0;
        for (int i = 0; i < SUBGRID_SIZE; ++i) {
            for (int j = 0; j < SUBGRID_SIZE; ++j) {
                grid[row + i][col + j] = nums[k++];
            }
        }
    }

    bool fillRemaining(int r, int c) {
        if (c >= GRID_SIZE && r < GRID_SIZE - 1) {
            r = r + 1;
            c = 0;
        }
        if (r >= GRID_SIZE && c >= GRID_SIZE) {
            return true;
        }
        if (r < SUBGRID_SIZE) {
            if (c < SUBGRID_SIZE) c = SUBGRID_SIZE;
        }
        else if (r < GRID_SIZE - SUBGRID_SIZE) {
            if (c == (int)(r / SUBGRID_SIZE) * SUBGRID_SIZE) c = c + SUBGRID_SIZE;
        }
        else {
            if (c == GRID_SIZE - SUBGRID_SIZE) {
                r = r + 1;
                c = 0;
                if (r >= GRID_SIZE) return true;
            }
        }
        std::vector<int> numbers = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        std::shuffle(numbers.begin(), numbers.end(), rng);
        for (int num : numbers) {
            if (isSafe(r, c, num)) {
                grid[r][c] = num;
                if (fillRemaining(r, c + 1)) {
                    return true;
                }
                grid[r][c] = EMPTY_CELL;
            }
        }
        return false;
    }

    void removeDigits(int cellsToRemove) {
        int count = cellsToRemove;
        // Create a list of all cell coordinates
        std::vector<std::pair<int, int>> cells;
        for (int r = 0; r < GRID_SIZE; ++r) for (int c = 0; c < GRID_SIZE; ++c) cells.push_back({ r,c });
        std::shuffle(cells.begin(), cells.end(), rng); // Shuffle coordinates randomly

        int removedCount = 0;
        for (const auto& cell : cells) {
            if (grid[cell.first][cell.second] != EMPTY_CELL) {
                grid[cell.first][cell.second] = EMPTY_CELL;
                removedCount++;
                if (removedCount >= count) break; // Stop when enough cells are removed
            }
        }

        initialGrid = grid; // Save the puzzle state
    }

public:
    Sudoku() : grid(GRID_SIZE, std::vector<int>(GRID_SIZE, 0)), initialGrid(GRID_SIZE, std::vector<int>(GRID_SIZE, 0)), rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    void generatePuzzle(int cellsToRemove = MEDIUM_DIFFICULTY) {
        grid.assign(GRID_SIZE, std::vector<int>(GRID_SIZE, 0));
        fillDiagonalSubgrids();
        fillRemaining(0, SUBGRID_SIZE);
        removeDigits(cellsToRemove);
    }

    bool loadGridFromInput() {
        std::cout << YELLOW << "Enter the Sudoku puzzle row by row (9 digits per row, use 0 for empty cells):\n" << RESET;
        grid.assign(GRID_SIZE, std::vector<int>(GRID_SIZE, 0));
        std::string rowStr;

        for (int i = 0; i < GRID_SIZE; ++i) {
            bool rowOk = false;
            while (!rowOk) {
                std::cout << "Row " << (i + 1) << ": ";
                // Read the whole line to handle potential extra characters
                if (!std::getline(std::cin, rowStr)) {
                    std::cout << RED << "Error reading input. Exiting.\n" << RESET;
                    return false; // Handle potential stream error
                }

                // Trim leading/trailing whitespace (optional but good practice)
                rowStr.erase(0, rowStr.find_first_not_of(" \t\n\r\f\v"));
                rowStr.erase(rowStr.find_last_not_of(" \t\n\r\f\v") + 1);


                if (rowStr.length() != GRID_SIZE) {
                    std::cout << RED << "Error: Input must be exactly 9 digits long. You entered " << rowStr.length() << " characters. Try again.\n" << RESET;
                    continue;
                }
                bool allDigits = true;
                for (int j = 0; j < GRID_SIZE; ++j) {
                    if (!isdigit(rowStr[j])) {
                        std::cout << RED << "Error: Input must contain only digits (0-9). Found '" << rowStr[j] << "'. Try again.\n" << RESET;
                        allDigits = false;
                        break;
                    }
                    grid[i][j] = rowStr[j] - '0';
                }
                if (allDigits) {
                    rowOk = true;
                }
            }
        }

        if (!isValidInitialGrid()) {
            std::cout << RED << BOLD << "\nError: The entered puzzle has conflicts (violates Sudoku rules).\n" << RESET;
            grid.assign(GRID_SIZE, std::vector<int>(GRID_SIZE, 0)); // Clear invalid grid
            initialGrid = grid;
            return false;
        }

        initialGrid = grid;
        return true;
    }

    bool solve() {
        // Ensure we are solving from the initial state, allowing re-solving attempts
        grid = initialGrid;
        return solveInternal();
    }

    // NEW METHOD: Explains the solution by listing filled cells
    void explainSolution() const {
        std::cout << BLUE << BOLD << "\n--- Solution Explanation (Backtracking) ---" << RESET << std::endl;
        std::cout << CYAN << "The solver works by trying numbers in empty cells." << std::endl;
        std::cout << CYAN << "If a number leads to a dead end, it backtracks and tries another." << std::endl;
        std::cout << CYAN << "The following cells were filled to complete the puzzle:" << RESET << std::endl;

        int filledCount = 0;
        for (int r = 0; r < GRID_SIZE; ++r) {
            for (int c = 0; c < GRID_SIZE; ++c) {
                // Check if cell was empty initially but is filled now
                if (initialGrid[r][c] == EMPTY_CELL && grid[r][c] != EMPTY_CELL) {
                    std::cout << "  - Placed " << GREEN << BOLD << grid[r][c] << RESET
                        << " at (Row " << BOLD << (r + 1) << RESET
                        << ", Col " << BOLD << (c + 1) << RESET << ")" << std::endl;
                    filledCount++;
                }
            }
        }

        if (filledCount == 0) {
            std::cout << YELLOW << "  (The puzzle was already solved or had no empty cells)" << RESET << std::endl;
        }
        else {
            std::cout << GREEN << "\nSolver successfully filled " << filledCount << " empty cells." << RESET << std::endl;
        }
        std::cout << BLUE << BOLD << "-------------------------------------------" << RESET << std::endl;
    }


    void printGrid(const std::string& title, bool showInitialHighlights = false) const {
        std::cout << BG_BLUE << WHITE << BOLD << "\n+-------------------------------------+" << RESET << "\n";
        std::cout << BG_BLUE << WHITE << BOLD << "|            " << std::setw(18) << std::left << title << " |" << RESET << "\n";
        std::cout << BG_BLUE << WHITE << BOLD << "+-------------------------------------+" << RESET << "\n";

        const std::vector<std::vector<int>>& gridToPrint = grid;

        for (int i = 0; i < GRID_SIZE; ++i) {
            if (i % SUBGRID_SIZE == 0 && i != 0) {
                std::cout << CYAN << "|-----------+-----------+-----------|" << RESET << "\n";
            }
            std::cout << CYAN << "| " << RESET;
            for (int j = 0; j < GRID_SIZE; ++j) {
                if (j % SUBGRID_SIZE == 0 && j != 0) {
                    std::cout << CYAN << "| " << RESET;
                }

                int val = gridToPrint[i][j];
                std::string color = WHITE;

                if (val != EMPTY_CELL) {
                    if (showInitialHighlights) {
                        color = MAGENTA; // Highlight all numbers in initial display
                    }
                    else {
                        if (initialGrid[i][j] != EMPTY_CELL) {
                            color = MAGENTA; // Original number
                        }
                        else {
                            color = GREEN;   // Solved number
                        }
                    }
                }
                else {
                    color = YELLOW; // Empty cell
                }


                if (val == EMPTY_CELL) {
                    std::cout << color << BOLD << ". " << RESET;
                }
                else {
                    std::cout << color << BOLD << val << " " << RESET;
                }
            }
            std::cout << CYAN << "|" << RESET << "\n";
        }
        std::cout << BG_BLUE << WHITE << BOLD << "+-------------------------------------+" << RESET << "\n";
    }

    static void displayAZD() {
        // AZD display code remains the same
        std::cout << "\n\n";
        std::cout << RED << BOLD << "             AAA             ZZZZZZZZZZZZZ     DDDDDDDD       " << RESET << "\n";
        std::cout << RED << BOLD << "            AAAAA            ZZZZZZZZZZZZZ     DDDDDDDDDD     " << RESET << "\n";
        std::cout << RED << BOLD << "           AAA AAA                  ZZZ        DDD     DDD    " << RESET << "\n";
        std::cout << RED << BOLD << "          AAA   AAA                ZZZ         DDD     DDD    " << RESET << "\n";
        std::cout << RED << BOLD << "         AAAAAAAAAAA              ZZZ          DDD     DDD    " << RESET << "\n";
        std::cout << RED << BOLD << "        AAAAAAAAAAAAA            ZZZ           DDD     DDD    " << RESET << "\n";
        std::cout << RED << BOLD << "       AAA         AAA          ZZZ            DDD     DDD    " << RESET << "\n";
        std::cout << RED << BOLD << "      AAA           AAA      ZZZZZZZZZZZZZ     DDDDDDDDDD     " << RESET << "\n";
        std::cout << RED << BOLD << "     AAA             AAA     ZZZZZZZZZZZZZ     DDDDDDDD       " << RESET << "\n";
        std::cout << "\n\n";
    }
};

int main() {
    Sudoku game;
    char choice;
    bool puzzleReady = false;

    std::cout << BOLD << CYAN << "Welcome to the Advanced Sudoku Generator/Solver!" << RESET << std::endl;

    while (!puzzleReady) {
        std::cout << YELLOW << "\nChoose an option: (G)enerate puzzle, (L)oad puzzle: " << RESET;
        std::string lineInput;
        // Read the whole line to avoid issues with leftover input
        if (!std::getline(std::cin, lineInput) || lineInput.empty()) {
            continue; // Handle potential empty input or stream error
        }
        choice = std::tolower(lineInput[0]); // Take the first character, lowercased


        if (choice == 'g') {
            char difficultyChoiceChar;
            int cellsToRemove = MEDIUM_DIFFICULTY; // Default
            bool difficultySelected = false;

            while (!difficultySelected) {
                std::cout << YELLOW << "\nSelect puzzle difficulty:" << RESET << std::endl;
                std::cout << "  (E)asy   " << CYAN << "(~" << EASY_DIFFICULTY << " empty cells)" << RESET << std::endl;
                std::cout << "  (M)edium " << CYAN << "(~" << MEDIUM_DIFFICULTY << " empty cells, default)" << RESET << std::endl;
                std::cout << "  (H)ard   " << CYAN << "(~" << HARD_DIFFICULTY << " empty cells)" << RESET << std::endl;
                std::cout << "  (X)pert  " << CYAN << "(~" << EXPERT_DIFFICULTY << " empty cells)" << RESET << std::endl;
                std::cout << "Enter choice (E/M/H/X): ";

                if (!std::getline(std::cin, lineInput) || lineInput.empty()) {
                    std::cout << RED << "Invalid input. Please try again." << RESET << std::endl;
                    continue;
                }
                difficultyChoiceChar = std::tolower(lineInput[0]);


                switch (difficultyChoiceChar) {
                case 'e':
                    cellsToRemove = EASY_DIFFICULTY;
                    difficultySelected = true;
                    break;
                case 'm':
                    cellsToRemove = MEDIUM_DIFFICULTY;
                    difficultySelected = true;
                    break;
                case 'h':
                    cellsToRemove = HARD_DIFFICULTY;
                    difficultySelected = true;
                    break;
                case 'x':
                    cellsToRemove = EXPERT_DIFFICULTY;
                    difficultySelected = true;
                    break;
                default:
                    std::cout << RED << "Invalid difficulty choice. Please enter E, M, H, or X." << RESET << std::endl;
                }
            }

            std::cout << BLUE << "\nGenerating Sudoku puzzle..." << RESET << std::endl;
            game.generatePuzzle(cellsToRemove);
            puzzleReady = true;

        }
        else if (choice == 'l') {
            std::cout << BLUE << "\nLoading Sudoku puzzle from input..." << RESET << std::endl;
            // Pass std::cin to loadGridFromInput is not needed as it uses std::cin directly
            if (game.loadGridFromInput()) {
                puzzleReady = true;
            }
            else {
                std::cout << RED << "Failed to load puzzle. Please try again.\n" << RESET;
                // Loop will continue asking G/L
            }
        }
        else {
            std::cout << RED << "Invalid choice. Please enter 'G' or 'L'." << RESET << std::endl;
        }
    }

    // Display the initial puzzle
    game.printGrid("Initial Puzzle", true);

    std::string command;
    std::cout << YELLOW << "\nPuzzle ready. Type '" << BOLD << "solve" << RESET << YELLOW << "' and press Enter to see the solution: " << RESET;

    // Read the command using getline to handle potential extra input properly
    while (std::getline(std::cin, command) && command != "solve") {
        // Trim whitespace for robustness
        command.erase(0, command.find_first_not_of(" \t\n\r\f\v"));
        command.erase(command.find_last_not_of(" \t\n\r\f\v") + 1);
        if (command == "solve") break; // Exit if command is now correct after trimming

        std::cout << RED << "Unknown command. Type '" << BOLD << "solve" << RESET << RED << "' to continue: " << RESET;
    }
    // Check if loop exited due to stream error or EOF before getting "solve"
    if (command != "solve") {
        std::cerr << RED << "Failed to get solve command. Exiting." << RESET << std::endl;
        return 1;
    }


    std::cout << BLUE << "\nSolving Sudoku puzzle..." << RESET << std::endl;
    if (game.solve()) {
        game.printGrid("Solved Puzzle"); // Print solved grid
        game.explainSolution(); // <-- NEW: Explain the solution
    }
    else {
        std::cout << RED << BOLD << "This puzzle has no solution according to the solver." << RESET << std::endl;
    }

    Sudoku::displayAZD();

    return 0;
}