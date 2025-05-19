# 📘 Intermediate C++ Applications
![badge](https://img.shields.io/badge/C%2B%2B-17%2B-blue) ![Intermediate Level](https://img.shields.io/badge/Intermediate-Level-orange.svg)
 

Welcome to the **Intermediate C++ Applications** repository\! 🎉 This repository is designed for programmers who have a foundational understanding of C++ and want to level up by building more robust, feature-rich applications. Here, you'll find engaging projects that help sharpen your programming skills and problem-solving abilities.

## 🚀 What's Inside?

This repository features a collection of intermediate C++ projects designed to enhance your understanding of core programming concepts, Object-Oriented Programming (OOP), algorithms, and data handling. Each project is structured to provide practical experience in C++ programming.

## 🌟 Application Descriptions

✅ **Library Management System** 📚

A comprehensive command-line system for managing books and members in a library. It supports adding, displaying, and searching for books and members, as well as lending and returning books. Features include due date tracking, overdue book reports, and data persistence via text files.

✅ **Student Information System** 🧑‍🎓

An application designed to manage student records, including their personal details, courses, and grades. It allows for adding new students, editing existing information, deleting records, adding courses with grades, and calculating GPA. Data is saved to and loaded from a text file.

✅ **Mini Database (with CRUD functions)** 🗂️

A lightweight, command-line database system that supports Create, Read, Update, and Delete (CRUD) operations. Records are stored in a CSV-like file, with functionality for adding new records, viewing all records (sorted or unsorted), searching by name, and modifying or removing existing entries.

✅ **Maze Solver (with DFS/BFS algorithms)** 🧩

A console application that can generate mazes using algorithms like Recursive Backtracker or Prim's, and then solve them using Depth-First Search (DFS), Breadth-First Search (BFS), or A\*. It features a visual representation of the maze generation and solving process in the console, with options to save and load mazes.

✅ **Phonebook Application (Class-based)** 📞

A command-line application for managing a personal phonebook. Users can add new contacts (first name, last name, phone number, email), display all contacts in a sorted list, search for specific contacts, edit existing contact details, and delete contacts. Information is persisted in a data file.

✅ **Simple Sudoku Generator/Solver** 🔢

A tool that generates Sudoku puzzles of varying difficulty levels (Easy, Medium, Hard, Expert) and solves them using a backtracking algorithm. Users can also input their own Sudoku puzzles to be solved by the application. Features a console-based grid display.

✅ **Bank Account Management** 🏦

A command-line system that simulates secure banking operations. It includes user registration and login with password hashing, creation of savings and checking accounts, fund deposits and withdrawals, balance inquiries, and transaction history. Data is persisted across sessions with basic checksum verification for file integrity.

✅ **Memory Game** 🧠

A colorful command-line card-matching memory game. Players flip cards to find matching pairs of symbols. The game features multiple difficulty levels (board sizes), scoring based on matches and consecutive matches, and tracks high scores for each difficulty, saved to separate files.

✅ **Othello Game (with AI)** ⚫⚪

A console-based Othello (Reversi) game where the player competes against an AI opponent. The AI uses the Minimax algorithm with alpha-beta pruning and piece-square tables for move evaluation. The game displays the board, valid moves, and scores, with options to choose player color and AI difficulty.

✅ **Chess** ♟️

A command-line chess game implementing standard chess rules, including all piece movements, castling, en passant, and pawn promotion. Players can compete against an AI opponent which uses the Minimax algorithm for decision-making. Features include selection of player color and AI difficulty, along with high score tracking.

-----

### 🔢️ Download & Run

Each of the above projects is provided as a `.cpp` file. You can compile and run them using any standard C++ compiler.

-----

### 🔧 Running the Source Code

Clone the repository:

```bash
git clone https://github.com/aslihanzehradonmez/intermediate-cpp-applications.git
```

Navigate to the project folder:

```bash
cd intermediate-cpp-applications
```

Compile and run any C++ script:

```bash
g++ BankManagement.cpp -o BankManagement
./BankManagement
```

-----

## 🔠 Project Structure

```
intermediate-cpp-applications/
│
├── BankManagement.cpp
├── Chess.cpp
├── LibraryManagement.cpp
├── MazeSolver.cpp
├── MemoryGame.cpp
├── MiniDB.cpp
├── OthelloAI.cpp
├── PhonebookApp.cpp
├── StudentInformationSystem.cpp
└── SudokuSolver.cpp
```

This repository is a great resource for intermediate-level C++ developers looking to expand their skill set through practical examples. Dive in and start coding\! 🚀