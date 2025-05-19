#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <limits>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifdef _WIN32
void enableVirtualTerminalProcessing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return;
    }
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return;
    }
}
#endif

struct Card {
    char symbol;
    bool isVisible;
    bool isMatched;
    std::string colorCode;
};

struct HighScoreEntry {
    std::string playerName;
    int score;
    double timeTaken;

    bool operator<(const HighScoreEntry& other) const {
        if (score != other.score) {
            return score > other.score;
        }
        return timeTaken < other.timeTaken;
    }
};

class Board {
public:
    std::vector<std::vector<Card>> grid;
    int rows;
    int cols;
    std::vector<char> symbolsSet = {
        '$', '%', '&', '@', '#', '!', '*', '+', '=', '?',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'
    };
    std::vector<std::string> colorSet = {
        "\033[91m", "\033[92m", "\033[93m", "\033[94m", "\033[95m", "\033[96m",
        "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m",
        "\033[1;91m", "\033[1;92m", "\033[1;93m", "\033[1;94m", "\033[1;95m", "\033[1;96m"
    };

    Board() : rows(0), cols(0) {}

    void initialize(int r, int c) {
        rows = r;
        cols = c;
        if ((rows * cols) % 2 != 0) {
            throw std::invalid_argument("Board dimensions must result in an even number of cells.");
        }
        grid.assign(rows, std::vector<Card>(cols));

        int numPairs = (rows * cols) / 2;
        if (numPairs > static_cast<int>(symbolsSet.size())) {
            throw std::out_of_range("Not enough unique symbols for this board size.");
        }

        std::vector<std::pair<char, std::string>> cardValues;
        unsigned int seed_val = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
        std::mt19937 rng(seed_val);

        std::vector<char> currentSymbols = symbolsSet;
        std::shuffle(currentSymbols.begin(), currentSymbols.end(), rng);

        std::vector<std::string> currentColors = colorSet;
        std::shuffle(currentColors.begin(), currentColors.end(), rng);

        for (int i = 0; i < numPairs; ++i) {
            char symbol = currentSymbols[i];
            std::string color = currentColors[i % currentColors.size()];
            cardValues.push_back({ symbol, color });
            cardValues.push_back({ symbol, color });
        }

        std::shuffle(cardValues.begin(), cardValues.end(), rng);

        int k = 0;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                grid[i][j].symbol = cardValues[k].first;
                grid[i][j].colorCode = cardValues[k].second;
                grid[i][j].isVisible = false;
                grid[i][j].isMatched = false;
                k++;
            }
        }
    }

    void display() const {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        std::cout << "\n\033[1;36m    ";
        for (int j = 0; j < cols; ++j) {
            std::cout << std::setw(5) << j + 1 << " ";
        }
        std::cout << "\n  +";
        for (int j = 0; j < cols; ++j) {
            std::cout << "-----+";
        }
        std::cout << "\n";

        for (int i = 0; i < rows; ++i) {
            std::cout << "\033[1;36m" << std::setw(2) << i + 1 << "\033[0m |";
            for (int j = 0; j < cols; ++j) {
                std::string card_content;
                std::string symbol_str(1, grid[i][j].symbol);

                if (grid[i][j].isMatched) {
                    card_content = grid[i][j].colorCode + "\033[1m  " + symbol_str + "  \033[0m";
                    std::cout << "\033[42m" << card_content << "\033[0m";
                }
                else if (grid[i][j].isVisible) {
                    card_content = grid[i][j].colorCode + "\033[1m >" + symbol_str + "< \033[0m";
                    std::cout << "\033[103m" << card_content << "\033[0m";
                }
                else {
                    std::cout << "\033[100m\033[97m\033[1m  ?  \033[0m";
                }
                std::cout << "|";
            }
            std::cout << "\n  +";
            for (int j = 0; j < cols; ++j) {
                std::cout << "-----+";
            }
            std::cout << "\n";
        }
        std::cout << "\033[0m";
    }

    bool allCardsMatched() const {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (!grid[i][j].isMatched) {
                    return false;
                }
            }
        }
        return true;
    }
};

class Game {
public:
    Board board;
    int score;
    int attempts;
    int currentDifficultyIndex;
    int boardRows;
    int boardCols;
    int consecutiveMatches;

    static const int NUM_DIFFICULTY_LEVELS = 4;
    static const int MAX_HIGH_SCORES_PER_DIFFICULTY = 5;
    std::vector<HighScoreEntry> highScores[NUM_DIFFICULTY_LEVELS];
    std::string difficultyNames[NUM_DIFFICULTY_LEVELS] = { "Easy", "Medium", "Hard", "Expert" };
    std::string highScoreFilenames[NUM_DIFFICULTY_LEVELS] = {
        "memory_game_hs_easy.txt",
        "memory_game_hs_medium.txt",
        "memory_game_hs_hard.txt",
        "memory_game_hs_expert.txt"
    };

    Game() : score(0), attempts(0), currentDifficultyIndex(0), boardRows(0), boardCols(0), consecutiveMatches(0) {
        for (int i = 0; i < NUM_DIFFICULTY_LEVELS; ++i) {
            loadHighScores(i);
        }
    }

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    void displayWelcomeBanner() {
        std::cout << "\033[1;35m" << std::string(65, '*') << "\033[0m\n";
        std::cout << "\033[1;35m*\033[0m" << std::string(63, ' ') << "\033[1;35m*\033[0m\n";
        std::cout << "\033[1;35m* \033[1;36m         WELCOME TO THE ADVANCED MEMORY GAME             \033[1;35m*\033[0m\n";
        std::cout << "\033[1;35m*\033[0m" << std::string(63, ' ') << "\033[1;35m*\033[0m\n";
        std::cout << "\033[1;35m" << std::string(65, '*') << "\033[0m\n\n";
    }

    void displayHowToPlay() {
        clearScreen();
        displayWelcomeBanner();
        std::cout << "\033[1;34m-------------------- How to Play --------------------\033[0m\n\n";
        std::cout << "\033[36mObjective:\033[0m Match all pairs of symbols on the board.\n\n";
        std::cout << "\033[36mGameplay:\033[0m\n";
        std::cout << "  1. On your turn, you will select two cards.\n";
        std::cout << "  2. To select a card, enter its \033[1mrow number\033[0m, followed by a space,\n";
        std::cout << "     then its \033[1mcolumn number\033[0m (e.g., '\033[1;33m1 2\033[0m' for row 1, column 2).\n";
        std::cout << "  3. \033[1;32mIf the symbols on the two selected cards match:\033[0m\n";
        std::cout << "     They will remain revealed (\033[42m\033[30;1m Symbol \033[0m example).\n";
        std::cout << "     You score points! You also get bonus points for consecutive matches.\n";
        std::cout << "  4. \033[1;31mIf they do not match:\033[0m\n";
        std::cout << "     The cards will be hidden again after a short moment.\n";
        std::cout << "  5. Hidden cards are shown as \033[100m\033[97m\033[1m  ?  \033[0m.\n";
        std::cout << "  6. When you select a card, it will be highlighted (e.g., \033[103m\033[30;1m >S< \033[0m).\n\n";
        std::cout << "\033[36mGoal:\033[0m Find all pairs with the fewest attempts and highest score!\n\n";
        std::cout << "\033[1;34m-----------------------------------------------------\033[0m\n\n";
        std::cout << "Press Enter to return to the Main Menu...";
        std::cin.get();
    }

    void showMainMenu() {
        int choice;
        do {
            clearScreen();
            displayWelcomeBanner();
            std::cout << "\033[1;34mMain Menu:\033[0m\n";
            std::cout << "  \033[32m1. Play Game\033[0m\n";
            std::cout << "  \033[33m2. View High Scores\033[0m\n";
            std::cout << "  \033[36m3. How to Play\033[0m\n";
            std::cout << "  \033[31m4. Exit\033[0m\n";
            std::cout << "\033[1;34mEnter your choice (1-4): \033[0m";

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                choice = 0;
            }
            else {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            switch (choice) {
            case 1:
                playGameSession();
                break;
            case 2:
                displayHighScoresMenu();
                break;
            case 3:
                displayHowToPlay();
                break;
            case 4:
                std::cout << "\033[1;36mThank you for playing! Goodbye!\033[0m\n";
                displayAZD();
                break;
            default:
                std::cout << "\033[1;31mInvalid choice. Please press Enter and try again.\033[0m";
                std::cin.get();
                break;
            }
        } while (choice != 4);
    }

    void selectGameDifficulty() {
        clearScreen();
        displayWelcomeBanner();
        std::cout << "\033[1;34mSelect Difficulty Level:\033[0m\n";
        std::cout << "  \033[32m1. Easy   (2x2 Board)\033[0m\n";
        std::cout << "  \033[33m2. Medium (4x4 Board)\033[0m\n";
        std::cout << "  \033[31m3. Hard   (4x6 Board)\033[0m\n";
        std::cout << "  \033[35m4. Expert (6x6 Board)\033[0m\n";
        std::cout << "\033[1;34mEnter your choice (1-4): \033[0m";

        int choice;
        while (!(std::cin >> choice) || choice < 1 || choice > NUM_DIFFICULTY_LEVELS) {
            std::cout << "\033[1;31mInvalid input. Please enter a number between 1 and " << NUM_DIFFICULTY_LEVELS << ": \033[0m";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


        currentDifficultyIndex = choice - 1;
        switch (currentDifficultyIndex) {
        case 0: boardRows = 2; boardCols = 2; break;
        case 1: boardRows = 4; boardCols = 4; break;
        case 2: boardRows = 4; boardCols = 6; break;
        case 3: boardRows = 6; boardCols = 6; break;
        }
    }

    std::pair<int, int> getPlayerChoice(int cardTurnNumber, int firstCardRow = -1, int firstCardCol = -1) {
        int r, c;
        while (true) {
            if (cardTurnNumber == 1) {
                std::cout << "\033[1;37mSelect \033[1;33mFIRST\033[1;37m card (row col): \033[0m";
            }
            else {
                std::cout << "\033[1;37mSelect \033[1;33mSECOND\033[1;37m card (row col): \033[0m";
            }

            if (!(std::cin >> r >> c)) {
                std::cout << "\033[1;31mInvalid input format. Please enter two numbers (e.g., 1 2).\033[0m\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            r--;
            c--;

            if (r < 0 || r >= boardRows || c < 0 || c >= boardCols) {
                std::cout << "\033[1;31mCoordinates out of bounds. Try again.\033[0m\n";
            }
            else if (board.grid[r][c].isMatched) {
                std::cout << "\033[1;31mThis card is already matched. Choose another card.\033[0m\n";
            }
            else if (cardTurnNumber == 2 && r == firstCardRow && c == firstCardCol) {
                std::cout << "\033[1;31mYou selected the same card twice. Choose a different second card.\033[0m\n";
            }
            else if (board.grid[r][c].isVisible && cardTurnNumber == 2) { // Should not happen if logic is correct
                std::cout << "\033[1;31mThis card is already selected as the first card. Choose a different second card.\033[0m\n";
            }
            else {
                return { r, c };
            }
        }
    }

    void displayGameInfo() const {
        std::cout << "\n\033[1m";
        std::cout << "\033[1;32mScore: " << std::setw(5) << score << "  ";
        std::cout << "\033[1;31mAttempts: " << std::setw(3) << attempts << "  ";
        std::cout << "\033[1;33mConsecutive: " << std::setw(2) << consecutiveMatches << "\033[0m\n\n";
    }

    void loadHighScores(int difficultyIdx) {
        highScores[difficultyIdx].clear();
        std::ifstream inFile(highScoreFilenames[difficultyIdx]);
        if (inFile.is_open()) {
            HighScoreEntry entry;
            while (inFile >> entry.playerName >> entry.score >> entry.timeTaken) {
                highScores[difficultyIdx].push_back(entry);
            }
            inFile.close();
            std::sort(highScores[difficultyIdx].begin(), highScores[difficultyIdx].end());
        }
    }

    void saveHighScores(int difficultyIdx) {
        std::ofstream outFile(highScoreFilenames[difficultyIdx]);
        if (outFile.is_open()) {
            for (const auto& entry : highScores[difficultyIdx]) {
                outFile << entry.playerName << " " << entry.score << " " << entry.timeTaken << "\n";
            }
            outFile.close();
        }
    }

    void addHighScore(int difficultyIdx, const std::string& name, int playerScore, double playerTime) {
        HighScoreEntry newEntry = { name, playerScore, playerTime };
        highScores[difficultyIdx].push_back(newEntry);
        std::sort(highScores[difficultyIdx].begin(), highScores[difficultyIdx].end());
        if (highScores[difficultyIdx].size() > MAX_HIGH_SCORES_PER_DIFFICULTY) {
            highScores[difficultyIdx].resize(MAX_HIGH_SCORES_PER_DIFFICULTY);
        }
        saveHighScores(difficultyIdx);
    }

    void displayHighScoresMenu() {
        clearScreen();
        std::cout << "\033[1;34mView High Scores by Difficulty:\033[0m\n";
        for (int i = 0; i < NUM_DIFFICULTY_LEVELS; ++i) {
            std::cout << "  \033[32m" << (i + 1) << ". " << difficultyNames[i] << "\033[0m\n";
        }
        std::cout << "  \033[31m" << (NUM_DIFFICULTY_LEVELS + 1) << ". Back to Main Menu\033[0m\n";
        std::cout << "\033[1;34mEnter your choice: \033[0m";

        int choice;
        if (!(std::cin >> choice) || choice < 1 || choice > NUM_DIFFICULTY_LEVELS + 1) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\033[1;31mInvalid choice. Press Enter to continue.\033[0m";
            std::cin.get();
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice > 0 && choice <= NUM_DIFFICULTY_LEVELS) {
            displaySpecificHighScores(choice - 1);
        }
    }

    void displaySpecificHighScores(int difficultyIdx) {
        clearScreen();
        std::cout << "\033[1;35m--- High Scores for " << difficultyNames[difficultyIdx] << " (" << highScoreFilenames[difficultyIdx] << ") ---\033[0m\n";
        if (highScores[difficultyIdx].empty()) {
            std::cout << "\n\033[33mNo high scores recorded for this difficulty yet.\033[0m\n";
        }
        else {
            std::cout << "\033[1;36mRank | Player Name      | Score | Time (s)\033[0m\n";
            std::cout << std::string(50, '-') << "\n";
            int rank = 1;
            for (const auto& entry : highScores[difficultyIdx]) {
                std::cout << "\033[32m" << std::setw(4) << std::left << rank++ << "| "
                    << std::setw(16) << std::left << entry.playerName << " | "
                    << std::setw(5) << std::right << entry.score << " | "
                    << std::fixed << std::setprecision(2) << std::setw(8) << std::right << entry.timeTaken << "\033[0m\n";
            }
        }
        std::cout << "\n\033[1;34mPress Enter to return to the high scores menu...\033[0m";
        std::cin.get();
    }


    void playGameSession() {
        selectGameDifficulty();
        try {
            board.initialize(boardRows, boardCols);
        }
        catch (const std::exception& e) {
            std::cerr << "\033[1;31mError initializing board: " << e.what() << "\033[0m\n";
            std::cout << "Press Enter to return to main menu...";
            std::cin.get();
            return;
        }

        score = 0;
        attempts = 0;
        consecutiveMatches = 0;
        auto startTime = std::chrono::steady_clock::now();

        while (!board.allCardsMatched()) {
            board.display();
            displayGameInfo();

            std::pair<int, int> choice1 = getPlayerChoice(1);
            Card& card1_ref = board.grid[choice1.first][choice1.second];
            card1_ref.isVisible = true;

            board.display();
            displayGameInfo();

            std::pair<int, int> choice2 = getPlayerChoice(2, choice1.first, choice1.second);
            Card& card2_ref = board.grid[choice2.first][choice2.second];
            card2_ref.isVisible = true;

            board.display();
            attempts++;

            if (card1_ref.symbol == card2_ref.symbol) {
                std::cout << "\033[1;32mIt's a MATCH! Well done!\033[0m \a\n";
                card1_ref.isMatched = true;
                card2_ref.isMatched = true;
                // isVisible remains true for matched cards, display logic handles the green look
                score += 100 + (consecutiveMatches * 20);
                consecutiveMatches++;
            }
            else {
                std::cout << "\033[1;31mNo Match! The cards were " << card1_ref.colorCode << card1_ref.symbol << "\033[0m and " << card2_ref.colorCode << card2_ref.symbol << "\033[0m\033[1;31m. They will be hidden.\033[0m \a\n";
                consecutiveMatches = 0;
                std::this_thread::sleep_for(std::chrono::seconds(3)); // Increased pause to see the cards
                card1_ref.isVisible = false;
                card2_ref.isVisible = false;
                score = std::max(0, score - 10);
            }
            displayGameInfo();
            if (!board.allCardsMatched()) {
                std::cout << "Press Enter to continue to the next turn...";
                std::cin.get();
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;

        board.display(); // Show final board
        std::cout << "\033[1;32mCongratulations! You've matched all pairs!\033[0m\n";
        std::cout << "\033[1;34mFinal Score: " << score << "\033[0m\n";
        std::cout << "\033[1;34mTotal Attempts: " << attempts << "\033[0m\n";
        std::cout << "\033[1;34mTime Taken: " << std::fixed << std::setprecision(2) << elapsedTime.count() << " seconds\033[0m\n\n";

        bool isNewHighScore = highScores[currentDifficultyIndex].size() < MAX_HIGH_SCORES_PER_DIFFICULTY;
        if (!isNewHighScore && !highScores[currentDifficultyIndex].empty()) {
            const HighScoreEntry& lastPlaceScore = highScores[currentDifficultyIndex].back();
            if (score > lastPlaceScore.score || (score == lastPlaceScore.score && elapsedTime.count() < lastPlaceScore.timeTaken)) {
                isNewHighScore = true;
            }
        }
        else if (highScores[currentDifficultyIndex].empty()) {
            isNewHighScore = true;
        }


        if (isNewHighScore) {
            std::cout << "\033[1;33mCongratulations! You've achieved a high score for " << difficultyNames[currentDifficultyIndex] << " difficulty!\033[0m\n";
            std::cout << "\033[1;37mEnter your name (up to 10 chars, no spaces): \033[0m";
            std::string playerName;
            std::cin >> playerName;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


            if (playerName.length() > 10) playerName = playerName.substr(0, 10);
            if (playerName.empty()) playerName = "Player";

            addHighScore(currentDifficultyIndex, playerName, score, elapsedTime.count());
            std::cout << "\033[1;32mHigh score saved!\033[0m\n\n";
        }

        displayAZD();
        std::cout << "\nPress Enter to return to the main menu...";
        std::cin.get();
    }

    void displayAZD() {
        std::cout << "\033[1;95m\n\n";
        std::cout << "      AAAAA      ZZZZZZZZZ   DDDDDD    \n";
        std::cout << "     AA   AA        ZZZ      DD   DD   \n";
        std::cout << "    AAAAAAAA       ZZZ       DD   DD   \n";
        std::cout << "   AA     AA     ZZZ        DD   DD   \n";
        std::cout << "  AA       AA   ZZZZZZZZZ   DDDDDD    \n";
        std::cout << "\033[0m\n";
    }
};

int main() {
#ifdef _WIN32
    enableVirtualTerminalProcessing();
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    Game memoryGame;
    memoryGame.showMainMenu();

    return 0;
}