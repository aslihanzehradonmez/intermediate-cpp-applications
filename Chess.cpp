#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <random>
#include <ctime>
#include <limits>
#include <fstream>
#include <sstream>

// Forward Declarations
class Piece;
class Board;
class Game; // Forward declaration for use in AIPlayer and Player
class Player; // Forward declaration for Game class members
class AIPlayer; // Forward declaration for Game class members

enum class PieceColor { WHITE, BLACK, NONE };
enum class PieceType { PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, EMPTY };
enum class AIDifficulty { EASY, MEDIUM, HARD };

struct Position {
    int row = -1; // Default initialize
    int col = -1; // Default initialize

    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

struct Move {
    Position from = { -1,-1 }; // Default initialize
    Position to = { -1,-1 };   // Default initialize
    PieceType promotionPiece = PieceType::EMPTY;
    bool isEnPassantCapture = false;
    Position enPassantVictimPos = { -1, -1 };
    bool isCastlingMove = false;


    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotionPiece == other.promotionPiece;
    }
};

struct HighScoreEntry {
    std::string playerName; // Default constructor handles string
    int wins = 0;          // Default initialize
    AIDifficulty difficulty = AIDifficulty::MEDIUM; // Default initialize

    bool operator<(const HighScoreEntry& other) const {
        if (wins != other.wins) {
            return wins > other.wins;
        }
        if (difficulty != other.difficulty) {
            return static_cast<int>(difficulty) > static_cast<int>(other.difficulty);
        }
        return playerName < other.playerName;
    }
};

class Piece {
public:
    PieceColor pieceColor;
    PieceType pieceType;
    Position currentPosition;
    bool hasMoved;

    Piece(PieceColor color, PieceType type, Position pos)
        : pieceColor(color), pieceType(type), currentPosition(pos), hasMoved(false) {
    }
    virtual ~Piece() = default;

    PieceColor getColor() const { return pieceColor; }
    PieceType getType() const { return pieceType; }
    Position getPosition() const { return currentPosition; }
    void setPosition(Position pos) { currentPosition = pos; }
    bool getHasMoved() const { return hasMoved; }
    void setHasMoved(bool moved) { hasMoved = moved; }

    virtual std::vector<Move> getValidMoves(const Board& board) const = 0;
    virtual char getSymbol() const = 0;
    virtual std::shared_ptr<Piece> clone() const = 0;
    virtual int getValue() const = 0;


protected:
    void addMoveIfValid(std::vector<Move>& moves, Move currentMove, const Board& board, bool isCaptureOnly = false) const;
    void addStraightLineMoves(std::vector<Move>& moves, const Board& board, const std::vector<std::pair<int, int>>& directions) const;
};

class Board {
public:
    std::array<std::array<std::shared_ptr<Piece>, 8>, 8> grid;
    Move lastMove;
    Position enPassantTargetSquare;
    bool whiteKingSideCastlePossible = true;
    bool whiteQueenSideCastlePossible = true;
    bool blackKingSideCastlePossible = true;
    bool blackQueenSideCastlePossible = true;
    int halfMoveClock = 0;


    Board() : enPassantTargetSquare({ -1, -1 }) {
        initializeEmptyBoard();
        setupInitialPieces();
        lastMove = { {-1,-1},{-1,-1} }; // Initialize lastMove
    }

    Board(const Board& other) {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (other.grid[r][c]) {
                    this->grid[r][c] = other.grid[r][c]->clone();
                }
                else {
                    this->grid[r][c] = nullptr;
                }
            }
        }
        this->lastMove = other.lastMove;
        this->enPassantTargetSquare = other.enPassantTargetSquare;
        this->whiteKingSideCastlePossible = other.whiteKingSideCastlePossible;
        this->whiteQueenSideCastlePossible = other.whiteQueenSideCastlePossible;
        this->blackKingSideCastlePossible = other.blackKingSideCastlePossible;
        this->blackQueenSideCastlePossible = other.blackQueenSideCastlePossible;
        this->halfMoveClock = other.halfMoveClock;
    }


    void initializeEmptyBoard() {
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                grid[i][j] = nullptr;
            }
        }
        enPassantTargetSquare = { -1, -1 };
        lastMove = { {-1,-1},{-1,-1} }; // Ensure lastMove is reset
    }

    void setupInitialPieces();

    void displayBoard(PieceColor humanPlayerColorPerspective) const {
        std::cout << "\n    a   b   c   d   e   f   g   h" << std::endl;
        std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
        for (int r_disp = 0; r_disp < 8; ++r_disp) {
            int r_actual = (humanPlayerColorPerspective == PieceColor::WHITE || humanPlayerColorPerspective == PieceColor::NONE) ? r_disp : (7 - r_disp);
            std::cout << ((humanPlayerColorPerspective == PieceColor::WHITE || humanPlayerColorPerspective == PieceColor::NONE) ? (8 - r_actual) : (r_actual + 1)) << " |";
            for (int c_disp = 0; c_disp < 8; ++c_disp) {
                int c_actual = (humanPlayerColorPerspective == PieceColor::WHITE || humanPlayerColorPerspective == PieceColor::NONE) ? c_disp : (7 - c_disp);
                char pieceSymbol = ' ';
                if (grid[r_actual][c_actual]) {
                    pieceSymbol = grid[r_actual][c_actual]->getSymbol();
                }
                else {
                    pieceSymbol = ((r_actual + c_actual) % 2 == 0) ? ' ' : '.';
                }
                std::cout << " " << pieceSymbol << " |";
            }
            std::cout << " " << ((humanPlayerColorPerspective == PieceColor::WHITE || humanPlayerColorPerspective == PieceColor::NONE) ? (8 - r_actual) : (r_actual + 1)) << std::endl;
            std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
        }
        std::cout << "    a   b   c   d   e   f   g   h\n" << std::endl;
    }

    std::shared_ptr<Piece> getPieceAt(Position pos) const {
        if (!pos.isValid()) return nullptr;
        return grid[pos.row][pos.col];
    }

    void setPieceAt(Position pos, std::shared_ptr<Piece> piece) {
        if (!pos.isValid()) return;
        grid[pos.row][pos.col] = piece;
        if (piece) {
            piece->setPosition(pos);
        }
    }

    bool makeMove(Move& move) {
        std::shared_ptr<Piece> pieceToMove = getPieceAt(move.from);
        if (!pieceToMove) return false;

        bool isPawnMove = (pieceToMove->getType() == PieceType::PAWN);
        bool isCapture = (getPieceAt(move.to) != nullptr) || move.isEnPassantCapture;

        if (isPawnMove || isCapture) {
            halfMoveClock = 0;
        }
        else {
            halfMoveClock++;
        }

        Position oldEnPassantTarget = enPassantTargetSquare;
        enPassantTargetSquare = { -1, -1 };

        if (isPawnMove && std::abs(move.from.row - move.to.row) == 2) {
            enPassantTargetSquare = { (move.from.row + move.to.row) / 2, move.from.col };
        }

        if (move.isEnPassantCapture) {
            if (move.to == oldEnPassantTarget && oldEnPassantTarget.isValid()) {
                setPieceAt(move.enPassantVictimPos, nullptr);
            }
            else {
                return false;
            }
        }

        if (pieceToMove->getType() == PieceType::KING && std::abs(move.to.col - move.from.col) == 2) {
            move.isCastlingMove = true;
            Position rookFrom, rookTo;
            if (move.to.col > move.from.col) {
                rookFrom = { move.from.row, 7 };
                rookTo = { move.from.row, 5 };
            }
            else {
                rookFrom = { move.from.row, 0 };
                rookTo = { move.from.row, 3 };
            }
            std::shared_ptr<Piece> rook = getPieceAt(rookFrom);
            if (rook && rook->getType() == PieceType::ROOK) {
                setPieceAt(rookTo, rook);
                setPieceAt(rookFrom, nullptr);
                if (rook) rook->setHasMoved(true);
            }
            else { return false; }
        }

        setPieceAt(move.to, pieceToMove);
        setPieceAt(move.from, nullptr);
        pieceToMove->setHasMoved(true);


        if (pieceToMove->getType() == PieceType::KING) {
            if (pieceToMove->getColor() == PieceColor::WHITE) {
                whiteKingSideCastlePossible = false;
                whiteQueenSideCastlePossible = false;
            }
            else {
                blackKingSideCastlePossible = false;
                blackQueenSideCastlePossible = false;
            }
        }
        if (pieceToMove->getType() == PieceType::ROOK) {
            if (move.from.row == 7 && move.from.col == 0 && pieceToMove->getColor() == PieceColor::WHITE) whiteQueenSideCastlePossible = false;
            if (move.from.row == 7 && move.from.col == 7 && pieceToMove->getColor() == PieceColor::WHITE) whiteKingSideCastlePossible = false;
            if (move.from.row == 0 && move.from.col == 0 && pieceToMove->getColor() == PieceColor::BLACK) blackQueenSideCastlePossible = false;
            if (move.from.row == 0 && move.from.col == 7 && pieceToMove->getColor() == PieceColor::BLACK) blackKingSideCastlePossible = false;
        }

        if (pieceToMove->getType() == PieceType::PAWN) {
            if ((pieceToMove->getColor() == PieceColor::WHITE && move.to.row == 0) ||
                (pieceToMove->getColor() == PieceColor::BLACK && move.to.row == 7)) {
                if (move.promotionPiece == PieceType::EMPTY) {
                    move.promotionPiece = PieceType::QUEEN;
                }
            }
        }


        lastMove = move;
        return true;
    }


    Position findKing(PieceColor kingColor) const {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                std::shared_ptr<Piece> p = getPieceAt({ r, c });
                if (p && p->getType() == PieceType::KING && p->getColor() == kingColor) {
                    return { r, c };
                }
            }
        }
        return { -1, -1 };
    }

    bool isSquareAttacked(Position square, PieceColor attackerColor, const Board& currentBoard) const {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                std::shared_ptr<Piece> p = currentBoard.getPieceAt({ r,c });
                if (p && p->getColor() == attackerColor) {
                    std::vector<Move> pieceMoves = p->getValidMoves(currentBoard);
                    for (const auto& m : pieceMoves) {
                        if (m.to == square) {
                            if (p->getType() == PieceType::PAWN) {
                                if (m.from.col != m.to.col) return true;
                            }
                            else {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    int evaluateMaterial(PieceColor perspectiveColor) const {
        int score = 0;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                std::shared_ptr<Piece> p = getPieceAt({ r,c });
                if (p) {
                    int value = p->getValue();
                    if (p->getType() == PieceType::KING) value = 0;

                    if (p->getColor() == perspectiveColor) {
                        score += value;
                    }
                    else {
                        score -= value;
                    }
                }
            }
        }
        return score;
    }

    std::vector<Move> generateAllPseudoLegalMoves(PieceColor color) const {
        std::vector<Move> allMoves;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                std::shared_ptr<Piece> piece = getPieceAt({ r,c });
                if (piece && piece->getColor() == color) {
                    std::vector<Move> pieceMoves = piece->getValidMoves(*this);
                    allMoves.insert(allMoves.end(), pieceMoves.begin(), pieceMoves.end());
                }
            }
        }
        return allMoves;
    }
};


void Piece::addMoveIfValid(std::vector<Move>& moves, Move currentMove, const Board& board, bool isCaptureOnly) const {
    if (!currentMove.to.isValid()) return;
    std::shared_ptr<Piece> targetPiece = board.getPieceAt(currentMove.to);

    if (isCaptureOnly) {
        if (targetPiece && targetPiece->getColor() != this->pieceColor) {
            moves.push_back(currentMove);
        }
        return;
    }

    if (!targetPiece || targetPiece->getColor() != this->pieceColor) {
        if (this->pieceType == PieceType::PAWN) {
            if ((this->pieceColor == PieceColor::WHITE && currentMove.to.row == 0) ||
                (this->pieceColor == PieceColor::BLACK && currentMove.to.row == 7)) {
                moves.push_back({ currentMove.from, currentMove.to, PieceType::QUEEN, currentMove.isEnPassantCapture, currentMove.enPassantVictimPos, currentMove.isCastlingMove });
                moves.push_back({ currentMove.from, currentMove.to, PieceType::ROOK, currentMove.isEnPassantCapture, currentMove.enPassantVictimPos, currentMove.isCastlingMove });
                moves.push_back({ currentMove.from, currentMove.to, PieceType::BISHOP, currentMove.isEnPassantCapture, currentMove.enPassantVictimPos, currentMove.isCastlingMove });
                moves.push_back({ currentMove.from, currentMove.to, PieceType::KNIGHT, currentMove.isEnPassantCapture, currentMove.enPassantVictimPos, currentMove.isCastlingMove });
                return;
            }
        }
        moves.push_back(currentMove);
    }
}

void Piece::addStraightLineMoves(std::vector<Move>& moves, const Board& board, const std::vector<std::pair<int, int>>& directions) const {
    for (auto dir : directions) {
        for (int i = 1; i < 8; ++i) {
            Position nextPos = { currentPosition.row + dir.first * i, currentPosition.col + dir.second * i };
            if (!nextPos.isValid()) break;
            std::shared_ptr<Piece> targetPiece = board.getPieceAt(nextPos);
            Move currentMove = { {currentPosition}, nextPos };
            if (!targetPiece) {
                addMoveIfValid(moves, currentMove, board);
            }
            else {
                if (targetPiece->getColor() != this->pieceColor) {
                    addMoveIfValid(moves, currentMove, board);
                }
                break;
            }
        }
    }
}


class Pawn : public Piece {
public:
    Pawn(PieceColor color, Position pos) : Piece(color, PieceType::PAWN, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<Pawn>(*this); }
    int getValue() const override { return 100; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'P' : 'p'; }

    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        int direction = (pieceColor == PieceColor::WHITE) ? -1 : 1;

        Position forwardOnePos = { currentPosition.row + direction, currentPosition.col };
        Move forwardOneMove = { {currentPosition}, forwardOnePos };
        if (forwardOnePos.isValid() && !board.getPieceAt(forwardOnePos)) {
            addMoveIfValid(moves, forwardOneMove, board);
            if (!hasMoved) {
                Position forwardTwoPos = { currentPosition.row + 2 * direction, currentPosition.col };
                Move forwardTwoMove = { {currentPosition}, forwardTwoPos };
                if (forwardTwoPos.isValid() && !board.getPieceAt(forwardTwoPos)) {
                    addMoveIfValid(moves, forwardTwoMove, board);
                }
            }
        }

        Position captureOffsets[] = { {direction, -1}, {direction, 1} };
        for (const auto& offset : captureOffsets) {
            Position capPos = { currentPosition.row + offset.row, currentPosition.col + offset.col };
            if (capPos.isValid()) {
                std::shared_ptr<Piece> target = board.getPieceAt(capPos);
                Move capMove = { {currentPosition}, capPos };
                if (target && target->getColor() != pieceColor) {
                    addMoveIfValid(moves, capMove, board, true);
                }
                if (capPos == board.enPassantTargetSquare && board.enPassantTargetSquare.isValid()) {
                    capMove.isEnPassantCapture = true;
                    capMove.enPassantVictimPos = { currentPosition.row, capPos.col };
                    moves.push_back(capMove);
                }
            }
        }
        return moves;
    }
};

class Rook : public Piece {
public:
    Rook(PieceColor color, Position pos) : Piece(color, PieceType::ROOK, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<Rook>(*this); }
    int getValue() const override { return 500; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'R' : 'r'; }
    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        addStraightLineMoves(moves, board, { {0, 1}, {0, -1}, {1, 0}, {-1, 0} });
        return moves;
    }
};

class Knight : public Piece {
public:
    Knight(PieceColor color, Position pos) : Piece(color, PieceType::KNIGHT, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<Knight>(*this); }
    int getValue() const override { return 320; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'N' : 'n'; }
    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        int dr[] = { -2, -2, -1, -1, 1, 1, 2, 2 };
        int dc[] = { -1, 1, -2, 2, -2, 2, -1, 1 };
        for (int i = 0; i < 8; ++i) {
            addMoveIfValid(moves, { {currentPosition}, {currentPosition.row + dr[i], currentPosition.col + dc[i]} }, board);
        }
        return moves;
    }
};

class Bishop : public Piece {
public:
    Bishop(PieceColor color, Position pos) : Piece(color, PieceType::BISHOP, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<Bishop>(*this); }
    int getValue() const override { return 330; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'B' : 'b'; }
    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        addStraightLineMoves(moves, board, { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} });
        return moves;
    }
};

class Queen : public Piece {
public:
    Queen(PieceColor color, Position pos) : Piece(color, PieceType::QUEEN, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<Queen>(*this); }
    int getValue() const override { return 900; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'Q' : 'q'; }
    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        addStraightLineMoves(moves, board, { {0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} });
        return moves;
    }
};

class King : public Piece {
public:
    King(PieceColor color, Position pos) : Piece(color, PieceType::KING, pos) {}
    std::shared_ptr<Piece> clone() const override { return std::make_shared<King>(*this); }
    int getValue() const override { return 20000; }
    char getSymbol() const override { return (pieceColor == PieceColor::WHITE) ? 'K' : 'k'; }
    std::vector<Move> getValidMoves(const Board& board) const override {
        std::vector<Move> moves;
        int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        for (int i = 0; i < 8; ++i) {
            addMoveIfValid(moves, { {currentPosition}, {currentPosition.row + dr[i], currentPosition.col + dc[i]} }, board);
        }

        if (!hasMoved) {
            PieceColor color = getColor();
            PieceColor opponentColor = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
            if ((color == PieceColor::WHITE && board.whiteKingSideCastlePossible) || (color == PieceColor::BLACK && board.blackKingSideCastlePossible)) {
                if (!board.getPieceAt({ currentPosition.row, currentPosition.col + 1 }) &&
                    !board.getPieceAt({ currentPosition.row, currentPosition.col + 2 })) {
                    std::shared_ptr<Piece> rook = board.getPieceAt({ currentPosition.row, currentPosition.col + 3 });
                    if (rook && rook->getType() == PieceType::ROOK && !rook->getHasMoved() && rook->getColor() == color) {
                        if (!board.isSquareAttacked({ currentPosition.row, currentPosition.col }, opponentColor, board) &&
                            !board.isSquareAttacked({ currentPosition.row, currentPosition.col + 1 }, opponentColor, board) &&
                            !board.isSquareAttacked({ currentPosition.row, currentPosition.col + 2 }, opponentColor, board)) {
                            Move castleMove = { {currentPosition}, {currentPosition.row, currentPosition.col + 2} };
                            castleMove.isCastlingMove = true;
                            moves.push_back(castleMove);
                        }
                    }
                }
            }
            if ((color == PieceColor::WHITE && board.whiteQueenSideCastlePossible) || (color == PieceColor::BLACK && board.blackQueenSideCastlePossible)) {
                if (!board.getPieceAt({ currentPosition.row, currentPosition.col - 1 }) &&
                    !board.getPieceAt({ currentPosition.row, currentPosition.col - 2 }) &&
                    !board.getPieceAt({ currentPosition.row, currentPosition.col - 3 })) {
                    std::shared_ptr<Piece> rook = board.getPieceAt({ currentPosition.row, currentPosition.col - 4 });
                    if (rook && rook->getType() == PieceType::ROOK && !rook->getHasMoved() && rook->getColor() == color) {
                        if (!board.isSquareAttacked({ currentPosition.row, currentPosition.col }, opponentColor, board) &&
                            !board.isSquareAttacked({ currentPosition.row, currentPosition.col - 1 }, opponentColor, board) &&
                            !board.isSquareAttacked({ currentPosition.row, currentPosition.col - 2 }, opponentColor, board)) {
                            Move castleMove = { {currentPosition}, {currentPosition.row, currentPosition.col - 2} };
                            castleMove.isCastlingMove = true;
                            moves.push_back(castleMove);
                        }
                    }
                }
            }
        }
        return moves;
    }
};

void Board::setupInitialPieces() {
    for (int i = 0; i < 8; ++i) {
        grid[1][i] = std::make_shared<Pawn>(PieceColor::BLACK, Position{ 1, i });
        grid[6][i] = std::make_shared<Pawn>(PieceColor::WHITE, Position{ 6, i });
    }

    grid[0][0] = std::make_shared<Rook>(PieceColor::BLACK, Position{ 0, 0 });
    grid[0][7] = std::make_shared<Rook>(PieceColor::BLACK, Position{ 0, 7 });
    grid[7][0] = std::make_shared<Rook>(PieceColor::WHITE, Position{ 7, 0 });
    grid[7][7] = std::make_shared<Rook>(PieceColor::WHITE, Position{ 7, 7 });

    grid[0][1] = std::make_shared<Knight>(PieceColor::BLACK, Position{ 0, 1 });
    grid[0][6] = std::make_shared<Knight>(PieceColor::BLACK, Position{ 0, 6 });
    grid[7][1] = std::make_shared<Knight>(PieceColor::WHITE, Position{ 7, 1 });
    grid[7][6] = std::make_shared<Knight>(PieceColor::WHITE, Position{ 7, 6 });

    grid[0][2] = std::make_shared<Bishop>(PieceColor::BLACK, Position{ 0, 2 });
    grid[0][5] = std::make_shared<Bishop>(PieceColor::BLACK, Position{ 0, 5 });
    grid[7][2] = std::make_shared<Bishop>(PieceColor::WHITE, Position{ 7, 2 });
    grid[7][5] = std::make_shared<Bishop>(PieceColor::WHITE, Position{ 7, 5 });

    grid[0][3] = std::make_shared<Queen>(PieceColor::BLACK, Position{ 0, 3 });
    grid[7][3] = std::make_shared<Queen>(PieceColor::WHITE, Position{ 7, 3 });
    grid[0][4] = std::make_shared<King>(PieceColor::BLACK, Position{ 0, 4 });
    grid[7][4] = std::make_shared<King>(PieceColor::WHITE, Position{ 7, 4 });
    halfMoveClock = 0;
}


class Player {
public:
    PieceColor playerColor;
    Player(PieceColor color) : playerColor(color) {}
    virtual ~Player() = default;
    virtual Move getMove(const Board& board, Game* gameInstance) const = 0;
    PieceColor getColor() const { return playerColor; }
};

// AIPlayer class declaration (methods to be defined after Game)
class AIPlayer : public Player {
private:
    AIDifficulty difficulty;
    static bool rng_seeded;
    Game* game_ptr;

    int minimax(Board currentBoard, int depth, bool isMaximizingPlayer, PieceColor aiPlayerColor) const;

public:
    AIPlayer(PieceColor color, AIDifficulty diff);
    void setGamePtr(Game* gp);
    Move getMove(const Board& board, Game* gameInstance) const override;
};
bool AIPlayer::rng_seeded = false; // Static member initialization


class HumanPlayer : public Player {
public:
    HumanPlayer(PieceColor color) : Player(color) {}

    Position parsePosition(const std::string& s) const {
        if (s.length() != 2) return { -1, -1 };
        char fileChar = std::tolower(s[0]);
        char rankChar = s[1];

        int col = fileChar - 'a';
        int row_input = rankChar - '1';
        int row = 7 - row_input;

        if (col < 0 || col > 7 || row_input < 0 || row_input > 7) return { -1, -1 };
        return { row, col };
    }

    std::string formatMoveInput(std::string& line) const {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        return line;
    }


    Move getMove(const Board& board, Game* gameInstance) const override; // Definition after Game
};


enum class GameStatus { ONGOING, WHITE_WINS, BLACK_WINS, DRAW_STALEMATE, DRAW_NOMOVES, DRAW_50MOVES, DRAW_3FOLD };

// Game class FULL definition
class Game {
public:
    Board board;
    std::unique_ptr<Player> player1;
    std::unique_ptr<Player> player2;
    PieceColor currentPlayerTurn;
    GameStatus status;
    int fullMoveCounter;
    PieceColor humanPlayerColor = PieceColor::NONE;
    AIDifficulty aiDifficulty = AIDifficulty::MEDIUM;
    std::string humanPlayerName = "Player";


    Game() : currentPlayerTurn(PieceColor::WHITE), status(GameStatus::ONGOING), fullMoveCounter(1) {
    }

    void setupGame() {
        char choice_char;
        std::cout << "Welcome to Chess!" << std::endl;
        std::cout << "Would you like to play as White (w) or Black (b)? (w/b): ";
        std::cin >> choice_char;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        humanPlayerColor = (std::tolower(choice_char) == 'b') ? PieceColor::BLACK : PieceColor::WHITE;

        std::cout << "Select AI difficulty (1: Easy, 2: Medium, 3: Hard): ";
        int diff_choice;
        std::cin >> diff_choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (diff_choice == 1) aiDifficulty = AIDifficulty::EASY;
        else if (diff_choice == 3) aiDifficulty = AIDifficulty::HARD;
        else aiDifficulty = AIDifficulty::MEDIUM;

        std::cout << "Please enter your name: ";
        std::getline(std::cin, humanPlayerName);
        if (humanPlayerName.empty()) humanPlayerName = "Player";


        if (humanPlayerColor == PieceColor::WHITE) {
            player1 = std::make_unique<HumanPlayer>(PieceColor::WHITE);
            player2 = std::make_unique<AIPlayer>(PieceColor::BLACK, aiDifficulty);
        }
        else {
            player1 = std::make_unique<AIPlayer>(PieceColor::WHITE, aiDifficulty);
            player2 = std::make_unique<HumanPlayer>(PieceColor::BLACK);
        }

        // Set game_ptr for AIPlayer instances
        AIPlayer* ai1 = dynamic_cast<AIPlayer*>(player1.get());
        if (ai1) ai1->setGamePtr(this);
        AIPlayer* ai2 = dynamic_cast<AIPlayer*>(player2.get());
        if (ai2) ai2->setGamePtr(this);


        board.initializeEmptyBoard();
        board.setupInitialPieces();
    }


    bool isKingInCheck(PieceColor kingColor, const Board& currentBoard) const {
        Position kingPos = currentBoard.findKing(kingColor);
        if (!kingPos.isValid()) return false;
        PieceColor attackerColor = (kingColor == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
        return board.isSquareAttacked(kingPos, attackerColor, currentBoard);
    }

    std::vector<Move> generateLegalMoves(PieceColor color, const Board& currentBoard) const {
        std::vector<Move> legalMoves;
        std::vector<Move> pseudoLegalMoves = currentBoard.generateAllPseudoLegalMoves(color);

        for (const auto& move : pseudoLegalMoves) {
            Board tempBoard = currentBoard;
            Move tempMove = move;
            tempBoard.makeMove(tempMove);
            // For promotions during simulation, AI will pick Queen by default if move.promotionPiece is EMPTY
            // This happens if addMoveIfValid adds multiple promotion options. We need to test one.
            if (tempMove.promotionPiece == PieceType::EMPTY) { // If pawn reached end and no specific promotion chosen yet
                std::shared_ptr<Piece> p = tempBoard.getPieceAt(tempMove.to);
                if (p && p->getType() == PieceType::PAWN) {
                    if ((p->getColor() == PieceColor::WHITE && tempMove.to.row == 0) ||
                        (p->getColor() == PieceColor::BLACK && tempMove.to.row == 7)) {
                        tempMove.promotionPiece = PieceType::QUEEN; // Default to queen for check test
                    }
                }
            }
            applyPromotionIfAny(tempMove, tempBoard);
            if (!isKingInCheck(color, tempBoard)) {
                legalMoves.push_back(move);
            }
        }
        return legalMoves;
    }

    void applyPromotionIfAny(Move& move, Board& targetBoard) const {
        std::shared_ptr<Piece> pieceAtDest = targetBoard.getPieceAt(move.to);
        if (pieceAtDest && pieceAtDest->getType() == PieceType::PAWN && move.promotionPiece != PieceType::EMPTY) {
            if ((pieceAtDest->getColor() == PieceColor::WHITE && move.to.row == 0) ||
                (pieceAtDest->getColor() == PieceColor::BLACK && move.to.row == 7)) {

                PieceColor color = pieceAtDest->getColor();
                Position pos = pieceAtDest->getPosition();
                switch (move.promotionPiece) {
                case PieceType::QUEEN: targetBoard.setPieceAt(pos, std::make_shared<Queen>(color, pos)); break;
                case PieceType::ROOK: targetBoard.setPieceAt(pos, std::make_shared<Rook>(color, pos)); break;
                case PieceType::BISHOP: targetBoard.setPieceAt(pos, std::make_shared<Bishop>(color, pos)); break;
                case PieceType::KNIGHT: targetBoard.setPieceAt(pos, std::make_shared<Knight>(color, pos)); break;
                default: targetBoard.setPieceAt(pos, std::make_shared<Queen>(color, pos)); break;
                }
            }
        }
    }


    void playTurn() {
        if (currentPlayerTurn == PieceColor::WHITE) {
            std::cout << "Turn " << fullMoveCounter << " - ";
        }
        std::cout << (currentPlayerTurn == PieceColor::WHITE ? "White" : "Black") << "'s turn." << std::endl;

        std::vector<Move> legalMoves = generateLegalMoves(currentPlayerTurn, board);

        if (legalMoves.empty()) {
            if (isKingInCheck(currentPlayerTurn, board)) {
                status = (currentPlayerTurn == PieceColor::WHITE) ? GameStatus::BLACK_WINS : GameStatus::WHITE_WINS;
                std::cout << "Checkmate! ";
            }
            else {
                status = GameStatus::DRAW_STALEMATE;
                std::cout << "Stalemate! ";
            }
            return;
        }

        if (board.halfMoveClock >= 100) {
            status = GameStatus::DRAW_50MOVES;
            std::cout << "Draw by 50-move rule! ";
            return;
        }

        Move chosenMove;
        if (currentPlayerTurn == PieceColor::WHITE) {
            chosenMove = player1->getMove(board, this);
        }
        else {
            chosenMove = player2->getMove(board, this);
        }

        std::shared_ptr<Piece> pieceBeingMoved = board.getPieceAt(chosenMove.from);
        char pieceChar = pieceBeingMoved ? pieceBeingMoved->getSymbol() : '?';

        std::string fromCoord = "";
        fromCoord += (char)('a' + chosenMove.from.col);
        fromCoord += std::to_string(8 - chosenMove.from.row);

        std::string toCoord = "";
        toCoord += (char)('a' + chosenMove.to.col);
        toCoord += std::to_string(8 - chosenMove.to.row);


        std::cout << (currentPlayerTurn == PieceColor::WHITE ? "White" : "Black")
            << " played: " << pieceChar
            << " (" << fromCoord
            << ") to (" << toCoord << ")";

        std::shared_ptr<Piece> capturedPieceOriginal = board.getPieceAt(chosenMove.to);
        if (capturedPieceOriginal && !chosenMove.isCastlingMove) {
            std::cout << " capturing " << capturedPieceOriginal->getSymbol();
        }
        else if (chosenMove.isEnPassantCapture) {
            std::cout << " capturing en passant";
        }
        else if (chosenMove.isCastlingMove) {
            std::cout << " castles";
        }
        if (chosenMove.promotionPiece != PieceType::EMPTY && pieceBeingMoved && pieceBeingMoved->getType() == PieceType::PAWN) {
            std::cout << " promoting to ";
            switch (chosenMove.promotionPiece) {
            case PieceType::QUEEN: std::cout << "Queen"; break;
            case PieceType::ROOK: std::cout << "Rook"; break;
            case PieceType::BISHOP: std::cout << "Bishop"; break;
            case PieceType::KNIGHT: std::cout << "Knight"; break;
            default: break;
            }
        }
        std::cout << std::endl;


        bool moveSuccess = board.makeMove(chosenMove);
        if (!moveSuccess) {
            std::cerr << "Error: Invalid move was somehow selected (this is a bug)." << std::endl;
            status = (currentPlayerTurn == PieceColor::WHITE) ? GameStatus::BLACK_WINS : GameStatus::WHITE_WINS;
            return;
        }

        applyPromotionIfAny(chosenMove, board);


        if (currentPlayerTurn == PieceColor::BLACK) {
            fullMoveCounter++;
        }
        currentPlayerTurn = (currentPlayerTurn == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;


        std::vector<Move> nextPlayerLegalMoves = generateLegalMoves(currentPlayerTurn, board);
        if (nextPlayerLegalMoves.empty()) {
            if (isKingInCheck(currentPlayerTurn, board)) {
                status = (currentPlayerTurn == PieceColor::WHITE) ? GameStatus::BLACK_WINS : GameStatus::WHITE_WINS; // Previous player wins
                std::cout << "Checkmate! ";
            }
            else {
                status = GameStatus::DRAW_STALEMATE;
                std::cout << "Stalemate! ";
            }
        }
        if (fullMoveCounter > 200 && status == GameStatus::ONGOING) {
            std::cout << "Max turns reached. Game drawn." << std::endl;
            status = GameStatus::DRAW_NOMOVES;
        }
    }

    void start() {
        setupGame();
        int displayTurnLimit = 0;
        while (status == GameStatus::ONGOING && displayTurnLimit < 400) { // Limit game length
            board.displayBoard(humanPlayerColor);
            playTurn();
            displayTurnLimit++;
        }
        board.displayBoard(humanPlayerColor);

        std::cout << "\n--- Game Over ---" << std::endl;
        bool humanWon = false;
        switch (status) {
        case GameStatus::WHITE_WINS:
            std::cout << "White wins!" << std::endl;
            if (humanPlayerColor == PieceColor::WHITE) humanWon = true;
            break;
        case GameStatus::BLACK_WINS:
            std::cout << "Black wins!" << std::endl;
            if (humanPlayerColor == PieceColor::BLACK) humanWon = true;
            break;
        case GameStatus::DRAW_STALEMATE: std::cout << "Draw by Stalemate!" << std::endl; break;
        case GameStatus::DRAW_50MOVES: std::cout << "Draw by 50-move rule!" << std::endl; break;
        case GameStatus::DRAW_NOMOVES: std::cout << "Draw by move limit!" << std::endl; break;
        default: std::cout << "Game ended." << std::endl; break;
        }
        if (humanWon) {
            updateHighScore(humanPlayerName, 1, aiDifficulty);
        }
        displayHighScores();
    }

    std::vector<HighScoreEntry> loadHighScores() {
        std::vector<HighScoreEntry> scores;
        std::ifstream file("highscores.txt");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string name, wins_str, diff_str;
                std::getline(ss, name, ',');
                std::getline(ss, wins_str, ',');
                std::getline(ss, diff_str, ',');
                if (!name.empty() && !wins_str.empty() && !diff_str.empty()) {
                    try {
                        AIDifficulty diff = AIDifficulty::MEDIUM;
                        int diff_val = std::stoi(diff_str);
                        if (diff_val == static_cast<int>(AIDifficulty::EASY)) diff = AIDifficulty::EASY;
                        else if (diff_val == static_cast<int>(AIDifficulty::HARD)) diff = AIDifficulty::HARD;

                        scores.push_back({ name, std::stoi(wins_str), diff });
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error reading high score line: " << line << " (" << e.what() << ")" << std::endl;
                    }
                }
            }
            file.close();
        }
        std::sort(scores.begin(), scores.end());
        return scores;
    }

    void saveHighScores(const std::vector<HighScoreEntry>& scores) {
        std::ofstream file("highscores.txt");
        if (file.is_open()) {
            for (const auto& entry : scores) {
                file << entry.playerName << "," << entry.wins << "," << static_cast<int>(entry.difficulty) << std::endl;
            }
            file.close();
        }
    }

    void updateHighScore(const std::string& playerName, int winIncrement, AIDifficulty difficulty) {
        std::vector<HighScoreEntry> scores = loadHighScores();
        bool foundPlayer = false;
        for (auto& entry : scores) {
            if (entry.playerName == playerName && entry.difficulty == difficulty) {
                entry.wins += winIncrement;
                foundPlayer = true;
                break;
            }
        }
        if (!foundPlayer) {
            scores.push_back({ playerName, winIncrement, difficulty });
        }
        std::sort(scores.begin(), scores.end());
        if (scores.size() > 10) {
            scores.resize(10);
        }
        saveHighScores(scores);
    }

    void displayHighScores() {
        std::vector<HighScoreEntry> scores = loadHighScores();
        std::cout << "\n--- High Scores ---" << std::endl;
        if (scores.empty()) {
            std::cout << "No high scores recorded yet." << std::endl;
        }
        else {
            int rank = 1;
            for (const auto& entry : scores) {
                std::string diff_str = "Medium";
                if (entry.difficulty == AIDifficulty::EASY) diff_str = "Easy";
                else if (entry.difficulty == AIDifficulty::HARD) diff_str = "Hard";
                std::cout << rank++ << ". " << entry.playerName << " - " << entry.wins << " wins ("
                    << diff_str << ")" << std::endl;
            }
        }
    }

};

// AIPlayer method definitions
AIPlayer::AIPlayer(PieceColor color, AIDifficulty diff) : Player(color), difficulty(diff), game_ptr(nullptr) {
    if (!rng_seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        rng_seeded = true;
    }
}

void AIPlayer::setGamePtr(Game* gp) {
    game_ptr = gp;
}

int AIPlayer::minimax(Board currentBoard, int depth, bool isMaximizingPlayer, PieceColor aiPlayerColor) const {
    if (depth == 0) {
        return currentBoard.evaluateMaterial(aiPlayerColor);
    }
    if (!game_ptr) return 0; // Should not happen if setGamePtr is called

    PieceColor turnColor = isMaximizingPlayer ? aiPlayerColor : (aiPlayerColor == PieceColor::WHITE ? PieceColor::BLACK : PieceColor::WHITE);
    std::vector<Move> legalMoves = game_ptr->generateLegalMoves(turnColor, currentBoard);

    if (legalMoves.empty()) {
        if (game_ptr->isKingInCheck(turnColor, currentBoard)) {
            return isMaximizingPlayer ? -200000 - depth : 200000 + depth; // Checkmate, prefer faster checkmates
        }
        return 0; // Stalemate
    }

    if (isMaximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto& move : legalMoves) {
            Board nextBoard = currentBoard;
            Move tempMove = move;
            nextBoard.makeMove(tempMove);
            game_ptr->applyPromotionIfAny(tempMove, nextBoard);
            maxEval = std::max(maxEval, minimax(nextBoard, depth - 1, false, aiPlayerColor));
        }
        return maxEval;
    }
    else { // Minimizing player
        int minEval = std::numeric_limits<int>::max();
        for (const auto& move : legalMoves) {
            Board nextBoard = currentBoard;
            Move tempMove = move;
            nextBoard.makeMove(tempMove);
            game_ptr->applyPromotionIfAny(tempMove, nextBoard);
            minEval = std::min(minEval, minimax(nextBoard, depth - 1, true, aiPlayerColor));
        }
        return minEval;
    }
}

Move AIPlayer::getMove(const Board& board, Game* gameInstance) const {
    if (!game_ptr && gameInstance) {
        // This is a bit of a hack; ideally, game_ptr is always set via constructor or dedicated method by Game.
        // const_cast is generally not good, but for this specific internal logic where AIPlayer needs a non-const Game*
        // to call some of its own methods for simulation, and getMove is const.
        // A better design would be to pass a const Game* and have helper functions be static or part of Board if possible.
        const_cast<AIPlayer*>(this)->game_ptr = gameInstance;
    }
    if (!game_ptr) {
        throw std::runtime_error("AIPlayer game_ptr not set properly.");
    }

    std::vector<Move> legalMoves = game_ptr->generateLegalMoves(playerColor, board);

    if (legalMoves.empty()) {
        throw std::runtime_error("AIPlayer Error: No legal moves available.");
    }

    if (difficulty == AIDifficulty::EASY) {
        return legalMoves[std::rand() % legalMoves.size()];
    }

    Move bestMove = legalMoves[0];
    int bestScore = std::numeric_limits<int>::min();

    if (difficulty == AIDifficulty::MEDIUM) {
        for (const auto& move : legalMoves) {
            Board tempBoard = board;
            Move tempMove = move;
            tempBoard.makeMove(tempMove);
            game_ptr->applyPromotionIfAny(tempMove, tempBoard);
            int score = tempBoard.evaluateMaterial(playerColor);

            PieceColor opponentColor = (playerColor == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
            if (game_ptr->isKingInCheck(opponentColor, tempBoard)) {
                score += 50;
            }

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            else if (score == bestScore && (std::rand() % 3 == 0)) {
                bestMove = move;
            }
        }
    }
    else if (difficulty == AIDifficulty::HARD) {
        int minimaxDepth = 2;
        for (const auto& move : legalMoves) {
            Board tempBoard = board;
            Move tempMove = move; // make a mutable copy
            tempBoard.makeMove(tempMove);
            game_ptr->applyPromotionIfAny(tempMove, tempBoard);
            int currentMoveScore = minimax(tempBoard, minimaxDepth - 1, false, playerColor);

            if (currentMoveScore > bestScore) {
                bestScore = currentMoveScore;
                bestMove = move;
            }
            else if (currentMoveScore == bestScore && (std::rand() % 2 == 0)) {
                bestMove = move;
            }
        }
    }
    return bestMove;
}

// HumanPlayer getMove method definition
Move HumanPlayer::getMove(const Board& board, Game* gameInstance) const {
    std::string lineInput;
    Position fromPos, toPos;
    Move playerMove;

    std::vector<Move> legalMoves = gameInstance->generateLegalMoves(playerColor, board);


    while (true) {
        std::cout << "Enter your move (e.g., e2e4 or e2 e4): ";
        std::getline(std::cin, lineInput);

        std::string processedInput = formatMoveInput(lineInput);

        if (processedInput.length() == 4) {
            fromPos = parsePosition(processedInput.substr(0, 2));
            toPos = parsePosition(processedInput.substr(2, 2));
        }
        else {
            std::cout << "Invalid input format. Use 'e2e4' or 'e2 e4'." << std::endl;
            continue;
        }


        if (!fromPos.isValid() || !toPos.isValid()) {
            std::cout << "Invalid square entered. Columns 'a'-'h', rows '1'-'8'." << std::endl;
            continue;
        }

        std::shared_ptr<Piece> selectedPiece = board.getPieceAt(fromPos);
        if (!selectedPiece || selectedPiece->getColor() != playerColor) {
            std::cout << "You don't have a piece at " << processedInput.substr(0, 2) << " or it's not your piece." << std::endl;
            continue;
        }

        playerMove.from = fromPos;
        playerMove.to = toPos;
        playerMove.promotionPiece = PieceType::EMPTY;
        playerMove.isEnPassantCapture = false;
        playerMove.isCastlingMove = false;


        bool foundLegal = false;
        for (const auto& legal_m : legalMoves) {
            if (legal_m.from == playerMove.from && legal_m.to == playerMove.to) {
                playerMove = legal_m; // This copies all flags including potential promotion piece types from getValidMoves
                foundLegal = true;
                break;
            }
        }

        if (foundLegal) {
            if (selectedPiece->getType() == PieceType::PAWN) {
                if ((playerColor == PieceColor::WHITE && toPos.row == 0) || (playerColor == PieceColor::BLACK && toPos.row == 7)) {
                    char prom_char_input;
                    std::cout << "Pawn promotion! Choose piece (Q, R, B, N): ";
                    std::cin >> prom_char_input;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                    prom_char_input = std::tolower(prom_char_input);
                    if (prom_char_input == 'q') playerMove.promotionPiece = PieceType::QUEEN;
                    else if (prom_char_input == 'r') playerMove.promotionPiece = PieceType::ROOK;
                    else if (prom_char_input == 'b') playerMove.promotionPiece = PieceType::BISHOP;
                    else if (prom_char_input == 'n') playerMove.promotionPiece = PieceType::KNIGHT;
                    else {
                        std::cout << "Invalid choice, defaulting to Queen." << std::endl;
                        playerMove.promotionPiece = PieceType::QUEEN;
                    }
                }
            }
            break;
        }
        else {
            std::cout << "That's not a legal move. Try again." << std::endl;
        }
    }
    return playerMove;
}


void displayStylizedAZD() {
    std::cout << "\n\n"
        << "    A    ZZZZZ  DDDD  \n"
        << "   A A      Z   D   D \n"
        << "  AAAAA    Z    D   D \n"
        << " A     A  Z     D   D \n"
        << "A       AZZZZZ  DDDD  \n"
        << "\n\n";
}


int main() {
    Game chessGame;
    chessGame.start();
    displayStylizedAZD();
    return 0;
}