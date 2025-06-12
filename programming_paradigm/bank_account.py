class BankAccount:
    def __init__(self, initial_balance=0):
        """Initialize the BankAccount with an optional initial balance."""
        self.__account_balance = initial_balance

    def deposit(self, amount):
        """Add the amount to the account balance."""
        if amount > 0:
            self.__account_balance += amount
        else:
            print("Deposit amount must be positive.")

    def withdraw(self, amount):
        """Withdraw the amount from the account balance if sufficient funds exist."""
        if amount > self.__account_balance:
            return False
        elif amount <= 0:
            print("Withdraw amount must be positive.")
            return False
        else:
            self.__account_balance -= amount
            return True

    def display_balance(self):
        """Display the current balance."""
        print(f"Current Balance: ${self.__account_balance}")
