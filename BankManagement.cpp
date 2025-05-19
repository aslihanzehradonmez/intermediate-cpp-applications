#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <limits>
#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <map>
#include <random> 
#include <numeric> 
#include <cctype>

#ifdef _WIN32
#include <conio.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif


namespace ConsoleColors {
    const std::string RESET = "\033[0m";
    const std::string PROMPT = "\033[33m";
    const std::string ERROR_MSG = "\033[1;31m";
    const std::string SUCCESS_MSG = "\033[1;32m";
    const std::string HEADER = "\033[1m";
    const std::string ACCENT = "\033[36m";
    const std::string SYSTEM_INFO = "\033[35m";
}

namespace DateTimeUtil {
    std::string getCurrentTimestampStr() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm buf;
#ifdef _WIN32
        localtime_s(&buf, &now_time);
#else
        localtime_r(&now_time, &buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::time_t getCurrentTimeT() {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }

    std::string timeTToString(std::time_t t) {
        if (t == 0) return "0";
        std::tm buf;
#ifdef _WIN32
        localtime_s(&buf, &t);
#else
        localtime_r(&t, &buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
}

class Logger {
private:
    std::ofstream logFile;
    const std::string logFileName = "secure_bank_log.txt";
    static Logger* instance;

    Logger() {
        logFile.open(logFileName, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "FATAL ERROR: Could not open log file: " << logFileName << std::endl;
        }
    }

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger* getInstance() {
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }

    void log(const std::string& message, const std::string& level = "INFO") {
        if (logFile.is_open()) {
            logFile << "[" << DateTimeUtil::getCurrentTimestampStr() << "] [" << level << "] " << message << std::endl;
        }
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};
Logger* Logger::instance = nullptr;

namespace SecurityUtil {
    const unsigned int HASH_ITERATIONS = 1000;
    const unsigned long long HASH_PRIME_1 = 31;
    const unsigned long long HASH_PRIME_2 = 1000000007;

    std::string generateSalt(size_t length = 16) {
        const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string salt;
        salt.reserve(length);
        std::mt19937 rng(static_cast<unsigned int>(DateTimeUtil::getCurrentTimeT()) ^ static_cast<unsigned int>(std::random_device{}()));
        std::uniform_int_distribution<int> dist(0, static_cast<int>(charset.length()) - 1);
        for (size_t i = 0; i < length; ++i) {
            salt += charset[dist(rng)];
        }
        return salt;
    }

    std::string hashPassword(const std::string& password, const std::string& salt) {
        std::string toHash = salt + password + salt;
        unsigned long long currentHash = 0;

        for (char c : toHash) {
            currentHash = (currentHash * HASH_PRIME_1 + static_cast<unsigned char>(c)) % HASH_PRIME_2;
        }

        for (unsigned int i = 0; i < HASH_ITERATIONS; ++i) {
            std::string iterationString = std::to_string(currentHash) + salt + std::to_string(i);
            for (char c_iter : iterationString) {
                currentHash = (currentHash * HASH_PRIME_1 + static_cast<unsigned char>(c_iter)) % HASH_PRIME_2;
            }
        }
        std::ostringstream oss;
        oss << std::hex << currentHash;
        return oss.str();
    }

    unsigned int calculateSimpleChecksum(const std::vector<std::string>& dataLines) {
        unsigned int checksum = 0;
        for (const std::string& line : dataLines) {
            for (char c : line) {
                checksum = (checksum << 5) + checksum + static_cast<unsigned char>(c);
            }
        }
        return checksum % 65536;
    }
}


void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printMessage(const std::string& message, const std::string& color = ConsoleColors::RESET, bool newLine = true) {
    std::cout << color << message << ConsoleColors::RESET;
    if (newLine) {
        std::cout << std::endl;
    }
}


std::string getPasswordInput() {
    std::string password;
    char ch;

#ifdef _WIN32
    ch = _getch();
    while (ch != '\r' && ch != '\n') {
        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        }
        else if (isprint(static_cast<unsigned char>(ch))) {
            password.push_back(ch);
            std::cout << '*';
        }
        ch = _getch();
    }
    std::cout << std::endl;

#elif defined(__linux__) || defined(__APPLE__)
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);

    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
        perror("tcsetattr");
        printMessage("\nWarning: Could not set terminal for secure password input. Password will be visible.", ConsoleColors::ERROR_MSG);
        std::cin >> password;
        if (std::cin.peek() == '\n') std::cin.ignore();
        return password;
    }

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != '\n' && c != '\r') {
        if (c == 127 || c == 8) {
            if (!password.empty()) {
                password.pop_back();
                write(STDOUT_FILENO, "\b \b", 3);
            }
        }
        else if (isprint(static_cast<unsigned char>(c))) {
            password.push_back(c);
            write(STDOUT_FILENO, "*", 1);
        }
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0) {
        perror("tcsetattr (restore)");
    }
    std::cout << std::endl;

#else
    printMessage("(Warning: Password input will be visible on this system)", ConsoleColors::PROMPT, false);
    std::cin >> password;
    if (std::cin.peek() == '\n') std::cin.ignore();
#endif
    return password;
}


void pressEnterToContinue() {
    printMessage("\nPress Enter to continue...", ConsoleColors::PROMPT, false);
    if (std::cin.rdbuf()->in_avail() > 0 && std::cin.peek() == '\n') {
        std::cin.ignore();
    }
    else if (std::cin.rdbuf()->in_avail() > 0) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string dummy;
    std::getline(std::cin, dummy);
}


struct Transaction {
    std::string type;
    double amount;
    std::string timestamp;
    double balanceAfterTransaction;

    Transaction(std::string t, double amt, std::string ts, double bal)
        : type(std::move(t)), amount(amt), timestamp(std::move(ts)), balanceAfterTransaction(bal) {
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Type: " << type
            << ", Amount: " << amount
            << ", Time: " << timestamp
            << ", Balance After: " << balanceAfterTransaction;
        return oss.str();
    }

    std::string toFileString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << type << "|" << amount << "|" << timestamp << "|" << balanceAfterTransaction;
        return oss.str();
    }

    static Transaction fromFileString(const std::string& line) {
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> segments;
        while (std::getline(ss, segment, '|')) {
            segments.push_back(segment);
        }
        if (segments.size() == 4) {
            try {
                return Transaction(segments[0], std::stod(segments[1]), segments[2], std::stod(segments[3]));
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Invalid transaction data format in string: " + line + " Details: " + e.what());
            }
        }
        throw std::runtime_error("Invalid transaction file string format, expected 4 segments: " + line);
    }
};

class Account {
protected:
    std::string accountNumber;
    std::string accountHolderName;
    double balance;
    std::string ownerUsername;
    std::vector<Transaction> transactionHistory;

public:
    Account(std::string accNum, std::string holderName, double bal, std::string owner)
        : accountNumber(std::move(accNum)), accountHolderName(std::move(holderName)), balance(bal), ownerUsername(std::move(owner)) {
    }

    virtual ~Account() = default;

    std::string getAccountNumber() const { return accountNumber; }
    std::string getAccountHolderName() const { return accountHolderName; }
    double getBalance() const { return balance; }
    std::string getOwnerUsername() const { return ownerUsername; }
    const std::vector<Transaction>& getTransactionHistory() const { return transactionHistory; }

    void addTransaction(const Transaction& transaction) {
        transactionHistory.push_back(transaction);
    }

    void loadTransaction(const Transaction& transaction) {
        transactionHistory.push_back(transaction);
    }


    virtual void deposit(double amount) {
        if (amount > 0.009) {
            balance += amount;
            std::string ts = DateTimeUtil::getCurrentTimestampStr();
            addTransaction(Transaction("Deposit", amount, ts, balance));
            printMessage("Deposited: " + std::to_string(amount) + ". New balance: " + std::to_string(balance), ConsoleColors::SUCCESS_MSG);
            Logger::getInstance()->log("Deposit of " + std::to_string(amount) + " to account " + accountNumber + ". New balance: " + std::to_string(balance));
        }
        else {
            printMessage("Invalid deposit amount (must be > 0.00).", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Failed deposit attempt to account " + accountNumber + " with amount " + std::to_string(amount), "WARNING");
        }
    }

    virtual bool withdraw(double amount) {
        if (amount <= 0.009) {
            printMessage("Invalid withdrawal amount (must be > 0.00).", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Failed withdrawal attempt from account " + accountNumber + " with amount " + std::to_string(amount), "WARNING");
            return false;
        }
        if (balance >= amount) {
            balance -= amount;
            std::string ts = DateTimeUtil::getCurrentTimestampStr();
            addTransaction(Transaction("Withdrawal", amount, ts, balance));
            printMessage("Withdrew: " + std::to_string(amount) + ". New balance: " + std::to_string(balance), ConsoleColors::SUCCESS_MSG);
            Logger::getInstance()->log("Withdrawal of " + std::to_string(amount) + " from account " + accountNumber + ". New balance: " + std::to_string(balance));
            return true;
        }
        else {
            printMessage("Insufficient balance.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Failed withdrawal attempt from account " + accountNumber + " due to insufficient balance. Amount: " + std::to_string(amount), "WARNING");
            return false;
        }
    }

    virtual void displayAccountDetails() const {
        std::cout << ConsoleColors::ACCENT << "Account Number: " << ConsoleColors::RESET << accountNumber << std::endl;
        std::cout << ConsoleColors::ACCENT << "Account Holder: " << ConsoleColors::RESET << accountHolderName << std::endl;
        std::cout << ConsoleColors::ACCENT << "Owner Username: " << ConsoleColors::RESET << ownerUsername << std::endl;
        std::cout << ConsoleColors::ACCENT << "Balance: " << ConsoleColors::RESET << std::fixed << std::setprecision(2) << balance << " USD" << std::endl;
    }

    void displayTransactionHistory() const {
        printMessage("\n--- Transaction History for " + accountNumber + " ---", ConsoleColors::HEADER);
        if (transactionHistory.empty()) {
            printMessage("No transactions available.", ConsoleColors::SYSTEM_INFO);
        }
        else {
            for (const auto& t : transactionHistory) {
                std::cout << ConsoleColors::RESET << t.toString() << std::endl;
            }
        }
    }

    virtual std::string getType() const = 0;
    virtual std::string toFileString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << ownerUsername << "|" << getType() << "|" << accountNumber << "|" << accountHolderName << "|" << balance;
        return oss.str();
    }
    virtual void applyInterest() {}
};

class SavingsAccount : public Account {
private:
    double interestRate;

public:
    SavingsAccount(std::string accNum, std::string holderName, double bal, std::string owner, double rate)
        : Account(std::move(accNum), std::move(holderName), bal, std::move(owner)), interestRate(rate) {
    }

    void applyInterest() override {
        double interest = balance * interestRate;
        if (interest > 0.009) {
            balance += interest;
            std::string ts = DateTimeUtil::getCurrentTimestampStr();
            addTransaction(Transaction("Interest", interest, ts, balance));
            printMessage("Interest applied: " + std::to_string(interest) + ". New balance: " + std::to_string(balance), ConsoleColors::SUCCESS_MSG);
            Logger::getInstance()->log("Interest " + std::to_string(interest) + " applied to savings account " + accountNumber);
        }
    }

    void displayAccountDetails() const override {
        Account::displayAccountDetails();
        std::cout << ConsoleColors::ACCENT << "Account Type: " << ConsoleColors::RESET << "Savings Account" << std::endl;
        std::cout << ConsoleColors::ACCENT << "Interest Rate: " << ConsoleColors::RESET << std::fixed << std::setprecision(2) << (interestRate * 100) << "%" << std::endl;
    }

    std::string getType() const override { return "SAVINGS"; }

    std::string toFileString() const override {
        std::ostringstream oss;
        oss << Account::toFileString() << "|" << interestRate;
        return oss.str();
    }
};

class CheckingAccount : public Account {
private:
    double overdraftLimit;
    double transactionFee;

public:
    CheckingAccount(std::string accNum, std::string holderName, double bal, std::string owner, double overdraft, double fee)
        : Account(std::move(accNum), std::move(holderName), bal, std::move(owner)), overdraftLimit(overdraft), transactionFee(fee) {
    }

    bool withdraw(double amount) override {
        if (amount <= 0.009) {
            printMessage("Invalid withdrawal amount (must be > 0.00).", ConsoleColors::ERROR_MSG);
            return false;
        }
        double totalDeduction = amount + transactionFee;
        if (balance + overdraftLimit >= totalDeduction) {
            balance -= totalDeduction;
            std::string ts = DateTimeUtil::getCurrentTimestampStr();
            addTransaction(Transaction("Withdrawal (Net)", amount, ts, balance));

            printMessage("Withdrew: " + std::to_string(amount) + ", Transaction Fee: " + std::to_string(transactionFee) + ". New balance: " + std::to_string(balance), ConsoleColors::SUCCESS_MSG);
            Logger::getInstance()->log("Withdrawal " + std::to_string(amount) + " (Fee: " + std::to_string(transactionFee) + ") from checking account " + accountNumber + ". New balance: " + std::to_string(balance));
            return true;
        }
        else {
            printMessage("Insufficient funds including overdraft limit.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Failed withdrawal from checking account " + accountNumber + " due to insufficient funds (Amount: " + std::to_string(amount) + ")", "WARNING");
            return false;
        }
    }

    void displayAccountDetails() const override {
        Account::displayAccountDetails();
        std::cout << ConsoleColors::ACCENT << "Account Type: " << ConsoleColors::RESET << "Checking Account" << std::endl;
        std::cout << ConsoleColors::ACCENT << "Overdraft Limit: " << ConsoleColors::RESET << std::fixed << std::setprecision(2) << overdraftLimit << " USD" << std::endl;
        std::cout << ConsoleColors::ACCENT << "Transaction Fee: " << ConsoleColors::RESET << std::fixed << std::setprecision(2) << transactionFee << " USD" << std::endl;
    }

    std::string getType() const override { return "CHECKING"; }

    std::string toFileString() const override {
        std::ostringstream oss;
        oss << Account::toFileString() << "|" << overdraftLimit << "|" << transactionFee;
        return oss.str();
    }
};


struct User {
    std::string username;
    std::string salt;
    std::string hashedPasswordWithSalt;
    bool isAdmin;
    int failedLoginAttempts;
    std::time_t lockoutUntilTimestamp;

    User(std::string uname = "", std::string s = "", std::string pwd = "", bool admin = false, int attempts = 0, std::time_t lockout = 0)
        : username(std::move(uname)), salt(std::move(s)), hashedPasswordWithSalt(std::move(pwd)), isAdmin(admin), failedLoginAttempts(attempts), lockoutUntilTimestamp(lockout) {
    }

    std::string toFileString() const {
        return username + "|" + salt + "|" + hashedPasswordWithSalt + "|" + (isAdmin ? "1" : "0") + "|" + std::to_string(failedLoginAttempts) + "|" + std::to_string(lockoutUntilTimestamp);
    }

    static User fromFileString(const std::string& line) {
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> segments;
        while (std::getline(ss, segment, '|')) {
            segments.push_back(segment);
        }
        if (segments.size() == 6) {
            try {
                return User(segments[0], segments[1], segments[2], segments[3] == "1", std::stoi(segments[4]), static_cast<std::time_t>(std::stoll(segments[5])));
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Invalid user data format in string: " + line + " Details: " + e.what());
            }
        }
        throw std::runtime_error("Invalid user file string format, expected 6 segments: " + line);
    }
};

const int MAX_LOGIN_ATTEMPTS = 5;
const int LOCKOUT_DURATION_SECONDS = 300;

class Bank {
private:
    std::vector<std::unique_ptr<Account>> allAccounts;
    std::map<std::string, User> users;
    User currentUser;
    bool userLoggedIn = false;

    long long nextAccountNumberSeed;
    const std::string accountsDataFile = "s_accounts.txt";
    const std::string usersDataFile = "s_users.txt";
    const std::string transactionsDataFile = "s_transactions.txt";
    const std::string seedFileName = "s_account_seed.txt";
    const std::string checksumHeader = "CHECKSUM:";


    bool verifyAndLoadFile(const std::string& filename, std::vector<std::string>& lines) {
        std::ifstream inFile(filename);
        if (!inFile.is_open()) {
            Logger::getInstance()->log("File not found (normal for first run): " + filename, "INFO");
            return true;
        }

        std::string checksumLineStr;
        std::getline(inFile, checksumLineStr);
        unsigned int storedChecksum = 0;
        bool checksumWasPresent = false;

        if (checksumLineStr.rfind(checksumHeader, 0) == 0) {
            try {
                storedChecksum = std::stoul(checksumLineStr.substr(checksumHeader.length()));
                checksumWasPresent = true;
            }
            catch (const std::exception& e) {
                Logger::getInstance()->log("Invalid checksum format in " + filename + ". Error: " + e.what(), "ERROR");
                inFile.close();
                printMessage("ERROR: Corrupted checksum line in " + filename + ". File not loaded.", ConsoleColors::ERROR_MSG);
                return false;
            }
        }
        else {
            Logger::getInstance()->log("Checksum line missing or not first in " + filename + ". Proceeding without checksum verification for this load.", "WARNING");
            inFile.clear();
            inFile.seekg(0, std::ios::beg);
        }

        std::string line;
        while (std::getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();

        if (checksumWasPresent) {
            unsigned int calculatedChecksum = SecurityUtil::calculateSimpleChecksum(lines);
            if (calculatedChecksum != storedChecksum) {
                Logger::getInstance()->log("Checksum mismatch for " + filename + "! Data might be corrupted or tampered with. Calculated: " + std::to_string(calculatedChecksum) + ", Stored: " + std::to_string(storedChecksum), "CRITICAL");
                printMessage("CRITICAL WARNING: Data integrity check failed for " + filename + ". Data may be corrupted. Please check logs.", ConsoleColors::ERROR_MSG, true);
            }
            else {
                Logger::getInstance()->log("Checksum verified for " + filename, "INFO");
            }
        }
        return true;
    }

    void saveFileWithChecksum(const std::string& filename, const std::vector<std::string>& lines) const {
        unsigned int checksum = SecurityUtil::calculateSimpleChecksum(lines);
        std::ofstream outFile(filename, std::ios::trunc);
        if (!outFile.is_open()) {
            Logger::getInstance()->log("Failed to open " + filename + " for saving.", "ERROR");
            return;
        }
        outFile << checksumHeader << checksum << std::endl;
        for (const std::string& line : lines) {
            outFile << line << std::endl;
        }
        outFile.close();
        Logger::getInstance()->log("Saved " + filename + " with checksum " + std::to_string(checksum), "INFO");
    }


    void loadNextAccountNumberSeed() {
        std::ifstream seedFile(seedFileName);
        if (seedFile.is_open()) {
            seedFile >> nextAccountNumberSeed;
            if (seedFile.fail() || nextAccountNumberSeed < 1001) {
                nextAccountNumberSeed = 1001;
            }
            seedFile.close();
        }
        else {
            nextAccountNumberSeed = 1001;
        }
    }

    void saveNextAccountNumberSeed() const {
        std::ofstream seedFile(seedFileName, std::ios::trunc);
        if (seedFile.is_open()) {
            seedFile << nextAccountNumberSeed;
            seedFile.close();
        }
        else {
            Logger::getInstance()->log("Failed to save account number seed.", "ERROR");
        }
    }

    std::string generateAccountNumber() {
        return "ACC" + std::to_string(nextAccountNumberSeed++);
    }

    void loadUsers() {
        std::vector<std::string> lines;
        if (!verifyAndLoadFile(usersDataFile, lines)) {
            Logger::getInstance()->log("Critical error loading users file: " + usersDataFile, "CRITICAL");
        }

        users.clear();
        for (const std::string& line : lines) {
            try {
                User user = User::fromFileString(line);
                users[user.username] = user;
            }
            catch (const std::exception& e) {
                Logger::getInstance()->log(std::string("Error parsing user from line: ") + line + " - " + e.what(), "ERROR");
            }
        }

        if (users.find("admin") == users.end()) {
            std::string salt = SecurityUtil::generateSalt();
            User adminUser("admin", salt, SecurityUtil::hashPassword("adminP@$$wOrd", salt), true, 0, 0);
            users[adminUser.username] = adminUser;
            Logger::getInstance()->log("Default admin user created/recreated as it was missing.", "INFO");
            saveUsers();
        }
    }

    void saveUsers() const {
        std::vector<std::string> lines;
        for (const auto& pair : users) {
            lines.push_back(pair.second.toFileString());
        }
        saveFileWithChecksum(usersDataFile, lines);
    }


    void loadAccountsFromFile() {
        std::vector<std::string> lines;
        verifyAndLoadFile(accountsDataFile, lines);

        allAccounts.clear();
        for (const std::string& line : lines) {
            std::stringstream ss(line);
            std::string segment;
            std::vector<std::string> segments;
            while (std::getline(ss, segment, '|')) {
                segments.push_back(segment);
            }

            if (segments.size() < 5) {
                Logger::getInstance()->log("Skipping malformed account line (not enough segments): " + line, "WARNING");
                continue;
            }

            std::string owner = segments[0];
            std::string type = segments[1];
            std::string accNum = segments[2];
            std::string holderName = segments[3];
            double balance = 0.0;
            try {
                balance = std::stod(segments[4]);
            }
            catch (const std::exception& e) {
                Logger::getInstance()->log(std::string("Invalid balance format for account ") + accNum + " in line: " + line + " Error: " + e.what(), "ERROR");
                continue;
            }

            if (type == "SAVINGS" && segments.size() == 6) {
                double rate = 0.0;
                try { rate = std::stod(segments[5]); }
                catch (const std::exception& e) {
                    Logger::getInstance()->log("Invalid rate for savings account " + accNum + ". Error: " + e.what(), "ERROR"); continue;
                }
                allAccounts.push_back(std::make_unique<SavingsAccount>(accNum, holderName, balance, owner, rate));
            }
            else if (type == "CHECKING" && segments.size() == 7) {
                double overdraft = 0.0, fee = 0.0;
                try { overdraft = std::stod(segments[5]); fee = std::stod(segments[6]); }
                catch (const std::exception& e) {
                    Logger::getInstance()->log("Invalid params for checking account " + accNum + ". Error: " + e.what(), "ERROR"); continue;
                }
                allAccounts.push_back(std::make_unique<CheckingAccount>(accNum, holderName, balance, owner, overdraft, fee));
            }
            else {
                Logger::getInstance()->log("Unknown account type or incorrect segment count in line: " + line, "WARNING");
            }
        }
    }

    void saveAccountsToFile() const {
        std::vector<std::string> lines;
        for (const auto& acc : allAccounts) {
            lines.push_back(acc->toFileString());
        }
        saveFileWithChecksum(accountsDataFile, lines);
    }

    void loadTransactionsFromFile() {
        std::vector<std::string> lines;
        verifyAndLoadFile(transactionsDataFile, lines);

        for (const std::string& line : lines) {
            std::stringstream ss(line);
            std::string accNumFromFile;
            std::string transactionData;
            if (std::getline(ss, accNumFromFile, '#') && std::getline(ss, transactionData)) {
                Account* acc = findAccountGlobal(accNumFromFile);
                if (acc) {
                    try {
                        acc->loadTransaction(Transaction::fromFileString(transactionData));
                    }
                    catch (const std::exception& e) {
                        Logger::getInstance()->log(std::string("Error loading transaction for account ") + accNumFromFile + " from line: " + line + " - " + e.what(), "ERROR");
                    }
                }
                else {
                    Logger::getInstance()->log("Transaction found for non-existent account " + accNumFromFile + " in line: " + line, "WARNING");
                }
            }
            else {
                Logger::getInstance()->log("Malformed transaction line (missing '#'): " + line, "WARNING");
            }
        }
    }

    void saveTransactionsToFile() const {
        std::vector<std::string> lines;
        for (const auto& acc : allAccounts) {
            for (const auto& trans : acc->getTransactionHistory()) {
                lines.push_back(acc->getAccountNumber() + "#" + trans.toFileString());
            }
        }
        saveFileWithChecksum(transactionsDataFile, lines);
    }

    Account* findAccountGlobal(const std::string& accNum) const {
        for (const auto& acc : allAccounts) {
            if (acc->getAccountNumber() == accNum) {
                return acc.get();
            }
        }
        return nullptr;
    }


public:
    Bank() : nextAccountNumberSeed(1001), userLoggedIn(false) {
        Logger::getInstance()->log("Secure Bank system starting up.", "INFO");
        loadNextAccountNumberSeed();
        loadUsers();
        loadAccountsFromFile();
        loadTransactionsFromFile();
    }

    ~Bank() {
        if (userLoggedIn) {
        }
        else {
            const User* adminUser = users.count("admin") ? &users.at("admin") : nullptr;
            if (adminUser) saveUsers();
        }
        saveNextAccountNumberSeed();
        saveUsers();
        saveAccountsToFile();
        saveTransactionsToFile();

        Logger::getInstance()->log("Secure Bank system shutting down.", "INFO");
        delete Logger::getInstance();
    }

    bool login() {
        clearScreen();
        printMessage("--- LOGIN ---", ConsoleColors::HEADER);
        std::string username, password_str;
        std::cout << ConsoleColors::PROMPT << "Username: " << ConsoleColors::RESET;
        std::cin >> username;
        if (std::cin.peek() == '\n') {
            std::cin.ignore();
        }

        std::cout << ConsoleColors::PROMPT << "Password: " << ConsoleColors::RESET;
        password_str = getPasswordInput();

        auto it = users.find(username);
        if (it != users.end()) {
            User& userToLogin = it->second;

            if (userToLogin.lockoutUntilTimestamp > 0 && DateTimeUtil::getCurrentTimeT() < userToLogin.lockoutUntilTimestamp) {
                printMessage("Account is locked. Try again after: " + DateTimeUtil::timeTToString(userToLogin.lockoutUntilTimestamp), ConsoleColors::ERROR_MSG);
                Logger::getInstance()->log("Login attempt for locked account: " + username, "WARNING");
                return false;
            }

            if (userToLogin.lockoutUntilTimestamp > 0 && DateTimeUtil::getCurrentTimeT() >= userToLogin.lockoutUntilTimestamp) {
                userToLogin.lockoutUntilTimestamp = 0;
                userToLogin.failedLoginAttempts = 0;
                Logger::getInstance()->log("Lockout period expired for user: " + username, "INFO");
            }

            if (userToLogin.hashedPasswordWithSalt == SecurityUtil::hashPassword(password_str, userToLogin.salt)) {
                currentUser = userToLogin;
                userLoggedIn = true;
                users[username].failedLoginAttempts = 0;
                users[username].lockoutUntilTimestamp = 0;
                saveUsers();

                printMessage("Login successful. Welcome " + currentUser.username + "!", ConsoleColors::SUCCESS_MSG);
                Logger::getInstance()->log("User " + currentUser.username + " logged in successfully.", "INFO");
                return true;
            }
            else {
                users[username].failedLoginAttempts++;
                int attempts_left = MAX_LOGIN_ATTEMPTS - users[username].failedLoginAttempts;
                if (users[username].failedLoginAttempts >= MAX_LOGIN_ATTEMPTS) {
                    users[username].lockoutUntilTimestamp = DateTimeUtil::getCurrentTimeT() + LOCKOUT_DURATION_SECONDS;
                    Logger::getInstance()->log("Account " + username + " locked due to " + std::to_string(users[username].failedLoginAttempts) + " failed login attempts. Locked until " + DateTimeUtil::timeTToString(users[username].lockoutUntilTimestamp), "WARNING");
                    printMessage("Too many failed login attempts. Account locked until " + DateTimeUtil::timeTToString(users[username].lockoutUntilTimestamp), ConsoleColors::ERROR_MSG);
                }
                else {
                    printMessage("Invalid username or password. Attempts remaining: " + std::to_string(attempts_left), ConsoleColors::ERROR_MSG);
                }
                saveUsers();
                Logger::getInstance()->log("Failed login attempt for username: " + username + ". Attempt " + std::to_string(users[username].failedLoginAttempts), "WARNING");
                return false;
            }
        }
        else {
            printMessage("Invalid username or password.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Failed login attempt for non-existent username: " + username, "WARNING");
            return false;
        }
    }

    void registerUser() {
        clearScreen();
        printMessage("--- USER REGISTRATION ---", ConsoleColors::HEADER);
        std::string username, password_str, confirmPassword_str;
        std::cout << ConsoleColors::PROMPT << "Enter new username (min 3 chars): " << ConsoleColors::RESET;
        std::cin >> username;
        if (std::cin.peek() == '\n') std::cin.ignore();


        if (username.length() < 3) {
            printMessage("Username must be at least 3 characters long.", ConsoleColors::ERROR_MSG);
            return;
        }
        if (users.count(username)) {
            printMessage("Username already exists. Please choose another.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Registration failed: Username " + username + " already exists.", "INFO");
            return;
        }
        std::cout << ConsoleColors::PROMPT << "Enter password (min 6 chars): " << ConsoleColors::RESET;
        password_str = getPasswordInput();
        if (password_str.length() < 6) {
            printMessage("Password must be at least 6 characters long.", ConsoleColors::ERROR_MSG);
            return;
        }
        std::cout << ConsoleColors::PROMPT << "Confirm password: " << ConsoleColors::RESET;
        confirmPassword_str = getPasswordInput();

        if (password_str != confirmPassword_str) {
            printMessage("Passwords do not match.", ConsoleColors::ERROR_MSG);
            return;
        }

        std::string salt = SecurityUtil::generateSalt();
        std::string hashedPassword = SecurityUtil::hashPassword(password_str, salt);
        User newUser(username, salt, hashedPassword, false, 0, 0);
        users[username] = newUser;
        saveUsers();
        printMessage("User " + username + " registered successfully. Please log in.", ConsoleColors::SUCCESS_MSG);
        Logger::getInstance()->log("New user registered: " + username, "INFO");
    }

    void logout() {
        Logger::getInstance()->log("User " + currentUser.username + " logging out.", "INFO");
        saveAccountsToFile();
        saveTransactionsToFile();
        saveUsers();
        currentUser = User();
        userLoggedIn = false;
        printMessage("Logged out successfully.", ConsoleColors::SUCCESS_MSG);
    }

    bool isUserLoggedIn() const {
        return userLoggedIn;
    }
    bool isCurrentUserAdmin() const {
        return userLoggedIn && currentUser.isAdmin;
    }


    void createNewAccount() {
        clearScreen();
        printMessage("--- CREATE NEW ACCOUNT ---", ConsoleColors::HEADER);
        std::string holderName;
        double initialDeposit;
        int accountTypeChoice;

        std::cout << ConsoleColors::PROMPT << "Account Holder Name (default: " << currentUser.username << "): " << ConsoleColors::RESET;
        if (std::cin.rdbuf()->in_avail() > 0 && std::cin.peek() == '\n') {
            std::cin.ignore();
        }
        std::getline(std::cin, holderName);
        if (holderName.empty()) {
            holderName = currentUser.username;
        }


        while (true) {
            std::cout << ConsoleColors::PROMPT << "Initial Deposit (USD, min 0.01): " << ConsoleColors::RESET;
            std::cin >> initialDeposit;
            if (std::cin.fail() || initialDeposit < 0.009) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid deposit. Enter a non-negative value (min 0.01).", ConsoleColors::ERROR_MSG);
            }
            else {
                if (std::cin.peek() == '\n') std::cin.ignore();
                break;
            }
        }

        std::cout << ConsoleColors::PROMPT << "Select Account Type:" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::ACCENT << "1. Savings Account" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::ACCENT << "2. Checking Account" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::PROMPT << "Your choice: " << ConsoleColors::RESET;
        std::cin >> accountTypeChoice;
        if (std::cin.peek() == '\n') std::cin.ignore();


        std::string newAccNum = generateAccountNumber();

        if (accountTypeChoice == 1) {
            double interestRate;
            while (true) {
                std::cout << ConsoleColors::PROMPT << "Annual Interest Rate (e.g., 0.05 for 5%): " << ConsoleColors::RESET;
                std::cin >> interestRate;
                if (std::cin.fail() || interestRate < 0 || interestRate > 1) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    printMessage("Invalid rate. Enter a value between 0.00 and 1.00.", ConsoleColors::ERROR_MSG);
                }
                else {
                    if (std::cin.peek() == '\n') std::cin.ignore();
                    break;
                }
            }
            allAccounts.push_back(std::make_unique<SavingsAccount>(newAccNum, holderName, initialDeposit, currentUser.username, interestRate));
            Logger::getInstance()->log("Savings account " + newAccNum + " created for user " + currentUser.username, "INFO");
            printMessage("Savings Account " + newAccNum + " created successfully.", ConsoleColors::SUCCESS_MSG);
        }
        else if (accountTypeChoice == 2) {
            double overdraftLimit, transactionFee;
            while (true) {
                std::cout << ConsoleColors::PROMPT << "Overdraft Limit (USD, min 0.00): " << ConsoleColors::RESET;
                std::cin >> overdraftLimit;
                if (std::cin.fail() || overdraftLimit < 0) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    printMessage("Invalid limit. Enter a non-negative value.", ConsoleColors::ERROR_MSG);
                }
                else {
                    if (std::cin.peek() == '\n') std::cin.ignore();
                    break;
                }
            }
            while (true) {
                std::cout << ConsoleColors::PROMPT << "Transaction Fee (USD, min 0.00): " << ConsoleColors::RESET;
                std::cin >> transactionFee;
                if (std::cin.fail() || transactionFee < 0) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    printMessage("Invalid fee. Enter a non-negative value.", ConsoleColors::ERROR_MSG);
                }
                else {
                    if (std::cin.peek() == '\n') std::cin.ignore();
                    break;
                }
            }
            allAccounts.push_back(std::make_unique<CheckingAccount>(newAccNum, holderName, initialDeposit, currentUser.username, overdraftLimit, transactionFee));
            Logger::getInstance()->log("Checking account " + newAccNum + " created for user " + currentUser.username, "INFO");
            printMessage("Checking Account " + newAccNum + " created successfully.", ConsoleColors::SUCCESS_MSG);
        }
        else {
            printMessage("Invalid account type. Account creation failed.", ConsoleColors::ERROR_MSG);
            nextAccountNumberSeed--;
        }
    }

    Account* selectUserAccount(const std::string& operationPurpose) {
        printMessage("--- YOUR ACCOUNTS for " + operationPurpose + " ---", ConsoleColors::HEADER);
        std::vector<Account*> userAccounts;
        for (const auto& acc : allAccounts) {
            if (acc->getOwnerUsername() == currentUser.username) {
                userAccounts.push_back(acc.get());
            }
        }

        if (userAccounts.empty()) {
            printMessage("You have no accounts.", ConsoleColors::SYSTEM_INFO);
            return nullptr;
        }

        for (size_t i = 0; i < userAccounts.size(); ++i) {
            std::cout << ConsoleColors::ACCENT << (i + 1) << ". " << userAccounts[i]->getAccountNumber()
                << " (" << userAccounts[i]->getType() << ") - Balance: "
                << std::fixed << std::setprecision(2) << userAccounts[i]->getBalance() << " USD"
                << ConsoleColors::RESET << std::endl;
        }

        int choice = 0;
        while (true) {
            std::cout << ConsoleColors::PROMPT << "Select account (number) or 0 to cancel: " << ConsoleColors::RESET;
            std::cin >> choice;
            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid input. Please enter a number.", ConsoleColors::ERROR_MSG);
            }
            else if (choice == 0) {
                if (std::cin.peek() == '\n') std::cin.ignore();
                return nullptr;
            }
            else if (choice > 0 && static_cast<size_t>(choice) <= userAccounts.size()) {
                if (std::cin.peek() == '\n') std::cin.ignore();
                return userAccounts[static_cast<size_t>(choice) - 1];
            }
            else {
                if (std::cin.peek() == '\n') std::cin.ignore();
                printMessage("Invalid selection. Try again.", ConsoleColors::ERROR_MSG);
            }
        }
    }


    void performDeposit() {
        clearScreen();
        Account* acc = selectUserAccount("Deposit");
        if (!acc) return;

        double amount;
        while (true) {
            std::cout << ConsoleColors::PROMPT << "Amount to deposit (USD, min 0.01): " << ConsoleColors::RESET;
            std::cin >> amount;
            if (std::cin.fail() || amount < 0.009) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid amount. Enter a positive value (min 0.01).", ConsoleColors::ERROR_MSG);
            }
            else {
                if (std::cin.peek() == '\n') std::cin.ignore();
                break;
            }
        }
        acc->deposit(amount);
    }

    void performWithdrawal() {
        clearScreen();
        Account* acc = selectUserAccount("Withdrawal");
        if (!acc) return;

        double amount;
        while (true) {
            std::cout << ConsoleColors::PROMPT << "Amount to withdraw (USD, min 0.01): " << ConsoleColors::RESET;
            std::cin >> amount;
            if (std::cin.fail() || amount < 0.009) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid amount. Enter a positive value (min 0.01).", ConsoleColors::ERROR_MSG);
            }
            else {
                if (std::cin.peek() == '\n') std::cin.ignore();
                break;
            }
        }
        acc->withdraw(amount);
    }

    void checkBalanceAndHistory() {
        clearScreen();
        Account* acc = selectUserAccount("Balance Inquiry");
        if (!acc) return;

        SavingsAccount* sa = dynamic_cast<SavingsAccount*>(acc);
        if (sa) {
            printMessage("Checking/applying interest for savings account...", ConsoleColors::SYSTEM_INFO);
            sa->applyInterest();
        }
        acc->displayAccountDetails();
        acc->displayTransactionHistory();
    }

    void applyInterestToMySavings() {
        clearScreen();
        printMessage("--- APPLY INTEREST TO MY SAVINGS ACCOUNTS ---", ConsoleColors::HEADER);
        bool foundSavings = false;
        for (auto& acc_ptr : allAccounts) {
            if (acc_ptr->getOwnerUsername() == currentUser.username) {
                SavingsAccount* sa = dynamic_cast<SavingsAccount*>(acc_ptr.get());
                if (sa) {
                    printMessage("Applying interest for account " + sa->getAccountNumber() + "...", ConsoleColors::ACCENT);
                    sa->applyInterest();
                    foundSavings = true;
                }
            }
        }
        if (!foundSavings) {
            printMessage("You have no savings accounts to apply interest to.", ConsoleColors::SYSTEM_INFO);
        }
        else {
            printMessage("Interest application process completed for your savings accounts.", ConsoleColors::SUCCESS_MSG);
        }
    }


    void displayMyAccounts() const {
        clearScreen();
        printMessage("--- MY ACCOUNTS ---", ConsoleColors::HEADER);
        bool found = false;
        for (const auto& acc : allAccounts) {
            if (acc->getOwnerUsername() == currentUser.username) {
                acc->displayAccountDetails();
                std::cout << "-------------------------" << std::endl;
                found = true;
            }
        }
        if (!found) {
            printMessage("You have no accounts registered.", ConsoleColors::SYSTEM_INFO);
        }
    }

    void displayAllAccountsAdmin() const {
        if (!isCurrentUserAdmin()) {
            printMessage("Access Denied. Admin only.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Non-admin user " + currentUser.username + " attempted admin action: displayAllAccountsAdmin", "WARNING");
            return;
        }
        clearScreen();
        printMessage("--- ALL SYSTEM ACCOUNTS (ADMIN VIEW) ---", ConsoleColors::HEADER);
        if (allAccounts.empty()) {
            printMessage("No accounts in the system.", ConsoleColors::SYSTEM_INFO);
            return;
        }
        for (const auto& acc : allAccounts) {
            acc->displayAccountDetails();
            std::cout << "-------------------------" << std::endl;
        }
    }

    void displayAllUsersAdmin() const {
        if (!isCurrentUserAdmin()) {
            printMessage("Access Denied. Admin only.", ConsoleColors::ERROR_MSG);
            Logger::getInstance()->log("Non-admin user " + currentUser.username + " attempted admin action: displayAllUsersAdmin", "WARNING");
            return;
        }
        clearScreen();
        printMessage("--- ALL SYSTEM USERS (ADMIN VIEW) ---", ConsoleColors::HEADER);
        if (users.empty()) {
            printMessage("No users in the system.", ConsoleColors::SYSTEM_INFO);
            return;
        }
        for (const auto& pair : users) {
            const User& u = pair.second;
            std::cout << ConsoleColors::ACCENT << "Username: " << ConsoleColors::RESET << u.username
                << (u.isAdmin ? " (Admin)" : " (User)") << std::endl;
            std::cout << ConsoleColors::ACCENT << "Status: " << ConsoleColors::RESET
                << (u.lockoutUntilTimestamp > 0 && DateTimeUtil::getCurrentTimeT() < u.lockoutUntilTimestamp ? "Locked until " + DateTimeUtil::timeTToString(u.lockoutUntilTimestamp) : "Active")
                << " (Failed attempts: " << u.failedLoginAttempts << ")" << std::endl;
            std::cout << "-------------------------" << std::endl;
        }
    }
};


void displayLoginMenu() {
    clearScreen();
    std::cout << ConsoleColors::HEADER << "****************************************" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::HEADER << "*        SECURE BANKING SYSTEM         *" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::HEADER << "****************************************" << ConsoleColors::RESET << std::endl;
    std::cout << std::endl;
    std::cout << ConsoleColors::ACCENT << "1. Login" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "2. Register New User" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "3. Exit" << ConsoleColors::RESET << std::endl;
    std::cout << std::endl;
    std::cout << ConsoleColors::PROMPT << "Please make a selection (1-3): " << ConsoleColors::RESET;
}

void displayMainMenu(bool isAdmin) {
    clearScreen();
    std::cout << ConsoleColors::HEADER << "****************************************" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::HEADER << "* MAIN BANKING MENU         *" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::HEADER << "****************************************" << ConsoleColors::RESET << std::endl;
    std::cout << std::endl;
    std::cout << ConsoleColors::ACCENT << "1. Create New Account" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "2. Deposit Funds" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "3. Withdraw Funds" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "4. Check Balance & Transaction History" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "5. Display My Accounts" << ConsoleColors::RESET << std::endl;
    std::cout << ConsoleColors::ACCENT << "6. Apply Interest to My Savings Accounts" << ConsoleColors::RESET << std::endl;
    if (isAdmin) {
        std::cout << ConsoleColors::SYSTEM_INFO << "7. View All System Accounts (Admin)" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::SYSTEM_INFO << "8. View All System Users (Admin)" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::ACCENT << "9. Logout" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::ACCENT << "10. Exit System" << ConsoleColors::RESET << std::endl;
        std::cout << std::endl;
        std::cout << ConsoleColors::PROMPT << "Please make a selection (1-10): " << ConsoleColors::RESET;
    }
    else {
        std::cout << ConsoleColors::ACCENT << "7. Logout" << ConsoleColors::RESET << std::endl;
        std::cout << ConsoleColors::ACCENT << "8. Exit System" << ConsoleColors::RESET << std::endl;
        std::cout << std::endl;
        std::cout << ConsoleColors::PROMPT << "Please make a selection (1-8): " << ConsoleColors::RESET;
    }
}

void displayAZD() {
    std::string bannerColor = ConsoleColors::HEADER;
    std::string resetColor = ConsoleColors::RESET;
    std::cout << "\n\n";
    std::cout << bannerColor << "    AAAAA     ZZZZZZZZZ   DDDDDD    " << resetColor << std::endl;
    std::cout << bannerColor << "   AA   AA         ZZ     DD   DD   " << resetColor << std::endl;
    std::cout << bannerColor << "  AA     AA       ZZ      DD    DD  " << resetColor << std::endl;
    std::cout << bannerColor << "  AAAAAAAAA      ZZ       DD    DD  " << resetColor << std::endl;
    std::cout << bannerColor << "  AA     AA     ZZ        DD   DD   " << resetColor << std::endl;
    std::cout << bannerColor << "  AA     AA    ZZZZZZZZZ  DDDDDD    " << resetColor << std::endl;
    std::cout << "\n\n" << resetColor;
}


int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    Bank bankSystem;
    int choice;

    while (true) {
        while (!bankSystem.isUserLoggedIn()) {
            displayLoginMenu();
            std::cin >> choice;
            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid input. Please enter a number.", ConsoleColors::ERROR_MSG);
                pressEnterToContinue();
                continue;
            }
            if (std::cin.peek() == '\n') {
                std::cin.ignore();
            }


            switch (choice) {
            case 1:
                bankSystem.login();
                pressEnterToContinue();
                break;
            case 2:
                bankSystem.registerUser();
                pressEnterToContinue();
                break;
            case 3:
                printMessage("Exiting system securely...", ConsoleColors::SYSTEM_INFO);
                displayAZD();
                return 0;
            default:
                printMessage("Invalid selection. Please try again.", ConsoleColors::ERROR_MSG);
                pressEnterToContinue();
            }
        }


        while (bankSystem.isUserLoggedIn()) {
            bool isAdmin = bankSystem.isCurrentUserAdmin();
            displayMainMenu(isAdmin);
            std::cin >> choice;

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                printMessage("Invalid input. Please enter a number.", ConsoleColors::ERROR_MSG);
                pressEnterToContinue();
                continue;
            }
            if (std::cin.peek() == '\n') {
                std::cin.ignore();
            }


            int logoutChoice = isAdmin ? 9 : 7;
            int exitChoice = isAdmin ? 10 : 8;

            if (isAdmin) {
                switch (choice) {
                case 1: bankSystem.createNewAccount(); break;
                case 2: bankSystem.performDeposit(); break;
                case 3: bankSystem.performWithdrawal(); break;
                case 4: bankSystem.checkBalanceAndHistory(); break;
                case 5: bankSystem.displayMyAccounts(); break;
                case 6: bankSystem.applyInterestToMySavings(); break;
                case 7: bankSystem.displayAllAccountsAdmin(); break;
                case 8: bankSystem.displayAllUsersAdmin(); break;
                case 9: bankSystem.logout(); break;
                case 10:
                    printMessage("Exiting system securely...", ConsoleColors::SYSTEM_INFO);
                    bankSystem.logout();
                    displayAZD();
                    return 0;
                default: printMessage("Invalid selection.", ConsoleColors::ERROR_MSG);
                }
            }
            else {
                switch (choice) {
                case 1: bankSystem.createNewAccount(); break;
                case 2: bankSystem.performDeposit(); break;
                case 3: bankSystem.performWithdrawal(); break;
                case 4: bankSystem.checkBalanceAndHistory(); break;
                case 5: bankSystem.displayMyAccounts(); break;
                case 6: bankSystem.applyInterestToMySavings(); break;
                case 7: bankSystem.logout(); break;
                case 8:
                    printMessage("Exiting system securely...", ConsoleColors::SYSTEM_INFO);
                    bankSystem.logout();
                    displayAZD();
                    return 0;
                default: printMessage("Invalid selection.", ConsoleColors::ERROR_MSG);
                }
            }

            if (choice == logoutChoice || choice == exitChoice) {
            }
            else if (bankSystem.isUserLoggedIn()) {
                pressEnterToContinue();
            }
        }
    }

    return 0;
}