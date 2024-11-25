#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stack>
#include <queue>
#include <memory>
#include <ctime>

// InventoryItem class definition
class InventoryItem {
private:
    int id;
    std::string name;
    std::string category;
    int quantity;
    double price;
    int minStockLevel;

public:
    InventoryItem() = default;
    InventoryItem(int id, const std::string& name, const std::string& category, 
                 int quantity, double price, int minStockLevel)
        : id(id), name(name), category(category), quantity(quantity), 
          price(price), minStockLevel(minStockLevel) {}

    // Getters
    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getCategory() const { return category; }
    int getQuantity() const { return quantity; }
    double getPrice() const { return price; }
    int getMinStockLevel() const { return minStockLevel; }

    // Setters
    void setId(int newId) { id = newId; }
    void setName(const std::string& newName) { name = newName; }
    void setCategory(const std::string& newCategory) { category = newCategory; }
    void setQuantity(int newQuantity) { quantity = newQuantity; }
    void setPrice(double newPrice) { price = newPrice; }
    void setMinStockLevel(int newMinStockLevel) { minStockLevel = newMinStockLevel; }

    // Check if item is low on stock
    bool isLowStock() const { return quantity <= minStockLevel; }
};

// Transaction class for history tracking
class Transaction {
private:
    std::time_t timestamp;
    std::string action;
    int itemId;
    std::string details;

public:
    Transaction(const std::string& action, int itemId, const std::string& details)
        : action(action), itemId(itemId), details(details) {
        timestamp = std::time(nullptr);
    }

    std::string getFormattedTime() const {
        char buffer[26];
        ctime_s(buffer, sizeof(buffer), &timestamp);
        std::string time(buffer);
        return time.substr(0, time.length() - 1); // Remove newline
    }

    std::string toString() const {
        return getFormattedTime() + " - " + action + " (Item ID: " + 
               std::to_string(itemId) + ") " + details;
    }
};

// Category tree node
struct CategoryNode {
    std::string name;
    std::vector<std::shared_ptr<CategoryNode>> children;
    std::vector<int> itemIds; // Store item IDs in this category

    CategoryNode(const std::string& name) : name(name) {}
};

// Order class for queue
class Order {
private:
    int orderId;
    int itemId;
    int quantity;
    std::string status;
    std::time_t orderTime;

public:
    Order(int orderId, int itemId, int quantity)
        : orderId(orderId), itemId(itemId), quantity(quantity), status("Pending") {
        orderTime = std::time(nullptr);
    }

    int getOrderId() const { return orderId; }
    int getItemId() const { return itemId; }
    int getQuantity() const { return quantity; }
    std::string getStatus() const { return status; }
    std::time_t getOrderTime() const { return orderTime; }

    void setStatus(const std::string& newStatus) { status = newStatus; }
};

// WarehouseSystem class definition
class WarehouseSystem {
private:
    std::map<int, InventoryItem> inventory;
    std::string filename;
    int nextId;
    std::stack<Transaction> transactionHistory;
    std::queue<Order> orderQueue;
    std::shared_ptr<CategoryNode> categoryRoot;
    int nextOrderId;

    void loadFromFile() {
        std::ifstream file(filename);
        if (!file) {
            return;  // File doesn't exist yet
        }

        std::string line;
        // Skip header line
        std::getline(file, line);
        
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            
            int id;
            std::string name, category;
            int quantity;
            double price;
            int minStockLevel;
            
            // Parse CSV line
            std::getline(ss, token, ','); id = std::stoi(token);
            std::getline(ss, name, ',');
            std::getline(ss, category, ',');
            std::getline(ss, token, ','); quantity = std::stoi(token);
            std::getline(ss, token, ','); price = std::stod(token);
            std::getline(ss, token, ','); minStockLevel = std::stoi(token);
            
            inventory[id] = InventoryItem(id, name, category, quantity, price, minStockLevel);
            nextId = std::max(nextId, id + 1);
        }
    }

    void saveToFile() const {
        std::ofstream file(filename);
        file << "ID,Name,Category,Quantity,Price,MinStockLevel\n";
        
        for (const auto& [id, item] : inventory) {
            file << item.getId() << ","
                 << item.getName() << ","
                 << item.getCategory() << ","
                 << item.getQuantity() << ","
                 << std::fixed << std::setprecision(2) << item.getPrice() << ","
                 << item.getMinStockLevel() << "\n";
        }
    }

    std::vector<InventoryItem> getItemsByCategory(const std::string& category) const {
        std::vector<InventoryItem> items;
        for (const auto& [id, item] : inventory) {
            if (item.getCategory() == category) {
                items.push_back(item);
            }
        }
        return items;
    }

    void addTransaction(const std::string& action, int itemId, const std::string& details) {
        transactionHistory.push(Transaction(action, itemId, details));
    }

    void initializeCategoryTree() {
        categoryRoot = std::make_shared<CategoryNode>("Root");
    }

    std::shared_ptr<CategoryNode> findOrCreateCategory(const std::string& category) {
        // Split category path (e.g., "Electronics/Phones" -> ["Electronics", "Phones"])
        std::vector<std::string> path;
        std::stringstream ss(category);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            path.push_back(segment);
        }

        auto current = categoryRoot;
        for (const auto& name : path) {
            auto it = std::find_if(current->children.begin(), current->children.end(),
                [&name](const auto& child) { return child->name == name; });
            
            if (it == current->children.end()) {
                auto newNode = std::make_shared<CategoryNode>(name);
                current->children.push_back(newNode);
                current = newNode;
            } else {
                current = *it;
            }
        }
        return current;
    }

public:
    WarehouseSystem(const std::string& filename) 
        : filename(filename), nextId(1), nextOrderId(1) {
        loadFromFile();
        initializeCategoryTree();
    }

    void addItem(const InventoryItem& item) {
        inventory[item.getId()] = item;
        nextId = item.getId() + 1;
        
        // Add item to category tree
        auto categoryNode = findOrCreateCategory(item.getCategory());
        categoryNode->itemIds.push_back(item.getId());
        
        addTransaction("Add", item.getId(), 
            "Added " + item.getName() + " to category " + item.getCategory());
        saveToFile();
    }

    bool removeItem(int id) {
        if (inventory.erase(id) > 0) {
            saveToFile();
            return true;
        }
        return false;
    }

    bool updateItem(const InventoryItem& item) {
        if (inventory.find(item.getId()) != inventory.end()) {
            inventory[item.getId()] = item;
            saveToFile();
            return true;
        }
        return false;
    }

    InventoryItem* findItem(int id) {
        auto it = inventory.find(id);
        return (it != inventory.end()) ? &it->second : nullptr;
    }

    void displayAllItems() const {
        std::cout << std::setw(5) << "ID" << " | "
                  << std::setw(20) << "Name" << " | "
                  << std::setw(15) << "Category" << " | "
                  << std::setw(10) << "Quantity" << " | "
                  << std::setw(10) << "Price" << " | "
                  << std::setw(15) << "Min Stock" << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& [id, item] : inventory) {
            std::cout << std::setw(5) << item.getId() << " | "
                      << std::setw(20) << item.getName() << " | "
                      << std::setw(15) << item.getCategory() << " | "
                      << std::setw(10) << item.getQuantity() << " | "
                      << std::setw(10) << std::fixed << std::setprecision(2) << item.getPrice() << " | "
                      << std::setw(15) << item.getMinStockLevel() << "\n";
        }
    }

    void displayLowStockItems() const {
        bool found = false;
        for (const auto& [id, item] : inventory) {
            if (item.isLowStock()) {
                if (!found) {
                    std::cout << "Low Stock Items:\n";
                    found = true;
                }
                std::cout << "ID: " << item.getId() 
                          << ", Name: " << item.getName()
                          << ", Current Stock: " << item.getQuantity()
                          << ", Min Stock: " << item.getMinStockLevel() << "\n";
            }
        }
        if (!found) {
            std::cout << "No items are low on stock.\n";
        }
    }

    void displayByCategory(const std::string& category) const {
        auto items = getItemsByCategory(category);
        if (items.empty()) {
            std::cout << "No items found in category: " << category << "\n";
            return;
        }
        
        std::cout << "Items in category '" << category << "':\n";
        for (const auto& item : items) {
            std::cout << "ID: " << item.getId() 
                      << ", Name: " << item.getName()
                      << ", Quantity: " << item.getQuantity()
                      << ", Price: " << item.getPrice() << "\n";
        }
    }

    std::vector<InventoryItem> getAllItems() const {
        std::vector<InventoryItem> items;
        for (const auto& [id, item] : inventory) {
            items.push_back(item);
        }
        return items;
    }

    void sortByName() {
        auto items = getAllItems();
        std::sort(items.begin(), items.end(), 
                  [](const InventoryItem& a, const InventoryItem& b) {
                      return a.getName() < b.getName();
                  });
        
        // Display sorted items
        for (const auto& item : items) {
            std::cout << "ID: " << item.getId() 
                      << ", Name: " << item.getName()
                      << ", Category: " << item.getCategory()
                      << ", Quantity: " << item.getQuantity() << "\n";
        }
    }

    void sortByQuantity() {
        auto items = getAllItems();
        std::sort(items.begin(), items.end(), 
                  [](const InventoryItem& a, const InventoryItem& b) {
                      return a.getQuantity() < b.getQuantity();
                  });
        
        // Display sorted items
        for (const auto& item : items) {
            std::cout << "ID: " << item.getId() 
                      << ", Name: " << item.getName()
                      << ", Quantity: " << item.getQuantity() << "\n";
        }
    }

    int getNextId() const { return nextId; }

    void createOrder(int itemId, int quantity) {
        auto item = findItem(itemId);
        if (item && quantity > 0) {
            orderQueue.push(Order(nextOrderId++, itemId, quantity));
            addTransaction("Order Created", itemId, 
                "Ordered " + std::to_string(quantity) + " units");
            std::cout << "Order created successfully!\n";
        } else {
            std::cout << "Invalid item ID or quantity!\n";
        }
    }

    void processNextOrder() {
        if (orderQueue.empty()) {
            std::cout << "No orders to process!\n";
            return;
        }

        Order order = orderQueue.front();
        orderQueue.pop();

        auto item = findItem(order.getItemId());
        if (item) {
            if (item->getQuantity() >= order.getQuantity()) {
                item->setQuantity(item->getQuantity() - order.getQuantity());
                addTransaction("Order Processed", order.getItemId(),
                    "Processed order #" + std::to_string(order.getOrderId()) + 
                    " for " + std::to_string(order.getQuantity()) + " units");
                std::cout << "Order #" << order.getOrderId() << " processed successfully!\n";
                saveToFile();
            } else {
                std::cout << "Insufficient stock for order #" << order.getOrderId() << "!\n";
                // Put the order back in queue
                orderQueue.push(order);
            }
        }
    }

    void displayTransactionHistory(int limit = 10) const {
        std::cout << "\nRecent Transaction History:\n";
        std::cout << std::string(50, '-') << "\n";
        
        auto tempStack = transactionHistory;
        int count = 0;
        
        while (!tempStack.empty() && count < limit) {
            std::cout << tempStack.top().toString() << "\n";
            tempStack.pop();
            count++;
        }
    }

    void displayOrderQueue() const {
        if (orderQueue.empty()) {
            std::cout << "No pending orders.\n";
            return;
        }

        std::cout << "\nPending Orders:\n";
        std::cout << std::string(50, '-') << "\n";
        
        auto tempQueue = orderQueue;
        while (!tempQueue.empty()) {
            const auto& order = tempQueue.front();
            auto item = inventory.find(order.getItemId());
            
            std::cout << "Order #" << order.getOrderId() << ":\n"
                     << "  Item: " << (item != inventory.end() ? item->second.getName() : "Unknown")
                     << " (ID: " << order.getItemId() << ")\n"
                     << "  Quantity: " << order.getQuantity() << "\n"
                     << "  Status: " << order.getStatus() << "\n\n";
            
            tempQueue.pop();
        }
    }
};

// Helper functions for the main menu
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void displayMenu() {
    std::cout << "\nWarehouse Management System\n";
    std::cout << "1. Add New Item\n";
    std::cout << "2. Remove Item\n";
    std::cout << "3. Update Item\n";
    std::cout << "4. Find Item\n";
    std::cout << "5. Display All Items\n";
    std::cout << "6. Display Low Stock Items\n";
    std::cout << "7. Display Items by Category\n";
    std::cout << "8. Sort Items by Name\n";
    std::cout << "9. Sort Items by Quantity\n";
    std::cout << "10. Create Order\n";
    std::cout << "11. Process Next Order\n";
    std::cout << "12. Display Order Queue\n";
    std::cout << "13. Display Transaction History\n";
    std::cout << "0. Exit\n";
    std::cout << "Enter your choice: ";
}

InventoryItem inputItemDetails(WarehouseSystem& system, bool isNew = true) {
    int id = isNew ? system.getNextId() : 0;
    std::string name, category;
    int quantity, minStockLevel;
    double price;
    
    if (!isNew) {
        std::cout << "Enter item ID: ";
        std::cin >> id;
        clearInputBuffer();
    }
    
    std::cout << "Enter item name: ";
    std::getline(std::cin, name);
    
    std::cout << "Enter category: ";
    std::getline(std::cin, category);
    
    std::cout << "Enter quantity: ";
    std::cin >> quantity;
    
    std::cout << "Enter price: ";
    std::cin >> price;
    
    std::cout << "Enter minimum stock level: ";
    std::cin >> minStockLevel;
    
    clearInputBuffer();
    
    return InventoryItem(id, name, category, quantity, price, minStockLevel);
}

int main() {
    WarehouseSystem system("inventory.csv");
    int choice;
    
    do {
        displayMenu();
        std::cin >> choice;
        clearInputBuffer();
        
        switch (choice) {
            case 1: {  // Add New Item
                auto item = inputItemDetails(system);
                system.addItem(item);
                std::cout << "Item added successfully!\n";
                break;
            }
            case 2: {  // Remove Item
                int id;
                std::cout << "Enter item ID to remove: ";
                std::cin >> id;
                if (system.removeItem(id)) {
                    std::cout << "Item removed successfully!\n";
                } else {
                    std::cout << "Item not found!\n";
                }
                break;
            }
            case 3: {  // Update Item
                auto item = inputItemDetails(system, false);
                if (system.updateItem(item)) {
                    std::cout << "Item updated successfully!\n";
                } else {
                    std::cout << "Item not found!\n";
                }
                break;
            }
            case 4: {  // Find Item
                int id;
                std::cout << "Enter item ID to find: ";
                std::cin >> id;
                auto item = system.findItem(id);
                if (item) {
                    std::cout << "Item found:\n"
                              << "ID: " << item->getId() << "\n"
                              << "Name: " << item->getName() << "\n"
                              << "Category: " << item->getCategory() << "\n"
                              << "Quantity: " << item->getQuantity() << "\n"
                              << "Price: " << item->getPrice() << "\n"
                              << "Min Stock Level: " << item->getMinStockLevel() << "\n";
                } else {
                    std::cout << "Item not found!\n";
                }
                break;
            }
            case 5:  // Display All Items
                system.displayAllItems();
                break;
            case 6:  // Display Low Stock Items
                system.displayLowStockItems();
                break;
            case 7: {  // Display Items by Category
                std::string category;
                std::cout << "Enter category: ";
                std::getline(std::cin, category);
                system.displayByCategory(category);
                break;
            }
            case 8:  // Sort by Name
                system.sortByName();
                break;
            case 9:  // Sort by Quantity
                system.sortByQuantity();
                break;
            case 10: {  // Create Order
                int itemId, quantity;
                std::cout << "Enter item ID: ";
                std::cin >> itemId;
                std::cout << "Enter quantity: ";
                std::cin >> quantity;
                system.createOrder(itemId, quantity);
                break;
            }
            case 11:  // Process Next Order
                system.processNextOrder();
                break;
            case 12:  // Display Order Queue
                system.displayOrderQueue();
                break;
            case 13:  // Display Transaction History
                system.displayTransactionHistory();
                break;
            case 0:
                std::cout << "Thank you for using the Warehouse Management System!\n";
                break;
            default:
                std::cout << "Invalid choice! Please try again.\n";
        }
    } while (choice != 0);
    
    return 0;
}
