#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cctype> // For tolower

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

const std::string DB_FILENAME = "minidb.csv";
int globalNextId = 1;

class Record {
private:
    int id_;
    std::string name_;
    std::string data_;

public:
    Record(int id, std::string name, std::string data)
        : id_(id), name_(std::move(name)), data_(std::move(data)) {
    }

    int getID() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getData() const { return data_; }

    void setName(std::string name) { name_ = std::move(name); }
    void setData(std::string data) { data_ = std::move(data); }

    static std::string escapeCSV(const std::string& s) {
        std::string escaped = s;
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"\"");
            pos += 2;
        }
        if (escaped.find(',') != std::string::npos || escaped.find('"') != std::string::npos || escaped.find('\n') != std::string::npos) {
            escaped = "\"" + escaped + "\"";
        }
        return escaped;
    }

    static std::string unescapeCSV(const std::string& s) {
        if (s.length() >= 2 && s.front() == '"' && s.back() == '"') {
            std::string unescaped = s.substr(1, s.length() - 2);
            size_t pos = 0;
            while ((pos = unescaped.find("\"\"", pos)) != std::string::npos) {
                unescaped.replace(pos, 2, "\"");
                pos += 1;
            }
            return unescaped;
        }
        return s;
    }
};

void clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printHeader(const std::string& title) {
    std::cout << BLUE << BOLD << "\n=== " << title << " ===\n" << RESET << std::endl;
}

void printRecordDetail(const Record& record) {
    std::cout << CYAN << "----------------------------------------\n"
        << "ID   : " << BOLD << record.getID() << RESET << CYAN << "\n"
        << "Name : " << record.getName() << "\n"
        << "Data : " << record.getData() << "\n"
        << "----------------------------------------" << RESET << std::endl;
}

int findRecordIndexById(const std::vector<Record>& db, int id) {
    for (size_t i = 0; i < db.size(); ++i) {
        if (db[i].getID() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool loadFromFile(std::vector<Record>& db, const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cerr << YELLOW << "Warning: Database file '" << filename << "' not found. Starting fresh." << RESET << std::endl;
        return false;
    }

    db.clear();
    globalNextId = 1;
    int maxId = 0;
    std::string line;

    while (std::getline(inFile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> fields;
        char delimiter = ',';

        while (std::getline(ss, segment, delimiter)) {
            // Basic handling for quoted fields containing commas
            if (!segment.empty() && segment.front() == '"') {
                while (segment.back() != '"' && ss.good()) {
                    std::string next_segment;
                    std::getline(ss, next_segment, delimiter);
                    segment += delimiter + next_segment;
                }
            }
            fields.push_back(Record::unescapeCSV(segment));
        }


        if (fields.size() == 3) {
            try {
                int id = std::stoi(fields[0]);
                db.emplace_back(id, fields[1], fields[2]);
                if (id > maxId) {
                    maxId = id;
                }
            }
            catch (const std::invalid_argument& e) {
                std::cerr << RED << "Error parsing line (invalid ID): " << line << RESET << std::endl;
            }
            catch (const std::out_of_range& e) {
                std::cerr << RED << "Error parsing line (ID out of range): " << line << RESET << std::endl;
            }
        }
        else {
            std::cerr << RED << "Error parsing line (incorrect field count): " << line << RESET << std::endl;
        }
    }

    globalNextId = maxId + 1;
    inFile.close();
    std::cout << GREEN << "Database loaded successfully from " << filename << "." << RESET << std::endl;
    return true;
}

bool saveToFile(const std::vector<Record>& db, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << RED << "Error: Could not open file '" << filename << "' for saving." << RESET << std::endl;
        return false;
    }

    for (const auto& record : db) {
        outFile << record.getID() << ","
            << Record::escapeCSV(record.getName()) << ","
            << Record::escapeCSV(record.getData()) << "\n";
    }

    outFile.close();
    if (!outFile) {
        std::cerr << RED << "Error: Failed to write data completely to '" << filename << "'." << RESET << std::endl;
        return false;
    }
    std::cout << GREEN << "Database saved successfully to " << filename << "." << RESET << std::endl;
    return true;
}


void addRecord(std::vector<Record>& db) {
    printHeader("Add New Record");
    int newId = globalNextId++;

    std::string name;
    std::cout << YELLOW << "Enter Name: " << RESET;
    std::getline(std::cin >> std::ws, name);

    std::string data;
    std::cout << YELLOW << "Enter Data: " << RESET;
    std::getline(std::cin >> std::ws, data);

    db.emplace_back(newId, name, data);
    std::cout << GREEN << BOLD << "\nRecord added successfully with ID: " << newId << RESET << std::endl;
}

void viewRecords(const std::vector<Record>& db, bool sorted = false, bool byName = false) {
    printHeader(sorted ? (byName ? "View Records (Sorted by Name)" : "View Records (Sorted by ID)") : "View All Records");
    if (db.empty()) {
        std::cout << YELLOW << "Database is empty." << RESET << std::endl;
        return;
    }

    std::vector<Record> viewDb = db;

    if (sorted) {
        if (byName) {
            std::sort(viewDb.begin(), viewDb.end(), [](const Record& a, const Record& b) {
                std::string nameA = a.getName();
                std::string nameB = b.getName();
                std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
                std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
                return nameA < nameB;
                });
        }
        else {
            std::sort(viewDb.begin(), viewDb.end(), [](const Record& a, const Record& b) {
                return a.getID() < b.getID();
                });
        }
    }


    for (const auto& record : viewDb) {
        printRecordDetail(record);
    }
}

void searchRecords(const std::vector<Record>& db) {
    printHeader("Search Records by Name");
    if (db.empty()) {
        std::cout << YELLOW << "Database is empty. Cannot search." << RESET << std::endl;
        return;
    }
    std::string searchTerm;
    std::cout << YELLOW << "Enter search term (case-insensitive): " << RESET;
    std::getline(std::cin >> std::ws, searchTerm);

    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);

    int count = 0;
    std::cout << "\nSearch Results:" << std::endl;
    for (const auto& record : db) {
        std::string lowerName = record.getName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerSearchTerm) != std::string::npos) {
            printRecordDetail(record);
            count++;
        }
    }

    if (count == 0) {
        std::cout << YELLOW << "No records found matching '" << searchTerm << "'." << RESET << std::endl;
    }
    else {
        std::cout << GREEN << count << " record(s) found." << RESET << std::endl;
    }
}


void updateRecord(std::vector<Record>& db) {
    printHeader("Update Record");
    if (db.empty()) {
        std::cout << YELLOW << "Database is empty. Cannot update." << RESET << std::endl;
        return;
    }

    int idToUpdate;
    std::cout << YELLOW << "Enter ID of record to update: " << RESET;
    while (!(std::cin >> idToUpdate)) {
        std::cout << RED << "Invalid input. Please enter a number: " << RESET;
        std::cin.clear();
        clearInputBuffer();
    }
    clearInputBuffer();

    int index = findRecordIndexById(db, idToUpdate);

    if (index != -1) {
        std::cout << "\nRecord Found:" << std::endl;
        printRecordDetail(db[index]);

        std::string currentName = db[index].getName();
        std::string currentData = db[index].getData();

        std::cout << YELLOW << "Enter new Name (leave blank to keep current '" << currentName << "'): " << RESET;
        std::string newName;
        std::getline(std::cin, newName);

        std::cout << YELLOW << "Enter new Data (leave blank to keep current '" << currentData << "'): " << RESET;
        std::string newData;
        std::getline(std::cin, newData);

        bool updated = false;
        if (!newName.empty()) {
            db[index].setName(newName);
            updated = true;
        }
        if (!newData.empty()) {
            db[index].setData(newData);
            updated = true;
        }

        if (updated) {
            std::cout << GREEN << BOLD << "\nRecord updated successfully." << RESET << std::endl;
            printRecordDetail(db[index]);
        }
        else {
            std::cout << YELLOW << "\nNo changes made to the record." << RESET << std::endl;
        }

    }
    else {
        std::cout << RED << "Record with ID " << idToUpdate << " not found." << RESET << std::endl;
    }
}

void deleteRecord(std::vector<Record>& db) {
    printHeader("Delete Record");
    if (db.empty()) {
        std::cout << YELLOW << "Database is empty. Cannot delete." << RESET << std::endl;
        return;
    }

    int idToDelete;
    std::cout << YELLOW << "Enter ID of record to delete: " << RESET;
    while (!(std::cin >> idToDelete)) {
        std::cout << RED << "Invalid input. Please enter a number: " << RESET;
        std::cin.clear();
        clearInputBuffer();
    }
    clearInputBuffer();

    int index = findRecordIndexById(db, idToDelete);

    if (index != -1) {
        std::cout << "\nRecord to be deleted:" << std::endl;
        printRecordDetail(db[index]);

        char confirmation;
        std::cout << YELLOW << "Are you sure you want to delete this record? (y/N): " << RESET;
        std::cin >> confirmation;
        clearInputBuffer();

        if (confirmation == 'y' || confirmation == 'Y') {
            db.erase(db.begin() + index);
            std::cout << GREEN << BOLD << "\nRecord deleted successfully." << RESET << std::endl;
        }
        else {
            std::cout << YELLOW << "\nDeletion cancelled." << RESET << std::endl;
        }
    }
    else {
        std::cout << RED << "Record with ID " << idToDelete << " not found." << RESET << std::endl;
    }
}

void displayMenu() {
    std::cout << MAGENTA << BOLD << "\n--- Advanced Mini Database Menu ---\n" << RESET;
    std::cout << CYAN << " 1. Add Record\n";
    std::cout << CYAN << " 2. View All Records\n";
    std::cout << CYAN << " 3. View Records (Sorted by ID)\n";
    std::cout << CYAN << " 4. View Records (Sorted by Name)\n";
    std::cout << CYAN << " 5. Search Records (by Name)\n";
    std::cout << CYAN << " 6. Update Record\n";
    std::cout << CYAN << " 7. Delete Record\n";
    std::cout << CYAN << " 8. Save Database\n";
    std::cout << CYAN << " 9. Exit\n" << RESET;
    std::cout << YELLOW << "Enter your choice (1-9): " << RESET;
}

void displayAzD() {
    // Sadece büyük harflerle, kalýn ve renkli "AZD" yazdýrýr
    std::cout << "\n\n" << BOLD << MAGENTA << "**********" << std::endl;
    std::cout << BOLD << MAGENTA << " * AZD  *" << std::endl;
    std::cout << BOLD << MAGENTA << "**********" << RESET << std::endl;
}


int main() {
    std::vector<Record> database;
    int choice;

#ifdef _WIN32
    system("chcp 65001 > nul"); // Set console to UTF-8 without output
    system("cls");
#else
    // Assuming TERM supports colors, clear works
    system("clear");
#endif

    loadFromFile(database, DB_FILENAME);


    do {
        displayMenu();

        while (!(std::cin >> choice) || choice < 1 || choice > 9) {
            std::cout << RED << "Invalid input. Please enter a number between 1 and 9: " << RESET;
            std::cin.clear();
            clearInputBuffer();
            displayMenu();
        }
        clearInputBuffer();

        bool needsPause = true;

        switch (choice) {
        case 1:
            addRecord(database);
            break;
        case 2:
            viewRecords(database, false);
            break;
        case 3:
            viewRecords(database, true, false); // Sorted by ID
            break;
        case 4:
            viewRecords(database, true, true); // Sorted by Name
            break;
        case 5:
            searchRecords(database);
            break;
        case 6:
            updateRecord(database);
            break;
        case 7:
            deleteRecord(database);
            break;
        case 8:
            if (saveToFile(database, DB_FILENAME)) {
                // Success message already printed by saveToFile
            }
            else {
                // Error message already printed by saveToFile
            }
            break;
        case 9:
            std::cout << GREEN << "\nAttempting to save database before exiting..." << RESET << std::endl;
            saveToFile(database, DB_FILENAME);
            std::cout << GREEN << "Exiting program." << RESET << std::endl;
            needsPause = false;
            break;
        default:
            std::cout << RED << "Invalid choice." << RESET << std::endl; // Should not happen due to input validation
            break;
        }

        if (needsPause) {
            std::cout << YELLOW << "\nPress Enter to continue..." << RESET;
            std::cin.get();
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
        }

    } while (choice != 9);

    displayAzD();

    return 0;
}