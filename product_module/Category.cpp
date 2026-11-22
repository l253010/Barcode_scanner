// Category.cpp
// ----------------------------------------------------------------------------
// Implementation of Category and CategoryManager classes.
// Free of <vector>, <map>, <sstream>, <cassert>, and <cctype>.
// ----------------------------------------------------------------------------

#include "Category.h"
#include <fstream>

namespace ProductBilling {

namespace {
    bool isWhitespaceChar(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    std::string trim(const std::string& str) {
        if (str.empty()) return "";
        size_t first = 0;
        while (first < str.length() && isWhitespaceChar(str[first])) {
            ++first;
        }
        if (first == str.length()) return "";

        size_t last = str.length() - 1;
        while (last > first && isWhitespaceChar(str[last])) {
            --last;
        }
        return str.substr(first, (last - first + 1));
    }

    bool isDigitChar(char c) {
        return c >= '0' && c <= '9';
    }
}

// ============================================================================
// Category Entity Implementation
// ============================================================================

bool Category::isValid(std::string* errorMsg) const {
    std::string cleanId = trim(id);
    std::string cleanName = trim(name);
    std::string cleanAcr = trim(acronym);

    if (cleanId.length() != 2 || !isDigitChar(cleanId[0]) || !isDigitChar(cleanId[1])) {
        if (errorMsg) *errorMsg = "Category ID must be exactly 2 numeric digits (e.g. '10' through '99').";
        return false;
    }

    if (cleanId[0] == '0') {
        if (errorMsg) *errorMsg = "Category ID must start with a non-zero digit (10-99) to support standard barcodes.";
        return false;
    }

    if (cleanName.empty()) {
        if (errorMsg) *errorMsg = "Category Name cannot be empty.";
        return false;
    }

    if (cleanAcr.empty()) {
        if (errorMsg) *errorMsg = "Category Acronym cannot be empty.";
        return false;
    }

    if (cleanAcr.length() > 8) {
        if (errorMsg) *errorMsg = "Category Acronym must be 8 characters or fewer.";
        return false;
    }

    return true;
}

std::string Category::toFileRecord() const {
    std::string cleanId = trim(id);
    std::string cleanName = trim(name);
    std::string cleanAcr = trim(acronym);
    return cleanId + " " + cleanName + " " + cleanAcr + " #";
}

Category Category::fromDelimitedString(const std::string& line, std::string* errorMsg) {
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) {
        if (errorMsg) *errorMsg = "Empty category line.";
        return Category();
    }

    // Skip comment lines
    if ((trimmedLine[0] == '#' && trimmedLine.back() != '#') || trimmedLine.rfind("//", 0) == 0) {
        if (errorMsg) *errorMsg = "Comment line.";
        return Category();
    }

    // Each record must end with '#'
    if (trimmedLine.back() != '#') {
        if (errorMsg) *errorMsg = "Category record must end with '#': " + trimmedLine;
        return Category();
    }

    std::string content = trim(trimmedLine.substr(0, trimmedLine.length() - 1));
    if (content.empty()) {
        if (errorMsg) *errorMsg = "Record contains only '#' marker.";
        return Category();
    }

    // Tokenize by whitespace
    DynamicArray<std::string> tokens;
    size_t pos = 0;
    while (pos < content.length()) {
        while (pos < content.length() && isWhitespaceChar(content[pos])) {
            ++pos;
        }
        if (pos >= content.length()) break;
        size_t tokenStart = pos;
        while (pos < content.length() && !isWhitespaceChar(content[pos])) {
            ++pos;
        }
        tokens.push_back(content.substr(tokenStart, pos - tokenStart));
    }

    if (tokens.size() < 3) {
        if (errorMsg) {
            *errorMsg = "Insufficient tokens in category record (expected ID, Name, Acronym before '#'): " + trimmedLine;
        }
        return Category();
    }

    std::string catId = tokens[0];
    std::string catName;
    std::string catAcr;

    // Standard format: <ID> <Name...> <Acronym> #
    // If the last token is <= 8 chars, treat it as acronym and middle as name
    if (tokens[tokens.size() - 1].length() <= 8) {
        catAcr = tokens[tokens.size() - 1];
        catName = tokens[1];
        for (size_t i = 2; i < tokens.size() - 1; ++i) {
            catName += " " + tokens[i];
        }
    } else if (tokens[1].length() <= 8) {
        // Fallback for <ID> <Acronym> <Name...> #
        catAcr = tokens[1];
        catName = tokens[2];
        for (size_t i = 3; i < tokens.size(); ++i) {
            catName += " " + tokens[i];
        }
    } else {
        catAcr = tokens[tokens.size() - 1];
        catName = tokens[1];
        for (size_t i = 2; i < tokens.size() - 1; ++i) {
            catName += " " + tokens[i];
        }
    }

    Category cat(catId, catName, catAcr);
    if (!cat.isValid(errorMsg)) {
        return Category();
    }
    return cat;
}

// ============================================================================
// CategoryManager Implementation
// ============================================================================

CategoryManager::CategoryManager()
    : m_lastLoadedFilePath("")
{
}

int CategoryManager::findIndexById(const std::string& id) const {
    std::string cleanId = trim(id);
    if (cleanId.empty()) return -1;
    for (size_t i = 0; i < m_categories.size(); ++i) {
        if (m_categories[i].id == cleanId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool CategoryManager::loadFromFile(const std::string& filePath, std::string* statusMessage) {
    std::string cleanPath = trim(filePath);
    if (cleanPath.empty()) {
        if (statusMessage) *statusMessage = "Error: Category file path cannot be empty.";
        return false;
    }

    std::ifstream inFile(cleanPath);
    if (!inFile.is_open()) {
        if (statusMessage) {
            *statusMessage = "Error: Unable to open category file at path: " + cleanPath;
        }
        return false;
    }

    m_categories.clear();
    m_lastLoadedFilePath = cleanPath;

    std::string line;
    int loadedCount = 0;
    int skippedCount = 0;

    while (std::getline(inFile, line)) {
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;
        if ((trimmedLine[0] == '#' && trimmedLine.back() != '#') || trimmedLine.rfind("//", 0) == 0) {
            continue;
        }

        std::string parseErr;
        Category c = Category::fromDelimitedString(trimmedLine, &parseErr);
        if (c.isValid()) {
            int existingIdx = findIndexById(c.id);
            if (existingIdx >= 0) {
                m_categories[existingIdx] = c; // Update existing
            } else {
                m_categories.push_back(c);
            }
            loadedCount++;
        } else {
            skippedCount++;
        }
    }

    inFile.close();

    if (statusMessage) {
        *statusMessage = "Categories loaded successfully: " + std::to_string(m_categories.size()) +
                         " active categories (parsed " + std::to_string(loadedCount) +
                         ", skipped " + std::to_string(skippedCount) + ").";
    }

    return (m_categories.size() > 0);
}

bool CategoryManager::saveToFile(const std::string& filePath, std::string* statusMessage) const {
    std::string targetPath = trim(filePath);
    if (targetPath.empty()) {
        targetPath = m_lastLoadedFilePath;
    }
    if (targetPath.empty()) {
        if (statusMessage) *statusMessage = "Error: Destination file path is unspecified.";
        return false;
    }

    std::ofstream outFile(targetPath, std::ios::trunc);
    if (!outFile.is_open()) {
        if (statusMessage) {
            *statusMessage = "Error: Unable to open category file for writing: " + targetPath;
        }
        return false;
    }

    outFile << "# PCategory.txt - Product Categories Catalog\n";
    outFile << "# Format: <2-digit Category ID> <Category Name> <Acronym> #\n";

    for (size_t i = 0; i < m_categories.size(); ++i) {
        outFile << m_categories[i].toFileRecord() << "\n";
    }

    outFile.flush();
    outFile.close();

    if (statusMessage) {
        *statusMessage = "Categories persisted successfully (" + std::to_string(m_categories.size()) +
                         " records written to " + targetPath + ").";
    }

    return true;
}

bool CategoryManager::addCategory(const Category& category, std::string* errorMsg) {
    if (!category.isValid(errorMsg)) {
        return false;
    }

    int existingIdx = findIndexById(category.id);
    if (existingIdx >= 0) {
        if (errorMsg) {
            *errorMsg = "Category with ID '" + category.id + "' already exists ('" +
                        m_categories[existingIdx].name + "'). Category IDs must be unique.";
        }
        return false;
    }

    m_categories.push_back(category);
    return true;
}

bool CategoryManager::updateCategory(const Category& category, std::string* errorMsg) {
    if (!category.isValid(errorMsg)) {
        return false;
    }

    int idx = findIndexById(category.id);
    if (idx < 0) {
        if (errorMsg) {
            *errorMsg = "Category with ID '" + category.id + "' does not exist.";
        }
        return false;
    }

    m_categories[idx] = category;
    return true;
}

bool CategoryManager::removeCategory(const std::string& id, std::string* errorMsg) {
    int idx = findIndexById(id);
    if (idx < 0) {
        if (errorMsg) {
            *errorMsg = "Category with ID '" + id + "' not found.";
        }
        return false;
    }

    m_categories.erase(idx);
    return true;
}

const Category* CategoryManager::findCategoryById(const std::string& id) const {
    int idx = findIndexById(id);
    if (idx >= 0) {
        return &m_categories[idx];
    }
    return nullptr;
}

bool CategoryManager::categoryExists(const std::string& id) const {
    return findIndexById(id) >= 0;
}

DynamicArray<Category> CategoryManager::getAllCategories() const {
    return m_categories;
}

size_t CategoryManager::getCategoryCount() const {
    return m_categories.size();
}

const std::string& CategoryManager::getLastLoadedFilePath() const {
    return m_lastLoadedFilePath;
}

void CategoryManager::clear() {
    m_categories.clear();
    m_lastLoadedFilePath = "";
}

} // namespace ProductBilling
