#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <sstream>
#include <map>
#include <cctype>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

const int LOAN_PERIOD_DAYS = 14;
const int MAX_BORROW_LIMIT = 5;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader(const std::string& title) {
    clearScreen();
    const int headerWidth = 60;
    std::cout << BLUE << BOLD << std::string(headerWidth, '=') << RESET << std::endl;
    int padding = (headerWidth - static_cast<int>(title.length())) / 2;
    padding = std::max(0, padding);
    std::cout << BLUE << BOLD;
    std::cout << std::string(padding, ' ');
    std::cout << title;
    std::cout << RESET << std::endl;
    std::cout << BLUE << BOLD << std::string(headerWidth, '=') << RESET << std::endl << std::endl;
}


std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return s;
}

template <typename T>
T getInput(const std::string& prompt) {
    T value;
    while (true) {
        std::cout << YELLOW << prompt << RESET;
        std::cin >> value;
        if (std::cin.fail() || std::cin.peek() != '\n') {
            std::cout << RED << "Invalid input. Please try again." << RESET << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
    }
}

std::string getStringInput(const std::string& prompt) {
    std::string value;
    while (true) {
        std::cout << YELLOW << prompt << RESET;
        std::getline(std::cin >> std::ws, value);
        if (!value.empty()) {
            return value;
        }
        std::cout << RED << "Input cannot be empty. Please try again." << RESET << std::endl;
    }
}

std::string formatTime(const std::time_t& time) {
    char buffer[80];
    struct tm timeinfo;
#ifdef _WIN32
    errno_t err = localtime_s(&timeinfo, &time);
    if (err == 0) {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return std::string(buffer);
    }
    else {
        return "ErrorTime";
    }
#else
    struct tm* timeinfo_ptr = localtime_r(&time, &timeinfo);
    if (timeinfo_ptr != nullptr) {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return std::string(buffer);
    }
    else {
        struct tm* unsafe_timeinfo = std::localtime(&time);
        if (unsafe_timeinfo) {
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", unsafe_timeinfo);
            return std::string(buffer);
        }
        else {
            return "ErrorTime";
        }
    }
#endif
}


class Book {
private:
    long long isbn;
    std::string title;
    std::string author;
    std::string genre;
    int publicationYear;
    bool isAvailable;
    static long long nextIsbn;

public:
    Book(std::string t, std::string a, std::string g, int year)
        : title(t), author(a), genre(g), publicationYear(year), isAvailable(true) {
        isbn = nextIsbn++;
    }

    Book(long long existingIsbn, std::string t, std::string a, std::string g, int year, bool available)
        : isbn(existingIsbn), title(t), author(a), genre(g), publicationYear(year), isAvailable(available) {
        if (existingIsbn >= nextIsbn) {
            nextIsbn = existingIsbn + 1;
        }
    }

    long long getIsbn() const { return isbn; }
    std::string getTitle() const { return title; }
    std::string getAuthor() const { return author; }
    std::string getGenre() const { return genre; }
    int getPublicationYear() const { return publicationYear; }
    bool getAvailability() const { return isAvailable; }
    void setAvailability(bool status) { isAvailable = status; }

    void display() const {
        std::cout << CYAN << std::left << std::setw(15) << isbn
            << std::setw(35) << (title.length() > 33 ? title.substr(0, 30) + "..." : title)
            << std::setw(25) << (author.length() > 23 ? author.substr(0, 20) + "..." : author)
            << std::setw(15) << (genre.length() > 13 ? genre.substr(0, 10) + "..." : genre)
            << std::setw(10) << publicationYear
            << std::setw(15);
        if (isAvailable) {
            std::cout << GREEN << "Available";
        }
        else {
            std::cout << RED << "On Loan";
        }
        std::cout << RESET << std::endl;
    }

    static void setNextIsbn(long long isbn) {
        nextIsbn = isbn;
    }

    std::string serialize() const {
        std::stringstream ss;
        const char DELIM = '|';
        ss << isbn << DELIM << title << DELIM << author << DELIM << genre << DELIM << publicationYear << DELIM << (isAvailable ? 1 : 0);
        return ss.str();
    }

    static Book deserialize(const std::string& data) {
        const char DELIM = '|';
        std::stringstream ss(data);
        std::string segment;
        std::vector<std::string> seglist;

        while (std::getline(ss, segment, DELIM)) {
            seglist.push_back(segment);
        }

        if (seglist.size() != 6) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing book data: invalid format. Expected 6 fields. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }

        try {
            long long isbn = std::stoll(seglist[0]);
            std::string title = seglist[1];
            std::string author = seglist[2];
            std::string genre = seglist[3];
            int year = std::stoi(seglist[4]);
            bool available = std::stoi(seglist[5]) == 1;
            return Book(isbn, title, author, genre, year, available);
        }
        catch (const std::invalid_argument& e) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing book data: invalid number format. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }
        catch (const std::out_of_range& e) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing book data: number out of range. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }
    }
};
long long Book::nextIsbn = 1000000000001;

class Member {
private:
    int memberId;
    std::string name;
    std::string contactInfo;
    std::map<long long, std::time_t> borrowedBooks;
    static int nextMemberId;

public:
    Member(std::string n, std::string contact)
        : name(n), contactInfo(contact) {
        memberId = nextMemberId++;
    }

    Member(int existingId, std::string n, std::string contact, const std::map<long long, std::time_t>& borrowed)
        : memberId(existingId), name(n), contactInfo(contact), borrowedBooks(borrowed) {
        if (existingId >= nextMemberId) {
            nextMemberId = existingId + 1;
        }
    }

    int getMemberId() const { return memberId; }
    std::string getName() const { return name; }
    std::string getContactInfo() const { return contactInfo; }
    const std::map<long long, std::time_t>& getBorrowedBooks() const { return borrowedBooks; }
    size_t getBorrowedCount() const { return borrowedBooks.size(); }

    void borrowBook(long long isbn, std::time_t dueDate) {
        borrowedBooks[isbn] = dueDate;
    }

    bool returnBook(long long isbn) {
        return borrowedBooks.erase(isbn) > 0;
    }

    void display() const {
        std::cout << CYAN << std::left << std::setw(10) << memberId
            << std::setw(30) << (name.length() > 28 ? name.substr(0, 25) + "..." : name)
            << std::setw(30) << (contactInfo.length() > 28 ? contactInfo.substr(0, 25) + "..." : contactInfo) << RESET << std::endl;
        if (!borrowedBooks.empty()) {
            std::cout << MAGENTA << "  Borrowed Books (ISBN: Due Date): ";
            bool first = true;
            for (const auto& pair : borrowedBooks) {
                if (!first) std::cout << ", ";
                std::cout << pair.first << ":" << formatTime(pair.second);
                first = false;
            }
            std::cout << RESET << std::endl;
        }
    }

    static void setNextMemberId(int id) {
        nextMemberId = id;
    }

    std::string serialize() const {
        const char DELIM = '|';
        const char MAP_DELIM = ';';
        const char PAIR_DELIM = ':';
        std::stringstream ss;
        ss << memberId << DELIM << name << DELIM << contactInfo;
        if (!borrowedBooks.empty()) {
            ss << DELIM;
            bool first = true;
            for (const auto& pair : borrowedBooks) {
                if (!first) ss << MAP_DELIM;
                ss << pair.first << PAIR_DELIM << static_cast<long long>(pair.second);
                first = false;
            }
        }
        return ss.str();
    }

    static Member deserialize(const std::string& data) {
        const char DELIM = '|';
        const char MAP_DELIM = ';';
        const char PAIR_DELIM = ':';
        std::stringstream ss(data);
        std::string segment;
        std::vector<std::string> seglist;

        while (std::getline(ss, segment, DELIM)) {
            seglist.push_back(segment);
        }

        if (seglist.size() < 3 || seglist.size() > 4) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing member data: invalid number of segments. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }

        try {
            int id = std::stoi(seglist[0]);
            std::string name = seglist[1];
            std::string contact = seglist[2];
            std::map<long long, std::time_t> borrowed;

            if (seglist.size() == 4 && !seglist[3].empty()) {
                std::stringstream ss_map(seglist[3]);
                std::string pair_str;
                while (std::getline(ss_map, pair_str, MAP_DELIM)) {
                    std::stringstream ss_pair(pair_str);
                    std::string key_str, value_str;
                    if (std::getline(ss_pair, key_str, PAIR_DELIM) && std::getline(ss_pair, value_str)) {
                        borrowed[std::stoll(key_str)] = static_cast<std::time_t>(std::stoll(value_str));
                    }
                    else if (!pair_str.empty()) {
                        std::stringstream errorMsg;
                        errorMsg << "Error parsing member data: invalid borrowed book pair format. Pair: " << pair_str;
                        throw std::runtime_error(errorMsg.str());
                    }
                }
            }

            return Member(id, name, contact, borrowed);
        }
        catch (const std::invalid_argument& e) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing member data: invalid number format. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }
        catch (const std::out_of_range& e) {
            std::stringstream errorMsg;
            errorMsg << "Error parsing member data: number out of range. Data: " << data;
            throw std::runtime_error(errorMsg.str());
        }
    }
};
int Member::nextMemberId = 1001;

class Library {
private:
    std::vector<Book> books;
    std::vector<Member> members;
    std::string booksFilename = "library_books_v3.dat";
    std::string membersFilename = "library_members_v3.dat";

    void loadBooks() {
        std::ifstream inFile(booksFilename);
        if (!inFile) {
            std::cerr << YELLOW << "Warning: Book data file (" << booksFilename << ") not found or could not be opened. A new file will be created on save." << RESET << std::endl;
            return;
        }
        std::string line;
        long long maxIsbn = 1000000000000;
        int count = 0;
        int lineNum = 0;
        while (std::getline(inFile, line)) {
            lineNum++;
            try {
                if (!line.empty() && line.find_first_not_of(" \t\n\v\f\r") != std::string::npos) {
                    Book book = Book::deserialize(line);
                    books.push_back(book);
                    if (book.getIsbn() > maxIsbn) {
                        maxIsbn = book.getIsbn();
                    }
                    count++;
                }
            }
            catch (const std::exception& e) {
                std::cerr << RED << "Error loading book on line " << lineNum << ": " << e.what() << RESET << std::endl;
            }
        }
        Book::setNextIsbn(maxIsbn + 1);
        inFile.close();
        if (count > 0) {
            std::cout << GREEN << count << " books loaded successfully." << RESET << std::endl;
        }
    }

    void saveBooks() const {
        std::ofstream outFile(booksFilename);
        if (!outFile) {
            std::cerr << RED << "Error: Could not open book data file (" << booksFilename << ") for saving." << RESET << std::endl;
            return;
        }
        for (const auto& book : books) {
            outFile << book.serialize() << std::endl;
        }
        outFile.close();
    }

    void loadMembers() {
        std::ifstream inFile(membersFilename);
        if (!inFile) {
            std::cerr << YELLOW << "Warning: Member data file (" << membersFilename << ") not found or could not be opened. A new file will be created on save." << RESET << std::endl;
            return;
        }
        std::string line;
        int maxId = 1000;
        int count = 0;
        int lineNum = 0;
        while (std::getline(inFile, line)) {
            lineNum++;
            try {
                if (!line.empty() && line.find_first_not_of(" \t\n\v\f\r") != std::string::npos) {
                    Member member = Member::deserialize(line);
                    members.push_back(member);
                    if (member.getMemberId() > maxId) {
                        maxId = member.getMemberId();
                    }
                    count++;
                }
            }
            catch (const std::exception& e) {
                std::cerr << RED << "Error loading member on line " << lineNum << ": " << e.what() << RESET << std::endl;
            }
        }
        Member::setNextMemberId(maxId + 1);
        inFile.close();
        if (count > 0) {
            std::cout << GREEN << count << " members loaded successfully." << RESET << std::endl;
        }
    }

    void saveMembers() const {
        std::ofstream outFile(membersFilename);
        if (!outFile) {
            std::cerr << RED << "Error: Could not open member data file (" << membersFilename << ") for saving." << RESET << std::endl;
            return;
        }
        for (const auto& member : members) {
            outFile << member.serialize() << std::endl;
        }
        outFile.close();
    }

    Book* findBookByIsbn(long long isbn) {
        auto it = std::find_if(books.begin(), books.end(),
            [isbn](const Book& b) { return b.getIsbn() == isbn; });
        return (it != books.end()) ? &(*it) : nullptr;
    }

    Member* findMemberById(int id) {
        auto it = std::find_if(members.begin(), members.end(),
            [id](const Member& m) { return m.getMemberId() == id; });
        return (it != members.end()) ? &(*it) : nullptr;
    }

public:
    Library() {
        loadData();
    }

    ~Library() {
        saveData();
    }

    void loadData() {
        loadBooks();
        loadMembers();
        std::cout << YELLOW << "\nData loading complete. Press Enter to continue..." << RESET;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void saveData() const {
        saveBooks();
        saveMembers();
        std::cout << GREEN << "\nData saved successfully." << RESET << std::endl;
    }

    void addBook() {
        printHeader("Add New Book");
        std::string title = getStringInput("Book Title: ");
        std::string author = getStringInput("Author: ");
        std::string genre = getStringInput("Genre: ");
        int year = getInput<int>("Publication Year: ");

        books.emplace_back(title, author, genre, year);
        std::cout << GREEN << "\nBook '" << title << "' added successfully. ISBN: " << books.back().getIsbn() << RESET << std::endl;
        saveBooks();
    }

    void addMember() {
        printHeader("Add New Member");
        std::string name = getStringInput("Member Name: ");
        std::string contact = getStringInput("Contact Info (Email/Phone): ");

        members.emplace_back(name, contact);
        std::cout << GREEN << "\nMember '" << name << "' added successfully. Member ID: " << members.back().getMemberId() << RESET << std::endl;
        saveMembers();
    }

    void displayAllBooks(bool sort = false, const std::string& sortBy = "title") {
        printHeader("All Library Books");
        if (books.empty()) {
            std::cout << YELLOW << "There are no books in the library." << RESET << std::endl;
            return;
        }

        std::vector<Book> displayBooks = books;

        if (sort) {
            if (sortBy == "title") {
                std::sort(displayBooks.begin(), displayBooks.end(), [](const Book& a, const Book& b) {
                    return toLower(a.getTitle()) < toLower(b.getTitle());
                    });
                std::cout << MAGENTA << "(Sorted by Title)" << RESET << std::endl;
            }
            else if (sortBy == "author") {
                std::sort(displayBooks.begin(), displayBooks.end(), [](const Book& a, const Book& b) {
                    return toLower(a.getAuthor()) < toLower(b.getAuthor());
                    });
                std::cout << MAGENTA << "(Sorted by Author)" << RESET << std::endl;
            }
            else if (sortBy == "isbn") {
                std::sort(displayBooks.begin(), displayBooks.end(), [](const Book& a, const Book& b) {
                    return a.getIsbn() < b.getIsbn();
                    });
                std::cout << MAGENTA << "(Sorted by ISBN)" << RESET << std::endl;
            }
        }
        else {
            std::cout << MAGENTA << "(Unsorted - Default Order)" << RESET << std::endl;
        }


        std::cout << BOLD << MAGENTA << std::left
            << std::setw(15) << "ISBN"
            << std::setw(35) << "Title"
            << std::setw(25) << "Author"
            << std::setw(15) << "Genre"
            << std::setw(10) << "Year"
            << std::setw(15) << "Status" << RESET << std::endl;
        std::cout << MAGENTA << std::string(115, '-') << RESET << std::endl;

        for (const auto& book : displayBooks) {
            book.display();
        }
        std::cout << MAGENTA << std::string(115, '-') << RESET << std::endl;
    }

    void displayAllMembers(bool sort = false, const std::string& sortBy = "name") {
        printHeader("All Library Members");
        if (members.empty()) {
            std::cout << YELLOW << "There are no registered members." << RESET << std::endl;
            return;
        }

        std::vector<Member> displayMembers = members;

        if (sort) {
            if (sortBy == "name") {
                std::sort(displayMembers.begin(), displayMembers.end(), [](const Member& a, const Member& b) {
                    return toLower(a.getName()) < toLower(b.getName());
                    });
                std::cout << MAGENTA << "(Sorted by Name)" << RESET << std::endl;
            }
            else if (sortBy == "id") {
                std::sort(displayMembers.begin(), displayMembers.end(), [](const Member& a, const Member& b) {
                    return a.getMemberId() < b.getMemberId();
                    });
                std::cout << MAGENTA << "(Sorted by ID)" << RESET << std::endl;
            }
        }
        else {
            std::cout << MAGENTA << "(Unsorted - Default Order)" << RESET << std::endl;
        }


        std::cout << BOLD << MAGENTA << std::left
            << std::setw(10) << "ID"
            << std::setw(30) << "Name"
            << std::setw(30) << "Contact Info" << RESET << std::endl;
        std::cout << MAGENTA << std::string(70, '-') << RESET << std::endl;

        for (const auto& member : displayMembers) {
            member.display();
            std::cout << MAGENTA << std::string(70, '-') << RESET << std::endl;
        }
    }

    void lendBook() {
        printHeader("Lend Book");
        int memberId = getInput<int>("Enter Member ID: ");
        Member* member = findMemberById(memberId);

        if (!member) {
            std::cout << RED << "Member with ID " << memberId << " not found." << RESET << std::endl;
            return;
        }

        if (member->getBorrowedCount() >= MAX_BORROW_LIMIT) {
            std::cout << RED << "Member '" << member->getName() << "' has reached the maximum borrow limit (" << MAX_BORROW_LIMIT << " books)." << RESET << std::endl;
            return;
        }


        long long isbn = getInput<long long>("Enter ISBN of the book to lend: ");
        Book* book = findBookByIsbn(isbn);

        if (!book) {
            std::cout << RED << "Book with ISBN " << isbn << " not found." << RESET << std::endl;
            return;
        }

        if (!book->getAvailability()) {
            std::cout << RED << "Book '" << book->getTitle() << "' is currently on loan." << RESET << std::endl;
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto dueDateChrono = now + std::chrono::hours(24 * LOAN_PERIOD_DAYS);
        std::time_t dueDate = std::chrono::system_clock::to_time_t(dueDateChrono);


        book->setAvailability(false);
        member->borrowBook(isbn, dueDate);
        std::cout << GREEN << "\nBook '" << book->getTitle() << "' successfully lent to member '" << member->getName() << "'." << RESET << std::endl;
        std::cout << YELLOW << "Due Date: " << formatTime(dueDate) << RESET << std::endl;
        saveData();
    }

    void returnBook() {
        printHeader("Return Book");
        long long isbn = getInput<long long>("Enter ISBN of the book to return: ");
        Book* book = findBookByIsbn(isbn);

        if (!book) {
            std::cout << RED << "Book with ISBN " << isbn << " not found." << RESET << std::endl;
            return;
        }

        if (book->getAvailability()) {
            std::cout << YELLOW << "Book '" << book->getTitle() << "' is already marked as available." << RESET << std::endl;
            return;
        }

        Member* returningMember = nullptr;
        std::time_t dueDate = 0;
        for (auto& member : members) {
            const auto& borrowedMap = member.getBorrowedBooks();
            auto it = borrowedMap.find(isbn);
            if (it != borrowedMap.end()) {
                returningMember = &member;
                dueDate = it->second;
                break;
            }
        }


        if (returningMember) {
            if (returningMember->returnBook(isbn)) {
                book->setAvailability(true);
                std::cout << GREEN << "\nBook '" << book->getTitle() << "' successfully returned by member '" << returningMember->getName() << "'." << RESET << std::endl;

                std::time_t now_t;
                std::time(&now_t);
                if (now_t > dueDate) {
                    double diff_seconds_double = std::difftime(now_t, dueDate);
                    long long diff_seconds_ll = static_cast<long long>(diff_seconds_double);
                    int days_overdue = static_cast<int>(diff_seconds_ll / (60 * 60 * 24));
                    if (days_overdue > 0) {
                        std::cout << RED << BOLD << "This book was " << days_overdue << " day(s) overdue!" << RESET << std::endl;
                    }
                }

                saveData();
            }
            else {
                std::cerr << RED << "Error: Book was marked as lent, but not found in member's borrowed list. Data inconsistency possible. ISBN: " << isbn << RESET << std::endl;
                book->setAvailability(true);
                saveBooks();
            }

        }
        else {
            std::cerr << RED << "Error: Could not find the member who borrowed this book (ISBN: " << isbn << "). Data inconsistency possible." << RESET << std::endl;
            book->setAvailability(true);
            saveBooks();
        }
    }

    void searchBook() {
        printHeader("Search Books");
        std::cout << "Search Criteria:\n";
        std::cout << "1. Title\n";
        std::cout << "2. Author\n";
        std::cout << "3. ISBN\n";
        std::cout << "4. Genre\n";
        int choice = getInput<int>("Your choice: ");

        std::string queryStr;
        long long queryIsbn = 0;
        std::vector<const Book*> results;

        switch (choice) {
        case 1:
            queryStr = getStringInput("Enter Title to search: ");
            for (const auto& book : books) {
                if (toLower(book.getTitle()).find(toLower(queryStr)) != std::string::npos) {
                    results.push_back(&book);
                }
            }
            break;
        case 2:
            queryStr = getStringInput("Enter Author to search: ");
            for (const auto& book : books) {
                if (toLower(book.getAuthor()).find(toLower(queryStr)) != std::string::npos) {
                    results.push_back(&book);
                }
            }
            break;
        case 3:
            queryIsbn = getInput<long long>("Enter ISBN to search: ");
            {
                Book* book = findBookByIsbn(queryIsbn);
                if (book) {
                    results.push_back(book);
                }
            }
            break;
        case 4:
            queryStr = getStringInput("Enter Genre to search: ");
            for (const auto& book : books) {
                if (toLower(book.getGenre()).find(toLower(queryStr)) != std::string::npos) {
                    results.push_back(&book);
                }
            }
            break;
        default:
            std::cout << RED << "Invalid choice." << RESET << std::endl;
            return;
        }

        if (results.empty()) {
            std::cout << YELLOW << "\nNo books found matching your criteria." << RESET << std::endl;
        }
        else {
            std::cout << "\n" << BOLD << GREEN << "Search Results (" << results.size() << "):" << RESET << std::endl;
            std::cout << BOLD << MAGENTA << std::left
                << std::setw(15) << "ISBN"
                << std::setw(35) << "Title"
                << std::setw(25) << "Author"
                << std::setw(15) << "Genre"
                << std::setw(10) << "Year"
                << std::setw(15) << "Status" << RESET << std::endl;
            std::cout << MAGENTA << std::string(115, '-') << RESET << std::endl;
            for (const auto* bookPtr : results) {
                bookPtr->display();
            }
            std::cout << MAGENTA << std::string(115, '-') << RESET << std::endl;
        }
    }

    void searchMember() {
        printHeader("Search Members");
        std::cout << "Search Criteria:\n";
        std::cout << "1. Name\n";
        std::cout << "2. Member ID\n";
        int choice = getInput<int>("Your choice: ");
        std::string queryStr;
        int queryId = 0;

        std::vector<const Member*> results;

        switch (choice) {
        case 1:
            queryStr = getStringInput("Enter Name to search: ");
            for (const auto& member : members) {
                if (toLower(member.getName()).find(toLower(queryStr)) != std::string::npos) {
                    results.push_back(&member);
                }
            }
            break;
        case 2:
            queryId = getInput<int>("Enter Member ID to search: ");
            {
                Member* member = findMemberById(queryId);
                if (member) {
                    results.push_back(member);
                }
            }
            break;
        default:
            std::cout << RED << "Invalid choice." << RESET << std::endl;
            return;
        }

        if (results.empty()) {
            std::cout << YELLOW << "\nNo members found matching your criteria." << RESET << std::endl;
        }
        else {
            std::cout << "\n" << BOLD << GREEN << "Search Results (" << results.size() << "):" << RESET << std::endl;
            std::cout << BOLD << MAGENTA << std::left
                << std::setw(10) << "ID"
                << std::setw(30) << "Name"
                << std::setw(30) << "Contact Info" << RESET << std::endl;
            std::cout << MAGENTA << std::string(70, '-') << RESET << std::endl;
            for (const auto* memberPtr : results) {
                memberPtr->display();
                std::cout << MAGENTA << std::string(70, '-') << RESET << std::endl;
            }
        }
    }

    void displayOverdueBooks() {
        printHeader("Overdue Books Report");
        bool foundOverdue = false;
        std::time_t now_t;
        std::time(&now_t);

        std::cout << BOLD << MAGENTA << std::left
            << std::setw(15) << "ISBN"
            << std::setw(35) << "Title"
            << std::setw(10) << "Member ID"
            << std::setw(25) << "Member Name"
            << std::setw(15) << "Due Date"
            << std::setw(10) << "Days Over" << RESET << std::endl;
        std::cout << MAGENTA << std::string(110, '-') << RESET << std::endl;


        for (const auto& member : members) {
            for (const auto& pair : member.getBorrowedBooks()) {
                long long isbn = pair.first;
                std::time_t dueDate = pair.second;

                if (now_t > dueDate) {
                    foundOverdue = true;
                    Book* book = findBookByIsbn(isbn);
                    double diff_seconds_double = std::difftime(now_t, dueDate);
                    long long diff_seconds_ll = static_cast<long long>(diff_seconds_double);
                    int days_overdue = static_cast<int>(diff_seconds_ll / (60 * 60 * 24));

                    std::cout << RED << std::left
                        << std::setw(15) << isbn
                        << std::setw(35) << (book ? (book->getTitle().length() > 33 ? book->getTitle().substr(0, 30) + "..." : book->getTitle()) : "N/A")
                        << std::setw(10) << member.getMemberId()
                        << std::setw(25) << (member.getName().length() > 23 ? member.getName().substr(0, 20) + "..." : member.getName())
                        << std::setw(15) << formatTime(dueDate)
                        << std::setw(10) << days_overdue << RESET << std::endl;
                }
            }
        }

        if (!foundOverdue) {
            std::cout << GREEN << "\nNo books are currently overdue." << RESET << std::endl;
        }
        else {
            std::cout << MAGENTA << std::string(110, '-') << RESET << std::endl;
        }
    }
};

void displayMainMenu() {
    std::cout << YELLOW << BOLD << "\n--- Main Menu ---" << RESET << std::endl;
    std::cout << CYAN << " 1. Add Book" << RESET << std::endl;
    std::cout << CYAN << " 2. Add Member" << RESET << std::endl;
    std::cout << CYAN << " 3. Display All Books (Sorted by Title)" << RESET << std::endl;
    std::cout << CYAN << " 4. Display All Books (Sorted by Author)" << RESET << std::endl;
    std::cout << CYAN << " 5. Display All Members (Sorted by Name)" << RESET << std::endl;
    std::cout << CYAN << " 6. Display All Members (Sorted by ID)" << RESET << std::endl;
    std::cout << CYAN << " 7. Lend Book" << RESET << std::endl;
    std::cout << CYAN << " 8. Return Book" << RESET << std::endl;
    std::cout << CYAN << " 9. Search Books" << RESET << std::endl;
    std::cout << CYAN << "10. Search Members" << RESET << std::endl;
    std::cout << CYAN << "11. Show Overdue Books" << RESET << std::endl;
    std::cout << RED << " 0. Exit" << RESET << std::endl;
}

void displayAZD() {
    std::cout << std::endl << std::endl;
    std::cout << BOLD << MAGENTA << R"(
    AAAAAAAAAAAAA     ZZZZZZZZZZZZZZZZZ     DDDDDDDDDDDDD
   A::::::::::::A    Z:::::::::::::::Z     D::::::::::::DDD
  A:::::::::::::A    Z::::::::::::::Z      D:::::::::::::::DD
 A:::::A     A:::::A   Z::::::ZZZZZ        D:::::DDDDD:::::D
A:::::A     A:::::A   ZZZZZ     Z          D:::::D    D:::::D
A:::::AAAAAAAAA:::::A        Z             D:::::D     D:::::D
A::::::::::::::::::::A      Z              D:::::D     D:::::D
A:::::AAAAAAAAA:::::A     Z                D:::::D     D:::::D
A:::::A     A:::::A    Z                 D:::::D     D:::::D
A:::::A     A:::::A   Z                  D:::::D     D:::::D
A:::::A     A:::::A ZZZZZZZZZZZ          D:::::DDDDD:::::D
A:::::A     A:::::A Z:::::::::::::Z      D:::::::::::::::DD
A:::::A     A:::::A Z::::::::::::::Z     D::::::::::::DDD
AAAAAAAAA     AAAAAAAAA ZZZZZZZZZZZZZZZZZ    DDDDDDDDDDDDD
)" << RESET << std::endl;
}

void pauseExecution() {
    std::cout << YELLOW << "\nPress Enter to return to the main menu..." << RESET;
    // Clear potential error states before waiting
    std::cin.clear();
    // Ignore any leftover characters in the buffer before waiting for Enter
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    // Wait for Enter
    std::cin.get();
}


int main() {
    Library librarySystem;
    int choice = -1;

    do {
        printHeader("Advanced Library Management System");
        displayMainMenu();
        choice = getInput<int>("Enter your choice: ");

        switch (choice) {
        case 1:  librarySystem.addBook(); break;
        case 2:  librarySystem.addMember(); break;
        case 3:  librarySystem.displayAllBooks(true, "title"); break;
        case 4:  librarySystem.displayAllBooks(true, "author"); break;
        case 5:  librarySystem.displayAllMembers(true, "name"); break;
        case 6:  librarySystem.displayAllMembers(true, "id"); break;
        case 7:  librarySystem.lendBook(); break;
        case 8:  librarySystem.returnBook(); break;
        case 9:  librarySystem.searchBook(); break;
        case 10: librarySystem.searchMember(); break;
        case 11: librarySystem.displayOverdueBooks(); break;
        case 0:  std::cout << YELLOW << "Exiting program..." << RESET << std::endl; break;
        default: std::cout << RED << "Invalid choice. Please try again." << RESET << std::endl; break;
        }

        if (choice != 0) {
            pauseExecution();
        }

    } while (choice != 0);


    displayAZD();

    return 0;
}