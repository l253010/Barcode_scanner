#pragma once
// Category.h
// ----------------------------------------------------------------------------
// Category Specification and Management
// Part of Category Management, Product Management & Inventory Module.
//
// Compliant with assignment specification:
// Fields: 2-digit Category ID, Category Name, Category Acronym (max 8),
//         terminated by '#'.
// Stored in PCategory.txt.
// ----------------------------------------------------------------------------

#include <string>
#include "DynamicArray.h"

namespace ProductBilling {

/**
 * @brief Represents a single product category entity.
 */
struct Category {
    std::string id;       // 2-digit unique numeric ID (e.g. "10", "12", "20")
    std::string name;     // Full category display name (e.g. "Dairy & Bakery")
    std::string acronym;  // Short acronym (max 8 chars, e.g. "DAIRY")

    Category() = default;
    Category(const std::string& catId, const std::string& catName, const std::string& catAcronym)
        : id(catId), name(catName), acronym(catAcronym) {}

    bool isValid(std::string* errorMsg = nullptr) const;
    std::string toFileRecord() const;
    static Category fromDelimitedString(const std::string& line, std::string* errorMsg = nullptr);
};

/**
 * @brief Manages category records, file parsing, searching, and validation.
 *        Uses custom DynamicArray storage conforming to project constraints.
 */
class CategoryManager {
private:
    DynamicArray<Category> m_categories;
    std::string            m_lastLoadedFilePath;

    int findIndexById(const std::string& id) const;

public:
    CategoryManager();

    // ---- File Handling ----
    bool loadFromFile(const std::string& filePath, std::string* statusMessage = nullptr);
    bool saveToFile(const std::string& filePath = "", std::string* statusMessage = nullptr) const;

    // ---- Category Operations ----
    bool addCategory(const Category& category, std::string* errorMsg = nullptr);
    bool updateCategory(const Category& category, std::string* errorMsg = nullptr);
    bool removeCategory(const std::string& id, std::string* errorMsg = nullptr);

    // ---- Queries ----
    const Category* findCategoryById(const std::string& id) const;
    bool categoryExists(const std::string& id) const;
    DynamicArray<Category> getAllCategories() const;
    size_t getCategoryCount() const;
    const std::string& getLastLoadedFilePath() const;
    void clear();
};

} // namespace ProductBilling
