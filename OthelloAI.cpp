#define NOMINMAX 

#ifdef _WIN32
#include <windows.h> 
#endif

#include <iostream>
#include <vector>
#include <string>
#include <algorithm> 
#include <limits>    
#include <chrono>    
#include <thread>    
#include <iomanip>   
#include <random>    

const int BOARD_SIZE = 8;
enum class PlayerColor { NONE, BLACK, WHITE };
enum class Piece { EMPTY, BLACK_PIECE, WHITE_PIECE };

Piece playerToPiece(PlayerColor p) {
    if (p == PlayerColor::BLACK) return Piece::BLACK_PIECE;
    if (p == PlayerColor::WHITE) return Piece::WHITE_PIECE;
    return Piece::EMPTY;
}

PlayerColor getOpponent(PlayerColor p) {
    return (p == PlayerColor::BLACK) ? PlayerColor::WHITE : PlayerColor::BLACK;
}

struct Move {
    int row = -1, col = -1;
    bool isValid() const { return row != -1 && col != -1; }
    bool operator==(const Move& other) const { return row == other.row && col == other.col; }
};

namespace Console {
    const std::string RESET = "\033[0m";
    const std::string FG_BLACK = "\033[30m";
    const std::string FG_RED = "\033[31m";
    const std::string FG_GREEN = "\033[32m";
    const std::string FG_YELLOW = "\033[33m";
    const std::string FG_BLUE = "\033[34m";
    const std::string FG_MAGENTA = "\033[35m";
    const std::string FG_CYAN = "\033[36m";
    const std::string FG_WHITE = "\033[37m";
    const std::string FG_BRIGHT_BLACK = "\033[90m";
    const std::string FG_BRIGHT_WHITE = "\033[97m";

    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";

    const std::string PIECE_BLACK_DISPLAY_COLOR = FG_BRIGHT_BLACK;
    const std::string PIECE_WHITE_DISPLAY_COLOR = FG_BRIGHT_WHITE;
    const std::string VALID_MOVE_BG_COLOR = BG_GREEN;
    const std::string VALID_MOVE_FG_COLOR = FG_BLACK;
    const std::string LAST_MOVE_BG_COLOR = BG_YELLOW;
    const std::string BOARD_BORDER_COLOR = FG_BLUE;
    const std::string EMPTY_CELL_CHAR_COLOR = FG_BRIGHT_BLACK;
    const char EMPTY_CELL_CHAR = '.';
    const char VALID_MOVE_CHAR = '*';
    const char PIECE_CHAR = '#';


    void setCursorPosition(int row, int col) {
#ifdef _WIN32
        COORD coordinates;
        coordinates.X = static_cast<SHORT>(col);
        coordinates.Y = static_cast<SHORT>(row);
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coordinates);
#else
        printf("\033[%d;%dH", row + 1, col + 1);
        fflush(stdout);
#endif
    }

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
    void pause(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    void hideCursor() {
#ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); CONSOLE_CURSOR_INFO i; GetConsoleCursorInfo(h, &i); i.bVisible = FALSE; SetConsoleCursorInfo(h, &i);
#else
        printf("\033[?25l"); fflush(stdout);
#endif
    }
    void showCursor() {
#ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); CONSOLE_CURSOR_INFO i; GetConsoleCursorInfo(h, &i); i.bVisible = TRUE; SetConsoleCursorInfo(h, &i);
#else
        printf("\033[?25h"); fflush(stdout);
#endif
    }
    int getTerminalWidth() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_col;
#endif
    }

    int getTerminalHeight() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_row;
#endif
    }
    void clearLine(int row, int startCol = 0) {
        setCursorPosition(row, startCol);
        printf("%s", std::string(getTerminalWidth() - startCol, ' ').c_str());
        fflush(stdout);
    }
}


class Board {
public:
    std::vector<std::vector<Piece>> grid;
    const std::vector<std::vector<int>> pieceSquareTable = {
        { 500, -150, 30, 10, 10, 30, -150,  500},
        {-150, -250,  0,  0,  0,  0, -250, -150},
        {  30,    0,  1,  2,  2,  1,    0,   30},
        {  10,    0,  2, 16, 16,  2,    0,   10},
        {  10,    0,  2, 16, 16,  2,    0,   10},
        {  30,    0,  1,  2,  2,  1,    0,   30},
        {-150, -250,  0,  0,  0,  0, -250, -150},
        { 500, -150, 30, 10, 10, 30, -150,  500}
    };

    Board() : grid(BOARD_SIZE, std::vector<Piece>(BOARD_SIZE, Piece::EMPTY)) {
        grid[3][3] = Piece::WHITE_PIECE;
        grid[3][4] = Piece::BLACK_PIECE;
        grid[4][3] = Piece::BLACK_PIECE;
        grid[4][4] = Piece::WHITE_PIECE;
    }
    Board(const Board& other) : grid(other.grid) {}

    bool isWithinBounds(int r, int c) const {
        return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
    }

    std::vector<Move> getFlipsForMove(int r_start, int c_start, PlayerColor player) const {
        std::vector<Move> allFlips;
        if (!isWithinBounds(r_start, c_start) || grid[r_start][c_start] != Piece::EMPTY) {
            return allFlips;
        }

        Piece playerPiece = playerToPiece(player);
        Piece opponentPiece = playerToPiece(getOpponent(player));

        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;

                int r = r_start + dr;
                int c = c_start + dc;
                std::vector<Move> currentLineFlips;

                while (isWithinBounds(r, c) && grid[r][c] == opponentPiece) {
                    currentLineFlips.push_back({ r, c });
                    r += dr;
                    c += dc;
                }
                if (isWithinBounds(r, c) && grid[r][c] == playerPiece && !currentLineFlips.empty()) {
                    allFlips.insert(allFlips.end(), currentLineFlips.begin(), currentLineFlips.end());
                }
            }
        }
        return allFlips;
    }

    std::vector<Move> getValidMoves(PlayerColor player) const {
        std::vector<Move> validMoves;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (grid[r][c] == Piece::EMPTY) {
                    if (!getFlipsForMove(r, c, player).empty()) {
                        validMoves.push_back({ r, c });
                    }
                }
            }
        }
        return validMoves;
    }

    void applyMove(Move move, PlayerColor player, const std::vector<Move>& flips) {
        grid[move.row][move.col] = playerToPiece(player);
        for (const auto& p : flips) {
            grid[p.row][p.col] = playerToPiece(player);
        }
    }

    std::pair<int, int> getScore() const {
        int blackScore = 0;
        int whiteScore = 0;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (grid[r][c] == Piece::BLACK_PIECE) blackScore++;
                else if (grid[r][c] == Piece::WHITE_PIECE) whiteScore++;
            }
        }
        return { blackScore, whiteScore };
    }

    bool isGameOver() const {
        return getValidMoves(PlayerColor::BLACK).empty() && getValidMoves(PlayerColor::WHITE).empty();
    }

    int evaluate(PlayerColor player) const {
        PlayerColor opponent = getOpponent(player);
        Piece playerPiece = playerToPiece(player);
        Piece opponentPiece = playerToPiece(opponent);

        int heuristicScore = 0;

        int playerDiscs = 0;
        int opponentDiscs = 0;
        int playerPieceSquareScore = 0;
        int opponentPieceSquareScore = 0;

        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (grid[r][c] == playerPiece) {
                    playerDiscs++;
                    playerPieceSquareScore += pieceSquareTable[r][c];
                }
                else if (grid[r][c] == opponentPiece) {
                    opponentDiscs++;
                    opponentPieceSquareScore += pieceSquareTable[r][c];
                }
            }
        }
        heuristicScore += (playerPieceSquareScore - opponentPieceSquareScore);

        int discDiffWeight = 15;
        size_t totalDiscs = static_cast<size_t>(playerDiscs) + opponentDiscs;
        if (totalDiscs > BOARD_SIZE * BOARD_SIZE * 0.75) {
            discDiffWeight = 100;
        }
        else if (totalDiscs < BOARD_SIZE * BOARD_SIZE * 0.25) {
            discDiffWeight = 5;
        }
        heuristicScore += (playerDiscs - opponentDiscs) * discDiffWeight;

        int playerCorners = 0, opponentCorners = 0;
        const int CORNER_BONUS = 800;
        if (grid[0][0] == playerPiece) playerCorners++; else if (grid[0][0] == opponentPiece) opponentCorners++;
        if (grid[0][BOARD_SIZE - 1] == playerPiece) playerCorners++; else if (grid[0][BOARD_SIZE - 1] == opponentPiece) opponentCorners++;
        if (grid[BOARD_SIZE - 1][0] == playerPiece) playerCorners++; else if (grid[BOARD_SIZE - 1][0] == opponentPiece) opponentCorners++;
        if (grid[BOARD_SIZE - 1][BOARD_SIZE - 1] == playerPiece) playerCorners++; else if (grid[BOARD_SIZE - 1][BOARD_SIZE - 1] == opponentPiece) opponentCorners++;
        heuristicScore += (playerCorners - opponentCorners) * CORNER_BONUS;

        size_t playerMoves = getValidMoves(player).size();
        size_t opponentMoves = getValidMoves(opponent).size();
        const int MOBILITY_WEIGHT = 50;

        if (playerMoves + opponentMoves != 0) {
            heuristicScore += MOBILITY_WEIGHT * (static_cast<int>(playerMoves) - static_cast<int>(opponentMoves));
        }
        else {
            if (playerDiscs > opponentDiscs) heuristicScore += 20000;
            else if (opponentDiscs > playerDiscs) heuristicScore -= 20000;
        }

        if (playerDiscs == 0 && playerMoves == 0 && totalDiscs > 4) return -50000;
        if (opponentDiscs == 0 && opponentMoves == 0 && totalDiscs > 4) return 50000;

        return heuristicScore;
    }
};

class AIPlayer {
public:
    int maxDepth;
    std::chrono::milliseconds timeLimitPerMove;
    std::mt19937 rng;

    AIPlayer(int depth = 4, int timeLimitMs = 1500) :
        maxDepth(depth),
        timeLimitPerMove(std::chrono::milliseconds(static_cast<long long>(timeLimitMs))),
        rng(std::random_device{}()) {
    }

    Move findBestMoveIterativeDeepening(const Board& board, PlayerColor player) {
        Move bestMoveOverall = { -1, -1 };
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<Move> validMoves = board.getValidMoves(player);
        if (validMoves.empty()) return { -1, -1 };
        if (validMoves.size() == 1) return validMoves[0];

        std::shuffle(validMoves.begin(), validMoves.end(), rng);
        bestMoveOverall = validMoves[0];

        for (int currentDepth = 1; currentDepth <= maxDepth; ++currentDepth) {
            Move bestMoveThisIteration = { -1, -1 };
            int bestScoreThisIteration = std::numeric_limits<int>::min();
            originalAIPlayer = player;

            for (const auto& move : validMoves) {
                Board nextBoard = board;
                std::vector<Move> flips = nextBoard.getFlipsForMove(move.row, move.col, player); // Get flips before applying
                nextBoard.applyMove(move, player, flips); // Pass flips to applyMove
                int score = minimax(nextBoard, currentDepth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false, player, getOpponent(player));

                if (score > bestScoreThisIteration) {
                    bestScoreThisIteration = score;
                    bestMoveThisIteration = move;
                }
            }

            if (bestMoveThisIteration.isValid()) {
                bestMoveOverall = bestMoveThisIteration;
            }

            auto currentTime = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime) >= timeLimitPerMove && currentDepth < maxDepth) {
                break;
            }
        }
        return bestMoveOverall;
    }

private:
    PlayerColor originalAIPlayer;

    int minimax(Board& board, int depth, int alpha, int beta, bool maximizingPlayer, PlayerColor aiPlayerPerspective, PlayerColor currentPlayerTurn) {
        if (depth == 0 || board.isGameOver()) {
            return board.evaluate(aiPlayerPerspective);
        }

        std::vector<Move> validMoves = board.getValidMoves(currentPlayerTurn);

        if (validMoves.empty()) {
            return minimax(board, depth - 1, alpha, beta, !maximizingPlayer, aiPlayerPerspective, getOpponent(currentPlayerTurn));
        }

        std::shuffle(validMoves.begin(), validMoves.end(), rng);

        if (maximizingPlayer) {
            int maxEval = std::numeric_limits<int>::min();
            for (const auto& move : validMoves) {
                Board nextBoard = board;
                std::vector<Move> flips = nextBoard.getFlipsForMove(move.row, move.col, currentPlayerTurn);
                nextBoard.applyMove(move, currentPlayerTurn, flips);
                int eval = minimax(nextBoard, depth - 1, alpha, beta, false, aiPlayerPerspective, getOpponent(currentPlayerTurn));
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        }
        else {
            int minEval = std::numeric_limits<int>::max();
            for (const auto& move : validMoves) {
                Board nextBoard = board;
                std::vector<Move> flips = nextBoard.getFlipsForMove(move.row, move.col, currentPlayerTurn);
                nextBoard.applyMove(move, currentPlayerTurn, flips);
                int eval = minimax(nextBoard, depth - 1, alpha, beta, true, aiPlayerPerspective, getOpponent(currentPlayerTurn));
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }
};

class UIManager {
public:
    const int boardOffsetX = 4;
    const int boardOffsetY = 2;
    const int cellDisplayWidth = 3;
    int infoPanelStartRow;

    UIManager() {
        infoPanelStartRow = boardOffsetY + BOARD_SIZE + 2;
    }

    void drawBoard(const Board& board, const std::vector<Move>* validMoves, const Move* lastMove) {
        Console::setCursorPosition(boardOffsetY - 1, boardOffsetX - 2);
        printf("%s  ", Console::BOARD_BORDER_COLOR.c_str());
        for (int c = 0; c < BOARD_SIZE; ++c) {
            printf(" %c ", (char)('A' + c));
        }
        printf("%s\n", Console::RESET.c_str());

        for (int r = 0; r < BOARD_SIZE; ++r) {
            Console::setCursorPosition(boardOffsetY + r, boardOffsetX - 3);
            printf("%s%2d %s", Console::FG_YELLOW.c_str(), r + 1, Console::RESET.c_str());

            for (int c = 0; c < BOARD_SIZE; ++c) {
                bool isLast = lastMove && lastMove->row == r && lastMove->col == c;
                bool isValidOpt = false;
                if (validMoves) {
                    for (const auto& vm : *validMoves) {
                        if (vm.row == r && vm.col == c) {
                            isValidOpt = true;
                            break;
                        }
                    }
                }

                std::string pieceCharStr = " ";
                std::string pieceColor = Console::RESET;
                std::string bgColor = Console::RESET;

                if (board.grid[r][c] == Piece::BLACK_PIECE) {
                    pieceCharStr = std::string(1, Console::PIECE_CHAR);
                    pieceColor = Console::PIECE_BLACK_DISPLAY_COLOR;
                }
                else if (board.grid[r][c] == Piece::WHITE_PIECE) {
                    pieceCharStr = std::string(1, Console::PIECE_CHAR);
                    pieceColor = Console::PIECE_WHITE_DISPLAY_COLOR;
                }
                else {
                    if (isValidOpt) {
                        pieceCharStr = std::string(1, Console::VALID_MOVE_CHAR);
                        bgColor = Console::VALID_MOVE_BG_COLOR;
                        pieceColor = Console::VALID_MOVE_FG_COLOR;
                    }
                    else {
                        pieceCharStr = std::string(1, Console::EMPTY_CELL_CHAR);
                        pieceColor = Console::EMPTY_CELL_CHAR_COLOR;
                    }
                }

                if (isLast) {
                    bgColor = Console::LAST_MOVE_BG_COLOR;
                }

                printf("%s%s%*s%*s%s",
                    bgColor.c_str(),
                    pieceColor.c_str(),
                    (cellDisplayWidth - 1) / 2 + 1, pieceCharStr.c_str(),
                    (cellDisplayWidth - 1) - (cellDisplayWidth - 1) / 2, "",
                    Console::RESET.c_str());
            }
            printf("%s\n", Console::RESET.c_str());
        }
    }

    void displayInfo(PlayerColor currentPlayer, PlayerColor humanPlayerIdentity, int blackScore, int whiteScore, const std::string& message, const std::string& aiThought = "") {
        int currentRow = infoPanelStartRow;
        Console::clearLine(currentRow);
        Console::setCursorPosition(currentRow++, boardOffsetX - 2);
        std::string turnString = "N/A";
        if (currentPlayer == PlayerColor::BLACK) turnString = "Black";
        else if (currentPlayer == PlayerColor::WHITE) turnString = "White";

        if (currentPlayer != PlayerColor::NONE) {
            if (currentPlayer == humanPlayerIdentity) turnString += " (You)";
            else turnString += " (AI)";
        }

        printf("%sTurn: %s%s%s\n",
            Console::FG_CYAN.c_str(),
            Console::FG_WHITE.c_str(),
            turnString.c_str(),
            Console::RESET.c_str()
        );

        Console::clearLine(currentRow);
        Console::setCursorPosition(currentRow++, boardOffsetX - 2);
        printf("%sScore: %sBlack %d%s - %sWhite %d%s\n",
            Console::FG_CYAN.c_str(),
            Console::PIECE_BLACK_DISPLAY_COLOR.c_str(), blackScore, Console::RESET.c_str(),
            Console::PIECE_WHITE_DISPLAY_COLOR.c_str(), whiteScore, Console::RESET.c_str()
        );

        Console::clearLine(currentRow);
        Console::setCursorPosition(currentRow++, boardOffsetX - 2);
        printf("%sMessage: %s%s%s\n",
            Console::FG_YELLOW.c_str(),
            Console::FG_WHITE.c_str(),
            message.c_str(),
            Console::RESET.c_str()
        );

        if (!aiThought.empty()) {
            Console::clearLine(currentRow);
            Console::setCursorPosition(currentRow++, boardOffsetX - 2);
            printf("%sAI: %s%s%s\n",
                Console::FG_MAGENTA.c_str(),
                Console::FG_WHITE.c_str(),
                aiThought.c_str(),
                Console::RESET.c_str()
            );
        }
        Console::setCursorPosition(currentRow + 1, 0);
        fflush(stdout);
    }

    Move getHumanInput(const std::vector<Move>& validMoves, PlayerColor humanPlayer, int blackScore, int whiteScore) {
        std::string input;
        int inputRow = infoPanelStartRow + 4;

        while (true) {
            Console::clearLine(inputRow);
            Console::setCursorPosition(inputRow, boardOffsetX - 2);
            printf("%sEnter your move (%c%c - %c%c) or 'pass': %s",
                Console::FG_GREEN.c_str(),
                'A', '1',
                (char)('A' + BOARD_SIZE - 1), (char)('1' + BOARD_SIZE - 1),
                Console::RESET.c_str());
            fflush(stdout);

            if (!(std::cin >> input)) {
                displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "Input stream error. Exiting.");
                return { -2,-2 };
            }

            if (input.length() == 0) {
                displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "Invalid input. Try again.");
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            std::transform(input.begin(), input.end(), input.begin(), ::tolower);

            if (input == "pass") {
                if (validMoves.empty()) return { -1,-1 };
                else { displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "You have valid moves, cannot pass."); continue; }
            }

            if (input.length() >= 2 && input.length() <= 3) {
                int col = input[0] - 'a';
                int row = -1;
                try {
                    row = std::stoi(input.substr(1)) - 1;
                }
                catch (const std::exception&) {
                    row = -1;
                }

                if (col >= 0 && col < BOARD_SIZE && row >= 0 && row < BOARD_SIZE) {
                    Move m = { row, col };
                    for (const auto& vm : validMoves) {
                        if (vm.row == m.row && vm.col == m.col) return m;
                    }
                    displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "Invalid move. Not among valid options.");
                }
                else {
                    displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "Invalid format or out of bounds.");
                }
            }
            else {
                displayInfo(humanPlayer, humanPlayer, blackScore, whiteScore, "Input format error. Use e.g., A1");
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
};

class GameManager {
public:
    Board board;
    PlayerColor currentPlayer;
    AIPlayer ai;
    UIManager ui;
    PlayerColor humanPlayer;
    Move lastMovePlayed = { -1, -1 };

    GameManager(PlayerColor humanAs, int aiSearchDepth = 4, int aiTimeLimitMs = 2000)
        : currentPlayer(PlayerColor::BLACK),
        ai(aiSearchDepth, aiTimeLimitMs),
        humanPlayer(humanAs) {
        Console::clearScreen();
        Console::hideCursor();
        Console::setCursorPosition(0, 0);
        printf("%sWelcome to Ultra-Advanced Reversi/Othello!%s\n", Console::FG_CYAN.c_str(), Console::RESET.c_str());
        printf("Human plays as: %s\n", (humanPlayer == PlayerColor::BLACK ? "Black" : "White"));
        Console::pause(2000);
        Console::clearScreen();
    }
    ~GameManager() {
        Console::showCursor();
        Console::setCursorPosition(ui.infoPanelStartRow + 9, 0); // Adjusted for more lines in game over
    }

    void startGame() {
        bool gameRunning = true;
        std::string message = (currentPlayer == humanPlayer) ? "Your turn." : "AI's turn.";
        std::string aiMessageAddendum = "";

        while (gameRunning) {
            auto scores = board.getScore();
            std::vector<Move> validMoves = board.getValidMoves(currentPlayer);

            ui.drawBoard(board, (currentPlayer == humanPlayer ? &validMoves : nullptr), &lastMovePlayed);
            ui.displayInfo(currentPlayer, humanPlayer, scores.first, scores.second, message, aiMessageAddendum);
            aiMessageAddendum = ""; // Clear AI addendum after displaying it once

            if (board.isGameOver()) {
                message = "Game Over!";
                gameRunning = false;
                continue;
            }

            if (validMoves.empty()) {
                message = std::string(currentPlayer == PlayerColor::BLACK ? "Black" : "White") + " has no moves. Turn passes.";
                lastMovePlayed = { -1,-1 };
                currentPlayer = getOpponent(currentPlayer);

                ui.displayInfo(currentPlayer, humanPlayer, scores.first, scores.second, message);
                Console::pause(1500);

                if (board.getValidMoves(currentPlayer).empty()) {
                    message = "No player has valid moves. Game Over!";
                    gameRunning = false;
                }
                continue;
            }

            if (currentPlayer == humanPlayer) {
                Move humanMove = ui.getHumanInput(validMoves, humanPlayer, scores.first, scores.second);
                if (humanMove.row == -2 && humanMove.col == -2) {
                    gameRunning = false;
                    message = "Input error, exiting game.";
                    continue;
                }
                std::vector<Move> flips = board.getFlipsForMove(humanMove.row, humanMove.col, currentPlayer);
                board.applyMove(humanMove, currentPlayer, flips);
                lastMovePlayed = humanMove;
                message = "You played " + std::string(1, (char)('A' + humanMove.col)) + std::to_string(humanMove.row + 1) +
                    ". Flipped " + std::to_string(flips.size()) + " pieces. AI is thinking...";
            }
            else {
                ui.displayInfo(currentPlayer, humanPlayer, scores.first, scores.second, message, "Calculating best move...");
                Console::pause(100);

                auto aiStartTime = std::chrono::high_resolution_clock::now();
                Move aiMove = ai.findBestMoveIterativeDeepening(board, currentPlayer);
                auto aiEndTime = std::chrono::high_resolution_clock::now();
                auto aiDuration = std::chrono::duration_cast<std::chrono::milliseconds>(aiEndTime - aiStartTime);

                if (aiMove.isValid()) {
                    std::vector<Move> flips = board.getFlipsForMove(aiMove.row, aiMove.col, currentPlayer);
                    board.applyMove(aiMove, currentPlayer, flips);
                    lastMovePlayed = aiMove;
                    message = "AI played " + std::string(1, (char)('A' + aiMove.col)) + std::to_string(aiMove.row + 1) +
                        ". Flipped " + std::to_string(flips.size()) + " pieces. Your turn.";
                    aiMessageAddendum = "(Took " + std::to_string(aiDuration.count()) + "ms)";
                }
                else {
                    message = "AI passes (no valid moves). Your turn.";
                    lastMovePlayed = { -1,-1 };
                }
            }
            currentPlayer = getOpponent(currentPlayer);
        }

        auto finalScores = board.getScore();
        ui.drawBoard(board, nullptr, &lastMovePlayed);
        ui.displayInfo(PlayerColor::NONE, humanPlayer, finalScores.first, finalScores.second, message);

        Console::setCursorPosition(ui.infoPanelStartRow + 5, 0);
        printf("%s------------------- GAME OVER -------------------%s\n", Console::FG_GREEN.c_str(), Console::RESET.c_str());
        Console::setCursorPosition(ui.infoPanelStartRow + 6, 0);
        printf("Final Score: %sBlack %d%s - %sWhite %d%s\n",
            Console::PIECE_BLACK_DISPLAY_COLOR.c_str(), finalScores.first, Console::RESET.c_str(),
            Console::PIECE_WHITE_DISPLAY_COLOR.c_str(), finalScores.second, Console::RESET.c_str());
        Console::setCursorPosition(ui.infoPanelStartRow + 7, 0);
        if (finalScores.first > finalScores.second) printf("%sBlack wins!%s\n", Console::PIECE_BLACK_DISPLAY_COLOR.c_str(), Console::RESET.c_str());
        else if (finalScores.second > finalScores.first) printf("%sWhite wins!%s\n", Console::PIECE_WHITE_DISPLAY_COLOR.c_str(), Console::RESET.c_str());
        else printf("It's a draw!\n");
        Console::setCursorPosition(ui.infoPanelStartRow + 8, 0);
        printf("%s-----------------------------------------------%s\n\n", Console::FG_GREEN.c_str(), Console::RESET.c_str());
        fflush(stdout);
    }
};

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    PlayerColor humanColor = PlayerColor::BLACK;
    int aiSearchDepth = 4;
    int aiTimeLimitMs = 1500;

    Console::clearScreen();
    Console::hideCursor();

    std::string choice;
    int choiceRow = 5, choiceCol = 5;

    while (true) {
        Console::setCursorPosition(choiceRow, choiceCol);
        printf("Do you want to play as Black (B) or White (W)? (B/W): ");
        fflush(stdout);
        if (!(std::cin >> choice)) {
            Console::showCursor(); return 1;
        }

        if (choice == "B" || choice == "b") {
            humanColor = PlayerColor::BLACK;
            break;
        }
        else if (choice == "W" || choice == "w") {
            humanColor = PlayerColor::WHITE;
            break;
        }
        else {
            Console::setCursorPosition(choiceRow + 1, choiceCol);
            printf("%sInvalid choice. Please enter B or W.%s", Console::FG_RED.c_str(), Console::RESET.c_str());
            fflush(stdout);
            Console::pause(1000);
            Console::clearLine(choiceRow + 1, choiceCol);
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    Console::clearLine(choiceRow + 1, choiceCol);

    while (true) {
        Console::setCursorPosition(choiceRow + 2, choiceCol);
        printf("Select AI difficulty (search depth, 1-7, higher is harder): ");
        fflush(stdout);
        int difficultyInput;
        if (!(std::cin >> difficultyInput)) {
            Console::showCursor(); return 1;
        }

        if (std::cin.good() && difficultyInput >= 1 && difficultyInput <= 7) {
            aiSearchDepth = difficultyInput;
            break;
        }
        else {
            Console::setCursorPosition(choiceRow + 3, choiceCol);
            printf("%sInvalid input. Please enter a number between 1 and 7.%s", Console::FG_RED.c_str(), Console::RESET.c_str());
            fflush(stdout);
            Console::pause(1000);
            Console::clearLine(choiceRow + 3, choiceCol);
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    Console::clearScreen();
    GameManager game(humanColor, aiSearchDepth, aiTimeLimitMs);
    game.startGame();

    Console::showCursor();
    return 0;
}