#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/exception.h>
using namespace std;
using namespace sql;

// Database connection details
const string DB_HOST = "tcp://127.0.0.1:3306";
const string DB_USER = "root"; // Change to your MySQL username
const string DB_PASS = "Aba71219@21"; // Change to your MySQL password
const string DB_NAME = "food_ordering_system";

// Global database connection
sql::Driver* driver;
sql::Connection* con;
int currentUserId = -1; // Global variable to track logged in user

struct FoodItem {
    int id;
    string name;
    double price;
    string description;
};

struct Restaurant {
    int id;
    string name;
    string cuisine;
};

struct User {
    int id;
    string username;
    string password;
    string address;
};

// Function prototypes
bool connectToDatabase();
void closeDatabaseConnection();
void initializeDatabase();
int estimateDeliveryTime();
void clearInput();
void displayMainMenu();
void registerUser();
int loginUser(); // Changed from bool to int
void browseRestaurants(bool isGuest);
void viewRestaurantMenu(int restaurantId);
void placeOrder(int userId, int restaurantId);
void viewOrderHistory(int userId);
void displayUserMenu();
void displayGuestMenu();

// Database connection
bool connectToDatabase() {
    try {
        driver = get_driver_instance();
        con = driver->connect(DB_HOST, DB_USER, DB_PASS);
        con->setSchema(DB_NAME);
        return true;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
        return false;
    }
}

void closeDatabaseConnection() {
    if (con) {
        delete con;
    }
}

void initializeDatabase() {
    try {
        sql::Statement *stmt = con->createStatement();
        
        // Create tables if they don't exist
        stmt->execute("CREATE TABLE IF NOT EXISTS users ("
                      "id INT AUTO_INCREMENT PRIMARY KEY, "
                      "username VARCHAR(50) NOT NULL UNIQUE, "
                      "password VARCHAR(100) NOT NULL, "
                      "address TEXT NOT NULL, "
                      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
        
        stmt->execute("CREATE TABLE IF NOT EXISTS restaurants ("
                      "id INT AUTO_INCREMENT PRIMARY KEY, "
                      "name VARCHAR(100) NOT NULL, "
                      "cuisine VARCHAR(50) NOT NULL)");
        
        stmt->execute("CREATE TABLE IF NOT EXISTS food_items ("
                      "id INT AUTO_INCREMENT PRIMARY KEY, "
                      "restaurant_id INT NOT NULL, "
                      "name VARCHAR(100) NOT NULL, "
                      "price DECIMAL(10,2) NOT NULL, "
                      "description TEXT, "
                      "FOREIGN KEY (restaurant_id) REFERENCES restaurants(id))");
        
        stmt->execute("CREATE TABLE IF NOT EXISTS orders ("
                      "id INT AUTO_INCREMENT PRIMARY KEY, "
                      "user_id INT NOT NULL, "
                      "restaurant_id INT NOT NULL, "
                      "order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                      "delivery_time INT, "
                      "total_amount DECIMAL(10,2) NOT NULL, "
                      "FOREIGN KEY (user_id) REFERENCES users(id), "
                      "FOREIGN KEY (restaurant_id) REFERENCES restaurants(id))");
        
        stmt->execute("CREATE TABLE IF NOT EXISTS order_items ("
                      "id INT AUTO_INCREMENT PRIMARY KEY, "
                      "order_id INT NOT NULL, "
                      "item_id INT NOT NULL, "
                      "quantity INT DEFAULT 1, "
                      "price_at_order DECIMAL(10,2) NOT NULL, "
                      "FOREIGN KEY (order_id) REFERENCES orders(id), "
                      "FOREIGN KEY (item_id) REFERENCES food_items(id))");
        
        // Check if we need to insert sample data
        sql::ResultSet *res = stmt->executeQuery("SELECT COUNT(*) FROM restaurants");
        res->next();
        if (res->getInt(1) == 0) {
            // Insert sample restaurants
            stmt->execute("INSERT INTO restaurants (name, cuisine) VALUES "
                          "('KK Restaurant', 'Ethiopian'), "
                          "('Kibnesh', 'Ethiopian'), "
                          "('Teachers Lounge', 'International'), "
                          "('Workers Cafe', 'Fast Food')");
            
            // Insert sample food items
            // KK Restaurant
            stmt->execute("INSERT INTO food_items (restaurant_id, name, price, description) VALUES "
                          "(1, 'Doro Wot', 12.99, 'Spicy chicken stew with boiled eggs'), "
                          "(1, 'Tibs', 10.99, 'Sautéed beef with onions and spices'), "
                          "(1, 'Shiro', 8.99, 'Chickpea flour stew with berbere spice')");
            
            // Kibnesh
            stmt->execute("INSERT INTO food_items (restaurant_id, name, price, description) VALUES "
                          "(2, 'Kitfo', 14.99, 'Minced beef with mitmita spice'), "
                          "(2, 'Firfir', 9.99, 'Shredded injera with spicy sauce'), "
                          "(2, 'Vegetable Combo', 11.99, 'Assorted vegetarian dishes')");
            
            // Teachers Lounge
            stmt->execute("INSERT INTO food_items (restaurant_id, name, price, description) VALUES "
                          "(3, 'Spaghetti Bolognese', 10.49, 'Classic Italian pasta with meat sauce'), "
                          "(3, 'Chicken Curry', 11.99, 'Indian-style curry with rice'), "
                          "(3, 'Greek Salad', 8.99, 'Fresh vegetables with feta cheese')");
            
            // Workers Cafe
            stmt->execute("INSERT INTO food_items (restaurant_id, name, price, description) VALUES "
                          "(4, 'Cheeseburger', 6.99, 'Classic beef burger with cheese'), "
                          "(4, 'Chicken Sandwich', 7.49, 'Crispy chicken fillet with mayo'), "
                          "(4, 'French Fries', 3.99, 'Golden crispy potatoes')");
        }
        
        delete res;
        delete stmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error during initialization: " << e.what() << endl;
    }
}

int estimateDeliveryTime() {
    return 20 + (rand() % 41);
}

void clearInput() {
    cin.clear();
    while (cin.get() != '\n');
}

void displayMainMenu() {
    cout << "\n===== Online Food Ordering System =====" << endl;
}

void displayGuestMenu() {
    displayMainMenu();
    cout << "1. Register\n2. Login\n3. Browse Restaurants (Guest)\n4. Exit\n";
}

void displayUserMenu() {
    displayMainMenu();
    try {
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;
        
        pstmt = con->prepareStatement("SELECT username FROM users WHERE id = ?");
        pstmt->setInt(1, currentUserId);
        res = pstmt->executeQuery();
        
        if (res->next()) {
            cout << "Logged in as: " << res->getString("username") << endl;
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
    cout << "1. Browse Restaurants\n2. View Order History\n3. Logout\n4. Exit\n";
}

void registerUser() {
    try {
        string username, password, address;
        cout << "Enter username: ";
        getline(cin, username);
        cout << "Enter password (at least 6 characters): ";
        getline(cin, password);
        
        if (password.length() < 6) {
            cout << "Password must be at least 6 characters long!" << endl;
            return;
        }
        
        cout << "Enter delivery address: ";
        getline(cin, address);
        
        sql::PreparedStatement *pstmt;
        pstmt = con->prepareStatement("INSERT INTO users (username, password, address) VALUES (?, ?, ?)");
        pstmt->setString(1, username);
        pstmt->setString(2, password);
        pstmt->setString(3, address);
        pstmt->executeUpdate();
        
        delete pstmt;
        cout << "Registration successful!" << endl;
    } catch (sql::SQLException &e) {
        if (e.getErrorCode() == 1062) { // Duplicate entry
            cout << "Username already exists!" << endl;
        } else {
            cerr << "SQL Error: " << e.what() << endl;
        }
    }
}

int loginUser() {
    try {
        string username, password;
        cout << "Enter username: ";
        getline(cin, username);
        cout << "Enter password: ";
        getline(cin, password);
        
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;
        
        pstmt = con->prepareStatement("SELECT id FROM users WHERE username = ? AND password = ?");
        pstmt->setString(1, username);
        pstmt->setString(2, password);
        res = pstmt->executeQuery();
        
        if (res->next()) {
            int userId = res->getInt("id");
            cout << "Login successful!" << endl;
            delete res;
            delete pstmt;
            return userId;
        } else {
            cout << "Invalid username or password!" << endl;
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
    return -1;
}

void browseRestaurants(bool isGuest) {
    try {
        sql::Statement *stmt;
        sql::ResultSet *res;
        
        stmt = con->createStatement();
        res = stmt->executeQuery("SELECT * FROM restaurants");
        
        cout << "\nAvailable Restaurants:" << endl;
        int count = 1;
        while (res->next()) {
            cout << count++ << ". " << res->getString("name") 
                 << " (" << res->getString("cuisine") << ")" << endl;
        }
        
        if (count == 1) {
            cout << "No restaurants available." << endl;
            delete res;
            delete stmt;
            return;
        }
        
        int restChoice;
        cout << "Select a restaurant (0 to cancel): ";
        cin >> restChoice;
        clearInput();
        
        if (restChoice > 0) {
            // Get the actual restaurant ID from the result set
            res->absolute(restChoice);
            int restaurantId = res->getInt("id");
            
            if (isGuest) {
                viewRestaurantMenu(restaurantId);
                cout << "\nPlease login or register to place an order." << endl;
            } else {
                placeOrder(currentUserId, restaurantId);
            }
        }
        
        delete res;
        delete stmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
}

void viewRestaurantMenu(int restaurantId) {
    try {
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;
        
        // Get restaurant info
        pstmt = con->prepareStatement("SELECT name, cuisine FROM restaurants WHERE id = ?");
        pstmt->setInt(1, restaurantId);
        res = pstmt->executeQuery();
        
        if (res->next()) {
            cout << "\n=== " << res->getString("name") << " (" 
                 << res->getString("cuisine") << ") Menu ===" << endl;
        }
        
        delete res;
        delete pstmt;
        
        // Get menu items
        pstmt = con->prepareStatement("SELECT * FROM food_items WHERE restaurant_id = ?");
        pstmt->setInt(1, restaurantId);
        res = pstmt->executeQuery();
        
        int count = 1;
        while (res->next()) {
            cout << count++ << ". " << res->getString("name")
                 << " - $" << res->getDouble("price") << "\n   "
                 << res->getString("description") << endl;
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
}
void placeOrder(int userId, int restaurantId) {
    try {
        viewRestaurantMenu(restaurantId);
        
        vector<FoodItem> cart;
        vector<int> quantities;
        
        while (true) {
            cout << "\nEnter item number to add to cart (0 to finish): ";
            int itemChoice;
            cin >> itemChoice;
            clearInput();
            
            if (itemChoice == 0) break;
            
            sql::PreparedStatement *pstmt;
            sql::ResultSet *res;
            
            pstmt = con->prepareStatement(
                "SELECT id, name, price, description FROM food_items "
                "WHERE restaurant_id = ? LIMIT 1 OFFSET ?");
            pstmt->setInt(1, restaurantId);
            pstmt->setInt(2, itemChoice - 1);
            res = pstmt->executeQuery();
            
            if (res->next()) {
                FoodItem item;
                item.id = res->getInt("id");
                item.name = res->getString("name");
                item.price = res->getDouble("price");
                item.description = res->getString("description");
                
                cout << "Enter quantity for " << item.name << ": ";
                int quantity;
                cin >> quantity;
                clearInput();
                
                if (quantity > 0) {
                    cart.push_back(item);
                    quantities.push_back(quantity);
                    cout << "Added " << quantity << " x " << item.name << " to cart." << endl;
                }
            } else {
                cout << "Invalid choice!" << endl;
            }
            
            delete res;
            delete pstmt;
        }
        
        if (!cart.empty()) {
            cout << "\n=== Your Order ===" << endl;
            double total = 0.0;
            for (size_t i = 0; i < cart.size(); ++i) {
                cout << quantities[i] << " x " << cart[i].name 
                     << " - $" << (cart[i].price * quantities[i]) << endl;
                total += cart[i].price * quantities[i];
            }
            cout << "Total: $" << total << endl;
            
            // Get user address
            string address;
            sql::PreparedStatement *pstmt;
            sql::ResultSet *res;
            
            pstmt = con->prepareStatement("SELECT address FROM users WHERE id = ?");
            pstmt->setInt(1, userId);
            res = pstmt->executeQuery();
            
            if (res->next()) {
                address = res->getString("address");
                cout << "Delivery to: " << address << endl;
            }
            
            delete res;
            delete pstmt;
            
            cout << "\nConfirm order? (1=Yes, 0=No): ";
            int confirm;
            cin >> confirm;
            clearInput();
            
            if (confirm == 1) {
                // Start transaction
                con->setAutoCommit(false);
                
                try {
                    // Insert order - modified for Connector/C++ 9.3
                    pstmt = con->prepareStatement(
                        "INSERT INTO orders (user_id, restaurant_id, delivery_time, total_amount) "
                        "VALUES (?, ?, ?, ?)");
                    pstmt->setInt(1, userId);
                    pstmt->setInt(2, restaurantId);
                    pstmt->setInt(3, estimateDeliveryTime());
                    pstmt->setDouble(4, total);
                    pstmt->executeUpdate();
                    
                    // Get the last insert ID - modified approach
                    sql::Statement *stmt = con->createStatement();
                    res = stmt->executeQuery("SELECT LAST_INSERT_ID()");
                    int orderId = -1;
                    if (res->next()) {
                        orderId = res->getInt(1);
                    }
                    delete res;
                    delete stmt;
                    
                    if (orderId == -1) {
                        throw sql::SQLException("Failed to get order ID");
                    }
                    
                    // Insert order items
                    for (size_t i = 0; i < cart.size(); i++) {
                        pstmt = con->prepareStatement(
                            "INSERT INTO order_items (order_id, item_id, quantity, price_at_order) "
                            "VALUES (?, ?, ?, ?)");
                        pstmt->setInt(1, orderId);
                        pstmt->setInt(2, cart[i].id);
                        pstmt->setInt(3, quantities[i]);
                        pstmt->setDouble(4, cart[i].price);
                        pstmt->executeUpdate();
                        delete pstmt;
                    }
                    
                    // Commit transaction
                    con->commit();
                    cout << "Order placed successfully! Order ID: " << orderId << endl;
                    cout << "Estimated delivery time: " << estimateDeliveryTime() << " minutes." << endl;
                } catch (sql::SQLException &e) {
                    con->rollback();
                    throw e;
                }
                
                con->setAutoCommit(true);
            } else {
                cout << "Order canceled." << endl;
            }
        }
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
}

void viewOrderHistory(int userId) {
    try {
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;
        
        pstmt = con->prepareStatement(
            "SELECT o.id, o.order_date, o.delivery_time, o.total_amount, "
            "r.name AS restaurant_name, r.cuisine "
            "FROM orders o JOIN restaurants r ON o.restaurant_id = r.id "
            "WHERE o.user_id = ? ORDER BY o.order_date DESC");
        pstmt->setInt(1, userId);
        res = pstmt->executeQuery();
        
        cout << "\n=== Order History ===" << endl;
        
        if (!res->next()) {
            cout << "No order history found." << endl;
        } else {
            do {
                cout << "\nOrder #" << res->getInt("id") << endl;
                cout << "Date: " << res->getString("order_date") << endl;
                cout << "Restaurant: " << res->getString("restaurant_name") 
                     << " (" << res->getString("cuisine") << ")" << endl;
                cout << "Delivery time: " << res->getInt("delivery_time") << " minutes" << endl;
                cout << "Total: $" << res->getDouble("total_amount") << endl;
                
                cout << "Items:" << endl;
                
                // Get order items
                sql::PreparedStatement *itemStmt;
                sql::ResultSet *itemRes;
                
                itemStmt = con->prepareStatement(
                    "SELECT i.name, oi.quantity, oi.price_at_order "
                    "FROM order_items oi JOIN food_items i ON oi.item_id = i.id "
                    "WHERE oi.order_id = ?");
                itemStmt->setInt(1, res->getInt("id"));
                itemRes = itemStmt->executeQuery();
                
                while (itemRes->next()) {
                    cout << "  - " << itemRes->getInt("quantity") << " x "
                         << itemRes->getString("name") << " ($"
                         << itemRes->getDouble("price_at_order") << " each)" << endl;
                }
                
                delete itemRes;
                delete itemStmt;
                
                cout << "---------------------" << endl;
            } while (res->next());
        }
        
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        cerr << "SQL Error: " << e.what() << endl;
    }
}

int main() {
    srand(time(0));
    
    if (!connectToDatabase()) {
        cerr << "Failed to connect to database. Exiting..." << endl;
        return 1;
    }
    
    initializeDatabase();
    
    while (true) {
        if (currentUserId == -1) {
            displayGuestMenu();
            
            int choice;
            cout << "Enter your choice: ";
            cin >> choice;
            clearInput();
            
            switch (choice) {
                case 1:
                    registerUser();
                    break;
                case 2:
                    currentUserId = loginUser();
                    break;
                case 3:
                    browseRestaurants(true);
                    break;
                case 4:
                    cout << "Thank you for visiting!" << endl;
                    closeDatabaseConnection();
                    return 0;
                default:
                    cout << "Invalid choice!" << endl;
            }
        } else {
            displayUserMenu();
            
            int choice;
            cout << "Enter your choice: ";
            cin >> choice;
            clearInput();
            
            switch (choice) {
                case 1:
                    browseRestaurants(false);
                    break;
                case 2:
                    viewOrderHistory(currentUserId);
                    break;
                case 3:
                    currentUserId = -1;
                    cout << "Logged out successfully." << endl;
                    break;
                case 4:
                    cout << "Thank you for using our service!" << endl;
                    closeDatabaseConnection();
                    return 0;
                default:
                    cout << "Invalid choice!" << endl;
            }
        }
    }
    
    closeDatabaseConnection();
    return 0;
}