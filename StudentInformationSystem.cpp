#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <cctype>

namespace Ansi {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string FAINT = "\033[2m";
    const std::string ITALIC = "\033[3m";
    const std::string UNDERLINE = "\033[4m";
    const std::string FG_BLACK = "\033[30m";
    const std::string FG_RED = "\033[31m";
    const std::string FG_GREEN = "\033[32m";
    const std::string FG_YELLOW = "\033[33m";
    const std::string FG_BLUE = "\033[34m";
    const std::string FG_MAGENTA = "\033[35m";
    const std::string FG_CYAN = "\033[36m";
    const std::string FG_WHITE = "\033[37m";
    const std::string BG_BLACK = "\033[40m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    const std::string BG_WHITE = "\033[47m";
    const std::string MOVE_UP = "\033[1A";
    const std::string CLEAR_LINE = "\033[2K";
}

struct Course {
    std::string courseName;
    double grade;
};

class Student {
private:
    std::string studentID;
    std::string firstName;
    std::string lastName;
    std::vector<Course> courses;
    double gpa;

    void calculateGPA() {
        if (courses.empty()) {
            gpa = 0.0;
            return;
        }
        double totalPoints = 0.0;
        for (const auto& course : courses) {
            if (course.grade >= 90.0) totalPoints += 4.0;
            else if (course.grade >= 80.0) totalPoints += 3.0;
            else if (course.grade >= 70.0) totalPoints += 2.0;
            else if (course.grade >= 60.0) totalPoints += 1.0;
            else totalPoints += 0.0;
        }
        gpa = totalPoints / courses.size();
    }

public:
    Student() : gpa(0.0) {}

    Student(std::string id, std::string first, std::string last)
        : studentID(std::move(id)), firstName(std::move(first)), lastName(std::move(last)), gpa(0.0) {
    }

    Student(std::string id, std::string first, std::string last, const std::vector<Course>& crs)
        : studentID(std::move(id)), firstName(std::move(first)), lastName(std::move(last)), courses(crs) {
        calculateGPA();
    }

    void addCourse(const std::string& name, double grade) {
        courses.push_back({ name, grade });
        calculateGPA();
    }

    std::string getID() const { return studentID; }
    std::string getFirstName() const { return firstName; }
    std::string getLastName() const { return lastName; }
    double getGPA() const { return gpa; }
    const std::vector<Course>& getCourses() const { return courses; }

    void setFirstName(const std::string& first) { firstName = first; }
    void setLastName(const std::string& last) { lastName = last; }

    void displayHeader() const {
        std::cout << Ansi::BOLD << Ansi::FG_MAGENTA << std::left
            << std::setw(12) << "Student ID"
            << std::setw(20) << "First Name"
            << std::setw(20) << "Last Name"
            << std::setw(10) << "GPA (4.0)" << Ansi::RESET << std::endl;
        std::cout << Ansi::FAINT << Ansi::FG_MAGENTA << std::string(62, '-') << Ansi::RESET << std::endl;
    }

    void displaySummary() const {
        std::cout << Ansi::FG_CYAN << std::left
            << std::setw(12) << studentID
            << std::setw(20) << firstName
            << std::setw(20) << lastName
            << std::fixed << std::setprecision(2) << std::setw(10) << gpa << Ansi::RESET << std::endl;
    }

    void displayDetail() const {
        displaySummary();
        if (!courses.empty()) {
            std::cout << Ansi::FG_YELLOW << "  Courses:" << Ansi::RESET << std::endl;
            for (const auto& course : courses) {
                std::cout << "    - " << std::left << std::setw(25) << course.courseName
                    << ": " << std::fixed << std::setprecision(2) << course.grade << "/100" << std::endl;
            }
        }
        else {
            std::cout << Ansi::FAINT << "  No courses registered." << Ansi::RESET << std::endl;
        }
        std::cout << Ansi::FAINT << Ansi::FG_MAGENTA << std::string(62, '-') << Ansi::RESET << std::endl;
    }

    std::string serialize() const {
        std::ostringstream oss;
        oss << studentID << ";" << firstName << ";" << lastName;
        for (const auto& course : courses) {
            oss << ";" << course.courseName << "," << course.grade;
        }
        return oss.str();
    }

    static Student deserialize(const std::string& data) {
        std::stringstream ss(data);
        std::string segment;
        std::vector<std::string> parts;

        while (std::getline(ss, segment, ';')) {
            parts.push_back(segment);
        }

        if (parts.size() < 3) {
            throw std::runtime_error("Invalid student data format: Missing core fields.");
        }

        std::string id = parts[0];
        std::string first = parts[1];
        std::string last = parts[2];
        std::vector<Course> courses;

        for (size_t i = 3; i < parts.size(); ++i) {
            std::stringstream course_ss(parts[i]);
            std::string course_segment;
            std::vector<std::string> course_parts;
            while (std::getline(course_ss, course_segment, ',')) {
                course_parts.push_back(course_segment);
            }
            if (course_parts.size() == 2) {
                try {
                    double grade = std::stod(course_parts[1]);
                    courses.push_back({ course_parts[0], grade });
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << Ansi::FG_RED << "Warning: Invalid argument parsing course grade: '" << course_parts[1] << "' in segment '" << parts[i] << "'" << Ansi::RESET << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << Ansi::FG_RED << "Warning: Out of range value parsing course grade: '" << course_parts[1] << "' in segment '" << parts[i] << "'" << Ansi::RESET << std::endl;
                }
            }
            else {
                std::cerr << Ansi::FG_RED << "Warning: Malformed course data segment: '" << parts[i] << "'" << Ansi::RESET << std::endl;
            }
        }
        return Student(id, first, last, courses);
    }
};

class StudentInformationSystem {
private:
    std::map<std::string, Student> students;
    std::string dataFile = "students_data.txt";
    int nextStudentIDCounter = 1;

    void loadData() {
        std::ifstream file(dataFile);
        if (!file.is_open()) {
            std::cout << Ansi::FG_YELLOW << "Data file (" << dataFile << ") not found. A new one will be created upon saving." << Ansi::RESET << std::endl;
            updateNextStudentIDCounter();
            return;
        }

        std::string line;
        int maxIdNum = 0;
        int lineNumber = 0;
        while (std::getline(file, line)) {
            lineNumber++;
            if (line.empty()) continue;

            try {
                Student student = Student::deserialize(line);
                students[student.getID()] = student;

                if (student.getID().length() > 3 && student.getID().substr(0, 3) == "AZD") {
                    try {
                        int currentIdNum = std::stoi(student.getID().substr(3));
                        if (currentIdNum > maxIdNum) {
                            maxIdNum = currentIdNum;
                        }
                    }
                    catch (const std::invalid_argument&) {
                        std::cerr << Ansi::FG_RED << "Warning: Non-numeric suffix in Student ID: " << student.getID() << Ansi::RESET << std::endl;
                    }
                    catch (const std::out_of_range&) {
                        std::cerr << Ansi::FG_RED << "Warning: Numeric suffix out of range in Student ID: " << student.getID() << Ansi::RESET << std::endl;
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << Ansi::FG_RED << "Error loading data at line " << lineNumber << ": " << e.what() << " Line: \"" << line << "\"" << Ansi::RESET << std::endl;
            }
        }
        nextStudentIDCounter = maxIdNum + 1;
        file.close();
        std::cout << Ansi::FG_GREEN << students.size() << " student records successfully loaded from " << dataFile << Ansi::RESET << std::endl;
    }

    void saveData() const {
        std::ofstream file(dataFile);
        if (!file.is_open()) {
            std::cerr << Ansi::FG_RED << Ansi::BOLD << "FATAL ERROR: Could not open data file (" << dataFile << ") for saving! Data may be lost." << Ansi::RESET << std::endl;
            return;
        }
        for (const auto& pair : students) {
            file << pair.second.serialize() << std::endl;
        }
        file.close();
    }

    void updateNextStudentIDCounter() {
        if (students.empty()) {
            nextStudentIDCounter = 1;
            return;
        }
        int maxIdNum = 0;
        for (const auto& pair : students) {
            const std::string& currentID = pair.first;
            if (currentID.length() > 3 && currentID.substr(0, 3) == "AZD") {
                try {
                    int currentIdNum = std::stoi(currentID.substr(3));
                    if (currentIdNum > maxIdNum) {
                        maxIdNum = currentIdNum;
                    }
                }
                catch (...) {
                    continue;
                }
            }
        }
        nextStudentIDCounter = maxIdNum + 1;
    }

    void clearScreen() const {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    void printHeader(const std::string& title) const {
        clearScreen();
        std::cout << Ansi::BG_BLUE << Ansi::BOLD << Ansi::FG_WHITE << std::string(65, '=') << Ansi::RESET << std::endl;
        std::cout << Ansi::BG_BLUE << Ansi::BOLD << Ansi::FG_WHITE << " " << std::left << std::setw(63) << title << Ansi::RESET << std::endl;
        std::cout << Ansi::BG_BLUE << Ansi::BOLD << Ansi::FG_WHITE << std::string(65, '=') << Ansi::RESET << std::endl << std::endl;
    }

    void pause() const {
        std::cout << std::endl << Ansi::FG_YELLOW << "Press Enter to continue..." << Ansi::RESET;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::string generateStudentID() {
        std::ostringstream oss;
        oss << "AZD" << std::setw(4) << std::setfill('0') << nextStudentIDCounter++;
        return oss.str();
    }

    std::string readString(const std::string& prompt) {
        std::string input;
        std::cout << Ansi::FG_CYAN << prompt << Ansi::RESET;
        std::getline(std::cin, input);
        while (input.empty() || std::all_of(input.begin(), input.end(), ::isspace)) {
            std::cout << Ansi::FG_RED << "Input cannot be empty or just whitespace. Please try again: " << Ansi::RESET;
            std::getline(std::cin, input);
        }
        input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
        input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);
        return input;
    }

    double readDouble(const std::string& prompt, double minVal, double maxVal) {
        double value;
        std::cout << Ansi::FG_CYAN << prompt << Ansi::RESET;
        while (true) {
            std::cin >> value;
            if (std::cin.fail()) {
                std::cout << Ansi::FG_RED << "Invalid input. Please enter a numeric value: " << Ansi::RESET;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else if (value < minVal || value > maxVal) {
                std::cout << Ansi::FG_RED << "Input out of range. Please enter a number between "
                    << minVal << " and " << maxVal << ": " << Ansi::RESET;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return value;
            }
        }
    }

    char readChar(const std::string& prompt, const std::string& validChars) {
        char input;
        std::cout << Ansi::FG_CYAN << prompt << Ansi::RESET;
        while (true) {
            std::cin >> input;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            input = std::tolower(input);
            if (validChars.find(input) != std::string::npos) {
                return input;
            }
            std::cout << Ansi::FG_RED << "Invalid choice. Please enter one of [" << validChars << "]: " << Ansi::RESET;
        }
    }

public:
    StudentInformationSystem() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        loadData();
    }

    ~StudentInformationSystem() {
        saveData();
        std::cout << Ansi::FG_GREEN << "\nData saved. Exiting program." << Ansi::RESET << std::endl;
    }

    void addStudent() {
        printHeader("Add New Student");
        std::string first = readString("Enter student's first name: ");
        std::string last = readString("Enter student's last name: ");
        std::string newID = generateStudentID();
        Student newStudent(newID, first, last);
        students[newID] = newStudent;
        std::cout << Ansi::FG_GREEN << "\nStudent added successfully! Assigned ID: "
            << Ansi::BOLD << newID << Ansi::RESET << std::endl;
        char addCoursesChoice = readChar("Add courses for this student now? (y/n): ", "yn");
        if (addCoursesChoice == 'y') {
            addCourseToStudent(newID);
        }
        saveData();
        pause();
    }

    void editStudent() {
        printHeader("Edit Student Information");
        std::string studentID = readString("Enter the ID of the student to edit: ");
        auto it = students.find(studentID);
        if (it == students.end()) {
            std::cout << Ansi::FG_RED << "Student with ID '" << studentID << "' not found." << Ansi::RESET << std::endl;
        }
        else {
            std::cout << "\nCurrent Information for Student ID: " << studentID << std::endl;
            it->second.displayDetail();
            std::cout << std::endl;
            std::cout << Ansi::FG_YELLOW << "Enter new first name (leave blank and press Enter to keep current '" << it->second.getFirstName() << "'): " << Ansi::RESET;
            std::string newFirst;
            std::getline(std::cin, newFirst);
            newFirst.erase(0, newFirst.find_first_not_of(" \t\n\r\f\v"));
            newFirst.erase(newFirst.find_last_not_of(" \t\n\r\f\v") + 1);
            std::cout << Ansi::FG_YELLOW << "Enter new last name (leave blank and press Enter to keep current '" << it->second.getLastName() << "'): " << Ansi::RESET;
            std::string newLast;
            std::getline(std::cin, newLast);
            newLast.erase(0, newLast.find_first_not_of(" \t\n\r\f\v"));
            newLast.erase(newLast.find_last_not_of(" \t\n\r\f\v") + 1);
            bool changed = false;
            if (!newFirst.empty()) {
                it->second.setFirstName(newFirst);
                changed = true;
            }
            if (!newLast.empty()) {
                it->second.setLastName(newLast);
                changed = true;
            }
            if (changed) {
                std::cout << Ansi::FG_GREEN << "\nStudent information updated successfully." << Ansi::RESET << std::endl;
                saveData();
            }
            else {
                std::cout << Ansi::FG_YELLOW << "\nNo changes were made." << Ansi::RESET << std::endl;
            }
        }
        pause();
    }

    void deleteStudent() {
        printHeader("Delete Student Record");
        std::string studentID = readString("Enter the ID of the student to delete: ");
        auto it = students.find(studentID);
        if (it == students.end()) {
            std::cout << Ansi::FG_RED << "Student with ID '" << studentID << "' not found." << Ansi::RESET << std::endl;
        }
        else {
            std::cout << "\nStudent to be deleted:" << std::endl;
            it->second.displayDetail();
            char confirm = readChar("\n" + Ansi::BOLD + Ansi::FG_RED + "Are you sure you want to permanently delete this student? (y/n): " + Ansi::RESET, "yn");
            if (confirm == 'y') {
                students.erase(it);
                std::cout << Ansi::FG_GREEN << "\nStudent record deleted successfully." << Ansi::RESET << std::endl;
                saveData();
            }
            else {
                std::cout << Ansi::FG_YELLOW << "\nDeletion cancelled." << Ansi::RESET << std::endl;
            }
        }
        pause();
    }


    void addCourseToStudent(const std::string& studentID) {
        auto it = students.find(studentID);
        if (it == students.end()) {
            std::cout << Ansi::FG_RED << "Student with ID '" << studentID << "' not found." << Ansi::RESET << std::endl;
            return;
        }
        printHeader("Add Courses to Student: " + it->second.getFirstName() + " " + it->second.getLastName() + " (" + studentID + ")");
        std::cout << "Current Courses:" << std::endl;
        it->second.displayDetail();
        while (true) {
            std::cout << "\n";
            std::string courseName = readString("Enter Course Name (or type 'q' to finish): ");
            if (courseName == "q" || courseName == "Q") {
                break;
            }
            double grade = readDouble("Enter Grade for " + courseName + " (0-100): ", 0.0, 100.0);
            it->second.addCourse(courseName, grade);
            std::cout << Ansi::FG_GREEN << "Course '" << courseName << "' with grade " << grade << " added successfully." << Ansi::RESET << std::endl;
            saveData();
        }
        std::cout << Ansi::FG_YELLOW << "\nFinished adding courses for " << studentID << "." << Ansi::RESET << std::endl;
    }

    void viewAllStudents() const {
        printHeader("View All Students");
        if (students.empty()) {
            std::cout << Ansi::FG_YELLOW << "No students registered in the system." << Ansi::RESET << std::endl;
        }
        else {
            Student temp;
            temp.displayHeader();
            for (const auto& pair : students) {
                pair.second.displaySummary();
            }
            std::cout << Ansi::FAINT << Ansi::FG_MAGENTA << std::string(62, '-') << Ansi::RESET << std::endl;
            std::cout << Ansi::BOLD << "Total Students: " << students.size() << Ansi::RESET << std::endl;
        }
        pause();
    }

    void viewStudentsByGPA() {
        printHeader("Students Sorted by GPA (Descending)");
        if (students.empty()) {
            std::cout << Ansi::FG_YELLOW << "No students registered in the system." << Ansi::RESET << std::endl;
        }
        else {
            std::vector<Student> sortedStudents;
            sortedStudents.reserve(students.size());
            for (const auto& pair : students) {
                sortedStudents.push_back(pair.second);
            }
            std::sort(sortedStudents.begin(), sortedStudents.end(),
                [](const Student& a, const Student& b) {
                    if (a.getGPA() != b.getGPA()) {
                        return a.getGPA() > b.getGPA();
                    }
                    return a.getLastName() < b.getLastName();
                });
            sortedStudents[0].displayHeader();
            for (const auto& student : sortedStudents) {
                student.displaySummary();
            }
            std::cout << Ansi::FAINT << Ansi::FG_MAGENTA << std::string(62, '-') << Ansi::RESET << std::endl;
            std::cout << Ansi::BOLD << "Total Students: " << sortedStudents.size() << Ansi::RESET << std::endl;
        }
        pause();
    }

    void searchStudent() {
        printHeader("Search Student");
        std::cout << Ansi::FG_CYAN << "Search by:" << Ansi::RESET << std::endl;
        std::cout << "  1. Student ID (Exact Match)" << std::endl;
        std::cout << "  2. Last Name (Case-Insensitive, Partial Match)" << std::endl;
        int choice = static_cast<int>(readDouble("Enter your choice (1-2): ", 1, 2));
        if (choice == 1) {
            std::string searchID = readString("Enter Student ID to search for: ");
            auto it = students.find(searchID);
            if (it != students.end()) {
                std::cout << Ansi::FG_GREEN << "\nStudent Found (ID Match):" << Ansi::RESET << std::endl;
                it->second.displayHeader();
                it->second.displayDetail();
            }
            else {
                std::cout << Ansi::FG_RED << "\nNo student found with ID: " << searchID << Ansi::RESET << std::endl;
            }
        }
        else if (choice == 2) {
            std::string searchName = readString("Enter Last Name to search for: ");
            std::string searchNameLower = searchName;
            std::transform(searchNameLower.begin(), searchNameLower.end(), searchNameLower.begin(), ::tolower);
            std::vector<Student> matches;
            for (const auto& pair : students) {
                std::string lastNameLower = pair.second.getLastName();
                std::transform(lastNameLower.begin(), lastNameLower.end(), lastNameLower.begin(), ::tolower);
                if (lastNameLower.find(searchNameLower) != std::string::npos) {
                    matches.push_back(pair.second);
                }
            }
            if (!matches.empty()) {
                std::cout << Ansi::FG_GREEN << "\n" << matches.size() << " Student(s) Found (Last Name Match):" << Ansi::RESET << std::endl;
                matches[0].displayHeader();
                for (const auto& student : matches) {
                    student.displaySummary();
                }
                std::cout << Ansi::FAINT << Ansi::FG_MAGENTA << std::string(62, '-') << Ansi::RESET << std::endl;
            }
            else {
                std::cout << Ansi::FG_RED << "\nNo students found with a last name matching: '" << searchName << "'" << Ansi::RESET << std::endl;
            }
        }
        pause();
    }

    void displayAZD() const {
        std::cout << std::endl << std::endl;
        std::cout << Ansi::BOLD << Ansi::BG_MAGENTA << Ansi::FG_WHITE << R"(
                  █████╗ ███████╗██████╗
                 ██╔══██╗╚══███╔╝██╔══██╗
                 ███████║  ███╔╝ ██║  ██║
                 ██╔══██║ ███╔╝  ██║  ██║
                 ██║  ██║███████╗██████╔╝
                 ╚═╝  ╚═╝╚══════╝╚═════╝
        )" << Ansi::RESET << std::endl << std::endl;
    }

    void run() {
        int choice;
        do {
            clearScreen();
            std::cout << Ansi::BOLD << Ansi::FG_BLACK << "=================================================================" << Ansi::RESET << std::endl;
            std::cout << Ansi::BOLD << Ansi::FG_BLACK << "|         ADVANCED STUDENT INFORMATION SYSTEM (ASIS)            |" << Ansi::RESET << std::endl;
            std::cout << Ansi::BOLD << Ansi::FG_BLACK << "=================================================================" << Ansi::RESET << std::endl;
            std::cout << std::endl;
            std::cout << Ansi::FG_GREEN << "  1. " << Ansi::RESET << "Add New Student" << std::endl;
            std::cout << Ansi::FG_BLUE << "  2. " << Ansi::RESET << "View All Students" << std::endl;
            std::cout << Ansi::FG_BLUE << "  3. " << Ansi::RESET << "View Students by GPA (High to Low)" << std::endl;
            std::cout << Ansi::FG_YELLOW << "  4. " << Ansi::RESET << "Search Student (ID or Last Name)" << std::endl;
            std::cout << Ansi::FG_YELLOW << "  5. " << Ansi::RESET << "Add Courses to Student" << std::endl;
            std::cout << Ansi::FG_MAGENTA << "  6. " << Ansi::RESET << "Edit Student Information" << std::endl;
            std::cout << Ansi::FG_RED << "  7. " << Ansi::RESET << "Delete Student Record" << std::endl;
            std::cout << Ansi::FAINT << "  0. " << Ansi::RESET << "Save and Exit" << std::endl;
            std::cout << std::endl;
            choice = static_cast<int>(readDouble(Ansi::BOLD + "Enter your choice (0-7): " + Ansi::RESET, 0, 7));
            switch (choice) {
            case 1: addStudent(); break;
            case 2: viewAllStudents(); break;
            case 3: viewStudentsByGPA(); break;
            case 4: searchStudent(); break;
            case 5: {
                printHeader("Add Courses to Existing Student");
                std::string studentID = readString("Enter the ID of the student to add courses to: ");
                addCourseToStudent(studentID);
                if (students.count(studentID)) {
                    pause();
                }
                else {
                    pause();
                }
            }
                  break;
            case 6: editStudent(); break;
            case 7: deleteStudent(); break;
            case 0:
                break;
            default:
                std::cout << Ansi::FG_RED << "Invalid choice. Please try again." << Ansi::RESET << std::endl;
                pause();
                break;
            }
        } while (choice != 0);
        displayAZD();
    }
};

int main() {
#ifdef _WIN32
    system("chcp 437 > nul");
#endif
    StudentInformationSystem sis;
    sis.run();
    return 0;
}