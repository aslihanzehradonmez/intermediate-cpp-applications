#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <cctype>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define FAINT   "\033[2m"
#define ITALIC  "\033[3m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[97m"
#define GRAY    "\033[90m"

#define BOX_HLINE "-"
#define BOX_VLINE "|"
#define BOX_TL    "+"
#define BOX_TR    "+"
#define BOX_BL    "+"
#define BOX_BR    "+"
#define BOX_TJ    "+"
#define BOX_BJ    "+"
#define BOX_LJ    "+"
#define BOX_RJ    "+"
#define BOX_CROSS "+"

const std::string DATA_FILE = "phonebook.dat";
const size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

void drawLine(int width, const std::string& style = BOX_HLINE) {
    for (int i = 0; i < width; ++i) {
        std::cout << style;
    }
    std::cout << std::endl;
}

class Contact {
private:
    std::string firstName;
    std::string lastName;
    std::string phoneNumber;
    std::string email;

public:
    Contact() = default;

    Contact(std::string fn, std::string ln, std::string pn, std::string em)
        : firstName(std::move(fn)), lastName(std::move(ln)),
        phoneNumber(std::move(pn)), email(std::move(em)) {
    }

    std::string getFirstName() const { return firstName; }
    std::string getLastName() const { return lastName; }
    std::string getFullName() const { return firstName + " " + lastName; }
    std::string getPhoneNumber() const { return phoneNumber; }
    std::string getEmail() const { return email; }

    void setFirstName(const std::string& fn) { firstName = fn; }
    void setLastName(const std::string& ln) { lastName = ln; }
    void setPhoneNumber(const std::string& pn) { phoneNumber = pn; }
    void setEmail(const std::string& em) { email = em; }

    void display(size_t index = INVALID_INDEX, int nameWidth = 25, int phoneWidth = 15, int emailWidth = 30) const {
        std::cout << GRAY << BOX_VLINE << RESET << " ";
        if (index != INVALID_INDEX) {
            std::cout << CYAN << std::setw(3) << std::left << index << RESET;
        }
        else {
            std::cout << std::setw(3) << " ";
        }
        std::cout << " " << GRAY << BOX_VLINE << RESET << " ";

        std::cout << WHITE << std::setw(nameWidth) << std::left << getFullName() << RESET;
        std::cout << " " << GRAY << BOX_VLINE << RESET << " ";

        std::cout << WHITE << std::setw(phoneWidth) << std::left << phoneNumber << RESET;
        std::cout << " " << GRAY << BOX_VLINE << RESET << " ";

        std::cout << WHITE << std::setw(emailWidth) << std::left << email << RESET;
        std::cout << " " << GRAY << BOX_VLINE << RESET << std::endl;
    }

    friend std::ostream& operator<<(std::ostream& os, const Contact& contact);
    friend std::istream& operator>>(std::istream& is, Contact& contact);
};

std::ostream& operator<<(std::ostream& os, const Contact& contact) {
    os << contact.firstName << '\n';
    os << contact.lastName << '\n';
    os << contact.phoneNumber << '\n';
    os << contact.email << '\n';
    return os;
}

std::istream& operator>>(std::istream& is, Contact& contact) {
    std::getline(is, contact.firstName);
    if (!is || contact.firstName.empty()) {
        if (!is.eof()) is.setstate(std::ios::failbit);
        return is;
    }
    std::getline(is, contact.lastName);
    std::getline(is, contact.phoneNumber);
    std::getline(is, contact.email);
    return is;
}

class Phonebook {
private:
    std::vector<Contact> contacts;
    const int idxWidth = 4;
    const int nameWidth = 25;
    const int phoneWidth = 15;
    const int emailWidth = 30;
    const int totalWidth = idxWidth + nameWidth + phoneWidth + emailWidth + 11;

    void clearInputBuffer() const {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string getNonEmptyInput(const std::string& prompt) const {
        std::string input;
        while (true) {
            std::cout << CYAN << BOLD << prompt << RESET << " ";
            std::getline(std::cin, input);
            input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
            input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!input.empty()) {
                return input;
            }
            std::cout << RED << BOLD << " Error: " << RESET << RED << "Input cannot be empty. Please try again." << RESET << std::endl;
        }
    }

    size_t getValidatedIndexInput(const std::string& prompt, size_t maxIndex) const {
        size_t index;
        long long input_value;

        if (maxIndex == 0) {
            std::cout << YELLOW << BOLD << " Warning: " << RESET << YELLOW << "The list is empty. No contacts to select." << RESET << std::endl;
            return INVALID_INDEX;
        }
        while (true) {
            std::cout << CYAN << BOLD << prompt << RESET << GRAY << " (0-" << (maxIndex - 1) << "): " << RESET;
            if (!(std::cin >> input_value)) {
                std::cout << RED << BOLD << " Error: " << RESET << RED << "Invalid input. Please enter a number." << RESET << std::endl;
                std::cin.clear();
                clearInputBuffer();
            }
            else if (input_value < 0) {
                std::cout << RED << BOLD << " Error: " << RESET << RED << "Invalid index. Index cannot be negative." << RESET << std::endl;
                clearInputBuffer();
            }
            else {
                index = static_cast<size_t>(input_value);
                if (index >= maxIndex) {
                    std::cout << RED << BOLD << " Error: " << RESET << RED << "Invalid index. Please enter a number between 0 and " << (maxIndex - 1) << "." << RESET << std::endl;
                    clearInputBuffer();
                }
                else {
                    clearInputBuffer();
                    return index;
                }
            }
        }
    }

    void sortContacts() {
        std::sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
            std::string lowerA_last = a.getLastName();
            std::string lowerB_last = b.getLastName();
            std::transform(lowerA_last.begin(), lowerA_last.end(), lowerA_last.begin(),
                [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerB_last.begin(), lowerB_last.end(), lowerB_last.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (lowerA_last != lowerB_last) {
                return lowerA_last < lowerB_last;
            }
            std::string lowerA_first = a.getFirstName();
            std::string lowerB_first = b.getFirstName();
            std::transform(lowerA_first.begin(), lowerA_first.end(), lowerA_first.begin(),
                [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerB_first.begin(), lowerB_first.end(), lowerB_first.begin(),
                [](unsigned char c) { return std::tolower(c); });
            return lowerA_first < lowerB_first;
            });
    }

    void displaySectionTitle(const std::string& title) const {
        std::cout << "\n" << BOLD << BLUE;
        std::cout << BOX_TL; drawLine(totalWidth - 2, BOX_HLINE); std::cout << BOX_TR << std::endl;
        std::cout << BOX_VLINE << std::setw(totalWidth - 1) << std::left << " " + title << BOX_VLINE << std::endl;
        std::cout << BOX_LJ; drawLine(totalWidth - 2, BOX_HLINE); std::cout << BOX_RJ << std::endl;
        std::cout << RESET;
    }

public:
    Phonebook() {
        loadContacts();
    }

    ~Phonebook() {
        saveContacts();
    }

    void clearScreen() const {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    void pause() const {
        std::cout << std::endl << GRAY << ITALIC << "Press Enter to continue..." << RESET;
        std::cin.get();
    }

    void loadContacts() {
        std::ifstream inFile(DATA_FILE);
        if (!inFile) {
            return;
        }
        Contact temp;
        while (inFile >> temp) {
            contacts.push_back(temp);
        }
        inFile.close();
        sortContacts();
    }

    void saveContacts() const {
        std::ofstream outFile(DATA_FILE);
        if (!outFile) {
            std::cerr << RED << BOLD << " Error: " << RESET << RED << "Could not open file " << DATA_FILE << " for writing." << RESET << std::endl;
            return;
        }
        for (const auto& contact : contacts) {
            outFile << contact;
        }
        outFile.close();
    }

    void addContact() {
        displaySectionTitle("Add New Contact");
        std::string fn = getNonEmptyInput(" First Name   :");
        std::string ln = getNonEmptyInput(" Last Name    :");
        std::string pn = getNonEmptyInput(" Phone Number :");
        std::string em = getNonEmptyInput(" Email Address:");
        contacts.emplace_back(fn, ln, pn, em);
        sortContacts();
        std::cout << GREEN << BOLD << "\n Info: " << RESET << GREEN << "Contact added successfully." << RESET << std::endl;
    }

    void displayHeader() const {
        std::cout << GRAY << BOX_TL;
        drawLine(idxWidth + 1, BOX_HLINE); std::cout << BOX_TJ;
        drawLine(nameWidth + 2, BOX_HLINE); std::cout << BOX_TJ;
        drawLine(phoneWidth + 2, BOX_HLINE); std::cout << BOX_TJ;
        drawLine(emailWidth + 2, BOX_HLINE); std::cout << BOX_TR << std::endl;

        std::cout << BOX_VLINE << RESET << BOLD << BLUE << " " << std::setw(idxWidth) << std::left << "Idx" << RESET << GRAY << BOX_VLINE << RESET;
        std::cout << BOLD << BLUE << " " << std::setw(nameWidth + 1) << std::left << "Name" << RESET << GRAY << BOX_VLINE << RESET;
        std::cout << BOLD << BLUE << " " << std::setw(phoneWidth + 1) << std::left << "Phone" << RESET << GRAY << BOX_VLINE << RESET;
        std::cout << BOLD << BLUE << " " << std::setw(emailWidth + 1) << std::left << "Email" << RESET << GRAY << BOX_VLINE << RESET << std::endl;

        std::cout << BOX_LJ;
        drawLine(idxWidth + 1, BOX_HLINE); std::cout << BOX_CROSS;
        drawLine(nameWidth + 2, BOX_HLINE); std::cout << BOX_CROSS;
        drawLine(phoneWidth + 2, BOX_HLINE); std::cout << BOX_CROSS;
        drawLine(emailWidth + 2, BOX_HLINE); std::cout << BOX_RJ << std::endl;
        std::cout << RESET;
    }

    void displayFooter() const {
        std::cout << GRAY << BOX_BL;
        drawLine(idxWidth + 1, BOX_HLINE); std::cout << BOX_BJ;
        drawLine(nameWidth + 2, BOX_HLINE); std::cout << BOX_BJ;
        drawLine(phoneWidth + 2, BOX_HLINE); std::cout << BOX_BJ;
        drawLine(emailWidth + 2, BOX_HLINE); std::cout << BOX_BR << std::endl;
        std::cout << RESET;
    }

    void displayAllContacts() const {
        displaySectionTitle("All Contacts");
        if (contacts.empty()) {
            std::cout << YELLOW << BOLD << " Info: " << RESET << YELLOW << "The phonebook is empty." << RESET << std::endl;
            return;
        }
        displayHeader();
        for (size_t i = 0; i < contacts.size(); ++i) {
            contacts[i].display(i, nameWidth, phoneWidth, emailWidth);
        }
        displayFooter();
    }

    void searchContacts() const {
        displaySectionTitle("Search Contacts");
        if (contacts.empty()) {
            std::cout << YELLOW << BOLD << " Warning: " << RESET << YELLOW << "Phonebook is empty. Cannot search." << RESET << std::endl;
            return;
        }
        std::string searchTerm = getNonEmptyInput(" Search term (name, phone, email):");
        std::vector<size_t> resultsIndices;
        std::string lowerSearchTerm = searchTerm;
        std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(),
            [](unsigned char c) { return std::tolower(c); });

        for (size_t i = 0; i < contacts.size(); ++i) {
            std::string lowerFullName = contacts[i].getFullName();
            std::string lowerPhone = contacts[i].getPhoneNumber();
            std::string lowerEmail = contacts[i].getEmail();
            std::transform(lowerFullName.begin(), lowerFullName.end(), lowerFullName.begin(),
                [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerPhone.begin(), lowerPhone.end(), lowerPhone.begin(),
                [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerEmail.begin(), lowerEmail.end(), lowerEmail.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (lowerFullName.find(lowerSearchTerm) != std::string::npos ||
                lowerPhone.find(lowerSearchTerm) != std::string::npos ||
                lowerEmail.find(lowerSearchTerm) != std::string::npos) {
                resultsIndices.push_back(i);
            }
        }

        if (resultsIndices.empty()) {
            std::cout << YELLOW << BOLD << "\n Info: " << RESET << YELLOW << "No contacts found matching '" << searchTerm << "'." << RESET << std::endl;
        }
        else {
            std::cout << GREEN << BOLD << "\n Search Results:" << RESET << std::endl;
            displayHeader();
            for (size_t index : resultsIndices) {
                contacts[index].display(index, nameWidth, phoneWidth, emailWidth);
            }
            displayFooter();
        }
    }

    void editContact() {
        displaySectionTitle("Edit Contact");
        if (contacts.empty()) {
            std::cout << YELLOW << BOLD << " Warning: " << RESET << YELLOW << "Phonebook is empty. Cannot edit." << RESET << std::endl;
            return;
        }
        displayAllContacts();
        size_t index = getValidatedIndexInput("\n Enter the index of the contact to edit", contacts.size());

        if (index == INVALID_INDEX) {
            std::cout << YELLOW << BOLD << " Info: " << RESET << YELLOW << "Edit operation cancelled or invalid index provided." << RESET << std::endl;
            return;
        }

        Contact& contactToEdit = contacts[index];
        std::cout << "\n" << BOLD << MAGENTA << " Editing Contact: " << RESET << contactToEdit.getFullName() << std::endl;
        std::cout << GRAY << ITALIC << " (Leave field empty to keep current value)" << RESET << std::endl;

        std::string fn, ln, pn, em;
        std::cout << YELLOW << " New First Name   [" << contactToEdit.getFirstName() << "]: " << RESET;
        std::getline(std::cin, fn);
        if (!fn.empty()) contactToEdit.setFirstName(fn);

        std::cout << YELLOW << " New Last Name    [" << contactToEdit.getLastName() << "]: " << RESET;
        std::getline(std::cin, ln);
        if (!ln.empty()) contactToEdit.setLastName(ln);

        std::cout << YELLOW << " New Phone Number [" << contactToEdit.getPhoneNumber() << "]: " << RESET;
        std::getline(std::cin, pn);
        if (!pn.empty()) contactToEdit.setPhoneNumber(pn);

        std::cout << YELLOW << " New Email Address[" << contactToEdit.getEmail() << "]: " << RESET;
        std::getline(std::cin, em);
        if (!em.empty()) contactToEdit.setEmail(em);

        sortContacts();
        std::cout << GREEN << BOLD << "\n Info: " << RESET << GREEN << "Contact updated successfully." << RESET << std::endl;
    }

    void deleteContact() {
        displaySectionTitle("Delete Contact");
        if (contacts.empty()) {
            std::cout << YELLOW << BOLD << " Warning: " << RESET << YELLOW << "Phonebook is empty. Cannot delete." << RESET << std::endl;
            return;
        }
        displayAllContacts();
        size_t index = getValidatedIndexInput("\n Enter the index of the contact to delete", contacts.size());

        if (index == INVALID_INDEX) {
            std::cout << YELLOW << BOLD << " Info: " << RESET << YELLOW << "Delete operation cancelled or invalid index provided." << RESET << std::endl;
            return;
        }

        std::cout << RED << BOLD << "\n Are you sure? " << RESET << "Delete contact '" << contacts[index].getFullName() << "'? (Y/N): " << RESET;
        char confirmation;
        std::cin >> confirmation;
        clearInputBuffer();

        if (confirmation == 'y' || confirmation == 'Y') {
            contacts.erase(contacts.begin() + index);
            std::cout << GREEN << BOLD << "\n Info: " << RESET << GREEN << "Contact deleted successfully." << RESET << std::endl;
        }
        else {
            std::cout << YELLOW << BOLD << "\n Info: " << RESET << YELLOW << "Deletion cancelled." << RESET << std::endl;
        }
    }

    void displayMenu() const {
        std::cout << BOLD << CYAN;
        std::cout << BOX_TL; drawLine(41, BOX_HLINE); std::cout << BOX_TR << std::endl;
        std::cout << BOX_VLINE << "              PHONEBOOK MENU               " << BOX_VLINE << std::endl;
        std::cout << BOX_LJ;  drawLine(41, BOX_HLINE); std::cout << BOX_RJ << std::endl;
        std::cout << BOX_VLINE << RESET << " " << GREEN << "1." << RESET << " Add New Contact                    " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_VLINE << RESET << " " << BLUE << "2." << RESET << " Display All Contacts               " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_VLINE << RESET << " " << CYAN << "3." << RESET << " Search Contacts                    " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_VLINE << RESET << " " << YELLOW << "4." << RESET << " Edit Contact                       " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_VLINE << RESET << " " << RED << "5." << RESET << " Delete Contact                     " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_VLINE << RESET << " " << GRAY << "6." << RESET << " Exit                               " << BOLD << CYAN << BOX_VLINE << std::endl;
        std::cout << BOX_BL;  drawLine(41, BOX_HLINE); std::cout << BOX_BR << std::endl;
        std::cout << RESET;
    }

    int getUserChoice() const {
        int choice;
        while (true) {
            std::cout << BOLD << MAGENTA << "\n Enter your choice (1-6): " << RESET;
            if (!(std::cin >> choice)) {
                std::cout << RED << BOLD << " Error: " << RESET << RED << "Invalid input. Please enter a number." << RESET << std::endl;
                std::cin.clear();
                clearInputBuffer();
            }
            else if (choice < 1 || choice > 6) {
                std::cout << RED << BOLD << " Error: " << RESET << RED << "Invalid choice. Please enter a number between 1 and 6." << RESET << std::endl;
                clearInputBuffer();
            }
            else {
                clearInputBuffer();
                return choice;
            }
        }
    }

    void displayFarewell() const {
        std::cout << BOLD << MAGENTA << "\n\n"
            << "        AAAAAAAAA   ZZZZZZZZZZZZZ   DDDDDDDDDD\n"
            << "       AAAA AAAAA        ZZZZZ      DDD    DDDD\n"
            << "      AAAA   AAAA       ZZZZZ       DDD     DDD\n"
            << "     AAAAAAAAAAAAA     ZZZZZ        DDD     DDD\n"
            << "    AAAA       AAAA   ZZZZZ         DDD    DDDD\n"
            << "   AAAA         AAAA ZZZZZZZZZZZZZ  DDDDDDDDDD\n"
            << RESET << std::endl;
    }
};

int main() {
    Phonebook phonebook;
    int choice;

    do {
        phonebook.clearScreen();
        phonebook.displayMenu();
        choice = phonebook.getUserChoice();
        phonebook.clearScreen();

        switch (choice) {
        case 1:
            phonebook.addContact();
            break;
        case 2:
            phonebook.displayAllContacts();
            break;
        case 3:
            phonebook.searchContacts();
            break;
        case 4:
            phonebook.editContact();
            break;
        case 5:
            phonebook.deleteContact();
            break;
        case 6:
            std::cout << GREEN << BOLD << "\n Info: " << RESET << GREEN << "Exiting Phonebook. Goodbye!" << RESET << std::endl;
            break;
        default:
            std::cout << RED << BOLD << " Error: " << RESET << RED << "An unexpected error occurred." << RESET << std::endl;
            break;
        }
        if (choice != 6) {
            phonebook.pause();
        }

    } while (choice != 6);

    phonebook.displayFarewell();

    return 0;
}
